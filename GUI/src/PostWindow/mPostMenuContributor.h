#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mpostwindow_global.h"

#include <QPointer>   //3D曲线工具页缓存（R37）
#include <QWidget>

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;


class QToolButton;

namespace MechMainWindow
{
	class IRibbonHost;
}

namespace PostWindow
{
	class mPostAnimationMenuWidget;
	class mPostCloudMenuWidget;

	/*
	* 后处理菜单贡献者（PostWindow 模块；持有全部后处理Ribbon页构建逻辑，
	* 经共享层IRibbonHost接口注册到宿主菜单栏，注册调用在组合根mAppInitializer）
	*
	* 迁移地图（源 mMenuWidget.cpp，行号为迁移前基线）：
	*   initNVHCurveMenu   6089-6141  曲线页（原页名"NVH曲线"，已迁入 buildNVHCurvePage）
	*   initResultMenu     6143-6233  结果页（已迁入 buildResultPage）
	*   init3DResultMenu   6235-6822  云图页（已迁入 buildCloudPage，含_cloudMW及过滤启停联动）
	*   initAnimationMenu  6823-7021  动画页（已迁入 buildAnimationPage）
	*   initCurveMenu      7022-7137  后处理-曲线页（原页名"曲线"，已迁入 buildCurvePage；
	*                                  其mInfoWindow/mCurveWindow直调已信号化）
	*   initNoteMenu       7202-7241  注释页（已迁入 buildNotePage；该页功能当前禁用）
	*   空壳页x9           7139-7201  已迁入 buildShellPages（路径/流线图/切面图/等值面图/
	*                                  线性叠加/分析报告/矢量图/分区显示/后处理-集合）
	*   postMenu1子菜单    1684-1688  主页文件菜单"后处理"入口（云图结果/曲线结果）——待迁（最后一项）
	*   slot_openPostFile  11614-11649 打开结果文件（依赖mViewTreeFunction/mPostBrowserWindow）——待迁
	*   slot_exportFilterSTL 11145-11193 已迁入 exportFilterSTL
	* 明确不迁（易误判）：slot_browsertreeSelectedChanged(7243,纯前处理)、
	*   initNVHAnalysisMenu(9319,前处理工况)、isPostModel守卫x14(原地保留)
	* 模块边界：曲线树/曲线窗口等曲线功能实体属PostCurve模块（与后处理平级），不迁入后处理。
	*/
	class MPOSTWINDOW_EXPORT mPostMenuContributor
	{
	public:
		/*
		* 全局单例（无QObject父对象，纯构建逻辑）
		*/
		static mPostMenuContributor* getInstance();

		/*
		* 向Ribbon宿主注册全部后处理菜单页
		* 调用点：mMenuWidget::initMenuBtn（替代原initCurveMenu/initNoteMenu等调用）
		*/
		void registerMenus(IRibbonHost* host);

	private:
		mPostMenuContributor() = default;
		~mPostMenuContributor() = default;
		mPostMenuContributor(const mPostMenuContributor&) = delete;
		mPostMenuContributor& operator=(const mPostMenuContributor&) = delete;

		/*
		* "云图"页：新建/切换/自由节点/查询/过滤（自 init3DResultMenu 迁入，
		* 含 _cloudMW 控件包与过滤启停联动）
		*/
		void buildCloudPage(IRibbonHost* host);

/*
* "结果"页：按原型 V1.2 最终态整理（2026-11-09）——结果图形/变形控制/空间显示/
* 结果分析/报告与输出/工况 六原型组按序；未实现的原型按钮点击提示待接入。
* 原"待定"组（曲线/路径结果显示/标注/NVH贡献量三钮）已从菜单栏移除（2026-11-10）
*/
	void buildResultPage(IRibbonHost* host);

		/*
		* "动画"页：帧数控制/动画控制/输出，含播放启停状态机（自 initAnimationMenu 迁入）
		*/
		void buildAnimationPage(IRibbonHost* host);

		/*
		* "曲线"页（原"NVH曲线"）：新建曲线入口/曲线占位/标注开关（自 initNVHCurveMenu 迁入）
		*/
		void buildNVHCurvePage(IRibbonHost* host);

		/*
		* "3D曲线"页：五组九钮（R37 重组对齐设计稿 ref-ribbon-3dcurve-tab-full.png）——
		* 文件[新建]/曲线[定义曲线, 瀑布图]/格式[曲线样式, 坐标轴]/查询[数值, 数据查询]/
		* 标注[基准, 注释]。六新钮 chartType==5 启停联动（R34 注释钮同款）；五工具钮
		* 点击挂载 CurvePages 工具页（本类懒创建缓存复用），坐标轴钮选中首曲线 x 壳
		* 走既有轴页链路，新建/定义曲线/注释三钮行为原样
		*/
		void build3DCurvePage(IRibbonHost* host);

		/*
		* "报告"页：报告模板（保存模板/模板编辑，脚手架待接入）与报告输出
		* （弹 mExportReportDlg 导出曲线报告）
		*/
		void buildReportPage(IRibbonHost* host);

		/*
		* "后处理-曲线"页（原"曲线"）：曲线显隐/表格/样式/路径（自 initCurveMenu 迁入）
		*/
		void buildCurvePage(IRibbonHost* host);

		/*
		* "注释"页：二维线/箭头/文本/Log（自 initNoteMenu 迁入）
		*/
		void buildNotePage(IRibbonHost* host);

	/*
	* 9个空壳页注册（路径/流线图/切面图/等值面图/线性叠加/分析报告/矢量图/分区显示/后处理-集合，
	* 页内容为空，仅为动态页签切换注册页名）
	*/
	void buildShellPages(IRibbonHost* host);

	/*
	* "显示"页（后处理独立页，页键"显示#Post"，页签文本"显示"与前处理同名页分离）：
	* 视图（步骤/旋转/平移/缩放）、实体化（梁/杆）、模型（隐藏当前/隐藏其他/反转显示/显示全部），
	* 功能槽在 mPostDisplayMenuFunction（自前处理显示菜单的后处理分支分离移植）
	*/
	void buildDisplayPage(IRibbonHost* host);

	/*
	* "选择"页（后处理独立页，页键"选择#Post"）：网格（按照Id选择节点/单元+ID输入框）、
	* 批量操作（反向/清空），功能槽在 mPostSelectMenuFunction
	*/
	void buildSelectPage(IRibbonHost* host);

	/*
	* "模型"页（后处理独立页，页键"模型#Post"，按V1.2原型）：集合（部件集/节点集/
	* 单元边集/单元面集/单元集，按类型创建集合，经集合面板承载；单元边集面板暂无
	* 对应类型由接收槽提示开发中）、坐标系（直角/柱/球，占位待接入）——按钮只发
	* 全局信号 clickPostSetCreateSig/clickPostCoordinateCreateSig，功能在树侧槽实现
	*/
	void buildModelPage(IRibbonHost* host);

		//TODO 以下按迁移地图逐页迁入（迁入时在此追加声明并删除mMenuWidget中对应init/slot）：
		//void buildPostFileEntry(IRibbonHost* host);  //postMenu1后处理子菜单+打开结果（依赖mViewTreeFunction/mPostBrowserWindow）

		/*
		* 导出过滤后模型的STL（自 mMenuWidget::slot_exportFilterSTL 迁入，
		* 文件对话框与默认名逻辑在此，实际导出经 clickExportFilterSTLSig 由mInfoWindow执行）
		*/
		void exportFilterSTL();

		/*
		* 3D曲线工具页挂载统一入口（R37，挂载序列照 mPostInfoHub::createCurve 先例）：
		* unEditInfoWindow → activateDock("Info") → 窄宽压缩（mountToHost 同款）→
		* setMainWidget(page)
		*/
		void mount3DToolPage(QWidget* page);

	private:
		QToolButton* _curveBtn = nullptr;   //曲线显隐下拉按钮（原mMenuWidget::_curveBtn）
		//控件包是QWidget，析构必须早于QApplication：挂Ribbon宿主控件（Qt父子所有权，
		//随主窗口销毁），此处只存裸指针不加引用——本单例活到进程退出期（qApp已析构），
		//此前shared_ptr在退出期析构QWidget，栈内踩已销毁的Qt全局状态导致崩溃
		mPostAnimationMenuWidget* _animationMW = nullptr;   //动画页控件包（原mMenuWidget::_animationMW）
		mPostCloudMenuWidget* _cloudMW = nullptr;           //云图页控件包（原mMenuWidget::_cloudMW）

		//"3D曲线"页五工具页缓存（R37）：QPointer 懒创建复用——mInfoWindow::
		//setMainWidget 换页时对旧页只 hide+setParent(nullptr) 不删除，缓存保活是
		//硬要求（重复点击重新挂载=同实例重挂）；QPointer 自动兜底被 dock 销毁的
		//情形。存基类指针，具体类型只在 .cpp 构造/调参处可见（头文件不引 PostCurve 头）
		QPointer<QWidget> _waterfall3DPage;   //瀑布图工具页
		QPointer<QWidget> _curveStyle3DPage;  //曲线样式工具页
		QPointer<QWidget> _pointSeek3DPage;   //数值工具页
		QPointer<QWidget> _sectionQuery3DPage; //数据查询工具页
		QPointer<QWidget> _datum3DPage;       //基准工具页
	};
}
