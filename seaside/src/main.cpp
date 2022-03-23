#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "ocean.h"
#include "skybox.h"
#include "mirror.h"
#include "tools.h"

#include "camera.h"
#include "model.h"

#include <iostream>
#include <windows.h>

//IBL struct
struct IBLMAPS
{
    unsigned int irradianceMap;
    unsigned int prefilterMap;
    unsigned int brdfLUTTexture;
};


void renderCube();
void renderQuad();
IBLMAPS GetIBLMAPS(unsigned int envCubemap, GLFWwindow* window);

void renderSphere();
void LoadModels();
void SetProjectionView(Shader shader, glm::mat4 projection, glm::mat4 view);
void DrawModels(Shader shader, int drawCtrl);
void AddLights(Shader shader, bool renderBall);
void InitCubeMap(unsigned int& framebuffer, unsigned int& cubeBuffer);
void DrawCubeMap(glm::vec3 insideCameraPos, glm::mat4 model, glm::mat4 projection, glm::mat4 view, Skybox& skybox, unsigned int& framebuffer);
void LoadMirror(glm::mat4 &mirror_model, glm::vec4 &Plane, Mirror &mirror_texture, Shader &mirror_shader, unsigned int &cubeVAO);
void MirrorRender(glm::mat4 &mirror_model, glm::vec4 &Plane, Mirror &mirror_texture, Shader &mirror_shader, 
    unsigned int &cubeVAO, glm::mat4 projection, glm::mat4 view, Skybox &skybox, cOcean& ocean);

// settings
const unsigned int SCR_WIDTH = 2000;
const unsigned int SCR_HEIGHT = 1200;
const unsigned int objectNum = 9;

// camera
Camera camera(glm::vec3(0.0f, 2.0f, 6.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

int g_speed;      //移动速度

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

using namespace std;
//using namespace glm;

extern "C" __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;

const int SOFA = 1;
const int PICTURE = 1<<1;
const int LIGHT = 1<<2;
const int MIRROR_L = 1<<3;
const int MIRROR_R = 1<<4;
const int TABLE = 1<<5;
const int TEASET = 1<<6;
const int HOUSE = 1<<7;
const int LANTERN = 1<<8;
const int ALLMODELS = (1<<9)-1;//HOUSE+ TABLE+ MIRROR_R;//

//如果使用VS运行请注释掉此句define
//#define __RUN_WITH_CMAKELISTS__

string g_base_dir = "./";
#ifdef __RUN_WITH_CMAKELISTS__
g_base_dir = "../";
#endif

Model sofa, picture, light, mirror, table, teaSet, house, lantern;
Shader ourShader;
Shader depthShader;
Shader debugShader;

double positions[objectNum][3] = { //+right/-left high/low front/back 
    { 0.0 , 0 , -2.0 } ,    //sofa
    { 0.0 , 2.2 , -2.68 } , //picture
    { 0.0 , 3.0 , 0 } ,     //light
    { -1.5 , 1.5 , -2.9 } , //mirror left
    { 1.5 , 1.5 , -2.9 } ,  //mirror right
    { 0 , 0 , 0 } ,         //table
    { -0.05 , 1.0 , -0.05 } , //teaset
    { 0.0, 2.3, 0.0 },      //house
    { 0.5, 1.0, 0.2}        // lantern
};

glm::vec3 lightPositions[] = {
    glm::vec3(0.0f, 3.0f, 0.0f),
    glm::vec3(0.55f, 3.6f, 0.0f),
    glm::vec3(-0.55f, 3.6f, -0.05f),
    glm::vec3(0.25f, 3.6f, 0.5f),
    glm::vec3(-0.3f, 3.6f, 0.45f),
    glm::vec3(0.3f, 3.6f, -0.45f),
    glm::vec3(-0.3f, 3.6f, -0.45f),
    glm::vec3(100.5f, 1.0f, 0.2f)
};
//glm::vec3 orange = glm::vec3(237.0f, 204.0f, 139.0f);
glm::vec3 orange = glm::vec3(20.0f, 17.0f, 11.0f);
glm::vec3 yellow = glm::vec3(10.0f, 10.0f, 2.0f);
glm::vec3 lightColors[] = {
    orange,
    orange,
    orange,
    orange,
    orange,
    orange,
    orange,
    yellow
};

//申请立方体贴图buffer
unsigned int depthMapFBO;

int is_cube = 0;
int main()
{
    //窗口初始化
    GLFWwindow* window = NULL;
    InitWindow(window, SCR_WIDTH, SCR_HEIGHT, "Seaside House");

    //反转y轴
    stbi_set_flip_vertically_on_load(false);

    //深度测试
    glEnable(GL_DEPTH_TEST);

    string base_dir = g_base_dir;
    ourShader = Shader((base_dir + "shader/pbr.vert.glsl").c_str(), (base_dir + "shader/pbr.frag.glsl").c_str());
    depthShader = Shader((base_dir + "shader/depth.vert.glsl").c_str(), (base_dir + "shader/depth.frag.glsl").c_str());
    debugShader = Shader((base_dir + "shader/debug.vert.glsl").c_str(), (base_dir + "shader/debug.frag.glsl").c_str());
    

    // 阴影的深度贴图准备
    const unsigned int SHADOW_WIDTH = 20000, SHADOW_HEIGHT = 12000;
    glGenFramebuffers(1, &depthMapFBO);
    // create depth texture
    unsigned int depthMap;
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    GLfloat borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    // attach depth texture as FBO's depth buffer
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    ourShader.use();
    ourShader.setInt("albedoMap", 1);
    ourShader.setInt("normalMap", 2);
    ourShader.setInt("armMap", 3);
    ourShader.setInt("irradianceMap", 4);
    ourShader.setInt("prefilterMap", 5);
    ourShader.setInt("brdfLUT", 6);
    ourShader.setInt("shadowMap", 7);


    /****************** 天空盒 ****************/
    //生成纹理
#if 0
    string skybox_dir = base_dir + "skybox/SanFrancisco4/";
    vector<string> faces
    {
        skybox_dir + "posx.jpg",
        skybox_dir + "negx.jpg",

        skybox_dir + "posy.jpg",
        skybox_dir + "negy.jpg",

        skybox_dir + "posz.jpg",
        skybox_dir + "negz.jpg"
    };
#endif
    string skybox_dir = base_dir + "skybox/Ao8/";
    vector<string> faces
    {
        skybox_dir + "px.png",
        skybox_dir + "nx.png",

        skybox_dir + "py.png",
        skybox_dir + "ny.png",

        skybox_dir + "pz.png",
        skybox_dir + "nz.png"
    };
    unsigned int cubemapTexture = loadCubemap(faces);

    Skybox skybox(base_dir + "shader/skybox.vert.glsl", base_dir + "shader/skybox.frag.glsl");
    skybox.textureSkybox = cubemapTexture;		//天空盒贴图

    /****************** 海面 ****************/
    //FFT点数N，振幅A，风向[vec2]wind，单块海面边长length，是否显示线框，缩放系数，海面块数(nx,nz)，海面位置偏移[vec3]offset
    //### 单块海面最终的边长是length*scale，渲染时会缩放model
    glm::vec3 oceanOffset(-1000.0f, -12.0f, 600.0f);
    cOcean ocean(64, 0.0010f, glm::vec2(4.0f, 4.0f), 32, false, 0.3f, 64, 64, oceanOffset,
        base_dir + "shader/ocean.vert.glsl", base_dir + "shader/ocean.frag.glsl");
    
    ocean.textureSkybox = cubemapTexture;		//天空盒贴图
    ocean.sunlight_position = glm::vec3(0.0f, 200.0f, -100.0f);     //太阳光位置

    /****************** 模型 ****************/
    //加载模型
    LoadModels();
   
    //变换矩阵
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
    glm::mat4 view = camera.GetViewMatrix();

    //制作立方体贴图
    //初始化
    unsigned int teasetframebuffer;
    unsigned int teasetcubeBuffer;
    unsigned int lightframebuffer;
    unsigned int lightcubeBuffer;
    auto model = glm::mat4(1.0f);
    //teaset cubemap

    model = glm::mat4(1.0f);
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::translate(model, glm::vec3({ positions[6][0], positions[6][1], positions[6][2] }));
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    InitCubeMap(teasetframebuffer, teasetcubeBuffer);
    DrawCubeMap(glm::vec3(0, 0, 0), model, projection, view, skybox, teasetframebuffer);
    // teaset ibl
    IBLMAPS cubeIBLmaps = GetIBLMAPS(teasetcubeBuffer, window);
    teaSet.SetIBLTexture(cubeIBLmaps.irradianceMap, cubeIBLmaps.prefilterMap, cubeIBLmaps.brdfLUTTexture);

    //light cubemap
    model = glm::mat4(1.0f);
    model = glm::rotate(model, glm::radians(-5.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::translate(model, glm::vec3(positions[2][0], positions[2][1], positions[2][2]));
    model = glm::scale(model, glm::vec3(2.0f));
    InitCubeMap(lightframebuffer, lightcubeBuffer);
    DrawCubeMap(glm::vec3(0,0,0), model, projection, view, skybox, lightframebuffer);
    //light ibl
    IBLMAPS lightIBLmaps = GetIBLMAPS(lightcubeBuffer, window);
    light.meshes[0].textures.push_back(Texture{ lightIBLmaps.irradianceMap,"irradianceMap","" });
    light.meshes[0].textures.push_back(Texture{ lightIBLmaps.prefilterMap,"prefilterMap","" });
    light.meshes[0].textures.push_back(Texture{ lightIBLmaps.brdfLUTTexture,"brdfLUT","" });
    light.meshes[0].isIBL = true;
    //light.SetIBLTexture(lightIBLmaps.irradianceMap, lightIBLmaps.prefilterMap, lightIBLmaps.brdfLUTTexture);
    //light.meshes[1].isIBL = false;

    //房屋IBL
    IBLMAPS houseIBLmaps = GetIBLMAPS(skybox.textureSkybox, window);
    house.SetIBLTexture(houseIBLmaps.irradianceMap, houseIBLmaps.prefilterMap, houseIBLmaps.brdfLUTTexture);
    house.meshes[1].isIBL = false;

    //镜子预处理
    glm::mat4 mirror_model;
    glm::vec4 Plane;
    Mirror mirror_texture;
    Shader mirror_shader;
    unsigned int cubeVAO;
    LoadMirror(mirror_model, Plane, mirror_texture, mirror_shader, cubeVAO);

    // 这部分渲染阴影深度贴图 ==============START==============
    glm::mat4 lightProjection, lightView;
    glm::mat4 lightSpaceMatrix;
    float near_plane = 0.1f, far_plane = 10.0f;
    lightProjection = glm::ortho(-2.8f, 2.2f, -2.8f, 2.6f, near_plane, far_plane);
    lightView = glm::lookAt(lightPositions[0], glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    lightSpaceMatrix = lightProjection * lightView;
    // render scene from light's point of view
    depthShader.use();
    depthShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    //绘制模型, 参数：(shader, 绘制哪些模型)
    DrawModels(depthShader, ALLMODELS ^ MIRROR_L);

    //glCullFace(GL_BACK);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // reset viewport
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // 这部分渲染阴影深度贴图 ==============END==============

    ourShader.use();
    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    ourShader.setMat4("lightSpaceMatrix", lightSpaceMatrix);

    // 沙发不渲染阴影
    sofa.setUseShadow(false);
    teaSet.setUseShadow(false);


    //深度测试
    glEnable(GL_DEPTH_TEST);
    //允许设置透明度
    glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    g_speed = 1;        //移动速度
    ocean.resetTimer();     //重新计时
    ocean.m_speed_factor = 1.0f;

    int render_cnt = 0;
    int render_n = 1;


    //渲染循环
    while (!glfwWindowShouldClose(window)) {
        //计算帧差
        float currentFrame = float(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        //O键加速，P键减速
        if (g_speed < -32)
            g_speed = -32;
        else if (g_speed > 50)
            g_speed = 50;
        //调整移动速度
        if (g_speed > 0)
            deltaTime *= g_speed;
        else if (g_speed < 0)
            deltaTime /= (-g_speed);

        //处理输入
        processInput(window);

        //是否重新计算海面FFT
        if (render_cnt % render_n == 0)
            ocean.compute_flag = true;
        render_cnt = (render_cnt + 1) % render_n;

        //设置背景，清除缓冲
        glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 阴影调试需要
        //debugShader.use();
        //glBindTexture(GL_TEXTURE_2D, depthMap);
        //renderQuad();

        //变换矩阵
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 1000.0f);
        glm::mat4 view = camera.GetViewMatrix();

        //渲染镜子准备
        MirrorRender(mirror_model, Plane, mirror_texture, mirror_shader, cubeVAO, projection, view, skybox, ocean);

        //正常渲染所有物体，包括镜子，此时没有镜像
        glStencilMask(0x00);
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

        //正常渲染开始
        ourShader.use();
        //设置shader中projection和view参数
        SetProjectionView(ourShader, projection, view);
        //绘制模型, 参数：(shader, 绘制哪些模型)
        DrawModels(ourShader, ALLMODELS ^ MIRROR_L);
        //添加光源效果，深度渲染无须此项
        AddLights(ourShader, false);

        /****************** 天空盒 ****************/
        skybox.render(projection, view);

        /****************** 海面 ****************/
        ocean.render(ocean.deltaTime(), projection, view);

        //交换缓冲，检查并调用事件
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    //释放资源
    ocean.release();
    skybox.release();

    glfwTerminate();
    return 0;
}

unsigned int sphereVAO = 0;
unsigned int indexCount;
void renderSphere()
{
    if (sphereVAO == 0)
    {
        glGenVertexArrays(1, &sphereVAO);

        unsigned int vbo, ebo;
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> uv;
        std::vector<glm::vec3> normals;
        std::vector<unsigned int> indices;

        const unsigned int X_SEGMENTS = 64;
        const unsigned int Y_SEGMENTS = 64;
        const float PI = 3.14159265359f;
        for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
        {
            for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
            {
                float xSegment = (float)x / (float)X_SEGMENTS;
                float ySegment = (float)y / (float)Y_SEGMENTS;
                float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
                float yPos = std::cos(ySegment * PI);
                float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

                positions.push_back(glm::vec3(xPos, yPos, zPos));
                uv.push_back(glm::vec2(xSegment, ySegment));
                normals.push_back(glm::vec3(xPos, yPos, zPos));
            }
        }

        bool oddRow = false;
        for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
        {
            if (!oddRow) // even rows: y == 0, y == 2; and so on
            {
                for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
                {
                    indices.push_back(y       * (X_SEGMENTS + 1) + x);
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                }
            }
            else
            {
                for (int x = X_SEGMENTS; x >= 0; --x)
                {
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                    indices.push_back(y       * (X_SEGMENTS + 1) + x);
                }
            }
            oddRow = !oddRow;
        }
        indexCount = indices.size();

        std::vector<float> data;
        for (unsigned int i = 0; i < positions.size(); ++i)
        {
            data.push_back(positions[i].x);
            data.push_back(positions[i].y);
            data.push_back(positions[i].z);           
            if (normals.size() > 0)
            {
                data.push_back(normals[i].x);
                data.push_back(normals[i].y);
                data.push_back(normals[i].z);
            }
            if (uv.size() > 0)
            {
                data.push_back(uv[i].x);
                data.push_back(uv[i].y);
            }
        }
        glBindVertexArray(sphereVAO);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        unsigned int stride = (3 + 2 + 3) * sizeof(float);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(1);        
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));        
    }

    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
}

//assimp库如果是自己使用x86编译生成的动态库，将不支持相对路径访问（也可能是不识别..或/），如果无法读取文件，请使用绝对路径
void LoadModels()
{
    string base_dir = g_base_dir;
    sofa = Model(base_dir + "models/sofa_03/sofa_03_4k.gltf");
    picture = Model(base_dir + "models/fancy_picture_frame_01/fancy_picture_frame_01_4k.gltf");
    light = Model(base_dir + "models/Chandelier_02/Chandelier_02_4k.gltf");
    mirror = Model(base_dir + "models/ornate_mirror_01/ornate_mirror_01_4k.gltf");
    table = Model(base_dir + "models/round_wooden_table_01/round_wooden_table_01_4k.gltf");
    teaSet = Model(base_dir + "models/tea_set_01/tea_set_01_4k.gltf");
    house = Model(base_dir + "models/simple_medieval_house/scene.gltf");
    //lantern = Model(base_dir + "models/lantern_01/lantern_01_4k.gltf");
    
}

//设置shader中projection和view参数
void SetProjectionView(Shader shader, glm::mat4 projection, glm::mat4 view)
{
    shader.use();
//    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
//    glm::mat4 view = camera.GetViewMatrix();
    if (is_cube) {
        projection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);
    }
    
    shader.setMat4("projection", projection);
    shader.setMat4("view", view);
    shader.setVec3("camPos", camera.Position);
}

//绘制模型, 参数：(shader, 绘制哪些模型)
void DrawModels(Shader shader, int drawCtrl)
{
    glm::mat4 model = glm::mat4(1.0f);
    if (drawCtrl & SOFA) {
        model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::translate(model, glm::vec3(positions[0][0], positions[0][1], positions[0][2])); // translate it down so it's at the center of the scene
        model = glm::scale(model, glm::vec3(1.0f));	// it's a bit too big for our scene, so scale it down
        shader.setMat4("model", model);
        sofa.Draw(shader);
    }

    if (drawCtrl & PICTURE) {
        model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::translate(model, glm::vec3(positions[1][0], positions[1][1], positions[1][2]));
        shader.setMat4("model", model);
        picture.Draw(shader);
    }

    if (drawCtrl & LIGHT) {
        model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(-5.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::translate(model, glm::vec3(positions[2][0], positions[2][1], positions[2][2]));
        model = glm::scale(model, glm::vec3(2.0f));
        shader.setMat4("model", model);
        light.Draw(shader);
    }

    if (drawCtrl & MIRROR_L) {
        model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::translate(model, glm::vec3(positions[3][0], positions[3][1], positions[3][2]));
        model = glm::scale(model, glm::vec3(2.0f));
        shader.setMat4("model", model);
        mirror.Draw(shader);
    }

    if (drawCtrl & MIRROR_R) {
        model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::translate(model, glm::vec3(positions[4][0], positions[4][1], positions[4][2]));
        model = glm::scale(model, glm::vec3(2.0f));
        shader.setMat4("model", model);
        mirror.Draw(shader);
    }

    if (drawCtrl & TABLE) {
        model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(positions[5][0], positions[5][1], positions[5][2]));
        shader.setMat4("model", model);
        table.Draw(shader);
    }

    if (drawCtrl & TEASET) {
        double teaPos[10][3] = { 
            { positions[6][0], positions[6][1], positions[6][2] } ,
            { positions[6][0]+0.4, positions[6][1], positions[6][2] } ,
            { positions[6][0]+0.4, positions[6][1], positions[6][2] } ,
            { positions[6][0], positions[6][1]+0.005, positions[6][2] },
            { positions[6][0]+0.2, positions[6][1], positions[6][2]+0.2 } ,
            { positions[6][0]+0.2, positions[6][1], positions[6][2] } ,
            { positions[6][0]+0.35, positions[6][1], positions[6][2]-0.15 } ,
            { positions[6][0]+0.35, positions[6][1], positions[6][2]-0.15 } ,
            { positions[6][0]+0.2, positions[6][1], positions[6][2]+0.2 } ,
            { positions[6][0]+0.2, positions[6][1], positions[6][2] }
        };

        for (unsigned int i = 0; i < teaSet.meshes.size(); i++){
            model = glm::mat4(1.0f);
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::translate(model, glm::vec3(teaPos[i][0], teaPos[i][1], teaPos[i][2]));
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            shader.setMat4("model", model);
            teaSet.meshes[i].Draw(shader);
        }
    }

    if (drawCtrl & HOUSE) {
        model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::translate(model, glm::vec3(positions[7][0], positions[7][1], positions[7][2]));
        model = glm::scale(model, glm::vec3(8.0f, 8.0f, 8.0f));	// it's a bit too big for our scene, so scale it down
        shader.setMat4("model", model);
        //house.Draw(shader);
        for (unsigned int i = 0; i < house.meshes.size()-1; i++){
            house.meshes[i].Draw(shader);
        }
        model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::translate(model, glm::vec3(positions[7][0], positions[7][1]-7.5f, positions[7][2]));
        model = glm::scale(model, glm::vec3(8.0f, 8.0f, 8.0f));	// it's a bit too big for our scene, so scale it down
        shader.setMat4("model", model);
        house.meshes[house.meshes.size()-1].Draw(shader);
    }

    if (drawCtrl & LANTERN) {
        model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::translate(model, glm::vec3(positions[8][0], positions[8][1], positions[8][2]));
        model = glm::scale(model, glm::vec3(1.0f));	// it's a bit too big for our scene, so scale it down
        shader.setMat4("model", model);
        lantern.Draw(shader);
    }

}

//添加光源效果，参数：(shader, 是否在光源点绘制小球)
void AddLights(Shader shader, bool renderBall)
{
    glm::mat4 lightmodel = glm::mat4(1.0f);
    for (unsigned int i = 0; i < sizeof(lightPositions) / sizeof(lightPositions[0]); ++i){
        //glm::vec3 newPos = glm::vec3(-lightPositions[i].x, -lightPositions[i].y, -lightPositions[i].z);
        glm::vec3 newPos = lightPositions[i];
        shader.setVec3("lightPositions[" + std::to_string(i) + "]", newPos);
        shader.setVec3("lightColors[" + std::to_string(i) + "]", lightColors[i]);
        lightmodel = glm::mat4(1.0f);
        lightmodel = glm::translate(lightmodel, lightPositions[i]);
        lightmodel = glm::scale(lightmodel, glm::vec3(0.1f));
        shader.setMat4("model", lightmodel);
        if (renderBall)
            renderSphere();
    }
}

void InitCubeMap(unsigned int& framebuffer, unsigned int& cubeBuffer)
{
    // create and bind FBO
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    
    glGenTextures(1, &cubeBuffer);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubeBuffer);
    for (int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, 512, 512, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    // attach it to currently bound FBO
    for (int i = 0; i < 6; ++i)
    {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubeBuffer, 0);
    }
    // generate renderbuffer (as depth buffer and stencil buffer)
    unsigned int rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 512, 512);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    // attach it to currently bound FBO
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    // check framebuffer status
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!\n";
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void DrawCubeMap(glm::vec3 insideCameraPos, glm::mat4 model, glm::mat4 projection, glm::mat4 view, Skybox &skybox,unsigned int &framebuffer)
{



    insideCameraPos = model * glm::vec4(insideCameraPos, 1.0);
    Camera rightCamera(insideCameraPos, glm::vec3(0.0f, -1.0f, 0.0f), 0.0f, 0.0f);
    Camera leftCamera(insideCameraPos, glm::vec3(0.0f, -1.0f, 0.0f), -180.0f, 0.0f);
    Camera topCamera(insideCameraPos, glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 90.0f);
    Camera bottomCamera(insideCameraPos, glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, -90.0f);
    Camera backCamera(insideCameraPos, glm::vec3(0.0f, -1.0f, 0.0f), 90.0f, 0.0f);
    Camera frontCamera(insideCameraPos, glm::vec3(0.0f, -1.0f, 0.0f), -90.0f, 0.0f);
    Camera temp_camera=camera;
    std::vector<Camera*> cameraList = { &rightCamera, &leftCamera, &topCamera, &bottomCamera,&backCamera ,&frontCamera, };
    is_cube = 1;
    for (int i = 0; i < 6; ++i)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glDrawBuffer(GL_COLOR_ATTACHMENT0 + i);
        camera = *cameraList[i];
        //正常渲染开始
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        skybox.render(projection, view);

        ourShader.use();
        AddLights(ourShader, false);
        //设置shader中projection和view参数
        SetProjectionView(ourShader, projection, view);
        //绘制模型, 参数：(shader, 绘制哪些模型)
        DrawModels(ourShader, ALLMODELS ^ MIRROR_L);
        //添加光源效果，深度渲染无须此项
        
    }
    is_cube = 0;
    camera = temp_camera;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void renderQuad()
{
    static unsigned int quadVAO, quadVBO;
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void renderCube()
{
    static unsigned int cubeVAO = 0;
    static unsigned int cubeVBO = 0;
    static bool init = false;
    // initialize (if necessary)
    if (!init)
    {
        init = true;
        float vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
            // bottom face
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            // top face
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
             1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
             1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // render Cube
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

IBLMAPS GetIBLMAPS(unsigned int envCubemap, GLFWwindow* window)
{
    static bool init = false;
    static unsigned int captureFBO;
    static unsigned int captureRBO;
    static Shader irradianceShader, prefilterShader, brdfShader;
    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] =
    {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),

    };
    if (!init)
    {
        init = true;
        glGenFramebuffers(1, &captureFBO);
        glGenRenderbuffers(1, &captureRBO);

        string base_dir = g_base_dir;
        irradianceShader = Shader((base_dir + "shader/cubemap.vert.glsl").c_str(), (base_dir + "shader/irradiance_convolution.frag.glsl").c_str());
        prefilterShader = Shader((base_dir + "shader/cubemap.vert.glsl").c_str(), (base_dir + "shader/prefilter.frag.glsl").c_str());
        brdfShader = Shader((base_dir + "shader/brdf.vert.glsl").c_str(), (base_dir + "shader/brdf.frag.glsl").c_str());

    }

    IBLMAPS retMaps;
    // then let OpenGL generate mipmaps from first mip face (combatting visible dots artifact)
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);


    // pbr: create an irradiance cubemap, and re-scale capture FBO to irradiance scale.
    // --------------------------------------------------------------------------------
    glGenTextures(1, &retMaps.irradianceMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, retMaps.irradianceMap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);
    // pbr: solve diffuse integral by convolution to create an irradiance (cube)map.
    // -----------------------------------------------------------------------------
    irradianceShader.use();
    irradianceShader.setInt("environmentMap", 0);
    irradianceShader.setMat4("projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

    glViewport(0, 0, 32, 32); // don't forget to configure the viewport to the capture dimensions.
    for (unsigned int i = 0; i < 6; ++i)
    {
        irradianceShader.setMat4("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
            retMaps.irradianceMap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderCube();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // pbr: create a pre-filter cubemap, and re-scale capture FBO to pre-filter scale.
    // --------------------------------------------------------------------------------
    glGenTextures(1, &retMaps.prefilterMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, retMaps.prefilterMap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // be sure to set minification filter to mip_linear 
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // generate mipmaps for the cubemap so OpenGL automatically allocates the required memory.
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    // pbr: run a quasi monte-carlo simulation on the environment lighting to create a prefilter (cube)map.
    // ----------------------------------------------------------------------------------------------------
    prefilterShader.use();
    prefilterShader.setInt("environmentMap", 0);
    prefilterShader.setMat4("projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    unsigned int maxMipLevels = 5;
    for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
    {
        // reisze framebuffer according to mip-level size.
        unsigned int mipWidth = unsigned int(128 * std::pow(0.5, mip));
        unsigned int mipHeight = unsigned int(128 * std::pow(0.5, mip));
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);
        glViewport(0, 0, mipWidth, mipHeight);

        float roughness = (float)mip / (float)(maxMipLevels - 1);
        prefilterShader.setFloat("roughness", roughness);
        for (unsigned int i = 0; i < 6; ++i)
        {
            prefilterShader.setMat4("view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, retMaps.prefilterMap, mip);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            renderCube();
        }
    }
    // pbr: generate a 2D LUT from the BRDF equations used.
    // ----------------------------------------------------
    glGenTextures(1, &retMaps.brdfLUTTexture);

    // pre-allocate enough memory for the LUT texture.
    glBindTexture(GL_TEXTURE_2D, retMaps.brdfLUTTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
    // be sure to set wrapping mode to GL_CLAMP_TO_EDGE
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // then re-configure capture framebuffer object and render screen-space quad with BRDF shader.
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, retMaps.brdfLUTTexture, 0);

    glViewport(0, 0, 512, 512);
    brdfShader.use();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderQuad();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    int scrWidth, scrHeight;
    glfwGetFramebufferSize(window, &scrWidth, &scrHeight);
    glViewport(0, 0, scrWidth, scrHeight);

    return retMaps;
}

void LoadMirror(glm::mat4 &mirror_model, glm::vec4 &Plane, Mirror &mirror_texture, Shader &mirror_shader, unsigned int &cubeVAO)
{
    
    //mirror define
    mirror_model = glm::mat4(1.0f);
    mirror_model = glm::rotate(mirror_model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    mirror_model = glm::translate(mirror_model, glm::vec3(positions[4][0], positions[4][1], positions[4][2]));
    mirror_model = glm::scale(mirror_model, glm::vec3(2.0f));
    glm::vec3 mirror_pos = glm::vec3(0,0,0), mirror_normal(0, 0, 1);
    mirror_pos = mirror_model * glm::vec4(mirror_pos, 1.0);
    mirror_normal = glm::mat3(mirror_model) * mirror_normal;
    mirror_texture = Mirror(mirror_pos, mirror_normal, SCR_WIDTH, SCR_HEIGHT);
    //clip 
    Plane = glm::vec4(mirror_normal, -glm::dot(mirror_pos, mirror_normal));

    string base_dir = g_base_dir;
    mirror_shader = Shader((base_dir + "shader/framebuffers.vert.glsl").c_str(), (base_dir + "shader/framebuffers.frag.glsl").c_str());

    float cubeVertices[] = {
        // positions          // texture Coords
         -1.0f, -1.0f,  0.0f,  0.0f, 0.0f,
         1.0f,  -1.0f,  0.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  0.0f,  1.0f, 1.0f,
         1.0f,  1.0f,  0.0f,  1.0f, 1.0f,
         -1.0f,  1.0f,  0.0f,  0.0f, 1.0f,
         -1.0f, -1.0f,  0.0f,   0.0f, 0.0f,
    };

    // cube VAO
    unsigned int cubeVBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnable(GL_STENCIL_TEST);
    //end mirror define
}

void MirrorRender(glm::mat4 &mirror_model, glm::vec4 &Plane, Mirror &mirror_texture, Shader &mirror_shader, 
    unsigned int &cubeVAO, glm::mat4 projection, glm::mat4 view, Skybox &skybox, cOcean &ocean)
{
        //mirror
        glEnable(GL_DEPTH_TEST);
        glStencilMask(0xFF);
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glDisable(GL_STENCIL_TEST);       
        //准备镜像
        glEnable(GL_CLIP_PLANE0);
        mirror_texture.start_draw(camera);

        //正常渲染开始
        //渲染物体
        ourShader.use();
        //设置shader中projection和view参数
        auto _view = camera.GetViewMatrix();
        SetProjectionView(ourShader, projection, _view);
        //绘制模型, 参数：(shader, 绘制哪些模型)
        DrawModels(ourShader, ALLMODELS^ MIRROR_L^MIRROR_R^LANTERN);
        //添加光源效果，深度渲染无须AddLights
        AddLights(ourShader, false);
        ourShader.setVec4("Plane", Plane);

        //天空盒
        skybox.render(projection, _view);
        //海面
        ocean.render(ocean.deltaTime(), projection, _view);
        ocean.compute_flag = false;     //一个渲染循环只计算一次FFT

        mirror_texture.end_draw(camera);
        glDisable(GL_CLIP_PLANE0);

        glEnable(GL_STENCIL_TEST);
        glStencilMask(0xFF); // （允许写入）
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        //添加深度
        //画镜子
        ourShader.use();
        SetProjectionView(ourShader, projection, view);
        auto model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::translate(model, glm::vec3(positions[4][0], positions[4][1], positions[4][2]+0.01f));
        model = glm::scale(model, glm::vec3(1.9f));
        ourShader.setMat4("model", model);
        mirror.Draw(ourShader);

        //清颜色
        glClear(GL_COLOR_BUFFER_BIT  | GL_STENCIL_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);

        //镜子缩小一点，然后准备画镜像
        glStencilMask(0xFF);
        glStencilFunc(GL_NEVER, 1, 0xFF);
        glStencilOp(GL_REPLACE, GL_KEEP, GL_KEEP);
        
        
        ourShader.use();
        SetProjectionView(ourShader, projection, view);
        model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::translate(model, glm::vec3(positions[4][0], positions[4][1], positions[4][2]));
        model = glm::scale(model, glm::vec3(1.9f));
        ourShader.setMat4("model", model);
        mirror.Draw(ourShader);

        //贴图像
        glDisable(GL_DEPTH_TEST);

        glStencilMask(0x00);
        glStencilFunc(GL_EQUAL, 1, 0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mirror_texture.textureColorbuffer);
        mirror_shader.use();
        glBindVertexArray(cubeVAO);


        glDrawArrays(GL_TRIANGLES, 0, 6);
        glEnable(GL_DEPTH_TEST);
        //end mirror
}
