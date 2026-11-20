#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

// 点在面上副边界墙提取（纯 OCC/STL，无 Qt，便于独立测试程序直接编译复用）
#include <TopoDS_Face.hxx>
#include <vector>

namespace MechMainWindow
{
    /**
     * @brief 沿面外轮廓 pcurve 采样生成边界"墙"数据（节点对 + 内向单位向量）
     *
     * 内向方向按 OCC 参数域定向约定解析求出：沿轮廓存储方向行走，材料恒在
     * 行进方向左侧（相对曲面自然法向 N = Su×Sv），故 inward = N × T。
     * 不再使用 BRepClass_FaceClassifier 探针（圆角弦弓高内小偏移双向都判 IN、
     * 探针落洞等问题的根源）。外轮廓用 BRepTools::OuterWire 直接取得；
     * 接缝边（BRep_Tool::IsClosed，圆柱等封闭面的缝合线）与退化边跳过；
     * 近共线段合并（<30°）控制墙数。
     *
     * 孔洞内环不建墙：半平面墙的可行域是凸集，带孔面不可能表示为半平面交，
     * 孔的弦墙会拦到孔对面的合法材料（需要引擎侧近距激活墙语义才能支持）。
     *
     * @param face   约束面（位置已含）
     * @param offX/offY/offZ  节点平移（COM 局部系约定，方向向量不受影响）
     * @param outNodes      追加写入节点坐标（N*3）
     * @param outWallEdges  追加写入墙线段节点对（K*2，索引相对 outNodes 全局）
     * @param outInward     追加写入内向单位向量（K*3）
     * @return 追加的墙数
     */
    int buildPosBoundaryWalls(const TopoDS_Face& face,
                              double offX, double offY, double offZ,
                              std::vector<double>& outNodes,
                              std::vector<int>& outWallEdges,
                              std::vector<double>& outInward);
}
