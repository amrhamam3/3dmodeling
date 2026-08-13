# CubeStudio — محرك نمذجة ثلاثي الأبعاد (تعليمي)

## البناء (محلياً أو عبر GitHub Actions)
كل المكتبات (GLFW / GLM / Dear ImGui / **GLAD 2**) تُجلَب تلقائياً عبر
CMake FetchContent - **لا توجد أي خطوة يدوية مطلوبة قبل البناء** بعد الآن.

```bash
cmake -S . -B build
cmake --build build --config Release
```

على GitHub، `.github/workflows/build.yml` يبني المشروع تلقائياً عند كل Push
على Linux و Windows، وينتج ملف تنفيذي جاهز للتحميل من تبويب **Actions**.

> **ملاحظة لمن جرّب نسخة سابقة:** كانت هناك خطوة يدوية لتوليد GLAD 1
> (تحميل من glad.dav1d.de أو تشغيل `pip install glad2`). تم استبدالها بالكامل
> بتكامل CMake الرسمي لـ GLAD 2 (`glad_add_library`) عبر FetchContent، بنفس
> أسلوب بقية المكتبات - هذا أوثق وهو سبب إصلاح مشاكل البناء على GitHub Actions.

## دعم اللغة العربية في الواجهة (Dear ImGui)
الواجهة (ImGui) لا تعرض حروفاً عربية افتراضياً (فقط Latin/ASCII) - أي نص
عربي سيظهر كعلامات استفهام "؟؟؟" إلى أن تضيف خطاً يدعم العربية:

1. حمّل خط "Noto Sans Arabic" (مجاني): https://fonts.google.com/noto/specimen/Noto+Sans+Arabic
2. ضع ملف `.ttf` داخل مجلد `fonts/` وسمِّه بالضبط: `NotoSansArabic-Regular.ttf`
3. أعد البناء - الكود يكتشف الملف تلقائياً ويحمّله بنطاقات Unicode العربية.

**قيد تقني مهم:** حتى مع الخط، الحروف العربية ستظهر **منفصلة عن بعضها**
(بدون الاتصال الطبيعي للخط) لأن Dear ImGui لا يملك محرك تشكيل نصوص (لا
HarfBuzz ولا FriBidi) ولا خوارزمية اتجاه ثنائي (Bidi) لعكس الاتجاه لـ RTL.
هذا قيد بالمكتبة نفسها، وليس خطأً بالكود. الخيارات لحل هذا بالكامل لاحقاً:
- دمج HarfBuzz (تشكيل الحروف) + FriBidi (ترتيب الاتجاه) قبل كل استدعاء
  لـ `ImGui::Text` مع نص عربي - عمل إضافي منفصل يمكن تنفيذه عند الطلب.
- أو تبسيط الأمر بتحويل نصوص الواجهة للإنجليزية (حل شائع عملياً بأدوات
  تقنية مشابهة) مع إبقاء تعليقات الكود بالعربية كما هي.

## بنية المشروع
```
CubeStudio/
├── CMakeLists.txt          ← يجلب GLFW + GLM + ImGui + GLAD 2 تلقائياً
├── fonts/                   ← ضع هنا خط عربي (راجع القسم أعلاه)
├── shaders/
│   ├── basic.vert / .frag   ← المكعب (إضاءة Lambert بسيطة)
│   └── line.vert / .frag    ← الشبكة والمحاور
└── src/
    ├── main.cpp              ← النافذة + الحلقة + ImGui + الكاميرا + التحرير
    ├── Shader.h / .cpp       ← تحميل الـ Shaders
    ├── Mesh.h / .cpp         ← بيانات المكعب (Vertices/Indices ديناميكية)
    ├── ArcballCamera.h       ← كاميرا Yaw/Pitch/Distance
    ├── RayUtils.h            ← Raycasting (تحديد/سحب نقطة)
    ├── Grid.h / .cpp         ← الشبكة ومحاور الإحداثيات
    └── ObjIO.h / .cpp        ← استيراد/تصدير OBJ
```

## الميزات المُنفَّذة حتى الآن
- نافذة GLFW + سياق OpenGL 3.3 Core + Render Loop
- Dear ImGui مدمجة (لوحة تحكم كاملة)
- مكعب حقيقي (24 Vertex / 36 Index) بإضاءة بسيطة
- كاميرا Arcball (دوران بالسحب + تقريب بالعجلة)
- Raycasting لتحديد وسحب زوايا المكعب
- شبكة أرضية ومحاور إحداثيات ملوّنة
- تراجع/إعادة (Undo/Redo) لتعديلات الزوايا
- استيراد/تصدير OBJ
