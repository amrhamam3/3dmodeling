#include "ObjIO.h"
#include <fstream>
#include <sstream>
#include <unordered_map>

bool exportOBJ(const std::string& path, const Mesh& mesh) {
    std::ofstream file(path);
    if (!file.is_open()) return false;

    const auto& verts = mesh.vertices();
    const auto& idx = mesh.indices();

    file << "# تم التصدير من CubeStudio\n";
    for (const auto& v : verts) {
        file << "v " << v.position.x << " " << v.position.y << " " << v.position.z << "\n";
    }
    for (const auto& v : verts) {
        file << "vn " << v.normal.x << " " << v.normal.y << " " << v.normal.z << "\n";
    }
    // كل نقطة عندنا تحمل Normal خاصاً بها (غير مشتركة)، لذا فهرس v وfهرس vn متطابقان لكل وجه
    for (size_t i = 0; i + 2 < idx.size(); i += 3) {
        unsigned int a = idx[i] + 1, b = idx[i + 1] + 1, c = idx[i + 2] + 1; // OBJ يبدأ من 1 وليس 0
        file << "f " << a << "//" << a << " " << b << "//" << b << " " << c << "//" << c << "\n";
    }
    return true;
}

// مفتاح فريد لكل مزيج (موقع، Normal) داخل تعريف وجه OBJ - نستخدمه حتى لا
// نكرر نفس الـ Vertex إذا تكرر نفس المزيج في أكثر من وجه
struct FaceVertexKey {
    int posIdx;
    int normIdx;
    bool operator==(const FaceVertexKey& o) const { return posIdx == o.posIdx && normIdx == o.normIdx; }
};
struct FaceVertexKeyHash {
    size_t operator()(const FaceVertexKey& k) const {
        return std::hash<long long>()((static_cast<long long>(k.posIdx) << 32) ^ (unsigned int)k.normIdx);
    }
};

// يحلّل عنصر وجه واحد بصيغه المختلفة: "v" أو "v/t" أو "v//n" أو "v/t/n"
static void parseFaceToken(const std::string& token, int& posIdx, int& normIdx) {
    posIdx = 0;
    normIdx = -1;
    size_t firstSlash = token.find('/');
    if (firstSlash == std::string::npos) {
        posIdx = std::stoi(token);
        return;
    }
    posIdx = std::stoi(token.substr(0, firstSlash));
    size_t secondSlash = token.find('/', firstSlash + 1);
    if (secondSlash != std::string::npos && secondSlash + 1 < token.size()) {
        normIdx = std::stoi(token.substr(secondSlash + 1));
    }
}

ObjImportResult importOBJ(const std::string& path) {
    ObjImportResult result;
    std::ifstream file(path);
    if (!file.is_open()) {
        result.success = false;
        result.errorMessage = "تعذّر فتح الملف: " + path;
        return result;
    }

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::unordered_map<FaceVertexKey, unsigned int, FaceVertexKeyHash> lookup;

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string tag;
        ss >> tag;

        if (tag == "v") {
            glm::vec3 p;
            ss >> p.x >> p.y >> p.z;
            positions.push_back(p);
        } else if (tag == "vn") {
            glm::vec3 n;
            ss >> n.x >> n.y >> n.z;
            normals.push_back(n);
        } else if (tag == "f") {
            std::vector<unsigned int> faceIndices;
            std::string token;
            while (ss >> token) {
                int posIdx = 0, normIdx = -1;
                try {
                    parseFaceToken(token, posIdx, normIdx);
                } catch (...) {
                    continue; // تجاهل أي عنصر مشوّه بدل إيقاف الاستيراد بالكامل
                }
                if (posIdx < 0) posIdx = (int)positions.size() + posIdx + 1; // فهرسة سالبة (نادرة في OBJ)
                if (posIdx < 1 || posIdx > (int)positions.size()) continue;

                FaceVertexKey key{posIdx, normIdx};
                auto it = lookup.find(key);
                unsigned int vertIndex;
                if (it != lookup.end()) {
                    vertIndex = it->second;
                } else {
                    Vertex v;
                    v.position = positions[posIdx - 1];
                    v.normal = (normIdx > 0 && normIdx <= (int)normals.size())
                                   ? normals[normIdx - 1]
                                   : glm::vec3(0.0f, 1.0f, 0.0f); // Normal افتراضي إن لم يوجد
                    result.vertices.push_back(v);
                    vertIndex = (unsigned int)(result.vertices.size() - 1);
                    lookup[key] = vertIndex;
                }
                faceIndices.push_back(vertIndex);
            }
            // تثليث أي وجه متعدد الأضلاع (Fan Triangulation) حول أول نقطة فيه
            for (size_t i = 1; i + 1 < faceIndices.size(); ++i) {
                result.indices.push_back(faceIndices[0]);
                result.indices.push_back(faceIndices[i]);
                result.indices.push_back(faceIndices[i + 1]);
            }
        }
    }

    result.success = !result.vertices.empty() && !result.indices.empty();
    if (!result.success) result.errorMessage = "لم يتم العثور على بيانات هندسية صالحة في الملف";
    return result;
}
