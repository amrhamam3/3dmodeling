#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <fstream>
#include "Shader.h"
#include "Mesh.h"
#include "ArcballCamera.h"
#include "RayUtils.h"
#include "Grid.h"
#include "ObjIO.h"

// ---------------------------------------------------------------
// إعدادات النافذة
// ---------------------------------------------------------------
static int windowWidth  = 1280;
static int windowHeight = 720;

static ArcballCamera camera;

static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    windowWidth = width;
    windowHeight = height;
    glViewport(0, 0, width, height);
}

// عجلة الفأرة = تقريب/تبعيد (الحدث الوحيد الذي يحتاج GLFW callback فعلياً؛
// زر الفأرة وموضعها نتابعهما بالاستطلاع Polling داخل حلقة الرسم لأننا
// نحتاج مصفوفات view/projection المحدَّثة لنفس الإطار عند حساب الشعاع)
static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (ImGui::GetIO().WantCaptureMouse) return;
    camera.zoom((float)yoffset);
}

// يحدّث بيانات الشبكة (Mesh) على الـ GPU بعد تغيير موقع إحدى الزوايا الثماني
static void applyCornersToMesh(Mesh& mesh, const std::vector<glm::vec3>& corners, const std::vector<int>& cornerMap) {
    auto& verts = mesh.vertices();
    for (size_t i = 0; i < verts.size(); ++i) {
        verts[i].position = corners[cornerMap[i]];
    }
    mesh.uploadVertices();
}

int main() {
    // -------------------- 1) تهيئة GLFW --------------------
    if (!glfwInit()) {
        std::cerr << "فشل تهيئة GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "CubeStudio - 3D Modeling App", nullptr, nullptr);
    if (!window) {
        std::cerr << "فشل إنشاء النافذة" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // V-Sync
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // -------------------- 2) تحميل دوال OpenGL عبر GLAD 2 --------------------
    // GLAD 2 يعيد رقم إصدار OpenGL المُحمَّل (0 يعني فشل) بدل true/false كما في GLAD 1
    int glVersion = gladLoadGL((GLADloadfunc)glfwGetProcAddress);
    if (!glVersion) {
        std::cerr << "فشل تهيئة GLAD" << std::endl;
        return -1;
    }
    glEnable(GL_DEPTH_TEST);

    // -------------------- 3) تهيئة Dear ImGui --------------------
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    // -------------------- تحميل خط يدعم رموز العربية --------------------
    // ملاحظة تقنية مهمة: Dear ImGui لا يملك محرك تشكيل نصوص (لا HarfBuzz ولا
    // FriBidi)، فحتى مع خط يحتوي رموز عربية، الحروف ستُرسم منفصلة عن بعضها
    // (بدون اتصال الحروف الطبيعي) ومن اليسار لليمين (LTR) وليس RTL كاملاً.
    // هذا قيد معروف في المكتبة نفسها وليس خطأً في هذا الكود - الحل الكامل
    // يحتاج معالجة النص بمكتبة تشكيل (HarfBuzz) وترتيب اتجاه (FriBidi) قبل
    // تمريره لـ ImGui::Text، وهو عمل إضافي منفصل لو احتجناه لاحقاً.
    const char* arabicFontPath = "fonts/NotoSansArabic-Regular.ttf";
    static const ImWchar arabicRanges[] = {
        0x0020, 0x00FF, // Basic Latin + Latin-1 (أرقام/نصوص إنجليزية في الواجهة)
        0x0600, 0x06FF, // Arabic
        0x0750, 0x077F, // Arabic Supplement
        0xFB50, 0xFDFF, // Arabic Presentation Forms-A
        0xFE70, 0xFEFF, // Arabic Presentation Forms-B
        0,
    };
    std::ifstream fontCheck(arabicFontPath);
    if (fontCheck.good()) {
        fontCheck.close();
        ImFontConfig fontConfig;
        fontConfig.OversampleH = 2;
        fontConfig.OversampleV = 2;
        io.Fonts->AddFontFromFileTTF(arabicFontPath, 20.0f, &fontConfig, arabicRanges);
    } else {
        std::cerr << "[تنبيه] لم يتم العثور على " << arabicFontPath
                  << " - سيُستخدم الخط الافتراضي وستظهر الحروف العربية كعلامات استفهام.\n"
                  << "        ضع خطاً يدعم العربية (مثل Noto Sans Arabic) داخل مجلد fonts/ لحل المشكلة.\n";
        io.Fonts->AddFontDefault();
    }

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // -------------------- 4) تحميل الـ Shader --------------------
    Shader basicShader("shaders/basic.vert", "shaders/basic.frag");
    Shader lineShader("shaders/line.vert", "shaders/line.frag");

    // شبكة أرضية + محاور إحداثيات (X أحمر، Y أخضر، Z أزرق) - مرجع بصري ثابت
    LineMesh grid = createGrid(10, 1.0f);
    LineMesh axes = createAxisGizmo(2.0f);
    bool showGrid = true;
    bool showAxes = true;

    // مكعب حقيقي مبني من Vertices + Indices
    Mesh cube = createCubeMesh(1.0f);

    // نسخة الزوايا الثماني الفريدة على الـ CPU (تُستخدم في الاختيار والسحب)،
    // ومصفوفة الربط بين كل نقطة من الـ 24 نقطة وزاويتها الأصلية
    std::vector<glm::vec3> corners = cubeCornerPositions(1.0f);
    const std::vector<glm::vec3> originalCorners = corners; // نسخة احتياطية لزر "إعادة الضبط"
    const std::vector<int> cornerMap = cubeCornerMapping();

    glm::vec3 shapeColor = glm::vec3(0.20f, 0.55f, 0.95f);
    bool autoRotate = true;
    float rotationSpeed = 0.6f;
    float currentAngle = 0.0f;
    float lastFrameTime = (float)glfwGetTime();

    // -------------------- حالة أداة التحديد/التحرير --------------------
    bool editMode = false;
    int selectedCorner = -1;         // -1 = لا يوجد تحديد حالياً
    const float pickRadiusPixels = 18.0f;
    double lastMouseX = 0.0, lastMouseY = 0.0;
    bool wasLeftDown = false;
    bool isOrbiting = false;
    std::vector<glm::vec3> dragStartCorners; // نسخة من الزوايا لحظة بداية السحب (للمقارنة عند التسجيل بالتاريخ)

    // -------------------- سجل التراجع/الإعادة (Undo/Redo) --------------------
    const size_t maxHistorySize = 50;
    std::vector<std::vector<glm::vec3>> history = { corners }; // الحالة الابتدائية = الإدخال الأول
    size_t historyIndex = 0;

    // يسجّل حالة جديدة في السجل (يحذف أي "إعادة" قديمة إن كنا رجعنا للخلف ثم عدّلنا)
    auto pushHistory = [&]() {
        history.resize(historyIndex + 1);           // احذف أي تفريعة Redo قديمة
        history.push_back(corners);
        if (history.size() > maxHistorySize) {
            history.erase(history.begin());          // حافظ على حجم محدود للسجل
        }
        historyIndex = history.size() - 1;
    };

    auto applyHistoryState = [&]() {
        corners = history[historyIndex];
        applyCornersToMesh(cube, corners, cornerMap);
        selectedCorner = -1;
    };

    auto undo = [&]() {
        if (historyIndex > 0) { historyIndex--; applyHistoryState(); }
    };
    auto redo = [&]() {
        if (historyIndex + 1 < history.size()) { historyIndex++; applyHistoryState(); }
    };

    // -------------------- حفظ / تحميل (OBJ) --------------------
    bool isEditableCube = true; // false بعد استيراد شكل خارجي (تعطّل أدوات تحرير الزوايا)
    char exportPathBuf[256] = "model.obj";
    char importPathBuf[256] = "model.obj";
    std::string ioStatusMessage;
    bool ioStatusIsError = false;

    // -------------------- 5) حلقة الرسم (Render Loop) --------------------
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        float currentTime = (float)glfwGetTime();
        float deltaTime = currentTime - lastFrameTime;
        lastFrameTime = currentTime;
        if (autoRotate && !editMode) currentAngle += deltaTime * rotationSpeed;

        // --- بداية إطار ImGui ---
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // اختصارات لوحة المفاتيح: Ctrl+Z = تراجع، Ctrl+Y أو Ctrl+Shift+Z = إعادة
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z) && !io.KeyShift) undo();
        if (io.KeyCtrl && (ImGui::IsKeyPressed(ImGuiKey_Y) || (io.KeyShift && ImGui::IsKeyPressed(ImGuiKey_Z)))) redo();

        ImGui::Begin("لوحة التحكم - CubeStudio");
        ImGui::Text("مرحباً بك في محرك النمذجة ثلاثية الأبعاد");
        ImGui::ColorEdit3("لون الشكل", &shapeColor.x);
        if (ImGui::Button("إعادة تعيين اللون")) {
            shapeColor = glm::vec3(0.20f, 0.55f, 0.95f);
        }
        ImGui::Separator();
        ImGui::Checkbox("تدوير تلقائي", &autoRotate);
        ImGui::SliderFloat("سرعة الدوران", &rotationSpeed, 0.0f, 3.0f);
        ImGui::Separator();
        ImGui::Text("الكاميرا (Arcball)");
        ImGui::Text(editMode ? "الدوران معطّل أثناء التحرير" : "سحب باليسار = دوران | العجلة = تقريب");
        ImGui::SliderFloat("المسافة", &camera.distance, camera.minDistance, camera.maxDistance);
        ImGui::Separator();
        ImGui::Text("أداة التحديد (Raycasting)");
        if (!isEditableCube) ImGui::BeginDisabled();
        ImGui::Checkbox("وضع التحرير (Edit Mode)", &editMode);
        if (!isEditableCube) ImGui::EndDisabled();
        if (editMode && isEditableCube) {
            ImGui::TextWrapped("انقر بالقرب من إحدى زوايا المكعب لتحديدها، ثم اسحب لتحريكها.");
            if (selectedCorner != -1) {
                ImGui::Text("الزاوية المحددة: #%d", selectedCorner);
            } else {
                ImGui::TextDisabled("لا يوجد تحديد حالياً");
            }
        }
        if (ImGui::Button("إعادة ضبط المكعب")) {
            corners = originalCorners;
            applyCornersToMesh(cube, corners, cornerMap);
            selectedCorner = -1;
            pushHistory();
        }
        ImGui::Separator();
        ImGui::Text("التراجع / الإعادة");
        bool canUndo = historyIndex > 0;
        bool canRedo = historyIndex + 1 < history.size();
        if (!canUndo) ImGui::BeginDisabled();
        if (ImGui::Button("تراجع (Ctrl+Z)")) undo();
        if (!canUndo) ImGui::EndDisabled();
        ImGui::SameLine();
        if (!canRedo) ImGui::BeginDisabled();
        if (ImGui::Button("إعادة (Ctrl+Y)")) redo();
        if (!canRedo) ImGui::EndDisabled();
        ImGui::Text("الخطوة %zu من %zu", historyIndex + 1, history.size());
        ImGui::Separator();
        ImGui::Text("المشهد");
        ImGui::Checkbox("إظهار الشبكة", &showGrid);
        ImGui::Checkbox("إظهار المحاور", &showAxes);
        ImGui::Separator();
        ImGui::Text("حفظ / تحميل (OBJ)");
        ImGui::InputText("مسار الحفظ", exportPathBuf, IM_ARRAYSIZE(exportPathBuf));
        if (ImGui::Button("تصدير OBJ")) {
            bool ok = exportOBJ(exportPathBuf, cube);
            ioStatusMessage = ok ? "تم الحفظ بنجاح" : "فشل الحفظ - تحقق من المسار";
            ioStatusIsError = !ok;
        }
        ImGui::InputText("مسار التحميل", importPathBuf, IM_ARRAYSIZE(importPathBuf));
        if (ImGui::Button("استيراد OBJ")) {
            ObjImportResult result = importOBJ(importPathBuf);
            if (result.success) {
                cube = Mesh(result.vertices, result.indices); // نقل (Move) - آمن بفضل تعديل Mesh
                isEditableCube = false;
                editMode = false;
                selectedCorner = -1;
                ioStatusMessage = "تم الاستيراد بنجاح (شكل غير قابل لتحرير الزوايا)";
                ioStatusIsError = false;
            } else {
                ioStatusMessage = result.errorMessage;
                ioStatusIsError = true;
            }
        }
        if (!ioStatusMessage.empty()) {
            ImGui::TextColored(ioStatusIsError ? ImVec4(0.95f,0.4f,0.4f,1) : ImVec4(0.4f,0.9f,0.5f,1),
                                "%s", ioStatusMessage.c_str());
        }
        if (!isEditableCube) {
            ImGui::TextDisabled("الشكل الحالي مستورد - أدوات تحرير الزوايا معطّلة");
            if (ImGui::Button("إرجاع مكعب قابل للتحرير")) {
                cube = createCubeMesh(1.0f);
                corners = originalCorners;
                history = { corners };
                historyIndex = 0;
                isEditableCube = true;
                selectedCorner = -1;
            }
        }
        ImGui::End();

        // -------------------- منطق الكاميرا والتحديد/السحب --------------------
        bool blockedByUI = io.WantCaptureMouse;
        double mouseX, mouseY;
        glfwGetCursorPos(window, &mouseX, &mouseY);
        bool leftDown = !blockedByUI && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

        // مصفوفات هذا الإطار (نحتاجها هنا لأن الالتقاط يتم في فضاء الشاشة)
        glm::mat4 model = glm::mat4(1.0f);
        if (!editMode) {
            model = glm::rotate(model, currentAngle, glm::vec3(0.3f, 1.0f, 0.0f));
        }
        // في وضع التحرير نُبقي model = Identity حتى يتطابق فضاء العالم مع
        // فضاء المكعب مباشرة، فيسهل حساب التقاطعات دون قلب الدوران باستمرار.
        glm::mat4 view = camera.viewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(45.0f),
            (float)windowWidth / (float)windowHeight, 0.1f, 100.0f);

        if (!editMode || !isEditableCube) {
            // وضع المشاهدة العادي (أو شكل مستورد غير قابل لتحرير الزوايا): السحب باليسار يدوّر الكاميرا
            if (leftDown) {
                if (isOrbiting) {
                    camera.orbit((float)(mouseX - lastMouseX), (float)(mouseY - lastMouseY));
                }
                isOrbiting = true;
            } else {
                isOrbiting = false;
            }
            selectedCorner = -1;
        } else {
            // وضع التحرير: نقرة جديدة تبحث عن أقرب زاوية على الشاشة
            glm::mat4 mvp = projection * view * model;

            if (leftDown && !wasLeftDown) {
                float bestDist = pickRadiusPixels;
                int bestIdx = -1;
                for (int i = 0; i < (int)corners.size(); ++i) {
                    glm::vec2 screenPos = worldToScreen(corners[i], mvp, windowWidth, windowHeight);
                    float d = glm::length(screenPos - glm::vec2((float)mouseX, (float)mouseY));
                    if (d < bestDist) { bestDist = d; bestIdx = i; }
                }
                selectedCorner = bestIdx;
                dragStartCorners = corners; // لقطة قبل بدء السحب - نقارن بها عند الإفلات
            }

            if (leftDown && selectedCorner != -1) {
                // سحب الزاوية المحددة على مستوى مواجه للكاميرا يمر بموقعها الحالي
                Ray ray = screenPointToRay(mouseX, mouseY, windowWidth, windowHeight, view, projection);
                glm::vec3 camForward = glm::normalize(camera.target - camera.position());
                glm::vec3 hit;
                if (rayPlaneIntersect(ray, corners[selectedCorner], camForward, hit)) {
                    corners[selectedCorner] = hit;
                    applyCornersToMesh(cube, corners, cornerMap);
                }
            }

            if (!leftDown && wasLeftDown && !dragStartCorners.empty() && corners != dragStartCorners) {
                // انتهى السحب وفعلاً تغيّر شكل المكعب -> سجّل حالة جديدة في التاريخ
                pushHistory();
            }
            if (!leftDown) {
                selectedCorner = -1;
            }
        }

        wasLeftDown = leftDown;
        lastMouseX = mouseX;
        lastMouseY = mouseY;

        // --- الرسم ---
        glClearColor(0.10f, 0.11f, 0.13f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // الشبكة والمحاور ثابتة في فضاء العالم (بدون model) وبلا إضاءة
        glm::mat4 vp = projection * view;
        lineShader.use();
        lineShader.setMat4("mvp", vp);
        if (showGrid) grid.draw();
        if (showAxes) axes.draw();

        basicShader.use();
        basicShader.setMat4("model", model);
        basicShader.setMat4("view", view);
        basicShader.setMat4("projection", projection);
        basicShader.setVec3("uColor", shapeColor);

        cube.draw();

        // --- رسم واجهة ImGui فوق المشهد ---
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // -------------------- 6) تنظيف الموارد --------------------
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
