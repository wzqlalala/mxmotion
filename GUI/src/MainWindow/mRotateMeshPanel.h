#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

namespace MechMainWindow
{
	// 旋转拉伸面板
	class MMAINWINDOW_EXPORT mRotateMeshPanel :public mMeshPanel
	{
		Q_OBJECT
	public:

		mRotateMeshPanel(QWidget* parent = Q_NULLPTR);

        ~mRotateMeshPanel();

		void applyRotateStretch();

		//更新面板
		void editMeshPanel(int meshId);
        //根据数据初始化面板
        void updatePanel() override;

        void centerChanged();
	private:
		FoldSection* m_ObjectFold = nullptr;        // 对象下拉框
		FoldSection* m_paramFold = nullptr;         // 参数下拉框

		QLabel* center;             // 中心点
		mCalcLineEdit* centerx;
		mCalcLineEdit* centery;
		mCalcLineEdit* centerz;
		QToolButton* m_centerPickBtn = nullptr; // 中心点拾取按钮

		QLabel* angle;              // 拉伸角度
        QLineEdit* angleEdit;
		QLabel* layer;              // 拉伸层数
        QLineEdit* layerEdit;

		// 所属部件
		QLabel* _meshPartLabel;
		QComboBox* _meshPartBox;
		QLineEdit* _meshPartEdit;


		vector<RenderPickResultData> MeshIDs;

		PosPickWidget1 posPick;
		VecPickWidget vecPick;
	};
}
