#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

// QT
#include <QRadioButton>

namespace MechMainWindow
{
	//局部尺寸面板
	class MMAINWINDOW_EXPORT mExtractSurfacePanel :public mMeshPanel
	{
		Q_OBJECT
	public:

        mExtractSurfacePanel(QWidget* parent = Q_NULLPTR);

		~mExtractSurfacePanel();

		void newMeshPanel() override;

		//更新面板
		void editMeshPanel(int meshId);
		//根据数据初始化面板
		void updatePanel() override;
        //创建
		void apply();

		//刷新部件下拉框
		void refreshPartCombo();

	private:
		//多线程
		mMeshThread* _meshthread = nullptr;

		FoldSection* m_rangeFold = nullptr;
		QLabel* _meshPartLabel;
		QComboBox* _meshPartBox;
		QLineEdit* _meshPartEdit;
		// 是否共节点（是/否）
		QRadioButton* _shareNodeYes = nullptr;
		QRadioButton* _shareNodeNo = nullptr;
		QPushButton* _comfirmButton;

        PosPickWidget1 posPick;
        vector<RenderPickResultData> MeshIDs;
	};
}