#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mMeshPanel.h"

namespace MechMainWindow
{
	//网格阵列面板
	class MMAINWINDOW_EXPORT mMatrixPanel :public mMeshPanel
	{
	public:

		mMatrixPanel(QWidget* parent = Q_NULLPTR);

		~mMatrixPanel();

		//应用阵列，dirSign为1沿给定方向阵列，为-1沿反方向阵列
		void apply(double dirSign = 1.0);
		//圆心
		void centerPointChanged();

		//更新面板
		void editMeshPanel(int meshId);
		//根据数据初始化面板
		void updatePanel() override;

	private:
		vector<RenderPickResultData> MeshIDs;
		PosPickWidget1 posPick;
		VecPickWidget vecPick;
		QComboBox* methodBox;
		mCalcLineEdit* centerEdit1;
		mCalcLineEdit* centerEdit2;
		mCalcLineEdit* centerEdit3;
		mCalcLineEdit* spacingEdit;
		mCalcLineEdit* numEdit;
	};
}
