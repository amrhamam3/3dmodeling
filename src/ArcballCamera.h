#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// كاميرا Arcball: تدور حول نقطة هدف ثابتة (Target) عبر إحداثيات كروية
// (Yaw / Pitch / Distance) بدل تحريك موقعها مباشرة - هذا هو أسلوب
// "الدوران حول الجسم" المعتاد في برامج النمذجة (Blender/Maya).
class ArcballCamera {
public:
    glm::vec3 target = glm::vec3(0.0f);

    float yaw   = -90.0f;   // بالدرجات - الدوران الأفقي حول Y
    float pitch = 20.0f;    // بالدرجات - الدوران الرأسي (محدود لتفادي الانقلاب)
    float distance = 4.0f;  // المسافة بين الكاميرا والهدف (التقريب/التبعيد)

    float minDistance = 1.0f;
    float maxDistance = 20.0f;
    float minPitch = -89.0f;
    float maxPitch = 89.0f;

    float orbitSensitivity = 0.25f;   // درجة لكل بكسل سحب
    float zoomSensitivity  = 0.5f;    // وحدة مسافة لكل "خطوة" عجلة الفأرة

    // يُستدعى عند سحب الفأرة (بينما الزر الأوسط أو الأيمن مضغوط)
    void orbit(float deltaX, float deltaY) {
        yaw   += deltaX * orbitSensitivity;
        pitch -= deltaY * orbitSensitivity;
        pitch = glm::clamp(pitch, minPitch, maxPitch);
    }

    // يُستدعى عند تحريك عجلة الفأرة
    void zoom(float scrollDelta) {
        distance -= scrollDelta * zoomSensitivity;
        distance = glm::clamp(distance, minDistance, maxDistance);
    }

    glm::vec3 position() const {
        float yawRad   = glm::radians(yaw);
        float pitchRad = glm::radians(pitch);
        glm::vec3 offset;
        offset.x = distance * cos(pitchRad) * cos(yawRad);
        offset.y = distance * sin(pitchRad);
        offset.z = distance * cos(pitchRad) * sin(yawRad);
        return target + offset;
    }

    glm::mat4 viewMatrix() const {
        return glm::lookAt(position(), target, glm::vec3(0.0f, 1.0f, 0.0f));
    }
};
