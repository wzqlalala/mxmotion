#include "mCurveAsciiReader.h"

#include <QFile>
#include <QFileInfo>

#include <limits>

namespace PostCurve
{
	namespace
	{
		//NaN 占位（非数值 token / 行短列）：保留行位，渲染/统计链路已有
		//非有限值跳过语义（如标签模板统计剔除、线性插值端点），此处只负责落值
		const double kNaN = std::numeric_limits<double>::quiet_NaN();

		//严格 UTF-8 可解码判定（RFC 3629：拒绝过长编码、代理区、>U+10FFFF）。
		//手写校验而非 QTextCodec 转换计数：无 codec 依赖、语义确定（QString::
		//fromUtf8 对非法序列静默替换 U+FFFD，无法区分"可严格解码"）
		bool isValidUtf8(const char* d, int n)
		{
			int i = 0;
			while (i < n)
			{
				const unsigned char c = static_cast<unsigned char>(d[i]);
				if (c < 0x80)
				{
					++i;   //ASCII 单字节
					continue;
				}
				int len = 0;
				unsigned cp = 0;
				if ((c & 0xE0) == 0xC0)      { len = 2; cp = c & 0x1F; }
				else if ((c & 0xF0) == 0xE0) { len = 3; cp = c & 0x0F; }
				else if ((c & 0xF8) == 0xF0) { len = 4; cp = c & 0x07; }
				else
					return false;   //0xC0/C1 过长、0xF8+ 非法首字节
				if (i + len > n)
					return false;   //多字节序列截断
				for (int k = 1; k < len; ++k)
				{
					const unsigned char cc = static_cast<unsigned char>(d[i + k]);
					if ((cc & 0xC0) != 0x80)
						return false;   //续字节非 10xxxxxx
					cp = (cp << 6) | (cc & 0x3F);
				}
				if ((len == 2 && cp < 0x80) || (len == 3 && cp < 0x800)
					|| (len == 4 && cp < 0x10000))
					return false;   //过长编码（本可用更短形式表示）
				if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF))
					return false;   //越界 / UTF-16 代理区
				i += len;
			}
			return true;
		}

		//文件字节 -> 文本（解析入口的编码层）：去 UTF-8 BOM；余下字节严格
		//UTF-8 可解码按 UTF-8，否则按 Local8Bit（中文 Windows = GBK 兼容，
		//覆盖 Excel 导出的 ANSI csv 与仪器导出 GBK 表头）
		QString decodeBytes(const QByteArray& bytes)
		{
			QByteArray body = bytes;
			if (body.size() >= 3
				&& static_cast<unsigned char>(body.at(0)) == 0xEF
				&& static_cast<unsigned char>(body.at(1)) == 0xBB
				&& static_cast<unsigned char>(body.at(2)) == 0xBF)
			{
				body = body.mid(3);   //去 UTF-8 BOM
			}
			if (isValidUtf8(body.constData(), body.size()))
				return QString::fromUtf8(body);
			return QString::fromLocal8Bit(body);
		}

		//按行切分（处理 \r\n / \n / \r 三种行尾；末行无行尾也成行）
		QStringList splitLines(const QString& text)
		{
			QStringList lines;
			const int n = text.size();
			int start = 0;
			for (int i = 0; i < n; ++i)
			{
				const QChar c = text.at(i);
				if (c != QLatin1Char('\n') && c != QLatin1Char('\r'))
					continue;
				lines.append(text.mid(start, i - start));
				//\r\n 作为一个行尾整体消费
				if (c == QLatin1Char('\r') && i + 1 < n && text.at(i + 1) == QLatin1Char('\n'))
					++i;
				start = i + 1;
			}
			lines.append(text.mid(start));
			return lines;
		}

		//剥 token 成对包裹引号（仅当首尾字符均为 '"' 且长度>=2；引号内文本不动）。
		//csv 约定引号包裹字段（多为含逗号/空白的文本列名）；不成对的单侧引号
		//视为名字的一部分，不剥
		QString stripPairedQuotes(QString tok)
		{
			if (tok.size() >= 2 && tok.startsWith(QLatin1Char('"')) && tok.endsWith(QLatin1Char('"')))
				tok = tok.mid(1, tok.size() - 2);
			return tok;
		}

		//单行 -> token 序列：含逗号按逗号切分（KeepEmptyParts——空位是合法
		//token 位置，如 curve1.csv 末列空名/尾逗号行）并剥成对引号；否则按
		//连续空白（Tab/空格）切分（空 token 无意义，直接丢弃）。逐行独立判定，
		//混用分隔符不保证（任务单约定：以数据行为准）
		QStringList splitTokens(const QString& line)
		{
			if (line.contains(QLatin1Char(',')))
			{
				const QStringList raw = line.split(QLatin1Char(','));
				QStringList tokens;
				tokens.reserve(raw.size());
				for (const QString& t : raw)
					tokens.append(stripPairedQuotes(t));
				return tokens;
			}
			//手工空白切分（不引正则；Qt 5.13 的 split 行为枚举为 QString::SkipEmptyParts，
			//手工循环同时规避版本差异）
			QStringList tokens;
			const int n = line.size();
			int i = 0;
			while (i < n)
			{
				while (i < n && line.at(i).isSpace())
					++i;
				if (i >= n)
					break;
				int j = i;
				while (j < n && !line.at(j).isSpace())
					++j;
				tokens.append(line.mid(i, j - i));
				i = j;
			}
			return tokens;
		}

		//表头 token 规整：去首尾空白、剥成对引号、再去一次首尾空白
		//（如 ` " STRAIN " ` -> `STRAIN`；引号内首尾空白视为格式噪音）。
		//返回空串表示"表头空位"，由调用方回退 "Column N"
		QString cleanHeaderToken(const QString& tok)
		{
			return stripPairedQuotes(tok.trimmed()).trimmed();
		}

		//token -> double（QString::toDouble 即 C locale，自容忍首尾空白；
		//空串/文本/带引号数字等非 double token 返回 NaN）
		double tokenToDouble(const QString& tok)
		{
			bool ok = false;
			const double v = tok.toDouble(&ok);
			return ok ? v : kNaN;
		}

		//token 序列是否全部可解析 double（首行判定：全数值 = 无表头；
		//空 token 不可解析 -> 按有表头处理）
		bool allTokensNumeric(const QStringList& tokens)
		{
			for (const QString& t : tokens)
			{
				bool ok = false;
				t.toDouble(&ok);
				if (!ok)
					return false;
			}
			return true;
		}
	}   //匿名命名空间

	bool mCurveAsciiReader::readFile(const QString& filePath, AsciiColumnFile* out, QString* err)
	{
		if (!out)
		{
			if (err) *err = QStringLiteral("输出参数为空");
			return false;
		}
		QFile file(filePath);
		if (!file.open(QIODevice::ReadOnly))
		{
			if (err) *err = QStringLiteral("无法打开文件：%1").arg(QFileInfo(filePath).fileName());
			return false;
		}
		return parseText(decodeBytes(file.readAll()), out, err);
	}

	bool mCurveAsciiReader::parseText(const QString& text, AsciiColumnFile* out, QString* err)
	{
		if (!out)
		{
			if (err) *err = QStringLiteral("输出参数为空");
			return false;
		}

		//切行后跳过空白行：首个非空白行做表头判定，其后每个非空白行是一个数据行
		const QStringList lines = splitLines(text);
		int firstIdx = -1;
		for (int i = 0; i < lines.size(); ++i)
		{
			if (!lines.at(i).trimmed().isEmpty())
			{
				firstIdx = i;
				break;
			}
		}
		if (firstIdx < 0)
		{
			if (err) *err = QStringLiteral("文件无有效数据列");   //全空白文件
			return false;
		}

		//首行判定（表头/无表头）与数据行范围
		const QStringList firstTokens = splitTokens(lines.at(firstIdx));
		const bool hasHeader = !allTokensNumeric(firstTokens);
		const int dataStart = hasHeader ? firstIdx + 1 : firstIdx;

		//列数 = max(表头 token 数, 各数据行 token 数)（ragged 行向上对齐）
		int columnCount = hasHeader ? firstTokens.size() : 0;
		for (int i = dataStart; i < lines.size(); ++i)
		{
			if (lines.at(i).trimmed().isEmpty())
				continue;
			columnCount = qMax(columnCount, splitTokens(lines.at(i)).size());
		}
		if (columnCount < 1)
		{
			if (err) *err = QStringLiteral("文件无有效数据列");
			return false;
		}

		//列名：表头 token 去引号去首尾空白；空位/缺位回退 "Column N"
		//（N=1 基列序号，按最终列数计）；无表头全列 "Column N"。
		//同名列不去重——装配按"首个匹配下标"取列（任务单约定）
		out->columnNames.clear();
		out->columnNames.reserve(columnCount);
		for (int c = 0; c < columnCount; ++c)
		{
			QString name;
			if (hasHeader && c < firstTokens.size())
				name = cleanHeaderToken(firstTokens.at(c));
			if (name.isEmpty())
				name = QStringLiteral("Column %1").arg(c + 1);
			out->columnNames.append(name);
		}

		//数据行装配：逐列等长（行短补 NaN，非数值 token 记 NaN 保留行位；
		//空白行跳过不产生数据行）
		out->columns.clear();
		for (int c = 0; c < columnCount; ++c)
			out->columns.append(QVector<double>());
		int rowCount = 0;
		for (int i = dataStart; i < lines.size(); ++i)
		{
			if (lines.at(i).trimmed().isEmpty())
				continue;
			const QStringList tokens = splitTokens(lines.at(i));
			for (int c = 0; c < columnCount; ++c)
			{
				if (c < tokens.size())
					out->columns[c].append(tokenToDouble(tokens.at(c)));
				else
					out->columns[c].append(kNaN);   //行 token 不足列数：尾部补 NaN
			}
			++rowCount;
		}
		if (rowCount < 1)
		{
			if (err) *err = QStringLiteral("文件无有效数据行");   //仅表头行
			return false;
		}

		out->rowCount = rowCount;
		return true;
	}
}
