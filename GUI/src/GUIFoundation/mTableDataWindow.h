#pragma once

//解决中文乱码
#pragma execution_character_set("utf-8")
#include "guifoundation_global.h"

#include "mDockBase.h"
#include <QFrame>
#include <QSize>
#include <QTableWidget>
#include <QAction>
#include <QMenu>
#include <QList>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QStyledItemDelegate>
#include <QMap>
#include <QSet>
#include <QSignalBlocker>

namespace MxFunctions
{
    class mCalcLineEdit;
}

namespace MechMainWindow
{
	// 前向声明
	class NumericItemDelegate;
	class mTableWidget;

	class GUIFOUNDATION_EXPORT mTableDataWindow : public QWidget
	{
		Q_OBJECT
	public:
		/*
		* 列输入校验类型
		*/
		enum class ColumnValidationType {
			NoValidation,         // 无特殊校验（默认只允许实数，禁止其他字符）
			Integer,              // 任意整数
			PositiveInteger,      // 正整数  (>0)
			NonNegativeInteger,   // 非负整数 (>=0)
			NonPositiveInteger,   // 非正整数 (<=0)
			NegativeInteger,      // 负整数  (<0)
			Real,                 // 任意实数
			PositiveReal,         // 正实数  (>0.0)
			NonNegativeReal,      // 非负实数 (>=0.0)
			NonPositiveReal,      // 非正实数 (<=0.0)
			NegativeReal          // 负实数  (<0.0)
		};

		/*
		* 获取单例指针
		*/
		static mTableDataWindow* getInstance();

		/*
		* 析构函数
		*/
		~mTableDataWindow();

        /*
        * 设置自定义表格显示
        */
        void setCustomWidget(QWidget* widget);

        /*
        * 设置初始表格显示
        */
        void resetInitialTable();

        //設置表標題和曲線標題
        void setTableTitle(const QString &title);

        //獲取表標題
        QString getTableTitle();

        //清空数据并保留结构（列）
        void clearAll();                                     

        // 清空并重置尺寸
        void clearAndReset(int rows, int cols);             

        //獲取表格數據
        void getTableData(QVector<QVector<double>>& x, QVector<QVector<double>>& y);
       
        //設置表頭
        void setHorizontalHeaders(const QStringList& labels);

        //設置初始數據
        void setTableData(const QVector<QVector<double>>& x, const QVector<QVector<double>>& y);
        void setTableData(const QVector<QVector<double>>& datas);


        //获取横向表头
        QStringList getTableHeader();

        //设置使用曲线
        void setUseCurve(bool use);

        //获取tablewidget
        mTableWidget* getTableWidget();

        //设置指定列的输入校验类型
        void setColumnValidation(int col, ColumnValidationType type);

        //批量设置多列的输入校验类型
        void setColumnValidations(const QMap<int, ColumnValidationType>& validations);

        //清除所有列的输入校验
        void clearColumnValidations();

    signals:

        //數據改變之後發送的信號
        void tableDataChangedSig(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title);

	private:
		/*
		* 默认构造函数
		*/
		mTableDataWindow(QWidget* parent = 0);

        /*
        * 獲取表格數據
        */
        void getTableData(QVector<QVector<double>> &x, QVector<QVector<double>> &y, QStringList& headers);

        // 工具函数：严格验证字符串是否为有效数字
        static bool parseValidDouble(const QString& text, double& value);

        //獲取y表頭
        QStringList getHorizontalHeaders();

	private slots:
		//切换语言
		void slot_changeLanguage(int language);

	private:
		//单例指针
		static mTableDataWindow* _instance;

        QString _title;//標題

        QScrollArea *_scrollArea;

        QWidget* _dynamicControls;

        //外部自定义控件
        QWidget* _customWidget;

        //表格
        mTableWidget* _tableWidget;

        //布局
        QVBoxLayout* _mainLayout;

        // 行数输入控件
        MxFunctions::mCalcLineEdit* _rowInput = nullptr;

        bool _usecurve = true;

        // 同步行数输入框与表格当前行数
        void syncRowInputToTable();
 };

    class GUIFOUNDATION_EXPORT mTableWidget : public QTableWidget
    {
        Q_OBJECT
    public:
        enum class SizeMode {
            FixedSize,   // 固定行列数；隐藏 + / - / clear/行数；回车仅换到下一行（若无下一行则停留）
            DynamicSize  // 动态行列数；支持 + / - / clear；回车到下一行，没有则自动新增一行
        };
        Q_ENUM(SizeMode)

        explicit mTableWidget(QWidget* parent = nullptr);

        // === 外部调用 API ===
        void setSizeMode(SizeMode mode);
        SizeMode sizeMode() const { return m_mode; }

        // 一次性设置固定的行数
        void setFixedRow(int rows);

        // 动态模式：外部可控制增删清空（也可通过内置右键菜单操作）
        int  addRow(int whereRow = -1);                      // 返回新增行索引（默认末尾）
        void removeSelectedRows();
        void clearAll();                                     // 清空数据并保留结构（列）
        void clearAndReset(int rows, int cols);              // 清空并重置尺寸

        // 批量设置/获取单元格文本
        void setCellText(int row, int col, const QString& text);
        QString cellText(int row, int col) const;

        // 便捷：设置列头、行头
        void setHorizontalHeaders(const QStringList& labels);
        void setVerticalHeaders(const QStringList& labels);
        void rebuildConnections(bool enable);

        // 将当前单元格输入值同步到所有选中单元格
        void applyValueToSelectedRanges(int sourceColumn, const QString& text);

        // 设置指定列的输入校验类型
        void setColumnValidation(int col, mTableDataWindow::ColumnValidationType type);

        // 批量设置多列的输入校验类型
        void setColumnValidations(const QMap<int, mTableDataWindow::ColumnValidationType>& validations);

        // 清除所有列的输入校验
        void clearColumnValidations();

        // 获取指定列的输入校验类型
        mTableDataWindow::ColumnValidationType columnValidation(int col) const;

    signals:
        // 数据更新信号：任何单元格文本变化都会发出
        void cellDataUpdated(int row, int col, const QString& newValue);

        // 结构变化相关
        void tableResized(int rows, int cols);
        void rowsAdded(const QList<int>& newRowIndexes);   // 一次可加多行时使用
        void rowsRemoved(const QList<int>& removedRowIndexes);

        // 操作反馈
        void cleared();                  // clearAll / clearAndReset
        void modeChanged(mTableWidget::SizeMode mode);
        void enterNavigated(int fromRow, int toRow, int col); // 回车跳转的行号反馈

    protected:
        // 鼠标行为：多选状态下单击已选单元格不丢失选区，直接触发编辑
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;

        // 键盘行为：实现回车逻辑与互斥模式的差异
        void keyPressEvent(QKeyEvent* event) override;

        // 右键菜单：供用户选择固定尺寸、切换模式、+ / - / clear
        void contextMenuEvent(QContextMenuEvent* event) override;

    private slots:
        void onItemChanged(QTableWidgetItem* item);

    private:
        void initUi();
        void initActions();


        void ensureAtLeastOneRowForDynamic(int atLeast = 1);
        void emitRowsAddedSignal(const QList<int>& indexes);
        void emitRowsRemovedSignal(const QList<int>& indexes);

    private:
        SizeMode m_mode = SizeMode::DynamicSize;

        // 右键菜单动作
        QMenu* m_ctxMenu = nullptr;
        QAction* m_actAddRow = nullptr;
        QAction* m_actRemoveRows = nullptr;
        QAction* m_actClear = nullptr;
        QAction* m_actSetFixed = nullptr;
        QAction* m_actSwitchToDynamic = nullptr;

        bool m_blockItemSignal = false; // 内部更新时，阻断 itemChanged 的回传

        // 列校验规则
        QMap<int, mTableDataWindow::ColumnValidationType> m_columnValidations;

        // 鼠标按下时记录的索引，用于在释放时判断是否需要触发编辑
        QModelIndex m_lastPressedIndex;
    };

    class GUIFOUNDATION_EXPORT NumericItemDelegate : public QStyledItemDelegate
    {
        Q_OBJECT
    public:
        explicit NumericItemDelegate(mTableWidget* tableWidget, QObject* parent = nullptr);

        void setValidationType(mTableDataWindow::ColumnValidationType type);
        mTableDataWindow::ColumnValidationType validationType() const;

        QWidget* createEditor(QWidget* parent,
            const QStyleOptionViewItem& option,
            const QModelIndex& index) const override;

        void setEditorData(QWidget* editor,
            const QModelIndex& index) const override;

        void setModelData(QWidget* editor,
            QAbstractItemModel* model,
            const QModelIndex& index) const override;

    private:
        mTableWidget* m_tableWidget;
        mTableDataWindow::ColumnValidationType m_validationType = mTableDataWindow::ColumnValidationType::NoValidation;
    };
}
