#pragma once
#pragma execution_character_set("utf-8")

#include "mPanelBase.h"
#include <QtWidgets>


QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

namespace MechFunctionData { class mFunctionBase; }

namespace MechMainWindow
{
    class mTableWidget;
    class mFunctionPanel : public mPanelBase
    {
        Q_OBJECT
    public:
        explicit mFunctionPanel(QWidget* parent = nullptr);
        ~mFunctionPanel();

        /* 外部旧入口 */
        void newFunctionPanel(int type);
        /* 获取QCombobox*/
        QComboBox* getFunctionTypeBox();

        void editFunctionPanel(int funcId);
        //刷新面板
        void updatePanel();
        // 唯一的数据同步槽
        void syncDataToFuncData();  
    public slots:
        void onTypeChanged(int);   // 类型切换

    private slots:
        void onAutoRefresh();      // 300 ms 防抖刷新
       

        /* 离散页 5 按钮 */
        void onInsertRow();
        void onDeleteRow();
        void onMoveUp();
        void onMoveDown();
        void onTrashRow();

        /* 导出按钮 */

        void onExportAnalytic();
        void onExportDiscrete();
        void onExportInterp();
        void onExportPiecewise();
        void onExportStep();
        /* 插值页 5 按钮 */
        void onInterpAddRow();
        void onInterpDelRow();
        void onInterpMoveUp();
        void onInterpMoveDown();
        void onInterpTrashRow();

        /* 分段页 5 按钮 */
        void onPieceAddRow();
        void onPieceDelRow();
        void onPieceMoveUp();
        void onPieceMoveDown();
        void onPieceTrashRow();
        /* 表格页按钮 */
        void onTableAddRow();
        void onTableDelRow();
        void onTableMoveUp();
        void onTableMoveDown();
        void onTableExport();
        void doTable();
        void doPowerTable();
        /* 周期页按钮 */
        void onPeriodicAddRow();
        void onPeriodicDelRow();
        void onPeriodicMoveUp();
        void onPeriodicMoveDown();
        void doPeriodic();
        /* 调制函数 */
        void doModulated();
        /* 指数衰减函数 */
        void doExpDecay();
        bool eventFilter(QObject* obj, QEvent* ev) override;
        //清空数据按钮，也就是对应面板里面的删除
        //void onClearRows();

    private:
        void buildUI();            // 纯代码构造与旧 ui 完全一致
        void initData();           //初始化界面上的数据
        void connectSignals();     // 信号槽
        void setupExtraControls(); // 采样点数、表头等

        /* 五种函数实现（完整搬迁） */
        void doAnalytic();
        void doDiscrete();
        void doInterp();
        void doPiecewise();
        bool checkPieceContinuity();
        void doStep();

        MechFunctionData::mFunctionBase* _funcData = nullptr;

        /* ====== 折叠根节点（仅布局，不影响控件名） ====== */
        FoldSection* m_typeFold = nullptr;
        FoldSection* m_varFold = nullptr;
        FoldSection* m_rangeFold = nullptr;
        FoldSection* m_exprFold = nullptr;
        FoldSection* m_discFold = nullptr;
        FoldSection* m_interpFold = nullptr;
        FoldSection* m_extrapFold = nullptr;
        FoldSection* m_pieceFold = nullptr;
        FoldSection* m_stepFold = nullptr;
        FoldSection* m_statFold = nullptr;
        FoldSection* m_interpDataFold = nullptr;   // 新增
        FoldSection* m_tableDataFold = nullptr;    // 表格数据
        FoldSection* m_tableFold = nullptr;        // 表格参数
        FoldSection* m_periodicDataFold = nullptr; // 周期函数数据
        FoldSection* m_periodicParamFold = nullptr; // 周期函数参数

        /* ====== 子控件（名字与旧 .ui 完全一致） ====== */
        QComboBox* comboBox = nullptr;
        QLabel* label = nullptr;
        QLabel* label_2 = nullptr;
        QLabel* label_5 = nullptr;
        QLabel* label_3 = nullptr;
        QLabel* label_6 = nullptr;
        QLineEdit* lineEdit = nullptr;
        QLineEdit* lineEdit_2 = nullptr;
        QSpinBox* spinSamplesAna = nullptr;
        QLabel* label_4 = nullptr;
        QLineEdit* lineEdit_3 = nullptr;
        QPushButton* pushButton_10 = nullptr;
        QPushButton* pushButton = nullptr;
        mTableWidget* tableWidget = nullptr;
        QPushButton* pushButton_5 = nullptr;
        QPushButton* pushButton_6 = nullptr;
        QPushButton* pushButton_7 = nullptr;
        QPushButton* pushButton_8 = nullptr;
        QPushButton* pushButton_9 = nullptr;
        QPushButton* pushButton_2 = nullptr;
        QPushButton* pushButton_3 = nullptr;
        QPushButton* pushButton_4 = nullptr;
        QTableWidget* tableWidget_2 = nullptr;
        QLabel* label_13 = nullptr;
        QComboBox* comboBox_4 = nullptr;
        QLabel* label_14 = nullptr;
        QComboBox* comboBox_5 = nullptr;
        QTableWidget* tableWidget_3 = nullptr;
        QLabel* label_8 = nullptr;
        QLabel* label_7 = nullptr;
        QLabel* label_9 = nullptr;
        QLineEdit* lineEdit_4 = nullptr;
        QLabel* label_23 = nullptr;
        QLineEdit* lineEdit_9 = nullptr;
        QLabel* label_24 = nullptr;
        QLineEdit* lineEdit_10 = nullptr;
        QPushButton* pushButton_11 = nullptr;
        QPushButton* pushButton_12 = nullptr;
        QPushButton* pushButton_13 = nullptr;
        QPushButton* pushButton_14 = nullptr;
        /* ===== 分段页按钮 ===== */
        QPushButton* pushButton_addPiece = nullptr;
        QPushButton* pushButton_delPiece = nullptr;
        QPushButton* pushButton_upPiece = nullptr;
        QPushButton* pushButton_downPiece = nullptr;
        QPushButton* pushButton_trashPiece = nullptr;
        /* ===== 插值专用控件 ===== */
        mTableWidget* tableWidget_interp = nullptr;   // 插值自己的表格
        QPushButton* pushButton_exportInterp = nullptr; // 插值导出按钮
        // 新增 5 个独立按钮
        QPushButton* pushButton_addInterp = nullptr;
        QPushButton* pushButton_delInterp = nullptr;
        QPushButton* pushButton_upInterp = nullptr;
        QPushButton* pushButton_downInterp = nullptr;
        QPushButton* pushButton_exportInterpOnly = nullptr;
        QPushButton* pushButton_trashInterp = nullptr;
        QPushButton* pushButton_plotInterp = nullptr;
        QPushButton* pushButton_drawInterp = nullptr;
        /* ====== 表格函数控件 ====== */
        mTableWidget* tableWidget_table = nullptr;   // 表格函数表格
        QComboBox* comboBox_xAxisType = nullptr;     // x轴类型（线性/对数）
        QComboBox* comboBox_yAxisType = nullptr;     // y轴类型（线性/对数）
        QComboBox* comboBox_extrapTable = nullptr;   // 外推方式（常数/线性）
        QLabel* m_labXAxisType = nullptr;            // x轴类型标签（用于动态显隐）
        QLabel* m_labYAxisType = nullptr;            // y轴类型标签（用于动态显隐）
        QLabel* m_labExtrapTable = nullptr;          // 外推方式标签（用于动态显隐）
        QLabel* m_labX1 = nullptr;                   // 偏移量x1标签
        QLineEdit* m_lineEdit_x1 = nullptr;          // 偏移量x1输入框
        QLabel* m_labX2 = nullptr;                   // 缩放值x2标签
        QLineEdit* m_lineEdit_x2 = nullptr;          // 缩放值x2输入框
        QLabel* m_labX3 = nullptr;                   // 横坐标下限x3标签
        QLineEdit* m_lineEdit_x3 = nullptr;          // 横坐标下限x3输入框
        QLabel* m_labX4 = nullptr;                   // 横坐标上限x4标签
        QLineEdit* m_lineEdit_x4 = nullptr;          // 横坐标上限x4输入框
        QPushButton* pushButton_addTable = nullptr;
        QPushButton* pushButton_delTable = nullptr;
        QPushButton* pushButton_upTable = nullptr;
        QPushButton* pushButton_downTable = nullptr;
        QPushButton* pushButton_drawTable = nullptr;
        QPushButton* pushButton_exportTable = nullptr;
        /* ====== 周期函数控件 ====== */
        mTableWidget* tableWidget_periodic = nullptr; // 周期函数表格
        QPushButton* pushButton_addPeriodic = nullptr;
        QPushButton* pushButton_delPeriodic = nullptr;
        QPushButton* pushButton_upPeriodic = nullptr;
        QPushButton* pushButton_downPeriodic = nullptr;
        QLabel* m_labPeriodicFreq = nullptr;
        QLineEdit* m_lineEditPeriodicFreq = nullptr;
        QLabel* m_labPeriodicT0 = nullptr;
        QLineEdit* m_lineEditPeriodicT0 = nullptr;
        QLabel* m_labPeriodicA0 = nullptr;
        QLineEdit* m_lineEditPeriodicA0 = nullptr;
        QLabel* m_labPeriodicFormula = nullptr;
        /* ====== 调制函数控件 ====== */
        FoldSection* m_modulatedFold = nullptr;      // 调制函数参数
        QLabel* m_labModulatedA0 = nullptr;
        QLineEdit* m_lineEditModulatedA0 = nullptr;
        QLabel* m_labModulatedA = nullptr;
        QLineEdit* m_lineEditModulatedA = nullptr;
        QLabel* m_labModulatedT0 = nullptr;
        QLineEdit* m_lineEditModulatedT0 = nullptr;
        QLabel* m_labModulatedF1 = nullptr;
        QLineEdit* m_lineEditModulatedF1 = nullptr;
        QLabel* m_labModulatedF2 = nullptr;
        QLineEdit* m_lineEditModulatedF2 = nullptr;
        QLabel* m_labModulatedFormula = nullptr;
        /* ====== 指数衰减函数控件 ====== */
        FoldSection* m_expDecayFold = nullptr;      // 指数衰减函数参数
        QLabel* m_labExpDecayA0 = nullptr;
        QLineEdit* m_lineEditExpDecayA0 = nullptr;
        QLabel* m_labExpDecayAm = nullptr;
        QLineEdit* m_lineEditExpDecayAm = nullptr;
        QLabel* m_labExpDecayT0 = nullptr;
        QLineEdit* m_lineEditExpDecayT0 = nullptr;
        QLabel* m_labExpDecayTd = nullptr;
        QLineEdit* m_lineEditExpDecayTd = nullptr;
        QLabel* m_labExpDecayFormula = nullptr;
        /* ====== 防抖 ====== */
        static QTimer* g_debounce;
    };
} // namespace MechMainWindow
