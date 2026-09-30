# 快速入門指南 (Quick Start Guide)

## 系統要求

### macOS
```bash
# 安裝依賴（使用 Homebrew）
brew install glfw3 glew glm opencv
```

### Windows
- Visual Studio 2019 或更新版本
- OpenGL 3.3+ 支持的顯卡
- 驅動程序需要更新

### Linux
```bash
# Ubuntu/Debian
sudo apt-get install libglfw3-dev libglew-dev libglm-dev libopencv-dev

# Fedora
sudo dnf install glfw-devel glew-devel glm-devel opencv-devel
```

## 編譯和運行

### macOS/Linux 使用 Makefile

```bash
# 進入項目目錄
cd CG_HW3_2/CG_HW3

# 編譯
make

# 運行
./CG_HW3
```

### Windows 使用 Visual Studio

1. 打開 `CG_HW3.sln`
2. 選擇 `Release` 或 `Debug` 配置
3. 構建解決方案（Ctrl + Shift + B）
4. 運行項目（F5）

## 文件結構解析

```
CG_HW3/
├── CG_HW3.cpp              # 主程序入口和渲染循環
├── headers.h               # 公共頭文件和依賴
├── camera.h/cpp            # 攝像機實現
├── light.h                 # 光源類（方向光、點光、聚光燈）
├── material.h              # 材質屬性（Phong 模型）
├── imagetexture.h/cpp      # 紋理加載和管理
├── shaderprog.h/cpp        # 著色器程序管理
├── trianglemesh.h/cpp      # 3D 網格加載和渲染
├── skybox.h/cpp            # 天空盒實現
├── shaders/                # GLSL 著色器
│   ├── phong.vert
│   ├── phong.frag
│   ├── skybox.vert
│   ├── skybox.frag
│   └── fillcolor.frag
├── textures/               # 紋理資源
├── Makefile                # 編譯配置
└── CG_HW3.vcxproj          # Visual Studio 項目文件

TestModels_HW3/            # 示例 3D 模型（OBJ 格式）
TestTextures_HW3/          # 示例紋理（PNG/JPG）
Library/                   # 第三方庫
```

## 核心代碼流程

### 初始化階段

```cpp
int main() {
    // 1. 初始化 GLFW 和 OpenGL
    glutInit();
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutCreateWindow("Lighting and Texturing");
    
    // 2. 設置 GLEW
    glewInit();
    
    // 3. 創建攝像機
    CreateCamera();
    
    // 4. 加載著色器
    CreateShaderLib();
    
    // 5. 加載 3D 模型
    LoadObjects("path/to/model.obj");
    
    // 6. 創建天空盒
    CreateSkybox("path/to/skybox/");
    
    // 7. 初始化光源
    dirLight = new DirectionalLight(...);
    pointLight = new PointLight(...);
    spotLight = new SpotLight(...);
    
    // 8. 設置回調函數
    glutDisplayFunc(RenderSceneCB);
    glutKeyboardFunc(ProcessKeysCB);
    glutReshapeFunc(ReshapeCB);
    
    // 9. 進入主循環
    glutMainLoop();
}
```

### 渲染循環

```cpp
void RenderSceneCB() {
    // 1. 清空緩衝區
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // 2. 使用著色器
    glUseProgram(phongShaderProgram->GetProgramId());
    
    // 3. 更新著色器統一變量
    UpdateShaderUniforms(phongShaderProgram);
    
    // 4. 綁定紋理
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, material->GetTexture());
    
    // 5. 設置模型視圖矩陣
    glm::mat4 mvpMatrix = 
        camera->GetProjectionMatrix() * 
        camera->GetViewMatrix() * 
        sceneObj.worldMatrix;
    
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, glm::value_ptr(mvpMatrix));
    
    // 6. 繪製網格
    mesh->Draw();
    
    // 7. 繪製光源可視化
    DrawLightVisualization();
    
    // 8. 繪製天空盒
    skybox->Draw();
    
    // 9. 交換緩衝區
    glutSwapBuffers();
}
```

## 常見操作

### 加載自定義模型

```cpp
// 在 CG_HW3.cpp 中修改加載路徑
LoadObjects("path/to/your/model.obj");

// 設置材質和紋理
PhongMaterial* material = new PhongMaterial();
material->SetKa(glm::vec3(0.1f, 0.1f, 0.1f));
material->SetKd(glm::vec3(0.8f, 0.8f, 0.8f));
material->SetKs(glm::vec3(1.0f, 1.0f, 1.0f));
material->SetNs(32.0f);
material->SetMapKd(ImageTexture::LoadImageTexture("path/to/texture.png"));
```

### 調整光源參數

```cpp
// 平行光
dirLight->SetDirection(glm::vec3(1.0f, 1.0f, -1.0f));
dirLightRadiance = glm::vec3(0.8f, 0.8f, 0.8f);

// 點光源
pointLight->SetPosition(glm::vec3(1.0f, 2.0f, 1.0f));
pointLightIntensity = glm::vec3(1.0f, 0.8f, 0.5f);

// 聚光燈
spotLight->SetPosition(glm::vec3(0.0f, 3.0f, 0.0f));
spotLight->SetDirection(glm::vec3(0.0f, -1.0f, 0.0f));
spotLightCutoffStartInDegree = 25.0f;
spotLightTotalWidthInDegree = 40.0f;
```

### 修改攝像機

```cpp
// 調整視場角
fovy = 45.0f;  // 度數

// 移動攝像機
cameraPos = glm::vec3(0.0f, 1.5f, 5.0f);
cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

// 調整裁剪平面
zNear = 0.1f;
zFar = 1000.0f;
```

## 著色器文件解析

### phong.vert（頂點著色器）

關鍵操作：
- 將頂點位置變換到世界空間和裁剪空間
- 計算法向量的世界空間表示
- 插值紋理坐標到片段著色器

```glsl
void main() {
    fragPos = vec3(worldMatrix * vec4(vertexPosition, 1.0));
    normal = normalize(mat3(transpose(inverse(worldMatrix))) * vertexNormal);
    texCoord = vertexTexCoord;
    
    gl_Position = projMatrix * viewMatrix * vec4(fragPos, 1.0);
}
```

### phong.frag（片段著色器）

關鍵操作：
- 採樣紋理顏色
- 計算三種光源的貢獻
- 組合最終顏色

```glsl
void main() {
    // 採樣紋理
    vec4 texColor = texture(texSampler, texCoord);
    vec3 texDiffuse = texColor.rgb;
    
    // 計算光照
    vec3 ambient = ambientLight * texDiffuse;
    vec3 dirLight = CalcDirLight(...);
    vec3 pointLight = CalcPointLight(...);
    vec3 spotLight = CalcSpotLight(...);
    
    // 組合結果
    vec3 result = ambient + dirLight + pointLight + spotLight;
    fragColor = vec4(result, 1.0);
}
```

## 性能調優建議

### 基礎優化
1. **減少著色器複雜度**：移除不必要的計算
2. **使用低分辨率紋理**：測試階段使用 512x512 或更小的紋理
3. **批量渲染**：合併相同著色器的物體

### 進階優化
1. **使用 VAO（頂點數組對象）**：減少重複的頂點屬性設置
2. **實現光源數量限制**：同時最多使用 3-5 個光源
3. **使用 FBO（幀緩衝對象）**：進行離屏渲染

## 除錯技巧

### 檢查著色器錯誤

```cpp
// 在 shaderprog.cpp 中啟用詳細日誌
GLint success;
glGetProgramiv(program, GL_LINK_STATUS, &success);
if (!success) {
    char infoLog[512];
    glGetProgramInfoLog(program, 512, nullptr, infoLog);
    std::cerr << "Shader linking failed: " << infoLog << std::endl;
}
```

### 檢查紋理加載

```cpp
// 在 imagetexture.cpp 中添加驗證
if (img.empty()) {
    std::cerr << "Failed to load image: " << filename << std::endl;
    return nullptr;
}
```

### 檢查模型加載

```cpp
// 在 trianglemesh.cpp 中添加頂點數檢查
std::cout << "Loaded mesh with " 
          << vertexPositions.size() << " vertices" << std::endl;
```

## 常見問題

### Q1：黑屏或無法渲染
**A**：
- 檢查著色器是否編譯成功（查看控制台錯誤信息）
- 驗證模型文件路徑正確
- 確認 OpenGL 上下文正確創建

### Q2：紋理顯示不正確
**A**：
- 檢查紋理座標是否正確加載（打印調試）
- 驗證紋理文件格式（支持 PNG、JPG）
- 確認顏色空間轉換正確（BGR→RGBA）

### Q3：光照計算異常
**A**：
- 驗證法向量計算（使用可視化調試）
- 檢查光源位置和方向是否設置正確
- 確認著色器統一變量正確傳遞

### Q4：性能低下
**A**：
- 減少紋理分辨率
- 減少模型複雜度
- 使用著色器優化技術

## 進階項目擴展

### 添加法線貼圖
```glsl
// 在 phong.frag 中
vec3 normalFromMap = texture(normalMap, texCoord).rgb;
normalFromMap = normalize(normalFromMap * 2.0 - 1.0);
normal = normalize(TBN * normalFromMap);
```

### 添加 PBR（基於物理的渲染）
```glsl
// 使用 Roughness 和 Metallic 參數
vec3 F0 = mix(vec3(0.04), albedo, metallic);
// ... Cook-Torrance 微平面模型
```

### 添加陰影映射
```glsl
// 使用深度圖計算陰影
float shadow = texture(shadowMap, shadowCoord).r > depth ? 1.0 : 0.0;
result *= (1.0 + 0.5 * shadow);
```

## 參考資源

- **OpenGL 官方文檔**：https://www.khronos.org/opengl/
- **GLM 庫文檔**：https://glm.g-truc.net/
- **LearnOpenGL 教程**：https://learnopengl.com/
- **著色器調試工具**：RenderDoc, NVIDIA NSight

---

祝編碼愉快！如有問題，請查看完整的技術實踐指南。
