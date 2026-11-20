#pragma once
//【GUI 兼容门控】dev_base 功能块依赖尚未推送的 BasicData/MXGeoCommand 新 API，
// 以宏门控暂闭（不删代码）；依赖合入后置 1 即恢复。命名 MXGUI_HAS_<功能族>，
// 新增门控请同步登记此处。
#define MXGUI_HAS_DIRECTMODELING_V2 0 // DirectModelingGeometry/Request 扩展（previewTransientDiscretized/RotateFace/setReplacementFace/setRotationAxis/simplifyResultOnCommit/rotationAxisPoint）
#define MXGUI_HAS_QUALITY_REPAIR_V2 0 // 质量检查/修复组族（孔检测/自由边/劈边/附加边/重复面/干涉移除/组结构）
#define MXGUI_HAS_SOLIDIFY_V2 0       // 实体化扩展（StlToStep/反向命令/缺失面组）
#define MXGUI_HAS_HOLE_FEATURE 0      // 孔特征（SolidEditOfHole/HoleCircle）
#define MXGUI_HAS_OPENEDGE_DETECT 0   // 开放边检测命令（DetectOfOpenEdges）
#define MXGUI_HAS_AUTOREMOVAL_V2 0    // 自动去除扩展（倒角检测/小径/Logo 面积参数）
#define MXGUI_HAS_PARAMETRIC_BASELINE 0 // 参数驱动基线回放（history().beginParametricBaseline/restoreParametricBaseline）
#define MXGUI_HAS_DELETE_EDGES 0      // 删除线引擎命令（geoEdit().editOfDeleteEdges）
