/**
 * @file cv64_n64_parser.h
 * @brief CV64 N64 Display List Parser - Parse N64 geometry data
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#ifndef CV64_N64_PARSER_H
#define CV64_N64_PARSER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Vertex structure (N64 format)
typedef struct {
    int16_t x, y, z;      // Position
    uint16_t _padding;
    int16_t s, t;         // Texture coordinates
    uint8_t r, g, b, a;   // Color/Normal
} CV64_N64Vertex;

// Parsed geometry data
typedef struct {
    float* vertices;       // Vertex positions (x, y, z)
    float* normals;        // Vertex normals (x, y, z)
    float* texcoords;      // Texture coordinates (s, t)
    uint8_t* colors;       // Vertex colors (r, g, b, a)
    uint16_t* indices;     // Triangle indices
    
    uint32_t vertexCount;
    uint32_t indexCount;
    uint32_t triangleCount;
    
    // Bounding box
    float minX, minY, minZ;
    float maxX, maxY, maxZ;
} CV64_ParsedGeometry;

/**
 * @brief Parse N64 vertex data
 * @param data Raw vertex data from ROM
 * @param dataSize Size of data
 * @param outGeometry Output geometry structure
 * @return true if parsing succeeded
 */
bool CV64_ParseN64Vertices(const uint8_t* data, size_t dataSize, CV64_ParsedGeometry* outGeometry);

/**
 * @brief Parse N64 display list
 * @param data Raw display list data from ROM
 * @param dataSize Size of data
 * @param outGeometry Output geometry structure
 * @return true if parsing succeeded
 */
bool CV64_ParseN64DisplayList(const uint8_t* data, size_t dataSize, CV64_ParsedGeometry* outGeometry);

/**
 * @brief Free parsed geometry data
 * @param geometry Geometry structure to free
 */
void CV64_FreeGeometry(CV64_ParsedGeometry* geometry);

/**
 * @brief Create simple test geometry (fallback)
 * @param outGeometry Output geometry structure
 * @return true if succeeded
 */
bool CV64_CreateTestGeometry(CV64_ParsedGeometry* outGeometry);

#ifdef __cplusplus
}
#endif

#endif // CV64_N64_PARSER_H
