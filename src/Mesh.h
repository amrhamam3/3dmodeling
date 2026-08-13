#pragma once
#include <vector>
#include <glad/gl.h>
#include <glm/glm.hpp>

// كل نقطة (Vertex) تحتوي موقعها ولونها الطبيعي (Normal) - مفيد لاحقاً للإضاءة
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
};

// فئة عامة تدير VAO/VBO/EBO لأي شكل هندسي مبني من Vertices + Indices.
// البيانات ديناميكية (GL_DYNAMIC_DRAW) حتى تدعم أدوات التحرير مثل
// سحب نقطة (Raycasting) وتعديل مواقعها في وقت التشغيل.
class Mesh {
public:
    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices);
    ~Mesh();

    // ممنوع النسخ: نسخ الفئة يعني نسخ معرّفات VAO/VBO/EBO، فيحاول الكائنان
    // حذف نفس الموارد على الـ GPU مرتين عند التدمير (Double Free).
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    // النقل مسموح ومُعرَّف يدوياً: نأخذ معرّفات الموارد من الكائن المصدر
    // ونصفّرها فيه حتى لا يحذفها في هدّامه (Destructor) بعد النقل.
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    void draw() const;
    size_t indexCount() const { return m_indices.size(); }

    // وصول لتعديل النقاط (يستخدمه Raycasting / أدوات التحديد)
    std::vector<Vertex>& vertices() { return m_vertices; }
    const std::vector<Vertex>& vertices() const { return m_vertices; }
    const std::vector<unsigned int>& indices() const { return m_indices; }

    // يعيد رفع بيانات m_vertices الحالية إلى الـ GPU بعد تعديلها على الـ CPU
    void uploadVertices();

private:
    std::vector<Vertex> m_vertices;
    std::vector<unsigned int> m_indices;
    unsigned int m_VAO = 0, m_VBO = 0, m_EBO = 0;

    void setup();
    void releaseGpuResources();
};

// دالة مساعدة تُنشئ مكعباً بحجم موحّد (طول الضلع = size) حول نقطة الأصل
Mesh createCubeMesh(float size = 1.0f);

// فهرس "الزاوية الفريدة" (0-7) لكل نقطة من الـ 24 نقطة التي يبنيها createCubeMesh.
// كل زاوية من زوايا المكعب الثماني تتكرر في 3 وجوه مختلفة (3 Vertices منفصلة
// بنفس الموقع لكن بـ Normal مختلف) - هذه الدالة تربط كل نقطة بزاويتها الأصلية
// حتى تقدر أدوات التحرير تحرّك الزاوية كاملة (بكل نسخها الثلاث) دفعة واحدة.
std::vector<int> cubeCornerMapping();

// المواقع الابتدائية للزوايا الثماني الفريدة لمكعب بحجم size (بنفس ترتيب cubeCornerMapping)
std::vector<glm::vec3> cubeCornerPositions(float size);
