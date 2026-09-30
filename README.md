# Lighting and Texturing

Advanced Computer Graphics project implementing realistic lighting and texture rendering using OpenGL and GLSL.

## 📋 Overview

This project demonstrates modern 3D graphics techniques including:
- **Multiple Light Types**: Directional lights, point lights, and spot lights
- **Phong Shading Model**: Professional lighting calculations for realistic material rendering
- **Texture Mapping**: Image-based texturing with UV coordinate support
- **Skybox Rendering**: Immersive environment mapping
- **Interactive Scene Control**: Real-time parameter adjustment for lights and camera

## ✨ Features

### Lighting System
- **Directional Light**: Parallel light rays simulating sun-like illumination
- **Point Light**: Omnidirectional light source with distance attenuation
- **Spot Light**: Cone-shaped light with adjustable cutoff angles
- **Ambient Lighting**: Global illumination for realistic shadow areas

### Material & Texture System
- **Phong Material Properties**:
  - Ambient coefficient (Ka)
  - Diffuse coefficient (Kd) with texture mapping support
  - Specular coefficient (Ks)
  - Specular exponent (Ns) for surface roughness
- **Image Texturing**: Load and apply 2D textures to 3D models
- **Skybox Environment**: Cubemap-based background with rotation control

### Interactive Features
- Real-time light position and direction adjustment
- Toggleable light sources for effect comparison
- Camera control with free navigation
- Model loading and switching
- Light parameter visualization

## 🎮 Controls

### Camera Movement
- `W` / `↑` - Move forward
- `S` / `↓` - Move backward
- `A` / `←` - Move left
- `D` / `→` - Move right

### Light Control
- `1` - Toggle Directional Light
- `2` - Toggle Point Light
- `3` - Toggle Spot Light
- `Q`/`E` - Move point light left/right
- `Z`/`X` - Move point light up/down
- `C`/`V` - Move point light forward/backward
- `U`/`I` - Move spot light left/right
- `O`/`P` - Move spot light up/down

### Skybox Control
- `[` / `]` - Rotate skybox

### Application
- `M` - Toggle model menu
- `N` - Load next model
- `ESC` - Exit

## 🏗️ Project Structure

```
├── CG_HW3/                      # Main source directory
│   ├── CG_HW3.cpp              # Main application and rendering loop
│   ├── camera.h/cpp            # Camera system with view matrix calculations
│   ├── light.h                 # Light class hierarchy (Directional/Point/Spot)
│   ├── material.h              # Material properties and Phong shading
│   ├── imagetexture.h/cpp      # Texture loading and management
│   ├── shaderprog.h/cpp        # Shader program wrapper and compilation
│   ├── trianglemesh.h/cpp      # Mesh loading and geometry management
│   ├── skybox.h/cpp            # Cubemap skybox implementation
│   ├── headers.h               # Common includes and dependencies
│   ├── shaders/                # GLSL shader programs
│   │   ├── phong.vert          # Phong shading vertex shader
│   │   ├── phong.frag          # Phong shading fragment shader
│   │   ├── skybox.vert         # Skybox vertex shader
│   │   ├── skybox.frag         # Skybox fragment shader
│   │   └── fillcolor.frag      # Simple color filling shader
│   └── textures/               # Texture resources
├── TestModels_HW3/             # 3D model test data (OBJ format)
├── TestTextures_HW3/           # Texture image files (PNG, JPG)
├── Library/                    # External libraries (OpenGL, GLM, OpenCV)
├── Makefile                    # Build configuration
└── README.md                   # This file
```

## 🛠️ Technical Architecture

### Shader System
The project uses a modular shader architecture with two main rendering pipelines:

#### Phong Shading Pipeline
Implements the Phong reflection model for realistic material rendering:

**Vertex Shader**:
- Transforms vertices to world space and clip space
- Calculates per-vertex normal transformations
- Passes texture coordinates for fragment shader

**Fragment Shader**:
- Calculates ambient, diffuse, and specular components
- Supports multiple simultaneous light sources
- Applies texture mapping to diffuse component
- Outputs final fragment color

#### Skybox Rendering
- Uses cubemap textures for immersive backgrounds
- Renders after main scene to optimize depth testing
- Supports real-time rotation for dynamic environment

### Material System
Materials encapsulate surface properties:
- **PhongMaterial**: Stores Ka, Kd, Ks, Ns coefficients and texture reference
- Allows per-model material customization
- Supports both solid colors and image textures

### Lighting Calculations

#### Ambient Lighting
```
I_ambient = Ka * I_light
```

#### Diffuse Lighting
```
I_diffuse = Kd * I_light * max(0, N·L)
```

#### Specular Lighting (Phong Model)
```
I_specular = Ks * I_light * max(0, R·V)^Ns
```

Where:
- N: Surface normal
- L: Light direction
- R: Reflected light direction
- V: View direction
- Ns: Specular exponent (surface roughness)

#### Attenuation for Point Lights
```
attenuation = 1.0 / (1.0 + 0.09*d + 0.032*d²)
```
Where d is the distance from light to surface.

#### Spot Light Falloff
- Inner cone: Full intensity
- Outer cone: Smooth falloff using cosine interpolation
- Parameters: cutoffStartDeg, cutoffTotalDeg

### Camera System
- **Perspective Projection**: Configurable FOV, near/far planes
- **Free Camera**: Position and target-based view matrix calculation
- **Real-time Navigation**: Keyboard-driven camera movement

### Texture Management
- **Image Loading**: Supports PNG and JPG via OpenCV
- **GPU Texture Objects**: Efficient texture binding and sampling
- **UV Coordinate Mapping**: Per-vertex texture coordinates from OBJ files
- **Mipmapping**: Automatic generation for texture filtering

### Mesh Management
- **OBJ File Loading**: Parses vertex positions, normals, and texture coordinates
- **Geometric Data Structure**: Organized vertex/index buffers for efficient rendering
- **GPU Buffering**: VBO/EBO for optimal draw performance

## 🔧 Building and Running

### Prerequisites
- C++ compiler (GCC/Clang on macOS/Linux, MSVC on Windows)
- OpenGL 3.3+
- CMake or Make build system
- External libraries:
  - GLM (header-only math library)
  - GLEW (OpenGL extension wrangling)
  - GLFW3 (window and input handling)
  - OpenCV (image loading)
  - FreeGLUT (legacy OpenGL utilities)

### Build Instructions

**Using Makefile (macOS/Linux)**:
```bash
cd CG_HW3
make
./CG_HW3
```

**Using Visual Studio (Windows)**:
```bash
# Open CG_HW3.sln in Visual Studio
# Build solution
# Run CG_HW3 project
```

### Dependencies Configuration

Ensure the Library directory contains:
- OpenCV headers and libraries
- GLEW headers and libraries
- GLM header files
- Other required OpenGL libraries

## 📚 Implementation Details

### Phong Shading Shader

The core lighting calculation in `shaders/phong.frag`:

```glsl
// Directional Light
vec3 dirLight = CalcDirLight(dirLight, normal, viewDir);

// Point Light with attenuation
vec3 pointLight = CalcPointLight(pointLight, normal, fragPos, viewDir);

// Spot Light with cone falloff
vec3 spotLight = CalcSpotLight(spotLight, normal, fragPos, viewDir);

// Combine all contributions
vec3 result = ambientLight + dirLight + pointLight + spotLight;
```

### Texture Coordinate Handling

Vertices include UV coordinates that map 2D textures onto 3D surfaces:
- Loaded from OBJ files (format: vt u v)
- Interpolated across surfaces via vertex shader
- Sampled in fragment shader for color lookup

### Real-time Parameter Adjustment

Interactive scene control modifies shader uniforms at runtime:
- Light positions/directions updated every frame
- Material properties can be toggled
- Camera transformation recalculated on input

## 🎯 Use Cases

- **Education**: Learning computer graphics fundamentals
- **Game Development**: Foundation for game engine rendering
- **3D Visualization**: Professional 3D model presentation
- **Research**: Testing new lighting algorithms and techniques

## 📝 Technical Best Practices

### Code Organization
- **Separation of Concerns**: Each class handles specific functionality (Mesh, Light, Material, Camera, Shader)
- **Header/Implementation Split**: Interface clarity and compilation efficiency
- **Resource Management**: Proper GPU memory allocation and cleanup

### Performance Considerations
- **GPU Buffering**: Vertex/Index buffers stored in VRAM for fast access
- **Shader Compilation**: Cached shader programs avoid redundant compilation
- **Texture Caching**: Textures loaded once and reused
- **Efficient Lighting**: Per-fragment calculations balanced for visual quality

### Shader Development
- **Modular Shader Functions**: Separate functions for each light type
- **Inline Comments**: Complex calculations explained in shader code
- **Optimization**: Minimized GPU-CPU data transfers

### Error Handling
- **Shader Compilation Checks**: Validate shader programs at runtime
- **File Loading Verification**: Graceful handling of missing resources
- **Buffer Creation Validation**: Ensure GPU memory allocation success

## 🚀 Future Enhancements

- **Advanced Shading Models**: PBR (Physically Based Rendering)
- **Normal Mapping**: Per-pixel surface detail
- **Shadow Mapping**: Real-time shadow rendering
- **Deferred Rendering**: Efficient multi-light rendering
- **Post-Processing Effects**: Bloom, depth of field, tone mapping
- **GPU Particle System**: Dynamic particles and effects

## 📄 License

This project is provided as-is for educational purposes.

## 👨‍💻 Author

Created as part of Computer Graphics coursework demonstrating modern rendering techniques.

## 📞 Support

For questions or issues, refer to the technical documentation or review the source code comments for implementation details.

---

**Last Updated**: 2025-12-25  
**OpenGL Version**: 3.3+  
**Language**: C++ with GLSL Shaders
