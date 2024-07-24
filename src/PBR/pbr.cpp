#include "glm/ext/matrix_transform.hpp"
#include "glm/ext/vector_float3.hpp"
#include "glm/fwd.hpp"
#include <array>
#include <functional>
#include <limits>
#include <random>
#include <string>
#include <tuple>

#include <glad/glad.h>
// GLAD first

#define STB_IMAGE_IMPLEMENTATION
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "util/class_camera.hpp"
#include "util/class_model.hpp"
#include "util/class_shader.hpp"
#include "util/debugTool.hpp"
#include "util/lightGroup.hpp"

using namespace glm;
using namespace std;

/**NOTE - 全局变量、摄像机、全局时钟以及函数
 */
const GLint CAMERA_WIDTH = 1920;
const GLint CAMERA_HEIGH = 1080;
const float cameraAspect = static_cast<float>(CAMERA_WIDTH) / static_cast<float>(CAMERA_HEIGH);
Camera*     camera       = new Camera(vec3(0.0F, 0.0F, 2.0F), vec3(0.0F, 1.0F, 0.0F), -90.0F, 0.0F);
float       mouseLastX = 0.0f, mouseLastY = 0.0f;  // 记录鼠标的位置
float       lastFrame = 0.0f, deltaTime = 0.0f;    // 全局时钟

void   framebuffer_size_callback(GLFWwindow* window, int w, int h);
void   mouse_callback(GLFWwindow* window, double xpos, double ypos);
void   scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void   processInput(GLFWwindow* window);
int    initGLFWWindow(GLFWwindow*& window);
GLuint createImageObjrct(const char* imagePath, const bool autoGammaCorrection = true, const bool flip_texture = true);
GLuint createSkyboxTexture(const char* imageFolder, const bool autoGammaCorrection = true);
void   createFBO(GLuint& fbo, GLuint& texAttachment, GLuint& rbo, const char* hint = "null");
void   createObjFromHardcode(GLuint&         vao,
                             GLuint&         vbo,
                             GLuint&         ebo,
                             vector<GLfloat> vertices,
                             vector<GLuint>  vertexIdx = {});
void   renderTextureToScreen(const GLuint screenVAO, const GLuint textureToShow, Shader& screenShader);

int main(int /*argc*/, char** /*argv*/)
{
    GLFWwindow* window;  // 创建GLFW窗口，初始化GLAD
    if (initGLFWWindow(window) == 0) return -1;

    /**NOTE - OpenGL基本设置
     */
    glEnable(GL_DEPTH_TEST);    // 启用深度缓冲
    glDepthFunc(GL_LEQUAL);     // 修改深度测试的标准
    glEnable(GL_STENCIL_TEST);  // 启用模板缓冲
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glEnable(GL_BLEND);                                 // 启用混合
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);  // 设置混合函数
    glEnable(GL_CULL_FACE);                             // 启用面剔除
    // glClearColor(0.2F, 0.3F, 0.3F, 1.0F);  // 设置清空颜色
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glDisable(GL_MULTISAMPLE);  // 启用多重采样
    /**NOTE - 文档中GL_MULTISAMPLE时默认启动的（true）
     */
    // glEnable(GL_FRAMEBUFFER_SRGB);  // 自动Gamme矫正
    /**NOTE - gamma矫正关闭
    我们在pbrShader中手动矫正gamma
    */
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);  // 在立方体贴图的面之间进行正确过滤

    /**NOTE - 模型和着色器、纹理
     */
    // Model
    Model box("./box/box.obj");
    Model plane("./plane/plane.obj");
    Model sphere("./sphere/sphere.obj");
    Model geosphere("./geosphere/geosphere.obj");
    Model testScene("./testScene/scene.obj");
    // Shader
    Shader phongShader("./shader/stdVerShader.vs.glsl", "./shader/simpleWritePhongLighting.fs.glsl");
    Shader skyboxShader("./shader/skyboxShader.vs.glsl", "./shader/skyboxShader.fs.glsl");
    Shader lightObjShader("./shader/stdVerShader.vs.glsl", "./shader/stdPureColor.fs.glsl");
    Shader myCubeHDR("./shader/myCubeHDR.vs.glsl", "./shader/myCubeHDR.fs.glsl");
    Shader generateCubeHDR("./shader/generateCubeHDR.vs.glsl", "./shader/generateCubeHDR.fs.glsl",
                           "./shader/generateCubeHDR.gs.glsl");
    Shader preCalculateSpecularIBL("./shader/myCubeHDR.vs.glsl", "./shader/preCalculateSpecularIBL.fs.glsl");
    Shader preCalculateSpecularIBL2("./shader/stdScreenShader.vs.glsl", "./shader/preCalculateSpecularIBL2.fs.glsl");
    Shader shadowShader("./shader/PointShadowVShader.vs.glsl", "./shader/PointShadowFShader.fs.glsl",
                        "./shader/PointShadowGShader.gs.glsl");
    // Texture
    GLuint cubeTexture = createSkyboxTexture("./texture/", true);
    GLuint woodTexture = createImageObjrct("./texture/wood.jpg", true);
    // pbr texture
    Shader pbrShader("./shader/stdVerShader.vs.glsl", "./shader/pbr.fs.glsl");
    GLuint pbr_albedo    = createImageObjrct("./texture/vbzkear_2K_Albedo.jpg", true);
    GLuint pbr_ao        = createImageObjrct("./texture/vbzkear_2K_AO.jpg", false);
    GLuint pbr_normal    = createImageObjrct("./texture/vbzkear_2K_Normal.jpg", false);
    GLuint pbr_roughness = createImageObjrct("./texture/vbzkear_2K_Roughness.jpg", false);

    /**NOTE - 灯光组
     */
    LightGroup         lightGroup;
    std::vector<Light> lights = {
        Light(0, vec3(1, 1, 1), 50, vec3(2, 2, 5)),  // 1
        Light(0, vec3(1, 1, 1), 50, vec3(4, 2, 5)),  // 2
        Light(0, vec3(1, 1, 1), 50, vec3(2, 4, 5)),  // 3
        Light(0, vec3(1, 1, 1), 50, vec3(4, 4, 5)),  // 4
    };
    lightGroup.addLight(lights);
    lightGroup.createLightUniformBuffer();
    lightGroup.bindingUniformBuffer(0);

    /**NOTE - ScreenTextureObject for debug
     */
    DebugTool debugTool;

    /**NOTE - 读取HDR纹理
     */
    stbi_set_flip_vertically_on_load(true);
    int          width, height, nrComponents;
    float*       data = stbi_loadf("./texture/dam_bridge_2k.hdr", &width, &height, &nrComponents, 0);
    unsigned int hdrTexture;
    if (data)
    {
        glGenTextures(1, &hdrTexture);
        glBindTexture(GL_TEXTURE_2D, hdrTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else { std::cout << "Failed to load HDR image." << std::endl; }

    /**NOTE - IBL漫反射项：从等距柱状投影到立方体贴图
     */
    GLuint captureFBO;
    glGenFramebuffers(1, &captureFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    // 立方体贴图
    GLuint envCubemap;
    glGenTextures(1, &envCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
    for (int i : {0, 1, 2, 3, 4, 5})
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA16F, 512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap,
                               0);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    std::array<GLenum, 6> colorAttachments = {
        GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2,
        GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5,
    };
    //  检查帧缓冲状态
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
    {
        cout << "Framebuffer is  complete!" << endl;
    }
    else { cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl; }
    // 解绑
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    // view and projection
    glm::mat4                captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    std::array<glm::mat4, 6> captureViews      = {
        lookAt(vec3(0.0f, 0.0f, 0.0f), vec3(1.0f, 0.0f, 0.0f), vec3(0.0f, -1.0f, 0.0f)),
        lookAt(vec3(0.0f, 0.0f, 0.0f), vec3(-1.0f, 0.0f, 0.0f), vec3(0.0f, -1.0f, 0.0f)),
        lookAt(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f)),
        lookAt(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, -1.0f, 0.0f), vec3(0.0f, 0.0f, -1.0f)),
        lookAt(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, 1.0f), vec3(0.0f, -1.0f, 0.0f)),
        lookAt(vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 0.0f, -1.0f), vec3(0.0f, -1.0f, 0.0f)),
    };
    /**NOTE - 将HDR存入CubeMap
     */
    glDisable(GL_CULL_FACE);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glDrawBuffers(6, colorAttachments.data());
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, 512, 512);
    // uniforms
    myCubeHDR.use();
    myCubeHDR.setParameter("projection", captureProjection);
    for (int i : {0, 1, 2, 3, 4, 5})
    {
        myCubeHDR.setParameter("view[" + std::to_string(i) + "]", captureViews[i]);
    }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrTexture);
    myCubeHDR.setParameter("equirectangularMap", 0);
    box.Draw(&myCubeHDR);
    //  恢复现场
    glEnable(GL_CULL_FACE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    /**NOTE - IBL镜面光照项
     */
    unsigned int prefilterMap;
    glGenTextures(1, &prefilterMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    /**NOTE - 将HDR存入CubeMap
     */
    glDisable(GL_CULL_FACE);
    preCalculateSpecularIBL.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrTexture);
    preCalculateSpecularIBL.setParameter("equirectangularMap", 0);
    preCalculateSpecularIBL.setParameter("projection", captureProjection);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glDrawBuffers(6, colorAttachments.data());
    for (int i : {0, 1, 2, 3, 4, 5})
    {
        preCalculateSpecularIBL.setParameter("view[" + std::to_string(i) + "]", captureViews[i]);
    }
    GLuint maxMipLevels = 5;
    for (GLuint mip = 0; mip < maxMipLevels; ++mip)
    {
        // reisze framebuffer according to mip-level size.
        GLuint mipWidth  = 128 * pow(0.5, mip);
        GLuint mipHeight = 128 * pow(0.5, mip);
        // 管理阴影的captureRBO我暂时就不设置了
        glViewport(0, 0, mipWidth, mipHeight);

        float roughness = (float)mip / (float)(maxMipLevels - 1);
        preCalculateSpecularIBL.setParameter("roughness", roughness);

        for (int i : {0, 1, 2, 3, 4, 5})
        {
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                                   prefilterMap, mip);
        }
        glClear(GL_COLOR_BUFFER_BIT);
        box.Draw(&preCalculateSpecularIBL);
    }
    //  恢复现场
    glEnable(GL_CULL_FACE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    /**NOTE - IBL镜面材质项
     */
    GLuint brdfLUTTexture;
    glGenTextures(1, &brdfLUTTexture);
    // pre-allocate enough memory for the LUT texture.
    glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    /**NOTE - 预计算材质项
     */
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    for (int i : {0, 1, 2, 3, 4, 5})
    {
        // 把所有的DrawBuffer都解除掉
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, 0, 0);
    }
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);
    glDrawBuffers(1, colorAttachments.data());
    glViewport(0, 0, 512, 512);
    glClear(GL_COLOR_BUFFER_BIT);
    preCalculateSpecularIBL2.use();
    glBindVertexArray(debugTool.getScreenVAO());
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    /**FIXME - 关于多个drawBuffer
    glviewport的最终写入大小好像和最小的drawBuffer有关
    原本这里有6个drawBuffer，attach0512大小，其余5个attach8pixel大小
    glviewport的大小是512，最终渲染出来的大小只有8X8像素的区域
     */

    /**NOTE - light shadow map
     */
    // shadowMap FBO
    GLuint shadowMapFBO;
    glGenFramebuffers(1, &shadowMapFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glCullFace(GL_FRONT);  // 正面剔除，深度会稍微大一些，但仍然再阴影得前面
    //
    const GLuint shadowMapSize = 2048;
    // 光空间的变换
    GLfloat               near              = 1.0F;
    GLfloat               far               = 100.0F;
    GLfloat               aspect            = static_cast<GLfloat>(shadowMapSize) / static_cast<GLfloat>(shadowMapSize);
    glm::mat4             shadow_projection = glm::perspective(glm::radians(90.0F), aspect, near, far);
    std::array<GLuint, 4> shadowMaps        = {0, 0, 0, 0};
    for (GLuint i : {0, 1, 2, 3})
    {
        glGenTextures(1, &shadowMaps[i]);
        glBindTexture(GL_TEXTURE_CUBE_MAP, shadowMaps[i]);
        for (int face : {0, 1, 2, 3, 4, 5})
        {
            // glTextureStorage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_DEPTH_COMPONENT, shadowMapSize,
            //                    shadowMapSize);
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_DEPTH_COMPONENT, shadowMapSize, shadowMapSize, 0,
                         GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        }
        // 设置纹理参数
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        //
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowMaps[i], 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        //  检查帧缓冲状态
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
        {
            std::cout << "Framebuffer is  complete!" << std::endl;
        }
        else { std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl; }

        // 计算阴影
        shadowShader.use();
        glViewport(0, 0, shadowMapSize, shadowMapSize);
        // uniform
        shadowShader.setParameter("model", mat4(1.0F));
        std::array<glm::mat4, 6> shadow_transforms = {
            shadow_projection * glm::lookAt(lights[i].getPostion(), lights[i].getPostion() + glm::vec3(1.0, 0.0, 0.0),
                                            glm::vec3(0.0, -1.0, 0.0)),
            shadow_projection * glm::lookAt(lights[i].getPostion(), lights[i].getPostion() + glm::vec3(-1.0, 0.0, 0.0),
                                            glm::vec3(0.0, -1.0, 0.0)),
            shadow_projection * glm::lookAt(lights[i].getPostion(), lights[i].getPostion() + glm::vec3(0.0, 1.0, 0.0),
                                            glm::vec3(0.0, 0.0, 1.0)),
            shadow_projection * glm::lookAt(lights[i].getPostion(), lights[i].getPostion() + glm::vec3(0.0, -1.0, 0.0),
                                            glm::vec3(0.0, 0.0, -1.0)),
            shadow_projection * glm::lookAt(lights[i].getPostion(), lights[i].getPostion() + glm::vec3(0.0, 0.0, 1.0),
                                            glm::vec3(0.0, -1.0, 0.0)),
            shadow_projection * glm::lookAt(lights[i].getPostion(), lights[i].getPostion() + glm::vec3(0.0, 0.0, -1.0),
                                            glm::vec3(0.0, -1.0, 0.0)),
        };
        for (int j = 0; j < 6; j++)
        {
            shadowShader.setParameter("shadowMatrices[" + to_string(j) + "]", shadow_transforms[j]);
        }
        shadowShader.setParameter("lightPos", lights[i].getPostion());
        shadowShader.setParameter("far_plane", far);
        // render
        glClear(GL_DEPTH_BUFFER_BIT);
        const int xNum = 5;
        const int yNum = 5;
        for (int k = 0; k <= xNum; k++)
        {
            for (int j = 0; j <= yNum; j++)
            {
                shadowShader.setParameter("model", scale(translate(mat4(1), vec3(k * 1, 0, j * 1)), vec3(0.5)));
                geosphere.Draw(&shadowShader);
            }
        }
    }
    // 恢复现场
    glCullFace(GL_BACK);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 渲染循环
    while (glfwWindowShouldClose(window) == 0)
    {
        // 更新时钟、摄像机并处理输入信号
        deltaTime = glfwGetTime() - lastFrame;
        lastFrame = glfwGetTime();
        processInput(window);

        // SECTION - 渲染循环
        /**NOTE - 清空屏幕
         */
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, CAMERA_WIDTH, CAMERA_HEIGH);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);  // 清除颜色、深度和模板缓冲

        /**NOTE - 更新视图变换
         */
        mat4 view       = camera->GetViewMatrix();
        mat4 projection = perspective(radians(camera->Zoom), cameraAspect, 0.1f, 100.0f);

        /**NOTE - 渲染
         */
        pbrShader.use();
        pbrShader.setParameter("view", view);
        pbrShader.setParameter("projection", projection);
        pbrShader.setParameter("cameraPos", camera->Position);
        pbrShader.setParameter("near", near);
        pbrShader.setParameter("far", far);
        // material
        pbrShader.setParameter("albedo", vec3(0.5, 0, 0));
        pbrShader.setParameter("ao", 1.0f);
        // texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, pbr_albedo);
        pbrShader.setParameter("albedoMap", 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pbr_ao);
        pbrShader.setParameter("aoMap", 1);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, pbr_roughness);
        pbrShader.setParameter("roughnessMap", 2);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, pbr_normal);
        pbrShader.setParameter("normalMap", 3);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
        pbrShader.setParameter("irradianceMap", 4);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
        pbrShader.setParameter("prefilterMap", 5);
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
        pbrShader.setParameter("brdfLUT", 6);
        // shadowMaps
        for (int i : {0, 1, 2, 3})
        {
            glActiveTexture(GL_TEXTURE7 + i);
            glBindTexture(GL_TEXTURE_CUBE_MAP, shadowMaps[i]);
            pbrShader.setParameter("shadowMaps[" + to_string(i) + "]", 7 + i);
        }
        // rendering
        const int xNum = 5;
        const int yNum = 5;
        for (int i = 0; i <= xNum; i++)
        {
            for (int j = 0; j <= yNum; j++)
            {
                pbrShader.setParameter("model", scale(translate(mat4(1), vec3(i * 1, 0, j * 1)), vec3(0.5)));
                //
                pbrShader.setParameter("metallic", (float)i / (float)xNum);
                pbrShader.setParameter("roughness", (float)j / (float)yNum);

                geosphere.Draw(&pbrShader);
            }
        }

        /**NOTE - 渲染灯光
         */
        for (const auto& light : lightGroup.getLights())
        {
            if (light.getLightType() != 1)  // 日光不渲染实体
            {
                lightObjShader.use();
                lightObjShader.setParameter("model", scale(translate(mat4(1), light.getPostion()), vec3(0.1)));
                lightObjShader.setParameter("view", view);
                lightObjShader.setParameter("projection", projection);
                lightObjShader.setParameter("lightColor", light.getColor());
                sphere.Draw(&lightObjShader);
                // FIXME - 常量对象只能调用它的常函数
            }
        }

        /**NOTE - skybox
         */
        glFrontFace(GL_CW);  // 把顺时针的面设置为“正面”。
        skyboxShader.use();
        skyboxShader.setParameter("view",
                                  mat4(mat3(view)));  // 除去位移，相当于锁头
        skyboxShader.setParameter("projection", projection);
        glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
        skyboxShader.setParameter("skybox", 0);
        box.Draw(&skyboxShader);
        glFrontFace(GL_CCW);
        // ~SECTION

        // 处理事件、交换缓冲区
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    // 释放内存
    glfwTerminate();
    delete camera;
    return 0;
}

/// @brief 函数“framebuffer_size_callback”根据窗口大小设置视口尺寸。
/// @param window `window` 参数是指向触发回调函数的 GLFW 窗口的指针。
/// @param w
/// “framebuffer_size_callback”函数中的“w”参数表示帧缓冲区的宽度，即OpenGL将渲染图形的区域的大小。
/// @param h
/// “framebuffer_size_callback”函数中的参数“h”表示帧缓冲区的高度（以像素为单位）。它用于设置在OpenGL
/// 上下文中渲染的视口高度。
void framebuffer_size_callback(GLFWwindow* window, int w, int h)
{
    glViewport(0, 0, w, h);
}

/// @brief 函数“mouse_callback”根据 GLFW 窗口中的鼠标移动更新相机的方向。
/// @param window “window”参数是指向接收鼠标输入的
/// GLFW窗口的指针。它用于标识鼠标事件发生的窗口。
/// @param xpos mouse_callback 函数中的 xpos 参数表示鼠标光标位置的当前 x坐标。
/// @param ypos 'mouse_callback` 函数中的 `ypos`参数表示鼠标光标在窗口内的当前
/// y坐标。它是一个双精度值，表示触发回调函数时鼠标光标的垂直位置。
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    camera->ProcessMouseMovement(xpos - mouseLastX, ypos - mouseLastY, true);
    mouseLastX = xpos;
    mouseLastY = ypos;
}

/// @brief 函数“scroll_callback”处理鼠标滚动输入以调整相机位置。
/// @param window `GLFWwindow* window`参数是指向接收滚动输入的窗口的指针。
/// @param xoffset `xoffset` 参数表示水平滚动偏移。
/// @param yoffset
/// 'scroll_callback`函数中的`yoffset`参数表示鼠标滚轮的垂直滚动偏移量。正值表示向上滚动，负值表示向下滚动。
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera->ProcessMouseScroll(yoffset);
}

/// @brief C++ 中的函数“processInput”处理来自
/// GLFW窗口的用户输入，以控制相机的移动和速度。
/// @param window processInput 函数中的 window 参数是一个指向
/// GLFWwindow对象的指针。该对象表示应用程序中用于渲染图形和处理用户输入的窗口。该函数使用此参数来检查按键并更新相机的移动和速度
void processInput(GLFWwindow* window)
{
    // 当Esc按下时，窗口关闭
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, 1);

    // 按下Shift时，飞行加速
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        camera->SpeedUp(true);
    else
        camera->SpeedUp(false);

    // 处理摄像机移动
    GLint direction[6] = {0, 0, 0, 0, 0, 0};
    direction[0]       = (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) ? 1 : 0;
    direction[1]       = (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) ? 1 : 0;
    direction[2]       = (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) ? 1 : 0;
    direction[3]       = (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) ? 1 : 0;
    direction[4]       = (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) ? 1 : 0;
    direction[5]       = (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) ? 1 : 0;
    camera->ProcessKeyboard(direction, deltaTime);
}

/// @brief 该函数使用指定的 OpenGL 上下文版本和回调初始化 GLFW 窗口。
/// @param window `initGLFWWindow` 函数中的 `window` 参数是指向GLFWwindow
/// 对象的指针的引用。此函数初始化GLFW，使用指定参数创建窗口，设置回调，隐藏光标，并初始化
/// GLAD 以进行OpenGL 加载。如果成功，则返回
/// @return
/// 函数“initGLFWWindow”返回一个整数值。如果GLFW窗口初始化成功则返回1，如果创建窗口或加载GLAD失败则返回0。
int initGLFWWindow(GLFWwindow*& window)
{
    // 初始化GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_SAMPLES, 4);  // 多重采样缓冲

    // 创建窗口
    window = glfwCreateWindow(CAMERA_WIDTH, CAMERA_HEIGH, "Window", NULL, NULL);
    if (window == nullptr)
    {
        std::cout << "failed to create a window!" << std::endl;
        glfwTerminate();
        return 0;
    }
    glfwMakeContextCurrent(window);
    // 注册回调函数
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    // 隐藏光标
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // 初始化GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "failed to load GLAD!" << std::endl;
        glfwTerminate();
        return 0;
    }
    return 1;
}

/// @brief 函数 createImageObject 加载图像文件，在
/// OpenGL中创建纹理对象，设置纹理参数，生成纹理，并返回纹理 ID。
/// @param imagePath “createImageObject”函数中的“imagePath”参数是一个指向
/// C风格字符串的指针，该字符串表示要加载并从中创建纹理对象的图像文件的路径。该路径应指向文件系统上图像文件的位置。
/// @return 函数 createImageObjrct 返回一个整数值，它是在
/// OpenGL中加载和创建的图像的纹理 ID。
GLuint createImageObjrct(const char* imagePath, const bool autoGammaCorrection, const bool flip_texture)
{
    // 读取
    GLint width, height, nrChannels;
    stbi_set_flip_vertically_on_load(flip_texture);  // 加载图片时翻转y轴
    GLubyte* data = stbi_load(imagePath, &width, &height, &nrChannels, 0);
    // 创建纹理对象
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    // 设置纹理属性
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // 生成纹理
    if (data)
    {
        if (nrChannels == 3)
        {
            glTexImage2D(GL_TEXTURE_2D, 0, (autoGammaCorrection) ? GL_SRGB : GL_RGB, width, height, 0, GL_RGB,
                         GL_UNSIGNED_BYTE, data);
            //  设置为GL_SRGB时，OpenGL回自动对图片进行重校
            // FIXME - 注意，对于在线性空间下创建的纹理，如法线贴图，不能设置SRGB重校。
        }
        else  // nrChannels == 4
        {
            glTexImage2D(GL_TEXTURE_2D, 0, (autoGammaCorrection) ? GL_SRGB_ALPHA : GL_SRGB_ALPHA, width, height, 0,
                         GL_RGBA, GL_UNSIGNED_BYTE, data);
        }
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
        return -1;
    }
    stbi_image_free(data);            // 释放图像的内存，无论有没有data都释放
    glBindTexture(GL_TEXTURE_2D, 0);  // 解绑
    return texture;
}

/// @brief 加载一个天空盒贴图
/// @param imageFolder 纹理集所在文件夹路径
/// @return 函数 createImageObjrct
/// 返回一个整数值，它是在OpenGL中加载和创建的图像的纹理 ID。
GLuint createSkyboxTexture(const char* imageFolder, const bool autoGammaCorrection)
{
    GLuint cubeTexture;
    glGenTextures(1, &cubeTexture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTexture);
    string basePath            = string(imageFolder);
    string cubeTextureNames[6] = {"right.jpg", "left.jpg", "top.jpg", "bottom.jpg", "front.jpg", "back.jpg"};
    for (int i = 0; i < 6; i++)
    {
        string cubeTexturePath = basePath + cubeTextureNames[i];
        int    width, height, nrChannels;
        stbi_set_flip_vertically_on_load(false);  // 加载图片时翻转y轴
        GLubyte* data = stbi_load(cubeTexturePath.c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, (autoGammaCorrection) ? GL_SRGB : GL_RGB, width, height,
                         0, GL_RGB, GL_UNSIGNED_BYTE, data);
            // 设置纹理属性
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        }
        else { std::cout << "Failed to load texture: " << cubeTexturePath << std::endl; }
        stbi_image_free(data);
    }
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);  // 解绑
    return cubeTexture;
}

/// @brief 创建一个FBO（帧缓冲对象）
/// @param hint 如果hint为"ms"，则使用多重采样
void createFBO(GLuint& fbo, GLuint& texAttachment, GLuint& rbo, const char* hint)
{
    bool useMutiSampled = (strcmp(hint, "ms") == 0);
    // FIXME - 使用strcmp()的时候，不可以出空指针

    // 创建一个帧缓冲
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    //  创建一个纹理附件
    glGenTextures(1, &texAttachment);
    if (useMutiSampled)
    {
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, texAttachment);
        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGB, CAMERA_WIDTH, CAMERA_HEIGH, GL_TRUE);
        // 如果最后一个参数为GL_TRUE，图像将会对每个纹素使用相同的样本位置以及相同数量的子采样点个数。
        glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, texAttachment, 0);
    }
    else
    {
        glBindTexture(GL_TEXTURE_2D, texAttachment);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, CAMERA_WIDTH, CAMERA_HEIGH, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texAttachment, 0);
    }

    // 创建一个多重采样渲染缓冲对象
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    if (useMutiSampled)
    {
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, CAMERA_WIDTH, CAMERA_HEIGH);
    }
    else { glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, CAMERA_WIDTH, CAMERA_HEIGH); }
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
    // FIXME - GL_DEPTH_STENCIL_ATTACHMENT写错了，导致深度缓冲没有初始化成功。

    //  检查帧缓冲状态
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
    {
        cout << "Framebuffer is  complete!" << endl;
    }
    else { cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << endl; }

    // 解绑
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // FIXME - 函数写错了，fbo没有解绑。导致默认的fbo为空。
    if (useMutiSampled) { glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0); }
    else { glBindTexture(GL_TEXTURE_2D, 0); }
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

/// @brief 从一组顶点的硬编码创建几何体
void createObjFromHardcode(GLuint& vao, GLuint& vbo, GLuint& ebo, vector<GLfloat> vertices, vector<GLuint> vertexIdx)
{
    bool useEBO = (vertexIdx.size() > 0);
    // VBO
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_STATIC_DRAW);
    // VAO
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    // EBO
    if (useEBO)
    {
        glGenBuffers(1, &ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, vertexIdx.size() * sizeof(GLuint), vertexIdx.data(), GL_STATIC_DRAW);
    }
    // 解绑
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    if (useEBO) { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); }
}

/// @brief 将指定texture绘制到屏幕上
void renderTextureToScreen(const GLuint screenVAO, const GLuint textureToShow, Shader& screenShader)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT);  // 清除颜色缓冲

    // 绘制屏幕几何对象
    glBindVertexArray(screenVAO);
    screenShader.use();
    glBindTexture(GL_TEXTURE_2D, textureToShow);
    screenShader.setParameter("screenTexture", 0);
    glDisable(GL_DEPTH_TEST);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    // 解绑
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    // 恢复深度测试
    glEnable(GL_DEPTH_TEST);
}

/**REVIEW - PBR
我们大致上清楚这个反射方程在干什么，但我们仍然留有一些迷雾尚未揭开。比如说我们究竟将怎样表示场景上的辐照度(Irradiance),
辐射率(Radiance) L ？我们知道辐射率L（在计算机图形领域中）表示光源的辐射通量(Radiant
flux)ϕ，或光源在给定立体角ω下发出的光能。在我们的情况下，不妨假设立体角ω无限小，这样辐射度就表示光源在一条光线或单个方向向量上的辐射通量。

基于以上的知识，我们如何将其转化为之前的教程中所积累的一些光照知识呢？
那么想象一下，我们有一个点光源（一个在所有方向都具有相同亮度的光源），它的辐射通量为用RGB表示为（23.47,
21.31, 20.79）。该光源的辐射强度(Radiant Intensity)等于其在所有出射光线的辐射通量。
然而，当我们为一个表面上的特定的点p着色时，在其半球领域Ω的所有可能的入射方向上，只有一个入射方向向量ωi直接来自于该点光源。
假设我们在场景中只有一个光源，位于空间中的某一个点，因而对于p点的其他可能的入射光线方向上的辐射率为0：
![](https://learnopengl-cn.github.io/img/07/02/lighting_radiance_direct.png)

如果从一开始，我们就假设点光源不受光线衰减（光照强度会随着距离变暗）的影响，那么无论我们把光源放在哪，入射光线的辐射率总是一样的（除去入射角cosθ对辐射率的影响之外）。
这是因为无论我们从哪个角度观察它，点光源总具有相同的辐射强度，我们可以有效地将其辐射强度建模为其辐射通量:
一个常量向量（23.47, 21.31, 20.79）。

然而，辐射率也需要将位置p作为输入，正如所有现实的点光源都会受光线衰减影响一样，点光源的辐射强度应该根据点p所在的位置和光源的位置以及他们之间的距离而做一些缩放。
因此，根据原始的辐射方程，我们会根据表面法向量n和入射角度wi来缩放光源的辐射强度。

在实现上来说：对于直接点光源的情况，辐射率函数L先获取光源的颜色值，
然后光源和某点p的距离衰减，接着按照n⋅wi缩放，但是仅仅有一条入射角为wi的光线打在点p上，
这个wi同时也等于在p点光源的方向向量。写成代码的话会是这样：

vec3  lightColor  = vec3(23.47, 21.31, 20.79);
vec3  wi          = normalize(lightPos - fragPos);
float cosTheta    = max(dot(N, Wi), 0.0);
float attenuation = calculateAttenuation(fragPos, lightPos);
float radiance    = lightColor * attenuation * cosTheta;

当涉及到直接光照(direct
lighting)时，辐射率的计算方式和我们之前计算只有一个光源照射在物体表面的时候非常相似。

请注意，这个假设成立的条件是点光源体积无限小，相当于在空间中的一个点。如果我们认为该光源是具有体积的，它的辐射率会在不只一个入射光方向上非零。

对于其它类型的从单点发出来的光源我们类似地计算出辐射率。比如，定向光(directional light)拥有恒定的wi
而不会有衰减因子；而一个聚光灯光源则没有恒定的辐射强度，其辐射强度是根据聚光灯的方向向量来缩放的。

这也让我们回到了对于表面的半球领域(hemisphere)Ω的积分∫上。由于我们事先知道的所有贡献光
源的位置，因此对物体表面上的一个点着色并不需要我们尝试去求解积分。我们可以直接拿光源的（
已知的）数目，去计算它们的总辐照度，因为每个光源仅仅只有一个方向上的光线会影响物体表面
的辐射率。这使得PBR对直接光源的计算相对简单，因为我们只需要有效地遍历所有有贡献的光源。
而当我们之后把环境照明也考虑在内的IBL教程中，我们就必须采取积分去计算了，这是因为光线可
能会在任何一个方向入射。
*/

/**REVIEW - 漫反射辐照度
基于图像的光照(Image based lighting,
IBL)是一类光照技术的集合。其光源不是如前一节教程中描述的可分解的直接光源，而是将周围环境整体视为一个大光源。IBL
通常使用（取自现实世界或从3D场景生成的）环境立方体贴图 (Cubemap)
，我们可以将立方体贴图的每个像素视为光源，在渲染方程中直接使用它。这种方式可以有效地捕捉环境的全局光照和氛围，使物体更好地融入其环境。

由于基于图像的光照算法会捕捉部分甚至全部的环境光照，通常认为它是一种更精确的环境光照输入格式，甚至也可以说是一种全局光照的粗略近似。基于此特性，IBL
对 PBR 很有意义，因为当我们将环境光纳入计算之后，物体在物理方面看起来会更加准确。

要开始将 IBL 引入我们的 PBR 系统，让我们再次快速看一下反射方程：

如前所述，我们的主要目标是计算半球 Ω 上所有入射光方向
wi的积分。解决上一节教程中的积分非常简单，因为我们事先已经知道了对积分有贡献的、若干精确的光线方向
wi。然而这次，来自周围环境的每个方向wi的入射光都可能具有一些辐射度，使得解决积分变得不那么简单。这为解决积分提出了两个要求：

- 给定任何方向向量 wi ，我们需要一些方法来获取这个方向上场景的辐射度。
- 解决积分需要快速且实时。

现在看，第一个要求相对容易些。我们已经有了一些思路：表示环境或场景辐照度的一种方式是（预处理过的）环境立方体贴图，给定这样的立方体贴图，我们可以将立方体贴图的每个纹素视为一个光源。使用一个方向向量
wi 对此立方体贴图进行采样，我们就可以获取该方向上的场景辐照度。

vec3 radiance =  texture(_cubemapEnvironment, w_i).rgb;

这给了我们一个只依赖于 wi 的积分（假设 p
位于环境贴图的中心）。有了这些知识，我们就可以计算或预计算一个新的立方体贴图，它在每个采样方向——也就是纹素——中存储漫反射积分的结果，这些结果是通过卷积计算出来的。

卷积的特性是，对数据集中的一个条目做一些计算时，要考虑到数据集中的所有其他条目。这里的数据集就是场景的辐射度或环境贴图。因此，要对立方体贴图中的每个采样方向做计算，我们都会考虑半球
Ω 上的所有其他采样方向。

为了对环境贴图进行卷积，我们通过对半球 Ω
上的大量方向进行离散采样并对其辐射度取平均值，来计算每个输出采样方向 wo 的积分。用来采样方向 wi
的半球，要面向卷积的输出采样方向 wo 。

这个预计算的立方体贴图，在每个采样方向 wo 上存储其积分结果，可以理解为场景中所有能够击中面向 wo
的表面的间接漫反射光的预计算总和。这样的立方体贴图被称为辐照度图，因为经过卷积计算的立方体贴图能让我们从任何方向有效地直接采样场景（预计算好的）辐照度。

辐射方程也依赖了位置
p，不过这里我们假设它位于辐照度图的中心。这就意味着所有漫反射间接光只能来自同一个环境贴图，这样可能会破坏现实感（特别是在室内）。渲染引擎通过在场景中放置多个反射探针来解决此问题，每个反射探针单独预计算其周围环境的辐照度图。这样，位置
p
处的辐照度（以及辐射度）是取离其最近的反射探针之间的辐照度（辐射度）内插值。目前，我们假设总是从中心采样环境贴图，把反射探针的讨论留给后面的教程。

![](https://learnopengl-cn.github.io/img/07/03/01/ibl_irradiance.png)

由于立方体贴图每个纹素中存储了（ wo
方向的）卷积结果，辐照度图看起来有点像环境的平均颜色或光照图。使用任何一个向量对立方体贴图进行采样，就可以获取该方向上的场景辐照度。

* NOTE - PBR 和 HDR
我们在光照教程中简单提到过：在 PBR 渲染管线中考虑高动态范围(High Dynamic Range,
HDR)的场景光照非常重要。由于 PBR
的大部分输入基于实际物理属性和测量，因此为入射光值找到其物理等效值是很重要的。无论我们是对光线的辐射通量进行研究性猜测，还是使用它们的直接物理等效值，诸如一个简单灯泡和太阳之间的这种差异都是很重要的，如果不在
HDR 渲染环境中工作，就无法正确指定每个光的相对强度。

因此，PBR 和 HDR 需要密切合作，但这些与基于图像的光照有什么关系？我们在之前的教程中已经看到，让 PBR
在 HDR
下工作还比较容易。然而，回想一下基于图像的光照，我们将环境的间接光强度建立在环境立方体贴图的颜色值上，我们需要某种方式将光照的高动态范围存储到环境贴图中。

我们一直使用的环境贴图是以立方体贴图形式储存——如同一个天空盒——属于低动态范围(Low Dynamic Range,
LDR)。我们直接使用各个面的图像的颜色值，其范围介于 0.0 和 1.0
之间，计算过程也是照值处理。这样虽然可能适合视觉输出，但作为物理输入参数，没有什么用处。

* NOTE - 辐射度的 HDR 文件格式
谈及辐射度的文件格式，辐射度文件的格式（扩展名为
.hdr）存储了一张完整的立方体贴图，所有六个面数据都是浮点数，允许指定 0.0 到 1.0
范围之外的颜色值，以使光线具有正确的颜色强度。这个文件格式使用了一个聪明的技巧来存储每个浮点值：它并非直接存储每个通道的
32 位数据，而是每个通道存储 8 位，再以 alpha
通道存放指数——虽然确实会导致精度损失，但是非常有效率，不过需要解析程序将每种颜色重新转换为它们的浮点数等效值。

可能与您期望的完全不同，因为图像非常扭曲，并且没有我们之前看到的环境贴图的六个立方体贴图面。这张环境贴图是从球体投影到平面上，以使我们可以轻松地将环境信息存储到一张等距柱状投影图(Equirectangular
Map)
中。有一点确实需要说明：水平视角附近分辨率较高，而底部和顶部方向分辨率较低,在大多数情况下，这是一个不错的折衷方案，因为对于几乎所有渲染器来说，大部分有意义的光照和环境信息都在水平视角附近方向。

* NOTE - 立方体贴图的卷积
如本节教程开头所述，我们的主要目标是计算所有间接漫反射光的积分，其中光照的辐照度以环境立方体贴图的形式给出。我们已经知道，在方向
wi 上采样 HDR 环境贴图，可以获得场景在此方向上的辐射度 L(p,wi)
。虽然如此，要解决积分，我们仍然不能仅从一个方向对环境贴图采样，而要从半球 Ω
上所有可能的方向进行采样，这对于片段着色器而言还是过于昂贵。

然而，计算上又不可能从 Ω
的每个可能的方向采样环境光照，理论上可能的方向数量是无限的。不过我们可以对有限数量的方向采样以近似求解，在半球内均匀间隔或随机取方向可以获得一个相当精确的辐照度近似值，从而离散地计算积分
∫ 。

然而，对于每个片段实时执行此操作仍然太昂贵，因为仍然需要非常大的样本数量才能获得不错的结果，因此我们希望可以预计算。既然半球的朝向决定了我们捕捉辐照度的位置，我们可以预先计算每个可能的半球朝向的辐照度，这些半球朝向涵盖了所有可能的出射方向
wo ：

给定任何方向向量 wi ，我们可以对预计算的辐照度图采样以获取方向 wi
的总漫反射辐照度。为了确定片段上间接漫反射光的数量（辐照度），我们获取以表面法线为中心的半球的总辐照度。获取场景辐照度的方法就简化为：

vec3 irradiance = texture(irradianceMap, N);
现在，为了生成辐照度贴图，我们需要将环境光照求卷积，转换为立方体贴图。假设对于每个片段，表面的半球朝向法向量 N
 ，对立方体贴图进行卷积等于计算朝向 N 的半球 Ω 中每个方向 wi 的总平均辐射率。
*/

/**REVIEW -  镜面反射 IBL
你会注意到 Cook-Torrance 镜面部分（乘以ks
）在整个积分上不是常数，不仅受入射光方向影响，还受视角影响。如果试图解算所有入射光方向加所有可能的视角方向的积分，二者组合数会极其庞大，实时计算太昂贵。Epic
Games 提出了一个解决方案，他们预计算镜面部分的卷积，为实时计算作了一些妥协，这种方案被称为分割求和近似法（split sum
approximation）。 分割求和近似将方程的镜面部分分割成两个独立的部分，我们可以单独求卷积，然后在 PBR
着色器中求和，以用于间接镜面反射部分 IBL。分割求和近似法类似于我们之前求辐照图预卷积的方法，需要 HDR
环境贴图作为其卷积输入。为了理解，我们回顾一下反射方程，但这次只关注镜面反射部分（在上一节教程中已经剥离了漫反射部分）：

由于与辐照度卷积相同的（性能）原因，我们无法以合理的性能实时求解积分的镜面反射部分。因此，我们最好预计算这个积分，以得到像镜面
IBL
贴图这样的东西，用片段的法线对这张图采样并计算。但是，有一个地方有点棘手：我们能够预计算辐照度图，是因为其积分仅依赖于ωi
，并且可以将漫反射反射率常数项移出积分，但这一次，积分不仅仅取决于ωi
，从 BRDF 可以看出

这次积分还依赖ωo
，我们无法用两个方向向量采样预计算的立方体图。如前一个教程中所述，位置p
与此处无关。在实时状态下，对每种可能的ωi
和ωo
的组合预计算该积分是不可行的。 Epic Games
的分割求和近似法将预计算分成两个单独的部分求解，再将两部分组合起来得到后文给出的预计算结果。分割求和近似法将镜面反射积分拆成两个独立的积分：

卷积的第一部分被称为预滤波环境贴图，它类似于辐照度图，是预先计算的环境卷积贴图，但这次考虑了粗糙度。因为随着粗糙度的增加，参与环境贴图卷积的采样向量会更分散，导致反射更模糊，所以对于卷积的每个粗糙度级别，我们将按顺序把模糊后的结果存储在预滤波贴图的
mipmap 中。例如，预过滤的环境贴图在其 5 个 mipmap 级别中存储 5 个不同粗糙度值的预卷积结果，如下图所示：

我们使用 Cook-Torrance BRDF
的法线分布函数(NDF)生成采样向量及其散射强度，该函数将法线和视角方向作为输入。由于我们在卷积环境贴图时事先不知道视角方向，因此
Epic Games 假设视角方向——也就是镜面反射方向——总是等于输出采样方向ωo ，以作进一步近似。翻译成代码如下：

这样，预过滤的环境卷积就不需要关心视角方向了。这意味着当从如下图的角度观察表面的镜面反射时，得到的掠角镜面反射效果不是很好（图片来自文章《Moving
Frostbite to PBR》）。然而，通常可以认为这是一个体面的妥协：

等式的第二部分等于镜面反射积分的 BRDF 部分。如果我们假设每个方向的入射辐射度都是白色的（因此L(p,x)=1.0
 ），就可以在给定粗糙度、光线 ωi
 法线 n
 夹角 n⋅ωi
 的情况下，预计算 BRDF 的响应结果。Epic Games 将预计算好的 BRDF 对每个粗糙度和入射角的组合的响应结果存储在一张 2D
查找纹理(LUT)上，称为BRDF积分贴图。2D 查找纹理存储是菲涅耳响应的系数（R 通道）和偏差值（G
通道），它为我们提供了分割版镜面反射积分的第二个部分：

生成查找纹理的时候，我们以 BRDF 的输入n⋅ωi
（范围在 0.0 和 1.0 之间）作为横坐标，以粗糙度作为纵坐标。有了此 BRDF
积分贴图和预过滤的环境贴图，我们就可以将两者结合起来，以获得镜面反射积分的结果：

float lod             = getMipLevelFromRoughness(roughness);
vec3 prefilteredColor = textureCubeLod(PrefilteredEnvMap, refVec, lod);
vec2 envBRDF          = texture2D(BRDFIntegrationMap, vec2(NdotV, roughness)).xy;
vec3 indirectSpecular = prefilteredColor * (F * envBRDF.x + envBRDF.y)

至此，你应该对 Epic Games
的分割求和近似法的原理，以及它如何近似求解反射方程的间接镜面反射部分有了一些基本印象。让我们现在尝试一下自己构建预卷积部分。

* NOTE - 预滤波HDR环境贴图
注意，因为我们计划采样 prefilterMap 的 mipmap，所以需要确保将其缩小过滤器设置为 GL_LINEAR_MIPMAP_LINEAR
以启用三线性过滤。它存储的是预滤波的镜面反射，基础 mip 级别的分辨率是每面
128×128，对于大多数反射来说可能已经足够了，但如果场景里有大量光滑材料（想想汽车上的反射），可能需要提高分辨率。

在上一节教程中，我们使用球面坐标生成均匀分布在半球 Ω
 上的采样向量，以对环境贴图进行卷积。虽然这个方法非常适用于辐照度，但对于镜面反射效果较差。镜面反射依赖于表面的粗糙度，反射光线可能比较松散，也可能比较紧密，但是一定会围绕着反射向量r
，除非表面极度粗糙：

所有可能出射的反射光构成的形状称为镜面波瓣。随着粗糙度的增加，镜面波瓣的大小增加；随着入射光方向不同，形状会发生变化。因此，镜面波瓣的形状高度依赖于材质。
在微表面模型里给定入射光方向，则镜面波瓣指向微平面的半向量的反射方向。考虑到大多数光线最终会反射到一个基于半向量的镜面波瓣内，采样时以类似的方式选取采样向量是有意义的，因为大部分其余的向量都被浪费掉了，这个过程称为重要性采样。

* NOTE - 蒙特卡洛积分和重要性采样
为了充分理解重要性采样，我们首先要了解一种数学结构，称为蒙特卡洛积分。蒙特卡洛积分主要是统计和概率理论的组合。蒙特卡洛可以帮助我们离散地解决人口统计问题，而不必考虑所有人。

例如，假设您想要计算一个国家所有公民的平均身高。为了得到结果，你可以测量每个公民并对他们的身高求平均，这样会得到你需要的确切答案。但是，由于大多数国家人海茫茫，这个方法不现实：需要花费太多精力和时间。

另一种方法是选择一个小得多的完全随机（无偏）的人口子集，测量他们的身高并对结果求平均。可能只测量 100
人，虽然答案并非绝对精确，但会得到一个相对接近真相的答案，这个理论被称作大数定律。我们的想法是，如果从总人口中测量一组较小的真正随机样本的N
，结果将相对接近真实答案，并随着样本数 N
 的增加而愈加接近。

蒙特卡罗积分建立在大数定律的基础上，并采用相同的方法来求解积分。不为所有可能的（理论上是无限的）样本值 x
 求解积分，而是简单地从总体中随机挑选样本 N
 生成采样值并求平均。随着 N
 的增加，我们的结果会越来越接近积分的精确结果：

 为了求解这个积分，我们在 a
 到 b
 上采样 N
 个随机样本，将它们加在一起并除以样本总数来取平均。pdf
 代表概率密度函数 (probability density function)，它的含义是特定样本在整个样本集上发生的概率。例如，人口身高的 pdf
看起来应该像这样

从该图中我们可以看出，如果我们对人口任意随机采样，那么挑选身高为 1.70 的人口样本的可能性更高，而样本身高为 1.50
的概率较低。

当涉及蒙特卡洛积分时，某些样本可能比其他样本具有更高的生成概率。这就是为什么对于任何一般的蒙特卡洛估计，我们都会根据 pdf
将采样值除以或乘以采样概率。到目前为止，我们每次需要估算积分的时候，生成的样本都是均匀分布的，概率完全相等。到目前为止，我们的估计是无偏的，这意味着随着样本数量的不断增加，我们最终将收敛到积分的精确解。

但是，某些蒙特卡洛估算是有偏的，这意味着生成的样本并不是完全随机的，而是集中于特定的值或方向。这些有偏的蒙特卡洛估算具有更快的收敛速度，它们会以更快的速度收敛到精确解，但是由于其有偏性，可能永远不会收敛到精确解。通常来说，这是一个可以接受的折衷方案，尤其是在计算机图形学中。因为只要结果在视觉上可以接受，解决方案的精确性就不太重要。下文我们将会提到一种（有偏的）重要性采样，其生成的样本偏向特定的方向，在这种情况下，我们会将每个样本乘以或除以相应的
pdf 再求和。

蒙特卡洛积分在计算机图形学中非常普遍，因为它是一种以高效的离散方式对连续的积分求近似而且非常直观的方法：对任何面积/体积进行采样——例如半球
Ω ——在该面积/体积内生成数量 N 的随机采样，权衡每个样本对最终结果的贡献并求和。

蒙特卡洛积分是一个庞大的数学主题，在此不再赘述，但有一点需要提到：生成随机样本的方法也多种多样。默认情况下，每次采样都是我们熟悉的完全（伪）随机，不过利用半随机序列的某些属性，我们可以生成虽然是随机样本但具有一些有趣性质的样本向量。例如，我们可以对一种名为低差异序列的东西进行蒙特卡洛积分，该序列生成的仍然是随机样本，但样本分布更均匀：

![](https://learnopengl-cn.github.io/img/07/03/02/ibl_low_discrepancy_sequence.png)

当使用低差异序列生成蒙特卡洛样本向量时，该过程称为拟蒙特卡洛积分。拟蒙特卡洛方法具有更快的收敛速度，这使得它对于性能繁重的应用很有用。

鉴于我们新获得的有关蒙特卡洛（Monte Carlo）和拟蒙特卡洛（Quasi-Monte
Carlo）积分的知识，我们可以使用一个有趣的属性来获得更快的收敛速度，这就是重要性采样。我们在前文已经提到过它，但是在镜面反射的情况下，反射的光向量被限制在镜面波瓣中，波瓣的大小取决于表面的粗糙度。既然镜面波瓣外的任何（拟）随机生成的样本与镜面积分无关，因此将样本集中在镜面波瓣内生成是有意义的，但代价是蒙特卡洛估算会产生偏差。

本质上来说，这就是重要性采样的核心：只在某些区域生成采样向量，该区域围绕微表面半向量，受粗糙度限制。通过将拟蒙特卡洛采样与低差异序列相结合，并使用重要性采样偏置样本向量的方法，我们可以获得很高的收敛速度。因为我们求解的速度更快，所以要达到足够的近似度，我们所需要的样本更少。因此，这套组合方法甚至可以允许图形应用程序实时求解镜面积分，虽然比预计算结果还是要慢得多。

* NOTE - 低差异序列

在本教程中，我们将使用重要性采样来预计算间接反射方程的镜面反射部分，该采样基于拟蒙特卡洛方法给出了随机的低差异序列。我们将使用的序列被称为
Hammersley 序列，Holger Dammertz 曾仔细描述过它。Hammersley 序列是基于 Van Der Corput
序列，该序列是把十进制数字的二进制表示镜像翻转到小数点右边而得。（译注：原文为 Van Der Corpus 疑似笔误，下文各处同）

给出一些巧妙的技巧，我们可以在着色器程序中非常有效地生成 Van Der Corput 序列，我们将用它来获得 Hammersley
序列，设总样本数为 N，样本索引为 i：

* NOTE - GGX 重要性采样

有别于均匀或纯随机地（比如蒙特卡洛）在积分半球 Ω
 产生采样向量，我们的采样会根据粗糙度，偏向微表面的半向量的宏观反射方向。采样过程将与我们之前看到的过程相似：开始一个大循环，生成一个随机（低差异）序列值，用该序列值在切线空间中生成样本向量，将样本向量变换到世界空间并对场景的辐射度采样。不同之处在于，我们现在使用低差异序列值作为输入来生成采样向量：

 此外，要构建采样向量，我们需要一些方法定向和偏移采样向量，以使其朝向特定粗糙度的镜面波瓣方向。我们可以如理论教程中所述使用
NDF，并将 GGX NDF 结合到 Epic Games 所述的球形采样向量的处理中：

基于特定的粗糙度输入和低差异序列值 Xi，我们获得了一个采样向量，该向量大体围绕着预估的微表面的半向量。注意，根据迪士尼对
PBR 的研究，Epic Games 使用了平方粗糙度以获得更好的视觉效果。

使用低差异 Hammersley 序列和上述定义的样本生成方法，我们可以最终完成预滤波器卷积着色器：

* NOTE - 预过滤卷积的伪像

* NOTE - 高粗糙度的立方体贴图接缝

在具有粗糙表面的表面上对预过滤贴图采样，也就等同于在较低的 mip
级别上对预过滤贴图采样。在对立方体贴图进行采样时，默认情况下，OpenGL不会在立方体面之间进行线性插值。由于较低的 mip
级别具有更低的分辨率，并且预过滤贴图代表了与更大的采样波瓣卷积，因此缺乏立方体的面和面之间的滤波的问题就更明显：

* NOTE - 预过滤卷积的亮点

由于镜面反射中光强度的变化大，高频细节多，所以对镜面反射进行卷积需要大量采样，才能正确反映 HDR
环境反射的混乱变化。我们已经进行了大量的采样，但是在某些环境下，在某些较粗糙的 mip
级别上可能仍然不够，导致明亮区域周围出现点状图案：
![](https://learnopengl-cn.github.io/img/07/03/02/ibl_prefilter_dots.png)

一种解决方案是进一步增加样本数量，但在某些情况下还是不够。另一种方案如 Chetan Jags
所述，我们可以在预过滤卷积时，不直接采样环境贴图，而是基于积分的 PDF 和粗糙度采样环境贴图的 mipmap ，以减少伪像：

* NOTE - 预计算 BRDF

* NOTE - 下一步是？

希望在本教程结束时，你会对 PBR 的相关内容有一个清晰的了解，甚至可以构造并运行一个实际的 PBR
渲染器。在这几节教程中，我们已经在应用程序开始阶段，渲染循环之前，预计算了所有 PBR
相关的基于图像的光照数据。出于教育目的，这很好，但对于任何 PBR
的实践应用来说，都不是很漂亮。首先，预计算实际上只需要执行一次，而不是每次启动时都要做。其次，当使用多个环境贴图时，你必须在每次程序启动时全部预计算一遍，这是个必须步骤。

因此，通常只需要一次将环境贴图预计算为辐照度贴图和预过滤贴图，然后将其存储在磁盘上（注意，BRDF
积分贴图不依赖于环境贴图，因此只需要计算或加载一次）。这意味着您需要提出一种自定义图像格式来存储 HDR 立方体贴图，包括其
mip 级别。或者将图像存储为某种可用格式——例如支持存储 mip 级别的 .dds——并按其格式加载。

此外，我们也在教程中描述了整个过程，包括生成预计算的 IBL 图像，以帮助我们进一步了解 PBR 管线。此外还可以通过 cmftStudio
或 IBLBaker 等一些出色的工具为您生成这些预计算贴图，也很好用。

有一点内容我们跳过了，即如何将预计算的立方体贴图作为反射探针：立方体贴图插值和视差校正。这是一个在场景中放置多个反射探针的过程，这些探针在特定位置拍摄场景的立方体贴图快照，然后我们可以将其卷积，作为相应部分场景的
IBL 数据。基于相机的位置对附近的探针插值，我们可以实现局部的细节丰富的
IBL，受到的唯一限制就是探针放置的数量。这样一来，例如从一个明亮的室外部分移动到较暗的室内部分时，IBL
就能正确更新。我将来会在某个地方编写有关反射探针的教程，但现在，我建议阅读下面 Chetan Jags 的文章来作为入门。
*/
