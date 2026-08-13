#include "Grid.h"

LineMesh::LineMesh(const std::vector<LineVertex>& vertices) {
    m_count = vertices.size();

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(LineVertex), vertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)offsetof(LineVertex, position));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(LineVertex), (void*)offsetof(LineVertex, color));

    glBindVertexArray(0);
}

LineMesh::~LineMesh() {
    if (m_VBO) glDeleteBuffers(1, &m_VBO);
    if (m_VAO) glDeleteVertexArrays(1, &m_VAO);
}

void LineMesh::draw() const {
    glBindVertexArray(m_VAO);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(m_count));
    glBindVertexArray(0);
}

LineMesh createGrid(int halfLines, float spacing) {
    std::vector<LineVertex> vertices;
    glm::vec3 gridColor(0.30f, 0.32f, 0.36f);
    glm::vec3 centerLineColor(0.45f, 0.47f, 0.52f); // خط المنتصف أوضح قليلاً

    float extent = halfLines * spacing;
    for (int i = -halfLines; i <= halfLines; ++i) {
        float pos = i * spacing;
        glm::vec3 c = (i == 0) ? centerLineColor : gridColor;

        // خطوط موازية لمحور X (تتغيّر على Z)
        vertices.push_back({{-extent, 0.0f, pos}, c});
        vertices.push_back({{ extent, 0.0f, pos}, c});

        // خطوط موازية لمحور Z (تتغيّر على X)
        vertices.push_back({{pos, 0.0f, -extent}, c});
        vertices.push_back({{pos, 0.0f,  extent}, c});
    }
    return LineMesh(vertices);
}

LineMesh createAxisGizmo(float length) {
    std::vector<LineVertex> vertices = {
        // محور X - أحمر
        {{0,0,0}, {0.9f, 0.2f, 0.2f}}, {{length,0,0}, {0.9f, 0.2f, 0.2f}},
        // محور Y - أخضر
        {{0,0,0}, {0.2f, 0.9f, 0.2f}}, {{0,length,0}, {0.2f, 0.9f, 0.2f}},
        // محور Z - أزرق
        {{0,0,0}, {0.25f, 0.45f, 0.95f}}, {{0,0,length}, {0.25f, 0.45f, 0.95f}},
    };
    return LineMesh(vertices);
}
