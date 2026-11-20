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
#include <QStringListModel>
#include <QLabel>
#include <QToolButton>

#include "mCalcLineEdit.h"
#include "mPanelBase.h"
#include "PosPickWidget1.h"	//PosPickWidget1已析出至GUIFoundation（前后处理共用拾取控件）
#include "mMeshPanelBase.h"
#include "mMeshOptPanelBase.h"

#include "mPhysicalFieldEnum.h"

#include "mMeshThread.h"

#include "mxr/view/HighlightController.h"
//【镜像六件套摘除·连带】渲染头收紧传递包含后补显式声明来源（MeshSelectInfoData）
#include "mxr/data/SelectDataStruct.h"
using namespace MxFunctions;
using namespace MGUIMesh;
namespace MechPreData
{
	class mViewportModel;
}

class MXMesh;
class RenderPickResultData;

namespace MechMainWindow
{
    static double findMinDouble(const std::vector<std::pair<int, double>>& vec)
    {
        if (vec.empty()) {
            throw std::invalid_argument("Vector is empty");
        }

        auto min_it = std::min_element(vec.begin(), vec.end(),
            [](const std::pair<int, double>& a, const std::pair<int, double>& b) {
                return a.second < b.second;
            });

        return min_it->second;
    }
    //std::vector<RenderPickResultData>去重
    bool pushBackUnique(std::vector<RenderPickResultData>& vec,
        const RenderPickResultData& data);
    //std::vector<MeshSelectInfoData>去重
    //【镜像六件套摘除·连带】渲染头不再泄漏 using namespace mxr，类型改为显式限定
    bool pushBackUnique(std::vector<mxr::MeshSelectInfoData>& vec,
        const mxr::MeshSelectInfoData& data);

    class VecPickWidget:public QObject
    {
        Q_OBJECT
        public:
        QLabel* coor = nullptr;
        QComboBox* coorBox = nullptr;
        QToolButton* coorBtn = nullptr;
        QLabel* dir = nullptr;
        QComboBox* dirBox = nullptr;
        QToolButton* dirBtn = nullptr;
        QLabel* vec = nullptr;
        QLineEdit* vecEdit = nullptr;
        QToolButton* vecBtn = nullptr;
        QWidget* propwid = nullptr;
        QWidget* vecLayout = nullptr;
        void initWidget(const QString& name, bool hasadd = false, bool hasThreePoint = false);
        void createCoor();

        // 三点确定相关控件
        bool _hasThreePoint = false;
        QWidget* threePointWidget = nullptr;
        mCalcLineEdit* tpX1 = nullptr;
        mCalcLineEdit* tpY1 = nullptr;
        mCalcLineEdit* tpZ1 = nullptr;
        mCalcLineEdit* tpX2 = nullptr;
        mCalcLineEdit* tpY2 = nullptr;
        mCalcLineEdit* tpZ2 = nullptr;
        mCalcLineEdit* tpX3 = nullptr;
        mCalcLineEdit* tpY3 = nullptr;
        mCalcLineEdit* tpZ3 = nullptr;
        QToolButton* tpBtn1 = nullptr;
        QToolButton* tpBtn2 = nullptr;
        QToolButton* tpBtn3 = nullptr;
        void setupThreePointPicker(QToolButton* tool, mCalcLineEdit* editx, mCalcLineEdit* edity, mCalcLineEdit* editz);
    };

	//网格设置面板
	class MMAINWINDOW_EXPORT mMeshPanel : public mPanelBase
	{
		Q_OBJECT

	public:
		/*
		* 默认构造函数
		*/
		mMeshPanel(QWidget* parent = Q_NULLPTR);

		/*
		* 析构函数
		*/
		~mMeshPanel();
        /*
        * 创建水平分割线
        */
        QFrame* createHorizontalLine(QWidget* parent);
		/*
		* 获取meshID
		*/
		int getMeshID();

	/*	template<typename T>
		T* toType(MechCaseData::mCaseBase* base)
		{
			return dynamic_cast<T*>(base);
		};*/
		

		//设置网格数据
		void newMeshPanel(int type);

        //设置网格操作数据
        void newMeshOptPanel(int type);

        //初始化默认尺寸
		virtual void initSize();
		
		//新建网格	
		virtual void newMeshPanel();

		//更新面板
		virtual void updatePanel();

		//更新面板
		virtual void editMeshPanel(int meshId);


		//多选网格子节点面板
		virtual void getMeshInfoById(const std::vector<int> ids);

		//切换视口	

		//部件生成网格
		void slot_createMeshByPartId(vector<int> partIds);

		//一键生成网格
		void slot_createMeshAuto();

		//一键删除网格
		void slot_deleteMeshAuto();

		//根据网格设置生成
		void slot_createMeshBySetting();

		//生成网格
		void slot_createMesh(int id);

		//生成网格全局尺寸
		void slot_createMesh1();

		//生成网格自适应剖分
		virtual void slot_createMeshAutoAdapt();

		//生成网格自定义加密
		void slot_createMeshCustomEncryption();

        //生成网格局部尺寸
        void slot_createMeshLocalSize(int id);

        //生成网格边抑制
        void slot_createMeshEdgeSuppress(int id, bool isSuppress);

		//保存包围盒
		virtual void slot_boxSave();

		//删除包围盒
		virtual void slot_boxDelete();

		//隐藏包围盒
		virtual void slot_boxHide();

		//预览包围盒
		virtual void slot_boxPreview();

		//显示包围盒
		virtual void slot_boxShow();

        //局部尺寸预览
        virtual void slot_localSizePreview();

		//更新ListWidget
		void slot_updateListWidget();

		//根据 _list 中的对象重新计算网格尺寸
		void recalculateSizeFromList();

		//取消网格编辑
		void unEditMeshPanel();

		MeshPanelData::mMeshPanelBase* getMeshPanelBase();

        //创建水平分割线
        QFrame* createHorizontalLine();
	private:
	signals:
		//一键生成
		void startSlot_createMeshAuto();
		//根据网格设置生成
		void startSlot_createMeshBySetting();
		//根据partId生成
		void startSlot_createMeshByPartId(vector<int> partIds);
		//根据id生成
		void startSlot_createMesh(int id,QStringList list);
		//全局尺寸
		void startSlot_createMesh1();
		//网格自适应
		void startSlot_createMeshAutoAdapt(int dim, const std::set<int>& geo_ids, MeshParam mesh_param);
		//自定义加密
		void startSlot_createMeshCustomEncryption();
        //局部尺寸
        void startSlot_createMeshLocalSize(int id, QStringList list, std::map<int, double> ids2SizesMap);
        //边抑制
        void startSlot_createMeshEdgeSuppress(int id, QStringList list, bool isSuppress);
  //按钮不置灰
		void setBtnEnabled();

	protected:

		////更新预应力下拉框
		//void updatePreloadCombobox(QComboBox* combobox);

	protected:

		//网格数据
		MeshPanelData::mMeshPanelBase* _meshPanelData = nullptr;
		//网格面板当前绑定节点的id（_meshPanelData可能在节点删除后悬空，用时须按id经管理器重查）
		int _meshPanelId = -1;
		//发起剖分时快照的单元阶数（经管理器重查后取值，完成回调不再解引用面板数据）
		int _meshOrderSnapshot = 1;
		//网格操作数据
		MeshPanelData::mMeshOptPanelBase* _meshOptPanelData = nullptr;

		//总布局
		QVBoxLayout* _mainLayout;

		//当前模型
		MechPreData::mViewportModel* _currentViewportModel;

		//网格多线程
		mMeshThread *m_meshThread;
		QThread *m_thread;


		QSet<QWidget*> BackgroundField;		    // 背景尺寸场

		QListWidget* _listAssignPart;

	};



	
}