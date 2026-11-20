#ifndef MNVHWINDOWCOMPONENT_H
#define MNVHWINDOWCOMPONENT_H

#include <QWidget>
#include <QList>
#include <QMap>
#include <QVariant>
#include <QObject>
#include <QTableWidget>
#include <QStringList>
#include <QStyledItemDelegate>
#include <QHeaderView>
#include <QEvent>
#include <QMap>
#include <QHash>
#include <QIcon>
#include <QDialog>
#include "mPlotStructs.h"
namespace PlotView
{
    class mPlotView;
}
using namespace PlotView;
class QVBoxLayout;
class QHBoxLayout;
class QLabel;
class QToolButton;
class QGroupBox;
class QTableWidget;
class QPushButton;
class QLineEdit;
class QCheckBox;
class QResizeEvent;
class QComboBox;
class QTabWidget;

/*****************************************输入框输入限制******************************************************/
/*
* 数值输入框限制：输入非合法数值形态时直接置空（允许输入过程中的临时形态，如 "1."、"1e-"）
*/
void installDoubleInputFilter(QLineEdit* edit);

/*
* 整数输入框限制：输入含非数字字符时直接置空
*/
void installIntInputFilter(QLineEdit* edit);
/*****************************************mCheckBoxHeader******************************************************/
class mCheckBoxHeader : public QHeaderView
{
    Q_OBJECT

public:
    explicit mCheckBoxHeader(Qt::Orientation orientation, QWidget* parent = nullptr);
    ~mCheckBoxHeader();

    void setColumnCheckBox(int col, bool show, const QString& text = QString());
    void updateCheckBoxPositions();
    void setColumnCheckState(int col, int state);
    int getColumnCheckState(int col) const;
    void clearAllCheckBoxes();

signals:
    void columnCheckStateChanged(int col, int state);

protected:
    void paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const override;
    void resizeEvent(QResizeEvent* event) override;
    void sectionResized(int logicalIndex, int oldSize, int newSize);

private:
    QMap<int, QCheckBox*> columnCheckBoxMap;
};
/*****************************************NavIconDelegate******************************************************/
class NavIconDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    explicit NavIconDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    void setPageCompleted(int pageIndex, bool completed);
    bool isPageCompleted(int pageIndex) const;
    void setCurrentPage(int pageIndex);
    int currentPage() const;
    void setPageEnabled(int pageIndex, bool enabled);
    bool isPageEnabled(int pageIndex) const;

    void setOutputPageIndex(int pageIndex);
    int outputPageIndex() const;

private:
    QIcon m_iconAnalysisActive;
    QIcon m_iconAnalysisInactive;
    QIcon m_iconOutputActive;
    QIcon m_iconOutputInactive;
    QIcon m_iconOutputClickable;
    QIcon m_iconCompleted;
    QHash<int, bool> m_pageCompleted;
    QHash<int, bool> m_pageEnabled;
    int m_currentPage = 0;
    int m_outputPageIndex = -1;
};

/*****************************************ResponseSettingFoldablePanel******************************************************/
class ResponseSettingFoldablePanel : public QWidget
{
    Q_OBJECT

public:
    explicit ResponseSettingFoldablePanel(const QString& panelName, QWidget* parent = nullptr);
    ~ResponseSettingFoldablePanel();
    int rowCount() const;              // 获取某区行数
    bool hasValidRow() const;          // 是否存在至少一条有效数据行
    QList<QPair<int, QString>> getTableIds() const; // 获取表格中所有有效的（ID, Label）
    void collapsedOnInitialization(); // 仅供外部在初始化时折叠面板使用
    void SetContentEnable(bool ok);

    /*
    * 恢复为初始默认状态：清空后按默认行数走添加行逻辑重建
    */
    void resetToDefault();
signals:
    void dataChanged();

private slots:
    void onFoldClicked();      // 点击标题切换折叠
    void onAddRow();            // 按钮“添加一行”触发
    void onDeleteRow();         // 按钮“删除”触发
    void onImportFromFile();    // 按钮“从文件导入”触发
    void onExportToFile();      // 按钮“导出文件”触发
    void slot_onPointCountChanged();
    void onTableItemChanged(QTableWidgetItem* item); // 表格项内容变化，处理占位符
private:
    // UI 组件
    bool m_isElementForce;
    int m_defaultRows = 0;    // 构造完成后的默认行数
    QWidget* m_titleWidget;
    QPushButton* m_toggleButton;
    QWidget* m_contentWidget;
    QLineEdit* m_PointCount = nullptr;
    QPushButton* m_AddRow = nullptr;
    QPushButton* m_RemoveRow = nullptr;
    QPushButton* m_ImportFile = nullptr;
    QPushButton* m_ExportFile = nullptr;
    QTableWidget* m_TableWidget = nullptr;
    std::vector<int> m_allNodeIds;
    bool m_isOpen;
    bool m_firstResizeDone = false;
    // 内部辅助函数
    QIcon getIcon(bool open);
    QIcon drawArrowIcon(bool open, const QColor& fill, const QColor& outline, int px, qreal dpr);
    //更新删除按钮状态
    void updateDeleteButtonState();
    void resizeEvent(QResizeEvent* event) override;
    void adjustColumnWidths();
};
/*****************************************CreateCurveDialogData******************************************************/
struct IncentiveCurve
{
    QString name;
    QString delay;
    QString dphase;
    int freqCombobox;
    int loadCombobox;
    QVector<double> colX;
    QVector<double> colY;
    QVector<double> colZ;
};
/*****************************************IncentiveNodeTableWidget******************************************************/
class IncentiveNodeTableWidget : public QTableWidget
{
    Q_OBJECT

public:
    explicit IncentiveNodeTableWidget(QWidget* parent = nullptr);
    ~IncentiveNodeTableWidget();
    struct IncentiveNodeRowData {
        QString nodeId;
        bool x = false;
        bool y = false;
        bool z = false;
        bool rx = false;
        bool ry = false;
        bool rz = false;
        QString label;
    };
    // 对外公开接口
    void addRow();

    /*
    * 恢复为初始默认状态：清空所有行并重建默认行
    */
    void resetToDefault();

    /*
    * 约束表使用：将默认状态置为空表（默认行数记为 0 并清除当前行）
    */
    void setEmptyDefault();
    void removeSelectedRows();
    void setRowCount(int count);
    void setTableData(const QList<IncentiveNodeRowData>& data);
    QList<IncentiveNodeRowData> getTableData() const;
    IncentiveNodeRowData getRowData(int row) const;

    void setShowLabelColumn(bool show);
private:
    void initRow(int row);
    void setupAppearance();
    void refreshHeader();
    bool m_showLabelColumn = false;
    int m_defaultRows = 1;    // 构造完成后的默认行数（resetToDefault 依据）
};
/*****************************************SplitButtonEdit******************************************************/
class SplitButtonEdit : public QWidget
{
    Q_OBJECT
public:
    explicit SplitButtonEdit(bool singleCurve, QWidget* parent = nullptr);

    // 设置/获取文本
    void setText(const QString& text);
    QString text() const;

    // 设置占位符
    void setPlaceholderText(const QString& text);

    // 设置按钮图标
    void setButtonIcon(const QString& imagePath);

    void setCurveData(const IncentiveCurve& data);
    IncentiveCurve getCurveData();
    bool m_isCompleted = false;
signals:
    void textChanged(const QString& text);
    //void textEdited(const QString& text);
    //void returnPressed();
    void buttonClicked();
    void CurveStatusChanged(bool isCompleted);
protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

    QLineEdit* m_lineEdit;
    QPushButton* m_button;
    QFrame* m_separator;
    QHBoxLayout* m_layout;
    IncentiveCurve m_curveData;
    QColor m_borderColor = QColor(220, 223, 230);
    int m_borderRadius = 4;
    int m_borderWidth = 1;
    bool m_singleCurve = false;
};

/*****************************************CreateCurveDialog******************************************************/
class CreateCurveDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CreateCurveDialog(QWidget* parent = nullptr);
    ~CreateCurveDialog();
    void initDialog();
    void setDialogData(IncentiveCurve data);
    /*
      * 获取单例指针
    */
    static CreateCurveDialog* getInstance();
    SplitButtonEdit* m_currentButton = nullptr;
    bool isDataValid();
private slots:
    void onSave();
    void onClose();
    void onPlotCurve();
    void onImportCurve();
private:
    void setupUI();
    //单例指针
    static CreateCurveDialog* _instance;
    QLineEdit* m_CurveNameEdit = nullptr;
    QPushButton* m_ImportCurve = nullptr;
    QLineEdit* m_LineEdit_DELAY = nullptr;
    QLineEdit* m_LineEdit_DPHASE = nullptr;
    QComboBox* m_ComboBox_Freq = nullptr;
    QComboBox* m_ComboBox_Load = nullptr;
    QPushButton* m_TableBtn = nullptr;
    QPushButton* m_ChartBtn = nullptr;
    QTabWidget* m_tabWidget = nullptr;
    QTableWidget* m_TableWidget = nullptr;
    QPushButton* m_SaveBtn = nullptr;
    QPushButton* m_CloseBtn = nullptr;
    QWidget* m_PlotView = nullptr;
    mPlotView* m_Plot1 = nullptr;
    mPlotView* m_Plot2 = nullptr;
    AxisSettings m_bottomAxis;
    AxisSettings m_leftAxis1;
    AxisSettings m_leftAxis2;
};
/*****************************************CreateSingleCurveDialog******************************************************/
class CreateSingleCurveDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CreateSingleCurveDialog(QWidget* parent = nullptr);
    ~CreateSingleCurveDialog();
    void initDialog();
    void setDialogData(IncentiveCurve data);
    /*
      * 获取单例指针
    */
    static CreateSingleCurveDialog* getInstance();
    SplitButtonEdit* m_currentButton = nullptr;
    bool isDataValid();
private slots:
    void onSave();
    void onClose();
    void onPlotCurve();
    void onImportCurve();
private:
    void setupUI();
    //单例指针
    static CreateSingleCurveDialog* _instance;
    QLineEdit* m_CurveNameEdit = nullptr;
    QPushButton* m_ImportCurve = nullptr;
    QLineEdit* m_LineEdit_DELAY = nullptr;
    QPushButton* m_TableBtn = nullptr;
    QPushButton* m_ChartBtn = nullptr;
    QTabWidget* m_tabWidget = nullptr;
    QTableWidget* m_TableWidget = nullptr;
    QPushButton* m_SaveBtn = nullptr;
    QPushButton* m_CloseBtn = nullptr;
    QWidget* m_PlotView = nullptr;
    mPlotView* m_Plot1 = nullptr;
};
/*****************************************CreatePSDCurveDialog******************************************************/
class CreatePSDCurveDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CreatePSDCurveDialog(QWidget* parent = nullptr);
    ~CreatePSDCurveDialog();
    void initDialog();
    void setDialogData(IncentiveCurve data);
    /*
      * 获取对话框当前编辑的曲线数据
    */
    IncentiveCurve getDialogData() const;
    /*
      * 获取单例指针
    */
    static CreatePSDCurveDialog* getInstance();
    bool isDataValid();
private slots:
    void onSave();
    void onClose();
    void onPlotCurve();
    void onImportCurve();
private:
    void setupUI();
    //单例指针
    static CreatePSDCurveDialog* _instance;
    QLineEdit* m_CurveNameEdit = nullptr;
    QPushButton* m_ImportCurve = nullptr;
    QComboBox* m_ComboBox_Freq = nullptr;
    QComboBox* m_ComboBox_Load = nullptr;
    QPushButton* m_TableBtn = nullptr;
    QPushButton* m_ChartBtn = nullptr;
    QTabWidget* m_tabWidget = nullptr;
    QTableWidget* m_TableWidget = nullptr;
    QPushButton* m_SaveBtn = nullptr;
    QPushButton* m_CloseBtn = nullptr;
    QWidget* m_PlotView = nullptr;
    mPlotView* m_Plot1 = nullptr;
    AxisSettings m_bottomAxis;
    AxisSettings m_leftAxis;
};
/*****************************************ComboDeleteItemDelegate******************************************************/
class QPainter;
class ComboDeleteItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit ComboDeleteItemDelegate(QObject* parent = nullptr);

    /*
      * 设置删除按钮图标；skipFirstRow 为 true 时第一行（固定项，如“无”）不显示按钮
    */
    void setDeleteIcon(const QIcon& icon, bool skipFirstRow = true);

signals:
    // 点击了某行的删除按钮
    void deleteRequested(const QModelIndex& index);

protected:
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index) override;

private:
    QRect deleteRect(const QStyleOptionViewItem& option) const;
    QIcon m_deleteIcon;
    bool m_skipFirstRow = true;
};
/*****************************************IncentiveSettingTableWidget******************************************************/
class IncentiveSettingTableWidget : public QTableWidget
{
    Q_OBJECT

public:
    explicit IncentiveSettingTableWidget(bool singCurve, QWidget* parent = nullptr);
    ~IncentiveSettingTableWidget();
    // 对外公开接口
    void addRow();

    /*
    * 恢复为初始默认状态：清空所有行并重建默认行
    */
    void resetToDefault();
    void removeSelectedRows();
    void setRowCount(int count);
    int getRowID(int row) const;

    bool isItemChecked(int row,int col);
    IncentiveCurve getItemData(int row, int col);
    double getWeightValue(int row, int col);
private:
    void initRow(int row);
    void setupAppearance();
    void refreshHeader();
    bool m_SingleCurve = false;
};

/*****************************************ButtonHeaderView******************************************************/
class ButtonHeaderView : public QHeaderView
{
    Q_OBJECT
public:
    explicit ButtonHeaderView(Qt::Orientation orientation, QWidget* parent = nullptr);

    // 设置第4列（索引3）的按钮图标和文字
    void setButtonIcon(const QIcon& icon);
    void setText(const QString& text);

signals:
    // 第4列按钮点击信号，参数为列索引（固定为3）
    void buttonClicked(int logicalIndex);

protected:
    void paintSection(QPainter* painter, const QRect& rect, int logicalIndex) const override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    QIcon m_buttonIcon;      // 按钮图标
    QString m_text;          // 表头文字
    int m_buttonColumn;      // 需要放置按钮的列索引（这里为3）
    QRect buttonRect(const QRect& sectionRect) const; // 计算按钮区域
};

#endif
