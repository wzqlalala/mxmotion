#pragma once
#pragma execution_character_set("utf-8")
#include "mmainwindow_global.h"

#include <QWidget>
#include <QTreeWidgetItem>
#include <QEvent>
#include <QHash>
#include <QVariant>
#include <QPair>
#include <QVector>
#include <QLabel>
#include <QLineEdit>
#include <QTableWidget>

#include "mMaterialBase.h"
#include "mUnitEnum.h"
#include "mMaterialProps.h"
// 折叠分割条相关类定义在命名空间之后，避免与 mOutputInfoWindow.h 中同名类冲突
#include <QSplitter>
#include <QSplitterHandle>
#include <QPainter>
#include <QToolButton>
namespace MechMainWindow
{
	class mCustomMaterialProps;
}

QT_BEGIN_NAMESPACE
namespace Ui { class mCustomMaterialWindowClass; };
QT_END_NAMESPACE


class CustomMatHoverFilter : public QObject
{
    Q_OBJECT
public:
    explicit CustomMatHoverFilter(QObject* parent = nullptr) : QObject(parent) {}
signals:
    void entered();
    void left();
protected:
    bool eventFilter(QObject* obj, QEvent* ev) override
    {
        if (ev->type() == QEvent::Enter) emit entered();
        if (ev->type() == QEvent::Leave) emit left();
        return QObject::eventFilter(obj, ev);
    }
};


class CustomMatCollapsibleSplitterHandle : public QSplitterHandle
{
    Q_OBJECT
public:
    CustomMatCollapsibleSplitterHandle(Qt::Orientation orientation, QSplitter* parent, int middleWidth = 400)
        : QSplitterHandle(orientation, parent), m_btn(nullptr), m_hover(false), m_middleWidth(middleWidth)
    {
        setMouseTracking(true);

        m_btn = new QToolButton(this);
        m_btn->hide();
        m_btn->setFixedSize(12, 50);
        m_btn->setText(">");
        m_btn->setStyleSheet(
            "QToolButton {"
            "  background-color: #FFFFFF;"
            "  border-top: 1px solid #A0A0A0;"
            "  border-bottom: 1px solid #A0A0A0;"
            "  border-left: none;"
            "  border-right: 1px solid #A0A0A0;"
            "  border-radius: 3px;"
            "  color: #606060;"
            "  font-weight: bold;"
            "}"
            "QToolButton:hover { background-color: #D0E7FD; }"
            "QToolButton:pressed { background-color: #B0D8FC; }"
        );
        m_btn->setToolButtonStyle(Qt::ToolButtonTextOnly);
        m_btn->hide();

        CustomMatHoverFilter* btnHover = new CustomMatHoverFilter(m_btn);
        m_btn->installEventFilter(btnHover);
        connect(btnHover, &CustomMatHoverFilter::entered, m_btn, &QToolButton::show);
        connect(btnHover, &CustomMatHoverFilter::left, this, [this]() {
            if (!m_hover && m_btn) m_btn->hide();
            });

        connect(m_btn, &QToolButton::clicked, this, [this]() {
            QSplitter* s = splitter();
            if (!s) return;

            int total = s->width() - s->handleWidth();
            QList<int> sizes = s->sizes();
            if (sizes.size() < 2) return;

            const int LEFT_MIN = 144;

            int leftTarget, rightTarget;

            if (sizes[1] <= 50) {
                leftTarget = qMin(m_middleWidth, total - LEFT_MIN);
                rightTarget = total - leftTarget;
            }
            else {
                leftTarget = total;
                rightTarget = 0;
            }

            s->setSizes({ leftTarget, rightTarget });
            updateArrow();
            });

        if (parent) {
            connect(parent, &QSplitter::splitterMoved, this, &CustomMatCollapsibleSplitterHandle::updateArrow);
        }

        updateArrow();
    }

    void updateArrow()
    {
        QSplitter* s = splitter();
        if (!s || !m_btn) return;

        QList<int> sizes = s->sizes();
        if (sizes.size() < 2) return;

        int right = sizes[1];
        int btnW = m_btn->width();
        int lineW = 2;
        int lineRightX = width() / 2 + lineW / 2;
        int lineLeftX = width() / 2 - lineW / 2;

        if (right <= 50) {
            m_btn->setText("<");
            m_btn->move(lineLeftX - btnW, m_btn->y());
            m_btn->setStyleSheet(
                "QToolButton {"
                "  background-color: #FFFFFF;"
                "  border-top: 1px solid #A0A0A0;"
                "  border-bottom: 1px solid #A0A0A0;"
                "  border-left: 1px solid #A0A0A0;"
                "  border-right: none;"
                "  border-radius: 3px;"
                "  color: #606060;"
                "  font-weight: bold;"
                "}"
                "QToolButton:hover { background-color: #D0E7FD; }"
                "QToolButton:pressed { background-color: #B0D8FC; }"
            );
        }
        else {
            m_btn->setText(">");
            m_btn->move(lineRightX, m_btn->y());
            m_btn->setStyleSheet(
                "QToolButton {"
                "  background-color: #FFFFFF;"
                "  border-top: 1px solid #A0A0A0;"
                "  border-bottom: 1px solid #A0A0A0;"
                "  border-left: none;"
                "  border-right: 1px solid #A0A0A0;"
                "  border-radius: 3px;"
                "  color: #606060;"
                "  font-weight: bold;"
                "}"
                "QToolButton:hover { background-color: #D0E7FD; }"
                "QToolButton:pressed { background-color: #B0D8FC; }"
            );
        }
        m_btn->update();
        this->update();
    }
public:
    void triggerToggle()
    {
        m_btn->click();
    }


protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        QRect r = rect();
        p.fillRect(r, QColor("#FFFFFF"));

        int lineWidth = 2;
        int offset = (r.width() - lineWidth) / 2;
        int margin = 5;
        QRect lineRect(offset, margin, lineWidth, r.height() - 2 * margin);
        p.fillRect(lineRect, m_hover ? QColor("#3A8DE8") : QColor("#D0D0D0"));
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
    int m_middleWidth = 400;

};

class CustomMatCollapsibleSplitter : public QSplitter
{
    Q_OBJECT
public:
    CustomMatCollapsibleSplitter(QWidget* parent = nullptr, int middleWidth = 400) : QSplitter(Qt::Horizontal, parent), m_middleWidth(middleWidth)
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
                CustomMatCollapsibleSplitterHandle* h = qobject_cast<CustomMatCollapsibleSplitterHandle*>(handle(i));
                if (h) h->updateArrow();
            }
            });
    }
    void toggleRightPanel()
    {
        auto handleObj = handle(0);
        auto h = qobject_cast<CustomMatCollapsibleSplitterHandle*>(handleObj);
        if (h)
        {
            h->triggerToggle();
        }
    }
    bool isRightPanelCollapsed() const
    {
        auto sizes = this->sizes();
        if (sizes.size() < 2) return false;
        return sizes[1] <= 50;
    }

protected:
    QSplitterHandle* createHandle() override
    {
        return new CustomMatCollapsibleSplitterHandle(orientation(), this, m_middleWidth);
    }

private:
    int m_middleWidth = 400;
};
namespace MechMainWindow
{
	class MMAINWINDOW_EXPORT mCustomMaterialWindow : public QWidget
	{
		Q_OBJECT

	public:
		mCustomMaterialWindow(QWidget* parent = nullptr);
		~mCustomMaterialWindow();

		/*
		* 将模型中的材料导入到自定义材料库
		* srcMatId: 模型树中源材料物理ID
		* matType:  MxFunctions::MaterialType 枚举值
		* name:     材料名称
		*/
		void addMaterialFromModel(int srcMatId, int matType, const QString& name);

	private:
		void initUI();
		void initConnect();
		void initMaterialTree();
		void initPropertyTables();

		// 在右侧 scrollArea 中创建/更新只读材料属性展示面板
		void showMaterialReadOnly(QString matName);
		void clearMaterialReadOnly();

		// 根据 MxFunctions::MaterialType 返回分类节点名称
		QString materialTypeToCategory(int matType) const;
        //根据文件名拿到数据
        std::shared_ptr<mMaterialBase>getMaterialDataByFile(const QString& filePath);

        //删除自定义材料物理文件以及txt文件索引行
        void deleteMaterialsAndIndexLines(const QSet<QString>& reNames);

        //按子节点数量刷新分类节点文本上的材料计数（如"塑性(3)"）
        void refreshCategoryCounts();

	private slots:
		void slot_MaterialSelected(const QItemSelection& selected, const QItemSelection& deselected);
		void slot_loadCustomDepotSig();
        void slot_removeMaterial();
        void slot_insertMaterial();
        void slot_addToMaterialDepot(QVector<int> matIds);
	protected:
		bool eventFilter(QObject* obj, QEvent* ev) override;

	private:
		Ui::mCustomMaterialWindowClass* ui;

		// 右侧只读展示面板容器
		QWidget* m_readOnlyPanel = nullptr;

		// 独立非单例材料属性控件管理器（避免与编辑面板共享状态）
		mCustomMaterialProps* m_customProps = nullptr;

		// 当前只读面板中临时创建、需要随面板一起释放的属性页实例
		QVector<mMaterialPropBase*> m_readOnlyProps;

        CustomMatCollapsibleSplitter* m_CustomSplitter;

        //界面展示配置
        QHash<MxFunctions::MaterialType, QVector<MxFunctions::MaterialClass>> m_PanelConfig;
	};
}





