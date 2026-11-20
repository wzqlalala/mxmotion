#include "mCurveEditorWidgets.h"

#include "../CurveCore/CurveDefine.h"
#include "../CurveCore/CurveDocument.h"
#include "../CurveCore/mCurveDocController.h"
#include "../mDlgGeometry.h"

#include <QColorDialog>
#include <QFontDialog>
#include <QDialog>
#include "mFramelessMessageBox.h"
#include "ThemeManager.h"	//自样式弹窗豁免标记（kSelfStyledProp）
#include <QMessageBox>
#include <QSignalBlocker>
#include <QPlainTextEdit>
#include <QLabel>
#include <QPixmap>
#include <QPainter>
#include <QLineF>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QStyleOptionButton>
#include <QListWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QLineEdit>
#include <QToolButton>
#include <QStandardItemModel>
#include <QUndoStack>
#include <QSizeGrip>
#include <QRegularExpression>
#include <QLocale>

namespace PostCurve
{
	namespace
	{
		//"编辑字符"弹窗标题栏拖动（无边框窗口，mEditCurveDlg 同款交互）：
		//按住标题栏拖动窗口位置；挂在标题栏上，随弹窗销毁
		class mTitleBarDragFilter : public QObject
		{
		public:
			explicit mTitleBarDragFilter(QDialog* dialog)
				: QObject(dialog), _dialog(dialog) {}

			bool eventFilter(QObject* watched, QEvent* event) override
			{
				switch (event->type())
				{
				case QEvent::MouseButtonPress:
					if (auto* me = static_cast<QMouseEvent*>(event);
						me->button() == Qt::LeftButton)
					{
						_dragging = true;
						_dragOffset = me->globalPos() - _dialog->frameGeometry().topLeft();
					}
					break;
				case QEvent::MouseMove:
					if (_dragging)
					{
						if (auto* me = static_cast<QMouseEvent*>(event);
							me->buttons() & Qt::LeftButton)
							_dialog->move(me->globalPos() - _dragOffset);
						else
							_dragging = false;
					}
					break;
				case QEvent::MouseButtonRelease:
					_dragging = false;
					break;
				default:
					break;
				}
				return QObject::eventFilter(watched, event);
			}

		private:
			QDialog* _dialog;
			bool _dragging = false;
			QPoint _dragOffset;
		};

		//"编辑字符"弹窗右下角缩放手柄贴角跟随：弹窗为栈上 QDialog 未子类化，
		//经事件过滤在 Resize 时重定位（对齐 mApplyStyleDlg 的 resizeEvent 跟随）
		class mGripCornerFilter : public QObject
		{
		public:
			explicit mGripCornerFilter(QSizeGrip* grip)
				: QObject(grip), _grip(grip) {}

			bool eventFilter(QObject* watched, QEvent* event) override
			{
				if (event->type() == QEvent::Resize && watched == _grip->parentWidget())
					_grip->move(_grip->parentWidget()->width() - _grip->width(),
						_grip->parentWidget()->height() - _grip->height());
				return QObject::eventFilter(watched, event);
			}

		private:
			QSizeGrip* _grip;
		};

		//"编辑字符"弹窗：白底无边框窗口，标题栏
		//（M徽标+标题+关闭，mEditCurveDlg 同款）+ 多行文本编辑区 + 右下
		//"取消/保存"+ 右下角缩放手柄。模态执行，返回 true 时 *out 携带编辑结果
		bool editMultilineText(const QString& initial, QString* out, QWidget* parent)
		{
			QDialog dlg(parent);
			dlg.setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
			//自样式弹窗豁免标记：本弹窗逐件自设完整设计稿 QSS（保存钮 #1385F6
			//蓝底、编辑框描边圆角等），ThemeManager 显示时整窗补发
			//（applyDialogTheme）的 setQss 是整表替换语义，会把自设样式冲成
			//主题通用形态，挂 mxSelfStyled 整窗豁免（约定名见 ThemeManager.h
			//kSelfStyledProp）。写面值字符串跟随 mFramelessMessageBox 的
			//mxBtnNoTheme 字面量先例，不为取常量新增对 Common 头的依赖
			dlg.setProperty("mxSelfStyled", true);
			//窗口细边框：白底标题栏与内容区同为浅色，加描边保证弹窗轮廓可辨识
			dlg.setObjectName("editTextDlg");
			dlg.setStyleSheet("QDialog#editTextDlg{background:#FFFFFF;border:1px solid #D9DCE0;}");
			//自样式弹窗豁免（mxSelfStyled）：本弹窗构造自带完整设计稿 QSS（保存钮
			//主蓝、关闭钮透明无边框），ThemeManager Show 时整窗重发为整体替换语义，
			//会把保存钮换成主题灰、给关闭钮加边框盒；挂标记跳过整窗补发
			//（mChannelMapDlg/mLayerManageDlg 同族先例）
			dlg.setProperty(MechMainWindow::ThemeManager::kSelfStyledProp, true);

			QVBoxLayout* outer = new QVBoxLayout(&dlg);
			outer->setContentsMargins(1, 1, 1, 1);
			outer->setSpacing(0);

			//标题栏：白底无分隔线
			QWidget* titleBar = new QWidget(&dlg);
			titleBar->setObjectName("editTextTitleBar");
			titleBar->setFixedHeight(52);
			titleBar->setStyleSheet("QWidget#editTextTitleBar{background:#FFFFFF;}");
			QHBoxLayout* titleLayout = new QHBoxLayout(titleBar);
			titleLayout->setContentsMargins(20, 0, 16, 0);
			titleLayout->setSpacing(3);
			QLabel* logoLabel = new QLabel(titleBar);
			logoLabel->setPixmap(QPixmap(":/mMainWidgets/Icon/logo.png")
				.scaled(14, 16, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
			QLabel* titleLabel = new QLabel(QStringLiteral("编辑字符"), titleBar);
			titleLabel->setStyleSheet(
				"QLabel{color:#333333; font-size:15px; font-weight:600; background:transparent;}");
			QToolButton* closeBtn = new QToolButton(titleBar);
			closeBtn->setIcon(QIcon(QStringLiteral(":/mMainWidgets/Icon/close3x.png")));
			closeBtn->setIconSize(QSize(24, 24));
			closeBtn->setFixedSize(32, 28);
			closeBtn->setStyleSheet(
				"QToolButton{background:transparent; border:none;}"
				"QToolButton:hover{ background:#EFF1F2; border-radius:4px; }");
			QObject::connect(closeBtn, &QToolButton::clicked, &dlg, &QDialog::reject);
			//标题栏可拖动：过滤器以弹窗为父对象，随弹窗销毁
			titleBar->installEventFilter(new mTitleBarDragFilter(&dlg));
			titleLayout->addWidget(logoLabel);
			titleLayout->addWidget(titleLabel, 1);
			titleLayout->addWidget(closeBtn);

			QWidget* body = new QWidget(&dlg);
			body->setStyleSheet("QWidget{background:white;}");
			QVBoxLayout* bodyLayout = new QVBoxLayout(body);
			//顶距 10px=标题栏与编辑区间隔；
			//底距 20px=按钮距窗口底边（对齐 mApplyStyleDlg）
			bodyLayout->setContentsMargins(20, 10, 20, 20);
			//bodyLayout 内只有编辑框和按钮行两行，spacing 即两者间距
			bodyLayout->setSpacing(30);
			QPlainTextEdit* editor = new QPlainTextEdit(initial, body);
			editor->setStyleSheet(
				"QPlainTextEdit{background:#FFFFFF; border:1px solid #DCDFE6; border-radius:4px;}");
			//编辑框最矮 120px（设计稿高度），弹窗拉伸时纵向随拉伸增长
			editor->setMinimumHeight(120);
			//分级约定：按行输入，第 1 行=一级标题，第 2 行=二级标题，第 3 行起
			//恒为三级标题（字号分级由渲染层按一级字体折算）
			editor->setPlaceholderText(QStringLiteral(
				"第一行为一级标题\n换行后为二级标题\n再换行起均为三级标题"));
			editor->moveCursor(QTextCursor::End);
			bodyLayout->addWidget(editor, 1);

			//底部按钮行（右下角）：取消=白底描边（mEditCurveDlg 按钮同款），
			//保存=蓝底白字（模块主蓝）
			QHBoxLayout* btnRow = new QHBoxLayout();
			btnRow->setSpacing(12);
			btnRow->addStretch(1);
			//弹窗父级 mCurveTextButton 自带 text-align:left（本按钮文字左对齐用），
			//会级联到弹窗内全部控件，按钮文字须显式 text-align:center 抵消，
			//否则两个按钮文字贴左边框
			QPushButton* cancelBtn = new QPushButton(QStringLiteral("取消"), body);
			cancelBtn->setStyleSheet(
				"QPushButton{background:#FFFFFF; border:1px solid #C9CDD4; border-radius:4px;"
				"color:#333333; padding:0px; text-align:center;}"
				"QPushButton:hover{ background:#EFF1F2; }");
			QPushButton* saveBtn = new QPushButton(QStringLiteral("保存"), body);
			//保存=模块统一主蓝 #1385F6（同各弹窗右下角主钮）
			saveBtn->setStyleSheet(
				"QPushButton{background:#1385F6; border:none; border-radius:4px;"
				"color:#FFFFFF; padding:0px; text-align:center;}"
				"QPushButton:hover{ background:#3D9EF8; }"
				"QPushButton:pressed{ background:#0F6ACB; }");
			for (QPushButton* b : { cancelBtn, saveBtn })
			{
				b->setFixedSize(44, 26);
				b->setCursor(Qt::PointingHandCursor);
			}
			QObject::connect(cancelBtn, &QPushButton::clicked, &dlg, &QDialog::reject);
			QObject::connect(saveBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
			btnRow->addWidget(cancelBtn);
			btnRow->addWidget(saveBtn);
			bodyLayout->addLayout(btnRow);

			outer->addWidget(titleBar);
			outer->addWidget(body, 1);

			//右下角缩放手柄（无边框窗口无原生缩放边框）：不占布局行——单独成行
			//会在底部多出一条手柄高度的空白（对齐 mApplyStyleDlg），改为覆盖
			//子件贴角，经 mGripCornerFilter 在弹窗 Resize 时跟随
			QSizeGrip* grip = new QSizeGrip(&dlg);
			grip->setFixedSize(grip->sizeHint());
			dlg.installEventFilter(new mGripCornerFilter(grip));

			dlg.resize(480, 284);   //默认高度 284（底部按钮距底边 20px，编辑框随拉伸增长）
			dlg.setMinimumSize(480, 284);
			//只记宽高：模态弹窗位置由系统按父窗口居中，无需记忆
			DlgGeom::bind(&dlg, "EditTextDlg", DlgGeom::Mode::SizeOnly);

			if (dlg.exec() != QDialog::Accepted)
				return false;
			*out = editor->toPlainText();
			return true;
		}
	}

	//--- mCurveColorButton ---

	mCurveColorButton::mCurveColorButton(const QColor& color, QWidget* parent)
		: QPushButton(parent)
		, _color(color)
	{
		//实心色块固定 24x24
		setFixedSize(24, 24);
		setToolTip(QStringLiteral("选择颜色"));
		setCursor(Qt::PointingHandCursor);
		connect(this, &QPushButton::clicked, this, &mCurveColorButton::onClicked);
	}

	QColor mCurveColorButton::color() const
	{
		return _color;
	}

	void mCurveColorButton::setBarMode(bool on)
	{
		if (_barMode == on)
			return;
		_barMode = on;
		if (on)
		{
			//通栏色条（三维注释页设计稿形态，B02）：解除 24x24 固定尺寸——宽度
			//随表单拉伸、高度=按钮默认行高（同行输入框，与 mCurveFontButton 同
			//形态）；整块填充+描边由 paintEvent 按 rect() 自适应（绘制实现本就
			//整块，bar 模式零改动），点击弹同款取色对话框（信号链共用）
			setMinimumSize(0, 0);
			setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
			setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		}
		else
		{
			//默认实心色块 24x24（2D 各页形态，行为不变）
			setFixedSize(24, 24);
		}
		update();
	}

	void mCurveColorButton::setColor(const QColor& color)
	{
		if (_color == color)
			return;
		_color = color;
		update();
	}

	void mCurveColorButton::onClicked()
	{
		QColor c = QColorDialog::getColor(_color, this, QStringLiteral("选择颜色"));
		if (c.isValid() && c != _color)
		{
			_color = c;
			update();
			emit colorChanged(_color);
		}
	}

	void mCurveColorButton::paintEvent(QPaintEvent* event)
	{
		Q_UNUSED(event)
		QPainter p(this);
		p.setRenderHint(QPainter::Antialiasing);
		//实心色块 + 1px 描边；hover 蓝边、禁用置灰（设计图纯色块形态，无文字无箭头）
		QColor border(0xD0, 0xD3, 0xD6);
		QColor fill = _color;
		if (!isEnabled())
		{
			border = QColor(0xE3, 0xE5, 0xE7);
			fill = QColor(0xF2, 0xF3, 0xF4);
		}
		else if (underMouse())
		{
			border = QColor(0x1E, 0x6F, 0xFF);
		}
		p.setPen(QPen(border, 1));
		p.setBrush(fill);
		p.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 2, 2);
	}

	//--- mCurveFontButton ---

	mCurveFontButton::mCurveFontButton(QWidget* parent)
		: mCurveFontButton(QFont(), parent)
	{
	}

	mCurveFontButton::mCurveFontButton(const QFont& unsetFont, QWidget* parent)
		: QPushButton(QStringLiteral("默认"), parent)
		, _unsetFont(unsetFont)
	{
		//文字左对齐、随表单拉伸（绘图选项字体按钮同款形态）
		setStyleSheet("text-align: left;");
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		setCursor(Qt::PointingHandCursor);
		connect(this, &QPushButton::clicked, this, [this, parent]()
		{
			bool ok = false;
			//初始值取当前字体：弹窗内字号/字重等与现值一致，用户只改所选项。
			//确认后整只 QFont 提交（族名+字号+字重），不存在旧字体族下拉
			//"仅换族名"的字号保留问题
			QFont picked = QFontDialog::getFont(&ok, _font, parent, QStringLiteral("选择字体"));
			if (!ok)
				return;
			applyFont(picked);
			emit fontChanged(_font);
		});
	}

	void mCurveFontButton::setFontValue(const QFont& font)
	{
		applyFont(font);
	}

	void mCurveFontButton::applyFont(const QFont& font)
	{
		_font = font;
		//与绘图选项一致：字体值等于"默认基准字体"（含族名为空的兜底）时显示
		//"默认"，按钮按环境默认字体渲染（防上一节点已选字体的显示残留）；
		//显式选择过的字体才显示族名。基准见构造参数：模型自带显式默认值的
		//（图例 Microsoft YaHei 9pt）其默认值同样按"默认"回显，模型值不变
		const bool unset = font.family().isEmpty() || font == _unsetFont;
		setText(unset ? QStringLiteral(" 默认") : font.family());
		setFont(unset ? QFont() : font);
	}

	//--- mCurveTextButton ---

	mCurveTextButton::mCurveTextButton(QWidget* parent)
		: QPushButton(parent)
	{
		//文字左对齐、随表单拉伸（字体按钮同款形态）
		setStyleSheet("text-align:left; padding-left:6px;");
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		setCursor(Qt::PointingHandCursor);
		connect(this, &QPushButton::clicked, this, [this]()
		{
			QString edited;
			//取消直接返回；保存且文本有变化才提交（同值不压撤回栈）
			if (!editMultilineText(_text, &edited, this) || edited == _text)
				return;
			applyText(edited);
			emit textCommitted(_text);
		});
	}

	void mCurveTextButton::setTextValue(const QString& text)
	{
		applyText(text);
	}

	void mCurveTextButton::applyText(const QString& text)
	{
		_text = text;
		updateDisplay();
	}

	void mCurveTextButton::updateDisplay()
	{
		//多级标题文本含换行，按钮单行回显先拍平为空格再截断；完整文本进 tooltip
		QString flat = _text;
		flat.replace(QLatin1Char('\n'), QLatin1Char(' '));
		setText(fontMetrics().elidedText(flat, Qt::ElideRight, qMax(1, width() - 12)));
		setToolTip(_text);
	}

	void mCurveTextButton::resizeEvent(QResizeEvent* event)
	{
		QPushButton::resizeEvent(event);
		//构造时宽度未定，首次布局与后续拉伸重算截断
		updateDisplay();
	}

	//--- mCurveCheckBox ---

	mCurveCheckBox::mCurveCheckBox(QWidget* parent)
		: QCheckBox(parent)
	{
		//禁用态文字色用自身样式表兜底（自身样式表优先级最高，宿主 QWidget 级
		//背景/文字规则盖不过它），保证"显示标签"等开关取消后勾选行文字变灰可见
		setStyleSheet("QCheckBox:disabled { color:#A8ABB2; }");
	}

	void mCurveCheckBox::paintEvent(QPaintEvent* event)
	{
		QCheckBox::paintEvent(event);
		if (!isChecked() || !isEnabled())
			return;
		//指示器方块（蓝底）由 FoldSection 的 QSS 画好，这里只叠加白色对勾
		QStyleOptionButton opt;
		initStyleOption(&opt);
		QRect r = style()->subElementRect(QStyle::SE_CheckBoxIndicator, &opt, this);
		if (r.width() < 8 || r.height() < 8)
			return;
		QPainter p(this);
		p.setRenderHint(QPainter::Antialiasing);
		QPen pen(Qt::white, 2);
		pen.setCapStyle(Qt::RoundCap);
		pen.setJoinStyle(Qt::RoundJoin);
		p.setPen(pen);
		QPointF a(r.left() + r.width() * 0.22, r.top() + r.height() * 0.53);
		QPointF b(r.left() + r.width() * 0.43, r.top() + r.height() * 0.74);
		QPointF c(r.left() + r.width() * 0.80, r.top() + r.height() * 0.28);
		p.drawLine(a, b);
		p.drawLine(b, c);
	}

	//--- 数字框工厂 ---

	QSpinBox* createCurveSpinBox(int min, int max, QWidget* parent)
	{
		QSpinBox* spin = new QSpinBox(parent);
		spin->setRange(min, max);
		spin->setButtonSymbols(QAbstractSpinBox::NoButtons);
		return spin;
	}

	QDoubleSpinBox* createCurveDoubleSpin(int decimals, double min, double max, double singleStep, QWidget* parent)
	{
		QDoubleSpinBox* spin = new QDoubleSpinBox(parent);
		spin->setDecimals(decimals);
		spin->setRange(min, max);
		spin->setSingleStep(singleStep);
		spin->setButtonSymbols(QAbstractSpinBox::NoButtons);
		return spin;
	}

	//--- 科学计数法数字框 ---

	mCurveSciDoubleSpin::mCurveSciDoubleSpin(int displayDecimals, double min, double max,
		double singleStep, QWidget* parent)
		: QDoubleSpinBox(parent)
		, _displayDecimals(displayDecimals)
	{
		//内部按 12 位小数存值：setValue 按 decimals 四舍五入，沿用外部显示精度
		//（4 位）会把 2e-11 这类参考值舍成 0
		setDecimals(12);
		setRange(min, max);
		setSingleStep(singleStep);
		setButtonSymbols(QAbstractSpinBox::NoButtons);
		//键入过程中不逐键提交（Qt 默认逐键发 valueChanged）：每键 commit 会触发
		//模型回显，回填的 setValue 无条件 updateEdit 把正在编辑的科学计数法文本
		//重写成定点形态（末尾删一位 "2e-12"->"2e-1" 即被改写成 "0.2000"，无法
		//继续编辑）。改走 Qt 标准的"编辑完成提交"：回车/失焦/隐藏时 interpret
		//统一解释提交（pendingEmit 保证不丢），也避免把删除途中的中间值压进撤销栈
		setKeyboardTracking(false);
	}

	//文本分两档：完整形态（小数/指数齐全，可整体求值）与半成品（正在键入
	//"2e""2e-""1."等）。完整且越界与原生数字框一致给 Intermediate——编辑结束
	//按边界纠正即可，判 Invalid 会拦得用户无法继续键入
	QValidator::State mCurveSciDoubleSpin::validate(QString& text, int& pos) const
	{
		Q_UNUSED(pos);
		static const QRegularExpression complete(
			QLatin1String("^[+-]?(\\d+(\\.\\d*)?|\\.\\d+)([eE][+-]?\\d+)?$"));
		static const QRegularExpression partial(
			QLatin1String("^[+-]?(\\d+(\\.\\d*)?|\\.\\d+)?([eE][+-]?\\d*)?$"));
		const QString trimmed = text.trimmed();
		if (complete.match(trimmed).hasMatch())
		{
			const double v = parsedValue(trimmed);
			if (qIsNaN(v))
				return QValidator::Intermediate;
			return (v >= minimum() && v <= maximum())
				? QValidator::Acceptable : QValidator::Intermediate;
		}
		return partial.match(trimmed).hasMatch()
			? QValidator::Intermediate : QValidator::Invalid;
	}

	double mCurveSciDoubleSpin::valueFromText(const QString& text) const
	{
		const double v = parsedValue(text.trimmed());
		return qIsNaN(v) ? 0.0 : v;
	}

	QString mCurveSciDoubleSpin::textFromValue(double value) const
	{
		//常规量级按显示精度定点回显（外观与舍入行为同原生数字框）；仅当非零值
		//定点后整体丢成 "0.0000"（如 2e-11 定点 4 位）时转科学计数法，尾数无效
		//零裁剪（"2.0000e-11" -> "2e-11"）
		const QLocale c = QLocale::c();
		const QString fixed = c.toString(value, 'f', _displayDecimals);
		if (value == 0.0 || c.toDouble(fixed) != 0.0)
			return fixed;
		QString sci = c.toString(value, 'e', qBound(0, _displayDecimals, 12));
		const int ePos = sci.indexOf(QLatin1Char('e'));
		if (ePos > 0)
		{
			QString mant = sci.left(ePos);
			if (mant.contains(QLatin1Char('.')))
			{
				while (mant.endsWith(QLatin1Char('0')))
					mant.chop(1);
				if (mant.endsWith(QLatin1Char('.')))
					mant.chop(1);
			}
			sci = mant + sci.mid(ePos);
		}
		return sci;
	}

	double mCurveSciDoubleSpin::parsedValue(const QString& text) const
	{
		bool ok = false;
		double v = locale().toDouble(text, &ok);   //本机语言环境优先（常规十进制）
		if (!ok)
			v = QLocale::c().toDouble(text, &ok);  //科学计数法按 C 环境兜底
		return ok ? v : qQNaN();
	}

	mCurveSciDoubleSpin* createCurveSciDoubleSpin(int displayDecimals, double min, double max,
		double singleStep, QWidget* parent)
	{
		return new mCurveSciDoubleSpin(displayDecimals, min, max, singleStep, parent);
	}

	//--- mCurvePenStyleCombo ---

	namespace
	{
		//绘图选项曲线页同款图标路径（mDrawingOptions.cpp 线型/线宽/符号三项一致）：
		//各属性页与绘图选项共用同一组 SVG，保证两处外观与语义完全对齐
		QStringList penStyleIconPaths()
		{
			return { QStringLiteral(":/mMainWidgets/Icon/NVH/实线.svg"),
				QStringLiteral(":/mMainWidgets/Icon/NVH/虚线.svg"),
				QStringLiteral(":/mMainWidgets/Icon/NVH/点划线1.svg"),
				QStringLiteral(":/mMainWidgets/Icon/NVH/点划线2.svg") };
		}

		QStringList lineWidthIconPaths()
		{
			return { QStringLiteral(":/mMainWidgets/Icon/NVH/1px.svg"),
				QStringLiteral(":/mMainWidgets/Icon/NVH/2px.svg"),
				QStringLiteral(":/mMainWidgets/Icon/NVH/3px.svg"),
				QStringLiteral(":/mMainWidgets/Icon/NVH/4px.svg") };
		}

		QStringList symbolStyleIconPaths()
		{
			//索引即模型样式值（0无 1圆 2方 3菱 4三角 5十字 6倒三角 7星(五角星)
			//8交叉 9点）：7 五角星用 Sym_Star5（米字 Star1 仅库内保留，不在下拉）
			return { QStringLiteral(":/mMainWidgets/Icon/NVH/标识无.svg"),
				QStringLiteral(":/mMainWidgets/Icon/NVH/添加.svg"),
				QStringLiteral(":/mMainWidgets/Icon/NVH/方.svg"),
				QStringLiteral(":/mMainWidgets/Icon/NVH/菱形.svg"),
				QStringLiteral(":/mMainWidgets/Icon/NVH/三角.svg"),
				QStringLiteral(":/mMainWidgets/Icon/NVH/十字.svg"),
				QStringLiteral(":/mMainWidgets/Icon/NVH/倒三角.svg"),
				QStringLiteral(":/mMainWidgets/Icon/NVH/星形.svg"),
				QStringLiteral(":/mMainWidgets/Icon/NVH/交叉.svg"),
				QStringLiteral(":/mMainWidgets/Icon/NVH/圆点.svg") };
		}

		//模板面板操作按钮图标（资源切图，与选取器工具条同用 Curve/操作 一组）：
		//0全选 1全取消 2互换（勾选反转，取反选切图）
		QIcon templateOpIcon(int kind)
		{
			static const QStringList paths = {
				QStringLiteral(":/mMainWidgets/Icon/Curve/操作/全选.svg"),
				QStringLiteral(":/mMainWidgets/Icon/Curve/操作/全不选.svg"),
				QStringLiteral(":/mMainWidgets/Icon/Curve/操作/反选.svg") };
			return QIcon(paths.value(kind));
		}
	}

	mCurvePenStyleCombo::mCurvePenStyleCombo(QWidget* parent)
		: QComboBox(parent)
	{
		//绘图选项曲线页同款线型图标（纯图标无文字），索引 0..3 即模型
		//TecLinePattern(0实线 1虚线 2点线 3点划线)
		const QStringList icons = penStyleIconPaths();
		for (int i = 0; i < icons.size(); ++i)
			addItem(QIcon(icons.at(i)), QString(), i);
	}

	int mCurvePenStyleCombo::penStyleIndex() const
	{
		return currentIndex();
	}

	void mCurvePenStyleCombo::setPenStyleIndex(int index)
	{
		if (index >= 0 && index < count())
			setCurrentIndex(index);
	}

	//--- mCurveLineWidthCombo ---

	mCurveLineWidthCombo::mCurveLineWidthCombo(QWidget* parent)
		: QComboBox(parent)
	{
		//绘图选项曲线页同款线宽样本图标，索引 0..3 = 线宽 1..4px
		const QStringList icons = lineWidthIconPaths();
		for (int i = 0; i < icons.size(); ++i)
			addItem(QIcon(icons.at(i)), QString(), i);
	}

	int mCurveLineWidthCombo::lineWidthIndex() const
	{
		return currentIndex();
	}

	void mCurveLineWidthCombo::setLineWidthIndex(int index)
	{
		if (index >= 0 && index < count())
			setCurrentIndex(index);
	}

	//--- mCurveSymbolStyleCombo ---

	mCurveSymbolStyleCombo::mCurveSymbolStyleCombo(QWidget* parent)
		: QComboBox(parent)
	{
		//绘图选项曲线页同款符号图标，索引直接对应模型 kSymbolStyle 0..9
		const QStringList icons = symbolStyleIconPaths();
		for (int i = 0; i < icons.size(); ++i)
			addItem(QIcon(icons.at(i)), QString(), i);
	}

	int mCurveSymbolStyleCombo::symbolStyleIndex() const
	{
		return currentIndex();
	}

	void mCurveSymbolStyleCombo::setSymbolStyleIndex(int index)
	{
		if (index >= 0 && index < count())
			setCurrentIndex(index);
	}

	//--- mCurveBorderStyleCombo（线型语义：图表编辑器"边框样式"行） ---

	mCurveBorderStyleCombo::mCurveBorderStyleCombo(QWidget* parent)
		: QComboBox(parent)
	{
		//0=无边框（"无"标识切图：圆圈斜杠），1..4 用线型图标（实线/虚线/划线/
		//点划线），索引直接对应 CurveLegendData::borderStyle 与引擎 setBorderStyle
		addItem(QIcon(QStringLiteral(":/mMainWidgets/Icon/NVH/标识无.svg")), QString(), 0);

		const QStringList icons = penStyleIconPaths();
		for (int i = 0; i < icons.size(); ++i)
			addItem(QIcon(icons.at(i)), QString(), i + 1);
	}

	int mCurveBorderStyleCombo::borderStyleIndex() const
	{
		return currentIndex();
	}

	void mCurveBorderStyleCombo::setBorderStyleIndex(int index)
	{
		if (index >= 0 && index < count())
			setCurrentIndex(index);
	}

	//--- mCurveBorderWidthCombo（宽度语义：图例"边框线宽"行） ---

	mCurveBorderWidthCombo::mCurveBorderWidthCombo(QWidget* parent)
		: QComboBox(parent)
	{
		//图例边框线宽选择：索引/值即线宽 px（0=无边框，"无"标识切图：圆圈斜杠；
		//1..4 用绘图选项曲线页同款线宽样本图标），提交 kLegendBorderWidth
		addItem(QIcon(QStringLiteral(":/mMainWidgets/Icon/NVH/标识无.svg")), QString(), 0);

		const QStringList icons = lineWidthIconPaths();
		for (int i = 0; i < icons.size(); ++i)
			addItem(QIcon(icons.at(i)), QString(), i + 1);
	}

	int mCurveBorderWidthCombo::borderWidthIndex() const
	{
		return currentIndex();
	}

	void mCurveBorderWidthCombo::setBorderWidthIndex(int index)
	{
		if (index >= 0 && index < count())
			setCurrentIndex(index);
	}

	//--- 曲线标签模板变量与 mCurveTemplateCombo ---

	void curveLabelVariables(QStringList* names, QStringList* snippets)
	{
		//对齐参考软件标签变量清单（截图03/属性编辑器）：X/Y 侧来源信息 + 计算值。
		//占位符命名沿用参考文本先例（{y.MXRequest}/{y.MXComponent}/{y.MXDatatype}/
		//Max={max(y)}），Type 对应结果类型（MXDatatype）
		if (names)
			*names = QStringList {
				QStringLiteral("X File Path"), QStringLiteral("X File Name"),
				QStringLiteral("X Subcase"), QStringLiteral("X Type"),
				QStringLiteral("X Request"), QStringLiteral("X Component"),
				QStringLiteral("Y File Path"), QStringLiteral("Y File Name"),
				QStringLiteral("Y Subcase"), QStringLiteral("Y Type"),
				QStringLiteral("Y Request"), QStringLiteral("Y Component"),
				QStringLiteral("Max Y"), QStringLiteral("At X") };
		if (snippets)
			*snippets = QStringList {
				QStringLiteral("{x.MXFilePath}"), QStringLiteral("{x.MXFileName}"),
				QStringLiteral("{x.MXSubcase}"), QStringLiteral("{x.MXDatatype}"),
				QStringLiteral("{x.MXRequest}"), QStringLiteral("{x.MXComponent}"),
				QStringLiteral("{y.MXFilePath}"), QStringLiteral("{y.MXFileName}"),
				QStringLiteral("{y.MXSubcase}"), QStringLiteral("{y.MXDatatype}"),
				QStringLiteral("{y.MXRequest}"), QStringLiteral("{y.MXComponent}"),
				QStringLiteral("Max={max(y)}"), QStringLiteral("At X={x}") };
	}

	void setupChartTypeCombo(QComboBox* combo)
	{
		if (!combo)
			return;
		combo->clear();
		combo->addItems(chartTypeDisplayNames());
		//0 XY曲线图 / 1 柱状图 / 2 复数图 / 3 极坐标图 已接渲染（极坐标自绘
		//item 族 §5）；蛛网（4）引擎暂无绘制能力，置灰占位
		for (int i = 4; i < combo->count(); ++i)
		{
			QStandardItem* it = qobject_cast<QStandardItemModel*>(combo->model())
				->item(i);
			if (!it)
				continue;
			it->setEnabled(false);
			it->setToolTip(QStringLiteral("图表类型待引擎支持，暂不可选"));
		}
	}

	void applyChartTypeSwitch(QComboBox* combo, int chartNodeId, int idx)
	{
		if (!combo)
			return;
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		if (!doc || !doc->node(chartNodeId))
			return;
		const int modelIdx = doc->nodeProperty(chartNodeId, CurveProp::kChartType).toInt();
		//R30：三维图（chartType=5）不参与类型切换，入口即拒——切出会破三维图固定
		//骨架/默认名（reconcile 按 XY profile 补建 X1/Y1/Header/Footer 并改名
		//"XY Plot"，即 R30 首建毒化同款形态）；切入被 R29 setNodeProperty 守卫
		//静默拒但下方确认框/曲线清理已先行=数据丢失路径，一并封死。idx==modelIdx
		//（含 5→5 回显）与既有回显短路同口径直接放行返回
		if (modelIdx == 5 || idx == 5)
		{
			if (idx != modelIdx)
			{
				//拒绝：下拉回退为模型值（屏蔽信号，无确认框、无曲线清理、零提交）
				QSignalBlocker blocker(combo);
				combo->setCurrentIndex(modelIdx);
			}
			return;
		}
		//下拉与模型同值=程序回填（切节点回显/另一视图改型驱动 reload），非用户
		//切换，直接放行；取消回退的 setCurrentIndex 也因此不会再进入本逻辑
		if (idx == modelIdx)
			return;
		//空图表切换不清任何数据，不打扰；挂有曲线才提示（截图流程②）
		bool hasCurve = false;
		for (int childId : doc->childNodeIds(chartNodeId))
		{
			CurveNode* n = doc->node(childId);
			if (n && n->type == CurveNodeType::Curve)
			{
				hasCurve = true;
				break;
			}
		}
		if (hasCurve && modelIdx == 0 && idx == 1)
		{
			//XY→柱状（原型截图流程②）：走"转换设置"提取数值框——均匀取 N 点
			//含首尾转换替换（一次撤销整批还原），不清曲线不丢数据；取消回退。
			//仅本组合走新框；其余类型切换维持下方确认框+清空（design 裁定③）
			bool ok = false;
			const int n = MechMainWindow::mFramelessMessageBox::getConvertCount(combo,
				QStringLiteral("转换设置"), QStringLiteral("设置提取数值（个）"),
				5, 2, 999, &ok);
			if (!ok)
			{
				//取消：下拉回退为模型值（屏蔽信号，不提交）
				QSignalBlocker blocker(combo);
				combo->setCurrentIndex(modelIdx);
				return;
			}
			mCurveDocController::getInstance()->convertCurvesToBar(chartNodeId, n);
			return;
		}
		if (hasCurve)
		{
			//"提示"确认框（原型文案两行，句号处换行）：默认键"继续"（主按钮形态），Esc=取消；
			//原型三段式形态只给本调用点（尾参 true，2026-11-05 用户圈定范围，
			//其余全工程提示框保持通用形态）
			const int btn = MechMainWindow::mFramelessMessageBox::warning(combo, QStringLiteral("提示"),
				QStringLiteral("此操作将清除当前窗口中的数据。\n您希望继续吗？"),
				QStringLiteral("取消"), QStringLiteral("继续"), QString(), 1, 0, true);
			if (btn != 1)
			{
				//取消：下拉回退为模型值（屏蔽信号，不提交）
				QSignalBlocker blocker(combo);
				combo->setCurrentIndex(modelIdx);
				return;
			}
		}
		//继续（截图流程③）：类型切换与清空全部曲线子节点同一宏命令提交，
		//Ctrl+Z 整批恢复；轴/图例/页眉等其余子节点保留，只清"曲线部分"
		mCurveDocController* ctrl = mCurveDocController::getInstance();
		QList<int> curveIds;
		for (int childId : doc->childNodeIds(chartNodeId))
		{
			if (doc->nodeType(childId) == CurveNodeType::Curve)
				curveIds.append(childId);
		}
		if (curveIds.isEmpty())
		{
			ctrl->setNodeProperty(chartNodeId, CurveProp::kChartType, idx);
			return;
		}
		ctrl->undoStack()->beginMacro(QStringLiteral("切换图表类型"));
		ctrl->setNodeProperty(chartNodeId, CurveProp::kChartType, idx);
		for (int curveId : curveIds)
			ctrl->removeNode(curveId);
		ctrl->undoStack()->endMacro();
	}

	mCurveTemplateCombo::mCurveTemplateCombo(QWidget* parent)
		: QComboBox(parent)
	{
		//箭头前编辑区可直接输入（参考软件"双击输入"形态——可编辑下拉点击即聚焦）
		setEditable(true);
		//禁用态用自身样式表兜底：自身样式表优先级最高（参数栏宿主/属性编辑器 body 的
		//QWidget{background:white} 会盖过祖先层与应用级的 :disabled 规则），
		//保证取消"显示标签"后本下拉置灰可见（灰底灰字，与应用禁用配色一致）
		setStyleSheet(
			"QComboBox:disabled { background-color:#F5F7FA; color:#A8ABB2; border-color:#E4E7ED; }"
			"QComboBox:disabled QLineEdit { background-color:#F5F7FA; color:#A8ABB2; }");
		_panel = new QFrame(this, Qt::Popup | Qt::FramelessWindowHint);
		_panel->setStyleSheet(
			"QFrame { background: white; border: 1px solid #c0c0c0; }"
			"QListWidget { border: 1px solid #E4E7ED; background: white; }"
			"QListWidget::item { padding: 2px 6px; }"
			"QListWidget::item:hover { background: #d0e6ff; }");
		QVBoxLayout* lay = new QVBoxLayout(_panel);
		lay->setContentsMargins(4, 4, 4, 4);
		lay->setSpacing(4);

		//顶部操作行：全选/全取消/互换（参考软件下拉面板形态，图标按钮）
		QWidget* btnRow = new QWidget(_panel);
		QHBoxLayout* btnLay = new QHBoxLayout(btnRow);
		btnLay->setContentsMargins(0, 0, 0, 0);
		btnLay->setSpacing(4);
		const struct { const char* tip; int kind; void (mCurveTemplateCombo::*fn)(); } ops[] = {
			{ "全选",  0, &mCurveTemplateCombo::checkAll },
			{ "全取消", 1, &mCurveTemplateCombo::checkNone },
			{ "互换",  2, &mCurveTemplateCombo::checkInvert },
		};
		for (const auto& op : ops)
		{
			QToolButton* btn = new QToolButton(btnRow);
			btn->setIcon(templateOpIcon(op.kind));
			btn->setIconSize(QSize(16, 16));
			btn->setAutoRaise(true);
			btn->setFixedSize(22, 20);
			btn->setToolTip(QString::fromUtf8(op.tip));
			connect(btn, &QToolButton::clicked, this, op.fn);
			btnLay->addWidget(btn);
		}
		btnLay->addStretch(1);
		lay->addWidget(btnRow);

		_list = new QListWidget(_panel);
		lay->addWidget(_list);
		//仅勾选框切换驱动拼装（QListWidget 勾选态变化走 itemChanged）
		connect(_list, &QListWidget::itemChanged, this, [this](QListWidgetItem*)
		{
			if (!_syncing)
				rebuildText();
		});
		//编辑区手输提交：编辑完成（回车/失焦）即按文本回设勾选态并上报
		if (lineEdit())
		{
			connect(lineEdit(), &QLineEdit::editingFinished, this, [this]()
			{
				syncCheckedFromText(templateText());
				emit templateTextChanged(templateText());
			});
		}
		QStringList names, snippets;
		curveLabelVariables(&names, &snippets);
		setVariables(names, snippets);
	}

	void mCurveTemplateCombo::setVariables(const QStringList& names, const QStringList& snippets)
	{
		_syncing = true;
		_list->clear();
		const int n = qMin(names.size(), snippets.size());
		for (int i = 0; i < n; ++i)
		{
			QListWidgetItem* it = new QListWidgetItem(names.at(i), _list);
			it->setFlags(it->flags() | Qt::ItemIsUserCheckable);
			it->setCheckState(Qt::Unchecked);
			it->setData(Qt::UserRole, snippets.at(i));
		}
		_syncing = false;
	}

	QString mCurveTemplateCombo::templateText() const
	{
		return lineEdit() ? lineEdit()->text() : QString();
	}

	void mCurveTemplateCombo::setTemplateText(const QString& text)
	{
		if (lineEdit())
			lineEdit()->setText(text);
		syncCheckedFromText(text);
	}

	void mCurveTemplateCombo::showPopup()
	{
		if (!_panel || _list->count() == 0)
			return;
		//面板落位本控件正下方：宽取两者较大值，高度=操作行(约24) + 条目收敛（超长出滚动条）
		const QPoint g = mapToGlobal(QPoint(0, height()));
		const int rowH = _list->sizeHintForRow(0);
		int h = 244;
		if (rowH > 0)
			h = qMin(_list->count(), 10) * rowH + 50;
		_panel->setGeometry(g.x(), g.y(), qMax(width(), _list->sizeHintForColumn(0) + 56), h);
		_panel->show();
		_panel->raise();
	}

	void mCurveTemplateCombo::rebuildText()
	{
		QStringList parts;
		for (int i = 0; i < _list->count(); ++i)
		{
			QListWidgetItem* it = _list->item(i);
			if (it->checkState() == Qt::Checked)
				parts << it->data(Qt::UserRole).toString();
		}
		const QString text = parts.join(QStringLiteral("~"));
		if (lineEdit())
			lineEdit()->setText(text);
		emit templateTextChanged(text);
	}

	void mCurveTemplateCombo::syncCheckedFromText(const QString& text)
	{
		_syncing = true;
		for (int i = 0; i < _list->count(); ++i)
		{
			QListWidgetItem* it = _list->item(i);
			const bool on = !text.isEmpty() && text.contains(it->data(Qt::UserRole).toString());
			it->setCheckState(on ? Qt::Checked : Qt::Unchecked);
		}
		_syncing = false;
	}

	void mCurveTemplateCombo::setItemsChecked(Qt::CheckState state)
	{
		_syncing = true;
		for (int i = 0; i < _list->count(); ++i)
			_list->item(i)->setCheckState(state);
		_syncing = false;
		rebuildText();
	}

	void mCurveTemplateCombo::checkAll()
	{
		setItemsChecked(Qt::Checked);
	}

	void mCurveTemplateCombo::checkNone()
	{
		setItemsChecked(Qt::Unchecked);
	}

	void mCurveTemplateCombo::checkInvert()
	{
		_syncing = true;
		for (int i = 0; i < _list->count(); ++i)
		{
			QListWidgetItem* it = _list->item(i);
			it->setCheckState(it->checkState() == Qt::Checked ? Qt::Unchecked : Qt::Checked);
		}
		_syncing = false;
		rebuildText();
	}
}
