# Ray Tracer using DXR

This repo implementes a ray tracer utilizing DirectX 12 and the DirectX Ray Tracing Pipeline (DXR).
Restricted (at least for now) to the simple functionality of tracing spheres only, with some coarse
approximations of light scattering behavior for ideal diffuse, metal, and dielectric materials.

## Results

| ![](https://github.com/vliopas97/DXR-RayTracer/blob/main/img/sample1.png?raw=true) | ![](https://github.com/vliopas97/DXR-RayTracer/blob/main/img/sample2.png?raw=true) |
|----------|----------|

<div style="text-align:left;">
  <i>Sample from the raytracer. From left to right: dielectric, diffuse, and metallic material. Runs in an 800 by 600 window, 32 rays per pixel, with a maximum recursion depth of 5. This gives about 15 FPS on my 1070Ti.</i>
</div>

## References

- Heavily based on the work of the [Ray Tracing book series](https://raytracing.github.io) by Peter Shirley.
- [Ray Tracing Gems series](https://www.realtimerendering.com/raytracinggems/).
- Matt Pettineo's [DXR Path Tracer](https://github.com/TheRealMJP/DXRPathTracer) for enabling bindless resource access in the context of D3D12.
