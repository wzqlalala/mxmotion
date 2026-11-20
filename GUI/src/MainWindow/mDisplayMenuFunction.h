#pragma once

//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mDockBase.h"
#include <QFrame>
#include <QSize>

#include "mPhysicalFieldEnum.h"
#include <QLineEdit>
#include <QVBoxLayout>
#include <QPushButton>
#include <QToolButton>
#include <QTableWidget>
#include <QScrollArea>
#include <QPropertyAnimation>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <memory>
#include <QString>
#include <QVector>
#include <set>
#include <vector>
#include <QStyledItemDelegate>
#include <QMdiSubWindow>
#include <QValidator>
#include <QDebug>
#include <QTimer>
#include <QRegularExpression>

namespace MechPreData
{
	class mViewportPrivate;
}
namespace mxr
{
	class IPreDataProxy;
}
namespace GeoNeutralData
{
	class mGeoPartManager;
	class GN_Part;
}
namespace MechMainWindow
{
	class mDisplayMenuFunction : public QObject
	{
		Q_OBJECT
	public:
		/*
		* 获取单例指针
		*/
		static mDisplayMenuFunction* getInstance();

		/*
		* 析构函数
		*/
		~mDisplayMenuFunction();


	private:
		/*
		* 默认构造函数
		*/
		mDisplayMenuFunction(QWidget* parent = 0);

		//清空高亮
		void clearHighlight();

	public slots:

		//上一个/下一个视角
		void slot_changeViewList(bool next);

		//自定义轴旋转
		void slot_angleRotation(QString axis, double value);

		//自定义方向平移
		void slot_directionPan(QString direction, double value);

		//自定义倍数缩放
		void slot_valueZoom(bool zoomin, double value);

		//自动旋转绕轴
		void slot_autoRotation(QString axis, bool enable);

		//设置后处理全部梁的显隐
		void slot_setPostAllBeamVisble(bool show);

		//设置后处理全部杆的显隐
		void slot_setPostAllRodVisble(bool show);

		//设置全部壳的显隐
		void slot_setAllShellVisble(bool show);

		//设置全部杆的显隐
		void slot_setAllRodBeamVisble(bool show, MxFunctions::PropertyType type);

		//设置全部弹簧的显隐
		void slot_setAllSpringVisble(bool show, MxFunctions::PropertyType type = MxFunctions::Elas1Property);

		//设置实体化显隐
		void slot_setEntityVisble(bool show, std::vector<int> partIds);

		//ID显示
		void slot_showNodeEleLabel();

		//ID隐藏
		void slot_hideNodeEleLabel();

		//ID全部隐藏
		void slot_hideAllNodeEleLabel();

		//隐藏当前
		void slot_maskModel();

		//隐藏其他
		void slot_maskOtherModel();

		//反转显示
		void slot_reverseMaskModel();

		//显示全部
		void slot_showAllModel();

		//显示全部自由节点
		void slot_showAllFreeNode();

		//隐藏全部自由节点
		void slot_hideAllFreeNode();

		//显示自由节点
		void slot_showFreeNode();

		//隐藏自由节点
		void slot_hideFreeNode();

        //显示自由节点
        void slot_deleteAllFreeNode();

        //隐藏自由节点
        void slot_deleteFreeNode();

		//所有视口的自由节点显示
		void slot_showFreeNode(std::vector<int> tempIds, bool showHide = true);
	private slots:

		/*
		* 切换视口
		*/
		void slot_subWindowActivated(QMdiSubWindow* subWindow);

	private:

		//更新id显示
		void updateNodeEleLabel();

		//更新自由节点显示
		void updateFreeNodeShow(MechPreData::mViewportPrivate* viewportPrivate);

		//获取部件默认初始平面向量
		std::vector<QVector3D> getDefaultUpVector(std::vector<int> parIds);	

		//更新最值
		void updateMaxAndMinData();

		/**********************几何隐藏相关****************************/
		// 几何隐藏
		void setGeoHidden(const std::vector<int>& pointID, const std::vector<int>& lineID, const std::vector<int>& faceID, const std::vector<int>& solidID, const std::vector<int>& partID);
		//几何隐藏其他
		void setGeoHiddenOther(const std::vector<int>& pointID, const std::vector<int>& lineID, const std::vector<int>& faceID, const std::vector<int>& solidID, const std::vector<int>& partID);
		// 几何显示
		void setGeoShow(const std::vector<int>& pointID, const std::vector<int>& lineID, const std::vector<int>& faceID, const std::vector<int>& solidID);
		//几何全部显示
		void setGeoShowAll();
		//几何反向显示
		void setGeoShowReverse();
		//通过几何实体重新计算几何面的显隐状态
		void calculateGeoFaceHiddenBySolid(const std::vector<int>& solidID);
		//获取Soild是否是隐藏
		bool getGeoPointHiddenByPoint(int id);
		bool getGeoSolidHiddenBySolid(int id);
		bool getGeoFaceHiddenByFace(int id);
		//【镜像六件套摘除】旧 getGeoModelData（渲染侧镜像）已删，读写改走契约代理 + GN 部件属性
		//获取当前视口前处理数据契约代理
		mxr::IPreDataProxy* getPreDataProxy();
		//获取 GN 部件管理器（几何显隐真源）
		GeoNeutralData::mGeoPartManager* getGeoPartMgr();
		//通过元素维度和 ID 反查所属 GN 部件
		GeoNeutralData::GN_Part* getGeoPartByElementId(int dim, int id);
		//写元素显隐（GN 侧，自动"体→面→边→点"向下级联）
		bool setGeoElementVisible(int dim, int id, bool visible);
		//读元素显隐（GN 侧，查不到按可见兜底）
		bool isGeoElementVisible(int dim, int id);
		//全部部件 ID（触发渲染重建用）
		std::set<int> getAllGeoPartIds();
		//全模型某维度元素 ID 全集（点含独立点、线含独立线，口径同旧镜像）
		std::set<int> getAllGeoElementIds(int dim);
		//某部件下的全部体
		std::vector<int> getSolidIdsInPart(int partId);

	private:
		//单例指针
		static mDisplayMenuFunction* _instance;

		QMetaObject::Connection conn_autoRotation;

		QTimer _autoRotationTimer;


	};
}
