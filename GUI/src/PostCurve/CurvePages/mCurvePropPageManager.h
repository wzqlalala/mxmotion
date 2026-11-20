#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 属性页注册表（单例）
//
// 职责：CurveNodeType -> 属性页工厂 的注册与分发（UI设计图02：选中不同
// 类型节点切换不同属性页）。页面按宿主分两族注册（UI设计图03.1：
// 参数栏与属性编辑器弹窗是两套内容，不是同一个页面的两种摆法）：
//
//   1. 参数栏页面族：registerPage / takeParamBarPage(nodeId)
//      按节点类型缓存【单实例】，切换同类型节点复用（只重绑 nodeId）。
//      mInfoWindow::editCurveNode 调用后 setMainWidget 挂载——
//      参数栏是单页面容器，同一时刻只显示一个页面。
//
//   2. 属性编辑器页面族：registerEditorPage / showEditorPopup(nodeId)
//      双击树节点触发，mPropertyEditorPopup 托管【独立实例】；
//      与参数栏实例互不影响，都经模型信号保持同步。
//      【回退策略】未注册 editor 页面的类型回退到参数栏页面族
//      （迁移期保证未迁移类型行为不变，逐类型替换完毕后移除回退）。
//
// 扩展新页面：registerPage / registerEditorPage(类型, 工厂lambda)。
// 默认注册表见 registerDefaultPages()（Header/Footer 复用图表页）。
// =============================================================================

#include "../PostCurveGlobal.h"
#include "../CurveCore/CurveDefine.h"

//QT
#include <QObject>
#include <QHash>
#include <QPointer>
#include <QPoint>
#include <functional>

namespace PostCurve
{
	class mCurvePropPageBase;
	class mPropertyEditorPopup;

	class POSTCURVE_EXPORT mCurvePropPageManager : public QObject
	{
		Q_OBJECT
	public:
		static mCurvePropPageManager* getInstance();

		//页面工厂：传入父部件返回新页面实例（无父，由宿主接管）
		using PageFactory = std::function<mCurvePropPageBase* (QWidget*)>;

	//注册参数栏页面工厂（后续新增节点类型/替换页面在此扩展；
	//默认注册表在构造时由 registerDefaultPages 完成）
	void registerPage(CurveNodeType type, PageFactory factory);

	//注册属性编辑器（弹窗）专属页面工厂；未注册的类型由
	//showEditorPopup 回退到参数栏页面族（迁移期策略）
	void registerEditorPage(CurveNodeType type, PageFactory factory);

		//参数栏宿主：按节点类型取缓存实例并绑定节点id。
		//（mInfoWindow::editCurveNode 调用；返回页面已 setNodeId，
		//  根节点/未注册类型返回 nullptr）
		QWidget* takeParamBarPage(int nodeId);

		//浮动属性编辑器（默认位置显示，UI设计图03.1）：
		//按需创建弹窗与页面（同类型换节点只重绑id，类型变化才重建），
		//标题为"类型 - 节点名"，show + raise + activateWindow
		void showEditorPopup(int nodeId);

		//同上，但先落位到 globalPos 再显示（设计图03.1：图上双击曲线/
		//图例项、右键"编辑"时弹在请求视窗左上角）
		void showEditorPopupAt(int nodeId, const QPoint& globalPos);

	private:
		mCurvePropPageManager(QObject* parent = nullptr);
		~mCurvePropPageManager() override;
		mCurvePropPageManager(const mCurvePropPageManager&) = delete;
		mCurvePropPageManager& operator=(const mCurvePropPageManager&) = delete;

	//默认页面注册（图表/曲线/坐标轴x3/图例/基准线/注释，Header/Footer 归入图表页）
	void registerDefaultPages();
	//取类型的参数栏工厂（未注册返回空 std::function）
	PageFactory factoryFor(CurveNodeType type) const;
	//弹窗准备（页面就绪+节点绑定+host），供 showEditorPopup / showEditorPopupAt 复用；
	//节点不可编辑（根节点/未注册类型）返回 false
	bool buildEditorPopup(int nodeId);

	static mCurvePropPageManager* _instance;                     //单例
	QHash<int, PageFactory> _factories;                          //CurveNodeType -> 参数栏页面工厂
	QHash<int, PageFactory> _editorFactories;                    //CurveNodeType -> 弹窗专属页面工厂（未注册回退 _factories）
		QHash<int, QPointer<mCurvePropPageBase>> _paramBarPages;     //CurveNodeType -> 参数栏缓存页（单实例）
		QPointer<mPropertyEditorPopup> _popup;                       //浮动编辑器弹窗（懒创建）
		QPointer<mCurvePropPageBase> _popupPage;                     //浮动编辑器当前页面（独立实例）
		int _popupPageType = -1;                                     //浮动编辑器当前页面类型（类型变化才换页）
	};
}
