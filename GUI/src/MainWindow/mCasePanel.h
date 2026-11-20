#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

//QT
#include <QWidget>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QHash>
#include <QLabel>

#include "mPanelBase.h"

namespace Ui
{
	class mCasePanel;
}

namespace MechMainWindow
{
	class MMAINWINDOW_EXPORT mCasePanel : public mPanelBase
	{
		Q_OBJECT

	public:
		/*
		* 默认构造函数
		*/
		mCasePanel(QWidget *parent = Q_NULLPTR);

		/*
		* 析构函数
		*/
		~mCasePanel();

		/*
		* 设置数据
		*/
		void setCaseData(QString name, int Type);
		
	private:
		//UI
		Ui::mCasePanel* _ui;

		//分析类型显示
		QLabel* _caseTypeLabel;

	};
}