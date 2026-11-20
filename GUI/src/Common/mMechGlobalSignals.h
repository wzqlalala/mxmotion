#pragma once
#include "mCommon_global.h"
#include "mOutputMessage.h"
#include "mModelEnum.h"
#include "mPhysicalFieldEnum.h"
#include "mModelStruct.h"

//QT
#include <QObject>
#include <QTreeWidgetItem>
#include <set>
#include <qvector.h>
#include <qvector3d.h>

namespace MechCommon
{

	/*
	* 菜单栏/树常驻形态：mMenuWidget（菜单）与 mTreeView（树）共用。
	* 暂时写死为 Motion（多体产品）；合并回 dev_Mech 时把返回值改回 Mech 即可
	*/
	enum class COMMON_EXPORT MenuRibbonForm
	{
		Mech,
		Motion
	};
	inline MenuRibbonForm resolveRibbonForm()
	{
		return MenuRibbonForm::Motion;
	}
	class COMMON_EXPORT mMechGlobalSignals : public QObject
	{
		Q_OBJECT
	public:
		/*
		* 获取单例指针
		*/
		static mMechGlobalSignals* getInstance();

		//模型树右键
		bool _isTreeRight;
	private:
		/*
		* 默认构造函数
		*/
		mMechGlobalSignals() = default;

	private:
		//单例指针
		static mMechGlobalSignals* _instance;

	signals:

		/*
		* 主动发送切换拾取模式(0://智能拾取;1:// 几何点 2://几何边 3:几何面;4:// 几何体;5:// 网格点;6:// 网格线;7:// 网格面; 8:// 网格单元;9:// 部件
		*/
		void activePickSig(int pickFilter);

		/*
		* 当前视口/模型即将切换：由视口模型管理器在切换内部状态（当前模型/视口指针）之前发出，
		* newViewportID 为即将激活的视口全局ID（模型切换/删除等目标视口未知场景传 -1）。
		* 与 currentViewportChanged 的关键区别：后者发出时当前模型已完成切换，
		* 需要以"旧当前模型"为落盘目标的接收者（如通用拾取控件，commit 按 getCVM 写入）
		* 必须在本信号中完成提交，否则旧模型拾取数据会写入新模型
		*/
		void viewportModelAboutToChange(int newViewportID);

		/*
		* 持有"模型内数据裸指针"的模块（如求解任务中心的run记录委托task指针）
		* 须在收到本信号时先行清理，避免模型析构后悬空访问
		*/
		void viewportModelAboutToBeDeleted(int modelID);

		/*
		* 显示信息到状态栏，永不消失
		*/
		void outputPermanentInfoSig(const QString& msg);

		/*
		* 创建集合成功信号
		*/
		void createUpdateSetSig(bool isCreate, int setID);

		/*
		* 批量删除集合信号（数据与树的同步由mTreeView接收处理）
		*/
		void deleteSetsSig(const QVector<int>& setIDs);

		/*
		* 批量删除载荷信号（数据与树的同步由mTreeView接收处理）
		*/
		void deleteLoadsSig(const QVector<int>& loadIDs);

		/*
		* 批量删除边界信号（数据与树的同步由mTreeView接收处理）
		*/
		void deleteBoundarysSig(const QVector<int>& boundaryIDs);

		/*
		* 批量删除预定义场信号（数据与树的同步由mTreeView接收处理）
		*/
		void deletePredefinedFieldsSig(const QVector<int>& fieldIDs);

		/*
		* 创建坐标系成功信号
		*/
		void createUpdateCoordinateSig(bool isCreate, int coordID);

		/*
		* 创建函数成功信号
		*/
		void createUpdateFunctionSig(bool isCreate, int funcID);

		/*
		* 创建截面形状成功信号
		*/
		void createUpdateSectionShapeSig(bool isCreate, int sectionshapeID);

		/*
		* 创建分析工况成功信号
		*/
		void createUpdateCaseSig(bool isCreate, int caseID);

		/*
		* 隐式模态分析转子动力学设置发生变化
		*/
		void caseRotorSettingChangedSig(int caseID);

		/*
		* 创建载荷成功信号
		*/
		void createUpdateLoadSig(bool isCreate, int loadID);

		/*
		* 创建边界成功信号
		*/
		void createUpdateBoundarySig(bool isCreate, int boundaryID);

		/*
		* 创建旋转分析成功信号
		*/
		void createUpdateRotateAnalyseSig(bool isCreate, int rotateAnalyseID);

		/*
		* 创建预定义场成功信号
		*/
		void createUpdatePredefinedFieldSig(bool isCreate, int fieldID);

		/*
		* 创建连接成功信号
		*/
		void createUpdateConnectorSig(bool isCreate, int connectorID);

        /*
        * 连接\连接副名称更新
        */
        void updateConnectorName(const int phyID, const QString& OldName, const QString& NewName, const QString& tarName);

        /*
        * 创建连接副成功信号
        */
        void createUpdateConnectorDeputySig(bool isCreate, int connectorID);

		/*
		* 创建接触成功信号
		*/
		void createUpdateContactSig(bool isCreate, int contactID);

		/*
		* 批量创建接触
		*/
		void batchCreateContactSig(std::vector<int> ids);

		/*
		* 创建模型修复成功信号
		*/
		void createUpdateModelFixSig(bool isCreate, int modelFixID);

		/*
		* 模型修复管理器中，勾选删除时发出信号，实时更新模型树
		*/
		void updateModelFixTreeView(int caseID, int modelFixID, int way);

		/*
		* 创建材料成功信号
		*/
		void createUpdateMaterialSig(bool isCreate, int matID);

		/*
		* 加载自定义材料库
		*/
		void loadCustomDepotSig();

        /*
        * 复合纤维增强材料
        */
        void loadFrpCompMatSig();

        /*
        * 添加到自定义材料库
        */
        void addToMaterialDepot(QVector<int> matIds);

        /*
        * 创建结构成功信号
        */
		void createUpdateOptimizationSig(bool isCreate);

        /*
        * 创建结构优化设计域成功信号
        */
        void createUpdateOptimizationDesignSig(bool isCreate, int designID);

        /*
        * 创建结构优化约束成功信号
        */
        void createUpdateOptimizationConstraintSig(bool isCreate, int designID);

		/*
        * 创建流道结构成功信号
        */
		void createUpdateRunnerOptimizationSig(bool isCreate);

        /*
        * 创建结构优化设计域成功信号
        */
        void createUpdateRunnerOptimizationDesignSig(bool isCreate, int designID);

        /*
        * 创建结构优化约束成功信号
        */
        void createUpdateRunnerOptimizationConstraintSig(bool isCreate, int designID);

        /*
        * 创建超单元参数成功信号
        */
        void createUpdateGenericParamSig(bool isCreate, int designID);

        /*
        * 创建超单元矩阵成功信号
        */
        void createUpdateGenericMatrixSig(bool isCreate);

        /*
        * 创建超单元调用成功信号
        */
        void createUpdateGenericAssignSig(bool isCreate);

        /*
        * 创建超单元部件成功信号
        */
        void createUpdateGenericPartSig(bool isCreate);

        /*
        * 创建求解控制成功信号
        */
        void createUpdateSolveControlSig(bool isCreate);

        /*
        * 创建全局输出控制成功信号
        */
        void createUpdateGlobalOutputControlSig(bool isCreate);

  /*
        /*
        * 创建声固耦合对搜索成功信号
        */
        void createUpdateASCouplingSearchSig(bool isCreate);

        /*
        * 创建属性成功信号
        */
        void createUpdatePropertySig(bool isCreate, int propertyID);

        /*
        * 创建多体运动体成功信号
        */
        void createUpdateMotionBodySig(bool isCreate, int motionBodyID);

        /*
        * 批量创建多体运动体成功信号（树一次追加全部节点+一次选中/状态刷新，
        * 替代逐体发射 createUpdateMotionBodySig 的 N 次选中/滚动/信息表重建）
        */
        void createUpdateMotionBodyBatchSig(const QVector<int>& motionBodyIDs);

        /*
        * 创建多体运动副成功信号
        */
        void createUpdateMotionJointSig(bool isCreate, int motionJointID);

        /*
        * 创建多体耦合副成功信号
        */
        void createUpdateMotionCouplingSig(bool isCreate, int motionCouplingID);

        /*
        * 创建多体驱动成功信号
        */
        void createUpdateMotionDriveSig(bool isCreate, int motionDriveID);

        /*
        * 创建多体标记点成功信号
        */
        void createUpdateMotionMarkerSig(bool isCreate, int motionMarkerID);

        /*
        * 创建多体力元（矢量力/力矩/标量力等）成功信号
        */
        void createUpdateMotionVectorForceSig(bool isCreate, int motionVectorForceID);

        /*
        * 创建多体接触成功信号
        */
        void createUpdateMotionContactSig(bool isCreate, int motionContactID);

        /*
         * 创建多体弹簧成功信号
         */
        void createUpdateMotionSpringSig(bool isCreate, int motionSpringID);

        /*
        * 创建多体控制输入成功信号
        */
        void createUpdateMotionControlInputSig(bool isCreate, int motionControlInputID);

        /*
        * 创建多体机电导入成功信号
        */
        void createUpdateMotionElectroImportSig(bool isCreate, int motionElectroImportID);

        /*
        * 创建多体机电导出成功信号
        */
        void createUpdateMotionElectroExportSig(bool isCreate, int motionElectroExportID);

        /*
        * 创建多体液压协同成功信号
        */
        void createUpdateMotionHydraulicSig(bool isCreate, int motionHydraulicID);

        /*
        * 创建多体强化学习成功信号
        */
        void createUpdateMotionRLSig(bool isCreate, int motionRLID);

        /*
        * 创建多体传感器成功信号
        */
        void createUpdateMotionSensorSig(bool isCreate, int motionSensorID);

        /*
        * 创建多体控制器成功信号
        */
        void createUpdateMotionControllerSig(bool isCreate, int motionControllerID);

        /*
        * 创建多体自动柔性体成功信号
        */
        void createUpdateMotionAutoFlexSig(bool isCreate, int motionAutoFlexID);

        /*
        * 创建多体方案成功信号
        */
        void createUpdateMotionScenarioSig(bool isCreate, int motionScenarioID);

        /*
        * 多体方案求解成功信号
        */
        void createUpdateMotionResultSig(bool isCreate, int motionScenarioID);

        /*
        * 创建非结构质量子节点成功信号
        */
        void createUpdateNonStructuralQualitySig(bool isCreate, int nonStructuralQualityID);

        /*
        * 创建/更新场子节点成功信号（离散场与分析场共用，按数据子类型区分）
        */
        void createUpdateFieldSig(bool isCreate, int fieldID);
		/*
		* 创建三维结果信号
		*/
		void create3DResultSig();

		/*
		* 菜单栏点击三维结果
		*/
		void click3DResultSig();

		/*
		* 菜单栏点击曲线
		*/
		void clickPostCurveSig();

		/*
		* 曲线页点击表格（曲线数据表格窗口）
		*/
		void clickShowTableDataSig();

		/*
		* 曲线显隐变化后刷新曲线显示
		*/
		void clickUpdateCurveDisplaySig();

		/*
		* 曲线页点击曲线样式
		*/
		void clickCurveStyleSig();

		/*
		* 结果页NVH诊断优化：面板贡献量
		*/
		void clickPanelContributionSig();

		/*
		* 结果页NVH诊断优化：传递路径贡献量（dev_base 的 mPostBrowserWindow 在用，
		* dev_base 版头文件此声明在别处，合并到此处统一保留）
		*/
		void clickPathContributionSig();

		/*
		* 动画页帧控制：第一帧/上一帧/下一帧/最后一帧
		*/
		void clickAniFirstFrameSig();
		void clickAniPrevFrameSig();
		void clickAniNextFrameSig();
		void clickAniLastFrameSig();

		/*
		* 动画页播放控制：播放暂停开关/动画参数区启用状态
		*/
		void clickAniPlaySig(bool isrun);
		void clickAniSetStatusSig(bool isEnable);

		/*
		* 动画页渲染设置：渲染范围/循环播放/失效单元隐藏/动画速率
		*/
		void clickAniSetRenderRangeSig(const QString& type);
		void clickAniSetLoopPlaySig(bool istrue);
		void clickAniSetHideFailedElementSig(bool checked);
		void clickAniSetFrameRateSig(int fr);

		/*
		* 动画页输出：导出视频/自定义视频
		*/
		void clickAniExportVideoSig();
		void clickCreateCustomVideoSig();

		/*
		* 云图页查询：显示值/隐藏值/隐藏全部/高级查询
		*/
		void clickCloudShowValueSig();
		void clickCloudHideValueSig();
		void clickCloudHideAllSig();
		void clickAdvancedQuerySig();

		/*
		* 云图页过滤：启动过滤(min,max,规则)/清空过滤
		*/
		void clickCloudStartFilterSig(QString min, QString max, QString type);
		void clickCloudClearFilterSig();

		/*
		* 云图页导出过滤后模型STL（文件路径由发起方选定）
		*/
		void clickExportFilterSTLSig(QString fileName);

		/*
		* 截屏双钮（主页视图区）：快速截屏/自定义截屏
		* （接收端mPostInfoHub，经总线解耦MainWindow->PostWindow）
		*/
		void clickAniQuickScreenshotSig();
		void clickCustomScreenshotSig();

		/*
		* 命令行导入成功后自动快照（带求解目录路径）
		*/
		void clickAniQuickScreenshotPathSig(const QString& path);

		/*
		* 前处理树入口的注释/3D结果创建（结果树右键"标注"、导入后处理结果）
		*/
		void clickCreateNoteSig(int caseid);
		void clickCreate3DResultSig(bool isRead, int caseid);

		/*
		* 主窗口resize/move：重绘自定义截屏框
		*/
		void requestRepaintRecordWindowSig();

		/*
		* 参数栏总复位时连带复位后处理面板（路径拾取态等，
		* 接收端mPostInfoHub::uneditPath，直连保持原同步语义）
		*/
		void requestUneditPostPanelsSig();

		/*
		* 前处理树信号挂起/恢复（mViewTreeFunction 视口激活操作包裹用；
		* 接收端mBrowserWindow直连到树视图 disconnectSig/connectSig，保持同步语义）
		*/
		void suspendBrowserTreeSig();
		void resumeBrowserTreeSig();

		/*
		* 打开后处理结果文件：云图结果 / 曲线结果
		* （入口：主页文件菜单"后处理"子菜单与主页"后处理"按钮，
		*   执行方：mPostBrowserWindow——后处理树建/删入口所在）
		*/
		void clickOpenPostCloudFileSig();
		void clickOpenPostNVHFileSig();

		/*
		* 后处理结果树整体刷新（导入/删除模型后；发送方mPost3DResult等，
		* 接收方PostWindow侧mPostBrowserWindow）
		*/
		void updateResultTreeSig();


        /*
        * 后处理结果文件导入失败（异步解析失败/结果库无效；发送方mPost3DResult，
        * postModelID 为导入启动时捕获的宿主后处理模型ID——失败瞬间当前模型可能已漂移）。
        * 接收方：会话恢复协调器mPostSessionCoordinator（恢复中跳过该条继续推进，
        * 无参旧槽连接兼容）、后处理树窗口mPostBrowserWindow（回滚槽回收创建漏斗
        * 新建的空模型树）。异步失败时 key 级作废通知见 postResultCreateFailedSig
        */
        void postResultImportFailedSig(int postModelID);

        /*
        * 菜单栏导出Word
        */
        void clickPostExportWordSig();

        /*
        * 菜单栏功能阵列
        */
        void clickPostParrayMimageSig();

        /*
        * 菜单栏功能阵列
        */
        void clickPostCompVideoSig();

        /*
        * 对比视频-独立视口形态（图表卡片同款"信号请求+主窗口代办"模式）：
        * PostWindow 侧 mVideoWindow 不依赖 MainWindow，建/关视口格子经此请求；
        * 外部删除（视图树删节点）回落时经 sig_videoViewportClosed 通知面板回弹
        */
        void sig_requestVideoViewportOpen();
        void sig_requestVideoViewportClose();
        void sig_videoViewportClosed();

        /*
        * 菜单栏爆炸视图
        */
        void clickPostExplodeSig();

        /*
        * 菜单栏测量工具视图
        */
        void clickPostMeasureToolSig();

        /*
        * 菜单栏截面力
        */
        void clickPostSectionForceSig();

        /*
        * 菜单栏曲线提取（结果分析组，参数面板）
        */
        void clickPostCurveExtractSig();

        /*
        * 菜单栏查询（结果分析组，V1.3 查询参数面板，≠既有"高级查询"clickAdvancedQuerySig）
        */
        void clickPostQuerySig();

        /*
        * 菜单栏截面剖切（结果页空间显示组，参数面板）
        */
        void clickPostSectionCutSig();

        /*
        * 菜单栏步序列（结果页工况组，V1.3 派生工况参数面板，≠云图派生旧线性叠加 clickLinearSuperpositionSig）
        */
        void clickPostStepSequenceSig();

        /*
        * 菜单栏包络（结果页工况组，V1.3 包络派生工况参数面板）
        */
        void clickPostEnvelopeSig();

        /*
        * 菜单栏向量图（结果页结果图形组，V1.3 向量参数面板，≠云图派生旧矢量图 clickVectorSig）
        */
        void clickPostVectorPlotSig();

        /*
        * 菜单栏模态贡献量分析（曲线页诊断组，唯一入口）：唤醒 NVHToolSet 独立
        * 窗口页（mPostInfoHub::createModalContribution→activateDock；
        * add-nvh-toolset-window-page 2026-11-16 裁定，参数栏嵌入链已下线）
        */
        void clickPostModalContributionSig();

        /*
        * 菜单栏路径结果显示
        */
        void clickPostResultPathSig();

        /*
        * 菜单栏点击注释
        */
        void clickPostNoteSig();

        /*
        * 菜单栏点击标注
        */
        void clickPostLabelSig();

        /*
        * 菜单栏点击标签（结果页签·结果分析组，≠既有"标注"的 clickPostLabelSig）
        */
        void clickPostTagSig();

		/*
		* 菜单栏点击路径
		*/
		void clickPathSig();

		/*
		* 点击动画信号（1:一键生成 2:自定义）
		*/
		void clickAnimationSig(int type);

		/*
		* 创建动画信号（1:一键生成 2:自定义）
		*/
		void createAnimationSig(int type);

		/*
		* 点击线性叠加信号
		*/
		void clickLinearSuperpositionSig();

		/*
		* 点击标量运算信号
		*/
		void clickScalarOperationSig();

		      /*
		      * 点击切面图信号
		      */
        void clickSectionSig();

        /*
        * 点击等值面图信号
        */
        void clickIsosurfaceSig();

        /*
        * 点击等值面图信号（结果页工具栏：打开等值面参数面板，作用于当前激活
        * 云图视口，不建树节点；树节点创建入口仍走 clickIsosurfaceSig）
        */
        void clickIsosurfacePanelSig();

        /*
        * 点击变形控制信号（打开变形控制参数面板）
        */
        void clickDeformationSig();

        /*
        * 创建云图缓存树节点信号（viewName-云图视口名 presetIndex-缓存快照索引，负 phyID 子节点）
        */
        void createCloudPresetSig(QString viewName, int presetIndex);

        /*
        * 点击矢量图信号
        */
        void clickVectorSig();

        /*
        * 点击流线图信号
        */
        void clickStreamLineSig();

        /*
        * 点击分区信号
        */
        void clickPartitionSig();

        /*
        * 点击集合信号
        */
        void clickPostSetSig();

        /*
        * 模型页按类型创建集合信号（setType 按原型按钮序：0 部件集/1 节点集/2 单元边集/
        * 3 单元面集/4 单元集，接收槽内映射到集合面板类型下拉）
        */
        void clickPostSetCreateSig(int setType);

        /*
        * 模型页创建后处理坐标系信号（coordType：0 直角/1 柱/2 球，
        * 对应 MxFunctions::CoordinateType Cartesian/Cylindrical/Spherical，供后续接线）
        */
        void clickPostCoordinateCreateSig(int coordType);

        /*
        * 点击二维线信号
        */
        void clickLineSig();

        /*
        * 点击箭头信号
        */
        void clickArrowSig();

        /*
        * 点击文本信号
        */
        void clickTextSig();

        /*
        * 点击Log信号
        */
        void clickLogSig();

		/*
		* 创建线性叠加节点信号
		*/
		void createLinearSuperpositionSig();

		/*
		* 创建标量运算视口信号
		*/
		void createScalarOperationViewport(QString viewport);

		/*
		* 创建标量运算节点信号
		*/
		void createScalarOperationSig();

		 /*
		* 创建切面图节点信号
		*/
        void createSectionSig();

        /*
       * 创建集合节点信号
       */
        void createPostSetSig();

        /*
        * 创建等值面图节点信号
        */
        void createIsosurfaceSig();

        /*
        * 创建矢量图节点信号
        */
        void createVectorSig();

        /*
        * 创建流线图节点信号
        */
        void createStreamLineSig();

  /*
  * 创建曲线信号
  */
  void createCurveSig();

		/*
		* 创建路径信号
		*/
		void createPathSig();

        /*
        * 创建二维线节点信号
        */
        void createLineSig();

        /*
        * 创建箭头节点信号
        */
        void createArrowSig();

        /*
        * 创建导出Word报告节点信号
        */
        void createExportWordSig();

        /*
        * 创建文本节点信号
        */
        void createTextSig();

        /*
        * 创建Log节点信号
        */
        void createLogSig();

		/*
		* 创建三维结果视口
		*/
		void create3DResultViewport(QString viewportName); 

		/*
		* 创建动画视口（FR-01/FR-05 单视口形态：语义为"进入动画模式"，viewportName
		* 已是云图视口名，不再派生/创建第二视口）
		*/
		void createAnimationViewport(QString viewportName, int type);

		/*
		* 重算后处理 Ribbon 页启停（"结果/云图/动画"页 enable 态）。FR-01/FR-05 单视口
		* 形态下动画在云图视口就地生成/播放、全程无视口切换——原依赖 currentViewportChanged
		* 触发的"动画"页启停重算改为生成/删除收尾显式发射本信号（mMenuWidget 消费）
		*/
		void refreshPostMenuEnableSig();

		/*
		* 创建线性叠加视口
		*/
		void createLinearViewport(QString viewportName);

		/*
		* 设置动画菜单栏禁用状态
		*/
		void setAnimationMenuStatus(bool isenable);

		/*
		* 传递postrender发出的信号传到动画界面
		*/
		void updateAnimationPanelSigManger(int f);

		/*
		* 更新三维结果
		*/
		void update3DResultMenu(QString viewport);

		/*
		* 高级查询切帧信号
		*/
		void changedFrame(int f);

		/*
		* 云图面板切帧信号
		* 用于通知标注面板等跟随云图当前帧刷新
		*/
		void postCloudFrameChanged(int f);

		/*
		* 更新三维结果菜单界面
		*/
		void updateResultMenuGUI();

		/*
		* 更新三维结果信息页界面
		*/
		void updateResultInfoGUI();

		/*
		* 更新后处理视口名称
		*/
		void updatePostViewName(QString name);

		/*
		* 更新后处理模型树
		*/
		void updatePostModelTree();

		/*
		* 后处理测量记录变更（测量工具面板镜像写回数据管理器后发射；
		* 后处理树"测量"分支据此同步子节点）
		*/
		void postMeasureRecordsChangedSig();

		/*
		* 后处理测量编辑对象变化（测量工具面板选中切换/新建登记/删除回落后发射；
		* 参数=当前编辑记录 id（-1 无编辑对象）。后处理树"测量"分支高亮据此跟随
		* ——V1.3"延迟切换后高亮与编辑对象一致"）
		*/
		void measureEditingChangedSig(int recordId);

		/*
		* 后处理标签记录变更（结果标签面板记录结构变化[新建/转正/删除]后发射；
		* 后处理树"标签"分支据此重建子节点）
		*/
		void postTagRecordsChangedSig();

		/*
		* 移除后处理模型树根（后处理树模型独立后，前处理侧删除后处理模型时
		* 经本信号请求后处理树移除对应根节点；参数为模型数据ID）
		*/
		void removePostModelTreeSig(int modelDataID);

		/*
		* 计算完成跳转后处理：携带结果文件路径列表请求打开后处理
		* （新建后处理模型+树+视口 → 导入结果 → 视口激活自动切换 Ribbon/dock 到后处理模式）。
		* 由求解接入方在计算完成回调中 emit；若发自非UI线程，接收端为Auto连接会排队到主线程执行
		*/
		void solveFinishedSig(const QStringList& resultFilePaths);

		/*
		* 幂等跳转到后处理结果（"再次跳转"接口）：resultKey 为结果跳转注册表的实例key
		* （创建实例的modelDataID十进制串，见 PostCommon::IPostResultService），
		* resultFilePaths 为导入回退用的完整文件列表。接收端（后处理树窗口）查结果跳转注册表：
		* 命中→激活已创建的云图视口（不重复建模型/导入，结果文件已更新时删旧重建）；
		* 未命中→回退为 solveFinishedSig 同款"新建+导入"并登记新key
		*/
		void requestShowPostResultSig(const QString& resultKey, const QStringList& resultFilePaths);

		/*
		* 后处理实例失效通知：后处理侧某结果实例（resultKey，见 PostCommon::IPostResultService）
		* 被删除或登记失效时发射。接收方（前处理）应作废保存的同值key（入口置灰/标记失效）。
		* 触发边界：模型删除（含showPostResult存在性校验发现模型已删）时发射；
		* 结果文件mtime变化"删旧重建该key实例"（key保留）不发；进程退出不发（key契约进程内）
		*/
        void postResultRemovedSig(const QString& resultKey);

        /*
        * 后处理实例创建失败通知（key级）：异步导入解析失败且回滚整个新建模型时，
        * 由 mPostBrowserWindow 回滚槽对被作废的每个key发射（先于 postResultRemovedSig）。
        * 与 postResultRemovedSig 的语义区分：本信号="该实例的异步导入失败"（前处理提示
        * "结果导入失败"）；postResultRemovedSig="实例已被删除"（入口置灰）。两信号相伴
        * 到达、收到任一即作废key；仅"异步导入失败"场景有本信号
        */
        void postResultCreateFailedSig(const QString& resultKey);

		/*
		* 更新动画菜单栏界面
		*/
		void updateAnimationMenuGUI(QString view);

		/*
		* 更新动画失效单元隐藏开关界面
		*/
		void updateAnimationHideFailedElementGUI(bool checked);

		/*
		* 更新动画参数页界面
		*/
		void updateAnimationInfoGUI();

		/*
		* 设置动画输出视口
		*/
		void setAnimationOutputViewport(QString view);

		/*
		* 动画输出时暂停动画
		*/
		void pauseAnimation();

		/*
		* 更新三维结果
		*/
		void updateFreeNodeBtnStatus(bool isDisabled);

		/*
		* 切换菜单栏窗口
		*/
		void switchMenuBarWindow(const QString& name);

		/*
		* 创建网格(全局尺寸)成功信号
		*/
		void createUpdateGlobalDimSig(bool isCreate);



		/*
		* 创建网格(体网格)成功信号
		*/
		void createUpdateMeshSig(bool isCreate,int meshPanelID);

		/*
		* 创建网格操作节点成功信号
		*/
		void createUpdateMeshOptSig(bool isCreate,int meshPanelID);

		/*
		* 视图事件过滤器
		*/
		void signal_osgWidgetEvent(QEvent* event);

		/*
		* 获取屏幕绘制曲线的点坐标
		*/
		void signal_GeoDividerLineData(QVector<QVector3D>);

		/*
		* 模型树状态更新
		*/
		void updateBrowserTreeStatus(MxFunctions::PhysicalFieldType, QHash<int, MxFunctions::BrowserTreeStatus>);

		/*
		* 后处理模型树状态更新
		*/
		void updatePostBrowserTreeStatus(MxFunctions::BrowserTreeType, int phyID, int caseID, MxFunctions::BrowserTreeStatus statu);

		/*
		* 信息输出表格更新
		*/
		void updateOutPutInfoTable(const QHash<QString, QSet<int>>&, const QList<outputInfoData>&);

		/*
		* 常态信息表格更新
		*/
		void updataNormalInfo(QList<outputInfoData>&);

		/*
		* 重命名，改变信息输出表格
		*/
		void resetNameUpdateOutPutInfoTable(const QString& module, const int& phyID, const QString& newName);

		/*
		* 单位系统切换
		*/
		void switchUnitSystem();

		/*
		* 单位制切换
		*/
		void unitSystemChanged(int);
		void angleUnitChanged(int);
		void speedUnitChanged(int);
		void tempUnitChanged(int);

		/*
		* 模型树选中状态改变
		*/
		void browsertreeSelectedChanged(const QList<mItemKey>& items, bool valid);

		/*
		* 重新点击树上节点
		*/
		void clikNodesOnTree(bool clearPicked);

		/*
		* 清空截面形状视图
		*/
		void clearGeoWidget();
		
		/*
		* 相同网格类型的交集，更新状态
		*/
		void updateMeshStatus();

		/*
		* 材料属性可用性（主线装配/材料联动信号，装配树重建时屏蔽全部材料属性面板）
		*/
		void disableAllMaterialProps(bool on);

		/*
		* 删除几何，更新网格子节点列表
		*/
		void updateMeshList(std::map<int, std::set<int>> map);

		/*
		* 导入几何模型，更新状态
		*/
		void updateStatusImportGeo();

		/*
		* 更新结果状态（modelID为结果所属模型，接收方据此定位模型而非当前模型）
		*/
		void updateStatusPostResult(int modelID, int caseID);

        /*
        * 计算完成导入后处理结果（modelID为结果所属模型）
        */
        void importPostResult(int modelID, int caseID);

        /*
        * 模型切换时，更新实时表格
        */
        void updateTableChangedModel(bool isChanged);

        /*
         * 载荷边界管理器中，勾选删除时发出信号，实时更新模型树
         */
        void updataLoadBoundaryTreeView(int caseID, int loadBoundaryId, int way);
      
        /*
         * 预定义场管理器中，勾选删除时发出信号，实时更新模型树
         */
        void updatePredefinedFieldTreeView(int caseID, int fieldID, int way);

        /*
         * 相互作用管理器中，勾选删除时发出信号，实时更新模型树
         */
        void updateContactTreeView(int caseID, int fieldID, int way);
      
        /*
         * 频率采样方式创建成功信号
         */
        void updateFreqSamplPointTree(bool isCreate, int freqSamplPointID);

        /*
        * 层合结构创建成功信号
        */
        void updateLaminateTree(bool isCreate, int laminateID);

        /*
         * 周期性边界创建成功信号
         */
        void createUpdatePeriodicBoundarySig(bool isCreate, int pbID);

        /*
         * 贡献量创建成功信号
         */
        void createUpdateContriSig(bool isCreate, int ContriID, QString name);

        /*
         * 峰值筛选创建成功信号
         */
        void createUpdatePeakFilterSig(bool isCreate, int peakFilterID);

        /*
         * 工况下峰值筛选创建成功信号
         */
        void createUpdateCasePeakFilterSig(bool isCreate, int caseID);

        /*
         * 线程执行完成
         */
        void threadExecuteFinishSig();

        /*
        * 删除模型树几何节点
        */
        void deleteGeoTree(std::set<int> partIds);

        //新增导入成功信号
        void importSuccessful();

        // 写文件前通知界面做准备（无参数）
        void requestPrepareWriteFile();
	public:
		/*
		* mpcomm 平台会话中：平台模式下仅允许单模型（禁用"新建"）。
		*/
		bool _isPlatformMode = false;

	signals:
		/*
		* 平台经 mpcomm 下发求解请求（EXECUTE_ACTION solve）。
		* 主窗口/求解线程收到后触发求解。
		*/
		void mpcommStartSolveRequested();

        /*
        * 几何模型导入完成（含文件路径），供 mpcomm 向平台回传 committed。
        */
        void mpcommImportGeometryFinished(const QString& geometryPath);

        /*
        * mpcomm 平台触发：请求导入指定几何/计算文件（OPEN_NODE/REFRESH_INPUTS 上下文带来）。
        */
        void mpcommImportGeometryRequested(const QString& filePath);
        void mpcommImportCalcFileRequested(const QString& filePath);

        /*
        * mpcomm：运动数据(Motion JSON)导入/导出完成回传（fromPlatform 标记平台触发，桥接防回声）。
        */
        void mpcommImportMotionDataFinished(bool success, const QString& path, bool fromPlatform);

        /*
        * 求解状态变化（started/finished/failed），供 mpcomm 回报平台。
        * outputs 可带 resultPath。
        */
        void mpcommSolverStatusChanged(const QString& state, const QVariantMap& outputs);

        /*
        * 手动导出特征工况 JSON 完成（供 mpcomm 向平台 results 通道上报该文件）。
        */
        void mpcommLoadCaseExported(const QString& filePath);

        /*
        * 模型被删除（删除视口模型），供 mpcomm 通知平台清空单元格状态。
        */
        void mpcommModelCleared();

        /*
        * 计算文件（.mx）导入完成，供 mpcomm 向平台回传 committed(mesh+setup)。
        * 不做回声抑制：所有导入完成（含平台触发的）都回报，平台幂等。
        */
        void mpcommImportCalcFileFinished(bool success, const QString& filePath);

        /*
        * 计算文件（.mx）导出完成（手动"导出计算模型"），供 mpcomm 向平台回传
        * committed(mesh+setup)。求解流程的自动导出不走此信号（求解完成会单独
        * 回报 committed(solution+results)，避免重复刷新 mesh/setup 行状态）。
        */
        void mpcommExportCalcFileFinished(bool success, const QString& filePath);

        /*
        * mpcomm 平台触发：接收到上游场数据（role=mpif，OPEN_NODE/REFRESH_INPUTS
        * 上下文带来），请求处理该文件。
        */
        void mpcommImportMpifRequested(const QString& filePath);

        /*
        * mpcomm：OPEN_NODE 挂起的后处理结果文件（前序导入完成后触发），请求
        * 打开这些结果文件（执行方为 PostWindow 的后处理树窗口：建 post 模型树/
        * 视口+读文件+登记跳转）。
        */
        void mpcommImportPostResultsSig(const QStringList& filePaths);

        /*
        * mpcomm：桥接已把 OPEN_NODE 的待导数据（templateId/geometry/calcfile/
        * mpif）挂到 qApp 属性上。OPEN_NODE 经队列投递，可能早于也可能晚于界面
        * 构造：界面构造时主动取一次，本信号发出后再取一次，两种时序都覆盖。
        */
        void mpcommPendingContextChanged();

        /*
        * 几何模型导出完成（手动"导出几何模型" stp/step/iges），供 mpcomm 向平台
        * 回传 committed(geometry)。
        */
        void mpcommExportGeometryFinished(const QString& filePath);
	};
}
