#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

//QT
#include <QObject>
#include <QString>
#include <set>

namespace mxr
{
	class REBUILD_PostRend;
	class REBUILD_PostRender;
}

namespace PostData
{
	class mPostDataBase;
	class mPostModelData;
}

namespace MechPreData
{
	class mPostGUIData;
}

template<typename T> class QPointer;

namespace PostWindow
{
	/*
	* 后处理"显示"菜单功能（单例）：视口相机操作/梁杆实体化/模型单元显隐。
	* 自前处理 mDisplayMenuFunction 的后处理分支分离移植（后处理"显示"页为独立
	* Ribbon页，PostWindow 不依赖 MainWindow），仅经视口管理器(PreData)与
	* 结果数据库/渲染(RebuildBasicData/RebuildRender) SDK 工作
	*/
	class mPostDisplayMenuFunction : public QObject
	{
		Q_OBJECT

	public:
		/*
		* 获取单例指针
		*/
		static mPostDisplayMenuFunction* getInstance();

		/*
		* 析构函数
		*/
		~mPostDisplayMenuFunction();

	public slots:
		//上一个/下一个视角
		void slot_changeViewList(bool next);

		//自定义轴旋转（axis 形如 "+Sx"/"-Sz"，绕屏幕轴）
		void slot_angleRotation(QString axis, double value);

		//自定义方向平移（direction: up/down/left/right）
		void slot_directionPan(QString direction, double value);

		//自定义倍数缩放
		void slot_valueZoom(bool zoomin, double value);

		//梁实体化开关（按下显示全部梁实体化，弹起隐藏）
		void slot_setAllBeamVolume(bool show);

		//杆实体化开关（按下显示全部杆实体化，弹起隐藏）
		void slot_setAllRodVolume(bool show);

		//隐藏当前（隐藏当前拾取的单元）
		void slot_maskModel();

		//隐藏其他（隐藏当前拾取以外的全部单元）
		void slot_maskOtherModel();

		//反转显示（已显示/已隐藏的单元互换）
		void slot_reverseMaskModel();

		//显示全部单元
		void slot_showAllModel();

	private:
		/*
		* 默认构造函数
		*/
		explicit mPostDisplayMenuFunction(QObject* parent = nullptr);

		/*
		* 取当前后处理视口上下文（视口/渲染器/云图GUI数据/结果库/模型数据），
		* 任一环节为空返回false（槽统一从此入口做前置校验）
		*/
		bool getPostContext(mxr::REBUILD_PostRend*& postRend, mxr::REBUILD_PostRender*& postRender,
			MechPreData::mPostGUIData*& guidata, QPointer<PostData::mPostDataBase>& postDB,
			PostData::mPostModelData*& modelData);

		/*
		* 单元显隐变更后同步云图极值与颜色条（自前处理 mDisplayMenuFunction 同名函数移植）
		*/
		void updateMaxAndMinData();

		/*
		* 单元显隐变更后的模型重建：受影响部件面拓扑重分类（与树侧单元集显隐
		* rebuildPostElementRender 同修）——ParseMesh 的 faceBuilt 守卫会跳过已建
		* 部件，被藏单元的邻接面仍是"内部面"（渲染只画边界面表）→穿透空洞；
		* 先清受影响部件面数据强制重收录，边界/内部按当前显隐重算
		*/
		void rebuildModelRender(PostData::mPostModelData* modelData, mxr::REBUILD_PostRender* postRender,
			mxr::REBUILD_PostRend* postRend, const std::set<int>& reclassifyParts);
	};
}
