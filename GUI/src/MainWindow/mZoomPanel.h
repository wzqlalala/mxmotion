#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mMeshPanel.h"

namespace MechMainWindow
{
	//网格缩放面板
	class MMAINWINDOW_EXPORT mZoomPanel :public mMeshPanel
	{
	public:

		mZoomPanel(QWidget* parent = Q_NULLPTR);

		~mZoomPanel();

		//应用缩放，dirSign为1按因子缩放，为-1按因子倒数缩放
		void apply(double dirSign = 1.0);
		//基准点
		void centerChanged();

		//更新面板
		void editMeshPanel(int meshId);
		//根据数据初始化面板
		void updatePanel() override;

	private:
		vector<RenderPickResultData> MeshIDs;
		PosPickWidget1 posPick;
		mCalcLineEdit* centerEdit1;
		mCalcLineEdit* centerEdit2;
		mCalcLineEdit* centerEdit3;
		mCalcLineEdit* xEdit;
		mCalcLineEdit* yEdit;
		mCalcLineEdit* zEdit;
		QCheckBox* newMeshPart;
	};
}
