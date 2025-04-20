# Rose - A Graphics Engine

The goal when creating this engine was to drive my learning about computer graphics techniques. 

## Installation

This project makes use of Git Submodules, so ensure that you perform a recursive clone...

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

## Features

Some features that Rose supports includes...

- Phong illumination
- Skyboxes
- Cascaded shadow mapping
- normal & parallax mapping
- Clustered deferred rendering
- Bloom

## Roadmap

Some future features and improvements to be made...

- Linux support
- Better support for multiple graphics APIs
- PBR
- AO
- And plenty of improvements to existing systems...

## Contributing

Contributions are welcome. Feel free to fork the repository and submit a pull request.

## Resources

Some of the resources used that helped me develop Rose. I'd like to give my thanks to the authors of these resources...

- [LearnOpenGL](https://learnopengl.com/)
- [Real Time Rendering](https://www.realtimerendering.com/)
- [A Primer of Efficient Rendering Algorithms & Clustered Shading](https://www.aortiz.me/2018/12/21/CG.html)
- [Clustered Shading](https://github.com/DaveH355/clustered-shading)
- [Shadow Mapping](https://alextardif.com/shadowmapping.html)
