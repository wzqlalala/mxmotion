#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include <memory>

//QT
#include <QWidget>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QHash>
#include <QGridLayout>
#include <QMdiSubWindow>
#include <QStringListModel>
#include <QLabel>

#include "mPanelBase.h"
#include "mMeshPanelBase.h"

#include "mPhysicalFieldEnum.h"

#include "mMeshThread.h"
#include "mMeshPanel.h"

#include "mxr/view/HighlightController.h"
using namespace MxFunctions;
using namespace MGUIMesh;
namespace MechPreData
{
	class mViewportModel;
}

class MXMesh;
namespace MechMainWindow
{

	class MMAINWINDOW_EXPORT mRenumberPanel :public mMeshPanel
	{
	public:

        mRenumberPanel(QWidget* parent = Q_NULLPTR);

		void slot_lock();
		void slot_unlock();
		void apply();

		~mRenumberPanel();

		void newMeshPanel() override;
		//更新面板
		void editMeshPanel(int meshId);
		//多选网格子节点面板
		void getMeshInfoById(const std::vector<int> ids);
		//根据数据初始化面板
		void updatePanel() override;
		//初始化默认尺寸
		void setMeshSize();
		//初始化默认参数
		void initSize() override;
        //替换
		void replaceObjects();
        //新增
        void addObjects();
        //删除
        void deleteObjects();
        //重编号Node
        void updateNodeLabel(QHash<int, int> oldToNewNode);
        //重编号Element
        void updateMeshLabel(QHash<int, QHash<int, int>> oldToNewElements);

        void updateNodeNum();
        void updateEleNum();
	private:
		//全局尺寸
		FoldSection* m_TranslationFold = nullptr;

		PosPickWidget1 posPick;
        QRadioButton* typeBtn1;
        QRadioButton* typeBtn2;
        QLabel* startId;
        QLineEdit* startIdEdit;

        QLabel* idGap;
        QLineEdit* idGapEdit;
        QLabel* maxIdEdit = nullptr;
        QLabel* minIdEdit = nullptr;
        vector<RenderPickResultData> MeshIDs;
        //上锁编号
        std::unordered_set<int> _lockNumber={};

        void updateMinMaxId();
	};
}