#pragma once

//模态贡献量数据模型（NVH工具集面板数据层）：读 Nastran PFMODE pch 全文件，聚合
//工况→结果类型→响应点→分量→[频率] 索引与 EIGEN MODES 模态表；提供投影/幅值/相位、
//RSS 排序、部分和/差值等数据函数。纯数据无 GUI 依赖（可单测）。
//解析经 Basic/PchReader 结构化读取：PFMODE 数据行→genericRecords（entityId=模态号、
//values=[实部,虚部]）、块头 $TOTAL RESPONSE→totalResponseMagnitude/PhaseDegrees、
//EIGEN MODES 表→genericRecords（entityId=模态号、values[0]=频率 Hz）。
//频率口径：EIGEN MODES 第二列即模态频率 Hz（样例 415.55/786.06 与需求文本吻合，
//不作 sqrt 特征值换算）；单频率点的 PFMODE 数据行只含参与量显著的模态，聚合按
//模态全集补零。

#include <complex>
#include <cstddef>
#include <iosfwd>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "mpostwindow_global.h"
#include "pch/PchModel.h"

namespace PchReader {
class PchFile;
}

namespace PostWindow {

//PostWindow DLL 导出：单测与 §2 面板均从库外消费本模型
class MPOSTWINDOW_EXPORT ModalContributionModel
{
public:
    ModalContributionModel() = default;

    //一次读全文件并聚合；失败返回 false 且 *errorMessage（非空时）给出原因。
    //filePath 为窄字节，须为本地 8 位编码（Windows ACP / Linux locale）——Qt 侧以
    //toLocal8Bit 转换（toStdString 的 UTF-8 字节在中文 Windows 打不开中文路径）
    bool loadFromFile(const std::string& filePath, std::string* errorMessage = nullptr);
    bool loadFromStream(std::istream& input, std::string* errorMessage = nullptr);

    bool isLoaded() const { return file_ != nullptr; }

    //已加载文件只读句柄（未加载返回空；loadFromFile/loadFromStream 成功后恒非
    //空）——面板工况显示文本复用 mCurveDataSourceStore::subcaseDisplayText 同源
    //口径（与"定义曲线"文件页工况列显示一致，2026-11-16）
    std::shared_ptr<const PchReader::PchFile> file() const { return file_; }

    //—— 级联选项集（加载结果页签）——

    std::vector<int> subcaseIds() const;
    //Subcase 全名（$TITLE，供表头行 1；未命中/无标题返回空，调用方回退 "Subcase <id>"）
    std::string subcaseTitle(int subcaseId) const;
    //结果类型名 = PFMODE 块 $ITEM 类型名（去 "(REAL/IMAG)" 尾注，如 "MODAL DISPLACEMENT"）
    std::vector<std::string> resultTypeNames(int subcaseId) const;
    std::vector<int> responsePointIds(int subcaseId, const std::string& resultTypeName) const;
    std::vector<std::string> components(int subcaseId, const std::string& resultTypeName,
                                        int gridId) const;

    //分量显示名映射：T1/T2/T3→X/Y/Z、R1/R2/R3→RX/RY/RZ，其余原样返回
    static std::string componentDisplayName(const std::string& component);

    //—— 频率维度（升序去重）——

    std::vector<double> frequencies(int subcaseId, const std::string& resultTypeName,
                                    int gridId, const std::string& component) const;
    //最近邻频率点（候选空序列返回 frequency 原值）
    static double nearestFrequency(const std::vector<double>& candidates, double frequency);

    //EIGEN MODES 模态表：模态号→频率 Hz；subcase 未命中且全文件仅一张表时回退该表
    std::map<int, double> modeFrequencies(int subcaseId) const;

    //—— 单频点数据 ——
    struct PointData
    {
        double frequency = 0.0;
        //总响应复数：TOTAL RESPONSE 幅值+相位合成；块缺该行时按模态求和合成
        std::complex<double> totalResponse {};
        //模态全集逐阶参与量（pch 缺行补 0）
        std::map<int, std::complex<double>> modeContributions;
    };
    std::optional<PointData> pointData(int subcaseId, const std::string& resultTypeName,
                                       int gridId, const std::string& component,
                                       double frequency) const;

    //—— 频率范围序列（[frequencyMin, frequencyMax] 闭区间）——
    struct ResponseSeries
    {
        std::vector<double> frequencies;
        std::vector<std::complex<double>> total;
        //模态全集逐点参与量（pch 缺行补 0），与 frequencies 对齐
        std::map<int, std::vector<std::complex<double>>> modes;
    };
    std::optional<ResponseSeries> responseSeries(int subcaseId, const std::string& resultTypeName,
                                                 int gridId, const std::string& component,
                                                 double frequencyMin, double frequencyMax) const;

    //—— 复数分量数据函数（静态可单测）——

    //投影：mode 在 total 方向的保号投影 = Re(conj(total)·mode)/|total|；|total|=0 返回 0
    static double projectedContribution(const std::complex<double>& total,
                                        const std::complex<double>& mode);
    static double magnitude(const std::complex<double>& value);
    //相位角（度），归一 [0, 360)
    static double phaseDegrees(const std::complex<double>& value);

    //—— 排序依据（选项集随图表类型收敛，枚举覆盖全部四档供 UI 映射英文名）——
    enum class SortMetric
    {
        ProjectedAbs,     //投影绝对值（单频点：柱状/极坐标）
        Magnitude,        //幅值（单频点：柱状/极坐标）
        ProjectedAbsRss,  //投影绝对值 RSS（频率范围：2D线图）
        MagnitudeRss      //幅值 RSS（频率范围：2D线图）
    };

    //单频点量值降序（同值按模态号升序）；Rss 档按对应单点口径取值
    static std::vector<std::pair<int, double>> rankAtPoint(const PointData& point, SortMetric metric);
    //频率范围 RSS = sqrt(Σx²)（x=逐点量值）降序（同值按模态号升序）
    static std::vector<std::pair<int, double>> rssRanking(const ResponseSeries& series, SortMetric metric);

    //—— 部分和/差值（结果研究）——

    //modeRatiosPercent：模态号→% To Exclude（0~100）
    //exclude=true：partial = total − Σ ratio·mode（从总响应剔除选定）；未列模态不剔除
    //exclude=false：partial = Σ ratio·mode（仅叠加选定）；未列模态不叠加
    static std::vector<std::complex<double>> partialSumSeries(const ResponseSeries& series,
                                                              const std::map<int, double>& modeRatiosPercent,
                                                              bool exclude);
    //diff = total − partial（与 partialSum 序列等长；空序列返回空）
    static std::vector<std::complex<double>> differenceSeries(const ResponseSeries& series,
                                                              const std::vector<std::complex<double>>& partialSum);

    //—— 柱状图 Y 自动域（NVH 专项，2026-11-17 第五批裁定：商软实测为准）——

    //自动域结果：min/max=恰域端点（两端落刻度）、step=主刻度步长
    struct BarYDomain
    {
        double min = 0.0;
        double max = 0.0;
        double step = 1.0;
    };
    //柱显示值全集（含 Response/模态/Others，%Plot=归一化后）→ Y 自动域。
    //规则为单样张假设、待商软样张对照迭代（padding/步长选择常量集中此处调参）：
    //  步长选择器（两档共用）：优先序列 {1,2,2.5,5}×10ⁿ 取极差/步 ≤ 8 的最小步；
    //  投影档：min=floor(vMin/step)·step−step、max=ceil(vMax/step)·step+step
    //    （48Hz 样张复现：域 1000~-1500、步 250）；
    //  幅值档（值恒非负）：min=0（零基线）、max=ceil(vMax/step)·step
    //    （图一画法复现：0 起、步 1.0e-06、顶 8.0e-06）
    static BarYDomain barYAutoDomain(const QVector<double>& values, bool projected);

private:
    //一个（工况,结果类型,响应点,分量）的全部频率块：块下标与频率按升序对齐
    struct ComponentSeries
    {
        std::string itemTypeName;               //块 $ITEM 类型名（去尾注）
        std::vector<double> frequencies;        //升序
        std::vector<std::size_t> blockIndices;  //PchFile::blocks 下标，与 frequencies 对齐
    };
    //索引：subcaseId → 结果类型名 → gridId → component → ComponentSeries
    using GridIndex = std::map<int, std::map<std::string, ComponentSeries>>;
    std::map<int, std::map<std::string, GridIndex>> index_;
    //模态表：subcaseId → 模态号 → 频率 Hz
    std::map<int, std::map<int, double>> modeFrequenciesBySubcase_;

    std::shared_ptr<const PchReader::PchFile> file_;

    const ComponentSeries* findSeries(int subcaseId, const std::string& resultTypeName,
                                      int gridId, const std::string& component) const;
};

} // namespace PostWindow
