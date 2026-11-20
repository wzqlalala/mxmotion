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
#include <QStackedWidget>
#include <QListWidget>
#include <QLabel>
#include <QTableWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QToolButton>
#include <QVBoxLayout>
#include <QRadioButton>
#include <QButtonGroup>

#include "mCalcLineEdit.h"
#include "mPanelBase.h"
#include "mPhysicalFieldEnum.h"
#include "mPickPointWidget.h"
#include "mPickWidgetController.h"
#include "mDirectionSelectorWidget.h"

using namespace MxFunctions;
using namespace MechCommon;
;
namespace MechContactData
{
	class mPeriodicBoundary;
}
namespace MechPreData
{
	class mViewportPrivate;
}

namespace MechMainWindow
{
	//周期性边界面板（平移/旋转共用）
	class MMAINWINDOW_EXPORT mPeriodicBoundaryPanel : public mPanelBase
	{
		Q_OBJECT

	public:
		/*
		* 默认构造函数
		*/
		mPeriodicBoundaryPanel(QWidget* parent = Q_NULLPTR);

		/*
		* 析构函数
		*/
		~mPeriodicBoundaryPanel();

		/*
		* 新建周期性边界
		*/
		void newPeriodicBoundaryPanel(MxFunctions::PeriodicBoundaryType type);

		/*
		* 编辑周期性边界
		*/
		void editPeriodicBoundaryPanel(int pbID = 0);

		/*
		* 取消编辑
		*/
		void uneditPeriodicBoundaryPanel();

		//周期边界数据即将被外部销毁（树上删除周期边界/删除全部）时的安全收尾：
		//必须在 deleteByPhysicalType 之前调用；传 -1 表示即将删除全部周期边界
		void onPeriodicBoundaryDataAboutToBeDestroyed(int pbID);

	private slots:
		//类型下拉框变化
		void slot_typeComboBoxChanged(int index);
        // 属性槽函数
        void slot_autoCalcToggled(bool checked);
        void slot_manualValueEditingFinished();
        void slot_adjustToggled(bool checked);
	protected:
		/*
		* 初始化界面
		*/
		void initUI();

		/*
		* 加载数据到界面
		*/
		void loadDataToUI();

		/*
		* 更新界面
		*/
		void updatePanel();

		// 更新显隐
		void updateFoldVisibility();
		void updateRowVisibility();

		// 从集合加载主/从面数据到 PickWidgetData
		void loadPeriodicBoundaryDataFromSet(MxFunctions::MapRelationType mapType, PickWidgetData& outData);

		// 提交主/从面拾取数据并刷新高亮
		void slot_commitMasterPeriodicBoundaryData();
		void slot_commitSlavePeriodicBoundaryData();

		// 清空临时高亮
		void clearFaceHighlight();

	protected:
		//周期性边界数据
		MechContactData::mPeriodicBoundary* _pbData = nullptr;

		//当前视口数据
		MechPreData::mViewportPrivate* _currentViewportPrivate = nullptr;

		//总布局
		QVBoxLayout* _mainLayout = nullptr;

		// ================= 控件定义 =================
		// 0. 类型
		QComboBox* cmb_type = nullptr;

		// 1. 边界面
		mPickWidgetController* pick_master = nullptr;
		mPickWidgetController* pick_slave = nullptr;
		QToolButton* btn_swap = nullptr;

		// 2. 方向/轴
		mDirectionSelectorWidget* _directionWidget = nullptr;

		// 方向箭头渲染
		void updateDirectionArrowRender();
		static constexpr const char* s_dirArrowRenderKey = "pb_direction_arrow";
		static const QVector3D s_dirArrowColor;

		// 方向控件状态与 _pbData 互相保存/反显
		void saveDirectionWidgetToPbData();
		void restoreDirectionWidgetFromPbData();

        // 3. 属性
        QRadioButton* rb_autoCalc = nullptr;
        QRadioButton* rb_manualSpecify = nullptr;
        QLineEdit* le_manualValue = nullptr;
        QCheckBox* cb_adjust = nullptr;
	};
}
