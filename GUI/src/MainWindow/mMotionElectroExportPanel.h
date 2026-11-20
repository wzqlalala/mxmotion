#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include "mPanelBase.h"
#include "mMotionElectroExport.h"

#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QTableWidget>
#include <QTabWidget>
#include <QToolButton>

namespace MechMainWindow
{
	class MMAINWINDOW_EXPORT mMotionElectroExportPanel : public mPanelBase
	{
		Q_OBJECT
	public:
		mMotionElectroExportPanel(QWidget* parent = nullptr);
		~mMotionElectroExportPanel();

		void newElectroExportPanel();
		void editElectroExportPanel(int id = 0);
		void uneditElectroExportPanel();
        /*
        * 数据对象即将被删除（树删除该类型前调用，此时对象仍有效）：置空编辑指针
        * 防悬垂——面板刷新链读已释放对象是未定义行为（衬套删除崩溃同源）
        */
        void onMotionElectroExportDataAboutToBeDestroyed(int id = -1);


	private:
		void connectSig();
		void disconnectSig();
		void setupPortTable(QTableWidget* table);

	private slots:
		void slot_browsePortListFile();

	private:
		QVBoxLayout* m_mainLayout;

		// 基础信息
		QLineEdit* m_edtName;
		QComboBox* m_cmbExportType;     // 导出类型
		QLineEdit* m_edtPortListFile;   // 端口列表文件
		QToolButton* m_btnBrowseFile;

		// 端口配置页签
		QTabWidget* m_tabPorts;
		QTableWidget* m_tableInputPorts;
		QTableWidget* m_tableOutputPorts;

		// 数据对象
		MechMotionData::mMotionElectroExport* m_data = nullptr;
	};
}
