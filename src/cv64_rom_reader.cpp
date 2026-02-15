/**
 * @file cv64_rom_reader.cpp
 * @brief CV64 ROM Reader Implementation
 */

#include "../include/cv64_rom_reader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

struct CV64_ROMFile_t {
    FILE* file;
    size_t size;
    char path[512];
};

CV64_ROMFile CV64_ROM_Open(const char* romPath) {
    if (!romPath) {
        return NULL;
    }
    
    CV64_ROMFile rom = (CV64_ROMFile)malloc(sizeof(struct CV64_ROMFile_t));
    if (!rom) {
        return NULL;
    }
    
    fopen_s(&rom->file, romPath, "rb");
    if (!rom->file) {
        free(rom);
        return NULL;
    }
    
    // Get file size
    fseek(rom->file, 0, SEEK_END);
    rom->size = ftell(rom->file);
    fseek(rom->file, 0, SEEK_SET);
    
    strcpy_s(rom->path, romPath);
    
    char logMsg[512];
    sprintf_s(logMsg, "[CV64] ROM opened: %s (size: %zu bytes)\n", romPath, rom->size);
    OutputDebugStringA(logMsg);
    
    return rom;
}

void CV64_ROM_Close(CV64_ROMFile rom) {
    if (!rom) {
        return;
    }
    
    if (rom->file) {
        fclose(rom->file);
    }
    
    free(rom);
}

size_t CV64_ROM_Read(CV64_ROMFile rom, uint32_t offset, void* buffer, size_t size) {
    if (!rom || !rom->file || !buffer) {
        return 0;
    }
    
    if (offset >= rom->size) {
        return 0;
    }
    
    fseek(rom->file, offset, SEEK_SET);
    size_t bytesRead = fread(buffer, 1, size, rom->file);
    
    return bytesRead;
}

size_t CV64_ROM_GetSize(CV64_ROMFile rom) {
    if (!rom) {
        return 0;
    }
    
    return rom->size;
}

bool CV64_ROM_IsValid(CV64_ROMFile rom) {
    return rom && rom->file && rom->size > 0;
}
