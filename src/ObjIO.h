#pragma once
#include <string>
#include <vector>
#include "Mesh.h"

// نتيجة استيراد ملف OBJ: بيانات جاهزة لبناء Mesh جديد منها مباشرة
struct ObjImportResult {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    bool success = false;
    std::string errorMessage;
};

// يكتب الشبكة الحالية كملف Wavefront OBJ (v / vn / f) قابل للفتح
// في أي برنامج نمذجة آخر (Blender، MeshLab...)
bool exportOBJ(const std::string& path, const Mesh& mesh);

// يقرأ ملف OBJ (يدعم v، vn، وf بصيغها الشائعة v، v//vn، v/vt/vn، v/vt)
// ويُحوّل أي وجه متعدد الأضلاع إلى مثلثات (Fan Triangulation)
ObjImportResult importOBJ(const std::string& path);
