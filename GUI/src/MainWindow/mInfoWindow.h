#pragma once

//解决中文乱码
#pragma execution_character_set("utf-8")
#include "mmainwindow_global.h"
#include "mPanelHostProvider.h"	//IPanelHost接口：PostWindow侧经provider调度面板

#include "mDockBase.h"
#include <QFrame>
#include <QSize>

#include <QTabWidget>
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
#include <QListWidgetItem>
#include <QScrollBar>

#include "mMaterialBase.h"
#include "mGeoPanelBase.h"
#include"mStructural.h"
#include "mPhysicalFieldEnum.h"
#include "mMotionForceElement.h"
#include "mMotionDriveBase.h"
#include "mMotionSensorType.h"


namespace MechMainWindow
{
    class mDiscreteFieldPanel;
    class mAnalysisFieldPanel;
    class mNsmManagerUnderCasePanel;
    class mModelFixManaUnderCasePanel;
    class mASCouplingSearchPanel;
    class mNonStructuralQualityPanel;
    class mStructuralOptimizationPanel;
    class mOptimizationGoalPanel;
    class mOptimizationDesignPanel;
    class mOptimizationConstraintPanel;
	//多体动力学面板（懒加载，ensurePanel实例化）
	class mMotionBodyPanel;
	class mMotionBodyBatchPanel;
	class mMotionJointPanel;
	class mMotionCouplingPanel;
	class mMotionDrivePanel;
	class mMotionMarkerPanel;
	class mMotionVectorForcePanel;
	class mMotionContactPanel;
	class mMotionSpringPanel;
	class mMotionScenarioPanel;
	class mMotionAnimationPanel;
	class mMotionLoadExportPanel;
	class mMotionCurvePanel;
	class mMotionControlInputPanel;
	class mMotionElectroImportPanel;
	class mMotionElectroExportPanel;
	class mMotionHydraulicPanel;
	class mMotionRLPanel;
	class mMotionSensorPanel;
	class mMotionControllerPanel;
	class mMotionAutoFlexPanel;
	class mMotionMechanismPanel;
	class mMotionTransmissionPanel;
	class mMotionMechElecHydPanel;
    class mRunnerOptimizationPanel;
    class mRunnerOptimizationGoalPanel;
    class mRunnerOptimizationDesignPanel;
    class mRunnerOptimizationConstraintPanel;
    class mGenericParamPanel;
    class mGenericAssignPanel;
    class mGenericMatrixPanel;
    class mGenericPartPanel;
    class mSolveControlPanel;
    class mGlobalOutputControlPanel;
	class mContactPairPanel;
	class mSectionShapePanel;
	class mFunctionPanel;
	class FoldTable;
	class mCasePanel;
	class mCaseSettingPanel;
	class mSetPanel;
	class mConnectorPannel;
	class mMotionLoadImportPanel;
	class mDeputyPannel;
	class mPeriodicBoundaryPanel;
	class mConnectorSearchPannel;
	class mPropertyPanel;
	class mElementPanel;
	class mPropertyCheckPanel;
	class mSetManagerPanel;
	class mCoordinatePanel;
	class mNodeCoordinatePanel;
    class mStructual;
	class mContactPanel;
	class mGlobalDimPanel;
	class mLocalSizePanel;
	class mCreatePointPanel;
	class mMeshEditPanel;
	class mCreateMeshPanel;
	class mCreateMetaMeshPanel;
	class mExtractSurfacePanel;
	class mRotateMeshPanel;
	class mMeshPanel;
	class mTranslationPanel;
	class mRotationPanel;
	class mStretchPanel;
	class mZoomPanel;
	class mMovePointPanel;
	class mChangeOrderPanel;
    class mDeleteMeshPanel;
	class mPostCompVideoPanel;   //【集成回补】dev 后处理面板前向声明（成员声明消费）
	class mSharedNodePanel;
	class mFreeEdgePanel;
	class mTEdgePanel;
	class mMeshIntersectPanel;
	class mMeshOverlapPanel;
	class mMeshInterferePanel;
	class mMeshStatisticsPanel;
	class mMirroringPanel;
	class mMatrixPanel;
	class mSmoothPanel;
	class mRemeshingPanel;
	class mSTLRemeshPanel;
	class mMeshNormalPanel;
	class mMoveMeshPanel;
	class mRenumberPanel;
	class mQualityPanel;
	class mCustomEncryptionPanel;
	class mAutoAdaptPanel;
	class mSolveTaskPanel;
	class mLineMeshPanel;
	class mEdgeSuppressPanel;
	class mFaceMeshPanel;
	class mSolidMeshPanel;
	class mMaterialPanel;
	class mWorkingConditionPanel;
	class mMaterialProps;
	class mMaterialPropBase;
	class mGeoFeaturePanel;
	class mGeoTransformPanel;
	class mGeoBoolOperationPanel;
	class mGeoEditPanel;
	class mGeoRepairPanel;
	class mGeoSolidifyPanel;
	class mGeoRemovalPanel;
	class mGeoDirectModelingPanel;
	class mGeoExtractPanel;
	class mGeoInfoPanel;
	class mGeoSharedTopologyPanel;
	class mGeoExtractPanel;
	class mWorkingDirPanel;
    class mModelTreeManager;
    class mFreqSamplPointPanel;
    class mLaminatePanel;
    class mModalContriPanel;
    class mNodeContriPanel;
    class mPanelContriPanel;
    class mTPContriPanel;
    class mERPPanel;
    class mPeakFilterPanel;
    class mCasePeakFilterPanel;
    class mEleSetCtrlPanel;
	class mMenuWidget;

	class MMAINWINDOW_EXPORT mInfoWindow : public QWidget, public MechCommon::IPanelHost
	{
		Q_OBJECT
	//signals:
	//	void setBtnEnabled();
	public:

  /*
  * 获取单例指针
  */
  static mInfoWindow* getInstance();

        /*
        * 获取几何信息面板
        */
        mGeoInfoPanel* getGeoInfoPanel();

        /*
        * 显示外部创建的面板(如三点确定草图平面面板)
        */
        void showCustomPanel(QWidget* panel);

        //宿主底色 surface 随主题（原为级联全部子孙的写死白色；现只命中自身，
        //刷新由主窗注册的 dock applier 驱动）
        void applyHostTheme();

		void connectEnable(QToolButton* btn, QToolButton* btn1);

		/*
		* 设置菜单栏对象，并透传给管理器表格
		*/
		void setMenuWidget(mMenuWidget* menuWidget);

		/*
		* 析构函数
		*/
		~mInfoWindow();


       
        /***************************************************************************************结构优化***********************************************************************************/

        /*
        * 创建结构优化
        */
        void createOptimization();

        /*
        * 编辑优化参数
        */
        void editOptimization();

        /*
        * 编辑优化目标
        */
        void editOptimizationGoal(const QString &type = "");

        /*
        * 创建设计域
        */
        void createOptimizationDesign(const QString& type = "实体");

        /*
        * 编辑设计域
        */
        void editOptimizationDesign(int id);

        /*
        * 创建优化约束
        */
        void createOptimizationConstraint(const QString& type = "质量");

        /*
        * 编辑优化约束
        */
        void editOptimizationConstraint(int id);

        /*
        * 取消编辑结构优化
        */
        void unEditOptimization();

        /***************************************************************************************流道优化***********************************************************************************/

        /*
        * 创建流道优化
        */
        void createRunnerOptimization();

        /*
        * 编辑优化参数
        */
        void editRunnerOptimization();

        /*
        * 编辑优化目标
        */
        void editRunnerOptimizationGoal(const QString& type = "");

        /*
        * 创建设计域
        */
        void createRunnerOptimizationDesign(const QString& type = "实体");

        /*
        * 编辑设计域
        */
        void editRunnerOptimizationDesign(int id);

        /*
        * 创建优化约束
        */
        void createRunnerOptimizationConstraint(const QString& type = "质量");

        /*
        * 编辑优化约束
        */
        void editRunnerOptimizationConstraint(int id);

        /*
        * 取消编辑流道优化
        */
        void unEditRunnerOptimization();

        /**************************************************************************************全局参数设置***************************************************************/
        /*
        * 创建超单元参数
        */
        void createGenericParam();

        /*
        * 编辑超单元参数
        */
        void editGenericParam(int id);

        /*
        * 创建超单元调用
        */
        void createGenericAssign();

        /*
        * 编辑超单元调用
        */
        void editGenericAssign();

        /*
        * 创建声固耦合对搜索
        */
        void createASCouplingSearch();

        /*
        * 编辑声固耦合对搜索(从树上点击过来的)
        */
        void editASCouplingSearchPanel();

        /*
        * 创建超单元矩阵名称
        */
        void createGenericMatrix();

        /*
        * 编辑超单元矩阵名称
        */
        void editGenericMatrix();

        /*
        * 创建求解控制
        */
        void createSolveControl();

        /*
        * 编辑求解控制
        */
        void editSolveControl();

        /*
        * 显示求解任务面板（树上"求解任务"节点点击进入）
        */
        void showSolveTaskPanel();

        /*
        * 创建全局输出控制控制
        */
        void createGlobalOutputControl();
        void editGlobalOutputControl();
        /**************************************************************************************贡献量***************************************************************/
        /*
        * 创建模态贡献量
        */
        void createModalContri(int id);

        /*
        * 编辑模态贡献量
        */
        void editModalContri(int id);

        /*
        * 创建节点贡献量
        */
        void createNodeContri(int id);
        /*
        * 创建超单元部件
        */
        void createGenericPart();

        /*
        * 编辑节点贡献量
        */
        void editNodeContri(int id);

        /*
        * 创建面板贡献量
        */
        void createPanelContri(int id);

        /*
        * 编辑面板贡献量
        */
        void editPanelContri(int id);

        /*
        * 创建传递路径贡献量
        */
        void createTPContri(int id);

        /*
        * 编辑传递路径贡献量
        */
        void editTPContri(int id);

        /*
        * 创建等效辐射功率
        */
        void createERP(int id);

        /*
        * 编辑等效辐射功率
        */
        void editERP(int id);

        /*
         * 创建峰值筛选
         */
        void createPeakFilter();

        /*
         * 编辑峰值筛选
         */
        void editPeakFilter(int id);

        /*
         * 创建工况下峰值筛选
         */
        void createCasePeakFilter(int caseId);

        /*
         * 编辑工况下峰值筛选
         */
        void editCasePeakFilter(int caseId);

  /***************************************************************************************分析工况***********************************************************************************/
        /*
        * 编辑超单元部件
        */
        void editGenericPart();

		/***************************************************************************************分析工况***********************************************************************************/

		/*
		* 创建分析
		*/
		void createCase(MxFunctions::CaseType type, int caseId = 0);

		/*
		* 编辑分析
		*/
		void editCase(int caseId);

		/*
		* 删除分析
		*/
		void deleteCase(int caseId);

		/*
		* 创建载荷
		*/
		void createLoad(MxFunctions::LoadBoundaryType type);

		/*
		* 创建边界
		*/
		void createBoundary(MxFunctions::LoadBoundaryType type);

		/*
		* 创建预定义场
		*/
		void createPredefinedField(MxFunctions::PredefinedFieldType type);

		/*
		* 创建旋转分析
		*/
		void createRotateAnalyse(MxFunctions::RotateAnalysyType type);

		/*
		* 编辑载荷
		*/
		void editLoad(int loadId,int caseId);

		/*
		* 编辑边界
		*/
		void editBoundary(int boundaryId, int caseId);

		/*
		* 编辑预定义场
		*/
		void editPredefinedField(int fieldId, int caseId);

		/*
		* 编辑旋转分析
		*/
		void editRotateAnalyse(int rotateId, int caseId);

		/*
		* 取消编辑载荷
		*/
		void unEditLoad();

		/*
		* 取消编辑边界
		*/
		void unEditBoundary();

		/*
		* 取消编辑预定义场
		*/
		void unEditPredefinedField();

		/*
		* 退出多体载荷导入面板：失活其几何面拾取控件（无拾取编辑态则跳过）
		*/
		void unEditMotionLoadImport();

		/*
		* 提交当前激活拾取控件的数据（导出/计算前调用，指针为空则跳过）。
		*/
		void commitActivePickWidget();

		      /**************非结构质量*************************************/

        /*
       * 创建非结构质量
       */
        void createNonStructuralQuality();

        void editNonStructuralQuality(int nonStructuralQualityId);

        void uneditNonStructuralQuality();

        /**************非结构质量管理********************/
        void editNsmManagerUnderCasePanel(int caseId);

        /**************模型修改管理（工况下）********************/
        void editModelFixManagerUnderCasePanel(int caseId);

        /***************离散场*******************/
        void createDiscreteField();
        void createDiscreteField(MxFunctions::DiscreteSpace spaceType);

        void editDiscreteField(int discreteFieldId);

        /***************分析场*******************/
        void createAnalysisField();
        void editAnalysisField(int analysisFieldId);
        void uneditAnalysisField();

        /*
        * 场数据即将被外部销毁（树上删除场节点/删除全部场）时的安全收尾：
        * 转发到分析场面板丢弃悬垂指针并安全关闭数据表对话框。
        * 必须在 deleteByPhysicalType 之前调用；传 -1 表示即将删除全部场。
        */
        void notifyFieldDataAboutToBeDestroyed(int fieldID);

        /*
        * 接触数据即将被外部销毁时的安全收尾（传 -1 表示即将删除全部接触），
        * 必须在 deleteByPhysicalType 之前调用
        */
        void notifyContactDataAboutToBeDestroyed(int contactID);

        /*
        * 周期边界数据即将被外部销毁时的安全收尾（传 -1 表示即将删除全部），
        * 必须在 deleteByPhysicalType 之前调用
        */
        void notifyPeriodicBoundaryDataAboutToBeDestroyed(int pbID);

        //力元（含衬套）数据即将被删除：面板置空编辑指针防悬垂（周期边界同款约定）
        void notifyVectorForceDataAboutToBeDestroyed(int feID = -1);
        void notifyMotionBodyDataAboutToBeDestroyed(int id = -1);
        void notifyMotionJointDataAboutToBeDestroyed(int id = -1);
        void notifyMotionCouplingDataAboutToBeDestroyed(int id = -1);
        void notifyMotionDriveDataAboutToBeDestroyed(int id = -1);
        void notifyMotionScenarioDataAboutToBeDestroyed(int id = -1);
        void notifyMotionMarkerDataAboutToBeDestroyed(int id = -1);
        void notifyMotionSpringDataAboutToBeDestroyed(int id = -1);
        void notifyMotionContactDataAboutToBeDestroyed(int id = -1);
        void notifyMotionSensorDataAboutToBeDestroyed(int id = -1);
        void notifyMotionControllerDataAboutToBeDestroyed(int id = -1);
        void notifyMotionAutoFlexDataAboutToBeDestroyed(int id = -1);
        void notifyMotionControlInputDataAboutToBeDestroyed(int id = -1);
        void notifyMotionElectroImportDataAboutToBeDestroyed(int id = -1);
        void notifyMotionElectroExportDataAboutToBeDestroyed(int id = -1);
        void notifyMotionHydraulicDataAboutToBeDestroyed(int id = -1);
        void notifyMotionRLDataAboutToBeDestroyed(int id = -1);
		/***************************************************************************************属性***********************************************************************************/

		/*
		* 创建属性
		*/
		void createProperty(PropertyType type);

		/*
		* 编辑属性
		*/
		void editProperty(int propertyId);

		/*
		* 取消编辑属性
		*/
		void uneditProperty();

		/*
		* 检查面板
		*/
		void checkProperty(int type);

		/*
		* 集合管理器面板（树上左键"集合"节点打开）
		*/
		void showSetManager();

		/*
		* 载荷管理器面板（树上左键"载荷管理器"节点打开）
		*/
		void showLoadManager();

		/*
		* 载荷引用面板（树上左键工况下"载荷"父节点打开）
		*/
		void showLoadReference(int caseID);

		/*
		* 边界管理器面板（树上左键"边界管理器"节点打开）
		*/
		void showBoundaryManager();

		/*
		* 边界引用面板（树上左键工况下"边界"父节点打开）
		*/
		void showBoundaryReference(int caseID);

		/*
		* 预定义场管理器面板（树上左键"预定义场"管理器节点打开）
		*/
		void showPredefinedFieldManager();

		/*
		* 预定义场引用面板（树上左键工况下"预定义场"父节点打开）
		*/
		void showPredefinedFieldReference(int caseID);

		/*
		* 接触管理器面板（树上左键"接触"节点打开）
		*/
		void showContactManager();

		/*
		* 接触引用面板（树上左键工况下"相互作用"父节点打开）
		*/
		void showContactReference(int caseID);

		/*
		* 模型修改管理器面板（树上左键"模型修改"节点打开）
		*/
		void showModelFixManager();

		/*
		* 模型修改引用面板（树上左键工况下"模型修改"父节点打开）
		*/
		void showModelFixReference(int caseID);

		/*
		* 退出检查面板
		*/
		void uneditCheckProperty();

		/*
		* 编辑单元
		*/
		void editElement(QString type);

		/*
		* 取消编辑单元
		*/
		void unEditElement();

		/***************************************************************************************集合***********************************************************************************/

		/*
		* 创建集合
		*/
		void createSet();

        void createSet(QString setName);

		/*
		* 编辑集合
		*/
		void editSet(int setId);

		/*
		* 取消编辑集合
		*/
		void uneditSet();

		/***************************************************************************************坐标系***********************************************************************************/

		/*
		* 创建坐标系
		*/
		void createCoordinate(MxFunctions::CoordinateType type = Cartesian, QVector3D origin = QVector3D(0,0,0));

		/*
		* 编辑坐标系
		*/
		void editCoordinate(int coorId);

		/*
		* 取消编辑坐标系
		*/
		void uneditCoordinate();

		/*
		* 创建节点坐标系
		*/
		void createNodeCoordinate();

		/*
		* 取消编辑节点坐标系
		*/
		void uneditNodeCoordinate();

        //新建结构优化面板
        void  createMyPanel();
		/***************************************************************************************函数***********************************************************************************/

		/*
		* 创建函数
		*/
		void createFunction(int type);

		/*
		* 编辑函数
		*/
		void editFunction(int FunctionId);

		/*
		* 编辑截面形状
		*/
		void editSectionShape(int SectionShapeId);

		/*
		* 取消编辑函数
		*/
		void uneditFunction();

		//创建截面形状
		void createSectionShape(int type);

		//复制面板连接信号
		void createCopyPanel(std::vector<int> copyId);
		/***************************************************************************************后处理***********************************************************************************/


		/*
		* 结果页创建族（曲线/word导出/功能阵列/对比视频）已迁 PostWindow::mPostInfoHub，
		* 调用方为 mPostTreeView::slot_createXXX
		*/

		/*
		* 曲线模块：按节点id在参数栏切换属性页（曲线树选中节点时调用）
		*/
		void editCurveNode(int nodeId);

        /*
            创建标注面板及节点

        /*
            导入完成后自动应用标注（不切出 Info 面板）

        /*
            创建爆炸视图面板及节点

        /*
            创建测量工具视图面板及节点

        /*
            创建路径结果显示面板及节点
        */

       
       


		/*
		* 切换显示曲线/导出Word面板：已迁PostWindow::mPostInfoHub
		*（调用方mPostBrowserWindow直接调hub）
		*/


		

		//曲线-更新显示曲线/显示曲线表格：接收端已随曲线面板迁PostWindow（mPostBrowserWindow）

        //贡献量-模态贡献量

        //贡献量-面板贡献量

        //贡献量-传递路径贡献量

		/***************************************************************************************材料模块***********************************************************************************/
		/*
		* 新建材料
		*/
		void createMaterial(MxFunctions::MaterialType matType = LinearElastic_Isotropic);

		/*
		* 编辑材料
		*/
		void editMaterial(int matId);

        /*
        * 切回编辑材料
        */
        void updateMaterial(int matId);

		/*
		* 取消编辑
		*/
		void uneditMaterial();

		/********************* 多体动力学：面板入口（面板经ensurePanel懒加载） **********************/
		void createMotionBody();
		void editMotionBody(int id);
		void unEditMotion();
		//批量定义运动体：进面板选部件子集，确定逐部件 1:1 建体
		void createMotionBodyBatch();
		//一键生成运动体：当前模型全部部件直接建体（不进面板）
		void createMotionBodyOneKey();
		void createMotionJoint(MxFunctions::JointType type);
		void editMotionJoint(int id);
		void createMotionCoupling(MxFunctions::CouplingType type);
		void editMotionCoupling(int id);
		void createMotionDrive(MxFunctions::DriveTargetType type, MechMotionData::DriveControlMode mode = MechMotionData::DriveControlMode::Velocity);
		void editMotionDrive(int id);
		void createMotionMarker();
		void editMotionMarker(int id);
		void createMotionVectorForce(MechMotionData::VectorLoadType type);
		void editMotionVectorForce(int id);
		void createMotionContact(int type);
		void editMotionContact(int id);
		void createMotionSpring(int type);
		void editMotionSpring(int id);
		void createMotionControlInput();
		void editMotionControlInput(int id);
		void createMotionElectroImport();
		void editMotionElectroImport(int id);
		void createMotionElectroExport();
		void editMotionElectroExport(int id);
		void createMotionHydraulic();
		void editMotionHydraulic(int id);
		void createMotionRL();
		void editMotionRL(int id);
		void createMotionSensor(MechMotionData::SensorType type = MechMotionData::SensorType::BodyDisplacement);
		void createMotionController();
		void editMotionSensor(int id);
		void editMotionController(int id);
		void createMotionAutoFlex();
		void editMotionAutoFlex(int id);
		void createMotionScenario();
		void editMotionScenario(int id);
		void solveMotionScenario(int id = 0);
		void editMotionAnimation(int id);
		void editMotionLoadExport(int id);
		void createMotionCurveSelector(int scenarioId);
		void editMotionCurveSelector(int scenarioId);

		//控制输入/机电耦合/液压/强化学习/系统建模工具（Ribbon调用，无Motion前缀历史命名）
		void createControlInput();
		void editControlInput(int id = 0);
		void uneditControlInput();
		void createElectroImport();
		void editElectroImport(int id = 0);
		void uneditElectroImport();
		void createElectroExport();
		void editElectroExport(int id = 0);
		void uneditElectroExport();
		void createHydraulic();
		void editHydraulic(int id = 0);
		void uneditHydraulic();
		void createRLPanel();
		void editRLPanel(int id = 0);
		void uneditRLPanel();
		void createMechanism();
		void editMechanism(int id = 0);
		void uneditMechanism();
		void createTransmission();
		void editTransmission(int id = 0);
		void uneditTransmission();
		void createMechElecHyd();
		void editMechElecHyd(int id = 0);
		void uneditMechElecHyd();

		/*
		* 保存材料属性
		*/
		void storeMaterialProps();

		/*
		* 添加材料属性
		*/
		void addMaterialProp(const MxFunctions::MaterialClass& propname);

		/*
		* 清空材料属性数据
		*/
		void clearMaterialProp(const MxFunctions::MaterialClass& propname);

		/*
		* 移除材料属性
		*/
		void remMaterialProp(const MxFunctions::MaterialClass& propname);

		/*
		* 保存材料属性
		*/
		void storeMaterialProp();

		/*
		* 材料库添加材料
		*/
		void addMaterialFromLib(const QString& name, const QHash<MxFunctions::MaterialClass, QHash<QString, QPair<QVector<Unit>, QVariant>>>& props);

		/*
		* 取消编辑窗口(包括设置空窗口，取消编辑子窗口)
		*/
        void unEditInfoWindow(bool clearPicked);//无参调用走IPanelHost::unEditInfoWindow()（默认true）

        /*
        * 树选中链路的去重配合：slot_itemSelectionChanged 已做完整退出清理后，
        * 本次点击内 show*Manager/show*Reference 会再次调用 unEditInfoWindow，
        * 先 suppress 使该次重复调用短路，避免 clear/渲染器删除/表格复位双份执行；
        * 链路结束后须 clear 复位（未消费也复位，避免误伤下一次真实调用）
        */
        void suppressUneditOnce() { _uneditSuppressed = true; }
        void clearUneditSuppression() { _uneditSuppressed = false; }

        /*
        * IPanelHost 接口实现：PostWindow::mPostInfoHub 经 MechCommon::panelHostRef() 调用
        */
        virtual void unEditInfoWindow() override { unEditInfoWindow(true); }
        virtual void setMainWidget(QWidget* w) override;
        virtual QWidget* mainWidget() const override { return currentMainWidget(); }

        /*
        * 参数栏当前主部件（无内容返回 nullptr）：图表卡片激活补齐路径据此判断
        * 参数栏是否已显示目标曲线属性页，避免无谓的卸载/重挂（会打断控件编辑态）
        */
        QWidget* currentMainWidget() const;

		/***************************************************************************************连接模块***********************************************************************************/
		/*
		* 搜素连接
		*/
		void searchConnector();
		
		/*
		* 取消连接搜索
		*/
		void unSearchConnector();

		/*
		* 创建连接
		*/
		void createConnector(MxFunctions::ConnectorType type);

		/*
		* 打开「多体载荷导入」面板（主动唤出，path 可预填 coupling 文件）
		*/
		void openMotionLoadImport(const QString& path = QString());

		/*
		* 编辑连接
		*/
		void editConnector(int connId);

		/*
		* 取消编辑连接
		*/
		void unEditConnector();

		/*
		* 渲染连接的临时节点
		*/
		void rendConnFreeNodes();

		/*
		* 取消连接的临时节点渲染
		*/
		void disRendConnFreeNodes();

		/***************************************************************************************连接副模块***********************************************************************************/
		/*
		* 创建连接副
		*/
		void createDeputy(MxFunctions::DeputyType type);

		/*
		* 编辑连接副
		*/
		void editDeputy(int deputyId);

		/*
		* 取消编辑连接副
		*/
		void unEditDeputy();

        /********************************************模型修改*****************************************************************************************************************************/
        //创建单元生死
        void createElementSetCtrl(int caseId = -1);
        //编辑单元生死
        void editModelFix(int modelFixId);
        //退出编辑单元生死：失活拾取控件并提交数据
        void uneditEleSetCtrl();

		/***************************************************************************************接触模块***********************************************************************************/
		/*
		* 创建接触
		*/
		void createContact(MxFunctions::ContactType type);

		/*
		* 编辑接触
		*/
		void editContact(int contactId);

		/*
		* 取消编辑接触
		*/
		void unEditContact();

		/*
		* 编辑接触对
		*/
		void editContactPairPanel();

		/*
		* 取消编辑接触对
		*/
		void unEditContactPairPanel();

		/***************************************************************************************周期性边界模块***********************************************************************************/
		/*
		* 创建周期性边界
		*/
		void createPeriodicBoundary(MxFunctions::PeriodicBoundaryType type);

		/*
		* 编辑周期性边界
		*/
		void editPeriodicBoundary(int pbId);

		/*
		* 取消编辑周期性边界
		*/
		void unEditPeriodicBoundary();

		/***************************************************************************************几何模块***********************************************************************************/
		
		/*
		* 新建信息窗口
		*/
		void createGeoInfo(const BrowserTreeType type, const std::vector<int> ids);

		/*
		* 更新信息窗口
		*/
		void updateGeoInfo(const std::vector<int> ids);

        /*
        * 取消编辑信息窗口
        */
		void uneditGeoInfo();

		
		/*
		* 新建特征窗口
		*/
		void createGeoFeature(const GeoOperationType type);

		/*
		* 新建变换窗口
		*/
		void createGeoTransform(const GeoOperationType type);

		/*
		* 新建布尔窗口
		*/
		void createGeoBool(const GeoOperationType type);

		/*
		* 新建编辑窗口
		*/
		void createGeoEdit(const GeoOperationType type);

        /*
        * 新建修复窗口
        */
        void createGeoRepair(const GeoOperationType type);

        /*
        * 新建共享拓扑
        */
        void createGeoSharedTopology(const GeoOperationType type);

        /*
        * 新建几何实体化
        */
        void createGeoSolidify(const GeoOperationType type);

		/*
		* 新建消除窗口
		*/
		void createGeoRemoval(const GeoOperationType type);

		/*
		* 几何-直接建模
		*/
		void createGeoDirectModeling(const GeoOperationType type);

		/*
		* 退出直接建模会话（ESC 清拾取等外部入口；面板未创建时为空操作）
		*/
		void exitGeoDirectModeling();

		/*
		* 新建提取窗口
		*/
		void createGeoExtract(const GeoOperationType type);
		/*
		* 取消几何窗口编辑
		*/
		void uneditGeometry();

		/***************************************************************************************网格模块***********************************************************************************/
		/*
		* 创建全局尺寸节点
		*/
		void createGlobalDimensions();


		/*
		* 编辑网格:全局尺寸
		*/
		void editGlobalDim(int meshId);


		/*
		* 编辑网格:体、面、线网格、、、局部尺寸
		*/
		void editMeshPanel(int meshId);

        /*
        * 编辑网格操作
        */
        void editOptMeshPanel(int meshId);

		/*
		* 创建多选网格子节点面板
		*/
		void createMeshPanelMultiSelect(const std::vector<int> ids);

		/*
		* 创建网格节点
		*/
		void createMeshPanel(int type);

        /*
        * 创建网格操作节点
        */
		void createMeshOptPanel(int type);

		/*
		* 根据partId生成网格
		*/
		void createMeshByPartId(std::vector<int> partIds);

		/*
		* 一键生成网格
		*/
		void createMeshAuto();

		/*
		* 一键删除网格
		*/
		void deleteMeshAuto();

		/*
		* 根据网格设置生成
		*/
		void createMeshBySetting();

		/*
		* 生成网格
		*/
		void createMesh(int id);

		/*
		* 生成网格全局尺寸
		*/
		void createMesh1();

		/*
		* 生成网格自适应剖分
		*/
		void createMeshAutoAdapt();

		/*
		* 生成网格自定义加密
		*/
		void createMeshCustomEncryption();

        /*
        * 生成网格局部尺寸
        */
  void createMeshLocalSize(int id);
  /*
  * 执行边抑制/取消抑制
  */
  void createMeshEdgeSuppress(int id, bool isSuppress);

  /*
  * 复制作用对象
  */
  void copyMeshChild();

		/*
		* 获取当前网格面板
		*/
		mMeshPanel* getMeshPanel() { return _meshPanel.get(); }

		//删除包围盒
		void boxDelete();

		//隐藏包围盒
		void boxHide();

		//预览包围盒
		void boxPreview();

        //局部尺寸预览
		void localSizePreview();

		//显示包围盒
		void boxShow();

		/*
		* 取消网格窗口编辑
		*/
		void unEditMeshPanel();

        /***********************************************************************频率点采样FREQ*******************************************************************************/
        void creatFREQ();
        void editFREQ(int freqId);

        /***********************************************************************层合结构*******************************************************************************/
         /*
        * 创建
        */
        void creatLaminate(LaminateType type);

        /*
        * 编辑
        */
        void editLaminate(int laminateId);

		/*************************************************************************工作目录***********************************************************************************/
		void createWorkingDir();

		/*************************************************************************单位制***********************************************************************************/
		/*
		* 更新所有面板单位
		*/
		void updateAllPanelUnits();

		QHash<int, int> BinaryToDecima = 
		{
			{14,0},{8,1},{4,2},{2,3},{1,4}, { 32,5 },{64,6},{128,7},{256,8},{528,9}
		};
		int _pickFilter=0;
		QList<QAction*>* _actions;
		QToolButton* createBtn;
		QToolButton* deleteBtn;
		QMenu* meshMenu;
		bool isMeshing=false;
	private:
		/*
		* 默认构造函数
		*/
		mInfoWindow(QWidget* parent = 0);

		// 树选中链路内 unEditInfoWindow 重复调用的一次性短路标志
		//（见 suppressUneditOnce/clearUneditSuppression 注释）
		bool _uneditSuppressed = false;

		void hideAll();

		/*
		* 面板懒加载：首次使用时才创建，避免启动阶段集中构造全部面板拖慢启动速度。
		* 使用方式：ensurePanel(_xxxPanel)->method(); 或 ensurePanel(_xxxPanel).get()
		*
		* 所有权归 Qt 父对象：setMainWidget 会把面板 setParent 到本容器，关窗时 Qt
		* 按对象指针 delete 面板。若用 make_shared，对象嵌在控制块内（偏移16字节），
		* Qt 释放的是块内偏移指针 → 调试堆断言崩溃，故必须裸 new 配空删除器。
		*/
		template<typename PanelT>
		std::shared_ptr<PanelT>& ensurePanel(std::shared_ptr<PanelT>& panel)
		{
			if (!panel)
				panel = std::shared_ptr<PanelT>(new PanelT, [](PanelT*) {});
			return panel;
		}


	private slots:
		//切换语言
		void slot_changeLanguage(int language);

		/*
		* 跨模型/视口切换前收尾（此时 getCVM 仍指向旧模型）：
		* 参数停靠栏面板均为全局共享单例，编辑态若不在旧模型下正常退出，
		* unEdit 链路按"当前模型"管理器复核数据会落空，折叠框残留布局，
		* 之后另一模型进入编辑即出现两个模型的面板同时显示
		*/
		void slot_viewportModelAboutToChange(int newViewportID);
	private:
		//模型树
		mModelTreeManager* _modelTreeManager;
		//单例指针
		static mInfoWindow* _instance;

		//懒加载缓存：启动时注入的菜单栏对象，管理器面板创建时再透传
		mMenuWidget* _menuWidget = nullptr;

		QWidget* m_info;

		//QScrollArea* _scrollArea = nullptr;

		// 垂直主布局
		QVBoxLayout* _mainLayout = nullptr;

		//
		QVBoxLayout* mainLay = nullptr;


		//分析表
		std::shared_ptr<mCasePanel> _casePanel;

		//分析设置表
		QList<std::shared_ptr<mCaseSettingPanel>> _caseSettingPanels;

		//属性表
		std::shared_ptr<mPropertyPanel> _propertyPanel;

        //非结构质量表
        std::shared_ptr<mNonStructuralQualityPanel> _mNonStructuralQualityPanel;

        //离散场
        std::shared_ptr<mDiscreteFieldPanel> _mDiscreteFieldPanel;

        //分析场
        std::shared_ptr<mAnalysisFieldPanel> _mAnalysisFieldPanel;

        //非结构质量管理（工况下）
        std::shared_ptr<mNsmManagerUnderCasePanel> _mNsmManagerUnderCasePanel;

        //模型修改管理（工况下）
        std::shared_ptr<mModelFixManaUnderCasePanel> _mModelFixManaUnderCasePanel;

		//单元表
		std::shared_ptr<mElementPanel> _elementPanel;

		//属性检查表
		std::shared_ptr<mPropertyCheckPanel> _propertyCheckPanel;

		//集合管理器表
		std::shared_ptr<mSetManagerPanel> _setManagerPanel;

		//集合表
		std::shared_ptr<mSetPanel> _setPanel;

        //结构优化表
        std::shared_ptr<mStructuralOptimizationPanel> _mStructuralOptimization;
        std::shared_ptr<mOptimizationGoalPanel> _optimizationGoalPanel;
        std::shared_ptr<mOptimizationDesignPanel> _optimizationDesignPanel;
        std::shared_ptr<mOptimizationConstraintPanel> _optimizationConstraintPanel;

        //流道优化表
        std::shared_ptr<mRunnerOptimizationPanel> _mRunnerOptimization;
        std::shared_ptr<mRunnerOptimizationGoalPanel> _runnerOptimizationGoalPanel;
        std::shared_ptr<mRunnerOptimizationDesignPanel> _runnerOptimizationDesignPanel;
        std::shared_ptr<mRunnerOptimizationConstraintPanel> _runnerOptimizationConstraintPanel;

        //超单元参数
        std::shared_ptr<mGenericParamPanel> _mGenericParamPanel;
        //超单元调用
        std::shared_ptr<mGenericAssignPanel> _mGenericAssignPanel;
        //超单元矩阵
        std::shared_ptr<mGenericMatrixPanel> _mGenericMatrixPanel;
        //超单元部件
        std::shared_ptr<mGenericPartPanel> _mGenericPartPanel;
        //求解控制
        std::shared_ptr<mSolveControlPanel> _mSolveControlPanel;
        //求解任务
        std::shared_ptr<mSolveTaskPanel> _mSolveTaskPanel;
        //全局输出控制
        std::shared_ptr<mGlobalOutputControlPanel> _mGlobalOutputControlPanel;

        //声固耦合对搜索
        std::shared_ptr<mASCouplingSearchPanel> _mASCouplingSearchPanel;

        //模态贡献量
        std::shared_ptr<mModalContriPanel> _mModalContriPanel;

        //节点贡献量
        std::shared_ptr<mNodeContriPanel> _mNodeContriPanel;

        //面板贡献量
        std::shared_ptr<mPanelContriPanel> _mPanelContriPanel;

        //传递路径贡献量
        std::shared_ptr<mTPContriPanel> _mTPContriPanel;

        //等效辐射功率
        std::shared_ptr<mERPPanel> _mERPPanel;

        //峰值筛选
        std::shared_ptr<mPeakFilterPanel> _mPeakFilterPanel;

        //工况下峰值筛选
        std::shared_ptr<mCasePeakFilterPanel> _mCasePeakFilterPanel;

        //坐标系表
		std::shared_ptr<mCoordinatePanel> _coordinatePanel;
        //新建结构优化
        std::shared_ptr<mStructual> _myPanel;

		//节点坐标系表
		std::shared_ptr<mNodeCoordinatePanel> _nodeCoordinatePanel;

		//函数（曲线）表
		std::shared_ptr<mFunctionPanel> _FunctionPanel;

		//截面形状表
		std::shared_ptr<mSectionShapePanel> _SectionShapePanel;

		//多体动力学面板（懒加载）
		std::shared_ptr<mMotionBodyPanel> _motionBodyPanel;
		std::shared_ptr<mMotionBodyBatchPanel> _motionBodyBatchPanel;
		std::shared_ptr<mMotionJointPanel> _motionJointPanel;
		std::shared_ptr<mMotionCouplingPanel> _motionCouplingPanel;
		std::shared_ptr<mMotionDrivePanel> _motionDrivePanel;
		std::shared_ptr<mMotionScenarioPanel> _motionScenarioPanel;
		std::shared_ptr<mMotionAnimationPanel> _motionAnimationPanel;
		std::shared_ptr<mMotionLoadExportPanel> _motionLoadExportPanel;
		std::shared_ptr<mMotionMarkerPanel> _motionMarkerPanel;
		std::shared_ptr<mMotionVectorForcePanel> _motionVectorForcePanel;
		std::shared_ptr<mMotionContactPanel> _motionContactPanel;
		std::shared_ptr<mMotionSpringPanel> _motionSpringPanel;
		std::shared_ptr<mMotionCurvePanel> _motionCurvePanel;
		std::shared_ptr<mMotionControlInputPanel> _motionControlInputPanel;
		std::shared_ptr<mMotionElectroImportPanel> _motionElectroImportPanel;
		std::shared_ptr<mMotionElectroExportPanel> _motionElectroExportPanel;
		std::shared_ptr<mMotionHydraulicPanel> _motionHydraulicPanel;
		std::shared_ptr<mMotionRLPanel> _motionRLPanel;
		std::shared_ptr<mMotionSensorPanel> _motionSensorPanel;
		std::shared_ptr<mMotionControllerPanel> _motionControllerPanel;
		std::shared_ptr<mMotionAutoFlexPanel> _motionAutoFlexPanel;
		std::shared_ptr<mMotionMechanismPanel> _motionMechanismPanel;
		std::shared_ptr<mMotionTransmissionPanel> _motionTransmissionPanel;
		std::shared_ptr<mMotionMechElecHydPanel> _motionMechElecHydPanel;

		//三维结果表

		//材料表
		std::shared_ptr<mMaterialPanel> _materialPanel;

		//连接表
		std::shared_ptr<mConnectorPannel> _connectorPannel;

		//多体载荷导入面板
		std::shared_ptr<mMotionLoadImportPanel> _motionLoadImportPanel;

		//连接副表
		std::shared_ptr<mDeputyPannel> _deputyPannel;

		//连接搜索表
		std::shared_ptr<mConnectorSearchPannel> _connectorSearchPannel;

		//接触表
		std::shared_ptr<mContactPanel> _contactPanel;

        //单元生死表
        std::shared_ptr<mEleSetCtrlPanel> _elementSetCtrlPanel;

		//接触对表
		std::shared_ptr<mContactPairPanel> _contactPairPanel;

		//周期性边界表
		std::shared_ptr<mPeriodicBoundaryPanel> _mPeriodicBoundaryPanel;

		//工况表
		std::shared_ptr<mWorkingConditionPanel> _workingConditionPanel;

		//网格 全局尺寸
		std::shared_ptr<mGlobalDimPanel> _globalDimPanel;

		//网格 体、面、线
		std::shared_ptr<mMeshPanel> _meshPanel;
		std::shared_ptr<mSolidMeshPanel> _mSolidMeshPanel;
		std::shared_ptr<mFaceMeshPanel> _mFaceMeshPanel;
		std::shared_ptr<mLineMeshPanel> _mLineMeshPanel;
		std::shared_ptr<mEdgeSuppressPanel> _mEdgeSuppressPanel;
		std::shared_ptr<mAutoAdaptPanel> _mAutoAdaptPanel;
		std::shared_ptr<mCustomEncryptionPanel> _mCustomEncryptionPanel;
		std::shared_ptr<mLocalSizePanel> _mLocalSizePanel;
		std::shared_ptr<mCreatePointPanel> _mCreatePointPanel;
		std::shared_ptr<mMeshEditPanel> _mMeshEditPanel;
		std::shared_ptr<mCreateMeshPanel> _mCreateMeshPanel;
		std::shared_ptr<mCreateMetaMeshPanel> _mCreateMetaMeshPanel;
		std::shared_ptr<mExtractSurfacePanel> _mExtractSurfacePanel;
		std::shared_ptr<mRotateMeshPanel> _mRotateMeshPanel;
		std::shared_ptr<mTranslationPanel> _mTranslationPanel;
		std::shared_ptr<mRotationPanel> _mRotationPanel;
		std::shared_ptr<mStretchPanel> _mStretchPanel;
		std::shared_ptr<mZoomPanel> _mZoomPanel;
		std::shared_ptr<mMovePointPanel> _mMovePointPanel;
		std::shared_ptr<mChangeOrderPanel> _mChangeOrderPanel;
		std::shared_ptr<mSharedNodePanel> _mSharedNodePanel;
		std::shared_ptr<mFreeEdgePanel> _mFreeEdgePanel;
		std::shared_ptr<mTEdgePanel> _mTEdgePanel;
		std::shared_ptr<mMeshIntersectPanel> _mMeshIntersectPanel;
		std::shared_ptr<mMeshOverlapPanel> _mMeshOverlapPanel;
		std::shared_ptr<mMeshInterferePanel> _mMeshInterferePanel;
		std::shared_ptr<mMeshStatisticsPanel> _mMeshStatisticsPanel;
		std::shared_ptr<mQualityPanel> _mQualityPanel;
		std::shared_ptr<mMirroringPanel> _mMirroringPanel;
		std::shared_ptr<mMatrixPanel> _mMatrixPanel;
		std::shared_ptr<mSmoothPanel> _mSmoothPanel;
		std::shared_ptr<mRemeshingPanel> _mRemeshingPanel;
		std::shared_ptr<mSTLRemeshPanel> _mSTLRemeshPanel;
		std::shared_ptr<mRenumberPanel> _mRenumberPanel;
		std::shared_ptr<mMeshNormalPanel> _mMeshNormalPanel;
		std::shared_ptr<mMoveMeshPanel> _mMoveMeshPanel;


		//特征模块表
		std::shared_ptr<mGeoFeaturePanel> _geoFeaturePanel;

		//变换模块表
		std::shared_ptr<mGeoTransformPanel> _geoTransformPanel;

		//布尔运算模块表
		std::shared_ptr<mGeoBoolOperationPanel> _geoBoolPanel;

		//几何编辑
		std::shared_ptr<mGeoEditPanel> _geoEditPanel;

        //几何编辑
        std::shared_ptr<mGeoRepairPanel> _geoRepairPanel;

        //共享拓扑
        std::shared_ptr<mGeoSharedTopologyPanel> _geoSharedTopologyPanel;

        //几何实体化
        std::shared_ptr<mGeoSolidifyPanel> _geoSolidifyPanel;

		//几何消除
		std::shared_ptr<mGeoRemovalPanel> _geoRemovalPanel;

		//直接建模
		std::shared_ptr<mGeoDirectModelingPanel> _geoDirectModelingPanel;

		//几何提取
		std::shared_ptr<mGeoExtractPanel> _geoExtractPanel;

		//信息模块表
		std::shared_ptr<mGeoInfoPanel> _geoInfoPanel;

		//动画表

		//生成动画表

		//动画自定义截图

		//动画自定义视频

		//云图高级查询 

		//工作目录
		std::shared_ptr<mWorkingDirPanel> _WorkingDirPanel;

		//后处理线性叠加

		//后处理标量运算

		//后处理切面图

        //后处理等值面图

        //后处理矢量图

        //后处理流线图

        //后处理集合

        //后处理标注

        //后处理对比视频
        std::shared_ptr<mPostCompVideoPanel>     _postCompVideoPanel;

        //后处理爆炸视图

        //后处理测量工具

        //后处理路径结果显示

		//后处理路径

		//后处理二维线

        //后处理箭头

        //后处理文本

        //后处理log

        //后处理模态贡献量

        //后处理面板贡献量

        //后处理传递路径贡献量

	public:
        //频率点采样方式->FREQ
        std::shared_ptr<mFreqSamplPointPanel> _FREQPanel;

        //层合结构
        std::shared_ptr<mLaminatePanel> _LaminatePanel;

		//载荷管理器表（模板实例，Traits决定数据通路；不透明指针避免头文件包含模板定义）
		std::shared_ptr<void> _loadManagerPanel;

		//载荷引用面板（含缩放系数列）
		std::shared_ptr<void> _loadReferencePanel;

		//边界管理器表
		std::shared_ptr<void> _boundaryManagerPanel;

		//边界引用面板（含缩放系数列）
		std::shared_ptr<void> _boundaryReferencePanel;

		//预定义场管理器表
		std::shared_ptr<void> _predefinedFieldManagerPanel;

		//预定义场引用面板（无缩放系数列）
		std::shared_ptr<void> _predefinedFieldReferencePanel;

		//接触管理器表
		std::shared_ptr<void> _contactManagerPanel;

		//接触引用面板（无缩放系数列，过盈接触外强制挂载）
		std::shared_ptr<void> _contactReferencePanel;

		//模型修改管理器表
		std::shared_ptr<void> _modelFixManagerPanel;

		//模型修改引用面板（无缩放系数列，有序table映射）
		std::shared_ptr<void> _modelFixReferencePanel;

 public:
		QHash<MeshType, std::vector<int>> _eleTypeToeleId;
	};

	/*****************************************************请弃用*********************************************************************/
	/* ---------- 单个折叠表格控件 ---------- */
	class FoldTable : public QWidget
	{
		Q_OBJECT
	public:
		explicit FoldTable(const QString &title, QWidget *parent = nullptr);
		void addRow(const QString &name, const QString &value);
		void addRow(QWidget* widget);
		void updateTableHeight();

		// 标题按钮悬停/选中色随主题重发（原硬编码浅色，深色/护眼下不跟随）；
		// 构造与切主题共用同一条路径（mInfoWindow 的主题补发链统一调用）
		void applyTheme();

	public slots:
		void onFoldClicked(bool checked);
		QIcon getFoldIcon(bool open);   // 根据折叠状态返回箭头图标

	private:
		QPushButton *m_btn;
		QTableWidget *m_table;
	};
}
