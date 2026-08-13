#pragma once
#include <vector>
#include <glad/gl.h>
#include <glm/glm.hpp>

// نقطة خط بسيطة: موقع + لون (بدون Normal - لا تحتاج إضاءة)
struct LineVertex {
    glm::vec3 position;
    glm::vec3 color;
};

// فئة خفيفة لرسم خطوط (GL_LINES) - تُستخدم للشبكة ومحاور الإحداثيات.
// منفصلة عن Mesh لأن تخطيط البيانات مختلف (position+color بدل position+normal)
// ونوع الرسم مختلف (GL_LINES بدل GL_TRIANGLES).
class LineMesh {
public:
    explicit LineMesh(const std::vector<LineVertex>& vertices);
    ~LineMesh();

    void draw() const;

private:
    unsigned int m_VAO = 0, m_VBO = 0;
    size_t m_count = 0;
};

// شبكة أرضية على مستوى XZ حول نقطة الأصل
LineMesh createGrid(int halfLines = 10, float spacing = 1.0f);

// محاور إحداثيات ملوّنة: X أحمر، Y أخضر، Z أزرق
LineMesh createAxisGizmo(float length = 2.0f);
