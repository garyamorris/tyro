# tyro

A small, opinionated 3D rendering engine in modern C++17 / OpenGL 3.3 core.

**tyro is a teaching project.** Its goal is to show — in the smallest amount
of code that's still recognisably a real engine — how a renderer fits
together: the frame loop, the scene graph, the shader pipeline, lighting,
shadows, IBL, and post-processing. It is not meant to compete with engines
you'd ship a game on; it's meant to be readable end-to-end. Each demo scene
is built to isolate a single concept so you can match code to pixels.

Built from scratch — including a custom math library, a hand-rolled GL function
loader, a procedural mesh library, an octree, frustum culling, shadow mapping,
hot-reloadable shaders, GPU timer queries, screen-space post effects, and a
text overlay driven by an embedded bitmap font. Ten cycleable demo scenes
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
- 20+ unique materials wired across 10 scenes:
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
- **IBL** — image-based lighting via Karis 2014 split-sum approximation. A
  procedural HDR sky is generated at startup (gradient + bright sun disc),
  then baked into: an environment cubemap (equirect → 512² × 6, with mips),
  a 32² diffuse irradiance cubemap (cosine-hemisphere convolution), a
  128² × 5-mip prefiltered radiance cubemap (GGX importance-sampled, mip-
  per-roughness), and a 512² RG16F BRDF LUT (Hammersley + Schlick G_Smith).
  The PBR shader picks up the ambient term as
  `kD·irradiance·albedo + prefilter·(F·brdf.x + brdf.y)`, with a
  roughness-aware Fresnel to avoid grazing-angle hot edges. A skybox pass
  renders the full env cube as background after the scene pass. Toggle
  with `K`.
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

| # | Name | What it shows | Teaches |
|---|---|---|---|
| 1 | Materials Showcase | Mesh × material grid: 5 mesh types × 4 materials | how the same geometry looks under different BRDFs |
| 2 | Light Garden | 6 colored point lights orbiting on a checker ground | multi-light Phong with attenuation |
| 3 | Octree Stress | 512 entities scattered, demonstrates octree+frustum culling | broad-phase culling — toggle with `F` to see the cost |
| 4 | Effects Demo | A few hero meshes with strong contrast for post-process effects | screen-space post-FX as a chain of fullscreen passes |
| 5 | Showcase Bay | Teapot + Spot the cow + primitives, shadowed | directional shadow mapping + PCF (toggle `J`) |
| 6 | Water Pond | Animated water plane (sin-sum vertex displacement + Fresnel) with floating teapot/cow | vertex displacement + Fresnel + per-pixel normals from finite differences |
| 7 | Geometry Lab | Three meshes pulsing via the explode geometry shader | the geometry shader stage |
| 8 | Texture Lab | 3×3 grid: textured (top row) vs procedural-pattern (mid) vs exotic (front) shaders | texture sampling vs procedural fragment shaders |
| 9 | PBR Lab | 3×5 sphere grid: metallic sweep (back), roughness sweep at metallic=1 (mid), textured PBR materials (front) — Cook-Torrance GGX, normal mapping, ACES tonemap, IBL ambient + skybox | physically-based shading parameters in isolation (metallic ↔ roughness) |
| 10 | Atrium | Full enclosed environment: brick walls, marble columns + ceiling, wood floor, central pedestal with iridescent orb, water pool, hologram + explode-torus + teapot + spot displays, 6 flickering torches + sun through skylight, PCF shadows, IBL | everything together — shadows, IBL, multiple materials, lights, post-FX |

## Gallery

<!-- Drop captures into docs/img/. See docs/img/README.md for filenames + sizes. -->

<p align="center">
  <img src="docs/img/atrium.png"       alt="Atrium scene"      width="48%"/>
  <img src="docs/img/pbr_lab.png"      alt="PBR Lab"           width="48%"/>
</p>
<p align="center">
  <img src="docs/img/light_garden.png" alt="Light Garden"      width="48%"/>
  <img src="docs/img/water_pond.png"   alt="Water Pond"        width="48%"/>
</p>

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
| `B` | Toggle debug bounds overlay (entity AABBs + octree node boxes — pair with `F` to see culling work) |
| `L` | Toggle light/shadow overlay (point-light radii as wire spheres + sun's shadow frustum) |
| `J` | Toggle directional shadow mapping |
| `K` | Toggle image-based lighting (IBL ambient + skybox) |
| `V` | Cycle FPS cap (120 / 60 / unlocked) |
| `F` | Toggle frustum culling (compare visible counts) |
| `H` | Toggle stats overlay |
| `1`–`7` | Override material on every entity (lit warm / lit cool / toon / rim / checker / unlit / wireframe) |
| `Esc` | Quit |

## Build

**Common prerequisites**

- CMake 3.20+
- A C++17 compiler
- An OpenGL 3.3 core driver
- Internet on first configure — CMake `FetchContent` clones GLFW 3.4 once

The post-build step copies `shaders/` and `assets/` next to the binary so the
demo can find them via relative paths.

> Only the Windows path is regularly tested. Linux/macOS instructions are
> provided in good faith based on the build's portability (the only Windows-
> specific link is `opengl32` in `CMakeLists.txt`), but they are **unverified**
> — reports welcome.

### Windows (verified)

Visual Studio 2019 (or Build Tools):

```pwsh
cmake -S . -B build -G "Visual Studio 16 2019" -A x64
cmake --build build --config Release
build\Release\tyro.exe
```

### Linux (unverified)

Install GLFW's X11/Wayland system dependencies first. On Debian / Ubuntu:

```sh
sudo apt install libx11-dev libxrandr-dev libxinerama-dev \
                 libxcursor-dev libxi-dev libgl1-mesa-dev
```

Then:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/tyro
```

### macOS (unverified)

Apple deprecated OpenGL in 10.14, but the GL 3.3 core profile still runs on
current macOS. Xcode command-line tools required.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/tyro
```

## Project layout

```
tyro/
├── CMakeLists.txt
├── README.md, LICENSE, ARCHITECTURE.md
├── assets/             - OBJ models (teapot, spot-the-cow) — not checked in;
│                        see Troubleshooting
├── shaders/            - GLSL: vert / frag / geom (51 files)
├── src/
│   ├── main.cpp        - the 8-scene demo, hotkeys, render orchestration
│   ├── core/Engine     - game loop, frame limiter, fixed timestep
│   ├── window/Window   - GLFW wrapper, mouse capture, GL context
│   ├── math/           - Math.h, AABB.h, Frustum.h
│   ├── renderer/       - Shader, Mesh, FrameBuffer, ShadowMap, Texture,
│   │                    GpuTimer, Primitives, ProceduralTextures, TextRenderer,
│   │                    Cubemap, IblBaker, Skybox
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

## Where to start reading

If you've cloned this and want to learn from it, a suggested path:

1. **Build and run it.** Cycle scenes with `[` / `]` and toggle features
   (`F` culling, `J` shadows, `K` IBL, `P` post-FX) so you can match each
   bit of code to a visible difference.
2. [`src/main.cpp`](src/main.cpp) — the demo. Each scene is built imperatively;
   this is where meshes, materials, lights, and entities are wired up.
3. [`src/core/Engine.cpp`](src/core/Engine.cpp) — the frame loop:
   poll → fixed-timestep update(s) → render → swap.
4. [`src/scene/Scene.cpp`](src/scene/Scene.cpp) — how per-frame uniforms are
   uploaded once per shader and how entities are culled before drawing.
5. [`shaders/phong_lit.frag`](shaders/phong_lit.frag) — start here for lit
   shading. Then [`shaders/pbr.frag`](shaders/pbr.frag) for the full
   Cook-Torrance + IBL version.
6. [`ARCHITECTURE.md`](ARCHITECTURE.md) — full frame pipeline + subsystem
   reference once you want the bird's-eye view.

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

## Troubleshooting

**`assets/` directory missing — build's post-build copy fails, or the demo
exits with "failed to open assets/teapot.obj"**

The `assets/` folder is not currently checked into the repo, but
`src/main.cpp` loads `assets/teapot.obj` and `assets/spot.obj` at startup,
and `CMakeLists.txt` copies the directory next to the binary as a post-build
step. You have two options:

- Obtain the models (links in [Acknowledgements](#acknowledgements)) and
  place them at `assets/teapot.obj` and `assets/spot.obj`.
- Or comment out the OBJ loads in `src/main.cpp` (search for `loadObjMesh`)
  to run on the procedural primitives only.

**CMake `FetchContent` fails on first configure (offline / firewall)**

The first configure clones GLFW 3.4 from GitHub. If your environment blocks
this, pre-clone `https://github.com/glfw/glfw.git` at tag `3.4` and either
point `FetchContent_Declare` at the local checkout, or vendor it under
`third_party/` like `stb`.

**Black window / "OpenGL 3.3 not supported"**

The driver is too old, or you've fallen back to software rendering. Update
GPU drivers. On Linux, confirm:

```sh
glxinfo | grep "OpenGL core profile version"
```

reports 3.3 or higher.

**Shaders fail to load at runtime**

The binary expects `shaders/` and `assets/` in its working directory (the
post-build step in `CMakeLists.txt` copies them next to the executable). Run
from the build output directory, not the source root.

**Hot-reload doesn't pick up shader edits**

The 250 ms file-watcher polls the *copied* `shaders/` next to the binary,
not the source-tree `shaders/`. Edit the copy, re-run the build to refresh
it, or replace the copy with a symlink to the source tree.

## License

MIT — see [LICENSE](LICENSE).
