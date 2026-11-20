#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include "mPanelBase.h"
#include "mMotionHydraulic.h"

#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QTableWidget>
#include <QTabWidget>
#include <QToolButton>
#include <QDoubleSpinBox>

namespace MechMainWindow
{
	class MMAINWINDOW_EXPORT mMotionHydraulicPanel : public mPanelBase
	{
		Q_OBJECT
	public:
		mMotionHydraulicPanel(QWidget* parent = nullptr);
		~mMotionHydraulicPanel();

		void newHydraulicPanel();
		void editHydraulicPanel(int id = 0);
		void uneditHydraulicPanel();
        /*
        * 数据对象即将被删除（树删除该类型前调用，此时对象仍有效）：置空编辑指针
        * 防悬垂——面板刷新链读已释放对象是未定义行为（衬套删除崩溃同源）
        */
        void onMotionHydraulicDataAboutToBeDestroyed(int id = -1);


	private:
		void connectSig();
		void disconnectSig();
		void setupFluidTable(QTableWidget* table);

	private slots:
		void slot_browseModelFile();
		void slot_browseParamFile();

	private:
		QVBoxLayout* m_mainLayout;

		// 基础信息
		QLineEdit* m_edtName;
		QComboBox* m_cmbHydraulicType;  // 液压元件类型: 油缸/阀/泵/管路
		QLineEdit* m_edtModelFile;      // 液压模型文件 (AMESim / SimHydraulics)
		QToolButton* m_btnBrowseModel;

		// 流体参数
		QDoubleSpinBox* m_spbFluidDensity;
		QDoubleSpinBox* m_spbFluidViscosity;
		QDoubleSpinBox* m_spbBulkModulus;

		// 端口/管道页签
		QTabWidget* m_tabHydraulic;
		QTableWidget* m_tablePorts;
		QTableWidget* m_tablePipes;

		// 数据对象
		MechMotionData::mMotionHydraulic* m_data = nullptr;
	};
}
