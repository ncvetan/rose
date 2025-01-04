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

## Todo

Some known bugs as well as future features...

- fix GUI bugs, especially within the viewport window
- automatic exposure adjustment

## Resources

Some of the resources used that helped me develop Rose...

- LearnOpenGl
- Real Time Rendering
- [A Primer of Efficient Rendering Algorithms & Clustered Shading](https://www.aortiz.me/2018/12/21/CG.html)
