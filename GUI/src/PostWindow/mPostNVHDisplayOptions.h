#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

// =============================================================================
// NVH工具集显示选项状态（模态贡献量分析，openspec D6/D30）
//
// 缩放三元组（档/dB10 参考值/dB20 参考值）按五语境独立存取（NVHContextScale
// × NVHCtx_Count，openspec context-display-options-state：2026-11-17 用户裁定
// 五语境原型），显示/加权/单位/绘图布局等公共行单份共享（NVHDisplayOptions，
// "设为全局"项级同步的载体）。纯值对象，无 GUI 依赖。
//
// 缩放枚举按显示选项对话框口径（Linear/Log/%Plot/dB10/dB20 五档单选）；落轴
// 属性时经 axisScaleMode/axisDbRef 换算模型口径（CurveAxisData.scaleMode
// 0 Linear/1 Log10/2 dB20/3 dB10，dbRef 单项共用——对话框侧 dB10/dB20 各自
// 独立参考值，产出时按当次档取值下发）。
// %Plot 是数据级换算档（贡献/总响应×100，轴保持 Linear，design D6），不是
// 通用轴刻度模式——axisScaleMode 落 0（Linear）。
// =============================================================================

#include <QString>

namespace PostWindow {

//缩放档（对话框单选顺序，spec 显示选项要求）
enum NVHScaleMode
{
    NVHScale_Linear = 0,
    NVHScale_Log,          //响应曲线 Y 轴默认档（spec 响应曲线先行产出）
    NVHScale_PercentPlot,
    NVHScale_dB10,
    NVHScale_dB20,
};

//显示选项语境（三页签四钮 + 结果显示页按当前图表类型展开三档；2026-11-17
//用户裁定五语境各自独立——加载/研究语境自 D30"未绑定共享"升格独立）
enum NVHDisplayCtx
{
    NVHCtx_Load = 0,       //加载结果-显示选项（应用链/响应曲线，预选 Log）
    NVHCtx_ShowBar,        //结果显示-柱状图（预选 %Plot，档集 {Linear,%Plot}）
    NVHCtx_ShowPolar,      //结果显示-极坐标图（预选且仅 Linear，design 69）
    NVHCtx_Show2D,         //结果显示-2D线图（预选 Log）
    NVHCtx_Research,       //结果研究 §7.3/§7.4（预选 %Plot，五档全可选）
    NVHCtx_Count,
};

//—— 公共行（单份共享；"设为全局"项级同步载体）——
struct NVHDisplayOptions
{
    //显示行（柱状/2D线图=相位形态；极坐标=首尾相接形态，行文案随语境联动，
    //spec 显示选项要求）
    //默认不显示相位：需求大图 01"默认选项如下"权威（2026-11-13 用户裁定对齐
    //原型；2026-11-11"默认双面板"口径作废——彼时系实现自定，需求稿未写死默认
    //档。连带默认形态：响应曲线=单幅值 XY、极坐标=非首尾相接散开，与大图
    //01/06 权威一致）
    bool displayPhase = false;     //false 不显示相位/非首尾相接  true 显示相位/首尾相接

    //加权（默认勾选 + A；weightMode 值域同 CurveAxisData.weightMode
    //0无/1A/2B/3C/4U）
    bool weightingOn = true;
    int weightMode = 1;

    //单位（勾选启用 + 时间/长度/力；一期仅存档，生产换算语义随真机对照
    //复核后补）
    bool unitsOn = false;
    int unitTime = 0;
    int unitLength = 0;
    int unitForce = 0;

    //绘图布局三选（0 在当前视图页上打开新窗口[默认] 1 在当前激活的窗口打开
    //2 在新视图页上打开新窗口，spec 显示选项要求）
    int layout = 0;

    //"设为全局"项级标记（加权/绘图布局两行有该勾选；勾选后三页签读同一份，
    //spec：项级同步）
    bool weightingGlobal = false;
    bool layoutGlobal = false;

    //轴侧计权（未勾选=无计权）
    int axisWeightMode() const { return weightingOn ? weightMode : 0; }
};

//—— 语境缩放（五语境各自独立，openspec context-display-options-state）——
struct NVHContextScale
{
    int scale = NVHScale_Log;      //语境起步档（Load/Show2D=Log；其余语境由面板构造置定）
    double db10Ref = 1.0;          //dB10 参考值（默认 1.0，spec）
    double db20Ref = 1.0;          //dB20 参考值（默认 1.0，spec）

    //CurveAxisData.scaleMode 口径（%Plot 落 Linear——数据级换算，非刻度模式）
    int axisScaleMode() const
    {
        switch (scale) {
        case NVHScale_Log:  return 1;
        case NVHScale_dB20: return 2;
        case NVHScale_dB10: return 3;
        default:            return 0;   //Linear/%Plot
        }
    }

    //当次缩放档的 dB 参考值（模型轴侧 dbRef 单项共用）
    double axisDbRef() const { return scale == NVHScale_dB10 ? db10Ref : db20Ref; }

    //轴标题缩放后缀（spec 响应曲线先行产出：Linear "(Scale=linear)"、Log
    //"(Scale=log)"、dB "(Scale=dB20<计权> Ref=<值>)"——计权=无省略字母、
    //A/B/C/U 写字母（2026-11-09 用户确认模板）；%Plot 无独立后缀（轴即 Linear）。
    //dB 计权字母取公共行计权态（计权属公共行，不入语境缩放）
    QString axisScaleSuffix(const NVHDisplayOptions& common) const
    {
        switch (scale) {
        case NVHScale_Linear:      return QStringLiteral("(Scale=linear)");
        case NVHScale_Log:         return QStringLiteral("(Scale=log)");
        case NVHScale_PercentPlot: return QStringLiteral("(Scale=linear)");
        case NVHScale_dB10:        return dBScaleSuffix(QStringLiteral("dB10"), common);
        case NVHScale_dB20:        return dBScaleSuffix(QStringLiteral("dB20"), common);
        }
        return QString();
    }

private:
    QString dBScaleSuffix(const QString& tag, const NVHDisplayOptions& common) const
    {
        const int weightMode = common.weightingOn ? common.weightMode : 0;
        QString text = QStringLiteral("(Scale=%1").arg(tag);
        if (weightMode >= 1 && weightMode <= 4)
            text += QStringLiteral("(%1)").arg(QChar('A' + (weightMode - 1)));   //dB20(A)/B/C/U——计权=无省略字母（spec 3.3 权威形态）
        const double ref = scale == NVHScale_dB10 ? db10Ref : db20Ref;
        //参考值整数形态按一位小数呈现（spec 例 "Ref=1.0"），非整数保有效位
        const QString refText = ref == static_cast<long double>(static_cast<long long>(ref))
            ? QString::number(ref, 'f', 1)
            : QString::number(ref, 'g', 6);
        return text + QStringLiteral(" Ref=%1)").arg(refText);
    }
};

} // namespace PostWindow
