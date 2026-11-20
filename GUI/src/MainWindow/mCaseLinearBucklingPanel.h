#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include <memory>

//QT
#include <QWidget>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QHash>
#include <QGridLayout>
#include <QMdiSubWindow>
#include <QStringListModel>
#include <QTreeWidget>
#include <QLineEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QFrame>
#include <QToolButton>

#include "mCaseSettingPanel.h"

#include "mPhysicalFieldEnum.h"

using namespace MxFunctions;
namespace MechMainWindow
{
	//线性屈曲分析面板
	class MMAINWINDOW_EXPORT mCaseLinearBucklingPanel :public mCaseSettingPanel
	{
	public:

		mCaseLinearBucklingPanel(QWidget* parent = Q_NULLPTR);

		~mCaseLinearBucklingPanel();

		void newCasePanel() override;

		//根据数据初始化面板
		void updatePanel() override;
	};
}