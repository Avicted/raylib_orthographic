# Transport Tycoon style engine example
- 2D Orthographic camera
- Frustum culling
- Batch rendering

### Build and Run
```bash
clear
meson setup build --buildtype=debug
cd build && ninja
cd ..
./build/raylib_orthographic
```


![demo](resources/output.gif "output.gif")

### License
MIT License