package com.mednes.android

import android.graphics.Bitmap
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.Rect
import android.os.Bundle
import android.view.MotionEvent
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.view.View
import android.widget.Button
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import java.io.File
import java.util.concurrent.atomic.AtomicBoolean

class MainActivity : AppCompatActivity(), SurfaceHolder.Callback {

    private lateinit var surfaceView: SurfaceView
    private lateinit var statusText: TextView
    private lateinit var fpsText: TextView
    
    private var emuBitmap: Bitmap? = null
    private val isRunning = AtomicBoolean(false)
    private var gameThread: Thread? = null
    
    // Variáveis do FPS
    private var fpsCounter = 0
    private var lastFpsTime = 0L

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        // Fullscreen
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
        // Inicializa o emulador quando a superfície é criada
        val appDir = getExternalFilesDir(null)
        val romFile = File(appDir, "rom.nes")

        if (romFile.exists()) {
            if (MedNESJni.loadRom(romFile.absolutePath)) {
                statusText.visibility = View.GONE
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
            val srcRect = Rect(0, 0, 256, 240)
            val dstRect = Rect()
            val paint = Paint(Paint.FILTER_BITMAP_FLAG) // Suaviza os pixels ao escalar
            
            lastFpsTime = System.currentTimeMillis()

            while (isRunning.get()) {
                val startTime = System.nanoTime()

                // 1. Roda o Core C++
                MedNESJni.stepFrame(emuBitmap!!)

                // 2. Desenha na tela (SurfaceView)
                val canvas = holder.lockCanvas()
                if (canvas != null) {
                    // Ajusta escala (Aspect Ratio)
                    val scale = Math.min(
                        canvas.width.toFloat() / 256f,
                        canvas.height.toFloat() / 240f
                    )
                    val w = (256 * scale).toInt()
                    val h = (240 * scale).toInt()
                    val x = (canvas.width - w) / 2
                    val y = (canvas.height - h) / 2
                    
                    dstRect.set(x, y, x + w, y + h)
                    
                    canvas.drawColor(Color.BLACK) // Limpa fundo
                    canvas.drawBitmap(emuBitmap!!, srcRect, dstRect, paint)
                    
                    holder.unlockCanvasAndPost(canvas)
                }

                // 3. Contador de FPS
                fpsCounter++
                val now = System.currentTimeMillis()
                if (now - lastFpsTime >= 1000) {
                    val fps = fpsCounter
                    fpsCounter = 0
                    lastFpsTime = now
                    runOnUiThread {
                        fpsText.text = "FPS: $fps"
                    }
                }
                
                // Limite de ~60 FPS (opcional, remove se quiser MAX performance)
                /*
                val frameTimeMs = (System.nanoTime() - startTime) / 1000000
                if (frameTimeMs < 16) {
                    try { Thread.sleep(16 - frameTimeMs) } catch (e: Exception) {}
                }
                */
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
