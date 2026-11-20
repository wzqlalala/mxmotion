#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

//QT
#include <QDialog>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QResizeEvent>
#include <QStyledItemDelegate>
#include <vector>

#include "mModelEnum.h"

using namespace MxFunctions;

namespace MechMainWindow
{
    class mMenuWidget;
    class mManagerTableWidget;

    // 自定义Checkbox委托：统一绘制、统一处理点击，替代单元格Widget方案
    class CheckBoxDelegate : public QStyledItemDelegate
    {
        Q_OBJECT
    public:
        explicit CheckBoxDelegate(QObject* parent = nullptr);
        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
        bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override;
        QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    signals:
        void checkStateChanged(int row, int col, bool checked);
    };

    //对象×工况勾选矩阵：行=对象（载荷/边界/预定义场/接触/模型修改），列=工况。
    //可内嵌展示（带筛选工具栏，用于Table dock），也可作为弹出对话框使用（无筛选工具栏，管理器面板入口）
    class mManagerTableWidget : public QWidget
    {
        Q_OBJECT
    public:
        explicit mManagerTableWidget();
        ~mManagerTableWidget();

        void refreshTable(BrowserTreeType itemType);
        void setMenuWidget(mMenuWidget* menuWidget);

        //弹出模式：隐藏筛选工具栏，供管理器面板的对话框复用
        void setPopupMode(bool popup);

    protected:
        void resizeEvent(QResizeEvent* event) override;
        void showEvent(QShowEvent* event) override;

    private slots:
        void onCellCheckStateChanged(int row, int col, bool checked);
        void onCellClicked(int row, int col);

        void onEditBtnClicked();
        void onCopyBtnClicked();
        void onDeleteBtnClicked();

        //筛选条件变化：重排矩阵行列
        void onFilterChanged();

    private:
        void resizeHeadersToContents();
        QList<int> getSelectedRows() const;
        void copySingleRow(int row);
        void deleteSingleRow(int row);

        //当前类型下对象的名称/类型名（供筛选与行头使用）
        QString objectNameOf(int dataId) const;
        QString objectTypeNameOf(int dataId) const;
        //重建类型筛选下拉（restoreType非空时优先恢复该项）
        void refreshTypeFilterCombo(const QString& restoreType = QString());

    private:
        QTableWidget* m_table;
        CheckBoxDelegate* m_checkDelegate;
        std::vector<int> m_ids;
        std::vector<int> m_caseIds;
        BrowserTreeType m_currentType;
        QTextEdit* m_infoEdit;
        QPushButton* btnEdit;
        QPushButton* btnCopy;
        QPushButton* btnDelete;
        mMenuWidget* m_menuWidget = nullptr;

        //筛选工具栏：类型筛选 + 工况类型筛选 + 名称筛选（对齐管理器面板筛选行；弹出模式下隐藏）
        QWidget* m_filterBar = nullptr;
        QComboBox* m_typeFilter = nullptr;
        QComboBox* m_caseFilter = nullptr;
        QLineEdit* m_nameFilter = nullptr;

        //弹窗模式顶部工具栏：类型筛选 + 工况类型筛选 + 全部激活/取消按钮
        QWidget* m_popupToolBar = nullptr;
        QComboBox* m_popupTypeFilter = nullptr;
        QComboBox* m_popupCaseTypeFilter = nullptr;
        QPushButton* m_btnActivateAll = nullptr;

        //底部选中信息区（选中载荷 xxx / 选中工况 xxx）
        QWidget* m_selectionBar = nullptr;
        QLabel* m_selectedObjectLabel = nullptr;
        QLabel* m_selectedCaseLabel = nullptr;

        //弹窗模式标志（决定显示哪套工具栏与底部按钮）
        bool m_popupMode = false;

        //批量勾选/取消当前显示的全部可勾选单元格
        void slot_activateAllBtn();
        //刷新全部激活按钮文字（当前显示单元格全部勾选时为"全部取消"）
        void syncActivateAllBtnText();
        //刷新底部选中信息
        void syncSelectionInfo(int row, int col);
    };

    //管理器面板的矩阵弹出对话框（模态，对齐"载荷管理器原型"的配置工况载荷弹窗）：
    //持有独立的mManagerTableWidget，避免与Table dock中常驻矩阵发生重父子冲突
    class mManagerTablePopupDialog : public QDialog
    {
        Q_OBJECT
    public:
        explicit mManagerTablePopupDialog(QWidget* parent = nullptr);

        //模态展示指定类型的矩阵；每次弹出定位到anchor（配置工况按钮）右侧，可拖动但下次仍回到锚点位置
        void execMatrix(const QString& title, mMenuWidget* menuWidget, BrowserTreeType itemType, QWidget* anchor);

    private:
        //按anchor位置计算弹窗左上角（面板右侧，纵向与锚点对齐，不超出屏幕）
        void positionBesideAnchor(QWidget* anchor);

        mManagerTableWidget* m_matrix = nullptr;
    };
}
