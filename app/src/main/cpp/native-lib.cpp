#include <jni.h>
#include <string>
#include <android/log.h>
#include <android/bitmap.h>
#include <cstring>
#include "Core/6502.hpp"
#include "Core/Controller.hpp"
#include "Core/PPU.hpp"
#include "Core/ROM.hpp"

#define LOG_TAG "MedNES_Native"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Mapeamento de botões
#define KEY_A 97
#define KEY_B 98
#define KEY_SELECT 32
#define KEY_START 13
#define KEY_UP 1073741906
#define KEY_DOWN 1073741905
#define KEY_LEFT 1073741904
#define KEY_RIGHT 1073741903

MedNES::ROM* objRom = nullptr;
MedNES::Mapper* objMapper = nullptr;
MedNES::PPU* objPpu = nullptr;
MedNES::Controller* objController = nullptr;
MedNES::CPU6502* objCpu = nullptr;

extern "C" JNIEXPORT jboolean JNICALL
Java_com_mednes_android_MedNESJni_loadRom(JNIEnv* env, jobject, jstring romPath) {
    const char *path = env->GetStringUTFChars(romPath, 0);
    
    // Limpeza segura
    if (objCpu) { delete objCpu; objCpu = nullptr; }
    if (objController) { delete objController; objController = nullptr; }
    if (objPpu) { delete objPpu; objPpu = nullptr; }
    if (objMapper) { delete objMapper; objMapper = nullptr; }
    if (objRom) { delete objRom; objRom = nullptr; }

    objRom = new MedNES::ROM();
    
    try {
        objRom->open(std::string(path));
    } catch (...) {
        LOGE("Falha ao abrir arquivo: %s", path);
        env->ReleaseStringUTFChars(romPath, path);
        return false;
    }

    if (objRom->getPrgCode().empty()) {
        LOGE("ROM vazia ou erro de leitura.");
        env->ReleaseStringUTFChars(romPath, path);
        return false;
    }

    objMapper = objRom->getMapper();
    if (!objMapper) {
        LOGE("Mapper não suportado.");
        env->ReleaseStringUTFChars(romPath, path);
        return false;
    }

    objPpu = new MedNES::PPU(objMapper);
    objController = new MedNES::Controller();
    objCpu = new MedNES::CPU6502(objMapper, objPpu, objController);
    objCpu->reset();

    env->ReleaseStringUTFChars(romPath, path);
    return true;
}

extern "C" JNIEXPORT void JNICALL
Java_com_mednes_android_MedNESJni_stepFrame(JNIEnv* env, jobject, jobject bitmap) {
    if (!objCpu || !objPpu) return;
    
    // Executa o emulador até completar um frame
    while (!objPpu->generateFrame) { 
        objCpu->step(); 
    }
    objPpu->generateFrame = false;

    // Trava o bitmap do Android para escrever os pixels
    void* pixels;
    if (AndroidBitmap_lockPixels(env, bitmap, &pixels) < 0) return;

    uint32_t* src = objPpu->buffer;
    uint32_t* dst = (uint32_t*)pixels;
    int count = 256 * 240;

    // --- CORREÇÃO DE CORES E PERFORMANCE ---
    // Converte de ABGR (MedNES) para ARGB (Android) ou vice-versa trocando R e B
    for (int i = 0; i < count; ++i) {
        uint32_t color = src[i];
        // Formato: 0xAABBGGRR -> 0xAARRGGBB
        // Mantém Alpha (FF000000) e Verde (0000FF00)
        // Troca Azul (00FF0000) com Vermelho (000000FF)
        dst[i] = (color & 0xFF00FF00) | ((color & 0x00FF0000) >> 16) | ((color & 0x000000FF) << 16);
    }

    AndroidBitmap_unlockPixels(env, bitmap);
}

extern "C" JNIEXPORT void JNICALL
Java_com_mednes_android_MedNESJni_sendInput(JNIEnv* env, jobject, jint keyId, jboolean pressed) {
    if (!objController) return;
    int k = 0;
    switch(keyId) {
        case 0: k = KEY_A; break;
        case 1: k = KEY_B; break;
        case 2: k = KEY_SELECT; break;
        case 3: k = KEY_START; break;
        case 4: k = KEY_UP; break;
        case 5: k = KEY_DOWN; break;
        case 6: k = KEY_LEFT; break;
        case 7: k = KEY_RIGHT; break;
    }
    if (k != 0) objController->setButtonPressed(k, pressed);
}
