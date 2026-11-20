#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include "mBasicEnum.h"
#include "mPanelBase.h"

#include <memory>
#include <set>

//QT
#include <QWidget>
#include <QComboBox>

using namespace MxFunctions;
namespace MechStructOptionData
{
	class mRunnerOptimizationGoalData;
}
namespace MechMainWindow
{
	//流道优化目标面板
	class MMAINWINDOW_EXPORT mRunnerOptimizationGoalPanel : public mPanelBase
	{
		Q_OBJECT

	public:
		/*
		* 默认构造函数
		*/
		mRunnerOptimizationGoalPanel(QWidget* parent = Q_NULLPTR);

		/*
		* 析构函数
		*/
		~mRunnerOptimizationGoalPanel();

		/*
		* 编辑流道优化目标
		*/
		void editRunnerOptimizationGoalPanel(const QString& responseType = "");

		/*
		* 取消编辑流道优化目标
		*/
		void uneditRunnerOptimizationGoalPanel();

	private slots:

        /*
        * 优化方式改变
        */
        void slot_objectiveTypeChanged(const QString& text);

        /*
        * 优化响应改变
        */
        void slot_responseTypeChanged(const QString& text);

	protected:

		//数据
        MechStructOptionData::mRunnerOptimizationGoalData* _optimizationGoalData;

		//总布局
		QVBoxLayout* _mainLayout;

        // 优化方式下拉框
        QComboBox* _comboObjectiveType;

        // 优化响应下拉框
        QComboBox* _comboResponseType;
 };
}
