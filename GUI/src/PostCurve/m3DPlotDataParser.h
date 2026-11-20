#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 三维图数据纯解析器（R24 / 20261109-3D曲线-T2）
//
// 把用户真实数据文件（逗号/空白自适应点列、矩阵表 csv）解析为 Plot3D
// Series API 可直接消费的两类形态（点列 / 矩阵表），替代 R23 直连 Plot3D
// MapFileLoader 的数据口径（后者只按空白切分、每行 ≥4 列，用户文件全部
// 解析失败）。纯静态函数、无 GUI 依赖（只依赖 QtCore），可在
// tests/PostCurve3D 同款无头单测里独立编译（BUILD_STATIC 设定，同
// mCurveAsciiReader 先例）。
//
// 分层：本类不重复造切分/编码轮子——文件字节→文本→token 切分→AsciiColumnFile
// 全部复用 R22 mCurveAsciiReader（BOM/GBK 编码、逗号/空白逐行自适应、表头
// 判定、非数值 token NaN 占位、短行补 NaN、空行跳过等既有行为零回归），
// 本类只在其解析结果之上做形态判定与三维数据装配。
//
// 形态判定（矩阵表 vs 点列，isMatrixTable）：
//   矩阵表 = 表头行存在，且表头 token 全部可解析为数值或为空，且数据首列
//   （columns[0]）全为有限数值，且总列数 ≥3；其余 → 点列。
//   说明：AsciiColumnFile 不携带 hasHeader 标志，"表头行存在"以列名是否
//   不同于其位置回退名 "Column N" 判定（无表头时全列均为回退名）；表头空位
//   （如 3DSURFACE csv 首格为空）经回退名放行——即回退名按"空表头位"参与
//   "数值或空"检查（表头文本恰为 "Column N" 的退化样例按空位放行，属可
//   接受的有意取舍）。
//   维度口径：形态判定不校验维度（总列数 ≥3 即判矩阵表成立）——曲面/瀑布
//   消费方要求 rows/cols ≥2（同 MapData::isValid），退化矩阵由消费方回退
//   兜底；柱状图矩阵口径 1 行矩阵仍有效（=一排柱）。
//
// 点列装配（assemblePointSeries，曲线/柱状图口径）：
//   - 取前三条数值列为 X/Y/Z；仅 2 列时 Z=0（容错不拒绝）；>3 列取前三
//     （多余列忽略）；
//   - X/Y/Z 任一非有限的行整行跳过不进图（带回跳过计数）；
//   - 剩余 ≥1 行才成功，否则 err 带原因。
//
// 矩阵表装配（assembleMatrixSeries，曲面/瀑布/柱状图口径）：
//   - 横轴 xsAxis = 表头数值 token 逐个解析（空表头位跳过，对应数据列一并
//     排除，保证横轴与 Z 列对齐）；纵轴 ysAxis = columns[0] 数据（形态判定
//     已保证全有限）；
//   - Z 矩阵 = 各数值列数据，按 Plot3D Row-Major 口径展开
//     （zs[r*cols+c]，r=纵轴行、c=横轴列；rows/cols 同 MapData 语义）；
//     xs/ys 同步展开为逐单元数组（xs[r*cols+c]=xsAxis[c]、ys[r*cols+c]=
//     ysAxis[r]），与 Surface/Waterfall/Bar Series::setData 的扁平数组喂法
//     一致（参照 MapData）；
//   - 行尾空字段容忍：先裁掉尾部全 NaN 的列与行，再装配；
//   - 内部非有限单元按 0 兜底并带回计数（zeroFilledCells，弹窗提示行
//     注明"N 个无效单元按 0 处理"）；
//   - minZ/maxZ = 兜底后全部 Z 单元的极值（色带 setDataRange 用，同
//     MapData::minEff/maxEff 的消费方式）。
//
// 兜底口径（调用方职责，本类不感知）：曲面/瀑布图在矩阵表判定不成立或
// 装配失败时回退 R23 的 MapFileLoader::loadFile 扁平网格（Plot3D 旧样例
// MapResult.txt 行为不回归）；Plot3D RPM 瀑布专用格式仍不支持。
// =============================================================================

#include "mCurveAsciiReader.h"

#include <QString>
#include <QVector>

namespace PostCurve
{
	//三维点列装配结果（曲线/柱状图点列口径；三条等长，长度=有效点数）
	struct PointSeries3D
	{
		QVector<double> xs;        //X 坐标
		QVector<double> ys;        //Y 坐标
		QVector<double> zs;        //Z 坐标（仅 2 列文件恒为 0）
		int skippedRows = 0;       //因含非有限值整行跳过的行数
	};

	//三维矩阵表装配结果（曲面/瀑布/柱状图矩阵口径）。zs 按 Plot3D Row-Major
	//口径展开；xs/ys 同步展开为逐单元数组（Surface/Waterfall::setData 与
	//Bar::setData 都按逐单元扁平数组消费，同 MapData 喂法），xsAxis/ysAxis
	//保留紧凑轴向（柱宽点距估算/单测断言用）
	struct MatrixSeries3D
	{
		QVector<double> xsAxis;    //横轴值（表头数值 token），长度=cols
		QVector<double> ysAxis;    //纵轴值（columns[0] 数据），长度=rows
		QVector<double> xs;        //逐单元横轴（Row-Major：xs[r*cols+c]=xsAxis[c]）
		QVector<double> ys;        //逐单元纵轴（Row-Major：ys[r*cols+c]=ysAxis[r]）
		QVector<double> zs;        //逐单元 Z（Row-Major 展开），长度=rows*cols
		int rows = 0;              //纵轴样本数（数据行数，尾部全 NaN 行已裁）
		int cols = 0;              //横轴样本数（数值表头列数，尾部全 NaN 列已裁）
		int zeroFilledCells = 0;   //内部非有限单元按 0 兜底的个数
		double minZ = 0.0;         //兜底后 Z 极值（色带范围）
		double maxZ = 0.0;
	};

	class m3DPlotDataParser
	{
	public:
		m3DPlotDataParser() = delete;   //纯静态工具类，不实例化

		//读取并解析文件为列数据集（mCurveAsciiReader::readFile 的直通转发，
		//让调用方只需 include 本头文件）；失败 err 带原因
		static bool readFile(const QString& filePath, AsciiColumnFile* out,
			QString* err = nullptr);

		//形态判定：是否矩阵表（规则见类头注释；点列样例恒 false）
		static bool isMatrixTable(const AsciiColumnFile& file);

		//点列装配（曲线/柱状图口径）：规则见类头注释；失败 err 带原因
		static bool assemblePointSeries(const AsciiColumnFile& file,
			PointSeries3D* out, QString* err = nullptr);

		//矩阵表装配（曲面/瀑布/柱状图口径）：规则见类头注释；失败 err 带原因
		//（调用方在矩阵表装配失败时应回退 MapFileLoader::loadFile 扁平网格）
		static bool assembleMatrixSeries(const AsciiColumnFile& file,
			MatrixSeries3D* out, QString* err = nullptr);
	};
}
