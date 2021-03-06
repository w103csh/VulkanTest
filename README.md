# Guppy (Vulkan)
  This repository is a test ground for a basic game engine using the [Vulkan API](https://www.khronos.org/vulkan/). The repository was based off a sample in the [LunarG VulkanSamples repository](https://github.com/LunarG/VulkanSamples), specifically the [Hologram sample](https://github.com/LunarG/VulkanSamples/tree/master/Sample-Programs/Hologram). Very little from the sample remains except for some cross platform code, and CMake build files.

  The main focus of the code herein has been on the rendering side, but some other tiny features have been added along the way as well. There is a list of some of the things you can find inside the repo below. 

## How to Build and Run
[BUILD.md](BUILD.md) has directions for building and running the code.

## Features
* Windows build
  * Native & [GLFW](https://www.glfw.org/) window/input support
  * Directory listener for recompiling/loading shaders on file change at runtime.
* macOS build ([MoltenVK](https://github.com/KhronosGroup/MoltenVK))
  * [GLFW](https://www.glfw.org/) window/input support
  * Directory listener for recompiling/loading loading shaders on file change at runtime.
* [Dear ImGui](https://github.com/ocornut/imgui) integration
* [FMOD](https://www.fmod.com/) integration
* [XInput](https://docs.microsoft.com/en-us/windows/win32/xinput/xinput-game-controller-apis-portal) integration
* .obj mesh/material loading
* Basic CPU selection
* Other things I'm forgetting...

## Rendering
Recently in my experimenting with Vulkan pipelines/passes/shaders I decided to switch from forward shading to deferred. Below you can see a list of things I have implemented and which pipeline they were developed for. Ideally I would have enough time to make shaders for parity between all the pipelines, but there is only so much time in the day (ray tracing pipelines are coming soon hopefully!). I have tried to start making demo videos for some of the more interesting pipelines/shaders. More to come soon...

> **Note: The demo videos were taken on a MacBook Pro running Bootcamp Windows drivers using a Radeon Pro 560X at 1920x1080 in debug mode. The focus has been learning the math, and gaining a thorough understanding of the Vulkan API.**

* Forward shading
  * Multi-sampling and resolve attachments
  * PBR (Physically based rendering)
  * Fog
  * Positional lights
  * Parallax texture mapping
  * Normal (bump) mapping
  * Offscreen render to texture
    * Subsequent pass (same frame) use projective texture mapping onto geometry in the main scene.
    * Screen space post-processing (compute)
      * Gaussian blurring (glow)
      * HDR w/ bloom (doesn't work well)
* Deferred shading
  * Multi-sampling for output attachment passes, and resolve attachment for combine/screenspace pass with "by region" dependency.
  * Tessellation
    * Lines
  * Ocean simulation - *click image for video link*
  
    [![ocean_surface_thumbnail](https://user-images.githubusercontent.com/5341766/73319812-b86cf800-41fa-11ea-9f87-ef008389ac5f.jpg)](https://youtu.be/rdF66PNIm78 "Guppy - Ocean Simulation")
    
    The technique used to generate the surface comes from Jerry Tessendorf's *Simulating Ocean Water* (a pdf of the paper can be found in the misc folder in the repository [here](misc/simulating_ocean_water_tessendorf.pdf)). This is still very much a work in progress.
    * Wave dispersion, and Fast Fourier Transform compute shaders generate height, normal, and horizontal displacement maps.
    * The color of the surface comes from pretty much the same technique that is outlined in the paper. The surface is also lightly tessellated using [Phong Tessellation](https://perso.telecom-paristech.fr/boubek/papers/PhongTessellation/).
    * Currently working on:
      * Adaptively tessellated terrain
      * Speeding up the hastily done FFT compute shader.
      * Using the Jacobian properties of the surface described in the paper to generate foam and particle sprays
      * Environmental reflection on the surface.
      * Color from below the surface.

  * Particles (compute)
    * Fountains (euler integration)
      * Recyclable with parameterized position/velociity/acceleration/rotation
        * Billboarded quads
        * Meshes - *click image for video link*

          [![parictleMeshes](https://user-images.githubusercontent.com/5341766/70402048-40026b80-19ef-11ea-835a-1ff9ddfd35b4.jpg)](https://youtu.be/57DorBKAc1Q "Guppy - recyclable particle fountain meshes with shadows")
    * Attractors (euler integration)
      * Recyclable parameterized position/velocity/acceleration - *click image for video link*
      
        [![attractors](https://user-images.githubusercontent.com/5341766/70402018-2b25d800-19ef-11ea-9d6a-d610aa8d9ce6.jpg)](https://youtu.be/sWfPW5PMsjg "Guppy - 125000 particles affected by two gravity attractors demo")
    * Cloth (euler integration)
      * Spring lattice - *click image for video link*

        [![cloth](https://user-images.githubusercontent.com/5341766/70402032-324ce600-19ef-11ea-921b-d0eef1b32d73.jpg)](https://youtu.be/AQdR3c39388 "Guppy - Cloth shader demo (compute particle spring lattice)")
    * Height field fluids - *click image for video link*

        [![hff](https://user-images.githubusercontent.com/5341766/70402036-3b3db780-19ef-11ea-9885-eb30c486db25.jpg)](https://youtu.be/j3vdii2Hkyc "Guppy - Height field fluid demo")
        
        The color shaders used in the video are for debugging purposes like making sure normal generation is working correctly, and the simulation is stable and pausable.
  * Shadows
    * ~~One direction with PCF softening~~ removed in favor of below. Hopefully have a better solution soon.
    * Point lights that generate a dynamic depth cube map array using geometry shader - *click image for video link*

      [![shadowCubeMap](https://user-images.githubusercontent.com/5341766/70402055-42fd5c00-19ef-11ea-9721-f3dab508e86f.jpg)](https://youtu.be/ri3ZodRF7VY "Guppy - Point lights with shadow cube maps demo")
* Both
  * Materials
  * Instanced drawing
  * Triple buffering
  * Blinn Phong
  * Directional lights
  * Spot lights

## Contact Information
* [Colin Hughes](mailto:colin.s.hughes@gmail.com)