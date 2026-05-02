# tyro

A small, opinionated 3D rendering engine in modern C++17 / OpenGL 3.3 core.

Built from scratch — including a custom math library, a hand-rolled GL function
loader, a procedural mesh library, an octree, frustum culling, shadow mapping,
hot-reloadable shaders, GPU timer queries, screen-space post effects, and a
text overlay driven by an embedded bitmap font. Eight cycleable demo scenes
showcase the feature surface.

## Features

**Core**
- Custom math library — `Vec2/3/4`, `Mat3/4`, `Quat`, column-major (GL convention),
  with `lookAt` / `perspective` / `ortho` / `inverseTranspose` and friends
- Engine loop with Glenn-Fiedler fixed-timestep updates + variable rendering
- **Frame-rate cap** — software-timed 120 / 60 / unlocked, hybrid sleep-and-spin
  pacing with `timeBeginPeriod(1)` for sub-millisecond accuracy on Windows
- Free-fly camera (WASD + mouse-look + `Shift` sprint)

**Rendering**
- 6 mesh primitives generated procedurally: cube, sphere, plane, torus, cylinder,
  ground
- Real OBJ models via a hand-written parser (Newell teapot, Spot the cow)
- Multi-stage shader pipeline (vertex / geometry / fragment) — geometry-shader
  examples include normals overlay, wireframe via barycentric coords, and an
  exploding mesh effect
- 20+ unique materials wired across 9 scenes:
  - Phong (warm + cool), Toon (cel-shaded), Fresnel rim, Unlit matcap, Wireframe,
    Water (animated), Explode (geometry shader), Marble / Wood / Brick / Hex
    (procedural patterns), Iridescent, Hologram, **PBR** (Cook-Torrance GGX with
    metallic / roughness / normal maps and ACES tonemap)
- **Texturing** — `Texture` class via `stb_image`; texture-aware Phong shader;
  6 procedural textures generated and uploaded at startup (checker / brick /
  wood / marble / noise / hex), plus a Sobel-derived rough-surface normal map
- **PBR** — physically-based shading with Cook-Torrance GGX, Schlick Fresnel,
  Smith G, energy-conserving kS/kD split, normal mapping via per-vertex
  tangents (computed in the OBJ loader and primitive generators), and ACES
  tonemap. Same shader handles uniform-color, textured-albedo, and
  metallic-roughness-mapped variants.
- **Frame buffers** with selectable depth attachments (none / renderbuffer /
  sampleable depth texture)
- **Screen-space post-process chain** (cycle with `P`):
  None / Sobel edge / Bloom (3 passes) / Depth fog / SSAO-lite / Chromatic ab.
- **Directional shadow mapping** — 2K depth-only FBO, 3×3 PCF, slope-scaled
  polygon offset, automatic light-VP from `lights[0]`

**Scene management**
- Scene container holding meshes, shaders, materials, entities, lights, camera
- AABB + Gribb-Hartmann frustum extraction
- Static **octree** (recursive 8-way split) for broad-phase frustum culling

**Tooling**
- **Hot-reload shaders** — every 250 ms, each shader polls its source files'
  modtimes; on change it recompiles to a fresh program. Compile / link errors
  preserve the previous program. Edit a `.frag` while the demo is running and
  see the change land in <1 s.
- **GPU timer overlay** — per-pass timings via `GL_TIME_ELAPSED` queries with
  2-frame ping-pong (no pipeline stalls). Sections: Shadow / Scene / Overlay /
  PostFX / UI.
- **Stats overlay** — embedded 5×7 bitmap font, atlas + single batched draw
  call. Shows FPS (EMA), frame-time, draw calls, triangles, visible/total
  entities, octree node count, NVIDIA VRAM via the `GL_NVX_gpu_memory_info`
  extension.

## Demo scenes

Cycle with `[` and `]`:

| # | Name | What it shows |
|---|---|---|
| 1 | Materials Showcase | Mesh × material grid: 5 mesh types × 4 materials |
| 2 | Light Garden | 6 colored point lights orbiting on a checker ground |
| 3 | Octree Stress | 512 entities scattered, demonstrates octree+frustum culling |
| 4 | Effects Demo | A few hero meshes with strong contrast for post-process effects |
| 5 | Showcase Bay | Teapot + Spot the cow + primitives, shadowed |
| 6 | Water Pond | Animated water plane (sin-sum vertex displacement + Fresnel) with floating teapot/cow |
| 7 | Geometry Lab | Three meshes pulsing via the explode geometry shader |
| 8 | Texture Lab | 3×3 grid: textured (top row) vs procedural-pattern (mid) vs exotic (front) shaders |
| 9 | PBR Lab | 3×5 sphere grid: metallic sweep (back), roughness sweep at metallic=1 (mid), textured PBR materials (front) — Cook-Torrance GGX with normal mapping + ACES tonemap |

## Hotkeys

| Key | Action |
|---|---|
| `WASD` | Move; `Q`/`E` (or `Space`/`Ctrl`) up/down; `Shift` = sprint |
| Mouse | Look (when captured) |
| `Tab` | Toggle mouse capture |
| `[` / `]` | Previous / next scene |
| `P` | Cycle post-process effect |
| `N` | Toggle geometry-shader normals overlay |
| `T` | Toggle wireframe overlay |
| `J` | Toggle directional shadow mapping |
| `V` | Cycle FPS cap (120 / 60 / unlocked) |
| `F` | Toggle frustum culling (compare visible counts) |
| `H` | Toggle stats overlay |
| `1`–`7` | Override material on every entity (lit warm / lit cool / toon / rim / checker / unlit / wireframe) |
| `Esc` | Quit |

## Build

Requires Windows + Visual Studio 2019 (or Build Tools), CMake 3.20+, an
internet connection on first configure (CMake `FetchContent` pulls GLFW 3.4
once).

```pwsh
cmake -S . -B build -G "Visual Studio 16 2019" -A x64
cmake --build build --config Release
build\Release\tyro.exe
```

The post-build step copies `shaders/` and `assets/` next to the binary so the
demo can find them via relative paths.

## Project layout

```
tyro/
├── CMakeLists.txt
├── README.md, LICENSE, ARCHITECTURE.md
├── assets/             - OBJ models (cube, teapot, spot-the-cow)
├── shaders/            - GLSL: vert / frag / geom (~30 files)
├── src/
│   ├── main.cpp        - the 8-scene demo, hotkeys, render orchestration
│   ├── core/Engine     - game loop, frame limiter, fixed timestep
│   ├── window/Window   - GLFW wrapper, mouse capture, GL context
│   ├── math/           - Math.h, AABB.h, Frustum.h
│   ├── renderer/       - Shader, Mesh, FrameBuffer, ShadowMap, Texture,
│   │                    GpuTimer, Primitives, ProceduralTextures, TextRenderer
│   ├── scene/          - Camera, FlyCamera, Light, Material, Entity,
│   │                    Scene, Octree
│   └── loader/ObjLoader - hand-written .obj parser (v / vn / vt / f, fan-tri)
└── third_party/
    ├── glloader/       - hand-rolled GL 3.3 loader (~80 entry points)
    └── stb/            - stb_image (public domain, vendored)
```

## Architecture in one paragraph

The engine is structured as a thin `Application`-callback frame loop in
`Engine::run`. Each frame: poll input, run zero-or-more fixed-timestep updates
into `Application::onUpdate`, then call `onRender` exactly once. The render
phase issues a shadow-map pass into a 2K depth-only FBO, then a forward-lit
scene pass into an FBO with sampleable color + depth textures, then optional
debug overlays (geometry-shader normals / wireframe), then a post-process
chain that ping-pongs between two color-only FBOs and blits to the default
framebuffer, then a text-overlay quad batch on top. `Scene` owns meshes,
shaders, materials, entities, and lights; before the scene pass we extract a
view-projection frustum (Gribb-Hartmann) and cull entities through a static
octree. See [ARCHITECTURE.md](ARCHITECTURE.md) for the full breakdown.

## Acknowledgements

- **GLFW 3.4** — window + input + GL context creation. Pulled via CMake
  `FetchContent`. <https://www.glfw.org/>
- **stb_image** by Sean Barrett — public-domain image loading, vendored at
  `third_party/stb/`. <https://github.com/nothings/stb>
- **Newell Teapot** — Martin Newell, 1975. Public domain.
- **Spot the cow** by Keenan Crane (CC0). Distributed via Alec Jacobson's
  [common-3d-test-models](https://github.com/alecjacobson/common-3d-test-models)
  repository.
- The hand-rolled GL loader, math library, OBJ parser, octree, post-process
  shaders, bitmap font, and everything in `src/` are original code.

## License

MIT — see [LICENSE](LICENSE).
