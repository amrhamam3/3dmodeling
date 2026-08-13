#version 330 core
in vec3 vNormal;
out vec4 FragColor;

uniform vec3 uColor;

void main() {
    // إضاءة بسيطة جداً (Lambert تقريبي) فقط لإظهار أوجه المكعب بوضوح
    vec3 lightDir = normalize(vec3(0.5, 0.8, 0.6));
    float diff = max(dot(normalize(vNormal), lightDir), 0.0);
    vec3 shaded = uColor * (0.35 + 0.65 * diff);
    FragColor = vec4(shaded, 1.0);
}
