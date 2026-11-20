#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mMeshPanel.h"

namespace MechMainWindow
{
	
	//线网格面板
	class MMAINWINDOW_EXPORT mTranslationPanel :public mMeshPanel
	{
	public:

		mTranslationPanel(QWidget* parent = Q_NULLPTR);

        ~mTranslationPanel();

        void apply(double dirSign = 1.0);

		void newMeshPanel() override;
		//更新面板
		void editMeshPanel(int meshId);
		//根据数据初始化面板
		void updatePanel() override;
	private:
		//全局尺寸
        QLineEdit* xEdit;
        QLineEdit* yEdit;
        QLineEdit* zEdit;
		QCheckBox* newMeshPart;
		PosPickWidget1 posPick;
		vector<RenderPickResultData> MeshIDs;

        QComboBox* methodBox;
        QLineEdit* distanceEdit;

		// 矢量方向选择控件，与拉伸网格的拉伸方向保持一致
		VecPickWidget vecPick;
	};
}
