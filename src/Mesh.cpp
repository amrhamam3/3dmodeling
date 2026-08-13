#include "Mesh.h"

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices)
    : m_vertices(std::move(vertices)), m_indices(std::move(indices)) {
    setup();
}

Mesh::~Mesh() {
    releaseGpuResources();
}

void Mesh::releaseGpuResources() {
    if (m_EBO) glDeleteBuffers(1, &m_EBO);
    if (m_VBO) glDeleteBuffers(1, &m_VBO);
    if (m_VAO) glDeleteVertexArrays(1, &m_VAO);
    m_EBO = m_VBO = m_VAO = 0;
}

Mesh::Mesh(Mesh&& other) noexcept
    : m_vertices(std::move(other.m_vertices)),
      m_indices(std::move(other.m_indices)),
      m_VAO(other.m_VAO), m_VBO(other.m_VBO), m_EBO(other.m_EBO) {
    other.m_VAO = other.m_VBO = other.m_EBO = 0; // نمنع الهدّام في other من حذف نفس الموارد
}

Mesh& Mesh::operator=(Mesh&& other) noexcept {
    if (this != &other) {
        releaseGpuResources(); // حرّر موارد هذا الكائن قبل استبدالها
        m_vertices = std::move(other.m_vertices);
        m_indices = std::move(other.m_indices);
        m_VAO = other.m_VAO; m_VBO = other.m_VBO; m_EBO = other.m_EBO;
        other.m_VAO = other.m_VBO = other.m_EBO = 0;
    }
    return *this;
}

void Mesh::setup() {
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);

    glBindVertexArray(m_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    // GL_DYNAMIC_DRAW: نخبر السائق أن هذه البيانات ستتغيّر كثيراً (تحرير النقاط)
    glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(Vertex), m_vertices.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(unsigned int), m_indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

    glBindVertexArray(0);
}

void Mesh::uploadVertices() {
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    // glBufferSubData أسرع من glBufferData لأنه يعيد استخدام نفس تخصيص الذاكرة على GPU
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_vertices.size() * sizeof(Vertex), m_vertices.data());
}

void Mesh::draw() const {
    glBindVertexArray(m_VAO);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indices.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

// ---------------------------------------------------------------
// مكعب مكوَّن من 6 أوجه × 4 نقاط لكل وجه (24 Vertex) حتى يكون لكل
// وجه اتجاهه الطبيعي (Normal) الصحيح المستقل - ضروري للإضاءة والاختيار.
// ---------------------------------------------------------------
Mesh createCubeMesh(float size) {
    float h = size * 0.5f;

    std::vector<Vertex> vertices = {
        // الوجه الأمامي (+Z)
        {{-h,-h, h}, {0,0,1}}, {{ h,-h, h}, {0,0,1}}, {{ h, h, h}, {0,0,1}}, {{-h, h, h}, {0,0,1}},
        // الوجه الخلفي (-Z)
        {{ h,-h,-h}, {0,0,-1}}, {{-h,-h,-h}, {0,0,-1}}, {{-h, h,-h}, {0,0,-1}}, {{ h, h,-h}, {0,0,-1}},
        // الوجه الأيمن (+X)
        {{ h,-h, h}, {1,0,0}}, {{ h,-h,-h}, {1,0,0}}, {{ h, h,-h}, {1,0,0}}, {{ h, h, h}, {1,0,0}},
        // الوجه الأيسر (-X)
        {{-h,-h,-h}, {-1,0,0}}, {{-h,-h, h}, {-1,0,0}}, {{-h, h, h}, {-1,0,0}}, {{-h, h,-h}, {-1,0,0}},
        // الوجه العلوي (+Y)
        {{-h, h, h}, {0,1,0}}, {{ h, h, h}, {0,1,0}}, {{ h, h,-h}, {0,1,0}}, {{-h, h,-h}, {0,1,0}},
        // الوجه السفلي (-Y)
        {{-h,-h,-h}, {0,-1,0}}, {{ h,-h,-h}, {0,-1,0}}, {{ h,-h, h}, {0,-1,0}}, {{-h,-h, h}, {0,-1,0}},
    };

    std::vector<unsigned int> indices;
    for (unsigned int face = 0; face < 6; ++face) {
        unsigned int base = face * 4;
        indices.push_back(base + 0); indices.push_back(base + 1); indices.push_back(base + 2);
        indices.push_back(base + 2); indices.push_back(base + 3); indices.push_back(base + 0);
    }

    return Mesh(vertices, indices);
}

std::vector<int> cubeCornerMapping() {
    // يطابق ترتيب النقاط الـ 24 في createCubeMesh بالضبط
    return {
        0,1,2,3,   // أمامي
        4,5,6,7,   // خلفي
        1,4,7,2,   // أيمن
        5,0,3,6,   // أيسر
        3,2,7,6,   // علوي
        5,4,1,0,   // سفلي
    };
}

std::vector<glm::vec3> cubeCornerPositions(float size) {
    float h = size * 0.5f;
    return {
        {-h,-h, h}, // C0
        { h,-h, h}, // C1
        { h, h, h}, // C2
        {-h, h, h}, // C3
        { h,-h,-h}, // C4
        {-h,-h,-h}, // C5
        {-h, h,-h}, // C6
        { h, h,-h}, // C7
    };
}
