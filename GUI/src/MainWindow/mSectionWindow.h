#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include <memory>
#include <set>
#include "mViewportModel.h"

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
#include <QScrollArea>
#include <QMdiSubWindow>
#include <QActionGroup>
#include <QToolButton>
#include <QPushButton>
#include <QLineEdit>
//namespace MechPreData
//{
//    class mPreViewportPrivate;
//    class PreViewportWidget;
//    class mSection;
//}
//namespace mxr
//{
//    class CCSMRendWidget;
//}
namespace MechMainWindow
{
    //class CheckBoxFriendlyDelegate : public QStyledItemDelegate
    //{
    //	Q_OBJECT

    //signals:
    //	void checkBoxToggled(const QModelIndex& index, Qt::CheckState state) const;

    //public:
    //	using QStyledItemDelegate::QStyledItemDelegate;

    //	void paint(QPainter* painter, const QStyleOptionViewItem& option,
    //		const QModelIndex& index) const override
    //	{
    //		QStyleOptionViewItem opt = option;
    //		initStyleOption(&opt, index);
    //		QStyle* style = opt.widget ? opt.widget->style() : QApplication::style();

    //		painter->save();

    //		// 1. 自定义选中背景：避免遮挡复选框
    //		if (opt.state & QStyle::State_Selected) {
    //			QRect fullRect = opt.rect;

    //			QRect checkRect = style->subElementRect(QStyle::SE_ItemViewItemCheckIndicator, &opt, opt.widget);

    //			int spacing = 4;
    //			QRect textBgRect = fullRect;
    //			textBgRect.setLeft(checkRect.right() + spacing);

    //			painter->fillRect(textBgRect, QColor("#D0E9FF"));
    //		}

    //		// 2. 手动绘制复选框（根据真实状态）
    //		QVariant checkState = index.data(Qt::CheckStateRole);
    //		if (checkState.isValid()) {
    //			QStyleOptionButton checkOpt;
    //			checkOpt.state |= QStyle::State_Enabled;

    //			Qt::CheckState state = static_cast<Qt::CheckState>(checkState.toInt());
    //			if (state == Qt::Checked)
    //				checkOpt.state |= QStyle::State_On;
    //			else if (state == Qt::Unchecked)
    //				checkOpt.state |= QStyle::State_Off;
    //			else
    //				checkOpt.state |= QStyle::State_NoChange;

    //			checkOpt.rect = style->subElementRect(QStyle::SE_ItemViewItemCheckIndicator, &opt, opt.widget);
 //               style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);
    //		}

    //		// 3. 绘制文字
    //		QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &opt, opt.widget);
    //		style->drawItemText(painter, textRect, opt.displayAlignment,
    //			opt.palette, true, opt.text, QPalette::Text);

    //		painter->restore();
    //	}

    //	bool CheckBoxFriendlyDelegate::editorEvent(QEvent* event, QAbstractItemModel* model,
    //		const QStyleOptionViewItem& option, const QModelIndex& index) override
    //	{
    //		if (!(index.flags() & Qt::ItemIsUserCheckable)) {
    //			// 不是可勾选项，交由基类处理（如编辑）
    //			return QStyledItemDelegate::editorEvent(event, model, option, index);
    //		}

    //		if (event->type() == QEvent::MouseButtonRelease) {
    //			auto* mouseEvent = static_cast<QMouseEvent*>(event);

    //			QStyleOptionViewItem opt = option;
    //			initStyleOption(&opt, index);
    //			QStyle* style = opt.widget ? opt.widget->style() : QApplication::style();

    //			QRect checkRect = style->subElementRect(QStyle::SE_ItemViewItemCheckIndicator, &opt, opt.widget);

    //			if (checkRect.contains(mouseEvent->pos())) {
    //				// ✅ 只在点击复选框时修改勾选状态
    //				Qt::CheckState state = static_cast<Qt::CheckState>(index.data(Qt::CheckStateRole).toInt());
    //				Qt::CheckState newState = (state == Qt::Checked) ? Qt::Unchecked : Qt::Checked;
    //				model->setData(index, newState, Qt::CheckStateRole);

    //				emit checkBoxToggled(index, newState);  // ✅ 这里额外发信号
    //				return true;
    //			}
    //		}

    //		// ✅ 其他情况交回基类处理（比如进入编辑状态）
    //		return QStyledItemDelegate::editorEvent(event, model, option, index);
    //	}

    //	QWidget* CheckBoxFriendlyDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option,
    //		const QModelIndex& index) const {
    //		if (index.flags() & Qt::ItemIsEditable) {
    //			return QStyledItemDelegate::createEditor(parent, option, index);
    //		}
    //		return nullptr;
    //	}
    //};

    class ToggleActionGroup : public QActionGroup
    {
        Q_OBJECT
    public:
        explicit ToggleActionGroup(QObject* parent = nullptr)
            : QActionGroup(parent), m_lastAction(nullptr), m_ignore(false)
        {
            setExclusive(false);
            connect(this, &QActionGroup::triggered, this, &ToggleActionGroup::onTriggered);
        }

        void setCurrentAction(int index) {
            auto list = this->actions();
            for (int i = 0; i < list.size(); i++)
            {
                if (i != index)
                {
                    list[i]->setChecked(false);
                }
                else
                {
                    list[i]->setChecked(true);
                }
            }
        };

        QAction* getCurrentAction()
        {
            auto list = this->actions();
            for (int i = 0; i < list.size(); i++)
            {
                if (list[i]->isChecked())
                {
                    return list[i];
                }
            }
            return nullptr;
        }

        QAction* addAction(QAction* action)
        {
            QActionGroup::addAction(action);
            //action->setCheckable(true);

            //connect(action, &QAction::toggled, this, [this, action](bool checked) {
            //	if (checked) {
            //		emit actionChecked(action);
            //	}
            //	else {
            //		emit actionUnchecked(action);
            //	}
            //	});

            return action;
        }

        QAction* currentAction() const { return m_lastAction; }

    signals:
        void actionChecked(QAction* action);
        void actionUnchecked(QAction* action);

    private slots:
        void onTriggered(QAction* action)
        {
            auto list = this->actions();
            if (action->isChecked())
            {
                for (int i = 0; i < list.size(); i++)
                {
                    if (action != list[i])
                    {
                        list[i]->setChecked(false);
                    }
                }
                emit actionChecked(action);
            }
            else
            {
                emit actionUnchecked(action);
            }

            /*
            if (m_ignore) return;

            if (action == m_lastAction) {
                // 再次点击同一个 → 取消
                m_ignore = true;
                bool oldExclusive = isExclusive();
                setExclusive(false);          // 暂时关闭 exclusive
                action->setChecked(false);    // 触发 toggled(false)
                setExclusive(oldExclusive);   // 恢复 exclusive
                m_ignore = false;

                m_lastAction = nullptr;
                emit actionUnchecked(action);
            }
            */
        }

    private:
        QAction* m_lastAction;
        bool m_ignore;
    };
    //剖面面板
    class MMAINWINDOW_EXPORT mSectionWindow : public QWidget
    {
        Q_OBJECT

    public:

        /*
        * 获取单例指针
        */
        static mSectionWindow* getInstance();


        /*
        * 析构函数
        */
        ~mSectionWindow();

        /*
        * 创建剖面
        */
        void createSection(int type);

        /*
        * 编辑剖面
        */
        void editSection();

        /*
        * 退出剖面
        */
        void unEditSection();

        /*
        * 隐藏事件
        */
        void hideEvent(QHideEvent* event) override;

    private:
        /*
        * 默认构造函数
        */
        mSectionWindow(QWidget* parent = Q_NULLPTR);
        void startRenameEdit(QLineEdit* edit);
        void createToolBar();     // 创建工具栏
        void createTreeWidget();  // 创建树形面板
        QTreeWidgetItem* createTreeWidgetItem(MechPreData::mSection* section);//创建子项
        QList<int> getItemPath(QTreeWidgetItem* target);//获取Item的索引

        // 行内按钮不会自动触发 QTreeWidget 的选中逻辑，因此统一计算本次操作目标。
        QList<QTreeWidgetItem*> itemsForRowAction(QTreeWidgetItem* clickedItem);
        void setSectionsVisible(const QList<QTreeWidgetItem*>& items, bool visible);
        void deleteSections(const QList<QTreeWidgetItem*>& items);

        QSize mSectionWindow::sizeHint() const override
        {
            if (m_toolBar && m_treeWidget)
            {
                int w = qMax(m_toolBar->sizeHint().width(), m_treeWidget->sizeHint().width());
                int h = m_toolBar->sizeHint().height() + m_treeWidget->sizeHint().height();
                //return QSize(w, h);
                return QSize(1000, 100);
            }
            return QWidget::sizeHint();
        }

        void stopBoundViewportInteraction();

    public slots:

        /*
        * 切换视口
        */
        void slot_subWindowActivated(int viewportID);

        /*
        * 完成创建时的拾取
        */
        void slot_finishedCreatePick();


    protected:

        //总布局
        QVBoxLayout* _mainLayout;

        //单例指针
        static mSectionWindow* _instance;

        //工具栏
        QToolBar* m_toolBar;
        ToggleActionGroup* m_actionGroup;
        QTreeWidget* m_treeWidget;

        //编辑、重命名、删除
        /*QAction* m_actionEdit = nullptr;
        QAction* m_actionRename = nullptr;
        QAction* m_actionDelete = nullptr;*/
        QAction* m_actionType = nullptr;
        QAction* m_actionColor = nullptr;
        QAction* m_actionTransparency = nullptr;

        //拾取高亮数据改变的连接
        QMetaObject::Connection m_conn;

        QPointer<mxr::CCSMRendWidget> m_boundRenderWidget;
        QPointer<MechPreData::PreViewportWidget> m_boundPreViewportWidget;
        bool eventFilter(QObject* obj, QEvent* event) override;
    };
}


