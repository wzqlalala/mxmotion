#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
// Altair ASCII（.hwascii）写出器 —— 官方 Generic ASCII 结果格式
//
// 把 AgaModel 的逐帧结果写出为官方 Altair ASCII 明文（首行 ALTAIR ASCII FILE），
// 供 HyperView 直接读取或 HvTrans 转换为 H3D。
// 该格式只含结果数据：网格由 FemWriter 另行写出为 Nastran bulk .fem，
// 两者按实体 ID（ENTITY_ID）绑定（格式考证见 doc/altair_ascii_format_notes.md）。
// 因此本类不写任何网格内容；validate() 仍用于校验结果数组与网格规模一致。
//
// 文件内的头/数据块语法遵循官方关键字（$SUBCASE/$BINDING/$COLUMN_INFO/
// $RESULT_TYPE/$TIME/$MODE/$FREQUENCY），不再使用自造块结构。

#include "aga_data.h"
#include "miofile_global.h"

#include <string>

namespace altairresultio {

struct AgaWriteOptions
{
    int precision = 12;                // 浮点有效位（文件体积与精度的折中）
    bool writeElementalDatasets = true;// 是否写出单元数据集
    bool crlf = false;                 // Windows 记事本友好时可开
};

class MIOFILE_EXPORT AgaWriter
{
public:
    // 全量写出逐帧结果。失败时返回 false 并填充 error。
    // outputPath 按 UTF-8 编码（与 QString::toStdString 一致），Windows 上内部转宽字符打开，
    // 中文路径不会再落成乱码文件名。
    static bool write(const AgaModel& model,
                      const std::string& outputPath,
                      const AgaWriteOptions& opt,
                      std::string& error);

    // 写出前的基础校验：节点/单元 ID 重复、连接引用不存在、
    // 数据集数量与网格规模不匹配等。返回第一个发现的问题描述，无问题返回空串。
    static std::string validate(const AgaModel& model);
};

} // namespace altairresultio