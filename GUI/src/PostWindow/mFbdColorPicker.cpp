#include "mFbdColorPicker.h"
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QPainter>
#include <QMouseEvent>
#include <QScreen>
#include <QTimer>
#include <QGuiApplication>
#include <functional>

namespace PostWindow
{
	//SV方阵：横向白→当前色相、纵向透明→黑；拖动圆点取饱和度/明度（纯绘制控件，无moc需求）
	class FbdSvSquare : public QWidget
	{
	public:
		explicit FbdSvSquare(QWidget* parent) : QWidget(parent) { setCursor(Qt::CrossCursor); }
		void setHue(int h) { if (_h != h) { _h = h; update(); } }
		void setColor(const QColor& c)
		{
			int h = 0, sat = 0, val = 0;
			c.getHsv(&h, &sat, &val);
			if (_h != h || _s != sat || _v != val) { _h = h; _s = sat; _v = val; update(); }
		}
		QColor color() const { return QColor::fromHsv(_h, _s, _v); }
		std::function<void()> changed;
	protected:
		void paintEvent(QPaintEvent*) override
		{
			QPainter pt(this);
			const QRect r = rect();
			QLinearGradient hg(0, 0, r.width(), 0);
			hg.setColorAt(0, Qt::white);
			hg.setColorAt(1, QColor::fromHsv(_h, 255, 255));
			pt.fillRect(r, hg);
			QLinearGradient vg(0, 0, 0, r.height());
			vg.setColorAt(0, Qt::transparent);
			vg.setColorAt(1, Qt::black);
			pt.fillRect(r, vg);
			const int x = _s * (r.width() - 1) / 255;
			const int y = (255 - _v) * (r.height() - 1) / 255;
			pt.setRenderHint(QPainter::Antialiasing);
			if (_dragging)
				pt.setPen(QPen(Qt::black, 2));	//选中/拖动时绘制黑色圆框
			else
				pt.setPen(QPen(Qt::white, 2));	//未选中：原白色圆框
			pt.setBrush(Qt::NoBrush);
			pt.drawEllipse(QPoint(x, y), 5, 5);
			if (!_dragging)
			{
				pt.setPen(QPen(QColor(0, 0, 0, 110), 1));	//未选中：外侧细描边
				pt.drawEllipse(QPoint(x, y), 6, 6);
			}
		}
		void mousePressEvent(QMouseEvent* e) override { grabMouse(); _dragging = true; pick(e->pos()); }
		void mouseMoveEvent(QMouseEvent* e) override { if (e->buttons() & Qt::LeftButton) pick(e->pos()); }
		void mouseReleaseEvent(QMouseEvent*) override { releaseMouse(); _dragging = false; update(); }
	private:
		void pick(const QPoint& pos)
		{
			const QRect r = rect();
			_s = qBound(0, pos.x() * 255 / qMax(1, r.width() - 1), 255);
			_v = 255 - qBound(0, pos.y() * 255 / qMax(1, r.height() - 1), 255);
			update();
			if (changed) changed();
		}
		int _h = 0, _s = 255, _v = 255;
		bool _dragging = false;
	};

	//色相条：横向彩虹渐变，拖动取色相
	class FbdHueBar : public QWidget
	{
	public:
		explicit FbdHueBar(QWidget* parent) : QWidget(parent) { setCursor(Qt::CrossCursor); }
		int hue() const { return _h; }
		void setHue(int h) { h = ((h % 360) + 360) % 360; if (_h != h) { _h = h; update(); } }
		std::function<void()> changed;
	protected:
		void paintEvent(QPaintEvent*) override
		{
			QPainter pt(this);
			const QRect r = rect();
			QLinearGradient g(0, 0, r.width(), 0);
			for (int i = 0; i <= 6; ++i)
				g.setColorAt(i / 6.0, QColor::fromHsv(i * 60 % 360, 255, 255));
			pt.fillRect(r, g);
			const int x = _h * (r.width() - 1) / 359;
			pt.setRenderHint(QPainter::Antialiasing);
			if (_dragging)
				pt.setPen(QPen(Qt::black, 2));	//选中/拖动时绘制黑色圆框
			else
				pt.setPen(QPen(Qt::white, 2));	//未选中：原白色圆框
			pt.setBrush(Qt::NoBrush);
			pt.drawEllipse(QPoint(x, r.height() / 2), 6, 6);
			if (!_dragging)
			{
				pt.setPen(QPen(QColor(0, 0, 0, 110), 1));	//未选中：外侧细描边
				pt.drawEllipse(QPoint(x, r.height() / 2), 7, 7);
			}
		}
		void mousePressEvent(QMouseEvent* e) override { grabMouse(); _dragging = true; pick(e->pos().x()); }
		void mouseMoveEvent(QMouseEvent* e) override { if (e->buttons() & Qt::LeftButton) pick(e->pos().x()); }
		void mouseReleaseEvent(QMouseEvent*) override { releaseMouse(); _dragging = false; update(); }
	private:
		void pick(int x)
		{
			_h = qBound(0, x * 359 / qMax(1, width() - 1), 359);
			update();
			if (changed) changed();
		}
		int _h = 0;
		bool _dragging = false;
	};

	//放大镜：跟随光标的圆形网格放大镜（数据源为取色模式开始时的屏幕快照，不响应鼠标）
	//快照一次性转为QImage缓存：全屏toImage每帧执行会拖垮绘制线程（表现为放大内容"冻结"）
	class FbdMagnifier : public QWidget
	{
	public:
		explicit FbdMagnifier(const QPixmap& screenPix)
			: QWidget(nullptr, Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
		{
			setAttribute(Qt::WA_TransparentForMouseEvents);
			setAttribute(Qt::WA_TranslucentBackground);
			setFixedSize(140, 140);
			setScreenPix(screenPix);
		}
		void setScreenPix(const QPixmap& pix)
		{
			_img = pix.toImage();
			QScreen* scr = QGuiApplication::screenAt(QCursor::pos());
			const QRect sg = scr ? scr->geometry() : QRect();
			//多屏时全局原点非零（副屏/负坐标屏）：必须先减原点得屏内局部坐标再映射物理像素，
			//否则整体越界被qBound钳制在图像边缘，源区域不再随光标变化（放大内容"冻结"）
			_ox = sg.x();
			_oy = sg.y();
			_sx = sg.isValid() ? qreal(_img.width()) / qMax(1, sg.width()) : 1.0;
			_sy = sg.isValid() ? qreal(_img.height()) / qMax(1, sg.height()) : 1.0;
			update();
		}
		void follow(const QPoint& globalPos)
		{
			move(globalPos - QPoint(width() / 2, height() / 2));
			//必须同步repaint：半透明顶层窗口连续move期间update()的异步刷新可能被合并/丢弃，
			//Windows移动分层窗口时只拖动上一帧位图（表现为放大内容"冻结"在旧画面上滑行）
			repaint();
		}
	protected:
		void paintEvent(QPaintEvent*) override
		{
			const qreal radius = 70;
			const qreal zoom = 10;	//每个屏幕像素在放大镜中的边长
			const QPointF cur = QCursor::pos();	//全局逻辑坐标
			const QPointF c(width() / 2.0, height() / 2.0);
			const int half = qRound(radius / zoom);	//源像素半径
			const QRect src(qBound(0, qRound((cur.x() - _ox) * _sx) - half, qMax(0, _img.width() - half * 2)),
				qBound(0, qRound((cur.y() - _oy) * _sy) - half, qMax(0, _img.height() - half * 2)),
				half * 2, half * 2);
			QPainter pt(this);
			pt.save();
			pt.setRenderHint(QPainter::Antialiasing);
			QPainterPath clip;
			clip.addEllipse(c, radius, radius);
			pt.setClipPath(clip);
			pt.drawImage(QRectF(0, 0, width(), height()), _img, src);
			pt.setPen(QColor(0, 0, 0, 160));	//内部网格：黑色
			for (int i = 0; i <= half * 2; ++i)
			{
				pt.drawLine(QPointF(i * zoom, 0), QPointF(i * zoom, height()));
				pt.drawLine(QPointF(0, i * zoom), QPointF(width(), i * zoom));
			}
			pt.restore();
			pt.setRenderHint(QPainter::Antialiasing);
			pt.setPen(QPen(Qt::black, 2));	//外边框：黑色
			pt.setBrush(Qt::NoBrush);
			pt.drawEllipse(c, radius, radius);
			pt.setPen(QPen(Qt::black, 1));
			pt.drawRect(QRectF(c.x() - zoom / 2, c.y() - zoom / 2, zoom, zoom));	//中心待选像素
		}
	private:
		QImage _img;
		qreal _sx = 1.0, _sy = 1.0;
		qreal _ox = 0.0, _oy = 0.0;	//快照所属屏的全局原点（多屏非零）
	};

	FbdColorPicker::FbdColorPicker(QWidget* parent)
		: QWidget(parent)
	{
		//样式必须带 ID 选择器：无选择器规则会级联到全部子部件
		setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
		setObjectName("fbdColorPicker");
		setAttribute(Qt::WA_StyledBackground, true);
		setStyleSheet("#fbdColorPicker{background:#FFFFFF;border:1px solid #DCE2E9;border-radius:3px;}");
		setFixedSize(235, 250);

		auto* popupLayout = new QVBoxLayout(this);
		//颜色板满宽顶边（无上/左/右间距），其余行自带左右边距
		popupLayout->setContentsMargins(0, 0, 0, 12);
		popupLayout->setSpacing(8);

		_sv = new FbdSvSquare(this);
		_sv->setFixedHeight(140);
		popupLayout->addWidget(_sv);

		//吸管（进入屏幕取色）/当前颜色预览圆/色相条
		QWidget* hueRow = new QWidget(this);
		QHBoxLayout* hueLayout = new QHBoxLayout(hueRow);
		hueLayout->setContentsMargins(12, 0, 12, 0);
		hueLayout->setSpacing(8);
		_eyedropper = new QPushButton(hueRow);
		_eyedropper->setFixedSize(20, 20);
		_eyedropper->setToolTip(QString::fromUtf8("拾取屏幕颜色"));
		_eyedropper->setStyleSheet("QPushButton{border:0;background:transparent;}"
			"QPushButton:hover{background:#F0F7FF;border-radius:2px;}");
		hueLayout->addWidget(_eyedropper);
		_preview = new QLabel(hueRow);
		_preview->setFixedSize(30, 30);
		_preview->setStyleSheet("background:#FFFFFF;border:1px solid #DCE2E9;border-radius:15px;");
		hueLayout->addWidget(_preview);
		hueLayout->addStretch();
		_hue = new FbdHueBar(hueRow);
		_hue->setFixedHeight(12);
		hueLayout->addWidget(_hue, 1);
		popupLayout->addWidget(hueRow);

		//分量输入行（RGB/HSL共三格；HEX时单格显示）
		QWidget* valRow = new QWidget(this);
		QHBoxLayout* valLayout = new QHBoxLayout(valRow);
		valLayout->setContentsMargins(12, 0, 12, 0);
		valLayout->setSpacing(8);
		const std::initializer_list<QLineEdit**> chanEdits = { &_r, &_g, &_b };
		for (QLineEdit** edPtr : chanEdits)
		{
			QLineEdit* ed = new QLineEdit(valRow);
			ed->setFixedHeight(26);
			ed->setAlignment(Qt::AlignCenter);
			*edPtr = ed;
			valLayout->addWidget(ed, 1);
			connect(ed, &QLineEdit::editingFinished, this, [this]() { slotColorEditorsChanged(); });
		}
		_hex = new QLineEdit(valRow);
		_hex->setFixedHeight(26);
		_hex->setAlignment(Qt::AlignCenter);
		_hex->hide();
		valLayout->addWidget(_hex, 1);
		connect(_hex, &QLineEdit::editingFinished, this, [this]() { slotColorEditorsChanged(); });
		//右侧占位与通道名行的切换按钮等宽（保证三格输入与通道名纵向对齐）
		QWidget* valPad = new QWidget(valRow);
		valPad->setFixedWidth(28);
		valLayout->addWidget(valPad);
		popupLayout->addWidget(valRow);

		//通道名行 + 格式循环切换（RGB/HSL/HEX；真实图标资源到位前暂用箭头字符）
		QWidget* labRow = new QWidget(this);
		QHBoxLayout* labLayout = new QHBoxLayout(labRow);
		labLayout->setContentsMargins(12, 0, 12, 0);
		labLayout->setSpacing(8);
		const std::initializer_list<QPair<QLabel**, const char*>> chanLabs =
			{ { &_lblR, "R" }, { &_lblG, "G" }, { &_lblB, "B" } };
		for (const auto& lb : chanLabs)
		{
			QLabel* lab = new QLabel(QString::fromLatin1(lb.second), labRow);
			lab->setAlignment(Qt::AlignCenter);
			lab->setStyleSheet("QLabel{color:#788391;background:transparent;font-size:11px;border:0;}");
			*lb.first = lab;
			labLayout->addWidget(lab, 1);
		}
		_hexLabel = new QLabel(QString::fromUtf8("HEX"), labRow);
		_hexLabel->setAlignment(Qt::AlignCenter);
		_hexLabel->setStyleSheet("QLabel{color:#788391;background:transparent;font-size:11px;border:0;}");
		_hexLabel->hide();
		labLayout->addWidget(_hexLabel, 1);
		labLayout->addStretch();
		_modeBtn = new QPushButton(labRow);
		_modeBtn->setText(QString::fromUtf8("⇅"));
		_modeBtn->setFixedSize(28, 16);
		_modeBtn->setStyleSheet("QPushButton{border:0;background:transparent;color:#788391;font-size:11px;}"
			"QPushButton:hover{background:#F0F7FF;border-radius:2px;}");
		labLayout->addWidget(_modeBtn);
		popupLayout->addWidget(labRow);
		popupLayout->addStretch();

		//取色模式光标轮询：悬停在未开启mouseTracking的部件上时无MouseMove事件，定时器保证放大镜持续跟随
		_pickTimer = new QTimer(this);
		_pickTimer->setInterval(16);
		connect(_pickTimer, &QTimer::timeout, this, [this]() {
			if (!_picking || !_magnifier)
				return;
			//快照仅覆盖单屏：光标跨屏时重取该屏快照（抓取期间隐藏放大镜，避免自身入镜形成自反馈）
			QScreen* scr = QGuiApplication::screenAt(QCursor::pos());
			if (scr && scr != _snapScreen)
			{
				_snapScreen = scr;
				_magnifier->hide();
				_magnifier->setScreenPix(scr->grabWindow(0));
				_magnifier->show();
				_magnifier->raise();
			}
			_magnifier->follow(QCursor::pos());
			});

		//SV/色相变化 → 应用颜色并回填分量输入
		_sv->changed = [this]() { applyColor(_sv->color()); };
		_hue->changed = [this]() {
			_sv->setHue(_hue->hue());
			applyColor(_sv->color());
			};
		connect(_eyedropper, &QPushButton::clicked, this, [this]() { startScreenPick(); });
		connect(_modeBtn, &QPushButton::clicked, this, [this]() {
			_mode = (_mode + 1) % 3;
			applyColorMode();
			});
		applyColorMode();
	}

	QColor FbdColorPicker::currentColor() const
	{
		return _cur;
	}

	void FbdColorPicker::setCurrentColor(const QColor& color)
	{
		if (!color.isValid())
			return;
		_cur = color;
		_sv->setColor(color);
		_hue->setHue(color.hue());
		refreshColorEditors();
		if (_preview)
			_preview->setStyleSheet(QString("background:%1;border:1px solid #DCE2E9;border-radius:15px;")
				.arg(color.name()));
	}

	void FbdColorPicker::popupAt(QPoint pos)
	{
		QScreen* scr = QGuiApplication::screenAt(pos);
		const QRect avail = (scr ? scr : QGuiApplication::primaryScreen())->availableGeometry();
		if (pos.x() + width() > avail.right())
			pos.setX(avail.right() - width() - 4);
		if (pos.x() < avail.left())
			pos.setX(avail.left() + 4);
		pos.setY(qBound(avail.top(), pos.y(), qMax(avail.top(), avail.bottom() - height())));
		move(pos);
		show();
	}

	void FbdColorPicker::applyColorMode()
	{
		const bool hex = _mode == 2;
		_r->setVisible(!hex);
		_g->setVisible(!hex);
		_b->setVisible(!hex);
		_hex->setVisible(hex);
		_lblR->setVisible(!hex);
		_lblG->setVisible(!hex);
		_lblB->setVisible(!hex);
		_hexLabel->setVisible(hex);
		_lblR->setText(_mode == 1 ? QString::fromLatin1("H") : QString::fromLatin1("R"));
		_lblG->setText(_mode == 1 ? QString::fromLatin1("S") : QString::fromLatin1("G"));
		_lblB->setText(_mode == 1 ? QString::fromLatin1("L") : QString::fromLatin1("B"));
		refreshColorEditors();
	}

	void FbdColorPicker::refreshColorEditors()
	{
		const QColor c = _cur;
		if (_mode == 1)
		{
			int h = 0, s = 0, l = 0;
			c.getHsl(&h, &s, &l);
			_r->setText(QString::number(h));
			_g->setText(QString::number(qRound(s * 100 / 255.0)) + QString::fromUtf8("%"));
			_b->setText(QString::number(qRound(l * 100 / 255.0)) + QString::fromUtf8("%"));
		}
		else if (_mode == 2)
		{
			_hex->setText(c.name().toUpper());
		}
		else
		{
			_r->setText(QString::number(c.red()));
			_g->setText(QString::number(c.green()));
			_b->setText(QString::number(c.blue()));
		}
	}

	void FbdColorPicker::applyColor(const QColor& color)
	{
		if (!color.isValid())
			return;
		_cur = color;
		//同步SV方阵与色相条（吸管取色等外部来源；源自方阵/色相条时为无操作）
		_sv->setColor(color);
		_hue->setHue(color.hue());
		refreshColorEditors();
		_preview->setStyleSheet(QString("background:%1;border:1px solid #DCE2E9;border-radius:15px;")
			.arg(color.name()));
		emit colorChanged(_cur);
	}

	void FbdColorPicker::hideEvent(QHideEvent* event)
	{
		QWidget::hideEvent(event);
		//取色模式下弹窗被外部隐藏（如切换应用激活导致popup关闭）：必须同步退出取色，
		//否则_picking残留、放大镜永久跟随且事件过滤器持续消费全局点击
		if (_picking)
			endScreenPick();
	}

	void FbdColorPicker::startScreenPick()
	{
		//取色模式：弹窗保持原样（不隐藏、不改窗口标志），放大镜小窗跟随光标；
		//事件过滤器接管全局鼠标/按键：左键取色、右键取消、Esc取消
		QScreen* scr = QGuiApplication::screenAt(QCursor::pos());
		if (!scr)
			scr = QGuiApplication::primaryScreen();
		_screenPix = scr->grabWindow(0);	//快照含弹窗影像，但真实弹窗置顶其上
		_snapScreen = scr;
		_picking = true;
		if (!_magnifier)
			_magnifier = new FbdMagnifier(_screenPix);
		else
			_magnifier->setScreenPix(_screenPix);
		_magnifier->follow(QCursor::pos());
		_magnifier->show();
		_magnifier->raise();
		_pickTimer->start();
		qApp->installEventFilter(this);
	}

	void FbdColorPicker::endScreenPick()
	{
		_picking = false;
		_pickTimer->stop();
		qApp->removeEventFilter(this);
		if (_magnifier)
		{
			_magnifier->deleteLater();
			_magnifier = nullptr;
		}
	}

	QColor FbdColorPicker::colorAtScreen(const QPoint& globalPos) const
	{
		//点击确认时实时抓取光标所在屏（起始快照仅覆盖进模式时的那一块屏，跨屏后不再适用）；
		//与放大镜同样必须先减该屏全局原点再映射物理像素，否则副屏上取到的是钳制后的边缘错误像素
		QScreen* scr = QGuiApplication::screenAt(globalPos);
		const QRect sg = scr ? scr->geometry() : QRect();
		if (!sg.isValid())
			return QColor();
		const QImage img = scr->grabWindow(0).toImage();
		const qreal sx = qreal(img.width()) / qMax(1, sg.width());
		const qreal sy = qreal(img.height()) / qMax(1, sg.height());
		return img.pixelColor(qBound(0, qRound((globalPos.x() - sg.x()) * sx), img.width() - 1),
			qBound(0, qRound((globalPos.y() - sg.y()) * sy), img.height() - 1));
	}

	bool FbdColorPicker::eventFilter(QObject* watched, QEvent* event)
	{
		if (_picking)
		{
			switch (event->type())
			{
			case QEvent::MouseMove:
				if (_magnifier)
					_magnifier->follow(QCursor::pos());
				break;
			case QEvent::MouseButtonPress:
			{
				//点击弹窗（含颜色部分）：结束取色模式，放大镜关闭，事件继续交给弹窗子控件正常交互
				const QPoint gp = static_cast<QMouseEvent*>(event)->globalPos();
				if (geometry().contains(gp))
				{
					endScreenPick();
					break;
				}
				if (static_cast<QMouseEvent*>(event)->button() == Qt::LeftButton)
				{
					const QColor c = colorAtScreen(QCursor::pos());
					endScreenPick();
					if (c.isValid())
						applyColor(c);
				}
				else
				{
					endScreenPick();	//右键取消
				}
				return true;	//消费点击：弹窗不因点击外部而自动关闭，底层界面也不响应
			}
			case QEvent::KeyPress:
				if (static_cast<QKeyEvent*>(event)->key() == Qt::Key_Escape)
				{
					endScreenPick();
					return true;
				}
				break;
			default:
				break;
			}
		}
		return QWidget::eventFilter(watched, event);
	}

	void FbdColorPicker::slotColorEditorsChanged()
	{
		QColor c = _cur;
		if (_mode == 2)
		{
			QString hex = _hex->text().trimmed();
			if (!hex.startsWith(QLatin1Char('#')))
				hex.prepend(QLatin1Char('#'));
			const QColor parsed(hex);
			if (hex.length() == 7 && parsed.isValid())
				c = parsed;
		}
		else if (_mode == 1)
		{
			//剔除百分号等非数字后按范围收敛
			auto num = [](const QString& t) {
				QString digits;
				for (const QChar& ch : t)
					if (ch.isDigit())
						digits += ch;
				return digits.toInt();
				};
			const int h = qBound(0, num(_r->text()), 359);
			const int sat = qBound(0, num(_g->text()), 100);
			const int lig = qBound(0, num(_b->text()), 100);
			c = QColor::fromHsl(h, sat * 255 / 100, lig * 255 / 100);
		}
		else
		{
			c = QColor(qBound(0, _r->text().toInt(), 255),
				qBound(0, _g->text().toInt(), 255),
				qBound(0, _b->text().toInt(), 255));
		}
		if (!c.isValid())
			return;
		_cur = c;
		_sv->setColor(c);
		_hue->setHue(c.hue());
		refreshColorEditors();
		_preview->setStyleSheet(QString("background:%1;border:1px solid #DCE2E9;border-radius:15px;")
			.arg(c.name()));
		emit colorChanged(_cur);
	}
}
