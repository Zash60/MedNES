#include <jni.h>
#include <string>
#include <android/log.h>
#include <android/bitmap.h>
#include <cstring>
#include "Core/6502.hpp"
#include "Core/Controller.hpp"
#include "Core/PPU.hpp"
#include "Core/ROM.hpp"

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "MedNES", __VA_ARGS__)

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
    
    // Limpeza completa para evitar crash ao recarregar
    if (objRom) {
        if(objCpu) { delete objCpu; objCpu = nullptr; }
        if(objController) { delete objController; objController = nullptr; }
        if(objPpu) { delete objPpu; objPpu = nullptr; }
        if(objMapper) { delete objMapper; objMapper = nullptr; }
        delete objRom; objRom = nullptr;
    }

    objRom = new MedNES::ROM();
    try {
        objRom->open(std::string(path));
    } catch (...) {
        env->ReleaseStringUTFChars(romPath, path);
        return false;
    }

    if (objRom->getPrgCode().empty()) {
        env->ReleaseStringUTFChars(romPath, path);
        return false;
    }

    objMapper = objRom->getMapper();
    if (!objMapper) {
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
    
    // Avança a CPU até que a PPU sinalize o fim do frame (VBlank)
    while (!objPpu->generateFrame) { 
        objCpu->step(); 
    }
    objPpu->generateFrame = false;

    void* pixels;
    if (AndroidBitmap_lockPixels(env, bitmap, &pixels) < 0) return;
    
    uint32_t* src = objPpu->buffer;
    uint32_t* dst = (uint32_t*)pixels;
    
    // Loop otimizado para 60 FPS
    // 256 * 240 = 61440 pixels
    // Conversão de formato de cor ABGR (interno) para ARGB (Android)
    for (int i = 0; i < 61440; ++i) {
        uint32_t c = src[i];
        dst[i] = (c & 0xFF00FF00) | ((c & 0x00FF0000) >> 16) | ((c & 0x000000FF) << 16);
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
