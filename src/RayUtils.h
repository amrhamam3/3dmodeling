#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>

// شعاع (Ray) في فضاء العالم: نقطة بداية + اتجاه (موحّد الطول)
struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
};

// يحوّل إحداثيات الفأرة على الشاشة (بالبكسل) إلى شعاع في فضاء العالم.
// الفكرة: نأخذ نقطتين على نفس عمود النقر (قرب المستوى القريب وقرب المستوى
// البعيد لهرم الرؤية)، نحوّلهما من NDC إلى فضاء العالم عبر مقلوب مصفوفة
// (Projection * View)، ثم الشعاع هو الخط الواصل بينهما.
inline Ray screenPointToRay(double mouseX, double mouseY, int screenW, int screenH,
                             const glm::mat4& view, const glm::mat4& projection) {
    float x = (2.0f * (float)mouseX) / (float)screenW - 1.0f;
    float y = 1.0f - (2.0f * (float)mouseY) / (float)screenH; // Y مقلوب (0 أعلى الشاشة)

    glm::mat4 invVP = glm::inverse(projection * view);

    glm::vec4 nearPoint = invVP * glm::vec4(x, y, -1.0f, 1.0f);
    glm::vec4 farPoint  = invVP * glm::vec4(x, y,  1.0f, 1.0f);
    nearPoint /= nearPoint.w;
    farPoint  /= farPoint.w;

    Ray ray;
    ray.origin = glm::vec3(nearPoint);
    ray.direction = glm::normalize(glm::vec3(farPoint) - glm::vec3(nearPoint));
    return ray;
}

// يسقط نقطة ثلاثية الأبعاد على الشاشة (إحداثيات بكسل) عبر مصفوفة MVP كاملة.
// نستخدمها لإيجاد أقرب زاوية للمكعب لموضع نقرة الفأرة (اختيار في فضاء الشاشة
// أدق وأسهل من المقارنة في فضاء العالم مباشرة).
inline glm::vec2 worldToScreen(const glm::vec3& worldPos, const glm::mat4& mvp, int screenW, int screenH) {
    glm::vec4 clip = mvp * glm::vec4(worldPos, 1.0f);
    glm::vec3 ndc = glm::vec3(clip) / clip.w;
    return glm::vec2(
        (ndc.x * 0.5f + 0.5f) * (float)screenW,
        (1.0f - (ndc.y * 0.5f + 0.5f)) * (float)screenH
    );
}

// تقاطع شعاع مع مستوى لا نهائي (نقطة عليه + اتجاه عمودي عليه Normal).
// نستخدمه أثناء السحب: نحرّك النقطة المحددة على مستوى مواجه للكاميرا
// يمر بموقعها الحالي، حتى تتبع الفأرة بشكل طبيعي.
inline bool rayPlaneIntersect(const Ray& ray, const glm::vec3& planePoint,
                               const glm::vec3& planeNormal, glm::vec3& outPoint) {
    float denom = glm::dot(planeNormal, ray.direction);
    if (fabs(denom) < 1e-6f) return false; // الشعاع موازٍ للمستوى

    float t = glm::dot(planePoint - ray.origin, planeNormal) / denom;
    if (t < 0.0f) return false; // التقاطع خلف الكاميرا

    outPoint = ray.origin + ray.direction * t;
    return true;
}
