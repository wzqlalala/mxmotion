//#pragma once
////解决中文乱码
//#pragma execution_character_set("utf-8")
//
//#include "mCommon_global.h"
//
////MxFunctions
//#include <mBasicEnum.h>
//
////QT
//#include <QMdiArea>
//#include <QHash>
//#include <QMdiSubWindow>
//#include <QPair>
//#include <set>
//#include <memory>
//#include <QList>
//
//#include <QTreeWidget>
//#include <QVector3D>
//
//#include "mModelStruct.h"
//
//namespace MBaseRend
//{
//	class mBaseRend;
//}
//
//namespace MRendData
//{
//	class IMeshPickResultBase;
//}
//
//namespace MLableRend
//{
//	class mLableRendController_pre;
//	class mLableRendController_common;
//}
//namespace MDataLable
//{
//	class mLableDataController_pre;
//	class mLableDataController_common;
//	struct TempPoint_Common;
//}
//namespace MPlotView
//{
//	class mPlotView;
//}
//namespace MRendView
//{
//	class CCSMRendWidget;
//}
//namespace MDataGeo
//{
//	class mGeoModelData1;
//	class mGeoPickData1;
//}
//namespace MDataPre
//{
//	class mPreMeshPickData1;
//}
//
//class MFace;
//class MEdge;
//using namespace MxFunctions;
//using namespace std;
//using namespace MBaseRend;
//using namespace MRendView;
//
//using namespace MLableRend;
//using namespace MDataLable;
//using namespace MRendData;
//namespace MechCommon
//{
//	class COMMON_EXPORT mViewportManager : public QMdiArea
//	{
//		Q_OBJECT
//	public:
//		/*
//		* 获取单例指针
//		*/
//		static mViewportManager* getInstance();
//
//		/*
//		 *信号连接
//		 */
//		void connectSignal();
//
//		/*
//		* 新建前处理3D视口
//		*/
//		void openViewport3D(MRendView::CCSMRendWidget* pview);
//
//		/*
//		* 关闭前处理3D视口
//		*/
//		void closeViewport3D();
//
//		/*
//		* 获取实时曲线视图
//		*/
//		MPlotView::mPlotView* getTimeCurveView();
//
//		/*
//		* 根据名称获取曲线视图
//		*/
//		MPlotView::mPlotView* getTimeCurveView(const QString& name);
//
//		/*
//		* 关闭所有视口
//		*/
//		void closeViewports();
//
//		/*
//		* 获取前处理视口
//		*/
//		CCSMRendWidget *getPreViewport();
//
//		/*
//		*  清除网格调整和偏置标签渲染
//		*/
//		void clearCommonFont();
//
//		/*
//		* 设置值为int类型标签渲染
//		*/
//		void appendCommonFontInt(const QString& name, QVector<QVector3D> pos1, QVector<int> txt);
//
//		/*
//		* 设置值为double类型标签渲染
//		*/
//		void appendCommonFontdouble(const QString& name, QVector<QVector3D> pos1, QVector<double> txt);
//
//		/*
//		* 获取前处理mdi窗口
//		*/
//		QMdiSubWindow *getPreSubViewport();
//
//		/*
//		* 设置窗口平铺布局
//		*/
//		void setSubWindowLayout(const QString& objectName, const QStringList& names);
//
//		// 快速判断是否拾取
//		// 线是否和鼠标相交
//		bool isLinePick(const std::vector<QVector3D>& lines, const QVector2D& mouse);
//		// 面是否和鼠标相交
//		bool isFacePick(const std::vector<std::vector<QVector3D>>& lines, const QVector2D& mouse);
//
//		/********************************************几何数据接口**********************************************/
//		/*
//		* 获取几何数据类
//		*/
//		bool getGeoData();
//
//		/*
//		* 删除部件
//		*/
//		void deletePartData(int partName);
//
//		/*
//		* 设置几何显隐
//		*/
//		void setGeoPartVisible(const int& name, bool isshow);
//
//		/*
//		* 设置几何颜色
//		*/
//		void setGeoPartColor(const int& name, const QColor& color);
//
//		/*
//		* 设置单个几何硬点结构的显隐
//		*/
//		void setGeoHardPointVisual(int partName, bool isShow);
//
//		/*
//		* 设置所有Part几何硬点结构的显隐
//		*/
//		void setAllPartGeoHardPointVisual(bool isShow);
//
//		/*
//		* 设置Part的顶点数组
//		*/
//		void setGeoPartVertexArray(int PartID, int PartType, const std::vector<QVector3D>& xyz);
//
//		/*
//		* 添加几何零件数据(点)
//		*/
//
//		void appendGeoPointData(int PartID, int PartType, int VerId, int vertexIndex, int Type);
//		/*
//		* 添加几何零件数据(线)
//		*/
//
//		void appendGeoLineData(int PartID, int PartType, int EdgeId, const std::vector<int>& vertexsIndex, int Type);
//
//		/*
//		* 添加几何零件数据(面)
//		*/
//
//		void appendGeoFaceData(int PartID, int PartType, int FaceId, const std::vector<int>& vertexsIndex);
//
//		/*
//		* 添加几何零件数据(实体)
//		*/
//
//		void appendGeoSolidData(int PartID, int PartType, int SolidId, const std::vector<int>& faceIds);
//
//		/*
//		 * 设置旋转中心为模型中心
//		*/
//		void SetRotateCenterToModelCenter();
//
//		/****************************设置渲染样式*********************************/
//
//		/*
//		* 设置显示模式
//		*/
//		void setShowFunction(int);
//
//		/*
//		* 设置节点的大小
//		*/
//		void setNodeSize(int size);
//
//		/*
//		* 设置节点的颜色
//		*/
//		void setNodeColor(QVector4D color);
//
//		/*
//		* 设置点的大小
//		*/
//		void setPointSize(int size);
//
//		/*
//		* 设置边界线的宽度
//		*/
//		void setEdgeLineWidth(float size);
//
//		/*
//		* 设置面线的颜色
//		*/
//		void setFaceLineColor(QVector4D color);
//
//		/*
//		* 设置背景的显示模式
//		*/
//		void setBackGroundMode(BackGroundMode backgroundmode);
//		/*
//		 * 获取背景的显示模式
//		 */
//		BackGroundMode getBackGroundMode();
//		/*
//		 * 设置上下或者左右的颜色
//		 */
//		void setGradientColor(QVector3D color1, QVector3D color2);
//		/*
//		 * 获取上下或者左右的颜色
//		 */
//		void getGradientColor(QVector3D &color1, QVector3D &color2);
//		/*
//		 * 设置纯色的颜色
//		 */
//		void setPureColor(QVector3D color);
//		/*
//		 * 获取纯色的颜色
//		 */
//		void getPureColor(QVector3D &color);
//
//		/*
//		* 设置是否重置视图
//		*/
//		void resetCameraCenter();
//
//		/**************************文字图标渲染接口********************************/
//		/*
//		 * 获取前处理当前窗口的图标渲染控制对象
//		 */
//		shared_ptr<mLableRendController_pre> getCurrentLableRendController_pre();
//
//		/*
//		* 获取当前窗口通用图标渲染控制对象
//		*/
//		shared_ptr<mLableRendController_common> getCurrentLableRendController_common();
//
//		/*
//		 * 获取前处理当前窗口的图标数据控制对象
//		 */
//		shared_ptr<mLableDataController_pre> getCurrentLableDataController_pre();
//
//		/********************************网格节点渲染**************************************/
//
//		//节点渲染
//		void appendLableRendData(const QString & Name, QVector<QVector3D> pos, float size, QVector3D color);
//
//		void deleteLableRendData(const QString& Name);
//
//		/**************************几何拾取********************************/
//		//一次性高亮，清除上一次高亮
//		void setAllPickGeoPointData(const set<int> &Pointids);
//		void setAllPickGeoLineData(set<int> Lineids);
//		void setAllPickGeoFaceData(set<int> Faceids);
//		void setAllPickGeoSolidData(set<int> Solidids);
//		void setAllPickGeoPartData(set<int> partNames);
//
//
//		//累积高亮
//		void setAddPickGeoPointData(set<int> Pointids);
//		void setAddPickGeoLineData(set<int> Lineids);
//		void setAddPickGeoFaceData(set<int> Faceids);
//		void setAddPickGeoSolidData(set<int> Solidids);
//		void setAddPickGeoPartData(set<int> partNames);
//
//		//减少高亮
//		void setReducePickGeoPointData(set<int> Pointids);
//		void setReducePickGeoLineData(set<int> Lineids);
//		void setReducePickGeoFaceData(set<int> Faceids);
//		void setReducePickGeoSolidData(set<int> Solidids);
//		void setReducePickGeoPartData(set<int> partNames);
//
//		/*
//		* 临时高亮
//		*/
//		// 一次性高亮,清楚上一次高亮
//		//void setAllPickNodeData_Temp(const set<MXMeshVertex*>& vs);
//		//void setAllPickMeshData_Temp(const set<MXMeshElement*>& mesh);
//		void setAllPickLineData_Temp(const set<MEdge*>& lines);
//		void setAllPickFaceData_Temp(const set<MFace*>& faces);
//
//		// 几何隐藏
//		void setGeoHidden(const std::vector<int>& pointID, const std::vector<int>& lineID, const std::vector<int>& faceID, const std::vector<int>& solidID);
//		// 几何显示
//		void setGeoShow(const std::vector<int>& pointID, const std::vector<int>& lineID, const std::vector<int>& faceID, const std::vector<int>& solidID);
//		//几何全部显示
//		void setGeoShowAll();
//		//几何反向显示
//		void setGeoShowReserve();
//
//
//		
//		//新增剖切预览（拉伸）
//		void updateRend_Temp();
//
//		// 获取单次结果部件  网格或者几何
//		int getOncePickPartData();
//
//		//获取拾取
//		set<int> getPickPointIDs();
//		set<int> getPickLineIDs();
//		set<int> getPickFaceIDs();
//		set<int> getPickSolidIDs();
//		set<int> getPickGeoPartIDs();
//
//		QVector<int> getPickPointIdsOrder();
//        //获取当前的拾取的几何线IDs
//        std::set<int> getCurrentPickGeoLineIDs();
//        //获取当前的拾取的几何部件IDs
//        std::set<int> getCurrentPickGeoPartIDs();
//        //获取几何当前是增加拾取还是减少拾取（0:左键增加,1:右键减少）
//        int getGeoPickFunction();
//
//		//获取屏幕上点点
//		QVector<QVector3D> getPickVertexOnScreenDatas();
//		//获取面上点点
//		QHash<int, QVector<QVector3D>> getPickVertexOnGeoFaceDatas();
//		//获取线上点点
//		QHash<int, QVector<QVector3D>> getPickVertexOnGeoLineDatas();
//		//获取所有几何线ID
//		set<int> getAllGeoLineIDs();
//		//获取所有几何面ID
//		set<int> getAllGeoFaceIDs();
//		//获取所有几何实体ID
//		set<int> getAllGeoSolidIDs();
//
//		// 获取点坐标
//		QVector3D getPointVertex(int id) const;
//		//// 获取线
//		//QVector<QVector3D> getLinePointVertex(int id) const;
//		//// 获取面
//		//QVector<QVector3D> getFacePointVertex(int id) const;
//		// 获取实体面ID
//		std::set<int> getSolidFaceIds(int id) const;
//
//		/**************************网格拾取********************************/
//		//一次性高亮，清除上一次高亮
//		void setAllPickNodeData(const std::vector<RenderPickResultData>& nodes);
//		void setAllPickMeshData(const std::vector<RenderPickResultData>& meshs);
//		void setAllPickMeshLineData(const std::vector<RenderPickResultData>& meshlines);
//		void setAllPickMeshFaceData(const std::vector<RenderPickResultData>& meshfaces);
//		void setAllPickMeshPartData(const std::vector<RenderPickResultData>& partNames);
//
//		//累积高亮
//		void setAddPickNodeData(const std::vector<RenderPickResultData>& nodes);
//		void setAddPickMeshData(const std::vector<RenderPickResultData>& meshs);
//		void setAddPickMeshLineData(const std::vector<RenderPickResultData>& meshlines);
//		void setAddPickMeshFaceData(const std::vector<RenderPickResultData>& meshfaces);
//		void setAddPickMeshPartData(const std::vector<RenderPickResultData>& partNames);
//
//		//减少高亮
//		void setReducePickNodeData(const std::vector<RenderPickResultData>& nodes);
//		void setReducePickNodeOrderData(const std::vector<RenderPickResultData>& nodes);
//		void setReducePickMeshData(const std::vector<RenderPickResultData>& meshs);
//		void setReducePickMeshLineData(const std::vector<RenderPickResultData>& meshlines);
//		void setReducePickMeshFaceData(const std::vector<RenderPickResultData>& meshfaces);
//		void setReducePickMeshPartData(const std::vector<RenderPickResultData>& partNames);
//
//		// 取消高亮（不区分GEO MESH）
//		void setReducePickPartData(const std::vector<int>& partIDs);
//		bool isPickedPart(int partID);
//
//		//获取拾取
//		std::vector<RenderPickResultData> getPickNodeIDs();
//		std::vector<RenderPickResultData> getPickNodeIDsOrder();
//		std::vector<RenderPickResultData> getPickMeshIDs();
//		std::vector<RenderPickResultData> getPickPartIDs();
//		std::vector<RenderPickResultData> getPickMeshLineIDs();
//		std::vector<RenderPickResultData> getPickMeshFaceIDs();
//		std::vector<RenderPickResultData> getPickMeshPartNames();
//		std::vector<RenderPickResultData> getPickMeshShellIDs();
//		//获取当前单次增加高亮拾取的节点编号列表(目前仅限界面操作的结果)
//		std::vector<RenderPickResultData> getCurrentOnceAddPickNodeIDs();
//		//获取当前单次增加高亮拾取的单元编号列表(目前仅限界面操作的结果)
//		std::vector<RenderPickResultData> getCurrentOnceAddMeshIDs();
//		//获取当前单次增加高亮拾取的part编号列表(目前仅限界面操作的结果)
//		std::vector<RenderPickResultData> getCurrentOnceAddPartIDs();
//
//		//设置\\获取拾取的筛选器
//		void setPickFilter(PickFilter pickfilter);
//		void SetPickFilter(int Index);
//		PickFilter getPickFilter();
//		intptr_t GetPickFilter();
//		//设置多选拾取
//		void SetMutiplyPickStatus(bool M);
//		//设置框选模式
//		void setMultiplyPickMode(MultiplyPickMode multiplyPickMode);
//		void SetMultiplyPickMode(int Index);
//		//获取框选模式
//		MultiplyPickMode getMultiplyPickMode();
//		intptr_t GetMultiplyPickMode();
//		//设置拾取角度
//		void SetPickAngle(double angle);
//
//		//开始包围盒or包围球范围拾取 (包围球为直径端点，包围盒为对角线端点) pickFunction (0增加高亮, 1减少高亮)
//		void startBoundingQueryPick(const QVector3D& minRange, const QVector3D& maxRange, PickQueryBoundingType boundingType, int pickFunction);
//
//		/*
//		* 更新拾取高亮渲染
//		*/
//		void updateGeoPickRender();
//		/*
//		* 更新高亮渲染
//		*/
//		void updatePickRender();
//		/*
//		 *清空所有拾取
//		 */
//		void clearAllPickData();
//
//		/*
//		 *清空几何所有拾取
//		 */
//		void clearAllGeoPickData();
//
//		/*
//		 *清空网格所有拾取
//		 */
//		void clearAllMeshPickData();
//
//		/*
//		 *清空网格节点拾取
//		 */
//		void clearPickNodeData();
//
//		/*
//		 *清空网格拾取
//		 */
//		void clearPickMeshData();
//
//		/*
//		* 更新几何高亮渲染
//		*/
//		void updateGeoRender();
//
//		/*
//		* 刷新渲染
//		*/
//		void updateRender();
//
//		/*
//		* 重置视口
//		*/
//		void ResetOrthorCamera();
//
//		/*
//		* 更新视口
//		*/
//		void UpdateOrthorCamera();
//
//		/*
//		 *获取屏幕法向
//		 */
//		QVector3D getScreenNormal();
//
//		/*
//		 *获取屏幕平行方向
//		 */
//		QVector3D getScreenParellel();
//
//		/*
//		 *设置几何模型颜色
//		 */
//		void setGeoPartColor(int partName, QVector3D partColor);
//
//
//		/*
//		 *
//		 */
//
//		bool getGeoPartVisual(int partName);
//		/*
//		 *设置删除几何
//		 */
//		void deleteGeoPart(int partName);
//
//		/*
//		 *设置所有几何删除
//		 */
//		void deleteAllGeoPart();
//
//
//
//		/*
//		 *设置所有几何隐藏
//		 */
//		void setAllGeoVisiable(bool is);
//
//		/*
//		 *设置几何线的属性
//		 */
//		void setGeoLineProperty(std::set<int> geoLineIDs, int t);
//
//		/*
//		 *添加删除的item名字
//		 */
//		void appendDeleteItemName(QString s) { deleNameList.push_back(s); };
//
//		/*
//		 *清空删除的item名字
//		 */
//		void clearDeleteItemName() { deleNameList.clear(); }
//
//		/*
//		 *获取删除的item名字
//		 */
//		QList<QString> getDeleteItemName() { return deleNameList; }
//
//		/*
//		*网格剖分实体boi加密渲染染色包围盒
//		*/
//		void renderOCtreeBoundingBox(QString name, double min[3], double max[3]);
//		
//		/*
//		*网格剖分渲染包围盒
//		*/
//		void renderBoundingBox(QString name, double min[3], double max[3]);
//
//		/*
//		*删除网格剖分渲染包围盒
//		*/
//		void deleteRenderBoundingBox(QString& name);
//
//		void deleteRenderCylinder(QString& name);
//
//		void deleteRenderSphere(QString& name);
//
//		/*
//		*更新网格剖分渲染包围盒状态
//		*/
//		void updateRenderBoundingBox(QString& name, bool state);
//
//		void updateRenderCylinder(QString& name, bool state);
//
//		void updateRenderSphere(QString& name, bool state);
//
//		/*
//		*网格剖分渲染Sphere包围盒
//		*/
//		void renderSphere(QString name, QVector3D center, float radius);
//
//		/*
//		*网格剖分渲染Sphere
//		*/
//		void renderSphere(QString name, QVector3D center, double maxlc, double minlc, float radius);
//
//		/*
//		*网格剖分渲染Cylinder包围盒
//		*/
//		void renderCylinder(QString key, QVector3D center, QVector3D height_half_dir, float radius);
//
//		/*s
//		*网格剖分渲染箭头
//		*/
//
//		void renderArrows(QString key, QVector3D pos, QVector3D dir);
//
//		/*s
//		*网格剖分渲染点
//		*/
//		void renderArrows(QString key, QVector3D pos, QVector3D dir, double lc, double maxlc, double minlc);
//
//		/*
//		*几何拾取渲染符号
//		*/
//		void appendCommonFontQString(const QString & name, QVector<QVector3D> pos1, QVector<QString> txt, QVector3D mColor = QVector3D(0,0,0),float size = 1);
//		/*
//		*  清除网格调整和偏置标签渲染
//		*/
//		void clearGeoCommonFont(QString name);
//		/*
//		*  清除网格调整和偏置标签渲染
//		*/
//		void setGeoCommonFontIsShow(QString name,bool isShow);
//
//
//		/*
//		* 袁明 添加比例尺
//		*/
//		void addScaleBar();
//
//		public slots:
//			
//		/*
//		* 拾取连接
//		*/
//		std::vector<QString> getConnectPick(float x, float y);
//
//		/*
//		* 拾取绑定
//		*/
//		std::vector<QString> getBindPick(float x, float y);
//
//		/*
//		* 拾取接触
//		*/
//		std::vector<QString> getContactPick(float x, float y);
//
//	signals:
//		/*拾取完成信号*/
//		void FinishPickedSig();
//
//		/*
//		 * 更新模型颜色信号
//		 */
//		void updateGeoTreeModelColorSig();
//
//		/*
//		 *根据模型名字删除模型
//		 */
//		void deleteGeoPartSig(int name);
//		/*
//		 *设置所有几何删除
//		 */
//		void deleteAllGeoPartSig();
//
//		/*
//		 *几何模型删除后更新树
//		 */
//		void updateDeletePartItem(QTreeWidget*);
//
//		/*
//		 *几何模型显隐后更新树
//		 */
//		void updateGeoPartItemVis(QTreeWidget*);
//		/*
//		 *几何模型更改颜色后后更新树
//		 */
//		void updateGeoPartItemColor(QTreeWidget*);
//
//		void buttonType();
//		/*
//		* 隐藏部件时，对应的工件从显示变为隐藏状态时，二维（一维）单元按钮同步取消选中状态
//		* 参数：0-二维单元渲染，1-一维单元渲染
//		*/
//		void actionCheckedSig(int type);
//
//		void connectNamePickSig(QString connectName);
//
//	private:
//		/*
//		* 默认构造函数
//		*/
//		mViewportManager();
//
//		/*
//		* 析构函数
//		*/
//		~mViewportManager();
//
//	private:
//		//单例指针
//		static mViewportManager* _instance;
//
//		//前处理3D视口
//		QPair<CCSMRendWidget*, QMdiSubWindow*> _preport;
//
//		//几何数据
//		MDataGeo::mGeoModelData1* _geoData = nullptr;
//
//		//几何拾取数据
//		MDataGeo::mGeoPickData1 *_geoPickData = nullptr;
//
//		//网格拾取数据
//		MRendData::IMeshPickResultBase *_meshPickData = nullptr;
//
//		// 网格高亮数据 临时
//		MDataPre::mPreMeshPickData1* _meshData_temp = nullptr;
//
//		//用于存储删除树后的模型名字，用来同步各个模块的树删除
//		QList<QString> deleNameList{};
//
//		//框选拾取模式
//		MultiplyPickMode _pickMode;
//
//	public:
//		//判断 隐藏、反隐藏、拾取按钮是否被选中，该标识符用于当其他功能中的拾取启用时，取消这三个按钮的选中状态，避免相互影响
//		bool buttonFlag;
//
//		//记录测量面板使用的拾取模式
//		int pickType;
//	};
//}