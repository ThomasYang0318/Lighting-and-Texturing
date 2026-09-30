# 技術實踐方法指南

本文檔詳細記錄了 Lighting and Texturing 項目中的核心技術實踐方法。

## 1. 光照系統的實踐方法

### 1.1 Phong 反射模型實現

Phong 光照模型是本項目的核心，它將光照分為三個主要成分：

#### 環境光 (Ambient Light)
```
I_ambient = Ka · L_ambient
```
- **Ka**：環境光反射係數 (Ambient Coefficient)
- **L_ambient**：全局環境光強度
- 實現目的：避免陰影區域過暗

#### 漫反射 (Diffuse Reflection)
```
I_diffuse = Kd · L_light · max(0, N·L)
```
- **Kd**：漫反射係數 (Diffuse Coefficient)
- **N**：表面法向量 (Surface Normal)
- **L**：指向光源的方向向量
- **L_light**：光源強度
- 實現細節：
  - 在片段着色器中計算
  - 支持紋理貼圖：使用紋理採樣替代純色 Kd
  - N·L < 0 時設為 0（背光面）

#### 高光 (Specular Highlights)
```
I_specular = Ks · L_light · max(0, R·V)^Ns
```
- **Ks**：高光反射係數 (Specular Coefficient)
- **R**：反射光方向（R = 2(N·L)N - L）
- **V**：視向量（指向攝像機方向）
- **Ns**：高光指數（控制高光尖銳度）
- 物理意義：
  - Ns 越大 → 表面越光滑 → 高光越尖銳
  - Ns = 32 → 光滑塑料
  - Ns = 128 → 光滑金屬

### 1.2 多光源處理

項目支持三種不同光源類型的同時計算：

#### 平行光 (Directional Light)
```cpp
struct DirectionalLight {
    glm::vec3 direction;  // 光源方向
    glm::vec3 radiance;   // 光源輻射度
};
```
- 計算方法：光方向對所有像素相同
- 應用場景：模擬太陽光
- 實現技巧：
  ```glsl
  vec3 CalcDirLight(DirectionalLight light, 
                     vec3 normal, 
                     vec3 viewDir,
                     vec3 texColor) {
      vec3 lightDir = normalize(-light.direction);
      
      // 漫反射
      float diff = max(dot(normal, lightDir), 0.0);
      vec3 diffuse = light.radiance * diff * texColor;
      
      // 高光
      vec3 reflectDir = reflect(-lightDir, normal);
      float spec = pow(max(dot(viewDir, reflectDir), 0.0), Ns);
      vec3 specular = light.radiance * spec * Ks;
      
      return diffuse + specular;
  }
  ```

#### 點光源 (Point Light)
```cpp
struct PointLight : public Light {
    glm::vec3 position;   // 光源位置
    glm::vec3 intensity;  // 光源強度
};
```
- 衰減公式：
  ```
  attenuation = 1.0 / (1.0 + 0.09·d + 0.032·d²)
  ```
  - d 是表面到光源的距離
  - 二次衰減符合物理規律（平方反比律）
  
- 實現技巧：
  ```glsl
  vec3 CalcPointLight(PointLight light,
                       vec3 normal,
                       vec3 fragPos,
                       vec3 viewDir,
                       vec3 texColor) {
      vec3 lightDir = normalize(light.position - fragPos);
      
      // 計算衰減
      float distance = length(light.position - fragPos);
      float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
      
      // 漫反射和高光計算
      float diff = max(dot(normal, lightDir), 0.0);
      vec3 diffuse = light.intensity * diff * texColor * attenuation;
      
      vec3 reflectDir = reflect(-lightDir, normal);
      float spec = pow(max(dot(viewDir, reflectDir), 0.0), Ns);
      vec3 specular = light.intensity * spec * Ks * attenuation;
      
      return diffuse + specular;
  }
  ```

#### 聚光燈 (Spot Light)
```cpp
struct SpotLight : public PointLight {
    glm::vec3 direction;      // 光錐軸向
    float cutoffStartDeg;     // 內錐角度
    float cutoffTotalDeg;     // 外錐角度
};
```
- 錐角衰減：
  ```
  theta = arccos(L·D)  // L: 指向光源的方向，D: 光錐軸向
  
  if theta < cutoffStart:
      intensity_factor = 1.0
  else if theta < cutoffStart + (cutoffTotal - cutoffStart)/2:
      intensity_factor = cos((theta - cutoffStart) / delta) ^ 2
  else:
      intensity_factor = 0.0
  ```

- 實現技巧：
  ```glsl
  vec3 CalcSpotLight(SpotLight light,
                      vec3 normal,
                      vec3 fragPos,
                      vec3 viewDir,
                      vec3 texColor) {
      // 計算聚光燈錐角衰減
      vec3 lightDir = normalize(light.position - fragPos);
      float theta = dot(lightDir, normalize(-light.direction));
      
      float epsilon = cos(radians(cutoffStart)) - cos(radians(cutoffStart + cutoffWidth));
      float intensity = clamp((theta - cos(radians(cutoffStart + cutoffWidth))) / epsilon,
                              0.0, 1.0);
      
      // 距離衰減
      float distance = length(light.position - fragPos);
      float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
      
      // 組合計算（同點光源邏輯，但乘以 intensity 衰減因子）
      return (diffuse + specular) * attenuation * intensity;
  }
  ```

### 1.3 光源實時調整

項目支持在運行時實時調整光源參數：

```cpp
// 光源位置移動
pointLight->MoveLeft(lightMoveSpeed);    // 沿 X 軸負方向移動
pointLight->MoveRight(lightMoveSpeed);   // 沿 X 軸正方向移動
pointLight->MoveUp(lightMoveSpeed);      // 沿 Y 軸正方向移動
pointLight->MoveDown(lightMoveSpeed);    // 沿 Y 軸負方向移動
pointLight->MoveForward(lightMoveSpeed); // 沿 Z 軸負方向移動
pointLight->MoveBackward(lightMoveSpeed);// 沿 Z 軸正方向移動
```

**最佳實踐**：
- 每個光源移動速度：0.2 單位/幀（根據場景尺度調整）
- 輸入響應式更新：直接修改光源參數，無需重新編譯着色器
- 著色器統一變量：傳遞光源參數到 GPU（最多同時支持 3-5 個光源）

---

## 2. 紋理映射系統

### 2.1 紋理坐標系統

紋理映射的核心是建立 3D 表面點到 2D 紋理圖像的映射關係。

#### 紋理坐標定義
```cpp
struct VertexPNT {
    glm::vec3 position;        // 3D 位置
    glm::vec3 normal;          // 法向量
    glm::vec2 texCoord;        // UV 紋理坐標
};
```

- **U 軸**：紋理水平方向 (0.0 → 1.0 = 左 → 右)
- **V 軸**：紋理垂直方向 (0.0 → 1.0 = 下 → 上)
- **UV 原點**：紋理左下角 (0, 0)

#### OBJ 文件格式中的紋理坐標
```
vt 0.0 0.0
vt 1.0 0.0
vt 0.5 1.0
...
f 1/1 2/2 3/3  # 頂點索引/紋理坐標索引
```

### 2.2 紋理加載和 GPU 綁定

#### 圖像加載流程
```cpp
ImageTexture* ImageTexture::LoadImageTexture(const std::string& filename) {
    // 1. 使用 OpenCV 讀取圖像
    cv::Mat img = cv::imread(filename);
    
    // 2. 翻轉 Y 軸（OpenCV 和 OpenGL 坐標系不同）
    cv::flip(img, img, 0);
    
    // 3. 轉換顏色空間（BGR → RGBA）
    cv::Mat imgRGBA;
    cv::cvtColor(img, imgRGBA, cv::COLOR_BGR2RGBA);
    
    // 4. 創建 OpenGL 紋理對象
    GLuint texId;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    
    // 5. 上傳紋理數據到 GPU
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 
                 img.cols, img.rows, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, imgRGBA.data);
    
    // 6. 設置紋理參數
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    // 7. 生成 Mipmap
    glGenerateMipmap(GL_TEXTURE_2D);
    
    return new ImageTexture(texId, img.cols, img.rows);
}
```

**最佳實踐**：
- 使用 RGBA 格式保持透明度信息
- 啟用 Mipmap 優化遠距離紋理採樣
- 設置 GL_REPEAT 模式處理邊界情況
- 線性插值提高紋理質量

### 2.3 著色器中的紋理採樣

```glsl
// 頂點著色器
in vec3 vertexPosition;
in vec3 vertexNormal;
in vec2 vertexTexCoord;

out vec3 fragPos;
out vec3 normal;
out vec2 texCoord;

void main() {
    fragPos = vec3(worldMatrix * vec4(vertexPosition, 1.0));
    normal = normalize(mat3(transpose(inverse(worldMatrix))) * vertexNormal);
    texCoord = vertexTexCoord;
    
    gl_Position = projMatrix * viewMatrix * vec4(fragPos, 1.0);
}

// 片段著色器
uniform sampler2D textureSampler;

in vec2 texCoord;

void main() {
    vec4 texColor = texture(textureSampler, texCoord);
    
    // 使用紋理顏色替代純色 Kd
    vec3 diffuse = Kd * texColor.rgb * lightIntensity;
    
    fragColor = vec4(diffuse, texColor.a);
}
```

### 2.4 紋理坐標補插

著色器自動進行紋理坐標的透視投影正確補插：

```glsl
// 頂點著色器中聲明為 out
out vec2 texCoord;

// 光柵化階段：自動線性插值
// 片段著色器中自動獲得插值後的座標
in vec2 texCoord;  // 正確映射到表面的紋理座標
```

---

## 3. 天空盒實現

### 3.1 Cubemap 紋理

天空盒使用立方體貼圖（Cubemap），包含 6 個面：
- 前 (Front / +Z)
- 後 (Back / -Z)
- 左 (Left / -X)
- 右 (Right / +X)
- 上 (Top / +Y)
- 下 (Bottom / -Y)

#### 立方體貼圖加載
```cpp
void Skybox::LoadSkybox(const std::string& skyboxDir) {
    const std::string faces[6] = {
        skyboxDir + "right.jpg",
        skyboxDir + "left.jpg",
        skyboxDir + "top.jpg",
        skyboxDir + "bottom.jpg",
        skyboxDir + "front.jpg",
        skyboxDir + "back.jpg"
    };
    
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureId);
    
    // 加載每個面
    for (int i = 0; i < 6; i++) {
        cv::Mat img = cv::imread(faces[i]);
        cv::flip(img, img, 0);
        cv::cvtColor(img, img, cv::COLOR_BGR2RGBA);
        
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA,
                     img.cols, img.rows, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, img.data);
    }
    
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}
```

### 3.2 天空盒着色器

```glsl
// 頂點著色器
in vec3 vertexPosition;
out vec3 skyboxTexCoord;

uniform mat4 projMatrix;
uniform mat4 viewMatrixNoTranslation;

void main() {
    // 移除平移分量，只保留旋轉
    mat4 viewNoTrans = viewMatrixNoTranslation;
    viewNoTrans[3] = vec4(0, 0, 0, 1);
    
    vec4 pos = projMatrix * viewNoTrans * vec4(vertexPosition, 1.0);
    gl_Position = pos.xyww;  // 深度始終為 1.0（最遠）
    
    skyboxTexCoord = vertexPosition;
}

// 片段著色器
in vec3 skyboxTexCoord;
uniform samplerCube skyboxTexture;

void main() {
    fragColor = texture(skyboxTexture, normalize(skyboxTexCoord));
}
```

### 3.3 渲染順序優化

天空盒渲染優化：
```cpp
void RenderSceneCB() {
    // 1. 先渲染場景物體
    glEnable(GL_DEPTH_TEST);
    RenderMainScene();
    
    // 2. 清除深度測試，渲染天空盒到最後
    glDepthFunc(GL_LEQUAL);
    glDisable(GL_DEPTH_TEST);
    RenderSkybox();
    glDepthFunc(GL_LESS);
    glEnable(GL_DEPTH_TEST);
}
```

**最佳實踐**：
- 使用 `gl_Position.xyww` 使深度為 1.0（最遠層）
- 移除視圖矩陣平移分量，只保留旋轉
- 天空盒應最後渲染，避免深度測試影響

---

## 4. 攝像機系統

### 4.1 攝像機參數

```cpp
struct Camera {
    glm::vec3 position;      // 攝像機位置
    glm::vec3 target;        // 視點目標
    glm::vec3 up;            // 上向量
    float fovy;              // 視場角（垂直方向）
    float aspectRatio;       // 寬高比
    float zNear, zFar;       // 近遠裁剪平面
};
```

### 4.2 視圖矩陣計算

```cpp
glm::mat4 Camera::GetViewMatrix() const {
    return glm::lookAt(position, target, up);
}

glm::mat4 Camera::GetProjectionMatrix() const {
    return glm::perspective(
        glm::radians(fovy),
        aspectRatio,
        zNear,
        zFar
    );
}
```

#### 數學原理

視圖矩陣 = 攝像機坐標系轉換矩陣：
```
V = [
    right.x      right.y      right.z      -dot(right, position)
    up.x         up.y         up.z         -dot(up, position)
    -forward.x   -forward.y   -forward.z   dot(forward, position)
    0            0            0            1
]
```

其中：
- `forward = normalize(position - target)`
- `right = normalize(cross(forward, up))`
- `up = normalize(cross(right, forward))`

### 4.3 自由攝像機移動

```cpp
void HandleCameraMovement(int key) {
    glm::vec3 forward = glm::normalize(cameraTarget - cameraPos);
    glm::vec3 right = glm::normalize(glm::cross(forward, cameraUp));
    glm::vec3 up = cameraUp;
    
    switch(key) {
        case 'W':  // 前進
            cameraPos += forward * moveSpeed;
            cameraTarget += forward * moveSpeed;
            break;
        case 'S':  // 後退
            cameraPos -= forward * moveSpeed;
            cameraTarget -= forward * moveSpeed;
            break;
        case 'A':  // 左移
            cameraPos -= right * moveSpeed;
            cameraTarget -= right * moveSpeed;
            break;
        case 'D':  // 右移
            cameraPos += right * moveSpeed;
            cameraTarget += right * moveSpeed;
            break;
    }
}
```

---

## 5. 性能優化策略

### 5.1 GPU 內存管理

#### 頂點緩衝對象 (VBO) 使用
```cpp
GLuint vboId;
glGenBuffers(1, &vboId);
glBindBuffer(GL_ARRAY_BUFFER, vboId);
glBufferData(GL_ARRAY_BUFFER, vertexDataSize, vertexData, GL_STATIC_DRAW);
```

- **GL_STATIC_DRAW**：數據不變（網格）
- **GL_DYNAMIC_DRAW**：頻繁更新（動畫）
- **GL_STREAM_DRAW**：每幀更新（粒子系統）

#### 索引緩衝對象 (EBO) 使用
```cpp
GLuint eboId;
glGenBuffers(1, &eboId);
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboId);
glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexDataSize, indexData, GL_STATIC_DRAW);
```

優點：減少頂點數據重複，節省內存帶寬。

### 5.2 著色器優化

#### 着色器編譯快取
```cpp
std::map<std::string, GLuint> shaderProgramCache;

GLuint GetShaderProgram(const std::string& name) {
    if (shaderProgramCache.count(name)) {
        return shaderProgramCache[name];
    }
    GLuint prog = CompileShaderProgram(name);
    shaderProgramCache[name] = prog;
    return prog;
}
```

#### 著色器統一變量批量更新
```cpp
// 在每一幀的開始更新所有光源數據
glUseProgram(phongShaderProgram);

glUniform3fv(dirLightPosLoc, 1, glm::value_ptr(dirLight.position));
glUniform3fv(pointLightPosLoc, 1, glm::value_ptr(pointLight.position));
glUniform3fv(spotLightPosLoc, 1, glm::value_ptr(spotLight.position));

// 批量修改優於多次 Use 着色器
```

### 5.3 紋理優化

#### Mipmap 使用
```cpp
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
glGenerateMipmap(GL_TEXTURE_2D);
```

優點：遠距離紋理採樣自動降低分辨率，提高採樣效率。

#### 紋理單元綁定
```cpp
glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_2D, diffuseTexture);
glUniform1i(diffuseTextureLoc, 0);

glActiveTexture(GL_TEXTURE1);
glBindTexture(GL_TEXTURE_2D, normalTexture);
glUniform1i(normalTextureLoc, 1);
```

### 5.4 渲染狀態優化

#### 批量渲染
```cpp
// 分組相同着色器的物體
std::vector<SceneObject> phongObjects;
std::vector<SceneObject> unifiedObjects;

// 使用同一着色器渲染一組物體
glUseProgram(phongShaderProgram);
for (auto& obj : phongObjects) {
    obj.Draw();
}
```

#### 深度測試優化
```cpp
glEnable(GL_DEPTH_TEST);
glDepthFunc(GL_LESS);
glDepthMask(GL_TRUE);  // 允許寫入深度

// ... 標準渲染 ...

glDepthMask(GL_FALSE);  // 天空盒不寫入深度
glDepthFunc(GL_LEQUAL);
RenderSkybox();
glDepthMask(GL_TRUE);
glDepthFunc(GL_LESS);
```

---

## 6. 除錯和驗證技巧

### 6.1 著色器除錯

#### 著色器編譯錯誤檢查
```cpp
GLuint CompileShader(GLenum type, const std::string& source) {
    GLuint shader = glCreateShader(type);
    glCompileShader(shader);
    
    // 檢查編譯狀態
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader compilation failed: " << infoLog << std::endl;
    }
    return shader;
}
```

#### 著色器視覺化除錯
```glsl
// 除錯：將法向量可視化
fragColor = vec4(normal * 0.5 + 0.5, 1.0);  // 轉換到 [0,1] 範圍

// 除錯：將深度值可視化
fragColor = vec4(vec3(gl_FragCoord.z), 1.0);

// 除錯：將紋理座標可視化
fragColor = vec4(texCoord, 0.0, 1.0);
```

### 6.2 光照計算驗證

測試各個光源獨立效果：
```cpp
bool enableDirLight = true;
bool enablePointLight = true;
bool enableSpotLight = true;

// 可以通過鍵盤快捷鍵切換開關，逐一檢查光照效果
if (key == '1') enableDirLight = !enableDirLight;
if (key == '2') enablePointLight = !enablePointLight;
if (key == '3') enableSpotLight = !enableSpotLight;
```

### 6.3 性能分析

```cpp
#include <chrono>

auto start = std::chrono::high_resolution_clock::now();
RenderSceneCB();
auto end = std::chrono::high_resolution_clock::now();

auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
std::cout << "Frame time: " << duration.count() << "ms" << std::endl;
```

---

## 總結

本項目展示了現代 OpenGL 圖形渲染的完整流程：
1. **三種光照類型**實現逼真的材質渲染
2. **紋理系統**提供複雜的視覺效果
3. **著色器架構**高效地管理渲染邏輯
4. **性能優化**確保實時渲染
5. **交互系統**支持動態調參和場景控制

這些技術是遊戲引擎、3D 可視化和科學計算等領域的基礎。
