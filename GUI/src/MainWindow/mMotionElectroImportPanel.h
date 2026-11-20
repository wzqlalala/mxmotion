#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include "mPanelBase.h"
#include "mMotionElectroImport.h"

#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QTableWidget>
#include <QTabWidget>
#include <QToolButton>
#include <QStackedWidget>

namespace MechMainWindow
{
	class MMAINWINDOW_EXPORT mMotionElectroImportPanel : public mPanelBase
	{
		Q_OBJECT
	public:
		mMotionElectroImportPanel(QWidget* parent = nullptr);
		~mMotionElectroImportPanel();

		void newElectroImportPanel();
		void editElectroImportPanel(int id = 0);
		void uneditElectroImportPanel();
        /*
        * 数据对象即将被删除（树删除该类型前调用，此时对象仍有效）：置空编辑指针
        * 防悬垂——面板刷新链读已释放对象是未定义行为（衬套删除崩溃同源）
        */
        void onMotionElectroImportDataAboutToBeDestroyed(int id = -1);


	private:
		void connectSig();
		void disconnectSig();
		void setupPortTable(QTableWidget* table);

	private slots:
		void slot_browseModelFile();
		void slot_browseHeaderFile();
		void slot_coSimTypeChanged(int index);

	private:
		QVBoxLayout* m_mainLayout;

		// 基础信息
		QComboBox* m_cmbCoSimType;      // 耦合类型: Simulink / FMU / 自定义
		QLineEdit* m_edtPlatform;       // 外部平台名称 (如 Simulink)
		QLineEdit* m_edtModelFile;      // 模型文件路径
		QToolButton* m_btnBrowseModel;
		QLineEdit* m_edtHeaderFile;     // 头文件路径 (Simulink 需要)
		QToolButton* m_btnBrowseHeader;

		// 端口配置页签
		QTabWidget* m_tabPorts;
		QTableWidget* m_tableInputPorts;
		QTableWidget* m_tableOutputPorts;

		// 数据对象
		MechMotionData::mMotionElectroImport* m_data = nullptr;
	};
}
