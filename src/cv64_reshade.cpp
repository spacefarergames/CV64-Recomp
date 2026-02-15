/**
 * @file cv64_reshade.cpp
 * @brief Castlevania 64 PC Recomp - ReShade Static Integration
 * 
 * Integrates ReShade post-processing framework statically into the executable.
 * This implementation:
 * - Disables ReShade's built-in UI overlay
 * - Disables auto-update checking
 * - Uses cv64_postprocessing.ini from patches folder for configuration
 * - Provides a simplified API for effect control
 * 
 * Based on ReShade by crosire (BSD-3-Clause license)
 * https://github.com/crosire/reshade
 * 
 * @copyright 2024 CV64 Recomp Team
 */

#include "../include/cv64_reshade.h"
#include <Windows.h>
#include <gl/GL.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <filesystem>

#pragma comment(lib, "opengl32.lib")

/*===========================================================================
 * OpenGL Type Definitions (not in basic gl.h)
 *===========================================================================*/

typedef char GLchar;
typedef ptrdiff_t GLsizeiptr;
typedef ptrdiff_t GLintptr;

/*===========================================================================
 * OpenGL Extension Function Pointers
 *===========================================================================*/

/* Framebuffer extensions */
typedef void (APIENTRY *PFNGLGENFRAMEBUFFERSPROC)(GLsizei n, GLuint *framebuffers);
typedef void (APIENTRY *PFNGLDELETEFRAMEBUFFERSPROC)(GLsizei n, const GLuint *framebuffers);
typedef void (APIENTRY *PFNGLBINDFRAMEBUFFERPROC)(GLenum target, GLuint framebuffer);
typedef void (APIENTRY *PFNGLFRAMEBUFFERTEXTURE2DPROC)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef GLenum (APIENTRY *PFNGLCHECKFRAMEBUFFERSTATUSPROC)(GLenum target);

/* Texture extensions */
typedef void (APIENTRY *PFNGLACTIVETEXTUREPROC)(GLenum texture);
typedef void (APIENTRY *PFNGLGENERATEMIPMAPPROC)(GLenum target);

/* Shader extensions */
typedef GLuint (APIENTRY *PFNGLCREATESHADERPROC)(GLenum type);
typedef void (APIENTRY *PFNGLDELETESHADERPROC)(GLuint shader);
typedef void (APIENTRY *PFNGLSHADERSOURCEPROC)(GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
typedef void (APIENTRY *PFNGLCOMPILESHADERPROC)(GLuint shader);
typedef void (APIENTRY *PFNGLGETSHADERIVPROC)(GLuint shader, GLenum pname, GLint *params);
typedef void (APIENTRY *PFNGLGETSHADERINFOLOGPROC)(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);

typedef GLuint (APIENTRY *PFNGLCREATEPROGRAMPROC)(void);
typedef void (APIENTRY *PFNGLDELETEPROGRAMPROC)(GLuint program);
typedef void (APIENTRY *PFNGLATTACHSHADERPROC)(GLuint program, GLuint shader);
typedef void (APIENTRY *PFNGLLINKPROGRAMPROC)(GLuint program);
typedef void (APIENTRY *PFNGLUSEPROGRAMPROC)(GLuint program);
typedef void (APIENTRY *PFNGLGETPROGRAMIVPROC)(GLuint program, GLenum pname, GLint *params);
typedef void (APIENTRY *PFNGLGETPROGRAMINFOLOGPROC)(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef GLint (APIENTRY *PFNGLGETUNIFORMLOCATIONPROC)(GLuint program, const GLchar *name);
typedef void (APIENTRY *PFNGLUNIFORM1IPROC)(GLint location, GLint v0);
typedef void (APIENTRY *PFNGLUNIFORM1FPROC)(GLint location, GLfloat v0);
typedef void (APIENTRY *PFNGLUNIFORM2FPROC)(GLint location, GLfloat v0, GLfloat v1);
typedef void (APIENTRY *PFNGLUNIFORM3FPROC)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
typedef void (APIENTRY *PFNGLUNIFORM4FPROC)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);

/* VAO/VBO extensions */
typedef void (APIENTRY *PFNGLGENVERTEXARRAYSPROC)(GLsizei n, GLuint *arrays);
typedef void (APIENTRY *PFNGLDELETEVERTEXARRAYSPROC)(GLsizei n, const GLuint *arrays);
typedef void (APIENTRY *PFNGLBINDVERTEXARRAYPROC)(GLuint array);
typedef void (APIENTRY *PFNGLGENBUFFERSPROC)(GLsizei n, GLuint *buffers);
typedef void (APIENTRY *PFNGLDELETEBUFFERSPROC)(GLsizei n, const GLuint *buffers);
typedef void (APIENTRY *PFNGLBINDBUFFERPROC)(GLenum target, GLuint buffer);
typedef void (APIENTRY *PFNGLBUFFERDATAPROC)(GLenum target, GLsizeiptr size, const void *data, GLenum usage);
typedef void (APIENTRY *PFNGLVERTEXATTRIBPOINTERPROC)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
typedef void (APIENTRY *PFNGLENABLEVERTEXATTRIBARRAYPROC)(GLuint index);

/* Read framebuffer extension */
#define GL_READ_FRAMEBUFFER         0x8CA8
#define GL_DRAW_FRAMEBUFFER         0x8CA9
typedef void (APIENTRY *PFNGLBLITFRAMEBUFFERPROC)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);

/* Extension constants */
#define GL_FRAMEBUFFER              0x8D40
#define GL_COLOR_ATTACHMENT0        0x8CE0
#define GL_DEPTH_ATTACHMENT         0x8D00
#define GL_DEPTH_COMPONENT24        0x81A6
#define GL_FRAMEBUFFER_COMPLETE     0x8CD5
#define GL_TEXTURE0                 0x84C0
#define GL_FRAGMENT_SHADER          0x8B30
#define GL_VERTEX_SHADER            0x8B31
#define GL_COMPILE_STATUS           0x8B81
#define GL_LINK_STATUS              0x8B82
#define GL_ARRAY_BUFFER             0x8892
#define GL_STATIC_DRAW              0x88E4
#define GL_CLAMP_TO_EDGE            0x812F

/* Additional constants for state save/restore */
#define GL_CURRENT_PROGRAM          0x8B8D
#define GL_VERTEX_ARRAY_BINDING     0x85B5
#define GL_DRAW_FRAMEBUFFER_BINDING 0x8CA6
#define GL_READ_FRAMEBUFFER_BINDING 0x8CAA
#define GL_ACTIVE_TEXTURE           0x84E0
#define GL_BLEND_SRC_ALPHA          0x80CB
#define GL_BLEND_DST_ALPHA          0x80CA

/* Extension function pointers */
static PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers = NULL;
static PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers = NULL;
static PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer = NULL;
static PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D = NULL;
static PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus = NULL;
static PFNGLBLITFRAMEBUFFERPROC glBlitFramebuffer = NULL;
static PFNGLACTIVETEXTUREPROC glActiveTexture = NULL;
static PFNGLGENERATEMIPMAPPROC glGenerateMipmap = NULL;
static PFNGLCREATESHADERPROC glCreateShader = NULL;
static PFNGLDELETESHADERPROC glDeleteShader = NULL;
static PFNGLSHADERSOURCEPROC glShaderSource = NULL;
static PFNGLCOMPILESHADERPROC glCompileShader = NULL;
static PFNGLGETSHADERIVPROC glGetShaderiv = NULL;
static PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = NULL;
static PFNGLCREATEPROGRAMPROC glCreateProgram = NULL;
static PFNGLDELETEPROGRAMPROC glDeleteProgram = NULL;
static PFNGLATTACHSHADERPROC glAttachShader = NULL;
static PFNGLLINKPROGRAMPROC glLinkProgram = NULL;
static PFNGLUSEPROGRAMPROC glUseProgram = NULL;
static PFNGLGETPROGRAMIVPROC glGetProgramiv = NULL;
static PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = NULL;
static PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation = NULL;
static PFNGLUNIFORM1IPROC glUniform1i = NULL;
static PFNGLUNIFORM1FPROC glUniform1f = NULL;
static PFNGLUNIFORM2FPROC glUniform2f = NULL;
static PFNGLUNIFORM3FPROC glUniform3f = NULL;
static PFNGLUNIFORM4FPROC glUniform4f = NULL;
static PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = NULL;
static PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays = NULL;
static PFNGLBINDVERTEXARRAYPROC glBindVertexArray = NULL;
static PFNGLGENBUFFERSPROC glGenBuffers = NULL;
static PFNGLDELETEBUFFERSPROC glDeleteBuffers = NULL;
static PFNGLBINDBUFFERPROC glBindBuffer = NULL;
static PFNGLBUFFERDATAPROC glBufferData = NULL;
static PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = NULL;
static PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = NULL;

/*===========================================================================
 * Static State
 *===========================================================================*/

static cv64_reshade_config_t s_config = { 0 };
static cv64_reshade_state_t s_state = { 0 };

static HWND s_hwnd = NULL;
static HGLRC s_hglrc = NULL;
static HDC s_hdc = NULL;

/* Shader programs for each effect */
static GLuint s_fxaa_program = 0;
static GLuint s_sharpen_program = 0;
static GLuint s_vibrance_program = 0;
static GLuint s_vignette_program = 0;
static GLuint s_filmgrain_program = 0;
static GLuint s_scanlines_program = 0;
static GLuint s_bloom_program = 0;
static GLuint s_colorgrade_program = 0;

/* Fullscreen quad VAO/VBO */
static GLuint s_quad_vao = 0;
static GLuint s_quad_vbo = 0;

/* FBO for ping-pong rendering */
static GLuint s_fbo[2] = { 0, 0 };
static GLuint s_fbo_texture[2] = { 0, 0 };
static int s_current_fbo = 0;

/* Performance timing */
static LARGE_INTEGER s_freq;
static LARGE_INTEGER s_start_time;

/*===========================================================================
 * Logging
 *===========================================================================*/

static void ReShadeLog(const char* msg) {
    OutputDebugStringA("[CV64_ReShade] ");
    OutputDebugStringA(msg);
    OutputDebugStringA("\n");
}

static void ReShadeLogf(const char* fmt, ...) {
    char buffer[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    ReShadeLog(buffer);
}

/*===========================================================================
 * OpenGL Extension Loading
 *===========================================================================*/

static bool LoadGLExtensions(void) {
    /* Load all required GL extensions */
    #define LOAD_GL_EXT(name) \
        name = (decltype(name))wglGetProcAddress(#name); \
        if (!name) { ReShadeLogf("Failed to load: %s", #name); return false; }
    
    LOAD_GL_EXT(glGenFramebuffers);
    LOAD_GL_EXT(glDeleteFramebuffers);
    LOAD_GL_EXT(glBindFramebuffer);
    LOAD_GL_EXT(glFramebufferTexture2D);
    LOAD_GL_EXT(glCheckFramebufferStatus);
    LOAD_GL_EXT(glActiveTexture);
    LOAD_GL_EXT(glGenerateMipmap);
    LOAD_GL_EXT(glCreateShader);
    LOAD_GL_EXT(glDeleteShader);
    LOAD_GL_EXT(glShaderSource);
    LOAD_GL_EXT(glCompileShader);
    LOAD_GL_EXT(glGetShaderiv);
    LOAD_GL_EXT(glGetShaderInfoLog);
    LOAD_GL_EXT(glCreateProgram);
    LOAD_GL_EXT(glDeleteProgram);
    LOAD_GL_EXT(glAttachShader);
    LOAD_GL_EXT(glLinkProgram);
    LOAD_GL_EXT(glUseProgram);
    LOAD_GL_EXT(glGetProgramiv);
    LOAD_GL_EXT(glGetProgramInfoLog);
    LOAD_GL_EXT(glGetUniformLocation);
    LOAD_GL_EXT(glUniform1i);
    LOAD_GL_EXT(glUniform1f);
    LOAD_GL_EXT(glUniform2f);
    LOAD_GL_EXT(glUniform3f);
    LOAD_GL_EXT(glUniform4f);
    LOAD_GL_EXT(glGenVertexArrays);
    LOAD_GL_EXT(glDeleteVertexArrays);
    LOAD_GL_EXT(glBindVertexArray);
    LOAD_GL_EXT(glGenBuffers);
    LOAD_GL_EXT(glDeleteBuffers);
    LOAD_GL_EXT(glBindBuffer);
    LOAD_GL_EXT(glBufferData);
    LOAD_GL_EXT(glVertexAttribPointer);
    LOAD_GL_EXT(glEnableVertexAttribArray);
    LOAD_GL_EXT(glBlitFramebuffer);
    
    #undef LOAD_GL_EXT
    
    ReShadeLog("All GL extensions loaded successfully");
    return true;
}

/*===========================================================================
 * Shader Compilation Helpers
 *===========================================================================*/

static GLuint CompileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        char log[512];
        glGetShaderInfoLog(shader, sizeof(log), NULL, log);
        ReShadeLogf("Shader compile error: %s", log);
        glDeleteShader(shader);
        return 0;
    }
    
    return shader;
}

static GLuint CreateProgram(const char* vs_source, const char* fs_source) {
    GLuint vs = CompileShader(GL_VERTEX_SHADER, vs_source);
    if (!vs) return 0;
    
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, fs_source);
    if (!fs) {
        glDeleteShader(vs);
        return 0;
    }
    
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    
    glDeleteShader(vs);
    glDeleteShader(fs);
    
    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (!status) {
        char log[512];
        glGetProgramInfoLog(program, sizeof(log), NULL, log);
        ReShadeLogf("Program link error: %s", log);
        glDeleteProgram(program);
        return 0;
    }
    
    return program;
}

/*===========================================================================
 * Shader Source Code
 *===========================================================================*/

/* Common vertex shader for fullscreen quad */
static const char* QUAD_VS = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;
out vec2 TexCoord;

void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";

/* FXAA Fragment Shader */
static const char* FXAA_FS = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D screenTexture;
uniform vec2 texelSize;
uniform float fxaaQuality;

#define FXAA_REDUCE_MIN (1.0/128.0)
#define FXAA_REDUCE_MUL (1.0/8.0)
#define FXAA_SPAN_MAX 8.0

void main() {
    vec2 uv = TexCoord;
    vec4 rgbNW = texture(screenTexture, uv + vec2(-1.0, -1.0) * texelSize);
    vec4 rgbNE = texture(screenTexture, uv + vec2(1.0, -1.0) * texelSize);
    vec4 rgbSW = texture(screenTexture, uv + vec2(-1.0, 1.0) * texelSize);
    vec4 rgbSE = texture(screenTexture, uv + vec2(1.0, 1.0) * texelSize);
    vec4 rgbM  = texture(screenTexture, uv);
    
    vec3 luma = vec3(0.299, 0.587, 0.114);
    float lumaNW = dot(rgbNW.rgb, luma);
    float lumaNE = dot(rgbNE.rgb, luma);
    float lumaSW = dot(rgbSW.rgb, luma);
    float lumaSE = dot(rgbSE.rgb, luma);
    float lumaM  = dot(rgbM.rgb, luma);
    
    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));
    
    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y = ((lumaNW + lumaSW) - (lumaNE + lumaSE));
    
    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    
    dir = min(vec2(FXAA_SPAN_MAX), max(vec2(-FXAA_SPAN_MAX), dir * rcpDirMin)) * texelSize;
    
    vec4 rgbA = 0.5 * (
        texture(screenTexture, uv + dir * (1.0/3.0 - 0.5)) +
        texture(screenTexture, uv + dir * (2.0/3.0 - 0.5)));
    
    vec4 rgbB = rgbA * 0.5 + 0.25 * (
        texture(screenTexture, uv + dir * -0.5) +
        texture(screenTexture, uv + dir * 0.5));
    
    float lumaB = dot(rgbB.rgb, luma);
    
    if (lumaB < lumaMin || lumaB > lumaMax) {
        FragColor = rgbA;
    } else {
        FragColor = rgbB;
    }
}
)";

/* Sharpening Fragment Shader (CAS-like) */
static const char* SHARPEN_FS = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D screenTexture;
uniform vec2 texelSize;
uniform float sharpenStrength;
uniform float sharpenClamp;

void main() {
    vec4 center = texture(screenTexture, TexCoord);
    vec4 top = texture(screenTexture, TexCoord + vec2(0.0, -texelSize.y));
    vec4 bottom = texture(screenTexture, TexCoord + vec2(0.0, texelSize.y));
    vec4 left = texture(screenTexture, TexCoord + vec2(-texelSize.x, 0.0));
    vec4 right = texture(screenTexture, TexCoord + vec2(texelSize.x, 0.0));
    
    vec4 neighbors = top + bottom + left + right;
    vec4 sharpen = center * 5.0 - neighbors;
    sharpen = clamp(sharpen, -sharpenClamp, sharpenClamp);
    
    FragColor = center + sharpen * sharpenStrength;
    FragColor.a = 1.0;
}
)";

/* Vibrance Fragment Shader */
static const char* VIBRANCE_FS = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D screenTexture;
uniform float vibranceStrength;

void main() {
    vec4 color = texture(screenTexture, TexCoord);
    
    float average = (color.r + color.g + color.b) / 3.0;
    float maxColor = max(color.r, max(color.g, color.b));
    float saturation = maxColor - average;
    
    float vibrance = vibranceStrength * (1.0 - saturation);
    
    color.rgb = mix(vec3(average), color.rgb, 1.0 + vibrance);
    
    FragColor = color;
}
)";

/* Vignette Fragment Shader */
static const char* VIGNETTE_FS = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D screenTexture;
uniform float vignetteStrength;
uniform float vignetteRadius;

void main() {
    vec4 color = texture(screenTexture, TexCoord);
    
    vec2 center = vec2(0.5, 0.5);
    float dist = distance(TexCoord, center);
    float vignette = smoothstep(vignetteRadius, vignetteRadius - 0.5, dist);
    
    color.rgb *= mix(1.0, vignette, vignetteStrength);
    
    FragColor = color;
}
)";

/* Film Grain Fragment Shader */
static const char* FILMGRAIN_FS = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D screenTexture;
uniform float grainIntensity;
uniform float grainSize;
uniform float time;

float rand(vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
    vec4 color = texture(screenTexture, TexCoord);
    
    vec2 grainCoord = TexCoord * grainSize + vec2(time * 0.1);
    float noise = rand(grainCoord) * 2.0 - 1.0;
    
    color.rgb += noise * grainIntensity;
    
    FragColor = color;
}
)";

/* Scanlines Fragment Shader */
static const char* SCANLINES_FS = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D screenTexture;
uniform float scanlineIntensity;
uniform float scanlineCount;
uniform vec2 resolution;

void main() {
    vec4 color = texture(screenTexture, TexCoord);
    
    float scanline = sin(TexCoord.y * resolution.y * 3.14159 * 2.0 / scanlineCount);
    scanline = (scanline + 1.0) * 0.5;
    scanline = mix(1.0, scanline, scanlineIntensity);
    
    color.rgb *= scanline;
    
    FragColor = color;
}
)";

/* Color Grading Fragment Shader */
static const char* COLORGRADE_FS = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D screenTexture;
uniform float brightness;
uniform float contrast;
uniform float saturation;
uniform float gamma;

void main() {
    vec4 color = texture(screenTexture, TexCoord);
    
    // Brightness
    color.rgb *= brightness;
    
    // Contrast
    color.rgb = (color.rgb - 0.5) * contrast + 0.5;
    
    // Saturation
    float luma = dot(color.rgb, vec3(0.299, 0.587, 0.114));
    color.rgb = mix(vec3(luma), color.rgb, saturation);
    
    // Gamma
    color.rgb = pow(color.rgb, vec3(1.0 / gamma));
    
    FragColor = clamp(color, 0.0, 1.0);
}
)";

/*===========================================================================
 * Resource Creation/Destruction
 *===========================================================================*/

static bool CreateResources(int width, int height) {
    ReShadeLogf("Creating resources for %dx%d", width, height);
    
    /* Create fullscreen quad */
    float quadVertices[] = {
        /* Position    TexCoord */
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
        
        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
    
    glGenVertexArrays(1, &s_quad_vao);
    glGenBuffers(1, &s_quad_vbo);
    
    glBindVertexArray(s_quad_vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_quad_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    glBindVertexArray(0);
    
    /* Create ping-pong FBOs */
    for (int i = 0; i < 2; i++) {
        glGenFramebuffers(1, &s_fbo[i]);
        glGenTextures(1, &s_fbo_texture[i]);
        
        glBindTexture(GL_TEXTURE_2D, s_fbo_texture[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        glBindFramebuffer(GL_FRAMEBUFFER, s_fbo[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_fbo_texture[i], 0);
        
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            ReShadeLogf("FBO %d incomplete!", i);
            return false;
        }
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    s_state.width = width;
    s_state.height = height;
    
    ReShadeLog("Resources created successfully");
    return true;
}

static void DestroyResources(void) {
    ReShadeLog("Destroying resources");
    
    if (s_quad_vao) {
        glDeleteVertexArrays(1, &s_quad_vao);
        s_quad_vao = 0;
    }
    if (s_quad_vbo) {
        glDeleteBuffers(1, &s_quad_vbo);
        s_quad_vbo = 0;
    }
    
    for (int i = 0; i < 2; i++) {
        if (s_fbo[i]) {
            glDeleteFramebuffers(1, &s_fbo[i]);
            s_fbo[i] = 0;
        }
        if (s_fbo_texture[i]) {
            glDeleteTextures(1, &s_fbo_texture[i]);
            s_fbo_texture[i] = 0;
        }
    }
}

static bool CompileShaders(void) {
    ReShadeLog("Compiling shaders...");
    
    s_fxaa_program = CreateProgram(QUAD_VS, FXAA_FS);
    if (!s_fxaa_program) {
        ReShadeLog("Failed to create FXAA program");
        return false;
    }
    
    s_sharpen_program = CreateProgram(QUAD_VS, SHARPEN_FS);
    if (!s_sharpen_program) {
        ReShadeLog("Failed to create sharpen program");
        return false;
    }
    
    s_vibrance_program = CreateProgram(QUAD_VS, VIBRANCE_FS);
    if (!s_vibrance_program) {
        ReShadeLog("Failed to create vibrance program");
        return false;
    }
    
    s_vignette_program = CreateProgram(QUAD_VS, VIGNETTE_FS);
    if (!s_vignette_program) {
        ReShadeLog("Failed to create vignette program");
        return false;
    }
    
    s_filmgrain_program = CreateProgram(QUAD_VS, FILMGRAIN_FS);
    if (!s_filmgrain_program) {
        ReShadeLog("Failed to create film grain program");
        return false;
    }
    
    s_scanlines_program = CreateProgram(QUAD_VS, SCANLINES_FS);
    if (!s_scanlines_program) {
        ReShadeLog("Failed to create scanlines program");
        return false;
    }
    
    s_colorgrade_program = CreateProgram(QUAD_VS, COLORGRADE_FS);
    if (!s_colorgrade_program) {
        ReShadeLog("Failed to create color grade program");
        return false;
    }
    
    ReShadeLog("All shaders compiled successfully");
    s_state.effects_loaded = true;
    return true;
}

static void DestroyShaders(void) {
    if (s_fxaa_program) { glDeleteProgram(s_fxaa_program); s_fxaa_program = 0; }
    if (s_sharpen_program) { glDeleteProgram(s_sharpen_program); s_sharpen_program = 0; }
    if (s_vibrance_program) { glDeleteProgram(s_vibrance_program); s_vibrance_program = 0; }
    if (s_vignette_program) { glDeleteProgram(s_vignette_program); s_vignette_program = 0; }
    if (s_filmgrain_program) { glDeleteProgram(s_filmgrain_program); s_filmgrain_program = 0; }
    if (s_scanlines_program) { glDeleteProgram(s_scanlines_program); s_scanlines_program = 0; }
    if (s_colorgrade_program) { glDeleteProgram(s_colorgrade_program); s_colorgrade_program = 0; }
    
    s_state.effects_loaded = false;
}

/*===========================================================================
 * INI File Parsing
 *===========================================================================*/

static std::map<std::string, std::map<std::string, std::string>> ParseINI(const std::string& path) {
    std::map<std::string, std::map<std::string, std::string>> sections;
    std::ifstream file(path);
    
    if (!file.is_open()) {
        return sections;
    }
    
    std::string line, currentSection;
    while (std::getline(file, line)) {
        /* Trim whitespace */
        size_t start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;
        line = line.substr(start);
        
        /* Skip comments */
        if (line[0] == ';' || line[0] == '#') continue;
        
        /* Section header */
        if (line[0] == '[') {
            size_t end = line.find(']');
            if (end != std::string::npos) {
                currentSection = line.substr(1, end - 1);
            }
            continue;
        }
        
        /* Key=Value */
        size_t eq = line.find('=');
        if (eq != std::string::npos) {
            std::string key = line.substr(0, eq);
            std::string value = line.substr(eq + 1);
            
            /* Trim key and value */
            size_t keyEnd = key.find_last_not_of(" \t");
            if (keyEnd != std::string::npos) key = key.substr(0, keyEnd + 1);
            
            size_t valStart = value.find_first_not_of(" \t");
            if (valStart != std::string::npos) value = value.substr(valStart);
            
            sections[currentSection][key] = value;
        }
    }
    
    return sections;
}

static void WriteINI(const std::string& path, const std::map<std::string, std::map<std::string, std::string>>& sections) {
    std::ofstream file(path);
    if (!file.is_open()) return;
    
    file << "; ===========================================================================\n";
    file << "; Castlevania 64 PC Recomp - Post-Processing Configuration\n";
    file << "; ===========================================================================\n";
    file << "; Configure ReShade post-processing effects.\n";
    file << "; ===========================================================================\n\n";
    
    for (const auto& section : sections) {
        file << "[" << section.first << "]\n";
        for (const auto& kv : section.second) {
            file << kv.first << "=" << kv.second << "\n";
        }
        file << "\n";
    }
}

/*===========================================================================
 * Default Configuration
 *===========================================================================*/

static void SetDefaultConfig(void) {
    s_config.enabled = true;
    s_config.preset = CV64_RESHADE_PRESET_ENHANCED;
    s_config.active_effects = CV64_EFFECT_FXAA | CV64_EFFECT_CAS | CV64_EFFECT_VIBRANCE;
    
    /* Default parameters */
    s_config.params.fxaa_quality = 0.75f;
    s_config.params.sharpen_strength = 0.5f;
    s_config.params.sharpen_clamp = 0.035f;
    s_config.params.vibrance_strength = 0.15f;
    s_config.params.brightness = 1.0f;
    s_config.params.contrast = 1.0f;
    s_config.params.saturation = 1.0f;
    s_config.params.gamma = 1.0f;
    s_config.params.vignette_strength = 0.3f;
    s_config.params.vignette_radius = 1.5f;
    s_config.params.grain_intensity = 0.05f;
    s_config.params.grain_size = 1.6f;
    s_config.params.bloom_intensity = 0.3f;
    s_config.params.bloom_threshold = 0.8f;
    s_config.params.scanline_intensity = 0.15f;
    s_config.params.scanline_count = 240;
    
    strcpy_s(s_config.shader_path, "patches\\shaders");
    strcpy_s(s_config.preset_path, "patches\\presets");
}

/*===========================================================================
 * Public API Implementation
 *===========================================================================*/

bool CV64_ReShade_Init(HWND hwnd, HGLRC hglrc) {
    ReShadeLog("Initializing ReShade integration...");
    
    if (s_state.initialized) {
        ReShadeLog("Already initialized");
        return true;
    }
    
    s_hwnd = hwnd;
    s_hglrc = hglrc;
    s_hdc = GetDC(hwnd);
    
    if (!s_hdc) {
        ReShadeLog("Failed to get device context");
        return false;
    }
    
    /* Make sure GL context is current */
    if (!wglMakeCurrent(s_hdc, s_hglrc)) {
        ReShadeLog("Failed to make GL context current");
        return false;
    }
    
    /* Load GL extensions */
    if (!LoadGLExtensions()) {
        ReShadeLog("Failed to load GL extensions");
        return false;
    }
    
    /* Set default configuration */
    SetDefaultConfig();
    
    ReShadeLogf("Default config set: enabled=%d, effects=0x%X", s_config.enabled, s_config.active_effects);
    
    /* Load configuration from file (if exists) */
    CV64_ReShade_LoadConfig();
    
    ReShadeLogf("After loading config: enabled=%d, effects=0x%X", s_config.enabled, s_config.active_effects);
    
    /* Get window size */
    RECT rect;
    GetClientRect(hwnd, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    
    ReShadeLogf("Window size: %dx%d", width, height);
    
    if (width <= 0) width = 1280;
    if (height <= 0) height = 720;
    
    /* Create GPU resources */
    if (!CreateResources(width, height)) {
        ReShadeLog("Failed to create resources");
        return false;
    }
    
    /* Compile shaders */
    if (!CompileShaders()) {
        ReShadeLog("Failed to compile shaders");
        DestroyResources();
        return false;
    }
    
    /* Initialize timing */
    QueryPerformanceFrequency(&s_freq);
    
    s_state.initialized = true;
    ReShadeLog("ReShade integration initialized successfully");
    
    return true;
}

void CV64_ReShade_Shutdown(void) {
    ReShadeLog("Shutting down ReShade integration...");
    
    if (!s_state.initialized) return;
    
    /* Save configuration */
    CV64_ReShade_SaveConfig();
    
    /* Destroy GPU resources */
    DestroyShaders();
    DestroyResources();
    
    if (s_hdc && s_hwnd) {
        ReleaseDC(s_hwnd, s_hdc);
    }
    
    s_hwnd = NULL;
    s_hglrc = NULL;
    s_hdc = NULL;
    
    memset(&s_state, 0, sizeof(s_state));
    
    ReShadeLog("ReShade integration shut down");
}

bool CV64_ReShade_LoadConfig(void) {
    ReShadeLog("Loading configuration from cv64_postprocessing.ini...");
    
    std::string configPath = "patches\\cv64_postprocessing.ini";
    auto sections = ParseINI(configPath);
    
    if (sections.empty()) {
        ReShadeLog("Config file not found or empty, using defaults");
        return false;
    }
    
    /* Parse [PostProcessing] section */
    auto& pp = sections["PostProcessing"];
    if (!pp.empty()) {
        if (pp.count("enabled")) s_config.enabled = (pp["enabled"] == "true" || pp["enabled"] == "1");
        if (pp.count("preset")) s_config.preset = (cv64_reshade_preset_t)atoi(pp["preset"].c_str());
    }
    
    /* Parse [Effects] section */
    auto& effects = sections["Effects"];
    if (!effects.empty()) {
        s_config.active_effects = 0;
        if (effects.count("fxaa") && (effects["fxaa"] == "true" || effects["fxaa"] == "1"))
            s_config.active_effects |= CV64_EFFECT_FXAA;
        if (effects.count("sharpen") && (effects["sharpen"] == "true" || effects["sharpen"] == "1"))
            s_config.active_effects |= CV64_EFFECT_CAS;
        if (effects.count("vibrance") && (effects["vibrance"] == "true" || effects["vibrance"] == "1"))
            s_config.active_effects |= CV64_EFFECT_VIBRANCE;
        if (effects.count("vignette") && (effects["vignette"] == "true" || effects["vignette"] == "1"))
            s_config.active_effects |= CV64_EFFECT_VIGNETTE;
        if (effects.count("filmgrain") && (effects["filmgrain"] == "true" || effects["filmgrain"] == "1"))
            s_config.active_effects |= CV64_EFFECT_FILMGRAIN;
        if (effects.count("scanlines") && (effects["scanlines"] == "true" || effects["scanlines"] == "1"))
            s_config.active_effects |= CV64_EFFECT_SCANLINES;
        if (effects.count("colorgrade") && (effects["colorgrade"] == "true" || effects["colorgrade"] == "1"))
            s_config.active_effects |= CV64_EFFECT_TONEMAP;
    }
    
    /* Parse [FXAA] section */
    auto& fxaa = sections["FXAA"];
    if (!fxaa.empty()) {
        if (fxaa.count("quality")) s_config.params.fxaa_quality = (float)atof(fxaa["quality"].c_str());
    }
    
    /* Parse [Sharpening] section */
    auto& sharpen = sections["Sharpening"];
    if (!sharpen.empty()) {
        if (sharpen.count("strength")) s_config.params.sharpen_strength = (float)atof(sharpen["strength"].c_str());
        if (sharpen.count("clamp")) s_config.params.sharpen_clamp = (float)atof(sharpen["clamp"].c_str());
    }
    
    /* Parse [Vibrance] section */
    auto& vibrance = sections["Vibrance"];
    if (!vibrance.empty()) {
        if (vibrance.count("strength")) s_config.params.vibrance_strength = (float)atof(vibrance["strength"].c_str());
    }
    
    /* Parse [ColorGrading] section */
    auto& colorgrade = sections["ColorGrading"];
    if (!colorgrade.empty()) {
        if (colorgrade.count("brightness")) s_config.params.brightness = (float)atof(colorgrade["brightness"].c_str());
        if (colorgrade.count("contrast")) s_config.params.contrast = (float)atof(colorgrade["contrast"].c_str());
        if (colorgrade.count("saturation")) s_config.params.saturation = (float)atof(colorgrade["saturation"].c_str());
        if (colorgrade.count("gamma")) s_config.params.gamma = (float)atof(colorgrade["gamma"].c_str());
    }
    
    /* Parse [Vignette] section */
    auto& vignette = sections["Vignette"];
    if (!vignette.empty()) {
        if (vignette.count("strength")) s_config.params.vignette_strength = (float)atof(vignette["strength"].c_str());
        if (vignette.count("radius")) s_config.params.vignette_radius = (float)atof(vignette["radius"].c_str());
    }
    
    /* Parse [FilmGrain] section */
    auto& grain = sections["FilmGrain"];
    if (!grain.empty()) {
        if (grain.count("intensity")) s_config.params.grain_intensity = (float)atof(grain["intensity"].c_str());
        if (grain.count("size")) s_config.params.grain_size = (float)atof(grain["size"].c_str());
    }
    
    /* Parse [Scanlines] section */
    auto& scanlines = sections["Scanlines"];
    if (!scanlines.empty()) {
        if (scanlines.count("intensity")) s_config.params.scanline_intensity = (float)atof(scanlines["intensity"].c_str());
        if (scanlines.count("count")) s_config.params.scanline_count = atoi(scanlines["count"].c_str());
    }
    
    ReShadeLog("Configuration loaded successfully");
    return true;
}

bool CV64_ReShade_SaveConfig(void) {
    ReShadeLog("Saving configuration to cv64_postprocessing.ini...");
    
    std::map<std::string, std::map<std::string, std::string>> sections;
    
    /* [PostProcessing] */
    sections["PostProcessing"]["enabled"] = s_config.enabled ? "true" : "false";
    sections["PostProcessing"]["preset"] = std::to_string((int)s_config.preset);
    
    /* [Effects] */
    sections["Effects"]["fxaa"] = (s_config.active_effects & CV64_EFFECT_FXAA) ? "true" : "false";
    sections["Effects"]["sharpen"] = (s_config.active_effects & CV64_EFFECT_CAS) ? "true" : "false";
    sections["Effects"]["vibrance"] = (s_config.active_effects & CV64_EFFECT_VIBRANCE) ? "true" : "false";
    sections["Effects"]["vignette"] = (s_config.active_effects & CV64_EFFECT_VIGNETTE) ? "true" : "false";
    sections["Effects"]["filmgrain"] = (s_config.active_effects & CV64_EFFECT_FILMGRAIN) ? "true" : "false";
    sections["Effects"]["scanlines"] = (s_config.active_effects & CV64_EFFECT_SCANLINES) ? "true" : "false";
    sections["Effects"]["colorgrade"] = (s_config.active_effects & CV64_EFFECT_TONEMAP) ? "true" : "false";
    
    /* [FXAA] */
    sections["FXAA"]["quality"] = std::to_string(s_config.params.fxaa_quality);
    
    /* [Sharpening] */
    sections["Sharpening"]["strength"] = std::to_string(s_config.params.sharpen_strength);
    sections["Sharpening"]["clamp"] = std::to_string(s_config.params.sharpen_clamp);
    
    /* [Vibrance] */
    sections["Vibrance"]["strength"] = std::to_string(s_config.params.vibrance_strength);
    
    /* [ColorGrading] */
    sections["ColorGrading"]["brightness"] = std::to_string(s_config.params.brightness);
    sections["ColorGrading"]["contrast"] = std::to_string(s_config.params.contrast);
    sections["ColorGrading"]["saturation"] = std::to_string(s_config.params.saturation);
    sections["ColorGrading"]["gamma"] = std::to_string(s_config.params.gamma);
    
    /* [Vignette] */
    sections["Vignette"]["strength"] = std::to_string(s_config.params.vignette_strength);
    sections["Vignette"]["radius"] = std::to_string(s_config.params.vignette_radius);
    
    /* [FilmGrain] */
    sections["FilmGrain"]["intensity"] = std::to_string(s_config.params.grain_intensity);
    sections["FilmGrain"]["size"] = std::to_string(s_config.params.grain_size);
    
    /* [Scanlines] */
    sections["Scanlines"]["intensity"] = std::to_string(s_config.params.scanline_intensity);
    sections["Scanlines"]["count"] = std::to_string(s_config.params.scanline_count);
    
    WriteINI("patches\\cv64_postprocessing.ini", sections);
    
    ReShadeLog("Configuration saved successfully");
    return true;
}

void CV64_ReShade_ApplyPreset(cv64_reshade_preset_t preset) {
    s_config.preset = preset;
    
    switch (preset) {
        case CV64_RESHADE_PRESET_NONE:
            s_config.active_effects = CV64_EFFECT_NONE;
            break;
            
        case CV64_RESHADE_PRESET_CLASSIC:
            s_config.active_effects = CV64_EFFECT_SCANLINES | CV64_EFFECT_VIGNETTE;
            s_config.params.scanline_intensity = 0.2f;
            s_config.params.vignette_strength = 0.4f;
            break;
            
        case CV64_RESHADE_PRESET_ENHANCED:
            s_config.active_effects = CV64_EFFECT_FXAA | CV64_EFFECT_CAS | CV64_EFFECT_VIBRANCE;
            s_config.params.fxaa_quality = 0.75f;
            s_config.params.sharpen_strength = 0.5f;
            s_config.params.vibrance_strength = 0.15f;
            break;
            
        case CV64_RESHADE_PRESET_CINEMATIC:
            s_config.active_effects = CV64_EFFECT_FILMGRAIN | CV64_EFFECT_VIGNETTE | CV64_EFFECT_TONEMAP;
            s_config.params.grain_intensity = 0.08f;
            s_config.params.vignette_strength = 0.5f;
            s_config.params.contrast = 1.1f;
            s_config.params.saturation = 0.9f;
            break;
            
        case CV64_RESHADE_PRESET_HORROR:
            s_config.active_effects = CV64_EFFECT_FILMGRAIN | CV64_EFFECT_VIGNETTE | CV64_EFFECT_TONEMAP;
            s_config.params.grain_intensity = 0.12f;
            s_config.params.vignette_strength = 0.7f;
            s_config.params.contrast = 1.3f;
            s_config.params.saturation = 0.6f;
            s_config.params.brightness = 0.85f;
            break;
            
        case CV64_RESHADE_PRESET_CUSTOM:
        default:
            /* Keep current settings */
            break;
    }
    
    ReShadeLogf("Applied preset: %d", (int)preset);
}

void CV64_ReShade_SetEffect(cv64_effect_flags_t effect, bool enabled) {
    if (enabled) {
        s_config.active_effects |= effect;
    } else {
        s_config.active_effects &= ~effect;
    }
}

bool CV64_ReShade_IsEffectEnabled(cv64_effect_flags_t effect) {
    return (s_config.active_effects & effect) != 0;
}

void CV64_ReShade_SetEnabled(bool enabled) {
    s_config.enabled = enabled;
}

bool CV64_ReShade_IsEnabled(void) {
    return s_config.enabled;
}

void CV64_ReShade_ProcessFrame(void) {
/* Debug: Log every 60 frames to confirm we're being called */
static int debug_frame_counter = 0;
debug_frame_counter++;
if (debug_frame_counter % 60 == 1) {
    ReShadeLogf("ProcessFrame called - initialized=%d, enabled=%d, effects=0x%X, width=%d, height=%d",
        s_state.initialized, s_config.enabled, s_config.active_effects, s_state.width, s_state.height);
}
    
if (!s_state.initialized) {
    if (debug_frame_counter % 60 == 1) ReShadeLog("Skipping: not initialized");
    return;
}
if (!s_config.enabled) {
    if (debug_frame_counter % 60 == 1) ReShadeLog("Skipping: disabled");
    return;
}
if (s_config.active_effects == 0) {
    if (debug_frame_counter % 60 == 1) ReShadeLog("Skipping: no effects active");
    return;
}
    
if (debug_frame_counter % 60 == 1) {
    ReShadeLog("Processing effects...");
}
    
LARGE_INTEGER startTime;
QueryPerformanceCounter(&startTime);
    
    /* ======================================================================
     * SAVE ALL GL STATE - Critical for not breaking GLideN64
     * ====================================================================== */
    GLint saved_program = 0;
    GLint saved_vao = 0;
    GLint saved_fbo_draw = 0;
    GLint saved_fbo_read = 0;
    GLint saved_texture = 0;
    GLint saved_active_texture = 0;
    GLint saved_viewport[4] = {0};
    GLboolean saved_depth_test = GL_FALSE;
    GLboolean saved_blend = GL_FALSE;
    GLboolean saved_cull_face = GL_FALSE;
    GLboolean saved_scissor_test = GL_FALSE;
    GLint saved_blend_src = 0, saved_blend_dst = 0;
    
    glGetIntegerv(GL_CURRENT_PROGRAM, &saved_program);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &saved_vao);
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &saved_fbo_draw);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &saved_fbo_read);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &saved_active_texture);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &saved_texture);
    glGetIntegerv(GL_VIEWPORT, saved_viewport);
    saved_depth_test = glIsEnabled(GL_DEPTH_TEST);
    saved_blend = glIsEnabled(GL_BLEND);
    saved_cull_face = glIsEnabled(GL_CULL_FACE);
    saved_scissor_test = glIsEnabled(GL_SCISSOR_TEST);
    glGetIntegerv(GL_BLEND_SRC_ALPHA, &saved_blend_src);
    glGetIntegerv(GL_BLEND_DST_ALPHA, &saved_blend_dst);
    
    /* Use actual viewport size, not our stored size */
    int actualWidth = saved_viewport[2];
    int actualHeight = saved_viewport[3];
    
    if (actualWidth <= 0 || actualHeight <= 0) {
        actualWidth = s_state.width;
        actualHeight = s_state.height;
    }
    
    /* Update our state if viewport changed */
    if (actualWidth != s_state.width || actualHeight != s_state.height) {
        ReShadeLogf("Viewport mismatch: stored=%dx%d, actual=%dx%d - resizing", 
            s_state.width, s_state.height, actualWidth, actualHeight);
        CV64_ReShade_Resize(actualWidth, actualHeight);
    }
    
    /* ======================================================================
     * Setup for post-processing
     * ====================================================================== */
    float texelSizeX = 1.0f / actualWidth;
    float texelSizeY = 1.0f / actualHeight;
    static float time = 0.0f;
    time += 0.016f;
    
    /* Copy from the source framebuffer (could be 0 or GLideN64's FBO) */
    glBindFramebuffer(GL_READ_FRAMEBUFFER, saved_fbo_draw);  /* Read from where GLideN64 was drawing */
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, s_fbo[0]);
    glBlitFramebuffer(0, 0, actualWidth, actualHeight,
                      0, 0, actualWidth, actualHeight,
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);
    
    s_current_fbo = 0;
    
    /* Setup render state for fullscreen passes */
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_SCISSOR_TEST);
    glBindVertexArray(s_quad_vao);
    glViewport(0, 0, actualWidth, actualHeight);
    
    /* Helper macro for ping-pong rendering */
    #define BEGIN_EFFECT(program) \
        glUseProgram(program); \
        glActiveTexture(GL_TEXTURE0); \
        glBindTexture(GL_TEXTURE_2D, s_fbo_texture[s_current_fbo]); \
        glUniform1i(glGetUniformLocation(program, "screenTexture"), 0); \
        glBindFramebuffer(GL_FRAMEBUFFER, s_fbo[1 - s_current_fbo]);
        
    #define END_EFFECT() \
        glDrawArrays(GL_TRIANGLES, 0, 6); \
        s_current_fbo = 1 - s_current_fbo;
    
    /* Apply FXAA */
    if (s_config.active_effects & CV64_EFFECT_FXAA) {
        BEGIN_EFFECT(s_fxaa_program);
        glUniform2f(glGetUniformLocation(s_fxaa_program, "texelSize"), texelSizeX, texelSizeY);
        glUniform1f(glGetUniformLocation(s_fxaa_program, "fxaaQuality"), s_config.params.fxaa_quality);
        END_EFFECT();
    }
    
    /* Apply Sharpening */
    if (s_config.active_effects & CV64_EFFECT_CAS) {
        BEGIN_EFFECT(s_sharpen_program);
        glUniform2f(glGetUniformLocation(s_sharpen_program, "texelSize"), texelSizeX, texelSizeY);
        glUniform1f(glGetUniformLocation(s_sharpen_program, "sharpenStrength"), s_config.params.sharpen_strength);
        glUniform1f(glGetUniformLocation(s_sharpen_program, "sharpenClamp"), s_config.params.sharpen_clamp);
        END_EFFECT();
    }
    
    /* Apply Vibrance */
    if (s_config.active_effects & CV64_EFFECT_VIBRANCE) {
        BEGIN_EFFECT(s_vibrance_program);
        glUniform1f(glGetUniformLocation(s_vibrance_program, "vibranceStrength"), s_config.params.vibrance_strength);
        END_EFFECT();
    }
    
    /* Apply Color Grading */
    if (s_config.active_effects & CV64_EFFECT_TONEMAP) {
        BEGIN_EFFECT(s_colorgrade_program);
        glUniform1f(glGetUniformLocation(s_colorgrade_program, "brightness"), s_config.params.brightness);
        glUniform1f(glGetUniformLocation(s_colorgrade_program, "contrast"), s_config.params.contrast);
        glUniform1f(glGetUniformLocation(s_colorgrade_program, "saturation"), s_config.params.saturation);
        glUniform1f(glGetUniformLocation(s_colorgrade_program, "gamma"), s_config.params.gamma);
        END_EFFECT();
    }
    
    /* Apply Film Grain */
    if (s_config.active_effects & CV64_EFFECT_FILMGRAIN) {
        BEGIN_EFFECT(s_filmgrain_program);
        glUniform1f(glGetUniformLocation(s_filmgrain_program, "grainIntensity"), s_config.params.grain_intensity);
        glUniform1f(glGetUniformLocation(s_filmgrain_program, "grainSize"), s_config.params.grain_size);
        glUniform1f(glGetUniformLocation(s_filmgrain_program, "time"), time);
        END_EFFECT();
    }
    
    /* Apply Scanlines */
    if (s_config.active_effects & CV64_EFFECT_SCANLINES) {
        BEGIN_EFFECT(s_scanlines_program);
        glUniform1f(glGetUniformLocation(s_scanlines_program, "scanlineIntensity"), s_config.params.scanline_intensity);
        glUniform1f(glGetUniformLocation(s_scanlines_program, "scanlineCount"), (float)s_config.params.scanline_count);
        glUniform2f(glGetUniformLocation(s_scanlines_program, "resolution"), (float)s_state.width, (float)s_state.height);
        END_EFFECT();
    }
    
    /* Apply Vignette (always last before blit) */
    if (s_config.active_effects & CV64_EFFECT_VIGNETTE) {
        BEGIN_EFFECT(s_vignette_program);
        glUniform1f(glGetUniformLocation(s_vignette_program, "vignetteStrength"), s_config.params.vignette_strength);
        glUniform1f(glGetUniformLocation(s_vignette_program, "vignetteRadius"), s_config.params.vignette_radius);
        END_EFFECT();
    }
    
    #undef BEGIN_EFFECT
    #undef END_EFFECT
    
    /* Blit final result back to the original draw framebuffer */
    glBindFramebuffer(GL_READ_FRAMEBUFFER, s_fbo[s_current_fbo]);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, saved_fbo_draw);
    glBlitFramebuffer(0, 0, actualWidth, actualHeight,
                      0, 0, actualWidth, actualHeight,
                      GL_COLOR_BUFFER_BIT, GL_NEAREST);
    
    /* ======================================================================
     * RESTORE ALL GL STATE
     * ====================================================================== */
    glUseProgram(saved_program);
    glBindVertexArray(saved_vao);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, saved_fbo_draw);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, saved_fbo_read);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, saved_texture);
    glActiveTexture(saved_active_texture);
    glViewport(saved_viewport[0], saved_viewport[1], saved_viewport[2], saved_viewport[3]);
    
    if (saved_depth_test) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (saved_blend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
    if (saved_cull_face) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (saved_scissor_test) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);
    
    /* Calculate processing time */
    LARGE_INTEGER endTime;
    QueryPerformanceCounter(&endTime);
    s_state.effect_time_ms = (double)(endTime.QuadPart - startTime.QuadPart) * 1000.0 / s_freq.QuadPart;
    
    s_state.frame_count++;
}

void CV64_ReShade_Resize(int width, int height) {
    if (width <= 0 || height <= 0) return;
    if (width == s_state.width && height == s_state.height) return;
    
    ReShadeLogf("Resizing to %dx%d", width, height);
    
    /* Recreate FBO textures with new size */
    for (int i = 0; i < 2; i++) {
        glBindTexture(GL_TEXTURE_2D, s_fbo_texture[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    }
    
    s_state.width = width;
    s_state.height = height;
}

const cv64_reshade_config_t* CV64_ReShade_GetConfig(void) {
    return &s_config;
}

const cv64_reshade_state_t* CV64_ReShade_GetState(void) {
    return &s_state;
}

bool CV64_ReShade_SetParam(const char* param, const char* value) {
    if (!param || !value) return false;
    
    float fval = (float)atof(value);
    int ival = atoi(value);
    
    if (strcmp(param, "fxaa_quality") == 0) s_config.params.fxaa_quality = fval;
    else if (strcmp(param, "sharpen_strength") == 0) s_config.params.sharpen_strength = fval;
    else if (strcmp(param, "sharpen_clamp") == 0) s_config.params.sharpen_clamp = fval;
    else if (strcmp(param, "vibrance_strength") == 0) s_config.params.vibrance_strength = fval;
    else if (strcmp(param, "brightness") == 0) s_config.params.brightness = fval;
    else if (strcmp(param, "contrast") == 0) s_config.params.contrast = fval;
    else if (strcmp(param, "saturation") == 0) s_config.params.saturation = fval;
    else if (strcmp(param, "gamma") == 0) s_config.params.gamma = fval;
    else if (strcmp(param, "vignette_strength") == 0) s_config.params.vignette_strength = fval;
    else if (strcmp(param, "vignette_radius") == 0) s_config.params.vignette_radius = fval;
    else if (strcmp(param, "grain_intensity") == 0) s_config.params.grain_intensity = fval;
    else if (strcmp(param, "grain_size") == 0) s_config.params.grain_size = fval;
    else if (strcmp(param, "bloom_intensity") == 0) s_config.params.bloom_intensity = fval;
    else if (strcmp(param, "bloom_threshold") == 0) s_config.params.bloom_threshold = fval;
    else if (strcmp(param, "scanline_intensity") == 0) s_config.params.scanline_intensity = fval;
    else if (strcmp(param, "scanline_count") == 0) s_config.params.scanline_count = ival;
    else return false;
    
    return true;
}

bool CV64_ReShade_GetParam(const char* param, char* buffer, size_t buffer_size) {
    if (!param || !buffer || buffer_size == 0) return false;
    
    float fval = 0.0f;
    
    if (strcmp(param, "fxaa_quality") == 0) fval = s_config.params.fxaa_quality;
    else if (strcmp(param, "sharpen_strength") == 0) fval = s_config.params.sharpen_strength;
    else if (strcmp(param, "sharpen_clamp") == 0) fval = s_config.params.sharpen_clamp;
    else if (strcmp(param, "vibrance_strength") == 0) fval = s_config.params.vibrance_strength;
    else if (strcmp(param, "brightness") == 0) fval = s_config.params.brightness;
    else if (strcmp(param, "contrast") == 0) fval = s_config.params.contrast;
    else if (strcmp(param, "saturation") == 0) fval = s_config.params.saturation;
    else if (strcmp(param, "gamma") == 0) fval = s_config.params.gamma;
    else if (strcmp(param, "vignette_strength") == 0) fval = s_config.params.vignette_strength;
    else if (strcmp(param, "vignette_radius") == 0) fval = s_config.params.vignette_radius;
    else if (strcmp(param, "grain_intensity") == 0) fval = s_config.params.grain_intensity;
    else if (strcmp(param, "grain_size") == 0) fval = s_config.params.grain_size;
    else if (strcmp(param, "bloom_intensity") == 0) fval = s_config.params.bloom_intensity;
    else if (strcmp(param, "bloom_threshold") == 0) fval = s_config.params.bloom_threshold;
    else if (strcmp(param, "scanline_intensity") == 0) fval = s_config.params.scanline_intensity;
    else if (strcmp(param, "scanline_count") == 0) {
        snprintf(buffer, buffer_size, "%d", s_config.params.scanline_count);
        return true;
    }
    else return false;
    
    snprintf(buffer, buffer_size, "%.4f", fval);
    return true;
}

void CV64_ReShade_Toggle(void) {
    s_config.enabled = !s_config.enabled;
    ReShadeLogf("Post-processing %s", s_config.enabled ? "enabled" : "disabled");
}

void CV64_ReShade_CyclePreset(void) {
    int next = ((int)s_config.preset + 1) % CV64_RESHADE_PRESET_COUNT;
    CV64_ReShade_ApplyPreset((cv64_reshade_preset_t)next);
}

bool CV64_ReShade_ReloadShaders(void) {
    ReShadeLog("Reloading shaders...");
    
    DestroyShaders();
    return CompileShaders();
}

double CV64_ReShade_GetProcessingTime(void) {
    return s_state.effect_time_ms;
}
