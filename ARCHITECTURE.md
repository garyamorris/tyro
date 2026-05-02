# Architecture

Reference document for the layout of the engine. For build / run / hotkeys,
see [README.md](README.md).

## Frame pipeline

```
                ┌──────────────────────────────────────┐
                │  Engine::run (per-frame loop)        │
                └─────────────┬────────────────────────┘
                              │
         pollEvents → resize check → frameTime / fixed-timestep accumulator
                              │
                  ┌───────────┴───────────┐
                  │  onUpdate (×N)        │  N = floor(accumulator / fixedDt)
                  │   - input             │      capped at maxStepsPerFrame
                  │   - flycam            │
                  │   - per-scene tick    │
                  │   - hot-reload poll   │
                  └───────────┬───────────┘
                              │
                  ┌───────────▼───────────────────────────────────────────┐
                  │  onRender                                             │
                  │                                                       │
                  │  ┌──────────────┐  cull camera frustum via octree     │
                  │  │  cull        │  + tighter per-entity AABB pass     │
                  │  └──────┬───────┘                                     │
                  │         │                                             │
                  │  ┌──────▼───────┐  shadowMap_ FBO (2K depth-only)     │
                  │  │  SHADOW      │  shadow_depth.vert/frag             │
                  │  │  pass        │  glPolygonOffset(2, 4)              │
                  │  └──────┬───────┘                                     │
                  │         │                                             │
                  │  ┌──────▼───────┐  sceneFbo_ (color + depth tex)      │
                  │  │  SCENE       │  Scene::render(visible) — uploads   │
                  │  │  pass        │  per-shader uniforms once,           │
                  │  │              │  per-entity uModel + drawElements   │
                  │  └──────┬───────┘                                     │
                  │         │                                             │
                  │  ┌──────▼───────┐  optional: normals.geom (lines)     │
                  │  │  OVERLAY     │  optional: wireframe.geom (PCF-AA)  │
                  │  │  pass        │  blend additively / depth LEQUAL    │
                  │  └──────┬───────┘                                     │
                  │         │                                             │
                  │  ┌──────▼───────┐  ping-pong pingFbo_ / pongFbo_      │
                  │  │  POST-FX     │  (color-only). Final pass writes    │
                  │  │  chain       │  to default framebuffer.            │
                  │  └──────┬───────┘                                     │
                  │         │                                             │
                  │  ┌──────▼───────┐  TextRenderer batch draw            │
                  │  │  UI overlay  │  (atlas texture, bitmap font)       │
                  │  └──────┬───────┘                                     │
                  │         │                                             │
                  │     gpuTimer_.endFrame()  — read prev frame's queries │
                  │                                                       │
                  └───────────┬───────────────────────────────────────────┘
                              │
                       swapBuffers + frame-rate cap
                              │
                              └─────────► (next frame)
```

## Subsystems

### `core/Engine`

`Engine::run(Application&, EngineConfig)` drives the loop. It owns the
`Window`, then polls / updates / renders / swaps. The frame-rate cap is
implemented as a hybrid sleep-and-spin in `waitUntil` — bulk
`std::this_thread::sleep_for` plus a tight spin for the last sub-millisecond.
On Windows we wrap the run with `timeBeginPeriod(1)` / `timeEndPeriod(1)` so
`sleep_for` actually wakes on time.

The fixed-timestep accumulator is a textbook Glenn-Fiedler integrator. With
`fixedDt = 1/120` and a 120 FPS render, you get one update per render — clean.
At 60 FPS render, two updates per render so animations stay smooth.

### `window/Window`

Thin GLFW wrapper. Creates a 3.3 core context, disables vsync (the Engine
caps frame rate), exposes cursor capture for mouse-look, and owns the
shared-context refcount.

### `math/`

- `Math.h` — `Vec2/3/4`, `Mat3/4`, `Quat`. Column-major matrix layout matches
  GL/GLSL so we can pass `glUniformMatrix4fv(..., GL_FALSE, m.data())` with no
  transpose. Includes `lookAt`, `perspective`, `ortho`, `rotate`, `translate`,
  `scale`, `inverseTranspose` for normal matrices.
- `AABB.h` — bounding box; `transformed()` uses the abs-of-3x3 trick from
  Akenine-Möller for transformed extents.
- `Frustum.h` — Gribb-Hartmann extraction from a view-projection matrix; the
  six planes are inward-pointing. Cull test uses the canonical "p-vertex"
  technique.

### `renderer/`

| Type | Responsibility |
|---|---|
| `Shader` | Vert+frag (+ optional geom) program, uniform setters, **hot-reload** via cached file paths + modtimes |
| `Mesh` | VAO+VBO+EBO, vertex layout (pos + normal + uv + tangent = 44 bytes), per-frame draw-call/triangle counters; `computeTangents()` (MOLLER 1996) used by primitives + OBJ loader |
| `FullscreenTriangle` | Lazy-init VAO, drives 3-vertex draw of a fullscreen tri (UVs derived in shader from `gl_VertexID`) |
| `FrameBuffer` | Color texture + selectable depth (`None` / `Renderbuffer` / sampleable `Texture`) |
| `ShadowMap` | Depth-only FBO with `glDrawBuffer(GL_NONE)` and `glReadBuffer(GL_NONE)` |
| `Texture` | stb_image-loaded or memory-loaded; mipmaps + GL_REPEAT defaults |
| `Primitives` | CPU procedural mesh generators: cube, sphere, plane, torus, cylinder |
| `ProceduralTextures` | RGBA8 byte-buffer generators: checker, brick, wood, marble, noise, hex |
| `GpuTimer` | `GL_TIME_ELAPSED` queries with 2-frame ping-pong; section-named breakdown |
| `TextRenderer` | 5×7 bitmap font atlas, single-quad-batch per frame |

### `scene/`

| Type | Responsibility |
|---|---|
| `Camera` | pos / target / up + fovYDeg / aspect / near / far → view, proj, frustum |
| `FlyCamera` | yaw / pitch from mouse delta, WASD / Q-E / Shift movement → applies to a Camera |
| `Light` | POD: directional or point, color, intensity, radius |
| `Material` | shader pointer + albedo / emissive / shininess + optional `Texture*` (TEX0) + UV scale/offset |
| `Entity` | mesh* + material* + transform (pos + quat rotation + scale) + local AABB |
| `Octree` | Recursive 8-way split. Items spanning multiple children stay at the parent. |
| `Scene` | Owns mesh / shader / material vectors; entities + lights; uploads per-frame uniforms (camera, lights, shadow, time) once per shader |

### `loader/ObjLoader`

Hand-written. Handles `v / vn / vt / f`, fan-triangulates n-gons, dedupes
`(v, vn, vt)` triples into an index buffer, computes face-averaged normals
when the OBJ has none.

### Shader catalogue

```
shaders/
├── pbr.{vert,frag}          - Cook-Torrance GGX, Schlick F, Smith G,
│                              normal mapping via TBN, ACES tonemap, PCF shadow
├── phong_lit.{vert,frag}    - main lit shader, texture-aware, PCF shadow
├── unlit.{vert,frag}        - normal-tinted matcap-ish
├── toon.{vert,frag}         - 3-band cel + rim
├── rim.{vert,frag}          - Fresnel-only glow
├── checker.{vert,frag}      - procedural checker, lit, PCF shadow
├── normals.{vert,geom,frag} - geom shader: line per vertex along normal
├── wireframe.{vert,geom,frag} - barycentric AA wireframe (overlay or solid)
├── shadow_depth.{vert,frag} - depth-only pass for shadow map
├── water.{vert,frag}        - sin-sum displacement + Fresnel + spec
├── explode.{vert,geom,frag} - face-normal extrusion, pulsing
├── marble.frag              - turbulent fbm banding
├── wood.frag                - radial rings + Y-axis grain
├── brick.frag               - UV-space brick layout + jitter
├── hex.frag                 - analytic hex distance, edge mask
├── iridescent.frag          - Fresnel-driven hue cycle
├── hologram.frag            - scanlines + cyan tint + glitch + edge glow
├── text.{vert,frag}         - bitmap font atlas, alpha-mask discard
├── blit.vert                - shared fullscreen-tri vertex shader
└── post_*.frag              - passthrough/sobel/bright/blur/composite/fog/ssao/chromatic
```

All procedural-pattern fragment shaders reuse `phong_lit.vert` for the geometry
path so they get the same `vWorldPos` / `vNormal` / `vUV` outputs.

## Per-frame uniform plumbing

Within a single render, `Scene::uploadSceneUniforms(shader)` is called the
first time a given shader is bound (tracked via `uploadedShaders_`). It uploads:

- `uView`, `uProj`, `uViewProj`, `uCameraPos`
- `uLights[i]` struct array + `uLightCount` (clamped to 8)
- `uLightVP`, `uShadowMap` (TEX3), `uShadowEnabled`
- `uTime` (drives water + explode + iridescent + hologram)

Then per-entity, `Material::apply()` sets material values and binds the
albedo texture to TEX0 if any. Finally per-entity:

- `uModel`
- `uNormalMatrix` (inverse-transpose of upper-left 3×3 of model)

Texture-unit assignments are stable: TEX0 = albedo, TEX1 = post-process bloom
input, TEX3 = shadow map. Shaders that don't declare a uniform silently ignore
its upload (location -1 → no-op).

## Rendering correctness notes

- The shadow pass writes to the same texture lit shaders read; we set
  `Scene::shadowEnabled = false` and `shadowMapTex = 0` for the duration of
  the shadow pass so the lit shaders' first-bind upload during that phase
  doesn't bind the in-flight render target as input.
- The wireframe overlay uses `glDepthFunc(GL_LEQUAL)` so coincident depth
  values are accepted (the same triangles are being redrawn).
- The water vertex shader recomputes the surface normal from finite
  differences of the wave field so lighting moves with the surface.

## Hot-reload semantics

`Shader::reloadIfChanged()` polls `std::filesystem::last_write_time` for each
of vert / frag / geom. If any changed since last poll:

1. Read the new source files.
2. Compile + link to a *new* program ID (the original is preserved).
3. On success, delete the old program and assign the new one.
4. On failure, the original keeps running — log the compile/link error and move
   on. Modtimes are updated unconditionally so we don't spin recompile on a
   broken file every frame; the user re-saves to retry.

Polled every 250 ms in `onUpdate`.
