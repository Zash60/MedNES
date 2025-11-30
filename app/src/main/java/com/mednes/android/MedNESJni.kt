package com.mednes.android
import android.graphics.Bitmap
object MedNESJni {
    init { System.loadLibrary("mednes") }
    external fun loadRom(path: String): Boolean
    external fun stepFrame(bitmap: Bitmap)
    external fun sendInput(keyId: Int, pressed: Boolean)
}
