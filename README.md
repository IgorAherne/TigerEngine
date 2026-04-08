# Tiger Engine 0.1 — Voxel Cone Tracing Global Illumination

A custom real-time rendering engine written in **C++ / OpenGL 4.3+**, featuring double-bounce indirect illumination via voxel cone tracing.  
Built as an MSc project at Newcastle University (2016) by **Igor Aherne**.

https://github.com/user-attachments/assets/2cae9384-99fb-4cff-a0b3-d0ec90afcb1c

All 3D assets in the demo scene were modelled by hand.

## Features

- **Double-bounce indirect illumination** — light bounces twice through the scene, gives realistic colour bleeding onto nearby surfaces.
- **Cone-traced soft shadows** — shadow penumbrae emerge naturally from the voxel volume.
- **Dynamic voxel reflections** — an additional sharp cone, oriented along the reflection vector, approximates specular reflections from the voxel data.
- **Cone-traced ambient occlusion** — natural darkening of crevices and corners where less light arrives.
- **Emissive surface lighting** — objects marked as emissive bypass shadow-mapping and act as area light sources whose glow propagates through the GI pipeline.
- **Quadralinear filtering** — samples are interpolated across four dimensions (3D position + mip level), giving smooth transitions between voxel resolutions.

---

## How It Works

The GI pipeline runs every frame in the following stages:

### 1 — Voxelisation

Scene triangles are rasterised into a 128³ 3D texture grid (`basecolor_tex3D`, RGBA16F). Each voxel stores the unlit material colour. A second 3D texture (`normal_and_brightness_tex3D`, RGBA16F) stores the surface normal with an emissive-brightness value in the alpha channel. An atomic counter texture (`count_texture`, R32UI) tracks how many triangles fall into each voxel so colours can be averaged in a follow-up compute-shader pass (`voxelizeAverage`).

### 2 — Direct-light injection

Point lights (with pre-computed shadow maps) illuminate each voxel: `lambert × basecolour × lightColour × shadowCoeff`. The result is written to `diffuse_tex3D`. Emissive voxels skip shadow attenuation and store their base-colour directly, so self-glowing surfaces are never darkened. Alpha is set to 1 for occupied voxels and 0 for empty ones.

### 3 — Mip-mapping the diffuse volume

`diffuse_tex3D` is mip-mapped. All four RGBA channels are averaged independently — diffuse colour is **not** pre-multiplied by alpha. Pre-multiplying would incorrectly darken partially occupied mip texels.

### 4 — First-bounce cone tracing (per-voxel)

For every occupied voxel, a hemisphere oriented along its stored normal is approximated by a bouquet of ~7 cones. Each cone is traced outward through the diffuse volume:

1. Start at the voxel, step outward along the cone direction.
2. At each step, sample the diffuse mip level whose texel size matches the current cone cross-section (quadralinear interpolation between the two nearest mip levels).
3. Accumulate colour weighted by the sample's alpha; stop when accumulated alpha ≥ 1 or the coarsest mip is reached.
4. No inverse-square fall-off is applied — the increasing mip samples already average over larger volumes.

Results from all cones are combined and stored in `bounce_and_brightness_tex3D`, which is then mip-mapped the same way. This first-bounce computation is performed inside `voxelizeMeshes()` via `filterInto_1st_bounce()`, completing the entire voxel-side pipeline in one call.

### 5 — Deferred geometry pass

A standard deferred pass fills a G-buffer with per-pixel position, base-colour, world-space normal + emission, specular, velocity, and depth (see the `renderTextures` enum in `renderer.h`).

### 6 — Screen-space second-bounce pass

For each visible fragment the world position and normal are recovered from the G-buffer. Cone tracing is performed a second time, sampling **both** the direct-light diffuse volume and the first-bounce volume. The two contributions are added to the fragment's deferred-lit colour, yielding double-bounce global illumination. This pass runs at a reduced resolution (`gi_resolution_divisor`) and is upscaled back to full resolution via an interpolation / stencil pass to maintain performance.

### 7 — Reflections

An 8th, narrower cone is traced along the reflection direction (derived from the fragment normal and view vector), providing approximate specular reflections from the voxel data.

---

## Controls

| Keys | Action |
|---|---|
| **W / A / S / D** | Move camera forward / left / back / right |
| **arrow keys** | move the orange sphere. Home/End moves it up and down |
| **Q / E** | Move camera down / up |
| **I / J / K / L / U / O** | Move point light forward / left / back / right / down / up |
| **U / O** | Move point light down / up |
| **Space** | Toggle between standard lighting and voxel GI mode |
| **Hold 1 or 2** | Toggle between single and double light bounce |
| **Hold 3 or 4** | downsampling of the VXGI screenspace resolution |
| **Hold 5 or 6** | increase/reduce specularity |

---

## Engine Architecture

The engine follows a **component-based game-object** model:

| Concept | Key class | Role |
|---|---|---|
| Game objects | `gameObject` | Container holding a transform plus attachable components (mesh, material, camera, light, behaviours). |
| Components | `component` (base), `mesh`, `material`, `camera`, `pointLight`, `basic_navigator` / `basic_rotator` / `basic_spinner` | Modular behaviours attached at runtime via `attachComponent<T>()`. |
| Scene graph | `scene` | Owns all game objects; drives per-frame `update()`. |
| Renderer | `renderer` | Manages one GLFW window + OpenGL context, deferred G-buffer, light FBO, and orchestrates the full render loop. Multiple renderers (windows) can share a GL context. |
| Voxel GI | `voxelGICore` | Owns the five 3D textures and all shaders for voxelisation, light injection, bounce computation, and screen-space GI output. Default: 128³ voxels covering a 50-unit world cube. |
| Math library | `vec2` / `vec3` / `vec4`, `mat3` / `mat4`, `plane`, `frustum` | Hand-written linear-algebra types. |
| Repositories | `repositories` | Central cache for loaded meshes. |

### Render loop (per frame)

```
timer::dt()
input::updateInput(dt)
scene::update(dt)                // updates all game objects & components
renderer::renderAllrenderers(dt)
  ├─ voxelGICore::clearVoxels()
  ├─ voxelGICore::voxelizeMeshes()      // runs BEFORE the camera loop
  │    ├─ modernVoxelize()              // rasterise triangles into 3D textures
  │    ├─ voxelizeAverage()             // compute-shader averaging
  │    ├─ diffuse_tex3D mip-map
  │    └─ filterInto_1st_bounce()       // per-voxel cone trace → mip-map bounce tex
  │
  ├─ for each camera:
  │    ├─ frustum cull
  │    ├─ opaque_geom_pass()            → fills G-buffer (gFBO)
  │    ├─ [SPACE toggles mode]
  │    │    ├─ normal: lighting_pass()  → point-light shadow maps → lightFBO
  │    │    └─ GI:     lighting_voxelGI_pass() → screen-space 2nd bounce
  │    │               (optionally displayVoxels() for debug)
  │    └─ gizmo_pass()
  │
  └─ swap buffers
input::flush_keys()
```

---

## Dependencies

- **OpenGL 4.3+** (compute shaders, image load/store, atomic counters)
- **GLEW** — OpenGL extension loading
- **GLFW 3** — window creation, input handling
- **stb_image** (bundled) — texture loading

---

## Building

The project was originally developed with Visual Studio 2015 on Windows. To build:

1. Install GLEW and GLFW3 development libraries (headers + `.lib` / `.dll`) - *already included by default*.
2. Open the project in Visual Studio (or configure CMake to point at the source files).
3. Make sure the include paths resolve `<GLEW/glew.h>` and `<GLFW/glfw3.h>` - already done by default. 
4. Build and run — the entry point is `main.cpp`.

---

## File Overview

| File | Purpose |
|---|---|
| `main.cpp` | Entry point — initialises framework, creates renderer, sets up demo scene with geometry, emissive spheres, and a point light. |
| `VoxelGICore.cpp / .h` | Core voxel cone tracing: voxelisation, light injection, first-bounce filtering, screen-space second-bounce output. |
| `renderer.cpp / .h` | Deferred-rendering pipeline: G-buffer, light pass, GI pass, gizmo pass. |
| `pointLight.cpp / .h` | Point light with shadow-map generation (1024² shadow map, range 1–40). |
| `pointLightGI.cpp / .h` | Point light adapted for GI voxel injection. |
| `material.cpp / .h` | Per-object material (colour, emissive intensity, shader binding). |
| `gameObject.cpp / .h` | Component-based game object with transform, parent/child hierarchy, and bounding-shape culling. |
| `scene.cpp / .h` | Scene management and per-frame update dispatch. |
| `camera.cpp / .h` | Camera component (FOV, near/far, frustum). |
| `mesh.cpp / .h` | Mesh component (VBO/VAO management, quad generation). |
| `shader.cpp / .h` | OpenGL shader program wrapper. |
| `texture.cpp / .h` | 2D and 3D texture wrapper (creation, mip-mapping, binding). |
| `repositories.cpp / .h` | Mesh asset cache. |
| `framework.cpp / .h` | GLFW init / terminate. |
| `input.cpp / .h` | Keyboard input polling. |
| `transform.cpp / .h` | Position, rotation, scale; local ↔ world matrix computation. |
| `frustum.cpp / .h` | View-frustum planes for culling. |
| `basic_navigator / rotator / spinner` | Simple movement / rotation behaviour components. |
| `vec2 / vec3 / vec4`, `mat3 / mat4`, `plane` | Hand-written math library. |
| `bshape`, `sphere_bshape`, `obb_bshape`, `bshape_intersections` | Bounding volumes and intersection tests for frustum culling. |

---

## Licence & Attribution

© 2016 Igor Aherne — Newcastle University, MSc Computer Science.
