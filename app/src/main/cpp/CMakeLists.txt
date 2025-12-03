cmake_minimum_required(VERSION 3.18.1)
project("mednes")

add_library(
        mednes
        SHARED
        native-lib.cpp
        Core/6502.cpp
        Core/APU.cpp          # <--- ADICIONADO: Necessário para o áudio funcionar
        Core/Controller.cpp
        Core/PPU.cpp
        Core/RAM.cpp
        Core/ROM.cpp
        Core/Mapper/CNROM.cpp
        Core/Mapper/Mapper.cpp
        Core/Mapper/MMC1.cpp
        Core/Mapper/NROM.cpp
        Core/Mapper/UnROM.cpp
)

target_include_directories(mednes PRIVATE Core)

find_library(log-lib log)
find_library(android-lib android)

# jnigraphics é necessário para manipular Bitmaps no C++
target_link_libraries(mednes ${log-lib} ${android-lib} jnigraphics)
