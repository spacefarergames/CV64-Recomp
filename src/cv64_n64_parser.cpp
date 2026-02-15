/**
 * @file cv64_n64_parser.cpp
 * @brief CV64 N64 Display List Parser Implementation
 */

#include "../include/cv64_n64_parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <windows.h>

// Helper: Byte swap for big-endian N64 data
static inline uint16_t swap16(uint16_t val) {
    return (val << 8) | (val >> 8);
}

static inline uint32_t swap32(uint32_t val) {
    return ((val << 24) & 0xFF000000) |
           ((val << 8)  & 0x00FF0000) |
           ((val >> 8)  & 0x0000FF00) |
           ((val >> 24) & 0x000000FF);
}

bool CV64_ParseN64Vertices(const uint8_t* data, size_t dataSize, CV64_ParsedGeometry* outGeometry) {
    if (!data || !outGeometry || dataSize < sizeof(CV64_N64Vertex)) {
        return false;
    }
    
    memset(outGeometry, 0, sizeof(CV64_ParsedGeometry));
    
    // Calculate number of vertices
    uint32_t vertexCount = (uint32_t)(dataSize / sizeof(CV64_N64Vertex));
    if (vertexCount > 1000) vertexCount = 1000; // Safety limit
    
    outGeometry->vertexCount = vertexCount;
    outGeometry->vertices = (float*)malloc(vertexCount * 3 * sizeof(float));
    outGeometry->normals = (float*)malloc(vertexCount * 3 * sizeof(float));
    outGeometry->texcoords = (float*)malloc(vertexCount * 2 * sizeof(float));
    outGeometry->colors = (uint8_t*)malloc(vertexCount * 4 * sizeof(uint8_t));
    
    if (!outGeometry->vertices || !outGeometry->normals || 
        !outGeometry->texcoords || !outGeometry->colors) {
        CV64_FreeGeometry(outGeometry);
        return false;
    }
    
    // Initialize bounding box
    outGeometry->minX = outGeometry->minY = outGeometry->minZ = FLT_MAX;
    outGeometry->maxX = outGeometry->maxY = outGeometry->maxZ = -FLT_MAX;
    
    // Parse vertices
    const CV64_N64Vertex* n64Verts = (const CV64_N64Vertex*)data;
    
    for (uint32_t i = 0; i < vertexCount; i++) {
        // Position (convert from N64 fixed-point to float)
        float x = (float)((int16_t)swap16(n64Verts[i].x)) / 32.0f;
        float y = (float)((int16_t)swap16(n64Verts[i].y)) / 32.0f;
        float z = (float)((int16_t)swap16(n64Verts[i].z)) / 32.0f;
        
        outGeometry->vertices[i * 3 + 0] = x;
        outGeometry->vertices[i * 3 + 1] = y;
        outGeometry->vertices[i * 3 + 2] = z;
        
        // Update bounding box
        if (x < outGeometry->minX) outGeometry->minX = x;
        if (y < outGeometry->minY) outGeometry->minY = y;
        if (z < outGeometry->minZ) outGeometry->minZ = z;
        if (x > outGeometry->maxX) outGeometry->maxX = x;
        if (y > outGeometry->maxY) outGeometry->maxY = y;
        if (z > outGeometry->maxZ) outGeometry->maxZ = z;
        
        // Texture coordinates (convert from N64 fixed-point)
        float s = (float)((int16_t)swap16(n64Verts[i].s)) / 32.0f;
        float t = (float)((int16_t)swap16(n64Verts[i].t)) / 32.0f;
        
        outGeometry->texcoords[i * 2 + 0] = s;
        outGeometry->texcoords[i * 2 + 1] = t;
        
        // Colors (can also be normals in some cases)
        outGeometry->colors[i * 4 + 0] = n64Verts[i].r;
        outGeometry->colors[i * 4 + 1] = n64Verts[i].g;
        outGeometry->colors[i * 4 + 2] = n64Verts[i].b;
        outGeometry->colors[i * 4 + 3] = n64Verts[i].a;
        
        // Normals (use colors as approximation for now)
        outGeometry->normals[i * 3 + 0] = (float)n64Verts[i].r / 127.5f - 1.0f;
        outGeometry->normals[i * 3 + 1] = (float)n64Verts[i].g / 127.5f - 1.0f;
        outGeometry->normals[i * 3 + 2] = (float)n64Verts[i].b / 127.5f - 1.0f;
    }
    
    // Create simple triangle list (assume triangles)
    outGeometry->triangleCount = vertexCount / 3;
    outGeometry->indexCount = vertexCount;
    outGeometry->indices = (uint16_t*)malloc(vertexCount * sizeof(uint16_t));
    
    if (outGeometry->indices) {
        for (uint32_t i = 0; i < vertexCount; i++) {
            outGeometry->indices[i] = (uint16_t)i;
        }
    }
    
    char logMsg[256];
    sprintf_s(logMsg, "[CV64] Parsed %u vertices, %u triangles\n", 
        vertexCount, outGeometry->triangleCount);
    OutputDebugStringA(logMsg);
    
    return true;
}

bool CV64_ParseN64DisplayList(const uint8_t* data, size_t dataSize, CV64_ParsedGeometry* outGeometry) {
    // For now, just try to parse as vertex data
    // TODO: Implement proper F3DEX2 display list parsing
    return CV64_ParseN64Vertices(data, dataSize, outGeometry);
}

void CV64_FreeGeometry(CV64_ParsedGeometry* geometry) {
    if (!geometry) {
        return;
    }
    
    if (geometry->vertices) free(geometry->vertices);
    if (geometry->normals) free(geometry->normals);
    if (geometry->texcoords) free(geometry->texcoords);
    if (geometry->colors) free(geometry->colors);
    if (geometry->indices) free(geometry->indices);
    
    memset(geometry, 0, sizeof(CV64_ParsedGeometry));
}

bool CV64_CreateTestGeometry(CV64_ParsedGeometry* outGeometry) {
    if (!outGeometry) {
        return false;
    }
    
    memset(outGeometry, 0, sizeof(CV64_ParsedGeometry));
    
    // Create a simple cube
    outGeometry->vertexCount = 8;
    outGeometry->triangleCount = 12;
    outGeometry->indexCount = 36;
    
    outGeometry->vertices = (float*)malloc(8 * 3 * sizeof(float));
    outGeometry->normals = (float*)malloc(8 * 3 * sizeof(float));
    outGeometry->colors = (uint8_t*)malloc(8 * 4 * sizeof(uint8_t));
    outGeometry->indices = (uint16_t*)malloc(36 * sizeof(uint16_t));
    
    if (!outGeometry->vertices || !outGeometry->normals || 
        !outGeometry->colors || !outGeometry->indices) {
        CV64_FreeGeometry(outGeometry);
        return false;
    }
    
    // Cube vertices
    float verts[] = {
        -1, -1,  1,  1, -1,  1,  1,  1,  1, -1,  1,  1, // Front
        -1, -1, -1, -1,  1, -1,  1,  1, -1,  1, -1, -1  // Back
    };
    memcpy(outGeometry->vertices, verts, sizeof(verts));
    
    // Simple normals
    for (int i = 0; i < 8; i++) {
        outGeometry->normals[i * 3 + 0] = 0.0f;
        outGeometry->normals[i * 3 + 1] = 0.0f;
        outGeometry->normals[i * 3 + 2] = 1.0f;
    }
    
    // Colors
    for (int i = 0; i < 8; i++) {
        outGeometry->colors[i * 4 + 0] = 255;
        outGeometry->colors[i * 4 + 1] = 255;
        outGeometry->colors[i * 4 + 2] = 255;
        outGeometry->colors[i * 4 + 3] = 255;
    }
    
    // Cube indices
    uint16_t indices[] = {
        0, 1, 2, 2, 3, 0, // Front
        4, 5, 6, 6, 7, 4, // Back
        3, 2, 6, 6, 5, 3, // Top
        0, 4, 7, 7, 1, 0, // Bottom
        0, 3, 5, 5, 4, 0, // Left
        1, 7, 6, 6, 2, 1  // Right
    };
    memcpy(outGeometry->indices, indices, sizeof(indices));
    
    outGeometry->minX = outGeometry->minY = outGeometry->minZ = -1.0f;
    outGeometry->maxX = outGeometry->maxY = outGeometry->maxZ = 1.0f;
    
    return true;
}
