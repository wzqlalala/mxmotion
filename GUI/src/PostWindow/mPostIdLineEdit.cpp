#include "mPostIdLineEdit.h"
#include "mpostwindow_global.h"

//QT
#include <QRegularExpression>
#include <QSet>
#include <algorithm>

namespace PostWindow
{
	QValidator::State mPostIdValidator::validate(QString& input, int& pos) const
	{
		Q_UNUSED(pos);

		// 空字符串允许继续输入
		if (input.isEmpty())
			return Intermediate;

		// 完整合法的正则（判断整个输入是否最终合法）
		// 支持：单个数字 / start:end（范围） / start:end:step（带步长）
		static const QRegularExpression fullRe(
			R"(^\s*\d+(\s*:\s*\d+(\s*:\s*\d+)?)?\s*(,\s*\d+(\s*:\s*\d+(\s*:\s*\d+)?)?\s*)*$)"
		);

		if (fullRe.match(input).hasMatch())
		{
			// 格式完整合法，业务校验不在此处做，
			// 避免输入过程中（如 2:1 后面还想输 0 变成 2:10）被阻断
			return Acceptable;
		}

		// 部分输入允许：只要每个字符都在允许集合内（数字、空白、逗号、冒号）
		for (QChar c : input)
		{
			if (!(c.isDigit() || c.isSpace() || c == ',' || c == ':'))
			{
				return Invalid; // 出现非法字符，立即阻止
			}
		}

		// 禁止以分隔符开头（如 ":", "," 等）
		QString trimmed = input.trimmed();
		if (!trimmed.isEmpty())
		{
			QChar first = trimmed.front();
			if (first == ':' || first == ',')
			{
				return Invalid;
			}
		}

		// 禁止连续分隔符（如 "1::2", "1,,2" 等永远不可能合法的情况）
		for (int i = 0; i < trimmed.size() - 1; ++i)
		{
			if ((trimmed[i] == ':' || trimmed[i] == ',') &&
				(trimmed[i + 1] == ':' || trimmed[i + 1] == ','))
			{
				return Invalid;
			}
		}

		// 每个逗号分隔的段内，':' 最多只能出现 2 次（start:end:step）
		for (const QString& part : input.split(','))
		{
			QString seg = part.trimmed();
			if (seg.isEmpty())
				continue;

			if (seg.count(':') > 2)
				return Invalid;
		}

		// 全部是允许字符但不是完整的合法格式 -> Intermediate（允许继续输入）
		return Intermediate;
	}

	mPostIdLineEdit::mPostIdLineEdit(QWidget* parent) : QLineEdit(parent)
	{
		setValidator(new mPostIdValidator(this));
		setToolTip("1 或者 3:6 或者 4, 5:10, 60:100:2");

		// 输入完成时解析
		connect(this, &QLineEdit::editingFinished, this, [this]() {
			slot_parseIdString(this->text());
		});

		// 实时输入时检查有效性并高亮颜色
		connect(this, &QLineEdit::textChanged, this, &mPostIdLineEdit::slot_textChanged);
	}

	mPostIdLineEdit::~mPostIdLineEdit()
	{
	}

	std::vector<int> mPostIdLineEdit::getIds()
	{
		return _ids;
	}

	void mPostIdLineEdit::slot_parseIdString(const QString& text)
	{
		QSet<int> idSet; // 去重
		QStringList parts = text.split(",", QString::SkipEmptyParts);

		for (const QString& part : parts)
		{
			QString trimmed = part.trimmed();

			// 匹配范围（纯 : 范围/步长）
			QRegularExpression reColon(R"((\d+)\s*:\s*(\d+)(?:\s*:\s*(\d+))?)");
			QRegularExpressionMatch match = reColon.match(trimmed);

			if (match.hasMatch())
			{
				int start = match.captured(1).toInt();
				int end = match.captured(2).toInt();
				int step = match.captured(3).isEmpty() ? 1 : match.captured(3).toInt();

				// 业务校验：最大值不能小于最小值
				if (start > end)
					continue;

				// 业务校验：步长必须 >= 1 且 <= (end - start)
				if (step < 1 || step > (end - start))
					continue;

				for (int i = start; i <= end; i += step)
				{
					idSet.insert(i);
				}
			}
			else if (!trimmed.isEmpty())
			{
				// 单个数字
				bool ok = false;
				int num = trimmed.toInt(&ok);
				if (ok)
				{
					idSet.insert(num);
				}
			}
		}

		_ids.clear();
		_ids.reserve(idSet.size());
		for (int v : idSet)
		{
			_ids.push_back(v);
		}

		std::sort(_ids.begin(), _ids.end());
	}

	void mPostIdLineEdit::setInputState(InputState state)
	{
		QPalette pal = palette();
		pal.setColor(QPalette::Base,
			state == InputState::Invalid ? QColor("#FFB6C1")
			: state == InputState::Incomplete ? QColor("#FFFACD")
			: QColor(Qt::white));
		// 校验只更新调色板，不清空/替换主题 QSS；文本和光标几何不随输入变化。
		setPalette(pal);
		update();
	}

	void mPostIdLineEdit::slot_textChanged(const QString& text)
	{
		if (text.isEmpty())
		{
			setInputState(InputState::Normal);
			return;
		}

		mPostIdValidator idValidator;

		QString input = text;
		int pos = cursorPosition();
		QValidator::State state = idValidator.validate(input, pos);

		if (state == QValidator::Acceptable)
		{
			// 格式完整合法，再做业务校验（最大值不能小于最小值、步长范围等）
			// 业务不合法也标红，但不阻断输入（让用户可以继续修改）
			static const QRegularExpression rangeRe(
				R"(^\s*(\d+)\s*:\s*(\d+)(?:\s*:\s*(\d+))?\s*$)"
			);
			bool businessValid = true;
			for (const QString& part : text.split(','))
			{
				QString seg = part.trimmed();
				if (seg.isEmpty())
					continue;

				QRegularExpressionMatch m = rangeRe.match(seg);
				if (m.hasMatch())
				{
					int start = m.captured(1).toInt();
					int end = m.captured(2).toInt();
					int step = m.captured(3).isEmpty() ? 1 : m.captured(3).toInt();

					if (start > end || step < 1 || step > (end - start))
					{
						businessValid = false;
						break;
					}
				}
			}
			setInputState(businessValid ? InputState::Normal : InputState::Invalid);
		}
		else if (state == QValidator::Invalid)
		{
			setInputState(InputState::Invalid);
		}
		else
		{
			setInputState(InputState::Incomplete);
		}
	}
}
