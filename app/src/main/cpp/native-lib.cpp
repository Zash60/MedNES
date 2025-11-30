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

// Mapping
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
    
    if (objRom != nullptr) {
        if(objCpu) delete objCpu; 
        if(objController) delete objController; 
        if(objPpu) delete objPpu; 
        if(objMapper) delete objMapper; 
        if(objRom) delete objRom;
        objRom = nullptr; objCpu = nullptr;
    }

    objRom = new MedNES::ROM();
    try {
        objRom->open(std::string(path));
    } catch (...) {
        LOGE("Erro ao abrir arquivo");
        env->ReleaseStringUTFChars(romPath, path);
        return false;
    }

    if (objRom->getPrgCode().empty()) {
        LOGE("ROM Vazia");
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
    
    // Executa emulação
    while (!objPpu->generateFrame) { 
        objCpu->step(); 
    }
    objPpu->generateFrame = false;

    void* pixels;
    if (AndroidBitmap_lockPixels(env, bitmap, &pixels) < 0) return;
    
    uint32_t* src = objPpu->buffer;
    uint32_t* dst = (uint32_t*)pixels;
    
    // Otimização de Loop: 256 * 240 = 61440 pixels
    // Trocamos R e B para corrigir as cores no Android
    for (int i = 0; i < 61440; ++i) {
        uint32_t c = src[i];
        // O formato MedNES parece ser 0xAABBGGRR ou 0xBBGGRRAA dependendo da endianness
        // Android usa ARGB (0xAARRGGBB).
        // Se o Mario estava azul, B e R estavam trocados.
        // (c & 0xFF00FF00) mantem Alpha e Green
        // ((c & 0x00FF0000) >> 16) move Blue para posição Red
        // ((c & 0x000000FF) << 16) move Red para posição Blue
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
