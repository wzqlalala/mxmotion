//-----------------------------------------------------------------------------
// mPostNVHContributionModel —— 模态贡献量数据模型实现
//
// 解析经 Basic/PchReader 结构化读取：PFMODE 块数据行→genericRecords（entityId=模态
// 号、values=[实部,虚部]）、块头 $TOTAL RESPONSE→幅值+相位（缺行按模态求和合成）、
// $ITEM 标签→结果类型名（去 "(REAL/IMAG)" 尾注）；EIGEN MODES 表→entityId→values[0]
// （Hz）。频率口径：EIGEN MODES 第二列即模态频率 Hz（不作 sqrt 特征值换算）；单频
// 率点 PFMODE 数据行只含参与量显著模态，聚合按 EIGEN MODES 模态全集补零。
//-----------------------------------------------------------------------------
#include "mPostNVHContributionModel.h"

#include "pch/PchReader.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include <QVector>

namespace PostWindow {

namespace {

constexpr double kFreqEps = 1e-9;   //频率命中容差（绝对）

//块 $ITEM 类型名提取：去 "(REAL/IMAG)" 尾注并收尾空白；无 ITEM 标签回退 schemaName
std::string itemTypeNameOf(const PchReader::PchResultBlock& block)
{
    for (const auto& label : block.labels) {
        if (label.name == "ITEM" && label.hasValue) {
            std::string value = label.value;
            const auto suffix = value.rfind("(REAL/IMAG)");
            if (suffix != std::string::npos) {
                value.erase(suffix);
            }
            while (!value.empty() && (std::isspace(static_cast<unsigned char>(value.back())) != 0)) {
                value.pop_back();
            }
            if (!value.empty()) {
                return value;
            }
        }
    }
    return block.schemaName;
}

//总响应合成：TOTAL RESPONSE 幅值+相位→复数；缺行按模态求和合成
std::complex<double> totalResponseOf(const PchReader::PchResultBlock& block,
                                     const std::map<int, std::complex<double>>& modes)
{
    if (block.totalResponseMagnitude && block.totalResponsePhaseDegrees) {
        const double mag = *block.totalResponseMagnitude;
        const double phaseRad = *block.totalResponsePhaseDegrees * 0.017453292519943295; //kDegToRad
        return { mag * std::cos(phaseRad), mag * std::sin(phaseRad) };
    }
    std::complex<double> sum {};
    for (const auto& [mode, value] : modes) {
        sum += value;
    }
    return sum;
}

//块数据行→模态号→复数参与量
std::map<int, std::complex<double>> modeValuesOf(const PchReader::PchResultBlock& block)
{
    std::map<int, std::complex<double>> out;
    for (const auto& record : block.genericRecords) {
        if (record.values.size() >= 2) {
            out[record.entityId] = { record.values[0], record.values[1] };
        }
    }
    return out;
}

//EIGEN MODES 表→模态号→频率 Hz
std::map<int, double> modeTableOf(const PchReader::PchResultBlock& block)
{
    std::map<int, double> out;
    for (const auto& record : block.genericRecords) {
        if (!record.values.empty()) {
            out[record.entityId] = record.values[0];
        }
    }
    return out;
}

} // namespace

bool ModalContributionModel::loadFromFile(const std::string& filePath, std::string* errorMessage)
{
    //窄字节路径契约 = 本地 8 位编码（Windows ACP / Linux locale），Qt 侧经 toLocal8Bit
    //转换后传入。先验 is_open：打不开（含编码不匹配的中文路径）给出准确原因，勿落入
    //空流 → 0 块 → "不是 PFMODE 文件"的误导报错（2026-11-10 真机实锤）
    std::ifstream input(filePath);
    if (!input.is_open()) {
        if (errorMessage) {
            *errorMessage = "cannot open file '" + filePath + "'";
        }
        return false;
    }
    std::string local;
    const bool ok = loadFromStream(input, &local);
    if (!ok && errorMessage) {
        *errorMessage = "cannot load '" + filePath + "': " + local;
        return false;
    }
    return true;
}

bool ModalContributionModel::loadFromStream(std::istream& input, std::string* errorMessage)
{
    index_.clear();
    modeFrequenciesBySubcase_.clear();
    file_ = nullptr;
    try {
        const PchReader::PchReader reader;
        auto file = std::make_shared<const PchReader::PchFile>(reader.read(input));
        if (file->blocks.empty()) {
            if (errorMessage) {
                *errorMessage = "no PFMODE/EIGEN MODES blocks found (not a PFMODE pch file?)";
            }
            return false;
        }
        //可用块判据（2026-11-18 补，openspec change contribution-load-fail-dialog）：
        //blocks 非空≠可用——任意可解析 pch（如 TB VTF/NTF 速度频响表，块全为
        //Unknown）此前会静默成功、级联全空且不弹失败窗；须至少含一个
        //EigenModes/StructureParticipationFactors 块才算本面板的合法贡献量结果
        bool hasUsableBlock = false;
        for (const auto& block : file->blocks) {
            if (block.resultType == PchReader::ResultType::EigenModes) {
                hasUsableBlock = true;
                auto table = modeTableOf(block);
                auto& slot = modeFrequenciesBySubcase_[block.subcaseId];
                slot.insert(table.begin(), table.end());
                continue;
            }
            if (block.resultType != PchReader::ResultType::StructureParticipationFactors) {
                continue;
            }
            hasUsableBlock = true;
            const auto name = itemTypeNameOf(block);
            auto& series = index_[block.subcaseId][name][block.pointId][block.component];
            series.itemTypeName = name;
            series.frequencies.push_back(block.abscissa.value_or(0.0));
            series.blockIndices.push_back(&block - file->blocks.data());
        }
        if (!hasUsableBlock) {
            index_.clear();
            modeFrequenciesBySubcase_.clear();
            file_ = nullptr;
            if (errorMessage) {
                *errorMessage = "no PFMODE/EIGEN MODES blocks found (not a PFMODE pch file?)";
            }
            return false;
        }
        file_ = std::move(file);
        return true;
    } catch (const std::exception& e) {
        index_.clear();
        modeFrequenciesBySubcase_.clear();
        file_ = nullptr;
        if (errorMessage) {
            *errorMessage = e.what();
        }
        return false;
    }
}

std::vector<int> ModalContributionModel::subcaseIds() const
{
    std::vector<int> out;
    for (const auto& [subcase, gridIndex] : index_) {
        out.push_back(subcase);
    }
    return out;
}

std::string ModalContributionModel::subcaseTitle(int subcaseId) const
{
    if (!file_) {
        return {};
    }
    for (const auto& block : file_->blocks) {
        if (block.subcaseId != subcaseId) {
            continue;
        }
        if (!block.title.empty()) {
            return block.title;
        }
        if (!block.subtitle.empty()) {
            return block.subtitle;
        }
        return {};
    }
    return {};
}

std::vector<std::string> ModalContributionModel::resultTypeNames(int subcaseId) const
{
    const auto it = index_.find(subcaseId);
    if (it == index_.end()) {
        return {};
    }
    std::vector<std::string> out;
    for (const auto& [name, gridIndex] : it->second) {
        out.push_back(name);
    }
    return out;
}

std::vector<int> ModalContributionModel::responsePointIds(int subcaseId,
                                                          const std::string& resultTypeName) const
{
    const auto it = index_.find(subcaseId);
    if (it == index_.end()) {
        return {};
    }
    const auto typeIt = it->second.find(resultTypeName);
    if (typeIt == it->second.end()) {
        return {};
    }
    std::vector<int> out;
    for (const auto& [pointId, comps] : typeIt->second) {
        out.push_back(pointId);
    }
    return out;
}

std::vector<std::string> ModalContributionModel::components(int subcaseId,
                                                            const std::string& resultTypeName,
                                                            int gridId) const
{
    const auto it = index_.find(subcaseId);
    if (it == index_.end()) {
        return {};
    }
    const auto typeIt = it->second.find(resultTypeName);
    if (typeIt == it->second.end()) {
        return {};
    }
    const auto pointIt = typeIt->second.find(gridId);
    if (pointIt == typeIt->second.end()) {
        return {};
    }
    std::vector<std::string> out;
    for (const auto& [component, entry] : pointIt->second) {
        out.push_back(component);
    }
    return out;
}

std::string ModalContributionModel::componentDisplayName(const std::string& component)
{
    static const std::map<std::string, std::string> kMap {
        { "T1", "X" }, { "T2", "Y" }, { "T3", "Z" },
        { "R1", "RX" }, { "R2", "RY" }, { "R3", "RZ" },
    };
    const auto it = kMap.find(component);
    return it != kMap.end() ? it->second : component;
}

std::vector<double> ModalContributionModel::frequencies(int subcaseId,
                                                        const std::string& resultTypeName,
                                                        int gridId,
                                                        const std::string& component) const
{
    const auto* series = findSeries(subcaseId, resultTypeName, gridId, component);
    return series != nullptr ? series->frequencies : std::vector<double> {};
}

double ModalContributionModel::nearestFrequency(const std::vector<double>& candidates, double frequency)
{
    if (candidates.empty()) {
        return frequency;
    }
    const auto it = std::min_element(candidates.begin(), candidates.end(),
        [frequency](double a, double b) {
            return std::abs(a - frequency) < std::abs(b - frequency);
        });
    return *it;
}

std::map<int, double> ModalContributionModel::modeFrequencies(int subcaseId) const
{
    const auto it = modeFrequenciesBySubcase_.find(subcaseId);
    if (it != modeFrequenciesBySubcase_.end()) {
        return it->second;
    }
    //subcase 未命中且全文件仅一张表时回退该表
    if (file_ != nullptr && std::count_if(file_->blocks.cbegin(), file_->blocks.cend(),
        [](const PchReader::PchResultBlock& b) {
            return b.resultType == PchReader::ResultType::EigenModes;
        }) == 1) {
        return modeFrequenciesBySubcase_.begin()->second;
    }
    return {};
}

std::optional<ModalContributionModel::PointData> ModalContributionModel::pointData(int subcaseId,
                                                           const std::string& resultTypeName,
                                                           int gridId, const std::string& component,
                                                           double frequency) const
{
    const auto* series = findSeries(subcaseId, resultTypeName, gridId, component);
    if (series == nullptr || series->frequencies.empty()) {
        return std::nullopt;
    }
    const double lower = series->frequencies.front();
    const double upper = series->frequencies.back();
    if (frequency < lower - kFreqEps || frequency > upper + kFreqEps) {
        return std::nullopt;
    }
    const double hit = nearestFrequency(series->frequencies, frequency);
    const auto idx = static_cast<std::size_t>(
        std::lower_bound(series->frequencies.begin(), series->frequencies.end(), hit) - series->frequencies.begin());
    const auto& block = file_->blocks[series->blockIndices[idx]];
    PointData out;
    out.frequency = hit;
    out.modeContributions = modeValuesOf(block);
    //缺行模态按 EIGEN MODES 全集补零
    const auto universe = modeFrequencies(subcaseId);
    for (const auto& [mode, hz] : universe) {
        out.modeContributions.emplace(mode, std::complex<double> {});
    }
    out.totalResponse = totalResponseOf(block, out.modeContributions);
    return out;
}

std::optional<ModalContributionModel::ResponseSeries> ModalContributionModel::responseSeries(int subcaseId,
                                                                     const std::string& resultTypeName,
                                                                     int gridId,
                                                                     const std::string& component,
                                                                     double frequencyMin,
                                                                     double frequencyMax) const
{
    const auto* series = findSeries(subcaseId, resultTypeName, gridId, component);
    if (series == nullptr || series->frequencies.empty()) {
        return std::nullopt;
    }
    ResponseSeries out;
    const auto universe = modeFrequencies(subcaseId);
    for (std::size_t i = 0; i < series->frequencies.size(); ++i) {
        const double f = series->frequencies[i];
        if (f < frequencyMin - kFreqEps || f > frequencyMax + kFreqEps) {
            continue;
        }
        const auto& block = file_->blocks[series->blockIndices[i]];
        out.frequencies.push_back(f);
        const auto modes = modeValuesOf(block);
        out.total.push_back(totalResponseOf(block, modes));
        for (const auto& [mode, hz] : universe) {
            const auto it = modes.find(mode);
            out.modes[mode].push_back(it != modes.end() ? it->second : std::complex<double> {});
        }
    }
    if (out.frequencies.empty()) {
        return std::nullopt;
    }
    return out;
}

double ModalContributionModel::projectedContribution(const std::complex<double>& total,
                                                     const std::complex<double>& mode)
{
    const double denom = std::abs(total);
    if (denom <= 0.0) {
        return 0.0;
    }
    return (total.real() * mode.real() + total.imag() * mode.imag()) / denom;
}

double ModalContributionModel::magnitude(const std::complex<double>& value)
{
    return std::abs(value);
}

double ModalContributionModel::phaseDegrees(const std::complex<double>& value)
{
    if (value.real() == 0.0 && value.imag() == 0.0) {
        return 0.0;
    }
    double deg = std::arg(value) * 180.0 / 3.14159265358979323846;
    if (deg < 0.0) {
        deg += 360.0;
    }
    return deg;
}

std::vector<std::pair<int, double>> ModalContributionModel::rankAtPoint(const PointData& point,
                                                                        SortMetric metric)
{
    std::vector<std::pair<int, double>> out;
    out.reserve(point.modeContributions.size());
    for (const auto& [mode, value] : point.modeContributions) {
        const double scored = (metric == SortMetric::Magnitude || metric == SortMetric::MagnitudeRss)
            ? magnitude(value)
            : std::fabs(projectedContribution(point.totalResponse, value)); //投影绝对值
        out.emplace_back(mode, scored);
    }
    std::stable_sort(out.begin(), out.end(), [](const auto& a, const auto& b) {
        if (a.second != b.second) {
            return a.second > b.second;
        }
        return a.first < b.first;
    });
    return out;
}

std::vector<std::pair<int, double>> ModalContributionModel::rssRanking(
    const ResponseSeries& series, SortMetric metric)
{
    //RSS = sqrt(Σx²)（x=逐点量值），降序（同值按模态号升序）
    std::vector<std::pair<int, double>> out;
    out.reserve(series.modes.size());
    for (const auto& [mode, values] : series.modes) {
        double acc = 0.0;
        for (std::size_t i = 0; i < values.size(); ++i) {
            const double x = (metric == SortMetric::MagnitudeRss)
                ? magnitude(values[i])
                : projectedContribution(series.total[i], values[i]);
            acc += x * x;
        }
        out.emplace_back(mode, std::sqrt(acc));
    }
    std::stable_sort(out.begin(), out.end(), [](const auto& a, const auto& b) {
        if (a.second != b.second) {
            return a.second > b.second;
        }
        return a.first < b.first;
    });
    return out;
}

std::vector<std::complex<double>> ModalContributionModel::partialSumSeries(
    const ResponseSeries& series, const std::map<int, double>& modeRatiosPercent, bool exclude)
{
    std::vector<std::complex<double>> out(series.frequencies.size(), std::complex<double> {});
    std::vector<const std::vector<std::complex<double>>*> picked;
    std::vector<double> ratios;
    for (const auto& [mode, ratio] : modeRatiosPercent) {
        const auto it = series.modes.find(mode);
        if (it != series.modes.end()) {
            picked.push_back(&it->second);
            ratios.push_back(ratio / 100.0);
        }
    }
    for (std::size_t i = 0; i < out.size(); ++i) {
        std::complex<double> sum {};
        for (std::size_t k = 0; k < picked.size(); ++k) {
            sum += ratios[k] * (*picked[k])[i];
        }
        out[i] = exclude ? series.total[i] - sum : sum;
    }
    return out;
}

std::vector<std::complex<double>> ModalContributionModel::differenceSeries(
    const ResponseSeries& series, const std::vector<std::complex<double>>& partialSum)
{
    std::vector<std::complex<double>> out;
    out.reserve(series.frequencies.size());
    for (std::size_t i = 0; i < series.frequencies.size(); ++i) {
        out.push_back(series.total[i] - partialSum[i]);
    }
    return out;
}

const ModalContributionModel::ComponentSeries* ModalContributionModel::findSeries(
    int subcaseId, const std::string& resultTypeName, int gridId,
    const std::string& component) const
{
    const auto subIt = index_.find(subcaseId);
    if (subIt == index_.end()) {
        return nullptr;
    }
    const auto typeIt = subIt->second.find(resultTypeName);
    if (typeIt == subIt->second.end()) {
        return nullptr;
    }
    const auto pointIt = typeIt->second.find(gridId);
    if (pointIt == typeIt->second.end()) {
        return nullptr;
    }
    const auto compIt = pointIt->second.find(component);
    return compIt != pointIt->second.end() ? &compIt->second : nullptr;
}

//柱状图 Y 自动域（NVH 专项，2026-11-17 第五批裁定：商软实测为准；规则假设与
//样张验收口径见头文件注释）
ModalContributionModel::BarYDomain ModalContributionModel::barYAutoDomain(
    const QVector<double>& values, bool projected)
{
    //有限值全集；空/全非法回落单位域（面板实际调用恒有值，不触达）
    QVector<double> finite;
    finite.reserve(values.size());
    for (double v : values) {
        if (std::isfinite(v)) {
            finite.append(v);
        }
    }
    if (finite.isEmpty()) {
        return BarYDomain{ 0.0, 1.0, 1.0 };
    }
    const double vMin = *std::min_element(finite.cbegin(), finite.cend());
    const double vMax = *std::max_element(finite.cbegin(), finite.cend());
    double range = vMax - vMin;
    if (!(range > 0.0)) {
        range = std::fabs(vMax) > 0.0 ? std::fabs(vMax) : 1.0; //全同值防零除
    }
    //步长选择器（两档共用）：优先序列 {1,2,2.5,5}×10ⁿ 升序扫描，取极差/步 ≤ 8 的最小步
    //  （48Hz 投影样张：极差 1927.23 → 250 命中 7.71≤8；幅值样张：7.01e-06 → 1e-06 命中 7.01≤8）
    double step = 1.0;
    bool found = false;
    const int nLo = static_cast<int>(std::floor(std::log10(range))) - 8;
    const int nHi = static_cast<int>(std::ceil(std::log10(range))) + 1;
    for (int n = nLo; n <= nHi && !found; ++n) {
        for (double m : { 1.0, 2.0, 2.5, 5.0 }) {
            const double candidate = m * std::pow(10.0, n);
            if (range / candidate <= 8.0) {
                step = candidate;
                found = true;
                break;
            }
        }
    }
    BarYDomain out;
    out.step = step;
    if (projected) {
        //投影档：整步对齐后上下再各让一步（48Hz 样张：-1250−250=-1500 / 750+250=1000）
        out.min = std::floor(vMin / step) * step - step;
        out.max = std::ceil(vMax / step) * step + step;
    } else {
        //幅值档：零基线 + 上沿整步对齐（图一画法：0 起、顶 8.0e-06）
        out.min = 0.0;
        out.max = std::ceil(vMax / step) * step;
    }
    //端点退化防御（幅值档 vMax≤0 等）：max 恒高于 min
    if (!(out.max > out.min)) {
        out.max = out.min + step;
    }
    return out;
}

} // namespace PostWindow
