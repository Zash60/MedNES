package com.mednes.android

import android.app.Activity
import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.PorterDuff
import android.os.Bundle
import android.view.MotionEvent
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.view.View
import android.widget.Button
import android.widget.TextView
import java.io.File
import java.util.concurrent.atomic.AtomicBoolean


class MainActivity : Activity(), SurfaceHolder.Callback {

    private lateinit var surfaceView: AspectSurfaceView  // <-- Change to custom class
    private lateinit var statusText: TextView
    private lateinit var fpsText: TextView
    
    private var emuBitmap: Bitmap? = null
    private val isRunning = AtomicBoolean(false)
    private var gameThread: Thread? = null
    
    // FPS
    private var fpsCounter = 0
    private var lastFpsTime = 0L

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        // Fullscreen Imersivo
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
        // --- O SEGREDO DA PERFORMANCE ---
        // Definimos o buffer interno para o tamanho nativo do NES.
        // O hardware do Android vai esticar isso para a tela cheia automaticamente e muito rápido.
        holder.setFixedSize(256, 240)
        
        val appDir = getExternalFilesDir(null)
        val romFile = File(appDir, "rom.nes")

        if (romFile.exists()) {
            if (MedNESJni.loadRom(romFile.absolutePath)) {
                statusText.visibility = View.GONE
                // ARGB_8888 é o mais rápido em arquiteturas ARM modernas
                emuBitmap = Bitmap.createBitmap(256, 240, Bitmap.Config.ARGB_8888)
                startGameLoop(holder)
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
        } catch (e: InterruptedException) {
            e.printStackTrace()
        }
    }

    private fun startGameLoop(holder: SurfaceHolder) {
    isRunning.set(true)
    gameThread = Thread {
        val paint = Paint()
        lastFpsTime = System.currentTimeMillis()
        
        // Prioridade máxima para render thread (crucial para 60 FPS)
        Thread.currentThread().priority = Thread.MAX_PRIORITY
        
        // Limiter preciso: 1/60s = 16.666ms em ns
        val targetFrameNs = 16_666_667L
        var nextFrameTimeNs = System.nanoTime()

        while (isRunning.get()) {
            val frameStartNs = System.nanoTime()

            // 1. Emulação nativa
            MedNESJni.stepFrame(emuBitmap!!)

            // 2. Render (hardware upscale rápido)
            val canvas = holder.lockCanvas()
            if (canvas != null) {
                canvas.drawBitmap(emuBitmap!!, 0f, 0f, paint)
                holder.unlockCanvasAndPost(canvas)
            }

            // 3. FPS counter (mantido igual)
            fpsCounter++
            val nowMs = System.currentTimeMillis()
            if (nowMs - lastFpsTime >= 1000) {
                val fps = fpsCounter
                fpsCounter = 0
                lastFpsTime = nowMs
                runOnUiThread { fpsText.text = "FPS: $fps" }
            }
            
            // 4. **NOVO: Limiter preciso com drift correction**
            val elapsedNs = System.nanoTime() - frameStartNs
            nextFrameTimeNs += targetFrameNs
            var sleepUntilNs = nextFrameTimeNs - System.nanoTime()
            
            // Correção de drift (acumula erro <1 frame)
            if (sleepUntilNs > targetFrameNs) sleepUntilNs = targetFrameNs
            
            // Sleep inteligente: ms principal + micro ajuste
            if (sleepUntilNs > 2_000_000L) {  // >2ms
                val sleepMs = (sleepUntilNs / 1_000_000L).toLong()
                try {
                    Thread.sleep(sleepMs)
                } catch (e: InterruptedException) { }
                sleepUntilNs -= sleepMs * 1_000_000L
            }
            
            // Busy-wait mínimo para sub-ms (evita CPU 100%, mas preciso)
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
