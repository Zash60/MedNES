package com.mednes.android

import android.app.Activity
import android.graphics.Bitmap
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.view.MotionEvent
import android.view.View
import android.widget.Button
import android.widget.ImageView
import android.widget.TextView
import java.io.File

class MainActivity : Activity() {
    private lateinit var screen: ImageView
    private lateinit var status: TextView
    private var emuBitmap: Bitmap? = null
    private var isRunning = false
    private val handler = Handler(Looper.getMainLooper())
    
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
        screen = findViewById(R.id.emulatorScreen)
        status = findViewById(R.id.statusText)
        
        emuBitmap = Bitmap.createBitmap(256, 240, Bitmap.Config.ARGB_8888)
        screen.setImageBitmap(emuBitmap)

        // --- MUDANÇA DE DIRETÓRIO ---
        // Usa o diretório privado do app no armazenamento externo.
        // O usuário deve colocar a ROM em: /Android/data/com.mednes.android/files/rom.nes
        val appDir = getExternalFilesDir(null)
        val romFile = File(appDir, "rom.nes")
        
        if (romFile.exists()) {
            if (MedNESJni.loadRom(romFile.absolutePath)) {
                isRunning = true
                status.visibility = View.GONE
                gameLoop()
            } else {
                status.text = "Erro: ROM inválida."
            }
        } else {
            status.text = "Arquivo não encontrado!\nColoque 'rom.nes' em:\n" + appDir?.absolutePath
            // Permite clicar no texto para tentar recarregar (útil se o usuário colocar o arquivo com o app aberto)
            status.setOnClickListener { 
                recreate() 
            }
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
    
    private fun setupBtn(id: Int, key: Int) {
        val btn = findViewById<Button>(id)
        btn?.setOnTouchListener { _, event ->
            if (event.action == MotionEvent.ACTION_DOWN) MedNESJni.sendInput(key, true)
            if (event.action == MotionEvent.ACTION_UP) MedNESJni.sendInput(key, false)
            true
        }
    }

    private fun gameLoop() {
        if (!isRunning) return
        MedNESJni.stepFrame(emuBitmap!!)
        screen.invalidate()
        handler.postDelayed({ gameLoop() }, 16)
    }
}
