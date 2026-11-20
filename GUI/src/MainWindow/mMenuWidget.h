#pragma once
#pragma execution_character_set("utf-8")
#include "mmainwindow_global.h"

#include <QWidget>
#include <QButtonGroup>
#include <QToolButton>
#include <QTabBar>
#include <QStackedWidget>
#include <QGridLayout>
#include <QLabel>
#include <QScrollArea>
#include <QMap>
#include <QMenuBar>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMenu>
#include <QEvent>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QSet>
#include <QMdiSubWindow>
#include <QSlider>
#include <QTreeWidgetItem>
#include <QMouseEvent>
#include <QListWidget>
#include <QComboBox>
#include <QAction>
#include <QTimer>
#include <QActionGroup>
#include <QStringList>
#include <QFrame>
#include <QFontMetrics>

#include <memory>

#include "mIOEnum.h"
#include "mPhysicalFieldEnum.h"
#include "mMechGlobalSignals.h"
#include "mCalcLineEdit.h"
#include "mModelStruct.h"

//最近打开
#include "mRecentDataBase.h"

//自动保存（mAutoSaveManager.h 顶部的 MX_AUTOSAVE_ENABLED 为功能总开关）
#include "mAutoSaveManager.h"

#include <QProxyStyle>
#include <QStyleOptionTab>
#include <QPainter>
#include <QStyleOptionTab>
#include "mxr/basic/PublicStruct.h"
// httpserver.h
#pragma once
#include <QObject>
#include <QThread>
#include <QDir>
#include <QWebEngineView>
#include <QStylePainter>
#include <QStyle>
#include "mChineseTextEdit.h"
#include "mAPIServerTypes.h"
#include "mRibbonWidget.h"
#include "MyTabBar.h"
#include "mRibbonButton.h"
#include "IRibbonHost.h"
#include "mOpenCurveDlg.h" //NVH 曲线对话框（src 侧后处理曲线入口，菜单槽 slot_NVHCurve 使用）
#include "UiMetrics.h"
// UiPalette 仅在 .cpp 使用，头文件不引入，减少级联包含

class HttpServer;
using namespace MxFunctions;

using MyTabBar = MechMainWindow::MyTabBar;

namespace MechMainWindow
{
	class mFramelessDialog;
	//菜单栏工作模式：跟随当前激活视口类型切换页签集合（页widget常驻QStackedWidget，仅控制TabBar页签显隐）
	enum class MenuWorkMode
	{
		Pre,   //前处理：常驻页签 + 前处理动态页（草图/截面属性/截面形状）
		Post,  //后处理：主页/模型/结果/显示/选择（"云图"等候选页按需临时插入）
		Curve  //曲线：主页/曲线
	};

	class mGroupWidget;

	class MenuActionWidget : public QWidget
	{
		Q_OBJECT
	public:
		MenuActionWidget(const QString& text, int type, const QColor& bgColor, const QColor& hlColor, QWidget* parent = nullptr)
			: QWidget(parent), _type(type), _bgColor(bgColor), _highlightColor(hlColor)
		{
			setFixedHeight(28); // 自定义高度

			// 使用 this->setStyleSheet 直接设置当前 widget 的样式
			QString style = QString(
				"color:#303313;"
				"background-color: %1;"  // 默认背景色
				"border: none;"         // 去掉边框（可选）
			).arg(_bgColor.name());

			this->setStyleSheet(style);  // 直接应用样式

			QHBoxLayout* layout = new QHBoxLayout(this);
			layout->setContentsMargins(10, 0, 10, 0);
			QLabel* label = new QLabel(text, this);
			layout->addWidget(label);
		}

		int type() const { return _type; }

	signals:
		void clicked(int type);

	protected:
		void mousePressEvent(QMouseEvent* event) override
		{
			emit clicked(_type);
			QWidget::mousePressEvent(event);
		}

		void enterEvent(QEvent* event) override
		{
			this->setStyleSheet(QString("color:#303313;background-color: %1;").arg(_highlightColor.name()));
			QWidget::enterEvent(event);
		}

		void leaveEvent(QEvent* event) override
		{
			this->setStyleSheet(QString("color:#303313;background-color: %1;").arg(_bgColor.name()));
			QWidget::leaveEvent(event);
		}

	private:
		int _type;

		QColor _bgColor;

		QColor _highlightColor;
	};
	class CopyButton : public QPushButton {
		Q_OBJECT
	public:
		explicit CopyButton(QWidget* parent = nullptr);

	protected:
		void paintEvent(QPaintEvent* event) override;
		void mousePressEvent(QMouseEvent* event) override;

	private:
		QMenu* menu;
	};

	class AutoScaledPNG : public QWidget
	{
		QPixmap pix;
	public:
		explicit AutoScaledPNG(const QString& path, QWidget* parent = nullptr)
			: QWidget(parent), pix(path) {
			if (!pix.isNull()) {
				// 关键：把 widget 缩成图片原始大小
				setFixedSize(pix.size() / 2);   // 或 setMaximumSize(pix.size());
			}
		}
		void setPixmap(const QPixmap& p)
		{
			pix = p; update();
		}
	protected:
		void paintEvent(QPaintEvent*) override
		{
			if (pix.isNull()) return;
			QPainter p(this);
			// 保持比例 + 全填：Qt::KeepAspectRatio 或 Qt::IgnoreAspectRatio
			p.drawPixmap(rect(), pix, pix.rect());
		}
	};

	/*
	* 组合按钮（双动作全高入口，规则 1.3"分类跳转+快捷创建"）
	*
	* 2026-10-23 重构：由"容器+QSS+两个子按钮"改为单个全自绘 mRibbonButton（Mode::Split），
	* 与普通全高按钮/整钮下拉按钮共用同一绘制路径：同一外框、同一文字行盒基线、
	* 同一悬停/选中/焦点绘制规则，三种全高按钮外观严格一致。
	* 本类只保留业务编排（功能项/菜单/信号），不再参与绘制与尺寸计算。
	* ThemeManager 尺寸口径：uiBigButton=true，走普通全高按钮分支统一重算（高/图标/宽/主题）。
	*
	* 支持两种模式：
	* 1. 开关模式（如显示模块"自动旋转"）：图标区为开关，文字区下拉选择参数项，开启时
	*    图标区进入选中态，状态变化通过 toggled 信号通知外部；
	* 2. 动作模式（如几何菜单）：图标区点击执行当前功能项，文字区下拉切换功能项并立即执行。
	*/
	class mComboButton : public mRibbonButton
	{
		Q_OBJECT
	public:
		/*
		* 动作模式的功能项
		*/
		struct Item {
			QString text;                       // 显示文字
			QString iconPath;                   // 图标资源路径
			QString tooltip;                    // 悬停提示
			std::function<void()> callback;     // 点击执行的功能
		};

		/*
		* 构造函数
		*/
		explicit mComboButton(QWidget* parent = nullptr);

		/*
		* 析构函数
		*/
		~mComboButton();

		/*
		* 配置为开关型组合按钮（带选中态）：图标区为开关，文字区下拉选择参数项（如+/-Sx/Sy/Sz）
		* @param dropTextPrefix 文字区文字前缀（如"自动旋转"）
		* @param mainIconPath 图标区图标资源路径
		* @param mainTooltip 图标区悬停提示
		* @param items 下拉参数项
		*/
		void setupToggleItems(const QString& dropTextPrefix, const QString& mainIconPath, const QString& mainTooltip, const QStringList& items);

		/*
		* 配置为动作型组合按钮（无选中态）：图标区点击执行当前功能项，文字区下拉切换并立即执行
		* @param items 功能项列表，第一项为默认主功能
		*/
		void setActionItems(const QList<Item>& items);

		/*
		* 统一宽度：按规则 1.3 全高按钮宽度公式重算并固定
		* 外部设置文字/图标后调用；ThemeManager 切档时经元方法自动调用
		*/
		Q_INVOKABLE void syncButtonWidth();

		/*
		* 设置组合按钮宽度（建议值，规则公式值兜底，不低于公式最小宽）
		*/
		void setButtonWidth(int width);

		// 以下为旧双控件接口的兼容转发，尺寸统一由 sizeHint/公式管理，调用后转 syncButtonWidth
		void setButtonSize(int width, int mainHeight, int dropHeight);
		void setMainButtonHeight(int height);
		void setDropDownHeight(int height);
		void setMainButtonIconSize(const QSize& size);

		/*
		* 开关模式下是否处于开启状态
		*/
		bool isChecked() const;

		/*
		* 当前文字区文字
		*/
		QString currentText() const;

	signals:
		/*
		* 开关模式状态变化时发出
		* @param checked 是否开启
		* @param text 当前参数项完整文字
		*/
		void toggled(bool checked, const QString& text);

	private slots:
		/*
		* 文字区点击：弹出选择菜单，菜单关闭后同步选中状态（开关模式）
		*/
		void slot_dropClicked();

		/*
		* 菜单项被选择：开关模式更新参数并保持开启；动作模式切换功能项并立即执行
		*/
		void slot_menuActionTriggered(QAction* action);

	private:
		/*
		* 初始化菜单与信号连接
		*/
		void initUi();

		/*
		* 动作模式：把当前功能项的图标/提示/文字应用到按钮
		*/
		void applyCurrentItem();

		/*
		* 规则 1.3 全高按钮宽度公式值（文字取上下两区最宽项+箭头占宽+留白）
		*/
		int ruleWidth() const;

	private:
		// true=开关模式（带选中态）；false=动作模式（无选中态）
		bool m_checkMode = false;

		// 开关模式文字区文字前缀
		QString m_dropTextPrefix;

		// 动作模式功能项与当前项索引
		QList<Item> m_actionItems;
		int m_currentIndex = -1;

		// 选择菜单（内容由setupToggleItems/setActionItems填充）
		QMenu* m_menu = nullptr;
	};
	class mTransFileInfoDialog;
	class mCenterMainWindow;
	class mMaterialWindow;
	class mCustomMaterialWindow;
	class mMaterialProps;
    class mCaseImplicitModalWindow;
    class mCaseImplicitStiffnessWindow;
    class mCaseImplicitVTFWindow;
    class mCaseImplicitSuperElementWindow;
    class mCaseImplicitNTFWindow;
    class mCaseImplicitFrequencyResponseWindow;
    class mCaseImplicitTransientWindow;
    class mCaseImplicitRandomVibrationWindow;
    class mCaseTimeSeriesHeatWizard;
    class mFrpCompMaterialWindow;

	class MMAINWINDOW_EXPORT mMenuWidget : public QMenuBar, public IRibbonHost
	{
		Q_OBJECT
	public:
		/*
		* 默认构造函数
		*/
		mMenuWidget(QMenuBar* parent = Q_NULLPTR);

		/*
		* 析构函数
		*/
		~mMenuWidget();

        /*
		* 通过 GeoServer 异步导入指定几何文件（最近打开/树视图等入口共用）；
		* timerParent 用于承载内部轮询 QTimer 的父对象，调用方传 this 即可。
		* 撤消：导入整体为一个撤消条目（ImportGeometry），导入进行中库侧
		* undo/redo 自动拒绝（historyBusy）。
		*/
		static void importGeometryViaServer(QObject* timerParent, const QString& fileName);

		/*
		* 导入几何模型的文件过滤器（菜单栏/树视图右键等入口共用）。
		* 按几何后端编译期切换：OCC→STEP/IGES/BREP/IDF；CGM→STEP/IGES +
		* 九种商用 CAD 直读（宏 MXGEO_BACKEND_* 由根 CMakeLists 依
		* RebuildGeometry 导出的后端路由发放）。
		*/
		static QString geoImportFileFilter();

        /*
        * 注册API
        */
        void registerAPI();

        /*
        * 删除注册
        */
        void unregisterAPI();

		/*
		* 中心视口
		*/
		void setCenterMainWindow(mCenterMainWindow* window);

		/*
		* 材料库
		*/
		void setMaterialWindow(mMaterialWindow* window);

        /*
        * 命令行导入求解
        */
        void cmdExecute(QStringList cmdList);

         /*
		* 点击主窗口标题栏时隐藏某些控件
		*/
		void hideTransformList();

		/*
		* 增加菜单
		*/
		void addMenu(const QString& menuNAme);

		/*
		* 新增选项组
		*/
		mGroupWidget* addGroup(QString menuName, QString txtGroupTitle) override;

		/*
		* 增加间距
		*/
		void addSpacerItem(QString menuName);

		/*
		* 设置QToolButton图标/悬浮提示，尺寸由按钮形态与主题档位管理
		*/
		void setQToolButton(QToolButton* btn, QString name, QString image, QString describe, Qt::ToolButtonStyle style= Qt::ToolButtonTextUnderIcon) override;

		/*
		* IRibbonHost：创建Ribbon页并注册页名（同名页已存在时直接复用）
		*/
		RibbonPage* createRibbonPage(const QString& name) override;

		/*
		* IRibbonHost：切换当前显示页（常驻页切固定页签，非常驻页占最右动态页签位）
		*/
		void switchToPage(const QString& name) override;

		/*
		* IRibbonHost：为控件安装悬浮提示
		*/
		void installTooltip(QWidget* target, const QString& name, const QString& image,
			const QString& describe, int msec = 200) override;

        /*
        * 判断指定工况-载荷/边界类型在菜单栏是否有对应按钮
        */
        bool hasCaseLoadBoundaryBtn(MxFunctions::CaseType caseType, MxFunctions::LoadBoundaryType lbType) const;

        /*
        * 判断指定工况-预定义场类型在菜单栏是否有对应按钮
        */
        bool hasCasePredefinedFieldBtn(MxFunctions::CaseType caseType, MxFunctions::PredefinedFieldType pfType) const;

		/*
		* 关闭主窗口时的退出确认流程（由mMechMainWindow::closeEvent调用）
		* 0 继续退出
		* 1 中止退出（用户取消或保存流程未完成）
		*/
		int isThreadCloseOrFileSave();

		/*
		* 即将丢弃当前数据的保存询问（退出、打开会话文件前共用）：
		* 询问是否保存mxses会话文件，"保存"则弹路径对话框并等待异步写完成。
		* 返回true=可以继续后续动作（已保存或不保存），false=用户取消（中止后续动作）
		*/
		bool confirmSaveMxsesBeforeDiscard(const QString& question);

	signals:
		void createSectionShapeSig(const QString& type);   // ← 写在这里
	private slots:
		//treeWidget过滤
		void onFilterTextChanged(const QString& text, QTreeWidget* tree);

		//零枚举，纯 sender 判断截面类型
		void onSectionButtonClicked();

		/*
		* 切换菜单栏窗口
		*/
		void switchMenuBarWindow(const QString& name);

		/*
		* 打开材料库
		*/
		void slot_openMaterial();

		/*
		* 打开自定义材料库
		*/
		void slot_openCustomMaterial();

        /*
        * 打开纤维增强复合材料
        */
        void slot_FrpCompMaterial();

		/*
		* 新建材料
		*/
		void slot_createMaterial();

		//创建工况
		void slot_createCase(MxFunctions::CaseType type);

		//创建NVH模态分析
		void slot_createNVHModelAnalysis();

        //创建NVH动刚度分析
        void slot_createNVHStiffnessAnalysis();

        //创建VTF的分析
        void slot_createNVHVTFAnalysis();

        //创建超单元的分析
        void slot_createNVHSuperElementAnalysis();

        //创建NTF的分析
        void slot_createNVHNTFAnalysis();

        //创建随机振动的分析
        void slot_createNVHRandomVibrationAnalysis();

        //创建通用频响的分析
        void slot_createNVHFreqResponseAnalysis();

        //创建瞬态的分析
        void slot_createNVHTransientAnalysis();

        //创建预定义场
        void slot_createPredefinedFiel(MxFunctions::PredefinedFieldType type);

        //创建旋转分析
        void slot_createRotateAnalyse(MxFunctions::RotateAnalysyType type);

        //创建载荷
        void slot_createLoad(MxFunctions::LoadBoundaryType type);

		//创建边界
		void slot_createBoundary(MxFunctions::LoadBoundaryType type);

		//创建连接
		void slot_createConnector(MxFunctions::ConnectorType type);

		//创建连接副
		void slot_createDeputy(MxFunctions::DeputyType type);

		//创建接触
		void slot_createContact(MxFunctions::ContactType type);

        //创建单元生死
        void slot_createElementSetCtrl();

		//创建集合
		void slot_createSet();

        void slot_createSet_withName(QString setName);

        //创建非结构体质量
        void slot_createNonStructuralQuality();

        //创建离散场
        void slot_createDiscreteFieldNoType();
        void slot_createDiscreteField(MxFunctions::DiscreteSpace spaceType);

        //创建分析场
        void slot_createAnalysisField();

       //结构优化
        void slot_createStrual();

        //流道优化
        void slot_createRunnerStrual();

		//创建剖面
		void slot_createSection(int type);

		//创建坐标系
		void slot_createCoordinate();

		/*
		* 个人开发面板（仅Debug）
		*/
		void slot_createMyPanel();

        //创建结构优化向导

		//创建函数
		void slot_createFunction(int type);

        //创建采样频率点
        void slot_createFREQ();

		//创建截面形状
		void slot_createSectionShape(int type);

        //提交计算
        void slot_startSolver();

        //命令行执行
        void slot_cmdExecute();
        /*
        * NVH曲线生成
        */
        void slot_NVHCurve();

		/*
		* 新建
		*/
		void slot_New();

		/*
		* 清空当前模型所有数据并更新界面
		*/
		void deleteViewportModel(bool isNew = false);

		/*
		* mpcomm 平台触发：导入指定几何/计算文件（OPEN_NODE/REFRESH_INPUTS 上下文带来，
		* 不回传 committed，由桥接回声抑制）
		*/
		void slot_mpcommImportGeometry(const QString& filePath);
		void slot_mpcommImportCalcFile(const QString& filePath);

		/*
		* mpcomm 平台触发：接收到上游场数据（role=mpif），执行点到点映射示例
		* （引路用，具体集成与面板由后续实现）
		*/
		void slot_mpcommImportMpif(const QString& filePath);

		/*
		* 打开「多体载荷导入」面板（菜单按钮主动唤出；可选预填 coupling 文件路径）
		*/
		void slot_openMotionLoadImport(const QString& filePath = QString());

		/*
		* mpif 点到点映射示例（引路用，供后续集成参考）：
		* 1) 解析文档打印全部 set 及变量清单；2) 取第一个 set + 第一个变量，
		*    用源 set 节点包围盒造 6 个目标点（域内 3 / 域外 2 / 精确命中 1）；
		* 3) Nearest 映射并打印每个目标点的映射值与状态。
		*/
		void runMpifMappingDemo(const QString& filePath);

		/*
		* mpcomm 平台触发：按 context.node.templateId 判定分析类型并创建对应工况
		* （静态结构/瞬态结构/模态/谐响应/响应谱/随机振动/屈曲分析）
		*/
		void slot_mpcommCreateCaseByTemplateId(const QString& templateId);

		/*
		* mpcomm：几何导入完成统一回报（内部辅助，挂在渲染完成信号上，
		* 先 disconnect 再 connect 防重复回报）
		*/
		void reportMpcommGeometryOnRenderFinished(const QString& filePath);

		void slotShowSectionShape();   // 点击“截面形状”按钮后触发

		/*
		* 重新调整符号大小
		*/
		void slot_resizeSymbol();

		/************************************************读写文件相关信号与槽****************************************************/

		/*
		* 打开
		*/
		void slot_openFile();

		/*
		* 保存
		*/
		bool slot_saveFile();

		/*
		* 另存为
		*/
		void slot_saveAsFile();

		/*
		* 打开会话文件(.mxses)
		*/
		void slot_openMxsesFile();

		/*
		* 保存会话文件(.mxses)
		*/
		void slot_saveMxsesFile();

		/*
		* 导入计算文件
		*/
		void slot_importMxFile();

		/*
		* 导入运动数据文件（Motion JSON，选择文件路径）
		*/
		void slot_importMotionJsonFile();

        /*
        * 导入计算文件
        */
        void importMxFile(const QString& fileName);

		/*
		* 导出计算文件
		*/
		void slot_exportMxFile();

		/*
		* 导出运动数据文件（Motion JSON）
		*/
		void slot_exportMotionJsonFile();
		/*
		* 导出几何文件
		*/
		void slot_exportGeoFile();

		/*
		* 导入几何文件
		*/
		void slot_importGeoFile();

        /*
        * 导入外部几何继承
        */
		void slot_importInheritanceGeoFile();

		/*
		* 导入网格文件
		*/
		void slot_importMeshFile();

		/*
		* 导出网格文件
		*/
		void slot_exportMeshFile();

		/*
		* 打开后处理结果文件：入口已信号化（clickOpenPostCloudFileSig/clickOpenPostNVHFileSig），
		* 执行方为mPostBrowserWindow::slot_openPostFile/slot_openPostNVHFile
		*/

        /*
         * 打开后处理NVH文件
         */
        void slot_openPostNVHFile();

        /*
        * 测试按钮
        */
        void slot_testButton();

		/*
		* 读写文件错误，输出错误信息
		*/
		void slot_ioFileError(MechCommon::IOFileErrorType type);

		/*
		* 读计算文件完成
		*/
		void slot_readSolveFinished();

        /*
        * 读网格面片文件完成
        */
        void slot_readMeshFinished();

		/*
		* 写计算文件完成
		*/
		void slot_writeSolveFinished();

		/*
		* 读mxcae文件完成
		*/
		void slot_readMxCaeFinished();

		/*
		* 写mxcae文件完成
		*/
		void slot_writeMxCaeFinished();

        /*
        * 写mf文件完成
        */
        void slot_writeMfFinished();

        /*
        * 读mf文件完成
        */
        void slot_readMfFinished();

		/*
		* 自动保存开关按钮切换
		*/
		void slot_toggleAutoSave(bool checked);

		/*
		* 自动保存间隔下拉菜单选择
		*/
		void slot_setAutoSaveInterval(QAction* action);

		/********************网格模块********************/
		void slotShowMeshMenu();

    private:
        // 直接建模模式切换：enter/exit + install/uninstall 预览显示组件
        void toggleDirectModelingMode(bool enter);

        /*
        * 集合创建防抖：每次创建会整体卸载/重挂信息面板并触发折叠框的
        * 延时刷新，快速连点会让多轮重挂交错，控件可能在脱离父窗口的
        * 空档期被显示成独立浮窗；故静置期内只保留最后一次请求
        */
        void deferCreateSet(const QString& setName);

        /*
     * 初始化弹窗
        */
        void slot_createNote();

        /*
        * 标注
        */
        void slot_createLabel();

        /*
        * 导出word报告
        */
        void slot_createWordReport();

        /*
        * 功能阵列
        */
        void slot_createParrayMimage();

        /*
        * 对比视频
        */
        void slot_createCompVideo();

        /*
        * 爆炸视图
        */
        void slot_createExplode();

        /*
        * 测量工具
        */
        void slot_createMeasureTool();

        /*
        * 路径结果显示
        */
        void slot_createResultPath();

    private:
    /*
     * 初始化弹窗
        */
        void initDialog();

        /*
        * 最近打开弹窗
        */
        void initRecentOpen();

        /*
        * 首选项Preferences弹窗
        */
        void initPreferences();

        /*
        * 关于MxSim.Mechanical弹窗
        */
        void initAbout();

        /*
        * 案例库弹窗
        */
        void initCaseLibrary();

        /*
        * 许可证弹窗
        */
        void initLicense();

		/*
		* 初始化菜单栏
		*/
		void initMenuBtn();

		/*
		* 初始化最近打开Menu
		*/
		void initRecentFile();

		/*
		* 初始化变量
		*/
		void initVariable();

		/*
		* mpcomm 平台接入初始化：信号接线 + 取回 OPEN_NODE 挂起的待导数据
		* （构造函数里调用）
		*/
		void initMpcomm();

		/*
		* mpcomm：取回桥接挂到 qApp 属性上的 OPEN_NODE 待导数据
		* （templateId/geometry/calcfile/mpif，取走即清，幂等）。
		* OPEN_NODE 早于构造时由 initMpcomm 直调；晚于构造时经
		* mpcommPendingContextChanged 信号触发。
		*/
		void consumePendingMpcommContext();

		/*
		* 主页菜单栏“文件”选项组中添加自动保存开关按钮（带间隔下拉菜单）
		*/
		void addAutoSaveBtn(mGroupWidget* group);

		/*
		* 启动后检测自动保存文件并弹窗询问是否恢复
		*/
		void checkAutoSaveRecovery();

		/*
		* 打开自动保存文件恢复上次工作内容（与slot_openFile流程一致）
		*/
		void restoreAutoSaveFile();

		/*
		* 主页菜单栏初始化
		*/
		void initHomeMenu();

		/*
		* 模型菜单栏初始化
		*/
		void initModelMenu();

        /*
        *材料库菜单初始化（新）
        */
        void initMaterialMenu();

		/*
		* 相互作用菜单初始化
		*/
		void initInteractionMenu();

        /*
        * 结构优化菜单初始化
        */
        void initStructuralMenu();

        /*
        * 流道优化菜单初始化
        */
        void initRunnerStructuralMenu();

        /*
        * 全局参数菜单初始化
        */
        void initGlobalParamMenu();

		/*
		* 截面属性菜单初始化
		*/
		void initPropertyMenu();

		/*
		* 集合菜单栏初始化
		*/
		void initSetMenu();

        /*
        * 场（离散场/分析场）
        */
        void initField();

		/*
		* 坐标系菜单栏初始化
		*/
		void initCoordinateMenu();

		/*
		* 显示菜单栏初始化
		*/
		void initDisplayMenu();

		/*
		* 选择菜单栏初始化
		*/
		void initSelectMenu();

		/*
		* 分析工况下拉菜单
		*/
		QMenu* getCaseMenu();
		QMenu* getSetMenu();

		/*
		* 相互作用下拉菜单
		*/
		QMenu* getInteractionMenu();

		/*
		* 单位制菜单
		*/
		QMenu* getUnitsMenu();

		/*
		* 截面形状菜单初始化
		*/
		void initSectionShapeMenu();

		/*
		* 函数菜单初始化
		*/
		void initFunctionMenu();

		/*
		* 多体动力学菜单初始化（阶段1：按钮接入mInfoWindow入口桩）
		*/
		void initMotionMenu();

		/*
		* MxAI菜单初始化
		*/
		void initMxAIMenu();

		/*
		*几何菜单初始化
		*/
		void initGeometry();

		/*
		*草图菜单初始化
		*/
		void initGeometrySketch();

		/*
		* 网格菜单初始化
		*/
		void initMeshMenu();

		/*
		* 分析工况菜单
		*/
		void initWorkingConditionMenu();

		/*
		* “分析工况”页签菜单（静态/动态/稳定性/传热分析及工况向导）
		*/
		void initAnalysisCaseMenu();

		/*
		* NVH分析菜单
		*/
		void initNVHAnalysisMenu();

        /*
        *材料菜单按钮 点击 调用面板的创建
        */
        void createMaterial(MxFunctions::MaterialType matType);

		//给坐标系转换列表添加行
		void addCoordinateTransform(const QString& type, const QString& value, int id = -1);

		/*
		* 初始化“视图”下拉按钮与快捷图标按钮（新建/保存/打开）
		*/
		void initViewMenu();

		/*
		* 按页签文字查找页签索引，不存在返回-1
		*/
		int findTabByText(const QString& name) const;

		/*
		* 页签文本→Ribbon页解析：Post模式下同名页优先解析到后处理独立页
		* （页键"显示#Post"/"选择#Post"，由PostWindow经IRibbonHost注册，
		* 与前处理同名页完全分离），其余模式及无后处理页时按原名直查
		*/
		RibbonPage* pageForTabText(const QString& title) const;

		/*
		* 保存菜单栏设置（常驻页签显隐、树菜单栏联动）到Settings.ini
		*/
		void saveMenuBarSettings();

		/*
		* 启动时恢复菜单栏设置：移除隐藏的常驻页签并同步“视图”菜单勾选状态
		*/
		void loadMenuBarSettings();

	public slots:
		/*
		* 切换菜单栏
		*/
		void slot_CurrentTabChanged(int index);

		/*
		* 模型树选中变化
		*/
        void slot_browsertreeSelectedChanged(const QList<mItemKey>& items, bool valid);

		/*
		* 转子动力学设置变化时刷新旋转速度/加速度按钮状态
		*/
		void slot_updateRotorButtons(int caseID);

		/*
		* 退出草图模式：移除"草图"动态页签（sketchExited 驱动，覆盖全部退出路径）
		*/
		void slot_sketchExited();

		/*
		* 切换视口
		*/
		void slot_subWindowActivated(int viewportID);

        /*
        * 用户点击/聚焦视口后的菜单模式跟随：补齐"点击的已是当前激活视口"场景——
        * 同ID激活被容器/管理器去重早退，currentViewportChanged 不发，
        * slot_subWindowActivated 的模式联动不执行（启动恢复错位、图表删除后
        * 静默转移激活等会留下旧模式）。同模式幂等；非渲染视口（图表卡片）跳过
        */
        void slot_viewportUserFocused(int viewportID);

		/*
		* 拾取完成
		*/
		void slot_finishedPick();

		/*
		* 显示/隐藏指定模式的常驻菜单页签（“视图-前处理/后处理/曲线常驻菜单栏”复选框触发）：
		* 属于当前模式时立即生效（按所属名单顺序插入/移除），非当前模式仅记录意图，
		* 模式切换时由setWorkMode按意图重建
		*/
		void setModeMenuVisible(MenuWorkMode mode, const QString& name, bool visible);

		/*
		* 计算指定模式下应显示的页签集合（按各自候选名单顺序过滤出勾选项）
		*/
		QStringList visibleTabsForMode(MenuWorkMode mode) const;

		/*
		* 指定模式的候选页签顺序名单
		*/
		QStringList candidateNamesForMode(MenuWorkMode mode) const;

		/*
		* 切换菜单栏工作模式：按模式重建TabBar页签集合（Post/Curve仅显示模式内页签，
		* Pre恢复常驻页签+前处理动态页），并落到默认页（Post→结果、Curve→曲线、Pre→主页）
		*/
		void setWorkMode(MenuWorkMode mode);

        /*
        * 按视口ID对齐菜单工作模式+树窗口：会话恢复收尾调用——恢复激活与当前激活
        * 同ID去重早退时无激活信号，currentViewportChanged→slot_subWindowActivated
        * 的模式联动缺席；与loadMenuBarSettings启动对齐同映射（workModeForViewportType）
        * 同组合（setWorkMode+syncTreeWindowsForMode），不含单位制/按钮启停等重联动
        */
        void alignModeToViewport(int viewportID);

        /*
        * 按激活视口类型启停后处理“结果”/“云图”页（仅云图视口ViewportPost3D激活时可用）与
        * “动画”页（仅动画视口<云图视口名>_Animation<N>激活时可用），无激活视口视为不可用；
        * 仅置灰页内功能（沿RibbonPage父子链禁用整页），页签保持可点击、可切换浏览。
        * 页签重建/插入路径（setWorkMode/setModeMenuVisible/switchMenuBarWindow临时插入）
        * 新建的页面对象默认enabled，插入后须重调本函数
        */
        void refreshPostMenuEnabledState();

		/*
		* 按工作模式同步左侧树（menu→tree方向统一收口）：Post→云图树、Curve→曲线树、
		* Pre→模型树；setWorkMode末尾与启动恢复路径调用
		*/
		void syncTreeWindowsForMode(MenuWorkMode mode);

		MenuWorkMode workMode() const { return _workMode; }

		/*
		* 单独持久化工作模式：仅由mMechMainWindow::closeEvent与saveLayout（DockingState）
		* 同时调用，两键同源同刻；setWorkMode不再即时写盘
		*/
		void saveWorkModeSetting();

	protected:
		//事件过滤器
		bool eventFilter(QObject* watched, QEvent* event) override;
	private:
		//最近打开menu
		QMenu* subMenu1;

		//集合创建防抖：定时器 + 待执行请求（空名 = 按拾取类型创建）
		QTimer* m_createSetDebounceTimer = nullptr;
		QString m_pendingCreateSetName;
		bool m_hasPendingCreateSet = false;

		//自动保存开关按钮（主页-文件选项组）
		QToolButton* _autoSaveBtn = nullptr;
		//数据库
		mRecentDataBase* db;
		//模型快照
		AutoScaledPNG* imageWidget;

		QButtonGroup groupBtn_;

		// 菜单栏的TabBar
		MyTabBar* m_menuTabBar_ = nullptr;

		// “视图”下拉按钮与其下拉菜单
		QToolButton* m_viewButton_ = nullptr;
		QMenu* m_viewMenu_ = nullptr;

		// 快捷图标按钮：新建/保存/打开
		QToolButton* m_quickNewBtn_ = nullptr;
		QToolButton* m_quickSaveBtn_ = nullptr;
		QToolButton* m_quickOpenBtn_ = nullptr;

		// 常驻（固定）菜单页签名单，“视图-前处理常驻菜单栏”按此勾选控制显隐；
		// 名单之外的菜单显示在最右侧动态页签位。
		// 形态开关：_ribbonForm 决定本名单取多体形态还是结构隐式形态（见 MenuRibbonForm）
		QStringList _residentMenuNames;

		// 菜单栏常驻形态：见 MechCommon::MenuRibbonForm（mMechGlobalSignals.h，
		// 与树形态共用同一解析入口 resolveRibbonForm）
		MechCommon::MenuRibbonForm _ribbonForm = MechCommon::MenuRibbonForm::Mech;
		// 结构隐式形态常驻名单（dev_Mech 合并后默认形态，原14页原样）
		QStringList _mechFormResidentMenus;
		// 多体形态常驻名单（主页/几何/五个多体功能区/求解/显示/选择）
		QStringList _motionFormResidentMenus;
		// 多体形态下彻底隐藏的隐式页（switchMenuBarWindow 守卫拦截，不落动态页签位）
		QStringList _motionFormHiddenMenus;
		MechCommon::MenuRibbonForm ribbonForm() const { return _ribbonForm; }

		// 当前菜单栏工作模式（跟随激活视口类型），决定TabBar上可见的页签集合
		MenuWorkMode _workMode = MenuWorkMode::Pre;

		// 后处理/曲线模式的候选页签名单（顺序即页签顺序，含全部可选页；主页三模式共享）
		QStringList _postMenuCandidates;   //后处理：主页/结果/云图/动画/注释/路径/流线图/.../分析报告
		QStringList _curveMenuCandidates;  //曲线：主页/曲线/报告

		// 各模式的默认勾选名单（首次运行/无配置时复选框的初始状态）
		QStringList _postModeTabs;   //后处理默认：主页/模型/结果/显示/选择（"云图"改临时页签，不默认勾选）
		QStringList _curveModeTabs;  //曲线默认：主页/曲线

		// 常驻菜单项勾选动作（菜单名→可勾QAction，按模式各一份；mUiMenu原生自绘行，勾选不关闭菜单），
		// 用于意图记录与启动恢复
		QMap<QString, QAction*> _residentMenuActions;
		QMap<QString, QAction*> _postMenuActions;
		QMap<QString, QAction*> _curveMenuActions;

		// 树菜单栏联动的三个模式动作（顺序与mMechGlobalSignals::TreeMenuLinkMode枚举值对应，
		// QActionGroup互斥单选），用于启动时恢复选中
		QList<QAction*> _treeLinkModeItems;
		// 堆叠窗口放置页面
		QStackedWidget* m_toolBarStackedWid_ = nullptr;
		// 保存菜单和对应窗口
		QMap<QString, RibbonPage*> m_StaWidMap;

		//转换文件
		mTransFileInfoDialog* m_transFileInfoDialog = nullptr;

		//中心视口
		mCenterMainWindow* m_centerMainWindow = nullptr;

		//材料库
		mMaterialWindow* m_materialWindow = nullptr;
		//自定义材料库
		mCustomMaterialWindow* m_customMaterialWindow = nullptr;
        //纤维增强复合
        mFrpCompMaterialWindow* m_FrpCompMaterialWindow = nullptr;

        //NVH模态分析
        mCaseImplicitModalWindow* m_caseImplicitModalWindow = nullptr;
        //NVH动刚度分析
        mCaseImplicitStiffnessWindow* m_caseImplicitStiffnessWindow = nullptr;
        //VTF分析
        mCaseImplicitVTFWindow* m_caseImplicitVTFWindow = nullptr;
        //超单元分析
        mCaseImplicitSuperElementWindow* m_caseImplicitSuperElementWindow = nullptr;
        //NTF分析
        mCaseImplicitNTFWindow* m_caseImplicitNTFWindow = nullptr;
        //通用频响分析
        mCaseImplicitFrequencyResponseWindow* m_caseImplicitFreqResponseWindow = nullptr;
        //瞬态分析
        mCaseImplicitTransientWindow* m_caseImplicitTransientWindow = nullptr;
        //随机振动分析
        mCaseImplicitRandomVibrationWindow* m_caseImplicitRandomVibrationWindow = nullptr;

        //时序热分析工况向导（分析工况页“工况向导”组入口，复用实例）
        mCaseTimeSeriesHeatWizard* m_timeSeriesHeatWizard = nullptr;

		//通用btn
		QHash<MxFunctions::MaterialType, QToolButton*>propBtns;
		//材料btn
		QHash<MxFunctions::MaterialType, QToolButton*>matBtns;

		// 工况-预定义场类型按钮映射
		QHash<MxFunctions::CaseType, QHash<MxFunctions::PredefinedFieldType, QToolButton*>> casePredefinedFieldBtns;
		// 工况-载荷/边界类型按钮映射
		QHash<MxFunctions::CaseType, QHash<MxFunctions::LoadBoundaryType, QToolButton*>> caseLoadBoundaryBtns;
		// 工况-符号组按钮映射（目前仅"重新调整"）
		QHash<MxFunctions::CaseType, QToolButton*> caseSymbolBtns;

		// 分析工况菜单栏选项卡
		QList<mGroupWidget*> _workingConditionGW;

		      //转移到材料面板文件
		//当前已选属性
		QSet<MxFunctions::MaterialType>curProps;
		//当前可选属性
		QSet<MxFunctions::MaterialType>allowProps;
		//当前材料
		MxFunctions::MaterialType curMat_;

		//主页菜单栏选项卡
		QList<std::shared_ptr<mGroupWidget>> _homeGW;

		//显示菜单栏选项卡
		QList<std::shared_ptr<mGroupWidget>> _displayGW;

		//选择菜单栏选项卡
		QList<std::shared_ptr<mGroupWidget>> _selectGW;

		//截面属性菜单栏选项卡
		QList<std::shared_ptr<mGroupWidget>> _propertyGW;

		//集合菜单栏选项卡
		QList<std::shared_ptr<mGroupWidget>> _setGW;

        //场菜单栏选项卡
        QList<std::shared_ptr<mGroupWidget>> _fieldGW;

		//坐标系菜单栏选项卡
		QList<std::shared_ptr<mGroupWidget>> _coordinateGW;
		QListWidget* _transformList;//坐标系变换列表

		//几何菜单栏选项卡
		QList<std::shared_ptr<mGroupWidget>> _geometryGW;

		//函数菜单栏选项卡
		QList<std::shared_ptr<mGroupWidget>> _functionGW;

		//草图菜单栏选项卡
		QList<std::shared_ptr<mGroupWidget>> _geometrySketchGW;

		//NVH分析选项卡
		QList<std::shared_ptr<mGroupWidget>>		_nvhAnalysisGW;

		//最近打开弹窗
		mFramelessDialog* recentlyOpenedDialog;
		QTableWidget* recentTable;
		//首选项弹窗
		QDialog* preferencesDialog;
		//关于MxSim.Mechanical弹窗
		mFramelessDialog* aboutMechanicalDialog;
		QVector3D curColor0 = QVector3D(225, 238, 250);
		QVector3D curColor1 = QVector3D(150, 199, 237);
		QVector3D curColor2 = QVector3D(225, 238, 250);
		//案例库弹窗
		mFramelessDialog* caseLibraryDialog;
		//许可证弹窗
		mFramelessDialog* licenseDialog;

		//拾取完成的连接
		QMetaObject::Connection _connPick;

		//显示模式
		int m_showFuntion;

		//工程案例
		QTreeWidget* case_tree;

		//pdf阅读器
		QWebEngineView* view;

		//关于迈曦
		QWebEngineView* viewAbout = nullptr;

		//最近打开数量/时间
		int recentflag = 0;
		int recentNum = 30;
		int recentDay = 30;

        // 用于后面分析工况的引用
        QToolButton* btn1Force = nullptr;
        QToolButton* btn2Torque = nullptr;
        QToolButton* btn3Pressure = nullptr;
        QToolButton* btn4Gravity = nullptr;
        QToolButton* btn5Bolt = nullptr;
        QToolButton* btn6RotatePower = nullptr;
        QToolButton* btn7Surface = nullptr;
        QToolButton* btn8Edgeface = nullptr;
        QToolButton* btn1Fixed = nullptr;
        QToolButton* btn4Displacement = nullptr;
        QToolButton* btnSpeed = nullptr;
        QToolButton* btnAcc = nullptr;
        QToolButton* btn6Temp = nullptr;
        QToolButton* btn7Virtual = nullptr;
        QToolButton* btn8Super = nullptr;
        QToolButton* btnSubMove = nullptr;
        QToolButton* btnSubTemp = nullptr;
        QToolButton* btnRotSpeed = nullptr;
        QToolButton* btnRotAcc = nullptr;
        QToolButton* btnResizeSymbol = nullptr;
        QToolButton* btnResizeSymbolLoad = nullptr;
        QToolButton* btn1HeatFlux = nullptr;
        QToolButton* btn2Radiation = nullptr;
        QToolButton* btn3Convection = nullptr;
        QToolButton* btn1Transient = nullptr;
        QToolButton* btn2FreqDomain = nullptr;
        QToolButton* btn3Combined = nullptr;
        QToolButton* btn4Spectrum = nullptr;
        QToolButton* btn5Acoustic = nullptr;
        QToolButton* btn6AcousticFreq = nullptr;
        QToolButton* btn1InitTemp = nullptr;
        QToolButton* btn2TempLoad = nullptr;
	};

}