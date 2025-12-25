#include "Scene.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

Scene::Scene() 
    : bookshelf(nullptr), libraryTable(nullptr), stool(nullptr), 
      waterDispenser(nullptr), cube(nullptr), sphere(nullptr), ceilingLamp(nullptr) {
}

Scene::~Scene() {
    delete bookshelf;
    delete libraryTable;
    delete stool;
    delete waterDispenser;
    delete cube;
    delete sphere;
    delete ceilingLamp;
}

void Scene::Initialize() {
    // ========= 加载所有模型 =========
    bookshelf = new Model("models/bookshelf.obj");
    libraryTable = new Model("models/library_table.obj");
    stool = new Model("models/stool.obj");
    waterDispenser = new Model("models/water_dispenser.obj");
    cube = new Model("models/cube.obj");
    sphere = new Model("models/sphere.obj");
    ceilingLamp = new Model("models/ceiling_lamp.obj");

    // ========= 加载所有 PBR 材质 =========
    oakMat = LoadMaterial_WoodVeneerOak_7760();        // 橡木（用于书架、桌子）
    woodFloorMat = LoadMaterial_WoodFloorAsh_4186();   // 木地板（用于地板）
    metalMat = LoadMaterial_MetalGalvanizedZinc_7184(); // 金属（用于饮水机）
    paintedMetalMat = LoadMaterial_MetalPaintedMatte_7037(); // 喷漆金属
    leatherMat = LoadMaterial_FabricLeatherCowhide_001(); // 皮革（用于椅子）
    tileMat = LoadMaterial_TilesTravertine_001();      // 大理石（用于墙面装饰）

    // ========= 程序化生成盆栽（纯色PBR贴图，无需额外资源）=========
    plants.reserve(6);
    for (unsigned int i = 0; i < 6; ++i) {
        plants.push_back(CreatePottedPlant(1000u + i));
    }
}

void Scene::SetupLighting(Shader& pbrShader) {
    // ========= 设置 PBR 光照系统（6个点光源营造图书馆氛围）=========
    // 天花板位置：wallHeight + floorTopY + floorThickness * 0.5f = 5.0 + 0.05 + 0.05 = 5.1f
    // 光源位置与顶灯模型位置一致，悬挂在天花板下方
    const float wallHeight = 5.0f;
    const float floorThickness = 0.1f;
    const float floorTopY = floorThickness * 0.5f;
    const float ceilingHeight = wallHeight + floorTopY + floorThickness * 0.5f;  // 5.1f
    const float lampHeight = ceilingHeight - 0.3f;  // 4.8f
    
    pbrShader.use();
    pbrShader.setInt("lightCount", 6);
    
    // 主灯光（中央上方，暖白色）
    pbrShader.setVec3("lights[0].position", glm::vec3(0.0f, lampHeight, 0.0f));
    pbrShader.setVec3("lights[0].color", glm::vec3(1.0f, 0.95f, 0.85f));
    pbrShader.setFloat("lights[0].intensity", 50.0f);
    
    // 左侧灯光（书架区域）
    pbrShader.setVec3("lights[1].position", glm::vec3(-5.0f, lampHeight, 0.0f));
    pbrShader.setVec3("lights[1].color", glm::vec3(1.0f, 0.98f, 0.9f));
    pbrShader.setFloat("lights[1].intensity", 40.0f);
    
    // 右侧灯光（书架区域）
    pbrShader.setVec3("lights[2].position", glm::vec3(5.0f, lampHeight, 0.0f));
    pbrShader.setVec3("lights[2].color", glm::vec3(1.0f, 0.98f, 0.9f));
    pbrShader.setFloat("lights[2].intensity", 40.0f);
    
    // 前方灯光（阅读区）
    pbrShader.setVec3("lights[3].position", glm::vec3(0.0f, lampHeight, -4.0f));
    pbrShader.setVec3("lights[3].color", glm::vec3(0.95f, 0.98f, 1.0f));
    pbrShader.setFloat("lights[3].intensity", 35.0f);
    
    // 后方灯光
    pbrShader.setVec3("lights[4].position", glm::vec3(0.0f, lampHeight, 4.0f));
    pbrShader.setVec3("lights[4].color", glm::vec3(0.9f, 0.95f, 1.0f));
    pbrShader.setFloat("lights[4].intensity", 30.0f);
    
    // 角落灯光（饮水机区域，稍冷色调）
    pbrShader.setVec3("lights[5].position", glm::vec3(6.0f, lampHeight, 6.0f));
    pbrShader.setVec3("lights[5].color", glm::vec3(0.85f, 0.9f, 1.0f));
    pbrShader.setFloat("lights[5].intensity", 25.0f);
    
    // ========= 添加来自外界的方向光（自然光）=========
    // 使用远距离点光源模拟方向光，从右侧（窗外）照射进来
    // 方向：从右向左（x负方向），稍微向下倾斜
    // 位置：在窗外很远的地方，模拟太阳光
    const glm::vec3 sunDirection = glm::normalize(glm::vec3(-0.5f, -0.3f, 0.0f));  // 从右侧斜向下照射
    const glm::vec3 sunPosition = glm::vec3(50.0f, 30.0f, 0.0f);  // 远距离位置，模拟方向光
    const glm::vec3 sunColor = glm::vec3(1.0f, 0.98f, 0.95f);  // 温暖的阳光色
    const float sunIntensity = 80.0f;  // 较强的自然光强度
    
    // 注意：由于shader只支持点光源，我们使用远距离点光源来模拟方向光
    // 在实际应用中，应该修改shader添加真正的方向光支持
    // 这里为了不修改shader，使用一个非常远的点光源
    pbrShader.setInt("lightCount", 7);  // 增加光源数量
    pbrShader.setVec3("lights[6].position", sunPosition);
    pbrShader.setVec3("lights[6].color", sunColor);
    pbrShader.setFloat("lights[6].intensity", sunIntensity);

    // 绑定采样器编号（纹理单元）
    pbrShader.setInt("albedoMap",    0);
    pbrShader.setInt("normalMap",    1);
    pbrShader.setInt("metallicMap",  2);
    pbrShader.setInt("roughnessMap", 3);
    pbrShader.setInt("aoMap",        4);
}

void Scene::Render(Shader& pbrShader, const glm::mat4& view, const glm::mat4& projection, const glm::vec3& camPos) {
    // 更新 PBR shader 的矩阵和相机
    pbrShader.use();
    pbrShader.setMat4("view", view);
    pbrShader.setMat4("projection", projection);
    pbrShader.setVec3("camPos", camPos);

    // ========= 渲染地板 =========
    glm::mat4 floorMatrix = glm::mat4(1.0f);
    floorMatrix = glm::translate(floorMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
    floorMatrix = glm::scale(floorMatrix, glm::vec3(15.0f, 0.1f, 15.0f));  // 大尺寸地板
    renderModelWithPBRMaterial(pbrShader, *cube, woodFloorMat, floorMatrix);

    // ========= 渲染墙壁与天花板（围合空间）=========
    const float roomSize = 15.0f;
    const float halfRoom = roomSize * 0.5f;
    const float wallThickness = 0.2f;
    const float wallHeight = 5.0f;
    const float floorThickness = 0.1f;
    const float floorTopY = floorThickness * 0.5f;

    // ========= 渲染盆栽（放在地面上）=========
    const glm::vec3 plantPositions[6] = {
        glm::vec3(-6.0f, floorTopY, -5.5f),
        glm::vec3(-6.0f, floorTopY,  0.0f),
        glm::vec3(-6.0f, floorTopY,  5.5f),
        glm::vec3( 6.0f, floorTopY, -5.5f),
        glm::vec3( 6.0f, floorTopY,  0.0f),
        glm::vec3( 4.2f, floorTopY,  4.2f)  // 避开饮水机(5.5,5.5)
    };
    const float plantRotY[6] = { 25.0f, -10.0f, 55.0f, -35.0f, 15.0f, -60.0f };

    for (int i = 0; i < 6; ++i) {
        glm::mat4 plantM = glm::mat4(1.0f);
        plantM = glm::translate(plantM, plantPositions[i]);
        plantM = glm::rotate(plantM, glm::radians(plantRotY[i]), glm::vec3(0.0f, 1.0f, 0.0f));

        renderMeshWithPBRMaterial(pbrShader, *plants[i].pot, plants[i].potMat, plantM);
        renderMeshWithPBRMaterial(pbrShader, *plants[i].soil, plants[i].soilMat, plantM);
        renderMeshWithPBRMaterial(pbrShader, *plants[i].leaves, plants[i].leavesMat, plantM);
    }

    // 左墙（x 负方向）
    glm::mat4 leftWallMatrix = glm::mat4(1.0f);
    leftWallMatrix = glm::translate(leftWallMatrix, glm::vec3(-(halfRoom + wallThickness * 0.5f), wallHeight * 0.5f + floorTopY, 0.0f));
    leftWallMatrix = glm::scale(leftWallMatrix, glm::vec3(wallThickness, wallHeight, roomSize));
    renderModelWithPBRMaterial(pbrShader, *cube, tileMat, leftWallMatrix, true);

    // 落地窗（x 正方向，替代右墙）
    // 使用金属材质模拟窗框，创建一个大的落地窗结构
    const float windowFrameThickness = 0.05f;
    const float windowHeight = wallHeight;
    
    // 窗框 - 顶部横梁
    glm::mat4 windowTopFrame = glm::mat4(1.0f);
    windowTopFrame = glm::translate(windowTopFrame, glm::vec3((halfRoom + windowFrameThickness * 0.5f), wallHeight * 0.5f + floorTopY, 0.0f));
    windowTopFrame = glm::scale(windowTopFrame, glm::vec3(windowFrameThickness, windowFrameThickness * 0.3f, roomSize));
    renderModelWithPBRMaterial(pbrShader, *cube, metalMat, windowTopFrame);
    
    // 窗框 - 底部横梁
    glm::mat4 windowBottomFrame = glm::mat4(1.0f);
    windowBottomFrame = glm::translate(windowBottomFrame, glm::vec3((halfRoom + windowFrameThickness * 0.5f), floorTopY + windowFrameThickness * 0.15f, 0.0f));
    windowBottomFrame = glm::scale(windowBottomFrame, glm::vec3(windowFrameThickness, windowFrameThickness * 0.3f, roomSize));
    renderModelWithPBRMaterial(pbrShader, *cube, metalMat, windowBottomFrame);
    
    // 窗框 - 左侧竖框
    glm::mat4 windowLeftFrame = glm::mat4(1.0f);
    windowLeftFrame = glm::translate(windowLeftFrame, glm::vec3((halfRoom + windowFrameThickness * 0.5f), wallHeight * 0.5f + floorTopY, -(halfRoom - windowFrameThickness * 0.5f)));
    windowLeftFrame = glm::scale(windowLeftFrame, glm::vec3(windowFrameThickness, windowHeight, windowFrameThickness));
    renderModelWithPBRMaterial(pbrShader, *cube, metalMat, windowLeftFrame);
    
    // 窗框 - 右侧竖框
    glm::mat4 windowRightFrame = glm::mat4(1.0f);
    windowRightFrame = glm::translate(windowRightFrame, glm::vec3((halfRoom + windowFrameThickness * 0.5f), wallHeight * 0.5f + floorTopY, (halfRoom - windowFrameThickness * 0.5f)));
    windowRightFrame = glm::scale(windowRightFrame, glm::vec3(windowFrameThickness, windowHeight, windowFrameThickness));
    renderModelWithPBRMaterial(pbrShader, *cube, metalMat, windowRightFrame);
    
    // 窗框 - 中间竖框（分割成两扇窗）
    glm::mat4 windowMiddleFrame = glm::mat4(1.0f);
    windowMiddleFrame = glm::translate(windowMiddleFrame, glm::vec3((halfRoom + windowFrameThickness * 0.5f), wallHeight * 0.5f + floorTopY, 0.0f));
    windowMiddleFrame = glm::scale(windowMiddleFrame, glm::vec3(windowFrameThickness, windowHeight, windowFrameThickness));
    renderModelWithPBRMaterial(pbrShader, *cube, metalMat, windowMiddleFrame);

    // 后墙（z 正方向）
    glm::mat4 backWallMatrix = glm::mat4(1.0f);
    backWallMatrix = glm::translate(backWallMatrix, glm::vec3(0.0f, wallHeight * 0.5f + floorTopY, (halfRoom + wallThickness * 0.5f)));
    backWallMatrix = glm::scale(backWallMatrix, glm::vec3(roomSize, wallHeight, wallThickness));
    renderModelWithPBRMaterial(pbrShader, *cube, tileMat, backWallMatrix, true);

    // 前墙（z 负方向）
    glm::mat4 frontWallMatrix = glm::mat4(1.0f);
    frontWallMatrix = glm::translate(frontWallMatrix, glm::vec3(0.0f, wallHeight * 0.5f + floorTopY, -(halfRoom + wallThickness * 0.5f)));
    frontWallMatrix = glm::scale(frontWallMatrix, glm::vec3(roomSize, wallHeight, wallThickness));
    renderModelWithPBRMaterial(pbrShader, *cube, tileMat, frontWallMatrix, true);

    // 天花板
    glm::mat4 ceilingMatrix = glm::mat4(1.0f);
    ceilingMatrix = glm::translate(ceilingMatrix, glm::vec3(0.0f, wallHeight + floorTopY + floorThickness * 0.5f, 0.0f));
    ceilingMatrix = glm::scale(ceilingMatrix, glm::vec3(roomSize, floorThickness, roomSize));
    renderModelWithPBRMaterial(pbrShader, *cube, tileMat, ceilingMatrix, true);

    // ========= 渲染顶灯（在每个光源位置）=========
    // 天花板位置：wallHeight + floorTopY + floorThickness * 0.5f = 5.0 + 0.05 + 0.05 = 5.1f
    // 顶灯悬挂在天花板下方，y坐标设为4.8f
    const float ceilingHeight = wallHeight + floorTopY + floorThickness * 0.5f;  // 5.1f
    const float lampHeight = ceilingHeight - 0.3f;  // 4.8f，让顶灯悬挂在天花板下方
    
    const glm::vec3 lightPositions[6] = {
        glm::vec3(0.0f, lampHeight, 0.0f),      // 主灯光
        glm::vec3(-5.0f, lampHeight, 0.0f),    // 左侧灯光
        glm::vec3(5.0f, lampHeight, 0.0f),     // 右侧灯光
        glm::vec3(0.0f, lampHeight, -4.0f),    // 前方灯光
        glm::vec3(0.0f, lampHeight, 4.0f),     // 后方灯光
        glm::vec3(6.0f, lampHeight, 6.0f)      // 角落灯光
    };
    
    for (int i = 0; i < 6; ++i) {
        glm::mat4 lampMatrix = glm::mat4(1.0f);
        lampMatrix = glm::translate(lampMatrix, lightPositions[i]);
        lampMatrix = glm::scale(lampMatrix, glm::vec3(0.3f));
        renderModelWithPBRMaterial(pbrShader, *ceilingLamp, metalMat, lampMatrix);
    }

    // ========= 渲染桌子，每排3张桌子以短边相连 =========

    const float tableShortEdge = 1.5f;  // 短边（x方向）
    const float tableLongEdge = 2.5f;   // 长边（z方向）
    const float tableSpacing = 0.1f;    // 桌子之间的间距（控制排内桌子间的距离）
    const int numRows = 3;              // 3排桌子
    const int tablesPerRow = 3;         // 每排3张桌子
    const float rowSpacing = 4.0f;      // 排之间的间距（只影响 x 方向，不影响排内桌子间距离）
    const float startX = -5.0f;         // 起始x位置
    const float startZ = -5.0f;        // 起始z位置
    
    for (int row = 0; row < numRows; ++row) {
        float rowX = startX + row * rowSpacing;
        
        // 渲染每排的3张桌子（沿z方向，短边相连）
        for (int table = 0; table < tablesPerRow; ++table) {
            float tableZ = startZ + table * (tableLongEdge + tableSpacing) + tableLongEdge * 0.5f;
            
            glm::mat4 tableMatrix = glm::mat4(1.0f);
            tableMatrix = glm::translate(tableMatrix, glm::vec3(rowX, 0.0f, tableZ));
            tableMatrix = glm::scale(tableMatrix, glm::vec3(1.2f));
            renderModelWithPBRMaterial(pbrShader, *libraryTable, oakMat, tableMatrix);
        }
    }

    // ========= 渲染椅子（独立定义位置，不依赖于桌子）=========
    // 椅子位置定义：每排桌子长边两侧各放置椅子
    // 第1排（row=0, x=-5.0）：左侧椅子
    const glm::vec3 chairPositions[] = {
        // 第1排左侧椅子（x=-5.0，面向x正方向）
        glm::vec3(-6.2f, 0.0f, -4.5f), glm::vec3(-6.2f, 0.0f, -2.0f), glm::vec3(-6.2f, 0.0f, 0.5f), glm::vec3(-6.2f, 0.0f, 3.0f),
        // 第1排右侧椅子（x=-5.0，面向x负方向）
        glm::vec3(-3.8f, 0.0f, -4.5f), glm::vec3(-3.8f, 0.0f, -2.0f), glm::vec3(-3.8f, 0.0f, 0.5f), glm::vec3(-3.8f, 0.0f, 3.0f),
        // 第2排左侧椅子（x=-1.0，面向x正方向）
        glm::vec3(-2.2f, 0.0f, -4.5f), glm::vec3(-2.2f, 0.0f, -2.0f), glm::vec3(-2.2f, 0.0f, 0.5f), glm::vec3(-2.2f, 0.0f, 3.0f),
        // 第2排右侧椅子（x=-1.0，面向x负方向）
        glm::vec3(0.2f, 0.0f, -4.5f), glm::vec3(0.2f, 0.0f, -2.0f), glm::vec3(0.2f, 0.0f, 0.5f), glm::vec3(0.2f, 0.0f, 3.0f),
        // 第3排左侧椅子（x=3.0，面向x正方向）
        glm::vec3(1.8f, 0.0f, -4.5f), glm::vec3(1.8f, 0.0f, -2.0f), glm::vec3(1.8f, 0.0f, 0.5f), glm::vec3(1.8f, 0.0f, 3.0f),
        // 第3排右侧椅子（x=3.0，面向x负方向）
        glm::vec3(4.2f, 0.0f, -4.5f), glm::vec3(4.2f, 0.0f, -2.0f), glm::vec3(4.2f, 0.0f, 0.5f), glm::vec3(4.2f, 0.0f, 3.0f)
    };
    
    const float chairRotations[] = {
        // 所有椅子都面朝-z方向（旋转180度）
        180.0f, 180.0f, 180.0f, 180.0f,  // 第1排左侧椅子
        180.0f, 180.0f, 180.0f, 180.0f,  // 第1排右侧椅子
        180.0f, 180.0f, 180.0f, 180.0f,  // 第2排左侧椅子
        180.0f, 180.0f, 180.0f, 180.0f,  // 第2排右侧椅子
        180.0f, 180.0f, 180.0f, 180.0f,  // 第3排左侧椅子
        180.0f, 180.0f, 180.0f, 180.0f   // 第3排右侧椅子
    };
    
    const int totalChairs = sizeof(chairPositions) / sizeof(chairPositions[0]);
    for (int i = 0; i < totalChairs; ++i) {
        glm::mat4 stoolMatrix = glm::mat4(1.0f);
        stoolMatrix = glm::translate(stoolMatrix, chairPositions[i]);
        stoolMatrix = glm::rotate(stoolMatrix, glm::radians(chairRotations[i]), glm::vec3(0.0f, 1.0f, 0.0f));
        stoolMatrix = glm::scale(stoolMatrix, glm::vec3(1.0f));
        renderModelWithPBRMaterial(pbrShader, *stool, leatherMat, stoolMatrix, true);
    }

    // ========= 渲染6排书架，每排2个书架背靠背，放在每排桌子的一端 =========
    // 书架垂直于墙面（沿x方向），放在桌子的一端（z方向的一端）
    const float bookshelfDepth = 1.0f;  // 书架深度
    const float bookshelfSpacing = 0.1f; // 两个书架之间的间距
    
    for (int row = 0; row < numRows; ++row) {
        float rowX = startX + row * rowSpacing;
        float bookshelfZ = startZ - 1.5f;  // 放在桌子的一端（z负方向）
        
        // 第一个书架（面向z正方向）
        glm::mat4 bookshelf1Matrix = glm::mat4(1.0f);
        bookshelf1Matrix = glm::translate(bookshelf1Matrix, glm::vec3(rowX + 0.2f, 0.0f, bookshelfZ + 0.09f));
        bookshelf1Matrix = glm::rotate(bookshelf1Matrix, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.f));
        bookshelf1Matrix = glm::scale(bookshelf1Matrix, glm::vec3(1.15f));
        renderModelWithPBRMaterial(pbrShader, *bookshelf, oakMat, bookshelf1Matrix);
        
        // 第二个书架（背靠背，面向z负方向）
        glm::mat4 bookshelf2Matrix = glm::mat4(1.0f);
        bookshelf2Matrix = glm::translate(bookshelf2Matrix, glm::vec3(rowX - 0.22f, 0.0f, bookshelfZ - bookshelfDepth - bookshelfSpacing + 1.2f));
        bookshelf2Matrix = glm::rotate(bookshelf2Matrix, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        bookshelf2Matrix = glm::scale(bookshelf2Matrix, glm::vec3(1.15f));
        renderModelWithPBRMaterial(pbrShader, *bookshelf, oakMat, bookshelf2Matrix);
    }

    // ========= 渲染饮水机（角落）=========
    glm::mat4 dispenserMatrix = glm::mat4(1.0f);
    dispenserMatrix = glm::translate(dispenserMatrix, glm::vec3(5.5f, 0.0f, 5.5f));
    dispenserMatrix = glm::rotate(dispenserMatrix, glm::radians(-45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    dispenserMatrix = glm::scale(dispenserMatrix, glm::vec3(1.0f));
    renderModelWithPBRMaterial(pbrShader, *waterDispenser, metalMat, dispenserMatrix);
}

void Scene::renderModelWithPBRMaterial(Shader& pbrShader, Model& model, PBRTextureMaterial& mat, 
                                       glm::mat4& modelMatrix, bool useGloss) {
    pbrShader.use();
    pbrShader.setMat4("model", modelMatrix);
    
    // 设置是否使用GLOSS贴图（需要反转roughness）
    pbrShader.setBool("useGlossMap", useGloss);
    
    // 绑定材质纹理
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mat.albedoTex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mat.normalTex);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, mat.metallicTex);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, mat.roughnessTex);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, mat.aoTex);
    
    // 材质参数（真实值来自贴图，这些是乘子或默认值）
    pbrShader.setVec3("material.albedo", glm::vec3(1.0f));
    pbrShader.setFloat("material.metallic", 0.0f);
    pbrShader.setFloat("material.roughness", 0.6f);
    pbrShader.setFloat("material.ao", 1.0f);
    
    model.Draw(pbrShader);
}

void Scene::renderMeshWithPBRMaterial(Shader& pbrShader, Mesh& mesh, PBRTextureMaterial& mat,
                                      glm::mat4& modelMatrix, bool useGloss) {
    pbrShader.use();
    pbrShader.setMat4("model", modelMatrix);
    pbrShader.setBool("useGlossMap", useGloss);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mat.albedoTex);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mat.normalTex);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, mat.metallicTex);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, mat.roughnessTex);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, mat.aoTex);

    pbrShader.setVec3("material.albedo", glm::vec3(1.0f));
    pbrShader.setFloat("material.metallic", 0.0f);
    pbrShader.setFloat("material.roughness", 0.6f);
    pbrShader.setFloat("material.ao", 1.0f);

    mesh.Draw(pbrShader);
}

