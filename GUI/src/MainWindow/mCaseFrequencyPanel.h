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

#include "mMaterialPropBase.h"
#include "mFreqTableSelectorWidget.h"

using namespace MxFunctions;
namespace MechMainWindow
{
	//频响分析面板
	class MMAINWINDOW_EXPORT mCaseFrequencyPanel :public mCaseSettingPanel
	{
	public:

		mCaseFrequencyPanel(QWidget* parent = Q_NULLPTR);

		~mCaseFrequencyPanel();

		void newCasePanel() override;

		//根据数据初始化面板
		void updatePanel() override;
    public slots:
        void slot_btnUse(bool checked);
        void slot_freqSampPointTable();

        void onRowAdded(int row);
        void onRowRemoved(int row);
        void onComboChanged(int row, const QString& text);

    private:
        //表格
        singleCheckBox* tableBtn = nullptr;
        mFreqTableSelectorWidget* table = nullptr;

	};
}