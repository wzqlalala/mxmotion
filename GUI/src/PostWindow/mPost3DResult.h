#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mpostwindow_global.h"

#include "mxr/data/SelectDataStruct.h"
#include "mxr/view/REBUILD_PostRend.h"
#include "mxr/view/REBUILD_PostRender.h"
#include "mCustomColorBar.h"

#include <memory>
#include <set>

//QT
#include <QWidget>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QHash>
#include <QSet>
#include <QGridLayout>
#include <QLabel>
#include <QToolButton>
#include <QMdiSubWindow>
#include <QComboBox>
#include <QSpinBox>
#include <QPointer>
#include <QButtonGroup>
#include <QCheckBox>
#include <QPushButton>
#include <QRadioButton>

class QAction;
class QListWidget;

#include "mPanelBase.h"
#include "mPostSTLExport.h"

//V1.3 云图面板共享库控件（MxFunctions）：数值过滤行/图例边界行/
//共框单选组/点击跳值滑块/主色按钮（PostWindow.md §6.7：跨面板形态一律取库）
//对象选择行已换 mPost 拾取栈（2026-11-18，mObjectPickComposite 下线）
#include "mPostGeneralPickUp.h"
#include "mValueFilterRow.h"
#include "mLegendBoundRow.h"
#include "mChoiceGroup.h"
#include "mSlider.h"
#include "mPrimaryButton.h"

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;


namespace PostData
{
	class mPostDataBase;
	class mPostDataManager;
	class mPostModelData;
	struct oneCaseInfo;
}

namespace MIOFile
{
    class mIONativeFile;
    class mIOVtkFile;
    class mMxdbThread;
    class mIMxdbFile1;
}

//ads（qtadvanceddocking）前向声明已随 v1.4 退役：原"动画播放"停靠页签（v1.3 停靠化）
//随 ADS 形态退役，控件条改挂主窗口 QToolBar 壳（animToolBarProvider 通道）

namespace MechPreData
{
	class mViewportModel;
	class mViewportPrivate;
	class mPostGUIData;
}

	namespace PostWindow
{
	/*
	* 处理失效单元：切帧时对比上/当前帧失效集合，增量设置单元显隐。
	* modeldb-后处理数据库  rebuildPostRend-当前渲染  fi-当前帧  lastFi-上一帧
	* 返回值：本帧是否存在失效单元显隐变化（用于决定是否重建渲染索引）
	*/
	bool processFailedElements(QPointer<PostData::mPostDataBase> modeldb, mxr::REBUILD_PostRend* rebuildPostRend, int fi, int lastFi);

	/*
	* 隐藏当前帧失效单元：以“全显示”为基准，仅隐藏当前帧失效单元。
	* 主要用于动画生成阶段，每帧独立处理，不依赖上一帧。
	* 返回值：本帧是否存在失效单元显隐变化。
	*/
	bool hideFailedElementsForFrame(QPointer<PostData::mPostDataBase> modeldb, mxr::REBUILD_PostRend* rebuildPostRend, int fi);

	/*
	* 带面缓存复用的隐藏当前帧失效单元：在 hideFailedElementsForFrame 基础上，
	* 通过 mesh signature 判断网格拓扑是否变化，复用上一帧的面拓扑数据。
	* lastLoseEles 为上一帧失效单元真实ID集合，用于失效集合增量更新。
	* 返回值：操作是否成功。
	*/
	bool hideFailedElementsForFrameWithCache(
		QPointer<PostData::mPostDataBase> modeldb,
		mxr::REBUILD_PostRend* rebuildPostRend,
		int fi,
		const std::set<int>& lastLoseEles);

	/*
	* 恢复当前帧失效单元显示：将当前帧所有失效单元恢复为可见。
	* 主要用于关闭失效单元隐藏开关时，恢复动画每一帧的显示状态。
	* 返回值：本帧是否存在失效单元显隐变化。
	*/
	bool restoreFailedElementsForFrame(QPointer<PostData::mPostDataBase> modeldb, mxr::REBUILD_PostRend* rebuildPostRend, int fi);

	class mPostPartScopeField;
	class MPOSTWINDOW_EXPORT mPost3DResultPanel : public mPanelBase
	{
		Q_OBJECT
	public:
		explicit mPost3DResultPanel(QWidget* parent = nullptr);
		//v1.4 保险收尾（面板 hub 单例正常不析构）：关弹窗+内容件从壳摘除（壳归主窗口
		//所有不删；删除期信号回环防御：先断四路再摘）
		~mPost3DResultPanel() override;

        //初始化光源弹窗
        void initLightWidget(QWidget* parent = nullptr);

		//引擎合一（2026-11-16）：浮层引擎态在播→pausePlayback 停播（预生成动画链互斥入口，
		//动画面板 slot_PlayAnimation 经 cloudResultPanel 调用）
		void stopRealtimeEngineIfActive();

		//新建三维结果
		bool new3DResultPanel(bool isRead, int caseId, QStringList paths = QStringList(),
			const QString& modelFilePath = QString(),   //modelFilePath：op2显式伴随网格（直导分支透传，空=自动探测）
			bool notifyScanHint = true);                //新版格式扫描提示开关（会话恢复的自动导入不提示）

		//重新读取结果数据更新视口
		void update3DResultViewport(int caseId, QStringList viewports);

		//把结果库批量应用到一组云图视口（会话恢复用；updateResultView 为私有
		//实现，此处公有转发）：逐视口 clear+setPostDataName+面板级联应用
		void applyResultToViewports(const QStringList& views, const QString& postname);

		//根据数据更新面板
		void update3DResultPanel(QString viewname);

		/*
		* 派生工况生成收尾（批次A1）：重灌目标视口工况下拉（分析+派生拼接口径同
		* update3DResultPanel）并级联切换到指定工况（blockSignals 重灌 + 手动
		* slot_chooseCase，帧表/变量/绘制走标准链）。caseName 不在列表内时不动作。
		*/
		void selectCaseByName(const QString& viewname, const QString& caseName);

		//读取mxdb文件
		bool readPostResultFile();

		//对新旧文件进行分类
		bool filterFiles(const QStringList paths,
			const QString& modelFilePath = QString(),   //op2显式伴随网格：优先于paths内多选的.bdf
			bool notifyScanHint = true);                //新版格式扫描提示开关（会话恢复的自动导入不提示）

		//导入重复模型
		void importRepeatModel(QString modelName);

		//导入新格式文件
		bool importNewFile(const QString& filename, bool notifyScanHint = true);

        //导入原生结果文件（OP2/ANSYS RST·RTH/LS-DYNA d3plot/H3D/ODB，走 MxNativeReaders；
        //explicitBdfFile 为空时由库按同名 .bdf/.nas 自行探测伴随模型）
        bool importNativeFile(const QString& filename,
                              const QString& explicitBdfFile = QString());

        /*
        * 导入 VTK 后处理文件组（.vtk/.vtp/.vtu/.vtm/.pvtp/.pvtu，走 VTKFileReader/vfr）：
        * 多选自动成组为帧序列（文件名自然排序 = 帧序，要求各帧拓扑一致），单文件即单帧
        */
        bool importVtkFile(const QStringList& filenames);

        /*
        * 获取指定后处理文件对应的 VTK 懒加载读取器；未注册时返回空指针。
        * 公有供面板外逻辑按路径查（与 nativeReaderFor 同口径）
        */
        MIOFile::mIOVtkFile* vtkReaderFor(const QString& postFile) const;

        /*
        * 获取指定后处理文件对应的懒加载读取器；未注册时返回空指针。
        * 公有供面板外逻辑按路径查（如变形控制读变量分量场），与云图显示链路（slot_showCloud）同口径；
        * 不能靠 qApp->findChildren 找 reader：其父链止于顶层主窗口，不在 QApplication 的 QObject 子树内
        */
        MIOFile::mIONativeFile* nativeReaderFor(const QString& postFile) const;

        /*
        * 获取指定后处理数据库对应的初始帧文件（.mxdb0）：老格式 mxdb 按需读取
        * （mIMxdbFile1::setInitFileName）依赖它探测产品线/精度，未注册时返回空串
        */
        QString initFileFor(const QString& postName) const;

        /*
        * 获取指定老格式 mxdb 结果的按需读取 reader（未注册时创建并缓存）。
        * key=getFile()，老格式该项即 .mxdb0 初始帧文件（兼作 setInitFileName 入参）。
        * 结果删除后旧 reader 因库查找落空读取失败（不崩溃），重导时由
        * openPostFileNames 入口按 key 摘除换新
        */
        MIOFile::mIMxdbFile1* mxdbOldReaderFor(const QString& postFile);

        /*
        * 获取指定新格式 mxdb 结果的按需读取 reader（未注册时创建并缓存）。
        * 601/602 的 open 为全文件扫描建帧索引，池化复用后仅首次扫描
        * （mMxdbThread::ensureReaderOpen 同文件短路）。重导时由 importNewFile 摘除换新
        */
        MIOFile::mMxdbThread* mxdbNewReaderFor(const QString& postFile, QPointer<PostData::mPostDataBase> modeldb);

		//导入老格式文件
		bool importOldFile(const QStringList& filenames);

		/** 打开后处理文件（用于前后处理对接，参数化打开） */
		bool openPostFileNames(const QStringList& names);

		void getLongestInitFileName(QStringList& ininames);

		//更新信息显示
		void updateInformation();

		//设置当前视图名称
		void setCurViewName(const QString& viewName);

		/*
		* 按 guidata 当前云图状态同步面板并重绘（变形变量联动云图用）：
		* 与 setCurViewName 的差异——不清理视口切面裁剪、不归还共享折叠组（等值面接管期间
		* 组合框仍可更新）、不假设面板可见；语义同"云图缓存"恢复的后半段（回填+重绘）。
		* viewName-目标视口（写入面板 _curViewName 后以 guidata 为单一数据源重绘）
		*/
		void reloadCloudFromGUIData(const QString& viewName);
		//R2 Q-LIFE-1 审计修复：跨视口浮层守卫公共助手——_curViewName 全部异名写点
		//（setCurViewName/reloadCloudFromGUIData/update3DResultPanel/slot_create3DResultViewport）
		//统一调用；异名且动画页签存活→先停播放再程序性隐藏 dock（v1.3 停靠化：
		//hideRealtimeAnimDock，不算用户关闭）；同名调用零动作
		void switchViewGuard(const QString& newViewName);
		bool isRealtimeAnimPlaying() const;   //P2-2 审计修复：实时动画浮层播放态对外查询（定义在 cpp；测量面板播放门控扩展消费）

		//--------- 共享折叠组（对齐原型共享参数块：等值面面板接管期间物理移入其布局） ---------
		//"云图变量/对象及坐标系/数据处理"三个折叠组（顺序即布局顺序）
		QList<FoldSection*> sharedSections() const;

		//从本面板布局移除共享折叠组（等值面面板接管前调用，widget 归属暂不变）
		void detachSharedSections();

		//幂等归还：折叠组不在本面板时取回并插回操作条之后原位
		void reattachSharedSections();

		//"云图变量"组内变形系数行的显隐（等值面接管共享组期间隐藏，同原型；归还云图时恢复）
		void setDeformationRowVisible(bool visible);

		//共享拾取条可见性转发（等值面接管共享组后恢复显示用）：本面板 hideEvent 的
		//setPickWidgetVisible(false) 会把拾取条显式隐藏，显式隐藏跨 reparent 保持——
		//折叠组搬入等值面布局后若无人恢复，"对象选择"行只剩类型下拉（拾取条不可见
		//不可用，与云图不一致）。恢复入口必须在接管方（takeSharedSections）而非本面板
		//showEvent（等值面激活期间本面板恒隐藏，永远走不到）
		void setSharedObjectPickVisible(bool visible);

		//共享拾取失活（等值面面板 hideEvent 归还折叠组后收口武装中的拾取，防视口
		//过滤器残留）：只失活不改可见性——与归还后本面板 showEvent 的恢复调用先后
		//顺序无关（幂等），拾取条可见性随宿主面板自身显隐自然呈现
		void deactivateSharedObjectPick();

		//等值面接管共享折叠组期间的面板外上下文同步：编辑上下文切到指定云图视口并按
		//其 guidata 纯回填——共享组（云图变量/对象及坐标系/数据处理）的显示/写入槽全部
		//锚在本面板 _curViewName 上，上下文漂移时（多云图视口/程序化绑定）接管期间的
		//编辑会落到别的视口，与云图面板自身编辑语义不一致。刻意不走 setCurViewName：
		//其重副作用（回收共享折叠组/删目标视口切面/整幅重绘重建单帧渲染器毁等值面）
		//在接管期间均不适用，此处仅切上下文+回填（updateResultInfoGUI 全控件 blockSignals）
		void syncEditContextToView(const QString& viewName);

		//过滤功能
		void startFilter(QString min, QString max, QString type);

		//清空过滤
		void clearFilter();

		//云图-过滤 导出过滤后模型的STL文件（fileName为保存路径）
		void exportFilterSTL(const QString& fileName);

		//显示值
		void showValue();

		//隐藏值
		void hideValue();

		//隐藏全部
		void hideAll();

		//切换至云图最大帧
		void switchToMaxFrame();

		/** 全帧统一值域（HyperView 语义）：当前变量跨所有帧的最小/最大（out.first=min，
		* out.second=max）。mxdb 结果逐帧真读一次并按（结果名+变量身份+帧数）缓存，切帧零
		* 开销命中；SolidColor 与 native/VTK 懒加载（逐帧真读代价被明确规避，见
		* switchToMaxFrame 同款注释）返回 false，调用方回退逐帧值域。动画生成/播放链经
		* mPostInfoHub 复用同一缓存 */
		bool allFrameRange(const QString& postName, QPointer<PostData::mPostDataBase> modeldb,
			const QString& varstr, const QString& compostr, const QString& layerstr,
			const QString& complexstr, QPair<double, double>& out);

        //设置显示模式
        void setShowMode();

	private:
		// 排序QStringList
		QStringList sortQStringList(const QStringList& frameIds);

		//绘制云图
		void drawCloudMap();

		/*
		* 云图延迟应用：面板参数改动只记脏不重绘，点"应用渲染"（或帧切换等整幅重绘）才生效。
		* setCloudDirty 置/清脏并联动"应用渲染"按钮高亮提示（cloudPending 动态属性）；
		* 整幅重绘成功（slot_showCloud/slot_changedFrame 尾部）与回填对齐（updateResultInfoGUI）统一清脏
		*/
		void setCloudDirty(bool dirty);

		/** 参数槽统一入口：记脏并点亮"应用渲染"按钮 */
		void markCloudDirty();

		/*
		* 从 guidata 重放离散三件套（离散开关/阶数/等差等比）到渲染会话。
		* 仅"应用渲染"路径调用：帧切换不重放（渲染会话自带状态），避免用 guidata
		* 默认值覆盖渲染会话默认态
		*/
		void applyDiscreteSettings(mxr::REBUILD_PostRender* postRender, MechPreData::mPostGUIData* guidata);

		/*
		* 按当前"数值过滤"下拉/过滤值下发过滤（无=清除；边界取 guidata 图例值域）。
		* 旧 slot_chooseFilterMode 的即时下发段抽出：延迟应用后由"应用渲染"统一调用
		*/
		void dispatchCloudFilter();

		/** 范围模式"全部"档值域解析，按图例范围三模式（HyperView Fixed/Dynamic Scale 对应）：
		* 手动=guidata 锁定值域（用户改上下限后跨帧保持）；全程=全帧统一值域（缓存，
		* 图例/配色跨帧稳定、颜色可跨帧比较）；逐帧=当前帧原始值域。值不可得均回退
		* 当前帧原始值域（queryProxy 的 original 最值） */
		void resolveFullRange(float& maxval, float& minval, const QString& postName,
			QPointer<PostData::mPostDataBase> modeldb, const QString& varstr, const QString& compostr,
			const QString& layerstr, const QString& complexstr,
			mxr::IPostRendQueryDataProxy* queryProxy, MechPreData::mPostGUIData* guidata);

		//查找满足条件的字符串
		bool isContainStr(const QStringList& names, QString str, QString& name);

		//保存当前云图设置为缓存快照（guidata->_cloudPresets 追加）
		int saveCloudPresetSnapshot();

        bool eventFilter(QObject* obj, QEvent* ev) override;


		//v1.4：面板退出瞬时态收口（§9 契约——宿主 setMainWidget 切换面板时触发，
		//等值面/切面图接管共享组等场景）：关闭设置弹窗防悬挂；播放链不动（切面板
		//不切上下文，与等值面接管共享组既定口径一致）；2026-11-18 增拾取控件
		//setPickWidgetVisible(false) 收口，showEvent 对称恢复
		void hideEvent(QHideEvent* event) override;
		void showEvent(QShowEvent* event) override;

		//实时动画 v1.4 常显（D-08"总是显示"+QToolBar 化，原 v1.3 ADS dock 退役）：
		//工具栏壳=mCenterMainWindow::initAnimToolBar 预创建（restoreLayout 前已存在→
		//saveState/restoreState 位置持久化成立），本面板经 MechCommon::animToolBarProvider
		//取壳回挂内容件（ensureRealtimeAnimMounted，幂等）。显隐语义=后处理上下文常显：
		//无结果/单帧→停播+控件禁用态（可见但不可操作，替代原"停播+隐藏"）；多帧→帧范围/
		//帧时间轴刷新+禁用解除。原"多帧自动显示/单帧自动隐藏/手动关闭不回弹"整组随 ADS
		//形态退役。调用点=帧表重建两处动态刷点（slot_chooseCase/updateResultInfoGUI）。
		//单动源互斥时机后移：原"创建即 deleteAnimation 停预生成动画"随常显退役，改在
		//首次播放放行路径上消费（slot_realtimePlayToggled，_animDeletePending 一次性旗，
		//帧表重建时置回）。录制互斥四向沿用（录制中工具栏常显保持暂停态，手动播放由
		//正向闸拒启+提示兜底）；录制结束不追溯
		void updateRealtimeAnimEnabled();
		//v1.4 挂载出口（原 ensureRealtimeAnimDock 退役替代）：控件条懒创建+四路信号接线
		//（协议不变）+P2-3 帧对齐（创建一次性）+经 provider 挂壳（幂等，provider 未就绪
		//跳过下次重入）+壳 installEventFilter（拖动/脱停关弹窗）
		void ensureRealtimeAnimMounted();
		//帧时间轴批量读取（开放问题 1 结论：mPostDataBase::getFrameTime 真读，1 基帧号；
		//按帧下拉项名解析帧号对齐——排序后下拉序≠帧号序；全帧<=0=数据源无帧时间，
		//FR-09/11/14 真接入数据源随 setFrameCount 同步下发控件条，缺失时控件条读数/
		//W2 时间域映射回退帧序/全表）。数十帧量级一次直读，随帧表刷新
		QVector<double> animFrameTimes();
		//FR-13 未变形构形真接入（D-05 升格）渲染侧三件：
		//applyRealtimeUndeformedShape=构形置零（记既定系数=不动 guidata，写门面
		//setDeformationScale(0,0,0)+当前帧 queryProxy 同步置零，先例 mPostDeformation.cpp:92-93；
		//deformationChanged 广播由门面照常触发，测量标注等消费方自动跟随）；
		//restoreRealtimeDeformedShape=恢复既定系数（applyPostDeformation 权威链，清武装）；
		//reassertRealtimeUndeformedShape=云图重绘链重断言（applyPostDeformation 每次重绘
		//按既定系数覆盖渲染态，未变形武装期须紧随其后重写置零——不武装则首次切帧即失效）。
		//与录制（playbackState=Stopped 期）/预生成互斥组合均为纯状态写入，不误触播放链
		void applyRealtimeUndeformedShape(REBUILD_PostRend* postRend);
		void restoreRealtimeDeformedShape(REBUILD_PostRend* postRend);
		void reassertRealtimeUndeformedShape(REBUILD_PostRend* postRend);

	private slots:
		/** [UiShot/自动化测试钩子] 自动导入模型（无文件对话框）：优先读固定路径
		 *  清单 import_files.txt（UTF-8 逐行一路径），缺席回退环境变量
		 *  MXTEST_IMPORT_FILES（分号分隔）；皆空静默直退，生产环境零影响 */

		/** 导入后处理新文件完成 */
		void slot_mxdbThreadFinished();

		/** 导入后处理旧文件完成 */
		void slot_mxdbOldThreadFinished();

        void slot_nativeThreadFinished(const QString& postname);

        void slot_vtkThreadFinished(const QString& postname);

		/** 读取后处理文件完成 */
		void slot_finishedImportFiles(const QString& postname);

		//创建云图视口	
		void slot_create3DResultViewport(QString name);

		/** 选择工况下拉框 */
		void slot_chooseCase(const QString&);

		/** 选择帧下拉框 */
		void slot_chooseFrame(const QString&);

		//浮层帧序变化 → 驱动 _comboFrame 切帧（复用 slot_chooseFrame→slot_showCloud 全链）
		void slot_realtimeFrameChanged(int index);
		//实时动画 v1.1：浮层播放开关（正向互斥即时判定：录制中/预生成动画在播→拒启自暂停）
		void slot_realtimePlayToggled(bool playing);
		//unify-anim-playback 职能改造：参数化插值档变化 → 录制互斥+反向闸（归属豁免）+
		//暂停态静态定位路由（positionInterpolation）/引擎档播放中提交路由（pause+定位+play
		//三连）；懒生成与插值直驱职能退役（播放推进=ensureParametricEnginePlayback 承载）
		void slot_realtimeInterpolationChanged(int step);
		//v1.1：浮层形态切换 → 渲染侧收尾（回帧模式退出浮层自生成的参数化形态）
		void slot_realtimeModeChanged(int mode);
		//D-05 升格 FR-13：构形切换（控件条 shapeChanged）→ 未变形=置零武装/变形后=恢复
		//既定系数（渲染侧收口，同四路信号面板收口同构；纯状态写入不触播放链）
		void slot_realtimeShapeChanged(int mode);
		//v1.1：录制发起（hub 广播，引擎链录制调用前）→ 播放中的浮层按 v1 停止语义让路自停
		void slot_realtimeRecordingStarted();
		//v1.4 前后处理上下文显隐（开放问题 6 结论：前处理隐藏/后处理常显）：边沿=
		//currentViewportChanged 增量刷新（同 ID 激活去重早退不发，mMenuWidget.h:688 已知语义）；
		//初值=构造期求值一次（T-2/J-02：信号无重放，面板构造晚于最后一次激活即漏；
		//restoreLayout 的 center->restoreState 还会把上一会话"可见"写回——只靠边沿必漏）。
		//判定=getCVM->getPostRenderWidget() 判空（SKILL 踩坑 14 同款前/后处理判定）
		void slot_viewportContextChanged();
		//ui-shot 弹窗截图驱动（mUiShotController::dialogSlots 登记名"mAnimSettingsPopup"）：
		//确保挂载后打开设置弹窗（--ui-shot-dialog 经 invokeMethod 调用，T-3 验收路径）
		void slot_openAnimSettingsForShot();
		//ui-shot 类型菜单截图驱动（dialogSlots 登记名"animTypeMenu"，R3 盲区闭合）：
		//确保挂载后非模态弹出类型菜单（QMenu::popup 不阻塞事件循环，可 grab 取证）
		void slot_openAnimTypeMenuForShot();

		/** 选择变量下拉框 */
		void slot_chooseVariable(const QString&);

		/** 选择分量 */
		void slot_chooseCompo(const QString&);

		/** 选择层下拉框 */
		void slot_chooseLayer(const QString&);

		/** 选择复数下拉框 */
		void slot_chooseComplex(const QString&);

		/** 选择渲染范围 */
		void slot_chooseRangeType(int index);

		/** 设置位置显隐 */
		void slot_setPositionVisible(bool isshow);

		/** 重置最大/小值 */
		void slot_resetMostValue();

		/** 最值改变 */
		void slot_editValueChanged();

		/** 平滑处理 */
		void slot_discreteCombox(const QString&);

		/** 离散阶数 */
		void slot_discreteOrder(int order);

		/** 设置离散类型 */
		void slot_discreteTypeComboBox(const QString& text);

        /** 显示隐藏颜色表 */
        void slot_showOrHideColorBar();

        /** 自定义颜色表 */
        void slot_customColorBar();

        /** 复位颜色表 */
        void slot_resetColorBar();

        /** 光源弹窗 */
        void slot_toggleLightPopup(bool istrue);

        /** 打开光源弹窗前从渲染读回当前光照状态刷新控件（渲染→界面同步） */
        void syncLightWidgetFromRender();

        /** 设置光源是否跟随摄像机 */
        void slot_setLightIsDependentCamera(const QString& istrue);

        /** 设置光源的位置 */
        void slot_setLightPosition();

        /** 设置光源的参数 */
        void slot_setLightParameters(int value);

		//云图
		void slot_showCloud();

		/** 改变最大/小值 */
		void slot_setMostValue(bool flag);

		//切换帧数
		void slot_changeFrame();

		/** 磨平复选框切换 */
		void slot_planishToggled(bool checked);

        void slot_planishIndexChanged(int index);

		/** 更新磨平复选框启用/禁用状态 */
		void updatePlanishEnabled();

		/*
		* 更新三维结果信息页界面
		*/
		void updateResultInfoGUI();

		/*
		* 更新云图节点的视图
		*/
		void updateResultView(QStringList views, QString postname);

		/** 保存云图缓存（快照 + 树节点） */
		void slot_saveCloudPreset();

		/** 清除渲染（恢复未着色基色显示） */
		void slot_clearRendering();

		/** 应用渲染（按当前面板设置重绘云图） */
		void slot_applyRendering();

		/** 打开变形控制面板（齿轮按钮，经总线转发） */
		void slot_openDeformation();

		/** 内联变形系数应用（回车） */
		void slot_applyInlineDeformation();

		/** 渲染对象类型切换（单元/部件，渲染端暂未对接；切换清拾取数据并重置"所有X"） */
		void slot_chooseObjectType(const QString& text);

		/** 拾取控件激活/失活收口：激活=按类型设视口过滤器（无效后处理上下文回退失活）；
		* 失活=清过滤器+恢复工具栏拾取类型；收割/高亮/重入回显由拾取栈控制器自管 */
		void slot_objectPickActiveChanged(bool active);

		/** 图例边界行编辑/复位启停统一出口：仅图例范围=手动时可改（逐帧/全程值域
		* 自动计算；眼睛显隐独立不受影响）——构造/切换槽/回填三处共调，禁多处各写 */
		void refreshLegendBoundEditState();

		/** 数据处理：磨平方式切换（9 项，映射现有两态渲染；高级磨平时显示变化率行） */
		void slot_chooseSmoothMethod(int index);

		/** 高级磨平变化率启用切换 */
		void slot_variationToggled(bool checked);

        /** 图例范围切换（逐帧/全程/手动，渲染端暂未对接） */
        void slot_chooseLegendRange();

        /** 信息标签-显隐切换（同渲染图例模式：隐藏态按钮变"显示"并禁用复位/自定义；
        * V1.3 原型虽删该组，用户定案功能保留——见差异表裁定表） */
        void slot_labelHide();

        /** 信息标签-复位（恢复默认显示状态：可见+默认字体15px/黑色） */
        void slot_labelReset();

        /** 信息标签-自定义（经总线打开注释/信息标签配置面板，字段与字体配置在该面板） */
        void slot_labelCustom();

	public slots:
		//切换帧（高级查询需要用到）
		void slot_changedFrame(int f);

        void setDefaultDiscreteOrder(int order);

	signals:
		/*
		* 后处理导入文件完成时发送的信号
		*/
		void finishedReadPostFileSig(const QString& postname);

		/*
		* 云图按当前设置重绘完成（携带视口名）。消费方：等值面面板——重绘的是
		* 其绑定视口且已生成等值面时，按存储阈值随新帧/新变量自动重提取
		* （HyperView 语义：同一实际值在新帧场上提取，未跨阈值的帧无等值面）
		*/
		void cloudRedrawnSig(const QString& viewName);

	private:
        /*
        * 判断指定后处理文件是否为按帧懒加载的原生读取结果（OP2/RST·RTH/d3plot/H3D/ODB）
        */
        bool isLazyNativeResult(const QString& postFile) const;

        /*
        * 判断指定后处理文件是否为按帧懒加载的 VTK 读取结果（.vtk/.vtp/.vtu/.vtm/.pvtp/.pvtu）
        */
        bool isLazyVtkResult(const QString& postFile) const;

        /*
        * FR-08 渐进装载兜底统一入口（补绘挂起集合版，2026-11-16）：seek 失败帧登记
        * 挂起集合并武装 frameReady/loadError 常驻连接——frameReady 命中集合才消费，
        * 补绘时临时把下拉指向挂起帧（blockSignals+恢复冻结显示），集合清空拆线；
        * arm 即重连自愈换 sender。slot_showCloud 兜底/路径B 兜底/实时播放快路径共用。
        */
        void armFrameLoadFallback(int fi, mxr::REBUILD_PostRender* postRender);

        /*
        * 播放推进轻量镜像（引擎合一 2026-11-16）：脚标/图例值域（手动/全程/逐帧三模式）/
        * 点大小，代理现取零重绘——浮层快路径（自驱 seek 后）与会话播放器镜像单通道
        *（playTick 补发 PlayAdvance frameChanged）共用
        */
        void mirrorFrameValueDisplay(REBUILD_PostRend* postRend, mxr::REBUILD_PostRender* postRender,
            MechPreData::mPostGUIData* frameGui, const QPointer<PostData::mPostDataBase>& frameDb, int fi);

        /*
        * 引擎档播放中设置即时下发（引擎合一 2026-11-16）：浮层 engineSettingsChanged
        * （弹窗速度/步长/方向/循环/范围变更）→ 会话 setFrameRate/setFrameInterval/
        * setPlaybackDirection/setLoop/setPlayRange 现值重下发（FR-12 播放中即时生效
        * 延伸到引擎档）；unify-anim-playback：改往返不再降档，setPingPong 参数下发
        * （范围块仅 Frame 模式——参数化域恒 1..20 无范围语义）
        */
        void reconfigureRealtimeEngine();

        /*
        * 引擎档镜像/收口三通道按视口重绑（审计 B-02）：frameChanged 镜像（含自然停机/
        * 录制互斥/值失配暂停收口）/interpolationAdvanced 镜像/playbackDirectionChanged
        * 回流三条 connect 原一次性绑死首挂载时点视口门面实例——多视口/先挂载后加载两
        * 路径下事件全落旧实例（镜像失随/停机回写失效/幽灵归属）。本助手按给定视口门面
        * 断开旧连接+建立新连接（QMetaObject::Connection 簿记，mPostAnimation
        * _frameChangedConns 先例）；门面未就绪=保持断开，下次挂载/切视口重试
        */
        void rebindRealtimeEngineMirrorConns(const QString& viewName);

        /*
        * 参数化形态（线性/正弦）引擎档承载（unify-anim-playback，恒引擎档无混合档）：
        * 懒生成（无数据门保留）→参数下发（setFrameRate/setLoop/setPingPong/
        * setPlaybackDirection/setFrameInterval/setPlayRange(1,kParamStepCount)）→档位域
        * 尾端定位→enterEnginePlayback→positionInterpolation→play()；点播放同步路径调用
        * （生成即就绪，首播延迟不劣于现状）
        */
        void ensureParametricEnginePlayback(mxr::REBUILD_PostRender* postRender);

        /*
        * 播放推进轻量镜像（引擎合一 2026-11-16）：脚标/图例值域（手动/全程/逐帧三模式）/
        * 点大小，代理现取零重绘——浮层快路径（自驱 seek 后）与会话播放器镜像单通道
        *（playTick 补发 PlayAdvance frameChanged）共用
        */

        /*
        * 播放推进轻量镜像（引擎合一 2026-11-16）：脚标/图例值域（手动/全程/逐帧三模式）/
        * 点大小，代理现取零重绘——浮层快路径（自驱 seek 后）与会话播放器镜像单通道
        *（playTick 补发 PlayAdvance frameChanged）共用
        */

		QString									_mxdbDirPath;               //文件夹路径
		QPointer<mxr::REBUILD_PostRend>			_curPostRend{};             //当前激活窗口
		QMetaObject::Connection					_frameReadyRedrawConn{};    //FR-08补绘：渐进装载完成(frameReady)补绘云图连接（挂起集合清空时拆线；arm 时重连自愈换 sender）
		QMetaObject::Connection					_frameLoadErrorConn{};      //S6：渐进装载失败(loadError)透出连接（生命周期=挂起集合非空）
		QSet<int>								_frameReadyPending{};       //FR-08补绘挂起帧集合：seek 失败帧登记，frameReady/loadError 命中消费——多帧并存互不拆线（原单连接会被下一帧派发拆线，渐进装载期整遍停帧）
		//v1.2 删除 _realtimeAnimDestroyedConn：原为浮层 destroyed 归位开关按钮的单发连接，
		//按钮已删无归位点，收起统一走 destroyRealtimeAnimWidget（deleteLater+清 QPointer）
		QString											_lastChooseVarIdentity{};       //P1-1/P6-3 审计修复：slot_chooseVariable 上次身份（视口|工况|变量|分量|层|复数|磨平），未变跳过纪元 bump
		int										_caseId = -1;
		bool									_isUpdateResult = false;    //是否更新结果
		QStringList								_updateViewNames;			//需要更新的视图名称
		bool									_isDrawCloudMap = true;     //是否允许绘制云图
		bool									_cloudParamDirty = false;   //云图-参数待应用（点"应用渲染"或整幅重绘后清零）
		bool									_frameNavGuard = false;     //帧导航/最大帧跳转期间屏蔽 slot_chooseFrame 信号放绘（由导航尾部统一补绘一次）
		bool									_rangeEditPending = false;  //云图-上下限/范围类修改待提交（应用时经 slot_setMostValue 落库）
		bool									_discreteEditPending = false; //云图-离散三件套待下发（用户改动/回填恢复时置位，应用时按位重放，防 guidata 默认值覆盖渲染会话默认态）
		QPointer<MIOFile::mPostSTLExportThread>	_stlExportThread{};         //云图-过滤 导出STL后台线程
		QHash<QString, PostData::oneCaseInfo*>	_caseInfos{};               //key-工况  value-工况信息
		std::set<int>							_ids{};					    //存储的显示的节点或单元ID
		QHash<QString, QString>					_postInitFiles{};           //key-post数据库名  value-对应初始帧文件(.mxdb0)
        //S6 读数缓存命中：key-"postPath|帧号"，value-读取身份（分支|变量|分量|复数|层[|磨平]）。
        //仅读数成功后登记；命中还须库侧 _resultVarNames 名字/类型/数据三重复核（见 slot_showCloud）
        QHash<QString, QString> _cloudReadIdentityByKey{};
		//全帧统一值域缓存：key=结果名|变量|分量|层|复数 → (min,max)；帧数守卫配套（结果重载帧数变化失效）
		QHash<QString, QPair<double, double>>	_allFrameRanges{};
		QHash<QString, int>						_allFrameRangeFrameCounts{};
        QHash<QString, MIOFile::mIONativeFile*> _nativeReaders{};
        //VTK 读取链路（key=组内首文件规范化路径，与 mPostDataBase::getFile 对齐）
        QHash<QString, MIOFile::mIOVtkFile*> _vtkReaders{};
        //异步导入失败回滚的归属捕获（VTK 链路，语义同 _nativeReaderOwner）
        QHash<MIOFile::mIOVtkFile*, int> _vtkReaderOwner{};
        //异步导入失败回滚的归属捕获：读线程/reader → 导入启动时的宿主后处理模型ID。
        //失败发生在读取线程完成之后（import3DResult 已返回），期间用户切换模型会使
        //getCVM 指向别的模型——删结果库/发失败通知必须按捕获ID操作才不误伤
        QHash<MIOFile::mIONativeFile*, int> _nativeReaderOwner{};
        QHash<MIOFile::mMxdbThread*, int>   _mxdbThreadOwner{};
        QHash<MIOFile::mIMxdbFile1*, int>   _mxdbOldThreadOwner{};
        //mxdb 按需读取 reader 池（key=结果文件路径=getFile()）：池化复用避免每次切变量/
        //逐帧动画重复构造 reader 与重开文件（601/602 重开=全文件扫描建帧索引）。
        //mMxdbThread 池对象 parent=this 随面板回收；mIMxdbFile1 默认构造无 parent 形参，
        //创建后 setParent(this)；重导入口（importNewFile/openPostFileNames）按 key 摘除换新
        QHash<QString, MIOFile::mIMxdbFile1*> _mxdbOldReaderPool{};
        QHash<QString, MIOFile::mMxdbThread*> _mxdbNewReaderPool{};
        //归属中转：slot_finishedImportFiles 的失败发射点（:2983/:2990）只收 postname，
        //而 mxdb 新旧两链的线程对象在发射 finishedReadPostFileSig 前即被销毁——
        //三条链在各自发射前把宿主模型ID存此（同对象直连同步分发，读取即本次导入
        //的归属，无跨事件循环滞留）
        int _finishedImportOwnerID = -1;
        mCustomColorBar*                        _customColorBar{};
        QVector<QColor>					        _defaultColors;
        QVector<QString>				        _defaultValues;


		QVBoxLayout*							_mainLayout;
		//共享折叠组宿主（等值面面板接管/归还，见 sharedSections）
		FoldSection*							_changeFold{};				//云图变量
		FoldSection*							_objectFold{};				//对象及坐标系
		FoldSection*							_planishFold{};				//数据处理
		QComboBox*								_comboCase;
		QComboBox*								_comboFrame;
		QPushButton*							_btnPreFrame;
		QPushButton*							_btnNextFrame;
		QPushButton*							_btnFirstFrame;
		QPushButton*							_btnLastFrame;
		//v1.2 删除 _btnRealtimeAnim（QToolButton checkable，帧导航行第 5 位"实时动画"开关）：
		//用户拍板云图界面不放开关按钮，浮层改按帧数自动显隐（已随 v1.4 重写为使能管理）
		//v1.3→v1.4 停靠化退役：原 ads::CDockWidget"动画播放"承载随 ADS 形态退役（04 评审
		//已核实退役清单完备）——控件条改挂主窗口 QToolBar 壳（animDisplayBar，QPointer
		//防悬空；与面板同生命周期复用，显隐=前处理隐藏/后处理常显）
		QPointer<class mPostRealtimeAnimWidget>	_realtimeAnimWidget{};      //实时动画控件条（工具栏壳内容件，播放控制/会话/帧档语义）
		//FR-13 未变形构形置零武装中的渲染会话（QPointer 防悬空；结果切换/视口销毁后
		//指针失效即视为退出未变形档，恢复既定系数由新会话重绘链按 guidata 权威值自愈）
		QPointer<mxr::REBUILD_PostRend>			_undeformedArmedRend{};
		//v1.4 单动源互斥一次性旗（原"创建即 deleteAnimation"时机后移）：首次播放放行消费；
		//帧表重建（切工况/切结果=面板级动画上下文重建）置回，与 v1.3 创建即停同频
		bool									_animDeletePending = true;
		bool									_realtimeEngineActive = false;	//引擎合一：浮层帧模式播放由会话播放器驱动态（play 配置+playTick 推进）
		//【审计 B-02】引擎档镜像三通道 connect 簿记（按视口门面重绑——多视口/先挂载后
		//加载路径下首挂载一次性绑定失效，切视口/挂载时经 rebindRealtimeEngineMirrorConns
		//断开旧实例连接+接新实例）
		QMetaObject::Connection					_rtMirrorFrameConn{};
		QMetaObject::Connection					_rtMirrorStepConn{};
		QMetaObject::Connection					_rtMirrorDirConn{};
		//v1.4 控件条挂壳 action（QToolBar::addWidget 返回；析构保险摘除用）
		QAction*								_animBarAction = nullptr;
		QComboBox*								_comboVariable;
		QComboBox*								_comboComponent;
		QComboBox*								_comboComplex;
		QComboBox*								_comboLayer;
		QComboBox*								_comboRangeMode;
		//V1.3 图例组：范围三选一/平滑方式/分布方式=共框单选组；最大/最小值行=图例边界行
		MxFunctions::mChoiceGroup*				_legendRangeChoice{};
		MxFunctions::mLegendBoundRow*			_legendMax{};
		MxFunctions::mLegendBoundRow*			_legendMin{};
        QSpinBox*								_spinOrder;
		MxFunctions::mChoiceGroup*				_interpSmoothChoice{};
		MxFunctions::mChoiceGroup*				_interpSpacingChoice{};
        QPushButton*                            _btnShow;
        QPushButton*                            _btnCustom;
        QPushButton*                            _btnReset;
        QComboBox*                              _comboLightPos;
        QLineEdit*                              _lineEditX;
        QLineEdit*                              _lineEditY;
        QLineEdit*                              _lineEditZ;
        MxFunctions::mSlider*                   _sliderAmbient{};
        MxFunctions::mSlider*                   _sliderDiffuse{};
        MxFunctions::mSlider*                   _sliderMirror{};

        FoldSection*                            _infoFold;
		QLabel*									_labelCase;
		QLabel*									_labelTotalFrame;
		QLabel*									_labelMax;
		QLabel*									_labelMin;
        QPushButton*                            _btnLight;
        QPointer<QWidget>                       _lightWidget;   //QPointer：ui-shot 弹窗"用后即焚"后可重建

		//--------- 操作条（页面级） ---------
		QPushButton*							_btnSavePreset{};			//保存云图缓存
		QPushButton*							_btnClearRendering{};		//清除渲染
		QPushButton*							_btnApplyRendering{};		//应用渲染

		//--------- 云图变量（原"切换"折叠组扩展） ---------
		QCheckBox*								_checkCornerData{};			//使用角点数据（渲染TODO）
		QLineEdit*								_editInlineDeformCoef{};	//变形系数（内联快捷输入）
		QToolButton*							_btnOpenDeformation{};		//变形控制齿轮入口
		QWidget*								_deformCoefRow{};			//变形系数半区（等值面接管共享组时隐藏，同原型；随尾部双列区右列）
		QWidget*								_variableTailRow{};			//组尾双列区（角点数据|变形系数，两等分通栏，同原型 tail-check-composite）

		//--------- 对象及坐标系（渲染TODO，先落UI持久化） ---------
		//2026-11-18 交互口径：未拾取默认"所有X"（随类型）/单击拾取钮武装拾取/查看小钮
		//（">"）开已拾取清单弹窗/垃圾桶清空拾取集合；范围选项弹窗自该行下线
		QComboBox*								_objectTypeCombo{};			//对象类型下拉（0=单元/1=部件；默认部件）
		mPostPickWidgetController*				_objectPickController{};	//对象选择拾取控件（mPost 拾取栈；数据进程态，渲染待接入，消费走 getAllData()）
		mPostPartScopeField*					_btnObjectScope{};			//范围状态控制器（隐形；弹窗已不可达，仅提供类型联动默认文案"所有X"+guidata 持久化口径）
		QComboBox*								_comboCoordSystem{};		//坐标系
		QToolButton*							_btnCoordAdd{};				//快速新建坐标系（V1.3 已启用；无可接链路时提示待接入）
		QCheckBox*								_checkTrackingCoord{};		//使用跟踪坐标（与边中点值双列）
		QCheckBox*								_checkMidsideValues{};		//边中点值
		QWidget*								_coordTailRow{};			//组尾双列区（跟踪坐标|边中点值，两等分通栏）

		//--------- 数据处理 ---------
		QComboBox*								_comboSmoothMethod{};		//数据磨平（9项，与跨部件同行；替代原 _comboPlanish）
		QCheckBox*								_checkAcrossParts{};		//跨部件磨平（渲染TODO，与磨平同行）
		QCheckBox*								_checkVariationEnabled{};	//高级磨平变化率启用（渲染TODO）
		QLineEdit*								_editVariationLimit{};		//高级磨平变化率上限%（渲染TODO）
		QWidget*								_variationRow{};			//高级磨平行容器（仅磨平方式=高级磨平时显示）
		MxFunctions::mValueFilterRow*			_filterRow{};				//数值过滤（V1.3：左输入|无/大于/小于/之间|右输入 三段拼接行）

		//--------- 图例（V1.3 更名，原"图例和标签"；信息标签行用户定案保留） ---------
		QPushButton*							_btnLabelHide{};			//信息标签-隐藏
		QPushButton*							_btnLabelReset{};			//信息标签-复位
		QPushButton*							_btnLabelCustom{};			//信息标签-自定义（打开注释面板配置）
	};
}
