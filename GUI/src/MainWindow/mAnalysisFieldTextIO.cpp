#pragma execution_character_set("utf-8")

#include "mAnalysisFieldTextIO.h"

#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

namespace MechMainWindow
{
	QList<AnalysisFieldRow> parseAnalysisFieldTextFile(
		const QString& path, int startRow, int endRow, int startColumn,
		bool isPointCloud, MxFunctions::PickDataType locate, int componentCount,
		QString& error)
	{
		error.clear();
		QFile file(path);
		if (!file.exists())
		{
			error = QStringLiteral("文件不存在：%1").arg(path);
			return {};
		}
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		{
			error = QStringLiteral("无法打开文件：%1").arg(path);
			return {};
		}
		QTextStream in(&file);
		in.setCodec("UTF-8");

		//最少列数（不含分量列）：点云=3，单元面=2，节点/单元=1
		const int minColumns = isPointCloud ? 3
			: (locate == MxFunctions::PickDataType::ElementFace ? 2 : 1);

		QList<AnalysisFieldRow> rows;
		const int rowFrom = qMax(1, startRow);
		const int rowTo = endRow;	//结束行小于起始行时按实际可读范围处理（读到文件尾）
		static const QRegularExpression separator(QStringLiteral("[\\s,]+"));
		//流式逐行读取：避免 readAll + split 把整文件复制多份，大文件下内存可控
		int lineNo = 0;
		while (!in.atEnd())
		{
			const QString line = in.readLine().trimmed();
			++lineNo;
			if (lineNo < rowFrom)
				continue;
			if (rowTo >= rowFrom && lineNo > rowTo)
				break;
			if (line.isEmpty())
				continue;
			QStringList tokens = line.split(separator, QString::SkipEmptyParts);
			if (tokens.size() < startColumn)
				continue;
			//丢弃起始列之前的列
			tokens = tokens.mid(startColumn - 1);
			if (tokens.size() < minColumns)
				continue;

			AnalysisFieldRow row;
			if (isPointCloud)
			{
				row.x = tokens.value(0);
				row.y = tokens.value(1);
				row.z = tokens.value(2);
			}
			else if (locate == MxFunctions::PickDataType::ElementFace)
			{
				row.elementId = tokens.value(0);
				row.faceId = tokens.value(1);
			}
			else
			{
				row.id = tokens.value(0);
			}
			//分量不足补空、超出截断
			for (int i = 0; i < componentCount; ++i)
				row.values.append(tokens.value(minColumns + i));
			rows.append(row);
		}
		file.close();
		return rows;
	}

	bool exportAnalysisFieldTextFile(
		const QString& path, const QList<AnalysisFieldRow>& rows,
		bool isPointCloud, MxFunctions::PickDataType locate, int componentCount,
		QString& error)
	{
		error.clear();
		QFile file(path);
		if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
		{
			error = QStringLiteral("无法写入文件：%1").arg(path);
			return false;
		}
		QTextStream out(&file);
		out.setCodec("UTF-8");

		//首行表头
		QStringList header;
		if (isPointCloud)
			header << QStringLiteral("X") << QStringLiteral("Y") << QStringLiteral("Z");
		else if (locate == MxFunctions::PickDataType::ElementFace)
			header << QStringLiteral("EID") << QStringLiteral("FaceID");
		else
			header << QStringLiteral("ID");
		for (int i = 1; i <= componentCount; ++i)
			header << QStringLiteral("Value%1").arg(i);
		out << header.join(QStringLiteral(",")) << "\n";

		for (const auto& row : rows)
		{
			QStringList fields;
			if (isPointCloud)
				fields << row.x << row.y << row.z;
			else if (locate == MxFunctions::PickDataType::ElementFace)
				fields << row.elementId << row.faceId;
			else
				fields << row.id;
			for (int i = 0; i < componentCount; ++i)
				fields << row.values.value(i);
			out << fields.join(QStringLiteral(",")) << "\n";
		}
		file.close();
		return true;
	}
}
