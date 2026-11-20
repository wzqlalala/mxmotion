#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include <memory>
#include <set>

//QT
#include <QWidget>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QHash>
#include <QGridLayout>
#include <QTreeWidget>
#include <QToolBar>
#include <QStyledItemDelegate>
#include <QApplication>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QPainter>
#include <QTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QDateTime>
#include <QLineEdit>
#include <QCheckBox>
#include "mDockBase.h"
#include "mModelStruct.h"
#include <QSplitter>
#include <QSplitterHandle>
#include <QPainter>
#include <QMouseEvent>
#include <QStyleOption>
#include <QToolButton>
#include <QDebug>
#include "mChineseTextEdit.h"
#include "UiPalette.h" //可折叠分割条随主题（规则 0.2）
class HoverFilter : public QObject
{
	Q_OBJECT
public:
	explicit HoverFilter(QObject* parent = nullptr) : QObject(parent) {}
signals:
	void entered();   // 悬停进入
	void left();      // 悬停离开
protected:
	bool eventFilter(QObject* obj, QEvent* ev) override
	{
		if (ev->type() == QEvent::Enter) emit entered();
		if (ev->type() == QEvent::Leave) emit left();
		return QObject::eventFilter(obj, ev);
	}
};

class CollapsibleSplitterHandle : public QSplitterHandle
{
    Q_OBJECT
public:
    CollapsibleSplitterHandle(Qt::Orientation orientation, QSplitter* parent)
        : QSplitterHandle(orientation, parent), m_btn(nullptr), m_hover(false)
    {
        setMouseTracking(true);

        m_btn = new QToolButton(this);
        m_btn->hide();
        m_btn->setFixedSize(12, 50);
        m_btn->setText(">");
        m_btn->setToolButtonStyle(Qt::ToolButtonTextOnly);
        m_btn->hide();
        applyTheme(); // 初始主题色（底色/边线/文字全部取色板）

        // 按钮自身的悬停也要保持显示
        HoverFilter* btnHover = new HoverFilter(m_btn);
        m_btn->installEventFilter(btnHover);
        connect(btnHover, &HoverFilter::entered, m_btn, &QToolButton::show);
        connect(btnHover, &HoverFilter::left, this, [this]() {
            if (!m_hover && m_btn) m_btn->hide();
            });

        connect(m_btn, &QToolButton::clicked, this, [this]() {
            QSplitter* s = splitter();
            if (!s) return;

            int total = s->width() - s->handleWidth();
            QList<int> sizes = s->sizes();
            if (sizes.size() < 2) return;
            int left = sizes[0];
            int right = sizes[1];

            const int LEFT_MIN = 144;
            const int MIDDLE = 800;

            int leftTarget, rightTarget;

            if (right <= 50) {
                // 当前右侧已收起（图二），回到中间
                leftTarget = qMin(MIDDLE, total - LEFT_MIN);
                rightTarget = total - leftTarget;
            }
            else {
                // 当前是中间状态（图一），收起右侧
                leftTarget = total;
                rightTarget = 0;
            }

            s->setSizes({ leftTarget, rightTarget });
            updateArrow();
            });

        if (parent) {
            connect(parent, &QSplitter::splitterMoved, this, &CollapsibleSplitterHandle::updateArrow);
        }

        updateArrow();
    }

    void updateArrow()
    {
        QSplitter* s = splitter();
        if (!s || !m_btn) return;

        QList<int> sizes = s->sizes();
        if (sizes.size() < 2) return;

        int total = s->width() - s->handleWidth();
        int left = sizes[0];
        int right = sizes[1];
        int btnW = m_btn->width();
        int lineW = 2;
        int lineRightX = width() / 2 + lineW / 2;
        int lineLeftX = width() / 2 - lineW / 2;

        if (right <= 50) {
            // 右侧已收起，分隔线在最右侧，按钮应在线左侧，箭头朝左
            m_btn->setText("<");
            m_btn->move(lineLeftX - btnW, m_btn->y());
        }
        else {
            // 右侧展开，分隔线在中间，按钮应在线右侧，箭头朝右
            m_btn->setText(">");
            m_btn->move(lineRightX, m_btn->y());
        }
        applyTheme(); // 按钮朝向变化时重发（边线左右侧不同）
    }

    /*
    * 主题重发（规则 0.2）：中线 = border（悬停 primary）、折叠按钮 = surface 底 /
    * border 边 / text 字，悬停 hover、按下 selected。把手本身不铺底色（透出所在
    * 窗口背景），只保留一根中线。
    * 原写死 #FFFFFF/#A0A0A0/#D0D0D0/#3A8DE8/#D0E7FD 全部移除。
    * 由 mDockLayoutManager::applyTheme() 经元方法统一驱动，构造时自调一次。幂等。
    */
    Q_INVOKABLE void applyTheme()
    {
        if (!m_btn) return;
        using namespace MxFunctions;
        QSplitter* s = splitter();
        bool collapsed = false;
        if (s)
        {
            QList<int> sizes = s->sizes();
            if (sizes.size() >= 2)
                collapsed = sizes[1] <= 50;
        }
        // 收起态按钮在线左侧：右边贴线不留边；展开态按钮在线右侧：左边贴线不留边
        const QString edge = collapsed
            ? QStringLiteral("border-right:none; border-left:1px solid %2;")
            : QStringLiteral("border-left:none; border-right:1px solid %2;");
        m_btn->setStyleSheet(QStringLiteral(
            "QToolButton {"
            "  background-color: %1;"
            "  border-top: 1px solid %2;"
            "  border-bottom: 1px solid %2;"
            "  %4"
            "  border-radius: 3px;"
            "  color: %3;"
            "  font-weight: bold;"
            "}"
            "QToolButton:hover { background-color: %5; }"
            "QToolButton:pressed { background-color: %6; }"
        ).arg(UiPalette::surface().name())
         .arg(UiPalette::border().name())
         .arg(UiPalette::text().name())
         .arg(edge)
         .arg(UiPalette::hover().name())
         .arg(UiPalette::selected().name()));
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override
    {
        // 不铺把手底色：透出所在窗口背景，只画一根中线（悬停 primary，平时 border）
        QPainter p(this);
        QRect r = rect();

        int lineWidth = 2;
        int offset = (r.width() - lineWidth) / 2;
        int margin = 5;
        QRect lineRect(offset, margin, lineWidth, r.height() - 2 * margin);
        p.fillRect(lineRect, m_hover ? MxFunctions::UiPalette::primary()
                                     : MxFunctions::UiPalette::border());
    }

    void resizeEvent(QResizeEvent* event) override
    {
        QSplitterHandle::resizeEvent(event);
        if (m_btn) {
            int btnH = m_btn->height();
            m_btn->move(m_btn->x(), (height() - btnH) / 2);
        }
        updateArrow();
    }

    void enterEvent(QEvent* event) override
    {
        m_hover = true;
        if (m_btn) m_btn->show();
        update();
        QSplitterHandle::enterEvent(event);
    }

    void leaveEvent(QEvent* event) override
    {
        m_hover = false;
        if (m_btn && !m_btn->underMouse()) m_btn->hide();
        update();
        QSplitterHandle::leaveEvent(event);
    }

private:
    QToolButton* m_btn;
    bool m_hover;
};



class CollapsibleSplitter : public QSplitter
{
    Q_OBJECT
public:
    CollapsibleSplitter(QWidget* parent = nullptr) : QSplitter(Qt::Horizontal, parent)
    {
        connect(this, &QSplitter::splitterMoved, this, [this](int pos, int index) {
            Q_UNUSED(pos)
                Q_UNUSED(index)
                QList<int> sizes = this->sizes();
            if (sizes.size() >= 2) {
                bool changed = false;
                const int MIN_W = 144;

                if (sizes[0] < MIN_W) {
                    sizes[0] = MIN_W;
                    changed = true;
                }
                if (sizes[1] < MIN_W) {
                    int need = MIN_W - sizes[1];
                    sizes[1] = MIN_W;
                    sizes[0] = qMax(MIN_W, sizes[0] - need);
                    changed = true;
                }
                if (changed) this->setSizes(sizes);
            }

            for (int i = 0; i < count() - 1; ++i) {
                CollapsibleSplitterHandle* h = qobject_cast<CollapsibleSplitterHandle*>(handle(i));
                if (h) h->updateArrow();
            }
            });
    }

protected:
    QSplitterHandle* createHandle() override
    {
        return new CollapsibleSplitterHandle(orientation(), this);
    }
};



namespace MechMainWindow
{
	//剖面面板
	class MMAINWINDOW_EXPORT mOutputInfoWindow : public QWidget
	{
		Q_OBJECT

	public:

		/*
		* 获取单例指针
		*/
		static mOutputInfoWindow* getInstance();


		/*
		* 析构函数
		*/
		~mOutputInfoWindow();

		/*
		* 获取表格警告、错误个数
		*/
		std::pair<int, int> getErrorWarningCount();

		/*
		* 打开表格窗口(显示动态信息)
		*/
		void showTableInfo();

	private:
		/*
		* 默认构造函数
		*/
		mOutputInfoWindow(QWidget* parent = Q_NULLPTR);

		/*
		* 初始化日志文件
		*/
		void initLogFile();

		/*
		* 写日志文件
		*/
		void writeLogFile(const QString& text);
      

	private slots:

		/*
		* 输出字符串
		*/
        void outputMessage(int Type, const QString& msg, bool statusBar, bool outputTime);

        /*
        * 输出字符串
        */
        void outputMessage(const QString& msg, bool statusBar, bool outputTime);

		/*
		* 输出类型及字符串
		*/
        void updateClearBtnState();

		/*
		* 行编辑框回车
		*/
		void enterKeySlot();

		/*
		* 信息输出表格更新
		*/
		void slot_updateOutPutInfoTable(const QHash<QString, QSet<int>>& deleteInfoData, const QList<outputInfoData>& infoData);

		/*
		*重命名
		*/
		void slot_resetName(const QString& module, const int& phyID, const QString& newName);

		/*
		* 常态信息更新
		*/
		void slot_updataNormalInfo(QList<outputInfoData>& infoData);

		/*
		* 清空常态信息
		*/
		void slot_clearNormalInfo();

public slots:
		/*
		* 信息显示
		*/
        void slot_tableInfoDataShow(int state);
	protected:

		//单例指针
		static mOutputInfoWindow* _instance;

		//富文本输入框
		mChineseTextEdit* _textEdit;

		//Python命令输入框
		QLineEdit* _commandEdit;

		//表格窗口
		QWidget* _tableWidget;

		//表格
		QTableWidget* _table;

		//表格显隐
		QPushButton* _tableShow;
		//错误
        QCheckBox* _error;
		//警告
        QCheckBox* _warning;
		QHash<QString, int> _count;
		//常态信息
        QCheckBox* _normalInfo;
		//动态信息
        QCheckBox* _dynamicInfo;
        QPushButton* clearTable;
		//颜色
		QColor NormalMessageColor = QColor(0, 0, 0);
		QColor WarningMessageColor = QColor(0, 0, 255);
		QColor ErrorMessageColor = QColor(255, 0, 0);
		QColor PythonMessageColor = QColor(0, 0, 0);
      
		//时间
		QString _time;
		//日志文件名
		QString _logfile;
		//日志对象
		QFile _file;
	};
}