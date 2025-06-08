# Rose - A Graphics Engine

## Introduction

The goal of this project is to provide myself with a platform to learn and experiment with different graphics techniques. When I started Rose, my knowledge of computer graphics was limited. I still have plently to learn, but have been able to grow immensely throughout the development of this project. 

## Installation

Rose makes use of Git Submodules, so ensure that you perform a recursive clone...

```
git clone --recurse-submodules <repo-URL>
```

To build the project, use CMake...

```
mkdir build
cd build
cmake <path to top level CMakeLists.txt>
cmake --build .
```

The graphics API used can be selected through rose/CMakeLists.txt. However, support is limited to OpenGL at this time.

## Features

Some features that Rose supports...

- Phong illumination
- Skyboxes
- Cascaded shadow mapping
- Normal & parallax mapping
- Clustered deferred rendering
- Bloom
- HDR

## Roadmap

Some future features and improvements to be made...

- Linux support (there is very little right now that isn't compatible with Linux, just a few small changes need to be made)
- Support for additional graphics APIs
- Improved bloom algorithm
- PBR
- Ambient Occlusion
- And plenty of improvements to existing systems...

## Contributing

Contributions are welcome. Feel free to fork the repository and submit a pull request.

## Resources

Some of the resources used that helped me develop Rose. I'd like to give a huge thanks to the authors of these resources...

- [LearnOpenGL](https://learnopengl.com/)
- [Real Time Rendering](https://www.realtimerendering.com/)
- [A Primer of Efficient Rendering Algorithms & Clustered Shading](https://www.aortiz.me/2018/12/21/CG.html)
- [Clustered Shading](https://github.com/DaveH355/clustered-shading)
- [Shadow Mapping](https://alextardif.com/shadowmapping.html)
- [Bloom](https://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare/)
