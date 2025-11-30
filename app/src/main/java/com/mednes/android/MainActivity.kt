package com.mednes.android

import android.app.Activity
import android.graphics.Bitmap
import android.os.Bundle
import android.view.MotionEvent
import android.view.View
import android.widget.Button
import android.widget.ImageView
import android.widget.TextView
import java.io.File
import android.os.Environment

class MainActivity : Activity() {
    private lateinit var screen: ImageView
    private lateinit var status: TextView
    private var emuBitmap: Bitmap? = null
    
    // Controle da Thread
    private var isRunning = false
    private var emuThread: Thread? = null
    
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

        screen = findViewById(R.id.emulatorScreen)
        status = findViewById(R.id.statusText)
        
        // Config.ARGB_8888 é padrão e rápido no Android
        emuBitmap = Bitmap.createBitmap(256, 240, Bitmap.Config.ARGB_8888)
        screen.setImageBitmap(emuBitmap)

        val downloadDir = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS)
        val romFile = File(downloadDir, "rom.nes")
        
        if (romFile.exists()) {
            if (MedNESJni.loadRom(romFile.absolutePath)) {
                status.visibility = View.GONE
                startEmulator()
            } else {
                status.text = "Failed to load ROM"
            }
        } else {
            status.text = "Place rom.nes in Downloads folder"
        }

        setupBtn(R.id.btnA, 0)
        setupBtn(R.id.btnB, 1)
        setupBtn(R.id.btnSelect, 2)
        setupBtn(R.id.btnStart, 3)
        setupBtn(R.id.btnUp, 4)
        setupBtn(R.id.btnDown, 5)
        setupBtn(R.id.btnLeft, 6)
        setupBtn(R.id.btnRight, 7)
    }

    private fun startEmulator() {
        isRunning = true
        emuThread = Thread {
            while (isRunning) {
                val startTime = System.currentTimeMillis()

                // 1. Roda um frame no C++ e atualiza o Bitmap
                MedNESJni.stepFrame(emuBitmap!!)
                
                // 2. Avisa a tela para redesenhar (postInvalidate é seguro para chamar de outra thread)
                screen.postInvalidate()

                // 3. Controle simples de FPS (Tenta manter ~60 FPS)
                val frameTime = System.currentTimeMillis() - startTime
                if (frameTime < 16) {
                    try {
                        Thread.sleep(16 - frameTime)
                    } catch (e: InterruptedException) {
                        break
                    }
                }
            }
        }
        emuThread?.start()
    }

    override fun onDestroy() {
        super.onDestroy()
        isRunning = false
        try {
            emuThread?.join(500)
        } catch (e: Exception) {}
    }
    
    private fun setupBtn(id: Int, key: Int) {
        val btn = findViewById<Button>(id)
        btn?.setOnTouchListener { _, event ->
            if (event.action == MotionEvent.ACTION_DOWN) MedNESJni.sendInput(key, true)
            if (event.action == MotionEvent.ACTION_UP) MedNESJni.sendInput(key, false)
            true
        }
    }
}
