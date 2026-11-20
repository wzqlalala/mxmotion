#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include "mxr/data/SelectDataStruct.h"
#include "mBasicEnum.h"

#include <memory>
#include <set>

//QT
#include <QWidget>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QHash>
#include <QGridLayout>
#include <QLabel>
#include <QToolButton>
#include <QMdiSubWindow>
#include <QTableWidget>

#include "mPanelBase.h"
#include "mPhysicalFieldEnum.h"

using namespace MxFunctions;
namespace MechPreData
{
	class mViewportModel;
}
namespace MechStructOptionData
{
	class mOptimizationConstraintData;
}
namespace MechMainWindow
{
	struct OptGoalPosPickWidget;
	//结构优化约束面板
	class MMAINWINDOW_EXPORT mOptimizationConstraintPanel : public mPanelBase
	{
		Q_OBJECT

	public:
		/*
		* 默认构造函数
		*/
		mOptimizationConstraintPanel(QWidget* parent = Q_NULLPTR);

		/*
		* 析构函数
		*/
		~mOptimizationConstraintPanel();

        /*
        * 创建结构优化约束
        */
        void createOptimizationConstraint(const QString& type = "");

		/*
		* 编辑结构优化约束(从树上点击过来的)
		*/
		void editOptimizationConstraintPanel(int id);

		/*
		* 取消编辑结构优化约束
		*/
		void uneditOptimizationConstraintPanel();

	private:

        //更新属性下拉框
        void updatePropertyCombobox(QComboBox* combobox);

        //更新工况下拉框(响应类型约束使用，与优化目标面板一致)
        void updateCaseCombobox(QComboBox* combobox, QList<CaseType> caseType = { ImplicitLinearStatic });

        //更新柔度表格
        void updateComplianceTable(bool deleteMap, QList<CaseType> caseType = { ImplicitLinearStatic });

        //更新频率表格
        void updateFrequencyTable(bool deleteMap);

        //初始化频率表格
        void initFrequencyTable();

        //表格数据改变信号连接与断开
        void connectFrequencyTableSig();
        void disconnectFrequencyTableSig();

        //回显节点选择状态(与优化目标面板OptGoalPosPickWidget::setState一致)
        void updatePickNodeState();

	private slots:

        /*
        * 截面属性下拉框改变
        */
        void slot_constraintTypeChanged(const QString& text);

        /*
        * 截面属性复选框改变
        */
        void slot_checkPropertyChanged(int state);

        /*
        * 截面属性下拉框改变
        */
        void slot_propertyChanged(const QString& text);

        /*
        * 工况改变(响应类型约束使用，与优化目标面板一致)
        */
        void slot_caseChanged(const QString& text);

        /*
        * 自由度改变
        */
        void slot_dofChanged(const QString& text);

        /*
        * 获取节点
        */
        void slot_getNode();

        /*
        * 清空节点
        */
        void slot_clearNode();

        /*
        * 替换节点
        */
        void slot_checkNode(int state);

        /*
        * 频率表格数据改变
        */
        void slot_lineChanged(QTableWidgetItem* item);

        /*
        * 频率表格添加一行
        */
        void slot_addLine();

        /*
        * 频率表格删除多行
        */
        void slot_deleteLine();

        /*
        * 频率表格清空
        */
        void slot_clearTable();

        /*
        * 柔度复选框改变
        */
        void slot_complianceCheckBox(bool isCheck);

        /*
        * 频率复选框改变
        */
        void slot_frequencyCheckBox(bool isCheck);

        /*
        * 目标频率类型改变
        */
        void slot_frequencyTargetTypeChanged(const QString& text);

        /*
        * 指定频率点改变
        */
        void slot_specifiedFrequencyChanged();

        /*
        * 频响优化类型改变
        */
        void slot_frfValueTypeChanged(const QString& text);

	protected:

		//数据
        MechStructOptionData::mOptimizationConstraintData* _optimizationConstraintData;

		//总布局
		QVBoxLayout* _mainLayout;

        QCheckBox* m_checkUpperBound{ nullptr }; ///< 约束上限开关
        mCalcLineEdit* m_lineUpperBound{ nullptr }; ///< 约束上限输入框

        QCheckBox* m_checkLowerBound{ nullptr }; ///< 约束下限开关
        mCalcLineEdit* m_lineLowerBound{ nullptr }; ///< 约束下限输入框

        QComboBox* m_comboConstraintType{ nullptr }; ///< 约束类型下拉框

        //截面属性
        QCheckBox* m_checkProperty;
        PanelLabel* m_lableProperty{ nullptr };
        QComboBox* m_comboProperty;

        //****************响应类型约束控件(与优化目标面板保持一致)****************
        //工况
        QComboBox* m_comboCase{ nullptr };

        //节点
        OptGoalPosPickWidget* m_pickNode{ nullptr };

        //自由度
        QComboBox* m_comboDof{ nullptr };

        //目标阶数
        mCalcLineEdit* m_orderEdit{ nullptr };

        //目标频率类型(指定频率点/最大响应点/最小响应点/平均/求和/RMS)
        QComboBox* m_comboFrequencyTargetType{ nullptr };

        //指定频率点
        mCalcLineEdit* m_editSpecifiedFrequency{ nullptr };

        //PSD/频响自由度专用下拉框(1-6)
        QComboBox* m_comboDofDynamic{ nullptr };

        //频响位移优化类型(幅值/实部/虚部)
        QComboBox* m_comboFrfValueType{ nullptr };

        //复用标签
        PanelLabel* m_labelCase{ nullptr };
        PanelLabel* m_labelFrequency{ nullptr };
        PanelLabel* m_labelNode{ nullptr };
        PanelLabel* m_labelDof{ nullptr };
        PanelLabel* m_labelSpecifiedFrequency{ nullptr };
        PanelLabel* m_labelFrfValueType{ nullptr };

        //综合柔度
        QTableWidget* m_tableCompliance{ nullptr };

        //综合频率
        QTableWidget* m_tableFrequency{ nullptr };
        QWidget* m_tablePara{ nullptr };
        QPushButton* m_addBtn{ nullptr };
        QPushButton* m_reduceBtn{ nullptr };
        QPushButton* m_clearBtn{ nullptr };
        QMetaObject::Connection m_tableChangeConn;

        //柔度指数
        QCheckBox* m_complianceCheck{ nullptr };
        QCheckBox* m_frequencyCheck{ nullptr };
	};
}