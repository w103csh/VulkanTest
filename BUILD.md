# Build Instructions

Instructions for building this repository on Windows, macOS, ~~Linux~~, and
~~Android~~. (I plan to make this work on Linux, Android, and iOS).

## Index

<!-- 1. [Contributing](#contributing-to-the-repository)
1. [Repository Content](#repository-content)
1. [Repository Set-Up](#repository-set-up) -->
1. [Windows Build](#building-on-windows)
1. [Mac Build](#building-on-mac)
1. ~~[Linux Build](#building-on-linux)~~
1. ~~[Android Build](#building-on-android)~~

<!-- ## Contributing to the Repository

If you intend to contribute, the preferred work flow is for you to develop
your contribution in a fork of this repository in your GitHub account and then
submit a pull request. Please see the [CONTRIBUTING.md](CONTRIBUTING.md) file
in this repository for more details.

## Repository Content

This repository contains the source code necessary to build the LunarG
Vulkan Samples
 - The Vulkan Samples repo is a set of source and data files in a specific
    directory hierarchy:
      - API-Samples - Samples that demonstrate the use of various aspects of the
        Vulkan API
      - Vulkan Tutorial - Steps you through the process of creating a simple Vulkan application, learning the basics along the way. This [Vulkan Tutorial link](https://vulkan.lunarg.com/doc/sdk/latest/windows/tutorial/html/index.html) allows you to view the Vulkan Tutorial on LunarXchange as well. 
      - Sample-Programs - Samples that are more functional and go deeper than simple API use.
      - Layer-Samples - Samples that are implemented as layers.  The Overlay layer sample is deprecated and does not build. -->

## Repository Set-Up

### Display Drivers

This repository does not contain a Vulkan-capable driver. You will need to
obtain and install a Vulkan driver from your graphics hardware vendor or from
some other suitable source if you intend to run Vulkan applications.

### Download the Repository

To create your local git repository:

    git clone https://github.com/w103csh/GuppyVulkan.git

### Repository Dependencies

This repository attempts to resolve some of its dependencies by using
components found from the following places, in this order:

1. CMake or Environment variable overrides (e.g., -DGLSLANG_INSTALL_DIR)
1. [LunarG Vulkan SDK](https://vulkan.lunarg.com/), located by the `VULKAN_SDK` environment variable
1. System-installed packages, mostly applicable on Linux

Dependencies that cannot be resolved by the SDK or installed packages must be
resolved with the "install directory" override and are listed below. The
"install directory" override can also be used to force the use of a specific
version of that dependency.

<!-- #### Vulkan-Headers

> **Note: This is not necessary if when you install the [LunarG Vulkan SDK](https://vulkan.lunarg.com/)**
> **, you also add `VULKAN_SDK` as an environmental**
> **variable specifiying the path. This is now assumed if buidling for macOS.**

This repository has a required dependency on the
[Vulkan Headers repository](https://github.com/KhronosGroup/Vulkan-Headers).
You must clone the headers repository and build its `install` target before
building this repository. The Vulkan-Headers repository is required because it
contains the Vulkan API definition files (registry) that are required to build
the samples. You must also take note of the headers' install
directory and pass it on the CMake command line for building this repository,
as described below. -->

#### glslang (not applicable for macOS)

This repository has a required dependency on the
[glslang repository](https://github.com/KhronosGroup/glslang).
The glslang repository is required because it contains components that are
required to build the samples. You must clone the glslang repository
and build its `install` target. Follow the build instructions in the glslang
[README.md](https://github.com/KhronosGroup/glslang/blob/master/README.md)
file. Ensure that the `update_glslang_sources.py` script has been run as part
of building glslang. You must also take note of the glslang install directory
and pass it on the CMake command line for building this repository, as
described below.

> **Note: There are some helpful comments in the set_env_vars.bat script for building and using debug and release versions that coexist at the**
> **same time.**

#### GLM

You can download GLM from [here](https://github.com/g-truc/glm/tags).

#### GLFW

You can download GLFW from [here](https://www.glfw.org/). This library is required to use
the [Dear ImGui](https://github.com/ocornut/imgui) debug UI. Eventually [Dear ImGui](https://github.com/ocornut/imgui) might become a dependecy.
> **Note: For macOS I had to clone the git repository and build a dynamic library that was**
> **version 3.3 which supports MoltenVK.**

#### ImGui

There currently is a repository dependency on the [dear imgui library](https://github.com/ocornut/imgui) in the cmake scripts. You need to clone the repository when running `CMake` (described later) add the absolute path of the cloned repository to either:
1. A command line arugment `-DIMGUI_REPO_DIR=absolute_path_to_repo_directory`
1. Or an environmental variable named `IMGUI_REPO_DIR`

After `CMake` has successfully setup the build files there are flags to turn off the `ImGui` UI layer entirely by commenting out the `USE_DEBUG_UI` in `Constants.h`. Making this flag dynamic through from the `CMake` scripts has not been done yet, but I plan on doing it and it should be simple. Unfortunately for now the [dear imgui library](https://github.com/ocornut/imgui) repo is a dependency.

> **Note: The latest known working commit of the [dear imgui library](https://github.com/ocornut/imgui) repo can be found in `current_versions.json`.**

#### MoltenVK (required for macOS only)

This repository has a required dependency (macOS only) on the
[MoltenVK repository](https://github.com/KhronosGroup/MoltenVK.git).
You have to clone the repository and build its `Packaging (macOS)` product before
building this repository. The MotenVK repository is required because it
contains a glsl to spir-v converter that the engine uses for hot swapping shaders during
runtime. This dependency will be removed soon (hopefully) because the necessary
dependency is actually [glslang](https://github.com/KhronosGroup/glslang), which comes with the macOS download of the [LunarG Vulkan SDK](https://vulkan.lunarg.com/) for macOS.

#### FMod (optional)

If you have want sound available during runtime the engine currently has some light hooks for the [FMod library](https://www.fmod.com/). There are two ways to add `Fmod` to the `CMake` build process:
1. Command line arugment `-DFMOD_DIR=absolute_path_to_sdk_directory`
1. Or by adding the absolute path of the SDK to an environmental variable named `FMOD_DIR`

>**Hint: Currently the FMod SDK directory the `CMake` module looks for contains the directories `api`, `bin`, `doc`, `plugins`, ...**



### Building Dependent Repositories with Known-Good Revisions

In the [Vulkan Samples repository](https://github.com/LunarG/VulkanSamples) they maintain a
[`known_good.json`](https://github.com/LunarG/VulkanSamples/blob/master/scripts/known_good.json)
file which has a list of commits that are known to work correctly. If the project is
throwing errors from the dependent projects I would try to rebuild them using those commits. I
have also added my own <code>current_versions.json</code> file where I try to keep a list of what version of things I am currently building the project with.

<!--
There is a Python utility script, `scripts/update_deps.py`, that you can use
to gather and build the dependent repositories mentioned above. This program
also uses information stored in the `scripts/known-good.json` file to checkout
dependent repository revisions that are known to be compatible with the
revision of this repository that you currently have checked out.

Here is a usage example for this repository:

    git clone https://github.com/LunarG/VulkanSamples.git
    cd VulkanSamples
    mkdir build
    cd build
    ../scripts/update_deps.py
    cmake -C helper.cmake ..
    cmake --build .

#### Notes

- You may need to adjust some of the CMake options based on your platform. See
  the platform-specific sections later in this document.
- The `update_deps.py` script fetches and builds the dependent repositories in
  the current directory when it is invoked. In this case, they are built in
  the `build` directory.
- The `build` directory is also being used to build this
  (VulkanSamples) repository. But there shouldn't be any conflicts
  inside the `build` directory between the dependent repositories and the
  build files for this repository.
- The `--dir` option for `update_deps.py` can be used to relocate the
  dependent repositories to another arbitrary directory using an absolute or
  relative path.
- The `update_deps.py` script generates a file named `helper.cmake` and places
  it in the same directory as the dependent repositories (`build` in this
  case). This file contains CMake commands to set the CMake `*_INSTALL_DIR`
  variables that are used to point to the install artifacts of the dependent
  repositories. You can use this file with the `cmake -C` option to set these
  variables when you generate your build files with CMake. This lets you avoid
  entering several `*_INSTALL_DIR` variable settings on the CMake command line.
- If using "MINGW" (Git For Windows), you may wish to run
  `winpty update_deps.py` in order to avoid buffering all of the script's
  "print" output until the end and to retain the ability to interrupt script
  execution.
- Please use `update_deps.py --help` to list additional options and read the
  internal documentation in `update_deps.py` for further information.

### Build Options

When generating native platform build files through CMake, several options can
be specified to customize the build. Some of the options are binary on/off
options, while others take a string as input. The following is a table of all
on/off options currently supported by this repository:

| Option | Platform | Default | Description |
| ------ | -------- | ------- | ----------- |
| BUILD_API_SAMPLES | All | `ON` | Controls whether or not the basic api samples are built. |
| BUILD_SAMPLE_LAYERS | All | `OFF` | Controls whether or not the Overlay sample layer is built.  The Overlay layer is currently deprcated and will not build. |

These variables should be set using the `-D` option when invoking CMake to
generate the native platform files. -->

## Building On Windows

### Windows Development Environment Requirements

- Windows
  - Any Personal Computer version supported by Microsoft
- Microsoft [Visual Studio](https://www.visualstudio.com/)
  - Versions
    <!-- - [2013 (update 4)](https://www.visualstudio.com/vs/older-downloads/)
    - [2015](https://www.visualstudio.com/vs/older-downloads/) -->
    - [2017](https://www.visualstudio.com/vs/downloads/)
    - [2019](https://www.visualstudio.com/vs/downloads/)
  - The Community Edition of each of the above versions is sufficient, as well as any more capable editions.
- [CMake](http://www.cmake.org/download/) (Version 2.8.11 or better)
  - Use the installer option to add CMake to the system PATH
- Git Client Support
  - [Git for Windows](http://git-scm.com/download/win) is a popular solution
    for Windows
  - Some IDEs (e.g., [Visual Studio](https://www.visualstudio.com/),
    [GitHub Desktop](https://desktop.github.com/))) have integrated
    Git client support

### Windows Build - Microsoft Visual Studio

The general approach is to run CMake to generate the Visual Studio project
files. Then either run CMake with the `--build` option to build from the
command line or use the Visual Studio IDE to open the generated solution and
work with the solution interactively.

#### Use `CMake` to Create the Visual Studio Project Files

Change your current directory to the root of the cloned repository directory,
create a build directory and generate the Visual Studio project files:

    cd GuppyVulkan
    mkdir build
    cd build
    cmake -A x64 -DGLM_LIB_DIR=absolute_path_to_install_dir \
                 -DGLSLANG_INSTALL_DIR=absolute_path_to_install_dir
                 -DGLFW_INCLUDE_DIR=absolute_path_to_include_dir \
                 -DGLFW_LIB=absolute_path_to_lib_file \
                 -DIMGUI_REPO_DIR=absolute_path_to_lib_file \
                 -DFMOD_DIR=absolute_path_to_lib_file \
                 ..

> **Note: The path delimeters must be forward slashes even on Windows.**
> The `..` parameter tells `cmake` the location of the root of the
> repository. If you place your build directory someplace else, you'll need to
> specify the location of the repository root differently.

The `-A` option is used to select either the "Win32" or "x64" architecture.

If a generator for a specific version of Visual Studio is required, you can
specify it for Visual Studio 2017, for example, with:

    64-bit: -G "Visual Studio 15 2017 Win64"
    32-bit: -G "Visual Studio 15 2017"

When generating the project files, the absolute path to a GLM
install directory must be provided. This can be done by setting the
`GLM_LIB_DIR` environment variable or by setting the
`GLM_LIB_DIR` CMake variable with the `-D` CMake option. In
either case, the variable should point to the installation directory of a
Vulkan-Headers repository built with the install target.

When generating the project files, the absolute path to a glslang install
directory must be provided. This can be done by setting the
`GLSLANG_INSTALL_DIR` environment variable or by setting the
`GLSLANG_INSTALL_DIR` CMake variable with the `-D` CMake option. In either
case, the variable should point to the installation directory of a glslang
repository built with the install target.

When generating the project file, the absolute path to a glfw include directory, and a glfw library file must be provided. This can be done by setting the `GLFW_INCLUDE_DIR` & `GLFW_LIB` environment variable or by setting the `GLFW_INCLUDE_DIR` & `GLFW_LIB` CMake variable with the -D CMake option. In either case, the variable should point to the installation directory of a glslang repository built with the install target.

> **Note: There are other similar `CMake` dependency flags that need to be set described in the section [Repository Set-Up](#repository-set-up)**

The above steps create a Windows solution file named
`GuppyVulkan.sln` in the build directory.

At this point, you can build the solution from the command line or open the
generated solution with Visual Studio.

#### Build the Solution From the Command Line

While still in the build directory:

    cmake --build .

to build the Debug configuration (the default), or:

    cmake --build . --config Release

to make a Release build.

#### Build the Solution With Visual Studio

Launch Visual Studio and open the `GuppyVulkan.sln` solution file
in the build folder. You may select "Debug" or "Release" from the Solution
Configurations drop-down list. Start a build by selecting the Build->Build
Solution menu item.

<!--## Building On Linux

### Linux Build Requirements

This repository has been built and tested on the two most recent Ubuntu LTS
versions. Currently, the oldest supported version is Ubuntu 14.04, meaning
that the minimum supported compiler versions are GCC 4.8.2 and Clang 3.4,
although earlier versions may work. It should be straightforward to adapt this
repository to other Linux distributions.

#### Required Package List

    sudo apt-get install git cmake build-essential libx11-xcb-dev \
        libxkbcommon-dev libwayland-dev libxrandr-dev

### Linux Build

The general approach is to run CMake to generate make files. Then either run
CMake with the `--build` option or `make` to build from the command line.


#### Use CMake to Create the Make Files

Change your current directory to the root of the cloned repository directory,
create a build directory and generate the make files.

    cd Vulkan-Samples
    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Debug \
          -DVULKAN_LOADER_INSTALL_DIR=absolute_path_to_install_dir \
          -DGLSLANG_INSTALL_DIR=absolute_path_to_install_dir \
          -DCMAKE_INSTALL_PREFIX=install ..

> Note: The `..` parameter tells `cmake` the location of the root of the
> repository. If you place your `build` directory someplace else, you'll need
> to specify the location of the repository root differently.

Use `-DCMAKE_BUILD_TYPE` to specify a Debug or Release build.

When generating the project files, the absolute path to a Vulkan-Loader
install directory must be provided. This can be done by setting the
`VULKAN_LOADER_INSTALL_DIR` environment variable or by setting the
`VULKAN_LOADER_INSTALL_DIR` CMake variable with the `-D` CMake option. In
either case, the variable should point to the installation directory of a
Vulkan-Headers repository built with the install target.

When generating the project files, the absolute path to a glslang install
directory must be provided. This can be done by setting the
`GLSLANG_INSTALL_DIR` environment variable or by setting the
`GLSLANG_INSTALL_DIR` CMake variable with the `-D` CMake option. In either
case, the variable should point to the installation directory of a glslang
repository built with the install target.

Note that if you don't want to use specific revisions of HEADERS, LOADER, 
and GLSLANG, the update_deps.py script mentioned above will handle all 
of the dependencies for you.

#### Build the Project

You can just run `make` to begin the build.

To speed up the build on a multi-core machine, use the `-j` option for `make`
to specify the number of cores to use for the build. For example:

    make -j4

You can also use

    cmake --build .

If your build system supports ccache, you can enable that via CMake option `-DUSE_CCACHE=On`

### Linux Notes

#### WSI Support Build Options

By default, the repository components are built with support for the
Vulkan-defined WSI display servers: Xcb, Xlib, and Wayland. It is recommended
to build the repository components with support for these display servers to
maximize their usability across Linux platforms. If it is necessary to build
these modules without support for one of the display servers, the appropriate
CMake option of the form `BUILD_WSI_xxx_SUPPORT` can be set to `OFF`.

## Building On Android

Install the required tools for Linux and Windows covered above, then add the
following.

- Build shaderc source code inside NDK
```java
$ cd ${ndk_root}/sources/third_party/shaderc
$ ../../../ndk-build NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=Android.mk APP_STL:=c++_static APP_ABI=all NDK_TOOLCHAIN_VERSION:=clang libshaderc_combined -j16
```
- Generate Android Studio Projects
```java
$ cd YOUR_DEV_DIRECTORY/VulkanSamples/API-Samples
$ cmake -DANDROID=ON -DABI_NAME=<armabi-v7a|arm64-v8a|...>
```
- Import VulkanSamples/API-Samples/android/build.gradle into Android Studio 2.3.0+.
- Or if building from a terminal:
```java
$ cd android
$ ./gradlew build
```
-->

## Building On Mac

### macOS Environment Requirements

These are what I know worked for me. Other things could work for you, but I do not
know for sure.

- macOS ~~Mojave~~ Catalina
- Xcode Version ~~10.2~~ 11.1
- [CMake](http://www.cmake.org/download/) (Version 3.13.4 or better)
  - Use the installer option to add CMake to the system PATH
- Python
  - The version built into macOS should work fine
- Git Client Support
  - Some IDEs (e.g., [Xcode](https://developer.apple.com/xcode/), [Visual Studio Code](https://code.visualstudio.com/download))
    have integrated support

### macOS Build - Xcode

The general approach is to run the CMake using the <code>buildMac.py</code> python
script to generate the Xcode project file. Then use the Xcode IDE to open the generated
project and work with the project interactively.

#### Use `Python` and `CMake` to Create the Xcode Project Files

Change your current directory to the root of the cloned repository directory,
create a build directory and generate the Xcode project files:

    cd GuppyVulkan
    mkdir build
    cd build
    python ../buildMac.py .. \
            -DMVK_PACKAGE_DIR=absolute_path_to_package_dir \
            -DGLM_LIB_DIR=absolute_path_to_install_dir \
            -DGLFW_INCLUDE_DIR=absolute_path_to_include_dir \
            -DGLFW_LIB=absolute_path_to_lib_file \
            -DIMGUI_REPO_DIR=absolute_path_to_lib_file \
            -DFMOD_DIR=absolute_path_to_lib_file \
            ..

> The `..` parameter tells `cmake` the location of the root of the
> repository. If you place your build directory somewhere other than `build`,
> you'll need to specify the location of the repository root differently.

When generating the project file, the absolute path to the [LunarG Vulkan SDK](https://vulkan.lunarg.com/)
install directory must be provided. This can be done by setting the
`VULKAN_SDK` environment variable. The variable should point to the installation
directory of the SDK.

When generating the project file, the absolute path to a MoltenVK
Package directory must be provided. This can be done by setting the
`MVK_PACKAGE_DIR` environment variable or by setting the
`MVK_PACKAGE_DIR` CMake variable with the `-D` CMake option. In
either case, the variable should point to the package directory of a
MoltenVK repository built with the `Packaging (macOS)` product.

When generating the project file, the absolute path to a GLM
install directory must be provided. This can be done by setting the
`GLM_LIB_DIR` environment variable or by setting the
`GLM_LIB_DIR` CMake variable with the `-D` CMake option. In
either case, the variable should point to the installation directory of a
Vulkan-Headers repository built with the install target.

When generating the project file, the absolute path to a glfw include
directory, and a glfw library file must be provided. This can be done by
setting the `GLFW_INCLUDE_DIR` & `GLFW_LIB` environment variable or by
setting the `GLFW_INCLUDE_DIR` & `GLFW_LIB` CMake variable with the
`-D` CMake option. In either case, the variable should point to the
installation directory of a glslang repository built with the install
target.

> **Note: There are other similar `CMake` dependency flags that need to be set described in the section [Repository Set-Up](#repository-set-up)**

> **<b>Note: I only had success with <code>.dylib</code> library files</b>**
> **<b>in gerenal. Build the libraries with the dynamic option if your not</b>**
> **<b>having success.</b>**

The above steps create a Xcode project file named
`GuppyVulkan.xcodeproj` in the build directory.

At this point, you can build the solution from the command line or open the
generated solution with Visual Studio.

#### Build the Project From the Command Line

Doesn't work yet...

#### Build the Project With Xcode

Launch Xcode and open the `GuppyVulkan.xcodeproj` project file
in the build folder. You may select "Debug" or "Release" from the <code>Guppy</code> product scheme editor in the info tab. Configurations drop-down list.
Start a build by selecting the <code>Guppy</code>
from the product drop down and hitting &#8984;B or &#8984;R.
