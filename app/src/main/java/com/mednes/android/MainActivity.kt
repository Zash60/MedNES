package com.mednes.android

import android.graphics.Bitmap
import android.os.Bundle
import android.util.Log
import android.view.MotionEvent
import android.view.View
import android.widget.Button
import android.widget.ImageView
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity
import java.io.File
import android.os.Environment

class MainActivity : AppCompatActivity() {

    private lateinit var screen: ImageView
    private lateinit var status: TextView
    private var emuBitmap: Bitmap? = null
    private var isRunning = false
    private var gameThread: Thread? = null
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        
        // Configuração de Tela Cheia (Immersive Sticky)
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
        
        // Cria o Bitmap
        emuBitmap = Bitmap.createBitmap(256, 240, Bitmap.Config.ARGB_8888)
        screen.setImageBitmap(emuBitmap)

        // --- IMPORTANTE: USO DE getExternalFilesDir ---
        // Isso evita erros de permissão no Android 10+.
        // O usuário deve colocar a ROM em: /sdcard/Android/data/com.mednes.android/files/rom.nes
        val appDir = getExternalFilesDir(null)
        val romFile = File(appDir, "rom.nes")
        
        Log.d("MedNES", "Procurando ROM em: ${romFile.absolutePath}")

        if (romFile.exists()) {
            Log.d("MedNES", "Arquivo encontrado. Tentando carregar...")
            if (MedNESJni.loadRom(romFile.absolutePath)) {
                Log.d("MedNES", "ROM carregada com sucesso no C++")
                status.visibility = View.GONE
                startGameLoop()
            } else {
                Log.e("MedNES", "Falha ao carregar ROM no C++")
                status.text = "Erro: Arquivo corrompido ou mapper não suportado."
            }
        } else {
            Log.e("MedNES", "Arquivo não encontrado")
            status.text = "Arquivo não encontrado!\nColoque 'rom.nes' em:\n${appDir?.absolutePath}"
        }

        setupControls()
    }
    
    private fun startGameLoop() {
        isRunning = true
        gameThread = Thread {
            while (isRunning) {
                val start = System.currentTimeMillis()
                
                // Roda o emulador (C++)
                MedNESJni.stepFrame(emuBitmap!!)
                
                // Solicita atualização da tela
                screen.postInvalidate()
                
                // Controle de FPS (aprox 60 FPS)
                val elapsed = System.currentTimeMillis() - start
                val wait = 16 - elapsed
                if (wait > 0) {
                    try { Thread.sleep(wait) } catch (e: Exception) {}
                }
            }
        }
        gameThread?.start()
    }

    override fun onDestroy() {
        super.onDestroy()
        isRunning = false
        try {
            gameThread?.join(1000)
        } catch (e: Exception) {
            e.printStackTrace()
        }
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
