# Transport Tycoon style engine example
- Perspective Camera3D, that looks like an orthographic camera
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

## Resources
- [https://kenney.nl/assets/train-kit](https://kenney.nl/assets/train-kit)

### License
MIT License