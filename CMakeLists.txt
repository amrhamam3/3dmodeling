cmake_minimum_required(VERSION 3.16)
project(CubeStudio CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ------------------------------------------------------------------
# جلب المكتبات تلقائياً عبر FetchContent (لا حاجة لتثبيت يدوي)
# ------------------------------------------------------------------
include(FetchContent)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.4
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
# نعطّل دعم Wayland: يحتاج أداة wayland-scanner غير مثبتة افتراضياً على
# runners لينكس بجيت هب (وغير ضرورية لهذا المشروع - X11 كافٍ للنوافذ العادية)
set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.90.9
)
FetchContent_MakeAvailable(imgui)

# ------------------------------------------------------------------
# GLAD 2 - نجلبه عبر FetchContent ونستخدم تكامل CMake الرسمي المدمج
# بالمستودع (glad_add_library) بدل تشغيل سكربت التوليد يدوياً.
# هذا أوثق بكثير من "pip install glad2 && python -m glad ..." اللي
# كان يفشل أحياناً بسبب اختلاف صيغة الأوامر بين البيئات.
# مرجع الطريقة الرسمية: cmake/ داخل مستودع Dav1dde/glad
# ------------------------------------------------------------------
FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG        v2.0.8
)
FetchContent_MakeAvailable(glad)
add_subdirectory("${glad_SOURCE_DIR}/cmake" "${CMAKE_BINARY_DIR}/glad_cmake")
glad_add_library(glad_gl_core_33 REPRODUCIBLE LOADER API gl:core=3.3)

# ------------------------------------------------------------------
# مصدر Dear ImGui + backends الخاصة بـ GLFW/OpenGL3
# ------------------------------------------------------------------
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
target_link_libraries(imgui PUBLIC glfw)
# ملاحظة: باكند imgui_impl_opengl3 يستخدم مُحمِّل OpenGL داخلي خاص به
# (مضمّن تلقائياً) - لا يحتاج GLAD الخاص بنا، لذلك ما نربطه هنا.

# ------------------------------------------------------------------
# التطبيق الرئيسي
# ------------------------------------------------------------------
add_executable(CubeStudio
    src/main.cpp
    src/Shader.cpp
    src/Mesh.cpp
    src/Grid.cpp
    src/ObjIO.cpp
)

target_include_directories(CubeStudio PRIVATE src)
target_link_libraries(CubeStudio PRIVATE glfw glad_gl_core_33 glm imgui)

# نسخ مجلد الـ shaders والخطوط بجانب الملف التنفيذي
add_custom_command(TARGET CubeStudio POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:CubeStudio>/shaders
)
add_custom_command(TARGET CubeStudio POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
    $<TARGET_FILE_DIR:CubeStudio>/fonts
)
if(EXISTS ${CMAKE_SOURCE_DIR}/fonts)
    add_custom_command(TARGET CubeStudio POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/fonts $<TARGET_FILE_DIR:CubeStudio>/fonts
    )
endif()
