#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <memory>
#include <QWidget>
#include <QObject>

#include "mpostwindow_global.h"

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { class mFramelessDialog; }
using namespace MechMainWindow;


namespace PostWindow
{
	class mPostCurvePanel;
	class mPostExportWordPanel;
	class mPostParrayMimagePanel;
	class mPostCompVideoPanel;
	class mPostResultPath;
	class mPostPanelParticipation;
	class mPostTransferPathAnalysis;

	class mPost3DResultPanel;
	class mPostAnimationPanel;
	class mPostGenerateAnimationPanel;
	class mPostCustomScreenshotPanel;
	class mPostCustomVideoPanel;
class mPostAdvancedQueryPanel;
	class mPostCurveExtractPanel;
	class mPostQueryPanel;
	class mPostSectionCutPanel;
	class mPostStepSequencePanel;
	class mPostEnvelopePanel;
	class mPostVectorPlotPanel;
class mPostLinearSuperpositionPanel;
	class mPostScalarOperationPanel;
	class mPostIsosurfacePanel;
	class mPostDeformationPanel;
	class mPostSectionPanel;
	class mPostVectorPanel;
	class mPostStreamLinePanel;
	class mPostSetPanel;
	class mPostNotesPanel;
	class mPostExplodePanel;
	class mPostMeasureTool;
	class mPostTagPanel;
	class mPostSectionForce;
	class mPostCoordinatePanel;
	class mPostPathPanel;
	class mPostLinePanel;
	class mPostArrowPanel;
	class mPostTextPanel;
	class mPostLogPanel;

	/*
	* 后处理信息面板集线器（自 mInfoWindow 增量迁出，居 PostWindow）
	*
	* mInfoWindow 是 Info dock 的容器与前处理参数面板调度者，其中寄生的后处理
	* 面板调度（面板懒创建+显示+初始化）逐步迁入本类——面板类迁入PostWindow后
	* mInfoWindow（留居MainWindow）不得再include其头（否则MainWindow->PostWindow
	* 反向include成循环链接）。
	*
	* 迁移地图（源 mInfoWindow.cpp，按族渐进迁入）：
	*   第1批  结果创建族/编辑族/路径贡献量族（createCurve/createExportWord/
	*          createParrayMimage/createCompVideo/editCurve/editExportWord/
	*          createResultPath/三个贡献量/updateDisplayCurve/showTableData）
	*   第2批  动画族/截屏视频族/3D结果过滤族/查询族/图表族(切面/等值面/矢量/
	*          流线/集合/二维线/箭头/文本/Log/路径)/注释族/爆炸/测量/叠加/标量
	*          （55函数+21面板成员+菜单信号connect自mInfoWindow构造函数迁入；
	*          mInfoWindow至此不再持有任何后处理面板成员与调度函数）
	*
	* 接收端约定：后处理菜单页(mPostMenuContributor)与MainWindow侧入口
	* （主页截屏按钮/主窗resize/前处理树的注释·3D结果·快照入口）经
	* mMechGlobalSignals 全局信号总线广播，由本类构造函数统一接线。
	*/
	class MPOSTWINDOW_EXPORT mPostInfoHub : public QObject
	{
		Q_OBJECT

	public:
		static mPostInfoHub* getInstance();

		/*
		* "结果"页创建族：激活Info dock、显示对应后处理面板并按工况初始化
		* （自 mInfoWindow::createXXX 迁入，行为原样）
		*/
		void createCurve(int caseid);
		void createExportWord(int caseid);
		void createParrayMimage(int caseid);
		void createCompVideo(int caseid);

	/*
	* 编辑族（树双击已有节点）：显示对应面板并按视图名刷新
	* （自 mInfoWindow::editCurve/editExportWord 迁入，调用方 mPostBrowserWindow）
	*/
	void editCurve(const QString& viewname);
	void editExportWord(const QString& viewname);

	/*
	* 重挂已有对比视频面板（不重置状态，无树节点/视图名，区别于 createCompVideo
	* 的新建语义）：叠加到视口勾选后云图激活联动把 Info 挂成云图面板，由此拉回
	* （newCompVideoPanel 现为空实现，但 createXxx 语义上属新建入口，将来补实现
	* 重置逻辑时不应被形态切换误触发）
	*/
	void editCompVideo();

		/*
		* 曲线显示刷新/表格数据（clickUpdateCurveDisplaySig/clickShowTableDataSig
		* 接收端转发目标，自 mInfoWindow 同名方法迁入）
		*/
		void updateDisplayCurve();
		void showTableData();

		/*
		* 路径结果/贡献量族（自 mInfoWindow::createResultPath/panelContributionAmount/
		* pathContributionAmount 迁入；模态贡献量旧链下线 openspec §8.1）
		*/
		void createResultPath(int caseid);
		void panelContributionAmount();
		void pathContributionAmount();

		/*
		* 模态贡献量分析参数面板（曲线页诊断组入口，embed-nvh-toolset-into-info-panel）：
		* NVH工具集面板三页签原样挂入参数栏——面板为自管理单例（getInstance 堆常驻），
		* 不经 ensurePanel，mountToHost 后宿主接管父子关系，切走再切回状态保留
		*/
		void createModalContribution();

		//-------------- 第2批（自 mInfoWindow 迁入） --------------

		/*
		* 3D结果族：创建/导入/更新/编辑 + 云图过滤/显隐值/导出STL/切最大帧
		*/
		void create3DResult(bool isRead, int caseid = -1);
		bool import3DResult(QStringList paths, int caseid = -1,
			const QString& modelFilePath = QString(),   //modelFilePath：op2显式伴随网格（透传给原生读取，空=自动探测）
			bool notifyScanHint = true);                //新版格式大文件扫描提示，仅用户手动导入发（会话恢复传 false）
		void update3DResult(int caseid, QStringList viewports);
		//把结果数据批量应用到一组云图视口（会话恢复用：逐视口 clear+绑定结果库+
		//面板级联应用；与导入收尾链的单视口应用互补，保证恢复的全部云图视口均有模型）
		void applyResultToViewports(const QStringList& viewports, const QString& postname);
		void edit3DResult(QString viewname);
		void slot_startFilter(QString min, QString max, QString type);
		void slot_clearFilter();
		void slot_exportFilterSTL(const QString& fileName);
		void slot_showValue();
		void slot_hideValue();
		void slot_hideAll();
		void slot_switchToMaxFrame();

		/*
		* 云图结果面板访问（懒创建）：等值面面板接管/归还"云图变量"等共享折叠组的宿主
		*/
		std::shared_ptr<mPost3DResultPanel> cloudResultPanel();

		/*
		* 注释/爆炸/测量/截面力
		*/
		void createNote(int caseid = -1);
		void autoApplyNotes(int caseid = -1);

		/*
		* 信息标签配置弹窗：共享注释面板临时顶层窗口化（不切 Info dock、不 unEdit 当前面板）；
		* 关闭仅隐藏不销毁，之后 setMainWidget 会按 Qt reparent 语义自动归位 dock
		*/
		void openNotesAsDialog(int caseid = -1);
		void createExplode(int caseid = -1);
	void createMeasureTool(int caseid = -1);

	/*
	* 树"测量"分支联动出口（V1.3 原型测量树入口）：
	* editMeasure=树子节点点击（打开面板并定位记录进编辑，统一编辑契约三选一
	* 确认内建——dirty 被"继续编辑"拦截时面板编辑对象不变）；
	* existingMeasureToolPanel=面板已创建则返回不懒创建（未开过返回 nullptr，
	* 树分支重建后的高亮查询用）
	*/
	void editMeasure(int recordId);
	mPostMeasureTool* existingMeasureToolPanel() const;
	void createTagPanel(int caseid = -1);

	/*
	* 树"标签"分支联动出口（结果标签面板会话记录的树侧只读通道）：
	* tagTreeRecords=记录快照（分支重建源；面板未创建返回空）；
	* selectTagRecord=树子节点点击路由（面板已由调用方先行打开，此处仅选中记录）
	*/
	QVector<QPair<int, QString>> tagTreeRecords() const;
	void selectTagRecord(int recordId);
	void createSectionForce(int caseid = -1);

		/*
		* 坐标系（模型页 直角/柱/球 三入口，clickPostCoordinateCreateSig 经树侧槽路由）：
		* createCoordinate=按类型新建（数据入 MechCoordinateData 管理器+面板预设+树分支重建）；
		* editCoordinate=树"坐标系"子节点单击/右键"编辑"路由（面板回填该坐标系进编辑）
		*/
		void createCoordinate(int coordType);
		void editCoordinate(int coordinateId);

		/*
		* 树"坐标系"分支联动出口：existingCoordinatePanel=面板已创建则返回不懒创建
		* （重命名同步查询用）；notifyCoordinateRenamed=树侧内联重命名后面板名称行跟随
		* （面板正编辑该坐标系时同步，避免下次提交回写旧名）；其余为面板会话记录的
		* 树侧操作通道（标签 tagTreeRecords 同款模式，树不直接改数据）
		*/
		mPostCoordinatePanel* existingCoordinatePanel() const;
		void notifyCoordinateRenamed(int coordinateId, const QString& newName);
		QVector<QPair<int, QString>> coordinateTreeRecords() const;
		bool deleteCoordinateRecords(const QList<int>& ids);
		int copyCoordinateRecord(int id);
		void clearCoordinateRecords();
		void renameCoordinateRecord(int coordinateId, const QString& newName);

		/*
		* 线性叠加/标量运算
		*/
		void createLinearSuperposition(int caseid);
		void createScalarOperation(int caseid);
		void editLinear(QString viewname);

		/*
		* 派生工况定义回编（树"派生工况"分支单击/右键"编辑"入口，批次A0）：
		* 按当前模型实例的 store 定义 kind 路由到对应面板装载编辑。
		*/
		void editDerivedCase(int definitionId);
		void editScalarOperation(QString viewname);

		/*
		* 动画族（云图页/动画页菜单信号接收端）
		*/
		void createAnimation(int type, int caseid);
		void editAnimation(QString viewname);
		//删除指定视口上的动画（FR-01/FR-05 单视口形态：树侧删除入口转发；
		//stop 回退停止时帧，视口不关闭）
		void deleteAnimation(QString viewname);
		void slot_firstFrame();
		void slot_lastFrame();
		void slot_PlayAnimation(bool isrun);
		void slot_nextFrame();
		void slot_endFrame();
		void slot_setRenderRange(const QString& type);
		void slot_setHideFailedElement(bool checked);
		void slot_setLoopPlay(bool istrue);
		void slot_setFrameRate(int fr);
		void slot_setStatus(bool isEnable);
		void slot_generateAnimation(QString ani, QString var, QString start, QString end);
		void slot_updateGenerateAnimationGUI(QString ani, QString var, QString start, QString end);
		int slot_aniCurFrame();
		int slot_aniStartFrame();
		int slot_aniEndFrame();

		/*
		* 截屏/视频族
		*/
		void createCustomScreenshot();
		void createCustomVideo();
		void slot_aniQuickScreenshot();
		void aniQuickScreenshot(QString path);
		void slot_aniExportVideo();
		void slot_videoFrameRange(int min, int max);
		void slot_videoEndFrame(int val);
		void slot_rePainterRecordWindow();

		/*
		* 录制×实时动画互斥（实时动画 v1.1）
		* - notifyRecordingStarted：录制发起广播。录制面板在引擎链
		*   RecordAnimation/RecordScreen 调用之前发射（同步直连）——实时动画面板收到后
		*   按 v1 停止语义停掉播放中的浮层，画面与逐帧控制权让给录制走查；
		* - isRecordingActive：录制进行中权威查询。聚合录制面板 _isStart 簿记
		*   （引擎链发起成功处置位、signal_EndAnimation/CaptureFinished 回执复位）。
		*   判定点说明：引擎侧 ZsEngine_AnimationWindow::isRecording() 为 protected
		*   不可达，GUI 侧录制簿记为唯一可及权威态（离线录制引擎链与 v1 走帧共用
		*   同一门面 seekFrame，录制期间 playbackState 保持 Stopped，须独立判定）
		*/
		void notifyRecordingStarted();
		bool isRecordingActive() const;

		/*
		* 高级查询
		*/
		void createAdvancedQuery();

		/*
		* 曲线提取参数面板（结果分析组入口）
		*/
		void createCurveExtract();

		/*
		* 查询参数面板（结果分析组入口，V1.3 查询界面）
		*/
		void createQuery();

		/*
		* 截面剖切参数面板（结果页空间显示组入口）
		*/
		void createSectionCut();

		/*
		* 步序列派生工况参数面板（结果页工况组入口，V1.3 派生载荷界面；
		* 每次入口点击新建一条"步序列-N"定义进入编辑——原型 createDerivedCase 语义）
		*/
		void createStepSequence();

		/*
		* 包络派生工况参数面板（结果页工况组入口，V1.3 派生载荷界面；
		* 每次入口点击新建一条"包络-N"定义进入编辑——原型 createDerivedCase 语义）
		*/
		void createEnvelope();

		/*
		* 向量图参数面板（结果页结果图形组入口，V1.3 向量参数界面）
		*/
		void createVectorPlot();

		/*
		* 图表族：切面/等值面/矢量/流线/集合/二维线/箭头/文本/Log/路径
		*/
		void createSection(int caseid);
		void editSection(QString viewname);
		void createIsosurface(int caseid);
		void editIsosurface(QString viewname);

		/*
		* 等值面参数面板入口（无树节点创建：作用于当前激活云图视口，同变形控制；
		* 入口为"结果"页"等值面图"按钮，经总线 clickIsosurfacePanelSig。
		* 等值面树节点创建入口仍走 clickIsosurfaceSig→createIsosurface）
		*/
		void openIsosurfacePanel();

		/*
		* 动画生成完成后恢复等值面（HyperView 语义：等值面随动画逐帧提取）：
		* 动画生成链重建渲染器会销毁源视口等值面，生成完成后按面板存储态立即
		* 重建；面板未创建/该视口未生成过则无操作。动画生成链（一键/自定义）收尾调用
		*/
		void restoreIsosurfaceAfterAnimation(const QString& viewname);

		/*
		* 变形控制（无树节点：作用于当前激活云图视口；入口为 Ribbon"云图"页
		* "变形控制"按钮与云图面板变形系数齿轮，均经总线 clickDeformationSig）
		*/
		void createDeformation();
		void editDeformation(QString viewname);
		void createVector(int caseid);
		void editVector(QString viewname);
		bool haveVector(QString cloudView);
		void createStreamLine(int caseid);
		void editStreamLine(QString viewname);
		bool haveStreamLine(QString cloudView);
		void createSet(int caseid, int panelType = -1);
		void editSet(QString viewname);
		//树单击集合子节点→面板编辑该集合（库内自动判型）
		void editSetByName(const QString& name);
		//树重命名/删除集合→面板同步（改名：编辑中集合跟随新名；删除：编辑中集合同型重开新集；
		//面板未创建时跳过——通知不触发懒加载挂载）
		void notifySetRenamed(const QString& oldName, const QString& newName);
		void notifySetsDeleted(const QStringList& names);
		void createLine(int caseid);
		void editLine(QString viewname);
		void createArrow(int caseid);
		void editArrow(QString viewname);
		void createText(int caseid);
		void editText(QString viewname);
		void createLog(int caseid);
		void editLog(QString viewname);
		void createPath(int caseid);
		void editPath(QString viewname);
		void uneditPath();

	private:
		/*
		* 面板挂载统一入口：setMainWidget 前套用窄参数栏通用适配（全部面板生效）。
		* Info dock 最小宽 300 且忽略内容最小宽（mDockLayoutManager 注册配置），
		* 行控件最小宽超出字段列时整行被折叠块右缘裁掉——云图面板四轮实证
		* （2026-10-26，详见 docs/云图参数面板-图例和标签窄栏裁切修复记录.md）。
		* 适配只降不升：下限取 min(规范值, 控件当前有效最小值)，紧凑面板零扰动；
		* 不改 sizeHint/拉伸，宽栏外观不变。幂等，可重复挂载。
		*/
		void mountToHost(QWidget* panel);

	signals:
		//录制发起广播（notifyRecordingStarted 转发；实时动画面板构造期接线，
		//接收端按 v1 停止语义停播放中的浮层——录制×实时互斥正向）
		void recordingStarted();

	private:
		mPostInfoHub(QObject* parent = nullptr);
		~mPostInfoHub() = default;
		mPostInfoHub(const mPostInfoHub&) = delete;
		mPostInfoHub& operator=(const mPostInfoHub&) = delete;

		/*
		* 菜单/主窗信号接线（自 mInfoWindow 构造函数迁入）
		*/
		void connectMenuSignals();

		/*
		* 面板懒加载（与mInfoWindow::ensurePanel同模式：首次使用时才创建）
		*
		* 所有权归 Qt 父对象：setMainWidget 会把面板 setParent 进 Info dock 容器，
		* 关窗时 Qt 按对象指针 delete 面板。若用 make_shared，对象嵌在控制块内
		* （偏移16字节），Qt 释放的是块内偏移指针 → 调试堆断言崩溃，故必须裸 new
		* 配空删除器（shared_ptr 仅作非占有式引用）。
		*/
		template<typename PanelT>
		std::shared_ptr<PanelT>& ensurePanel(std::shared_ptr<PanelT>& panel)
		{
			if (!panel)
				panel = std::shared_ptr<PanelT>(new PanelT, [](PanelT*) {});
			return panel;
		}

	private:
		std::shared_ptr<mPostCurvePanel> _postCurvePanel;
		std::shared_ptr<mPostExportWordPanel> _postExportWordPanel;
		std::shared_ptr<mPostParrayMimagePanel> _postParrayMimagePanel;
		std::shared_ptr<mPostCompVideoPanel> _postCompVideoPanel;
			std::shared_ptr<mPostResultPath> _postResultPathPanel;
		std::shared_ptr<mPostPanelParticipation> _postPanelContributionAmount;
		std::shared_ptr<mPostTransferPathAnalysis> _postPathContributionAmount;

		std::shared_ptr<mPost3DResultPanel> _3DResultPanel;
		std::shared_ptr<mPostAnimationPanel> _animationPanel;
		std::shared_ptr<mPostGenerateAnimationPanel> _generateAnimationPanel;
		std::shared_ptr<mPostCustomScreenshotPanel> _postCustomScreenshotPanel;
		std::shared_ptr<mPostCustomVideoPanel> _postCustomVideoPanel;
		std::shared_ptr<mPostAdvancedQueryPanel> _postAdvancedQueryPanel;
		std::shared_ptr<mPostCurveExtractPanel> _postCurveExtractPanel;
		std::shared_ptr<mPostQueryPanel> _postQueryPanel;
		std::shared_ptr<mPostSectionCutPanel> _postSectionCutPanel;
		std::shared_ptr<mPostStepSequencePanel> _postStepSequencePanel;
		std::shared_ptr<mPostEnvelopePanel> _postEnvelopePanel;
		std::shared_ptr<mPostVectorPlotPanel> _postVectorPlotPanel;
		std::shared_ptr<mPostLinearSuperpositionPanel> _postLinearSuperpositionPanel;
		std::shared_ptr<mPostScalarOperationPanel> _postScalarOperationPanel;
		std::shared_ptr<mPostIsosurfacePanel> _postIsosurfacePanel;
		std::shared_ptr<mPostDeformationPanel> _postDeformationPanel;
		std::shared_ptr<mPostSectionPanel> _postSectionPanel;
		std::shared_ptr<mPostVectorPanel> _postVectorPanel;
		std::shared_ptr<mPostStreamLinePanel> _postStreamLinePanel;
		std::shared_ptr<mPostSetPanel> _postSetPanel;
		std::shared_ptr<mPostNotesPanel> _postNotesPanel;
		//信息标签弹窗宿主（openNotesAsDialog 懒创建，父挂主窗随其销毁；注释面板
		//从 dock 弹出时装进其内容区，回 dock 时 mountToHost 收起）
		MechMainWindow::mFramelessDialog* _notesDlg{};
		std::shared_ptr<mPostExplodePanel> _postExplodePanel;
		std::shared_ptr<mPostMeasureTool> _postMeasureToolPanel;
		std::shared_ptr<mPostTagPanel> _postTagPanel;
		std::shared_ptr<mPostSectionForce> _postSectionForcePanel;
		std::shared_ptr<mPostCoordinatePanel> _postCoordinatePanel;
		std::shared_ptr<mPostPathPanel> _postPathPanel;
		std::shared_ptr<mPostLinePanel> _postLinePanel;
		std::shared_ptr<mPostArrowPanel> _postArrowPanel;
		std::shared_ptr<mPostTextPanel> _postTextPanel;
		std::shared_ptr<mPostLogPanel> _postLogPanel;
	};
}
