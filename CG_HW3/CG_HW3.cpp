#include "headers.h"
#include "trianglemesh.h"
#include "camera.h"
#include "shaderprog.h"
#include "light.h"
#include "imagetexture.h"
#include "skybox.h"
#include <sstream>
#include <iomanip>


// Global variables.
int screenWidth = 600;
int screenHeight = 600;
// Triangle mesh.
TriangleMesh* mesh = nullptr;
// Lights.
DirectionalLight* dirLight = nullptr;
PointLight* pointLight = nullptr;
SpotLight* spotLight = nullptr;
bool enableDirLight = true;
bool enablePointLight = true;
bool enableSpotLight = true;
glm::vec3 dirLightDirection = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 dirLightRadiance = glm::vec3(0.6f, 0.6f, 0.6f);
glm::vec3 pointLightPosition = glm::vec3(0.8f, 0.0f, 0.8f);
glm::vec3 pointLightIntensity = glm::vec3(0.5f, 0.1f, 0.1f);
glm::vec3 spotLightPosition = glm::vec3(0.0f, 1.0f, 0.0f);
glm::vec3 spotLightDirection = glm::vec3(0.0f, -1.0f, 0.0f);
glm::vec3 spotLightIntensity = glm::vec3(0.25f, 0.25f, 0.1f);
float spotLightCutoffStartInDegree = 30.0f;
float spotLightTotalWidthInDegree = 45.0f;
glm::vec3 ambientLight = glm::vec3(0.2f, 0.2f, 0.2f);
// Camera.
Camera* camera = nullptr;
glm::vec3 cameraPos = glm::vec3(0.0f, 1.0f, 5.0f);
glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float fovy = 30.0f;
float zNear = 0.1f;
float zFar = 1000.0f;
// Shader.
FillColorShaderProg* fillColorShader = nullptr;
PhongShadingDemoShaderProg* phongShadingShader = nullptr;
SkyboxShaderProg* skyboxShader = nullptr;
// UI.
const float lightMoveSpeed = 0.2f;
const float skyboxRotStep = 0.03f;
// Skybox.
Skybox* skybox = nullptr;
// Model menu.
std::vector<std::string> modelPaths;
// Skybox textures.
std::vector<std::string> skyboxTexturePaths;
int skyboxTextureIndex = 0;
// Directional light visualization.
GLuint dirLightVisVboId = 0;
const float dirLightVisLength = 1.0f;


// SceneObject.
struct SceneObject
{
    SceneObject() {
        mesh = nullptr;
        worldMatrix = glm::mat4x4(1.0f);
    }
    TriangleMesh* mesh;
    glm::mat4x4 worldMatrix;
};
SceneObject sceneObj;

// ScenePointLight (for visualization of a point light).
struct ScenePointLight
{
    ScenePointLight() {
        light = nullptr;
        worldMatrix = glm::mat4x4(1.0f);
        visColor = glm::vec3(1.0f, 1.0f, 1.0f);
    }
    PointLight* light;
    glm::mat4x4 worldMatrix;
    glm::vec3 visColor;
};
ScenePointLight pointLightObj;
ScenePointLight spotLightObj;

// Function prototypes.
void ReleaseResources();
// Callback functions.
void RenderSceneCB();
void ReshapeCB(int, int);
void ProcessSpecialKeysCB(int, int, int);
void ProcessKeysCB(unsigned char, int, int);
void SetupRenderState();
void LoadObjects(const std::string&);
void CreateCamera();
void CreateSkybox(const std::string);
void CreateShaderLib();
void CreateModelMenu();
void ProcessModelMenuCB(int);
glm::vec3 ScreenPercentToWorld(float, float, float);
void UpdateLightScreenAnchors(const glm::vec3&);
void InitSkyboxTextures();
void CycleSkyboxTexture(int);
void CreateDirLightVisGeometry();
void UpdateDirLightVisGeometry(const glm::vec3&);
void RenderOverlay();



void ReleaseResources()
{
    // Delete scene objects and lights.
    if (mesh != nullptr) {
        delete mesh;
        mesh = nullptr;
    }
    if (pointLight != nullptr) {
        delete pointLight;
        pointLight = nullptr;
    }
    if (dirLight != nullptr) {
        delete dirLight;
        dirLight = nullptr;
    }
    if (spotLight != nullptr) {
        delete spotLight;
        spotLight = nullptr;
    }
    // Delete camera.
    if (camera != nullptr) {
        delete camera;
        camera = nullptr;
    }
    // Delete shaders.
    if (fillColorShader != nullptr) {
        delete fillColorShader;
        fillColorShader = nullptr;
    }
    if (phongShadingShader != nullptr) {
        delete phongShadingShader;
        phongShadingShader = nullptr;
    }
    if (skyboxShader != nullptr) {
        delete skyboxShader;
        skyboxShader = nullptr;
    }
    if (skybox != nullptr) {
        delete skybox;
        skybox = nullptr;
    }
    if (dirLightVisVboId != 0) {
        glDeleteBuffers(1, &dirLightVisVboId);
        dirLightVisVboId = 0;
    }
}

glm::vec3 ScreenPercentToWorld(float xPercent, float yPercent, float ndcDepth)
{
    float xNdc = (xPercent / 100.0f) * 2.0f - 1.0f;
    float yNdc = (yPercent / 100.0f) * 2.0f - 1.0f;
    glm::vec4 clip = glm::vec4(xNdc, yNdc, ndcDepth, 1.0f);
    glm::mat4x4 invPV = glm::inverse(camera->GetProjMatrix() * camera->GetViewMatrix());
    glm::vec4 world = invPV * clip;
    if (world.w != 0.0f)
        world /= world.w;
    return glm::vec3(world);
}

void UpdateLightScreenAnchors(const glm::vec3& anchorWorld)
{
    if (camera == nullptr)
        return;
    glm::vec4 clip = camera->GetProjMatrix() * camera->GetViewMatrix() * glm::vec4(anchorWorld, 1.0f);
    float ndcDepth = clip.z / clip.w;
    glm::vec3 pointPos = ScreenPercentToWorld(85.0f, 55.0f, ndcDepth);
    glm::vec3 spotPos = ScreenPercentToWorld(50.0f, 80.0f, ndcDepth);
    if (pointLight != nullptr)
        pointLight->SetPosition(pointPos);
    if (spotLight != nullptr) {
        spotLight->SetPosition(spotPos);
        spotLight->SetDirection(glm::vec3(0.0f, -1.0f, 0.0f));
    }
}

void InitSkyboxTextures()
{
    skyboxTexturePaths = {
        "../TestTextures_HW3/photostudio_02_2k.png",
        "../TestTextures_HW3/sunflowers_2k.png",
        "../TestTextures_HW3/veranda_2k.png"
    };
    if (skyboxTextureIndex < 0 || skyboxTextureIndex >= (int)skyboxTexturePaths.size())
        skyboxTextureIndex = 0;
}

void CycleSkyboxTexture(int delta)
{
    if (skyboxTexturePaths.empty())
        return;
    skyboxTextureIndex = (skyboxTextureIndex + delta + (int)skyboxTexturePaths.size()) %
                         (int)skyboxTexturePaths.size();
    CreateSkybox(skyboxTexturePaths[skyboxTextureIndex]);
}

void CreateDirLightVisGeometry()
{
    if (dirLightVisVboId != 0)
        return;
    glGenBuffers(1, &dirLightVisVboId);
    VertexP verts[2];
    verts[0] = VertexP(glm::vec3(0.0f));
    verts[1] = VertexP(glm::vec3(0.0f));
    glBindBuffer(GL_ARRAY_BUFFER, dirLightVisVboId);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);
}

void UpdateDirLightVisGeometry(const glm::vec3& anchorWorld)
{
    if (dirLight == nullptr)
        return;
    CreateDirLightVisGeometry();
    glm::vec3 start = anchorWorld + glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 dir = glm::normalize(-dirLight->GetDirection());
    glm::vec3 end = start + dir * dirLightVisLength;
    VertexP verts[2];
    verts[0] = VertexP(start);
    verts[1] = VertexP(end);
    glBindBuffer(GL_ARRAY_BUFFER, dirLightVisVboId);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
}

static int MeasureTextWidth(const std::string& text)
{
    int w = 0;
    for (char c : text) {
        w += glutBitmapWidth(GLUT_BITMAP_8_BY_13, c);
    }
    return w;
}

static void DrawTextLine(int x, int y, const std::string& text)
{
    glRasterPos2i(x, y);
    for (char c : text) {
        glutBitmapCharacter(GLUT_BITMAP_8_BY_13, c);
    }
}

void RenderOverlay()
{
    if (camera == nullptr)
        return;

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, screenWidth, 0, screenHeight, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const int pad = 8;
    const int lineH = 15;
    const int lineCount = 3;
    const int boxX = 8;
    const int boxY = 8;
    int boxW = 0;
    const int boxH = pad * 2 + lineH * lineCount;

    glColor4f(0.0f, 0.0f, 0.0f, 0.55f);
    glBegin(GL_QUADS);
    glVertex2i(boxX, boxY);
    glVertex2i(boxX + boxW, boxY);
    glVertex2i(boxX + boxW, boxY + boxH);
    glVertex2i(boxX, boxY + boxH);
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);

    std::ostringstream oss;
    int y = boxY + boxH - pad - lineH;

    oss << "DirLight  : " << (enableDirLight ? "ON " : "OFF")
        << " dir=(" << std::fixed << std::setprecision(2)
        << dirLightDirection.x << "," << dirLightDirection.y << "," << dirLightDirection.z << ")";
    boxW = std::max(boxW, MeasureTextWidth(oss.str()));
    DrawTextLine(boxX + pad, y, oss.str());
    y -= 15;
    oss.str("");
    oss.clear();

    if (pointLight != nullptr) {
        glm::vec3 p = pointLight->GetPosition();
        oss << "PointLight: " << (enablePointLight ? "ON " : "OFF")
            << " pos=(" << std::fixed << std::setprecision(2)
            << p.x << "," << p.y << "," << p.z << ")";
        boxW = std::max(boxW, MeasureTextWidth(oss.str()));
        DrawTextLine(boxX + pad, y, oss.str());
        y -= 15;
        oss.str("");
        oss.clear();
    }

    if (spotLight != nullptr) {
        glm::vec3 p = spotLight->GetPosition();
        oss << "SpotLight : " << (enableSpotLight ? "ON " : "OFF")
            << " pos=(" << std::fixed << std::setprecision(2)
            << p.x << "," << p.y << "," << p.z << ")";
        boxW = std::max(boxW, MeasureTextWidth(oss.str()));
        DrawTextLine(boxX + pad, y, oss.str());
    }

    boxW += pad * 2;
    glColor4f(0.0f, 0.0f, 0.0f, 0.55f);
    glBegin(GL_QUADS);
    glVertex2i(boxX, boxY);
    glVertex2i(boxX + boxW, boxY);
    glVertex2i(boxX + boxW, boxY + boxH);
    glVertex2i(boxX, boxY + boxH);
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);
    y = boxY + boxH - pad - lineH;
    oss.str("");
    oss.clear();
    oss << "DirLight  : " << (enableDirLight ? "ON " : "OFF")
        << " dir=(" << std::fixed << std::setprecision(2)
        << dirLightDirection.x << "," << dirLightDirection.y << "," << dirLightDirection.z << ")";
    DrawTextLine(boxX + pad, y, oss.str());
    y -= lineH;
    oss.str("");
    oss.clear();
    if (pointLight != nullptr) {
        glm::vec3 p = pointLight->GetPosition();
        oss << "PointLight: " << (enablePointLight ? "ON " : "OFF")
            << " pos=(" << std::fixed << std::setprecision(2)
            << p.x << "," << p.y << "," << p.z << ")";
        DrawTextLine(boxX + pad, y, oss.str());
        y -= lineH;
        oss.str("");
        oss.clear();
    }
    if (spotLight != nullptr) {
        glm::vec3 p = spotLight->GetPosition();
        oss << "SpotLight : " << (enableSpotLight ? "ON " : "OFF")
            << " pos=(" << std::fixed << std::setprecision(2)
            << p.x << "," << p.y << "," << p.z << ")";
        DrawTextLine(boxX + pad, y, oss.str());
    }
    // Controls overlay (top-left).
    const int ctrlLines = 6;
    const int ctrlX = 8;
    int ctrlW = 0;
    const int ctrlH = pad * 2 + lineH * ctrlLines;
    const int ctrlY = screenHeight - ctrlH - 8;
    const char* ctrlLinesText[] = {
        "Controls:",
        "Lights: 1=Dir toggle  2=Spot toggle  3=Point toggle  0=All on",
        "Move Point: J/L=left/right  I/K=up/down  U/O=forward/back",
        "Move Spot : F/H=left/right  T/G=up/down  R/Y=forward/back",
        "Rotate Obj: A/D=Y axis  W/S=X axis",
        "Skybox: Q/E=rotate  Z/X=texture"
    };
    for (const char* line : ctrlLinesText) {
        ctrlW = std::max(ctrlW, MeasureTextWidth(line));
    }
    ctrlW += pad * 2;
    glColor4f(0.0f, 0.0f, 0.0f, 0.55f);
    glBegin(GL_QUADS);
    glVertex2i(ctrlX, ctrlY);
    glVertex2i(ctrlX + ctrlW, ctrlY);
    glVertex2i(ctrlX + ctrlW, ctrlY + ctrlH);
    glVertex2i(ctrlX, ctrlY + ctrlH);
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);
    int cy = ctrlY + ctrlH - pad - lineH;
    for (const char* line : ctrlLinesText) {
        DrawTextLine(ctrlX + pad, cy, line);
        cy -= lineH;
    }

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

static float curObjRotationY = 30.0f;
static float curObjRotationX = 0.0f;
const float rotStep = 0.002f;
void RenderSceneCB()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::vec3 anchorWorld(0.0f, 0.0f, 0.0f);
    bool hasAnchor = false;
    
    // Render skybox. ----------------------------------------------------------------------------
    if (skybox != nullptr) {
        skybox->Render(camera, skyboxShader);
    }
    // -------------------------------------------------------------------------------------------

    TriangleMesh* pMesh = sceneObj.mesh;
    if (pMesh != nullptr) {
        // Update transform.
        // curObjRotationY += rotStep;
        glm::mat4x4 S = glm::scale(glm::mat4x4(1.0f), glm::vec3(1.5f, 1.5f, 1.5f));
        glm::mat4x4 Rx = glm::rotate(glm::mat4x4(1.0f), glm::radians(curObjRotationX), glm::vec3(1, 0, 0));
        glm::mat4x4 Ry = glm::rotate(glm::mat4x4(1.0f), glm::radians(curObjRotationY), glm::vec3(0, 1, 0));
        sceneObj.worldMatrix = S * Ry * Rx;
        // -------------------------------------------------------
        // Note: if you want to compute lighting in the View Space, 
        //       you might need to change the code below.
        // -------------------------------------------------------
        glm::mat4x4 normalMatrix = glm::transpose(glm::inverse(sceneObj.worldMatrix));
        glm::mat4x4 MVP = camera->GetProjMatrix() * camera->GetViewMatrix() * sceneObj.worldMatrix;
        anchorWorld = glm::vec3(sceneObj.worldMatrix * glm::vec4(pMesh->GetObjCenter(), 1.0f));
        hasAnchor = true;
        (void)anchorWorld;
        
        // -------------------------------------------------------
        // Add your rendering code here.
        // -------------------------------------------------------
        phongShadingShader->Bind();
        glUniformMatrix4fv(phongShadingShader->GetLocM(), 1, GL_FALSE, glm::value_ptr(sceneObj.worldMatrix));
        glUniformMatrix4fv(phongShadingShader->GetLocNM(), 1, GL_FALSE, glm::value_ptr(normalMatrix));
        glUniformMatrix4fv(phongShadingShader->GetLocMVP(), 1, GL_FALSE, glm::value_ptr(MVP));
        glUniform3fv(phongShadingShader->GetLocCameraPos(), 1, glm::value_ptr(cameraPos));
        glUniform3fv(phongShadingShader->GetLocAmbientLight(), 1, glm::value_ptr(ambientLight));

        if (dirLight != nullptr) {
            glm::vec3 Ld = enableDirLight ? dirLight->GetRadiance() : glm::vec3(0.0f);
            glUniform3fv(phongShadingShader->GetLocDirLightDir(), 1, glm::value_ptr(dirLight->GetDirection()));
            glUniform3fv(phongShadingShader->GetLocDirLightRadiance(), 1, glm::value_ptr(Ld));
        }
        if (pointLight != nullptr) {
            glm::vec3 Lp = enablePointLight ? pointLight->GetIntensity() : glm::vec3(0.0f);
            glUniform3fv(phongShadingShader->GetLocPointLightPos(), 1, glm::value_ptr(pointLight->GetPosition()));
            glUniform3fv(phongShadingShader->GetLocPointLightIntensity(), 1, glm::value_ptr(Lp));
        }
        if (spotLight != nullptr) {
            glm::vec3 Ls = enableSpotLight ? spotLight->GetIntensity() : glm::vec3(0.0f);
            glUniform3fv(phongShadingShader->GetLocSpotLightPos(), 1, glm::value_ptr(spotLight->GetPosition()));
            glUniform3fv(phongShadingShader->GetLocSpotLightDir(), 1, glm::value_ptr(spotLight->GetDirection()));
            glUniform3fv(phongShadingShader->GetLocSpotLightIntensity(), 1, glm::value_ptr(Ls));
            glUniform1f(phongShadingShader->GetLocSpotLightCutoffStart(), spotLight->GetCutoffStartDeg());
            glUniform1f(phongShadingShader->GetLocSpotLightCutoffTotal(), spotLight->GetCutoffTotalDeg());
        }

        pMesh->Render(phongShadingShader, MVP);
        phongShadingShader->UnBind();
    }
    // -------------------------------------------------------------------------------------------

    // Visualize the light with fill color. ------------------------------------------------------
    if (dirLight != nullptr && enableDirLight && fillColorShader != nullptr && hasAnchor) {
        UpdateDirLightVisGeometry(anchorWorld);
        glm::mat4x4 MVP = camera->GetProjMatrix() * camera->GetViewMatrix();
        fillColorShader->Bind();
        glUniformMatrix4fv(fillColorShader->GetLocMVP(), 1, GL_FALSE, glm::value_ptr(MVP));
        glm::vec3 dirVisColor(0.95f, 0.85f, 0.2f);
        glUniform3fv(fillColorShader->GetLocFillColor(), 1, glm::value_ptr(dirVisColor));
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, dirLightVisVboId);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexP), 0);
        glPointSize(8.0f);
        glDrawArrays(GL_POINTS, 1, 1);
        glPointSize(1.0f);
        glDisableVertexAttribArray(0);
        fillColorShader->UnBind();
    }
    PointLight* pointLight = pointLightObj.light;
    if (pointLight != nullptr && enablePointLight) {
        glm::mat4x4 T = glm::translate(glm::mat4x4(1.0f), pointLight->GetPosition());
        pointLightObj.worldMatrix = T;
        glm::mat4x4 MVP = camera->GetProjMatrix() * camera->GetViewMatrix() * pointLightObj.worldMatrix;
        fillColorShader->Bind();
        glUniformMatrix4fv(fillColorShader->GetLocMVP(), 1, GL_FALSE, glm::value_ptr(MVP));
        glUniform3fv(fillColorShader->GetLocFillColor(), 1, glm::value_ptr(pointLightObj.visColor));
        // Render the point light.
        pointLight->Draw();
        fillColorShader->UnBind();
    }
    SpotLight* spotLight = (SpotLight*)(spotLightObj.light);
    if (spotLight != nullptr && enableSpotLight) {
        glm::mat4x4 T = glm::translate(glm::mat4x4(1.0f), spotLight->GetPosition());
        spotLightObj.worldMatrix = T;
        glm::mat4x4 MVP = camera->GetProjMatrix() * camera->GetViewMatrix() * spotLightObj.worldMatrix;
        fillColorShader->Bind();
        glUniformMatrix4fv(fillColorShader->GetLocMVP(), 1, GL_FALSE, glm::value_ptr(MVP));
        glUniform3fv(fillColorShader->GetLocFillColor(), 1, glm::value_ptr(spotLightObj.visColor));
        // Render the spot light.
        spotLight->Draw();
        fillColorShader->UnBind();
    }
    // -------------------------------------------------------------------------------------------

    RenderOverlay();

    glutSwapBuffers();
}

void ReshapeCB(int w, int h)
{
    // Update viewport.
    screenWidth = w;
    screenHeight = h;
    glViewport(0, 0, screenWidth, screenHeight);
    // Adjust camera and projection.
    float aspectRatio = (float)screenWidth / (float)screenHeight;
    camera->UpdateProjection(fovy, aspectRatio, zNear, zFar);
}

void ProcessSpecialKeysCB(int key, int x, int y)
{
    // Handle special (functional) keyboard inputs such as F1, spacebar, page up, etc. 
    switch (key) {
    // Rendering mode.
    case GLUT_KEY_F1:
        // Render with point mode.
        glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
        break;
    case GLUT_KEY_F2:
        // Render with line mode.
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        break;
    case GLUT_KEY_F3:
        // Render with fill mode.
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        break;
    
    // Light control.
    case GLUT_KEY_LEFT:
        if (pointLight != nullptr)
            pointLight->MoveLeft(lightMoveSpeed);
        break;
    case GLUT_KEY_RIGHT:
        if (pointLight != nullptr)
            pointLight->MoveRight(lightMoveSpeed);
        break;
    case GLUT_KEY_UP:
        if (pointLight != nullptr)
            pointLight->MoveUp(lightMoveSpeed);
        break;
    case GLUT_KEY_DOWN:
        if (pointLight != nullptr)
            pointLight->MoveDown(lightMoveSpeed);
        break;
    default:
        break;
    }
}

void ProcessKeysCB(unsigned char key, int x, int y)
{
    // Handle other keyboard inputs those are not defined as special keys.
    if (key == 27) {
        // Release memory allocation if needed.
        ReleaseResources();
        exit(0);
    }
    // Object rotation control (WASD).
    if (key == 'a')
        curObjRotationY -= 2.0f;
    if (key == 'd')
        curObjRotationY += 2.0f;
    if (key == 'w')
        curObjRotationX -= 2.0f;
    if (key == 's')
        curObjRotationX += 2.0f;

    // Skybox rotation (Q/E).
    if (skybox != nullptr) {
        if (key == 'q')
            skybox->SetRotation(skybox->GetRotation() - skyboxRotStep);
        if (key == 'e')
            skybox->SetRotation(skybox->GetRotation() + skyboxRotStep);
    }
    // Cycle skybox textures (Z/X).
    if (key == 'z')
        CycleSkyboxTexture(-1);
    if (key == 'x')
        CycleSkyboxTexture(1);

    // Point light control (IJKL).
    if (pointLight != nullptr && enablePointLight) {
        if (key == 'j')
            pointLight->MoveLeft(lightMoveSpeed);
        if (key == 'l')
            pointLight->MoveRight(lightMoveSpeed);
        if (key == 'i')
            pointLight->MoveUp(lightMoveSpeed);
        if (key == 'k')
            pointLight->MoveDown(lightMoveSpeed);
        if (key == 'u')
            pointLight->MoveForward(lightMoveSpeed);
        if (key == 'o')
            pointLight->MoveBackward(lightMoveSpeed);
    }

    // Spot light control (F/G/T/H).
    if (spotLight != nullptr && enableSpotLight) {
        if (key == 'f')
            spotLight->MoveLeft(lightMoveSpeed);
        if (key == 'h')
            spotLight->MoveRight(lightMoveSpeed);
        if (key == 't')
            spotLight->MoveUp(lightMoveSpeed);
        if (key == 'g')
            spotLight->MoveDown(lightMoveSpeed);
        if (key == 'r')
            spotLight->MoveForward(lightMoveSpeed);
        if (key == 'y')
            spotLight->MoveBackward(lightMoveSpeed);
    }

    if (key == '1') {
        enableDirLight = !enableDirLight;
    }
    if (key == '2') {
        enableSpotLight = !enableSpotLight;
    }
    if (key == '3') {
        enablePointLight = !enablePointLight;
    }
    if (key == '0') {
        enableDirLight = true;
        enablePointLight = true;
        enableSpotLight = true;
    }
}

void SetupRenderState()
{
    glEnable(GL_DEPTH_TEST);

    glm::vec4 clearColor = glm::vec4(0.44f, 0.57f, 0.75f, 1.00f);
    glClearColor(
        (GLclampf)(clearColor.r), 
        (GLclampf)(clearColor.g), 
        (GLclampf)(clearColor.b), 
        (GLclampf)(clearColor.a)
    );
}

void LoadObjects(const std::string& modelPath)
{
    // -------------------------------------------------------
	// Note: you can change the code below if you want to load
    //       the model dynamically.
	// -------------------------------------------------------

    if (mesh != nullptr) {
        delete mesh;
        mesh = nullptr;
    }
    mesh = new TriangleMesh();
    mesh->LoadFromFile(modelPath, true);
    mesh->ShowInfo();
    sceneObj.mesh = mesh;    
}

void CreateLights()
{
    if (dirLight != nullptr) {
        delete dirLight;
        dirLight = nullptr;
    }
    if (pointLight != nullptr) {
        delete pointLight;
        pointLight = nullptr;
    }
    if (spotLight != nullptr) {
        delete spotLight;
        spotLight = nullptr;
    }
    // Create a directional light.
    dirLight = new DirectionalLight(dirLightDirection, dirLightRadiance);
    // Create a point light.
    pointLight = new PointLight(pointLightPosition, pointLightIntensity);
    pointLightObj.light = pointLight;
    pointLightObj.visColor = glm::vec3(0.9f, 0.2f, 0.2f);
    // Create a spot light.
    spotLight = new SpotLight(spotLightPosition, spotLightIntensity, spotLightDirection, 
            spotLightCutoffStartInDegree, spotLightTotalWidthInDegree);
    spotLightObj.light = spotLight;
    spotLightObj.visColor = glm::vec3(0.2f, 0.9f, 0.2f);
    CreateDirLightVisGeometry();
}

void CreateCamera()
{
    // Create a camera and update view and proj matrices.
    camera = new Camera((float)screenWidth / (float)screenHeight);
    camera->UpdateView(cameraPos, cameraTarget, cameraUp);
    float aspectRatio = (float)screenWidth / (float)screenHeight;
    camera->UpdateProjection(fovy, aspectRatio, zNear, zFar);
}

void CreateSkybox(const std::string texFilePath)
{
    // -------------------------------------------------------
	// Note: you can change the code below if you want to change
    //       the skybox texture dynamically.
	// -------------------------------------------------------

    if (skybox != nullptr) {
        delete skybox;
        skybox = nullptr;
    }
    const int numSlices = 36;
    const int numStacks = 18;
    const float radius = 50.0f;
    skybox = new Skybox(texFilePath, numSlices, numStacks, radius);
}

void CreateShaderLib()
{
    fillColorShader = new FillColorShaderProg();
    if (!fillColorShader->LoadFromFiles("shaders/fixed_color.vs", "shaders/fixed_color.fs"))
        exit(1);

    phongShadingShader = new PhongShadingDemoShaderProg();
    if (!phongShadingShader->LoadFromFiles("shaders/phong_shading_demo.vs", "shaders/phong_shading_demo.fs"))
        exit(1);

    skyboxShader = new SkyboxShaderProg();
    if (!skyboxShader->LoadFromFiles("shaders/skybox.vs", "shaders/skybox.fs"))
        exit(1);
}

void ProcessModelMenuCB(int value)
{
    if (value < 0 || value >= (int)modelPaths.size())
        return;
    LoadObjects(modelPaths[value]);
    glutPostRedisplay();
}

void CreateModelMenu()
{
    modelPaths = {
        "../TestModels_HW3/LightingTest/ColorCube/ColorCube.obj",
        "../TestModels_HW3/LightingTest/Bunny/Bunny.obj",
        "../TestModels_HW3/LightingTest/Koffing/Koffing.obj",
        "../TestModels_HW3/LightingTest/Pillows/Pillows.obj",
        "../TestModels_HW3/LightingTest/Gengar/Gengar.obj",
        "../TestModels_HW3/LightingTest/Forklift/Forklift.obj",
        "../TestModels_HW3/LightingTest/Rose/Rose.obj",
        "../TestModels_HW3/LightingTest/Soccer/Soccer.obj",
        "../TestModels_HW3/FinalTest/Slowbro/Slowbro.obj",
        "../TestModels_HW3/FinalTest/Gengar/Gengar.obj",
        "../TestModels_HW3/FinalTest/Ferrari/Ferrari.obj",
        "../TestModels_HW3/FinalTest/Koffing/Koffing.obj",
        "../TestModels_HW3/FinalTest/Ivysaur/Ivysaur.obj",
        "../TestModels_HW3/FinalTest/MagikarpF/MagikarpF.obj",
        "../TestModels_HW3/FinalTest/TexCube/TexCube.obj",
        "../TestModels_HW3/FinalTest/Rose/Rose.obj",
        "../TestModels_HW3/FinalTest/Forklift/Forklift.obj"
    };

    glutCreateMenu(ProcessModelMenuCB);
    for (size_t i = 0; i < modelPaths.size(); ++i) {
        glutAddMenuEntry(modelPaths[i].c_str(), (int)i);
    }
    glutAttachMenu(GLUT_RIGHT_BUTTON);
}

int main(int argc, char** argv)
{
    // Setting window properties.
    glutInit(&argc, argv);
#ifdef FREEGLUT
    glutSetOption(GLUT_MULTISAMPLE, 4);
#endif
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_MULTISAMPLE);
    glutInitWindowSize(screenWidth, screenHeight);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Lighting and Texturing");

    // Initialization.
    SetupRenderState();
    LoadObjects("../TestModels_HW3/LightingTest/ColorCube/ColorCube.obj");
    CreateLights();
    CreateCamera();
    InitSkyboxTextures();
    CreateSkybox(skyboxTexturePaths[skyboxTextureIndex]);
    CreateShaderLib();
    CreateModelMenu();

    // Register callback functions.
    glutDisplayFunc(RenderSceneCB);
    glutIdleFunc(RenderSceneCB);
    glutReshapeFunc(ReshapeCB);
    glutSpecialFunc(ProcessSpecialKeysCB);
    glutKeyboardFunc(ProcessKeysCB);

    // Start rendering loop.
    glutMainLoop();

    return 0;
}
