#pragma once
#include "mmainwindow_global.h"
//解决中文乱码
#pragma execution_character_set("utf-8")
using namespace MxFunctions;
using namespace MGUIMesh;
namespace MechPreData
{
	class mViewportModel;
}
class MXMesh;
namespace MechMainWindow
{

	//线网格面板
	class MMAINWINDOW_EXPORT mMoveMeshPanel :public mMeshPanel
	{
	public:

        mMoveMeshPanel(QWidget* parent = Q_NULLPTR);

		~mMoveMeshPanel();

		void newMeshPanel() override;
		//更新面板
		void editMeshPanel(int meshId);
		//根据数据初始化面板
		void updatePanel() override;;

	private:
		//刷新部件下拉框
		void refreshPartCombo();
		//获取目标部件名（新建时取输入框并查重）
		QString getTargetPartName();
		//将颜色应用到部件并刷新渲染
		void applyColorToPart(const QString& partName, const QColor& color);
		//复制/移动执行后的收尾清理
		void finishOperate();
	private:
		//部件折叠框
		FoldSection* m_PartFold = nullptr;

		vector<RenderPickResultData> MeshIDs;
		PosPickWidget1 posPick;
		QComboBox* m_partBox = nullptr;
		QLineEdit* m_partEdit = nullptr;
        QVector3D curColor = QVector3D(255, 255, 255);
	};
}