// Minimal OpenGL 3.3 core loader — declares only what tyro needs.
// Functions are loaded at runtime via glfwGetProcAddress.
#ifndef TYRO_GL_LOADER_H
#define TYRO_GL_LOADER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned int  GLenum;
typedef unsigned char GLboolean;
typedef unsigned int  GLbitfield;
typedef void          GLvoid;
typedef signed char   GLbyte;
typedef short         GLshort;
typedef int           GLint;
typedef int           GLsizei;
typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef unsigned int  GLuint;
typedef float         GLfloat;
typedef float         GLclampf;
typedef double        GLdouble;
typedef double        GLclampd;
typedef char          GLchar;
typedef ptrdiff_t     GLintptr;
typedef ptrdiff_t     GLsizeiptr;
typedef unsigned long long GLuint64;

#define GL_FALSE 0
#define GL_TRUE  1

#define GL_DEPTH_BUFFER_BIT   0x00000100
#define GL_STENCIL_BUFFER_BIT 0x00000400
#define GL_COLOR_BUFFER_BIT   0x00004000

#define GL_POINTS         0x0000
#define GL_LINES          0x0001
#define GL_TRIANGLES      0x0004
#define GL_TRIANGLE_STRIP 0x0005

#define GL_DEPTH_TEST     0x0B71
#define GL_CULL_FACE      0x0B44
#define GL_BLEND          0x0BE2
#define GL_TEXTURE_2D     0x0DE1

#define GL_BACK           0x0405
#define GL_FRONT          0x0404
#define GL_CCW            0x0901
#define GL_CW             0x0900

#define GL_LESS           0x0201
#define GL_LEQUAL         0x0203

#define GL_BYTE           0x1400
#define GL_UNSIGNED_BYTE  0x1401
#define GL_SHORT          0x1402
#define GL_UNSIGNED_SHORT 0x1403
#define GL_INT            0x1404
#define GL_UNSIGNED_INT   0x1405
#define GL_FLOAT          0x1406

#define GL_VENDOR         0x1F00
#define GL_RENDERER       0x1F01
#define GL_VERSION        0x1F02

#define GL_ARRAY_BUFFER         0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_STATIC_DRAW          0x88E4
#define GL_DYNAMIC_DRAW         0x88E8

#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER   0x8B31
#define GL_GEOMETRY_SHADER 0x8DD9
#define GL_COMPILE_STATUS  0x8B81
#define GL_LINK_STATUS     0x8B82
#define GL_INFO_LOG_LENGTH 0x8B84

#define GL_LINES_ADJACENCY      0x000A
#define GL_TRIANGLES_ADJACENCY  0x000C
#define GL_LINE_STRIP           0x0003

#define GL_TEXTURE0       0x84C0
#define GL_TEXTURE1       0x84C1
#define GL_TEXTURE2       0x84C2
#define GL_TEXTURE3       0x84C3
#define GL_TEXTURE4       0x84C4
#define GL_TEXTURE5       0x84C5
#define GL_TEXTURE6       0x84C6
#define GL_TEXTURE7       0x84C7

#define GL_TIME_ELAPSED            0x88BF
#define GL_QUERY_RESULT            0x8866
#define GL_QUERY_RESULT_AVAILABLE  0x8867

#define GL_NONE                  0
#define GL_POLYGON_OFFSET_FILL   0x8037

#define GL_BLEND_SRC_ALPHA 0x0302
#define GL_SRC_ALPHA       0x0302
#define GL_ONE_MINUS_SRC_ALPHA 0x0303
#define GL_ONE             1
#define GL_ZERO            0

#define GL_R8              0x8229
#define GL_RED             0x1903
#define GL_DEPTH_COMPONENT32F 0x8CAC
#define GL_UNPACK_ALIGNMENT 0x0CF5
#define GL_PACK_ALIGNMENT   0x0D05

#define GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX           0x9047
#define GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX     0x9048
#define GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX   0x9049

#define GL_TEXTURE_MIN_FILTER 0x2801
#define GL_TEXTURE_MAG_FILTER 0x2800
#define GL_TEXTURE_WRAP_S     0x2802
#define GL_TEXTURE_WRAP_T     0x2803
#define GL_NEAREST            0x2600
#define GL_LINEAR             0x2601
#define GL_LINEAR_MIPMAP_LINEAR 0x2703
#define GL_REPEAT             0x2901
#define GL_CLAMP_TO_EDGE      0x812F

#define GL_RGB              0x1907
#define GL_RGBA             0x1908
#define GL_RGB8             0x8051
#define GL_RGBA8            0x8058
#define GL_RG               0x8227
#define GL_RG16F            0x822F
#define GL_RGB16F           0x881B
#define GL_RGBA16F          0x881A
#define GL_RGB32F           0x8815
#define GL_HALF_FLOAT       0x140B
#define GL_DEPTH_COMPONENT  0x1902
#define GL_DEPTH_COMPONENT24 0x81A6

#define GL_TEXTURE_CUBE_MAP             0x8513
#define GL_TEXTURE_CUBE_MAP_SEAMLESS    0x884F
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X  0x8515
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X  0x8516
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y  0x8517
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y  0x8518
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z  0x8519
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z  0x851A
#define GL_TEXTURE_WRAP_R               0x8072
#define GL_TEXTURE_BASE_LEVEL           0x813C
#define GL_TEXTURE_MAX_LEVEL            0x813D
#define GL_TEXTURE_MIN_LOD              0x813A
#define GL_TEXTURE_MAX_LOD              0x813B
#define GL_TEXTURE_LOD_BIAS             0x8501

#define GL_FRAMEBUFFER       0x8D40
#define GL_RENDERBUFFER      0x8D41
#define GL_COLOR_ATTACHMENT0 0x8CE0
#define GL_DEPTH_ATTACHMENT  0x8D00
#define GL_FRAMEBUFFER_COMPLETE 0x8CD5

#define GL_NO_ERROR       0
#define GL_INVALID_ENUM   0x0500
#define GL_INVALID_VALUE  0x0501
#define GL_INVALID_OPERATION 0x0502

// Function pointer typedefs + extern declarations.
#define TYRO_GL_FUNCS \
  X(void,   ClearColor,            (GLfloat, GLfloat, GLfloat, GLfloat)) \
  X(void,   Clear,                 (GLbitfield)) \
  X(void,   Viewport,              (GLint, GLint, GLsizei, GLsizei)) \
  X(void,   Enable,                (GLenum)) \
  X(void,   Disable,               (GLenum)) \
  X(void,   CullFace,              (GLenum)) \
  X(void,   FrontFace,             (GLenum)) \
  X(void,   DepthFunc,             (GLenum)) \
  X(GLenum, GetError,              (void)) \
  X(const GLubyte*, GetString,     (GLenum)) \
  X(void,   GenVertexArrays,       (GLsizei, GLuint*)) \
  X(void,   BindVertexArray,       (GLuint)) \
  X(void,   DeleteVertexArrays,    (GLsizei, const GLuint*)) \
  X(void,   GenBuffers,            (GLsizei, GLuint*)) \
  X(void,   BindBuffer,            (GLenum, GLuint)) \
  X(void,   BufferData,            (GLenum, GLsizeiptr, const void*, GLenum)) \
  X(void,   DeleteBuffers,         (GLsizei, const GLuint*)) \
  X(void,   VertexAttribPointer,   (GLuint, GLint, GLenum, GLboolean, GLsizei, const void*)) \
  X(void,   EnableVertexAttribArray, (GLuint)) \
  X(GLuint, CreateShader,          (GLenum)) \
  X(void,   ShaderSource,          (GLuint, GLsizei, const GLchar* const*, const GLint*)) \
  X(void,   CompileShader,         (GLuint)) \
  X(void,   GetShaderiv,           (GLuint, GLenum, GLint*)) \
  X(void,   GetShaderInfoLog,      (GLuint, GLsizei, GLsizei*, GLchar*)) \
  X(void,   DeleteShader,          (GLuint)) \
  X(GLuint, CreateProgram,         (void)) \
  X(void,   AttachShader,          (GLuint, GLuint)) \
  X(void,   LinkProgram,           (GLuint)) \
  X(void,   GetProgramiv,          (GLuint, GLenum, GLint*)) \
  X(void,   GetProgramInfoLog,     (GLuint, GLsizei, GLsizei*, GLchar*)) \
  X(void,   UseProgram,            (GLuint)) \
  X(void,   DeleteProgram,         (GLuint)) \
  X(GLint,  GetUniformLocation,    (GLuint, const GLchar*)) \
  X(void,   Uniform1i,             (GLint, GLint)) \
  X(void,   Uniform1f,             (GLint, GLfloat)) \
  X(void,   Uniform2f,             (GLint, GLfloat, GLfloat)) \
  X(void,   Uniform3f,             (GLint, GLfloat, GLfloat, GLfloat)) \
  X(void,   Uniform4f,             (GLint, GLfloat, GLfloat, GLfloat, GLfloat)) \
  X(void,   UniformMatrix3fv,      (GLint, GLsizei, GLboolean, const GLfloat*)) \
  X(void,   UniformMatrix4fv,      (GLint, GLsizei, GLboolean, const GLfloat*)) \
  X(void,   DrawArrays,            (GLenum, GLint, GLsizei)) \
  X(void,   DrawElements,          (GLenum, GLsizei, GLenum, const void*)) \
  X(void,   GenTextures,           (GLsizei, GLuint*)) \
  X(void,   BindTexture,           (GLenum, GLuint)) \
  X(void,   ActiveTexture,         (GLenum)) \
  X(void,   TexImage2D,            (GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const void*)) \
  X(void,   TexParameteri,         (GLenum, GLenum, GLint)) \
  X(void,   GenerateMipmap,        (GLenum)) \
  X(void,   DeleteTextures,        (GLsizei, const GLuint*)) \
  X(void,   GenFramebuffers,       (GLsizei, GLuint*)) \
  X(void,   BindFramebuffer,       (GLenum, GLuint)) \
  X(void,   FramebufferTexture2D,  (GLenum, GLenum, GLenum, GLuint, GLint)) \
  X(void,   DeleteFramebuffers,    (GLsizei, const GLuint*)) \
  X(GLenum, CheckFramebufferStatus,(GLenum)) \
  X(void,   GenRenderbuffers,      (GLsizei, GLuint*)) \
  X(void,   BindRenderbuffer,      (GLenum, GLuint)) \
  X(void,   RenderbufferStorage,   (GLenum, GLenum, GLsizei, GLsizei)) \
  X(void,   FramebufferRenderbuffer,(GLenum, GLenum, GLenum, GLuint)) \
  X(void,   DeleteRenderbuffers,   (GLsizei, const GLuint*)) \
  X(void,   LineWidth,             (GLfloat)) \
  X(void,   PolygonMode,           (GLenum, GLenum)) \
  X(void,   GetIntegerv,           (GLenum, GLint*)) \
  X(void,   BlendFunc,             (GLenum, GLenum)) \
  X(void,   PixelStorei,           (GLenum, GLint)) \
  X(void,   GenQueries,            (GLsizei, GLuint*)) \
  X(void,   DeleteQueries,         (GLsizei, const GLuint*)) \
  X(void,   BeginQuery,            (GLenum, GLuint)) \
  X(void,   EndQuery,              (GLenum)) \
  X(void,   GetQueryObjectuiv,     (GLuint, GLenum, GLuint*)) \
  X(void,   GetQueryObjectui64v,   (GLuint, GLenum, GLuint64*)) \
  X(void,   DrawBuffer,            (GLenum)) \
  X(void,   ReadBuffer,            (GLenum)) \
  X(void,   PolygonOffset,         (GLfloat, GLfloat)) \
  X(void,   DepthMask,              (GLboolean))

#define X(ret, name, args) typedef ret (APIENTRYP PFN_gl##name) args; extern PFN_gl##name gl##name;
#ifndef APIENTRY
  #if defined(_WIN32)
    #define APIENTRY __stdcall
  #else
    #define APIENTRY
  #endif
#endif
#ifndef APIENTRYP
  #define APIENTRYP APIENTRY *
#endif
TYRO_GL_FUNCS
#undef X

// Loader. Pass a function-pointer-getter (e.g. glfwGetProcAddress).
typedef void* (*tyro_gl_proc_loader)(const char* name);
int tyro_gl_load(tyro_gl_proc_loader loader);

#ifdef __cplusplus
}
#endif

#endif
