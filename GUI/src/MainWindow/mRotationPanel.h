#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mMeshPanel.h"

namespace MechMainWindow
{

	//旋转网格面板
	class MMAINWINDOW_EXPORT mRotationPanel :public mMeshPanel
	{
	public:

		mRotationPanel(QWidget* parent = Q_NULLPTR);

		void apply(double dirSign = 1.0);

		~mRotationPanel();
		//新建面板
		void newMeshPanel() override;
		//更新面板
		void editMeshPanel(int meshId);
		//根据数据初始化面板
		void updatePanel() override;
	private:
		QLabel* x;
		QLabel* w;
		QLineEdit* xEdit1;
		QLineEdit* xEdit2;
		QLineEdit* xEdit3;
		mCalcLineEdit* wEdit;

		vector<RenderPickResultData> MeshIDs;
		PosPickWidget1 posPick;
		VecPickWidget vecPick;
		QCheckBox* newMeshPart;
	};
}