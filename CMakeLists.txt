cmake_minimum_required(VERSION 3.20)
project(tyro CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W4 /permissive- /Zc:preprocessor)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS NOMINMAX)
endif()

# --- Dependencies -----------------------------------------------------------
include(FetchContent)

set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.4
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(glfw)

# --- GL loader (vendored, hand-rolled) --------------------------------------
add_library(gl_loader STATIC
  third_party/glloader/gl_loader.c
)
target_include_directories(gl_loader PUBLIC third_party/glloader)

# --- stb_image (vendored, single header) ------------------------------------
add_library(stb_image STATIC
  third_party/stb/stb_image_impl.cpp
)
target_include_directories(stb_image PUBLIC third_party/stb)
if(MSVC)
  target_compile_options(stb_image PRIVATE /W0)
endif()

# --- tyro engine ------------------------------------------------------------
add_executable(tyro
  src/main.cpp
  src/window/Window.cpp
  src/core/Engine.cpp
  src/renderer/Shader.cpp
  src/renderer/Mesh.cpp
  src/renderer/FrameBuffer.cpp
  src/renderer/Renderer.cpp
  src/renderer/Primitives.cpp
  src/renderer/TextRenderer.cpp
  src/renderer/GpuTimer.cpp
  src/renderer/ShadowMap.cpp
  src/renderer/Texture.cpp
  src/renderer/ProceduralTextures.cpp
  src/loader/ObjLoader.cpp
  src/scene/Camera.cpp
  src/scene/FlyCamera.cpp
  src/scene/Material.cpp
  src/scene/Entity.cpp
  src/scene/Octree.cpp
  src/scene/Scene.cpp
)

target_include_directories(tyro PRIVATE src)
target_link_libraries(tyro PRIVATE glfw gl_loader stb_image)

if(WIN32)
  target_link_libraries(tyro PRIVATE opengl32)
endif()

# Copy shaders + assets next to the binary so relative paths work at runtime.
add_custom_command(TARGET tyro POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:tyro>/shaders
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_SOURCE_DIR}/assets  $<TARGET_FILE_DIR:tyro>/assets
)
