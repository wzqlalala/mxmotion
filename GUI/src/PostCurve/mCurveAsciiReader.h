#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - csv/txt 通用列数据纯解析器（R22）
//
// 把按列组织的数值文本文件（.csv 逗号分隔 / .txt 空白分隔）解析为等长的
// 具名 double 列（AsciiColumnFile），供数据源仓库（mCurveDataSourceStore）
// 作为第二类数据源登记/装配。纯静态函数、无 GUI 依赖（只依赖 QtCore），
// 可独立编译单测（tests/PostCurveMath 同款 BUILD_STATIC 设定）。
//
// 解析规则（R22 需求文档 + 任务单 20261109-R22-T1"实现要求"第 2 条）：
//   - 编码：去 UTF-8 BOM；其余字节严格 UTF-8 可解码 -> 按 UTF-8，
//     否则按 Local8Bit（中文 Windows 下即 GBK 兼容）。
//   - 首行判定：首个非空白行的全部 token 均可解析 double -> 无表头
//     （首行也是数据行）；否则该行为表头行。
//   - 切分：行内含逗号 -> 按逗号切分（保留空位）并剥 token 成对包裹引号；
//     否则按连续空白（Tab/空格）切分。逐行独立判定，混用不保证（以数据行
//     为准：列数取表头与各数据行 token 数的最大值）。
//   - 列名：表头 token 去引号、去首尾空白；空位/缺位回退 "Column N"
//     （N=1 基列序号，按最终列数计）；无表头时全列 "Column N"。
//     列名不强制去重：同名列按"首个匹配下标"取列（装配侧约定）。
//   - 数据行：token 非 double -> NaN（保留行位）；行 token 不足列数时
//     尾部补 NaN；空白行跳过（不产生数据行）。
//   - 成功判定：>=1 列且 >=1 行数据；否则返回 false 并经 err 带原因
//     （文案与 pch"文件无有效数据块"同风格）。
// =============================================================================

#include <QString>
#include <QStringList>
#include <QVector>

namespace PostCurve
{
	//ascii 列数据集（数据源仓库的存储结构，与 PchReader::PchFile 分存——
	//不伪造 pch 数据块；各列等长，长度均等于 rowCount）
	struct AsciiColumnFile
	{
		QStringList columnNames;           //列名（显示序；同名列不强制去重）
		QVector<QVector<double>> columns;  //数值列（与 columnNames 一一对应、等长）
		int rowCount = 0;                  //数据行数（空白行不计）
	};

	class mCurveAsciiReader
	{
	public:
		mCurveAsciiReader() = delete;   //纯静态工具类，不实例化

		//读取并解析一个 csv/txt 文件：文件字节 -> 去 BOM/编码判定 -> parseText。
		//文件无法打开返回 false（err 带原因）；解析规则见类头注释。
		static bool readFile(const QString& filePath, AsciiColumnFile* out,
			QString* err = nullptr);

		//从已解码文本解析（readFile 的文本层，独立暴露便于单测构造边界样例）。
		//out 为空时返回 false（err"输出参数为空"，与仓库装配 err 风格一致）
		static bool parseText(const QString& text, AsciiColumnFile* out,
			QString* err = nullptr);
	};
}
