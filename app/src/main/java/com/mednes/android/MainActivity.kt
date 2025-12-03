package com.mednes.android

import android.app.Activity
import android.graphics.Bitmap
import android.graphics.Paint
import android.media.AudioAttributes
import android.media.AudioFormat
import android.media.AudioManager
import android.media.AudioTrack
import android.os.Build
import android.os.Bundle
import android.view.MotionEvent
import android.view.SurfaceHolder
import android.view.View
import android.widget.Button
import android.widget.TextView
import java.io.File
import java.util.concurrent.atomic.AtomicBoolean

class MainActivity : Activity(), SurfaceHolder.Callback {

    private lateinit var surfaceView: AspectSurfaceView
    private lateinit var statusText: TextView
    private lateinit var fpsText: TextView
    
    private var emuBitmap: Bitmap? = null
    private val isRunning = AtomicBoolean(false)
    private var gameThread: Thread? = null
    private var audioThread: Thread? = null // NOVA THREAD
    private var audioTrack: AudioTrack? = null // NOVO TRACK
    
    // FPS
    private var fpsCounter = 0
    private var lastFpsTime = 0L

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        window.decorView.systemUiVisibility = (
            View.SYSTEM_UI_FLAG_FULLSCREEN or
            View.SYSTEM_UI_FLAG_HIDE_NAVIGATION or
            View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY or
            View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION or
            View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN or
            View.SYSTEM_UI_FLAG_LAYOUT_STABLE
        )
        
        setContentView(R.layout.activity_main)

        surfaceView = findViewById(R.id.emulatorSurface)
        statusText = findViewById(R.id.statusText)
        fpsText = findViewById(R.id.fpsText)
        
        surfaceView.holder.addCallback(this)

        setupControls()
    }

    override fun surfaceCreated(holder: SurfaceHolder) {
        holder.setFixedSize(256, 240)
        
        val appDir = getExternalFilesDir(null)
        val romFile = File(appDir, "rom.nes")

        if (romFile.exists()) {
            if (MedNESJni.loadRom(romFile.absolutePath)) {
                statusText.visibility = View.GONE
                emuBitmap = Bitmap.createBitmap(256, 240, Bitmap.Config.ARGB_8888)
                startEmulator(holder)
            } else {
                statusText.text = "Failed to load ROM"
            }
        } else {
            statusText.text = "Place rom.nes in:\n${appDir?.absolutePath}"
        }
    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {}

    override fun surfaceDestroyed(holder: SurfaceHolder) {
        isRunning.set(false)
        try {
            gameThread?.join()
            audioThread?.join() // Join audio
        } catch (e: InterruptedException) {
            e.printStackTrace()
        }
    }

    private fun startEmulator(holder: SurfaceHolder) {
        isRunning.set(true)
        startAudio() // Inicia audio
        startGameLoop(holder)
    }

    private fun startAudio() {
        val sampleRate = 44100
        val bufferSize = AudioTrack.getMinBufferSize(
            sampleRate, 
            AudioFormat.CHANNEL_OUT_MONO, 
            AudioFormat.ENCODING_PCM_16BIT
        )

        audioTrack = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            AudioTrack.Builder()
                .setAudioAttributes(AudioAttributes.Builder()
                    .setUsage(AudioAttributes.USAGE_GAME)
                    .setContentType(AudioAttributes.CONTENT_TYPE_MUSIC)
                    .build())
                .setAudioFormat(AudioFormat.Builder()
                    .setEncoding(AudioFormat.ENCODING_PCM_16BIT)
                    .setSampleRate(sampleRate)
                    .setChannelMask(AudioFormat.CHANNEL_OUT_MONO)
                    .build())
                .setBufferSizeInBytes(bufferSize)
                .setTransferMode(AudioTrack.MODE_STREAM)
                .build()
        } else {
            AudioTrack(
                AudioManager.STREAM_MUSIC,
                sampleRate,
                AudioFormat.CHANNEL_OUT_MONO,
                AudioFormat.ENCODING_PCM_16BIT,
                bufferSize,
                AudioTrack.MODE_STREAM
            )
        }

        audioTrack?.play()

        audioThread = Thread {
            val audioBuffer = ShortArray(1024)
            while (isRunning.get()) {
                // Pega samples do C++
                val samplesRead = MedNESJni.getAudioSamples(audioBuffer)
                if (samplesRead > 0) {
                    // Escreve no hardware
                    audioTrack?.write(audioBuffer, 0, samplesRead)
                } else {
                    // Se não tiver som, dorme pouco pra não fritar CPU
                    try { Thread.sleep(2) } catch (e: Exception) {}
                }
            }
            audioTrack?.stop()
            audioTrack?.release()
        }
        audioThread?.start()
    }

    private fun startGameLoop(holder: SurfaceHolder) {
        gameThread = Thread {
            val paint = Paint()
            lastFpsTime = System.currentTimeMillis()
            Thread.currentThread().priority = Thread.MAX_PRIORITY
            
            val targetFrameNs = 16_666_667L
            var nextFrameTimeNs = System.nanoTime()

            while (isRunning.get()) {
                val frameStartNs = System.nanoTime()

                MedNESJni.stepFrame(emuBitmap!!)

                val canvas = holder.lockCanvas()
                if (canvas != null) {
                    canvas.drawBitmap(emuBitmap!!, 0f, 0f, paint)
                    holder.unlockCanvasAndPost(canvas)
                }

                fpsCounter++
                val nowMs = System.currentTimeMillis()
                if (nowMs - lastFpsTime >= 1000) {
                    val fps = fpsCounter
                    fpsCounter = 0
                    lastFpsTime = nowMs
                    runOnUiThread { fpsText.text = "FPS: $fps" }
                }
                
                val elapsedNs = System.nanoTime() - frameStartNs
                nextFrameTimeNs += targetFrameNs
                var sleepUntilNs = nextFrameTimeNs - System.nanoTime()
                
                if (sleepUntilNs > targetFrameNs) sleepUntilNs = targetFrameNs
                
                if (sleepUntilNs > 2_000_000L) {
                    val sleepMs = (sleepUntilNs / 1_000_000L).toLong()
                    try {
                        Thread.sleep(sleepMs)
                    } catch (e: InterruptedException) { }
                    sleepUntilNs -= sleepMs * 1_000_000L
                }
                
                while (sleepUntilNs > 0 && isRunning.get()) {
                    Thread.yield()
                    sleepUntilNs = nextFrameTimeNs - System.nanoTime()
                }
            }
        }
        gameThread?.start()
    }

    private fun setupControls() {
        setupBtn(R.id.btnA, 0)
        setupBtn(R.id.btnB, 1)
        setupBtn(R.id.btnSelect, 2)
        setupBtn(R.id.btnStart, 3)
        setupBtn(R.id.btnUp, 4)
        setupBtn(R.id.btnDown, 5)
        setupBtn(R.id.btnLeft, 6)
        setupBtn(R.id.btnRight, 7)
    }

    private fun setupBtn(id: Int, key: Int) {
        findViewById<Button>(id)?.setOnTouchListener { _, event ->
            if (event.action == MotionEvent.ACTION_DOWN) MedNESJni.sendInput(key, true)
            if (event.action == MotionEvent.ACTION_UP) MedNESJni.sendInput(key, false)
            true
        }
    }
}
