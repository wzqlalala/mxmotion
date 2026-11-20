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
#include <QComboBox>

#include "mPanelBase.h"
#include "mPhysicalFieldEnum.h"

using namespace MxFunctions;
namespace MechPreData
{
	class mViewportModel;
}
namespace MechStructOptionData
{
	class mRunnerOptimizationConstraintData;
}
namespace MechMainWindow
{
	//流道优化约束面板
	class MMAINWINDOW_EXPORT mRunnerOptimizationConstraintPanel : public mPanelBase
	{
		Q_OBJECT

	public:
		/*
		* 默认构造函数
		*/
		mRunnerOptimizationConstraintPanel(QWidget* parent = Q_NULLPTR);

		/*
		* 析构函数
		*/
		~mRunnerOptimizationConstraintPanel();

        /*
        * 创建流道优化约束
        */
        void createRunnerOptimizationConstraint(const QString& type = "");

		/*
		* 编辑流道优化约束(从树上点击过来的)
		*/
		void editRunnerOptimizationConstraintPanel(int id);

		/*
		* 取消编辑流道优化约束
		*/
		void uneditRunnerOptimizationConstraintPanel();

	private:

        //更新属性下拉框
        void updatePropertyCombobox(QComboBox* combobox);


	private slots:

        /*
        * 约束类型改变
        */
        void slot_constraintTypeChanged(const QString& text);

        /*
        * 约束方式改变
        */
        void slot_constraintModeChanged(const QString& text);

        /*
        * 截面属性复选框改变
        */
        void slot_checkPropertyChanged(int state);

        /*
        * 截面属性下拉框改变
        */
        void slot_propertyChanged(const QString& text);

	protected:

		//数据
        MechStructOptionData::mRunnerOptimizationConstraintData* _optimizationConstraintData;

		//总布局
		QVBoxLayout* _mainLayout;

        QCheckBox* m_checkUpperBound{ nullptr }; ///< 约束上限开关
        mCalcLineEdit* m_lineUpperBound{ nullptr }; ///< 约束上限输入框

        QCheckBox* m_checkLowerBound{ nullptr }; ///< 约束下限开关
        mCalcLineEdit* m_lineLowerBound{ nullptr }; ///< 约束下限输入框

        QComboBox* m_comboConstraintType{ nullptr }; ///< 约束类型下拉框
        QComboBox* m_comboConstraintMode{ nullptr }; ///< 约束方式下拉框（流体/固体）

        //截面属性
        QCheckBox* m_checkProperty;
        PanelLabel* m_lableProperty{ nullptr };
        QComboBox* m_comboProperty;
	};
}
