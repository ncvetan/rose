# Rose - A Graphics Engine

The primary goal when creating this engine was to learn about graphics programming techniques

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

## Details

Some features that Rose supports includes...

- Phong illumination
- Skyboxes
- Shadow mapping
- normal & parallax mapping
- HDR

Rose uses a mix of deferred and forward rendering techniques

## Resources

Some of the resources used that helped me develop Rose. I'd like to give my thanks to the authors of these resources...

- [LearnOpenGL](https://learnopengl.com/)
- [Real Time Rendering](https://www.realtimerendering.com/)
- [A Primer of Efficient Rendering Algorithms & Clustered Shading](https://www.aortiz.me/2018/12/21/CG.html)
- [Clustered Shading](https://github.com/DaveH355/clustered-shading)
