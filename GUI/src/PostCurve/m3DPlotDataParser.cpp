#include "m3DPlotDataParser.h"

#include <cmath>
#include <limits>

namespace PostCurve
{
	namespace
	{
		//NaN 判定与占位（与 mCurveAsciiReader 的 NaN 落值语义衔接：
		//非数值 token/短列补位/空表头位列均为 NaN，此处负责形态级清理）
		bool isFiniteValue(double v)
		{
			return std::isfinite(v);
		}

		//列名是否为 mCurveAsciiReader 的位置回退名 "Column N"
		//（无表头时全列为回退名；表头空位/缺位处亦为回退名）
		bool isFallbackColumnName(const QString& name, int columnIndex)
		{
			return name == QStringLiteral("Column %1").arg(columnIndex + 1);
		}

		//列全部为 NaN（行尾空字段/空表头位对应的数据列即此形态）
		bool isColumnAllNaN(const QVector<double>& column)
		{
			for (const double v : column)
			{
				if (isFiniteValue(v))
					return false;
			}
			return true;
		}
	}   //匿名命名空间

	bool m3DPlotDataParser::readFile(const QString& filePath, AsciiColumnFile* out, QString* err)
	{
		//直通转发 R22 解析器（编码/切分/表头判定既有行为零回归）
		return mCurveAsciiReader::readFile(filePath, out, err);
	}

	bool m3DPlotDataParser::isMatrixTable(const AsciiColumnFile& file)
	{
		//总列数 ≥3（矩阵表至少 1 纵轴列 + 2 数值列）
		const int columnCount = file.columnNames.size();
		if (columnCount < 3 || file.rowCount < 1)
			return false;

		//表头行存在 + 表头 token 全部"可解析数值或空"：AsciiColumnFile 不带
		//hasHeader 标志，以"列名不同于位置回退名"判定表头行存在；回退名视为
		//空表头位放行（含 3DSURFACE csv 的空首格），其余不可解析列名一票否决
		bool hasHeader = false;
		bool headerAllNumericOrEmpty = true;
		for (int c = 0; c < columnCount; ++c)
		{
			const QString& name = file.columnNames.at(c);
			if (!isFallbackColumnName(name, c))
			{
				hasHeader = true;
				bool ok = false;
				name.toDouble(&ok);
				if (!ok)
					headerAllNumericOrEmpty = false;
			}
		}
		if (!hasHeader || !headerAllNumericOrEmpty)
			return false;

		//数据首列（纵轴列）全为有限数值
		const QVector<double>& firstColumn = file.columns.at(0);
		for (const double v : firstColumn)
		{
			if (!isFiniteValue(v))
				return false;
		}
		return true;
	}

	bool m3DPlotDataParser::assemblePointSeries(const AsciiColumnFile& file,
		PointSeries3D* out, QString* err)
	{
		if (!out)
		{
			if (err) *err = QStringLiteral("输出参数为空");
			return false;
		}

		const int columnCount = file.columns.size();
		if (columnCount < 2)
		{
			//单列数据无 X/Y 可取（2 列是"Z=0 容错"的下限口径）
			if (err) *err = QStringLiteral("文件仅 %1 列数据，三维点列至少需要 2 列（X/Y）。")
				.arg(columnCount);
			return false;
		}

		//取前三条数值列为 X/Y/Z（仅 2 列时 Z=0；>3 列多余列忽略）
		const QVector<double>& columnX = file.columns.at(0);
		const QVector<double>& columnY = file.columns.at(1);
		const bool hasZ = columnCount >= 3;
		const QVector<double> columnZ = hasZ ? file.columns.at(2) : QVector<double>();

		out->xs.clear();
		out->ys.clear();
		out->zs.clear();
		out->skippedRows = 0;
		for (int r = 0; r < file.rowCount; ++r)
		{
			const double x = columnX.at(r);
			const double y = columnY.at(r);
			const double z = hasZ ? columnZ.at(r) : 0.0;
			//非有限值行整行跳过不进图（保留计数，不造残点）
			if (!isFiniteValue(x) || !isFiniteValue(y) || !isFiniteValue(z))
			{
				++out->skippedRows;
				continue;
			}
			out->xs.append(x);
			out->ys.append(y);
			out->zs.append(z);
		}

		//剩余 ≥1 行才成功（全 NaN 文件等）
		if (out->xs.isEmpty())
		{
			if (err) *err = QStringLiteral("文件无有效数据点（%1 行因含非有限值被跳过）。")
				.arg(out->skippedRows);
			return false;
		}
		return true;
	}

	bool m3DPlotDataParser::assembleMatrixSeries(const AsciiColumnFile& file,
		MatrixSeries3D* out, QString* err)
	{
		if (!out)
		{
			if (err) *err = QStringLiteral("输出参数为空");
			return false;
		}

		//横轴=表头数值 token 逐个解析：自第 2 列起取"列名可解析为数值"的列
		//（空表头位=回退名列跳过，对应数据列一并排除，保证横轴与 Z 列对齐；
		//首列是纵轴列不参与横轴）
		QVector<int> dataColumns;
		out->xsAxis.clear();
		const int columnCount = file.columnNames.size();
		for (int c = 1; c < columnCount; ++c)
		{
			bool ok = false;
			const double v = file.columnNames.at(c).toDouble(&ok);
			if (!ok)
				continue;
			dataColumns.append(c);
			out->xsAxis.append(v);
		}
		if (dataColumns.isEmpty())
		{
			if (err) *err = QStringLiteral("矩阵表缺少有效数值列（表头无可用横轴值）。");
			return false;
		}

		//行尾空字段容忍第一步：裁掉尾部全 NaN 的列（csv 行尾逗号对应的
		//多余列、带表头却无数据的尾巴列）
		int cols = dataColumns.size();
		while (cols > 0 && isColumnAllNaN(file.columns.at(dataColumns.at(cols - 1))))
			--cols;
		if (cols < 1)
		{
			if (err) *err = QStringLiteral("矩阵表无有效数据列（数值列全为空）。");
			return false;
		}
		dataColumns.resize(cols);
		out->xsAxis.resize(cols);

		//行尾空字段容忍第二步：裁掉尾部全 NaN 的数据行（整行空 token 行）
		int rows = file.rowCount;
		while (rows > 0)
		{
			bool anyFinite = false;
			for (int c = 0; c < cols; ++c)
			{
				if (isFiniteValue(file.columns.at(dataColumns.at(c)).at(rows - 1)))
				{
					anyFinite = true;
					break;
				}
			}
			if (anyFinite)
				break;
			--rows;
		}
		if (rows < 1)
		{
			if (err) *err = QStringLiteral("矩阵表无有效数据行（Z 单元全为空）。");
			return false;
		}

		//装配 Row-Major（zs[r*cols+c]）：纵轴=columns[0]（形态判定已保证
		//全有限），Z=各数值列数据；xs/ys 同步展开为逐单元数组（同 MapData
		//扁平喂法）；内部非有限单元按 0 兜底并计数
		out->ysAxis.clear();
		out->xs.clear();
		out->ys.clear();
		out->zs.clear();
		out->ysAxis.reserve(rows);
		const int cellCount = rows * cols;
		out->xs.reserve(cellCount);
		out->ys.reserve(cellCount);
		out->zs.reserve(cellCount);
		out->zeroFilledCells = 0;
		for (int r = 0; r < rows; ++r)
		{
			const double y = file.columns.at(0).at(r);
			out->ysAxis.append(y);
			for (int c = 0; c < cols; ++c)
			{
				double v = file.columns.at(dataColumns.at(c)).at(r);
				if (!isFiniteValue(v))
				{
					v = 0.0;
					++out->zeroFilledCells;
				}
				out->xs.append(out->xsAxis.at(c));
				out->ys.append(y);
				out->zs.append(v);
			}
		}
		out->rows = rows;
		out->cols = cols;

		//兜底后 Z 极值（色带 setDataRange 消费口径，同 MapData::minEff/maxEff）
		out->minZ = out->zs.front();
		out->maxZ = out->zs.front();
		for (const double v : out->zs)
		{
			out->minZ = qMin(out->minZ, v);
			out->maxZ = qMax(out->maxZ, v);
		}
		return true;
	}
}
