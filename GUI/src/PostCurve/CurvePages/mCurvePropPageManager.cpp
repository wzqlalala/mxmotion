#include "mCurvePropPageManager.h"
#include "mCurvePropPageBase.h"
#include "mCurvePropPages.h"
#include "mAxis3DPropPage.h"
#include "mCurve3DPropPage.h"
#include "mAnnotation3DPropPage.h"
#include "mCurveEditorPages.h"
#include "mPropertyEditorPopup.h"
#include "../CurveCore/CurveDocument.h"
#include "../CurveCore/mCurveDocController.h"

#include <QCoreApplication>
#include <QApplication>

namespace PostCurve
{
	mCurvePropPageManager* mCurvePropPageManager::_instance = nullptr;

	mCurvePropPageManager* mCurvePropPageManager::getInstance()
	{
		if (_instance == nullptr)
			_instance = new mCurvePropPageManager;
		return _instance;
	}

	mCurvePropPageManager::mCurvePropPageManager(QObject* parent)
		: QObject(parent)
	{
		registerDefaultPages();

		//弹窗页绑定的节点被删除：关闭弹窗并解绑（防悬空 id，
		//loadFromModel 对不存在的节点有空值守卫，解绑本身无副作用）
		connect(mCurveDocController::getInstance(), &mCurveDocController::sig_nodeRemoved,
			this, [this](int nodeId)
		{
			if (_popup && _popupPage && _popupPage->nodeId() == nodeId)
			{
				_popup->hide();
				_popupPage->setNodeId(-1);
			}
		});
	}

	mCurvePropPageManager::~mCurvePropPageManager()
	{
	}

	void mCurvePropPageManager::registerDefaultPages()
	{
		registerPage(CurveNodeType::Page, [](QWidget* p) { return new mPagePropPage(p); });
		registerPage(CurveNodeType::Chart, [](QWidget* p) { return new mChartPropPage(p); });
		registerPage(CurveNodeType::Curve, [](QWidget* p) { return new mCurveSeriesPropPage(p); });
		registerPage(CurveNodeType::CurveXData, [](QWidget* p) { return new mCurveDataPropPage(false, p); });  //X数据（设计图03.2）
		registerPage(CurveNodeType::CurveYData, [](QWidget* p) { return new mCurveDataPropPage(true, p); });   //Y数据（含坐标轴行；复数曲线 ym 壳共用本页，该形态整行隐藏）
		registerPage(CurveNodeType::CurveY2Data, [](QWidget* p) { return new mCurveDataPropPage(2, p); });     //复数下面板数据"yp"（无坐标轴行）
		registerPage(CurveNodeType::XAxis, [](QWidget* p) { return new mAxisPropPage(p); });
		registerPage(CurveNodeType::YAxis, [](QWidget* p) { return new mAxisPropPage(p); });
		registerPage(CurveNodeType::YAxis2, [](QWidget* p) { return new mAxisPropPage(p); });
		registerPage(CurveNodeType::XAxis2, [](QWidget* p) { return new mAxisPropPage(p); });
		registerPage(CurveNodeType::Legend, [](QWidget* p) { return new mLegendPropPage(p); });
		registerPage(CurveNodeType::Header, [](QWidget* p) { return new mHeaderPropPage(p); });   //页眉/页脚独立面板（设计图01.1）
		registerPage(CurveNodeType::Footer, [](QWidget* p) { return new mFooterPropPage(p); });
		registerPage(CurveNodeType::Baseline, [](QWidget* p) { return new mBaselinePropPage(p); });
		registerPage(CurveNodeType::Annotation, [](QWidget* p) { return new mAnnotationPropPage(p); });
		//三维图 x/y/z 轴壳（R28 入树固定子）：三壳共用一页三实例，构造参轴
		//下标 0=X 1=Y 2=Z 区分（仿 mCurveDataPropPage 先例）。弹窗族不注册：
		//壳无双击/右键弹窗入口，showEditorPopup 回退参数栏页=预期行为
		registerPage(CurveNodeType::AxisX3DData, [](QWidget* p) { return new mAxis3DPropPage(0, p); });
		registerPage(CurveNodeType::AxisY3DData, [](QWidget* p) { return new mAxis3DPropPage(1, p); });
		registerPage(CurveNodeType::AxisZ3DData, [](QWidget* p) { return new mAxis3DPropPage(2, p); });

		//属性编辑器弹窗专属页面族（设计图03.1/05~08/15，曲线/坐标轴/图例/注释；
		//其余类型未注册，showEditorPopup 回退到参数栏页面）
		registerEditorPage(CurveNodeType::Curve, [](QWidget* p) { return new mCurveEditorPropPage(p); });
		registerEditorPage(CurveNodeType::Annotation, [](QWidget* p) { return new mAnnotationEditorPropPage(p); });
		//三轴共用一页（对齐参数栏 mAxisPropPage 先例）；入口为双击纵轴标题（设计图05）
		registerEditorPage(CurveNodeType::XAxis, [](QWidget* p) { return new mAxisEditorPropPage(p); });
		registerEditorPage(CurveNodeType::YAxis, [](QWidget* p) { return new mAxisEditorPropPage(p); });
		registerEditorPage(CurveNodeType::YAxis2, [](QWidget* p) { return new mAxisEditorPropPage(p); });
		registerEditorPage(CurveNodeType::XAxis2, [](QWidget* p) { return new mAxisEditorPropPage(p); });
		//图例编辑页（设计图08）；入口为双击图例空白区域/右键图例"编辑"
		registerEditorPage(CurveNodeType::Legend, [](QWidget* p) { return new mLegendEditorPropPage(p); });
		//图表编辑页（参考软件图表属性编辑器）；入口为双击图表空白区域/右键空白"编辑"
		registerEditorPage(CurveNodeType::Chart, [](QWidget* p) { return new mChartEditorPropPage(p); });
		//基准线编辑页（设计图14，图4）；入口为双击基准线/右键基准线"编辑"
		registerEditorPage(CurveNodeType::Baseline, [](QWidget* p) { return new mBaselineEditorPropPage(p); });
	}

	void mCurvePropPageManager::registerPage(CurveNodeType type, PageFactory factory)
	{
		_factories.insert(static_cast<int>(type), factory);
	}

	void mCurvePropPageManager::registerEditorPage(CurveNodeType type, PageFactory factory)
	{
		_editorFactories.insert(static_cast<int>(type), factory);
	}

	mCurvePropPageManager::PageFactory mCurvePropPageManager::factoryFor(CurveNodeType type) const
	{
		return _factories.value(static_cast<int>(type));
	}

	QWidget* mCurvePropPageManager::takeParamBarPage(int nodeId)
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		if (!doc->node(nodeId))
			return nullptr;

		int typeInt = static_cast<int>(doc->nodeType(nodeId));
		if (typeInt == static_cast<int>(CurveNodeType::Project))
			return nullptr;   //根节点无属性页

		//关闭销毁阶段不再创建/回填属性页：此时新建 QWidget 会命中
		//"No style available without QApplication" 断言导致退出崩溃
		if (QCoreApplication::closingDown())
			return nullptr;

		//三维图曲线节点分流（R33）：Curve 类型二维/三维共用同一注册工厂槽
		//（registerPage(Curve, 二维页) 保持不动），按父图表 chartType 区分——
		//chartType==5 路由到三维专属页 mCurve3DPropPage。缓存键方案：二维页
		//沿用正 typeInt 缓存槽不动；三维页取负键 -typeInt 独立缓存槽（同类型
		//正负键互不覆盖）——二维↔三维曲线节点来回切换时两页各自缓存、回显
		//互不串值。守卫：chartNodeId 查不到（≤0）或图表节点缺失/非 Chart 时
		//按二维回落（异常树形态走既有页面，不因分流崩溃）
		//
		//三维图注释节点分流（R34）：Annotation 类型同款方案——二维注册工厂槽
		//与正键缓存槽不动（2D 注释页零回归），父图表 chartType==5 路由到三维
		//注释管理页 mAnnotation3DPropPage（负键 -typeInt 独立缓存槽，守卫与
		//回落口径逐条对齐 R33）。弹窗族不注册三维页：三维画布无双击/右键弹窗
		//入口，showEditorPopup 对注释仍回退既有 2D 注释弹窗页（行为不变）
		if (typeInt == static_cast<int>(CurveNodeType::Curve))
		{
			const int chartId = doc->chartNodeId(nodeId);
			const CurveNode* chart = (chartId > 0) ? doc->node(chartId) : nullptr;
			if (chart && chart->type == CurveNodeType::Chart
				&& chart->chart.chartType == 5)
				typeInt = -typeInt;   //三维曲线：负变体键独立缓存槽
		}
		else if (typeInt == static_cast<int>(CurveNodeType::Annotation))
		{
			const int chartId = doc->chartNodeId(nodeId);
			const CurveNode* chart = (chartId > 0) ? doc->node(chartId) : nullptr;
			if (chart && chart->type == CurveNodeType::Chart
				&& chart->chart.chartType == 5)
				typeInt = -typeInt;   //三维注释：负变体键独立缓存槽
		}

		//按类型缓存参数栏实例（切换节点复用；负键=三维图语境专属页（R33 三维
		//曲线页/R34 三维注释页），工厂不走注册表——三维页是节点在三维图语境下的
		//形态，非独立节点类型，注册表 NodeType→工厂 语义保持纯净）
		mCurvePropPageBase* page = _paramBarPages.value(typeInt);
		if (!page)
		{
			PageFactory f;
			if (typeInt < 0)
			{
				f = (typeInt == -static_cast<int>(CurveNodeType::Curve))
					? PageFactory{ [](QWidget* p) { return new mCurve3DPropPage(p); } }
					: PageFactory{ [](QWidget* p) { return new mAnnotation3DPropPage(p); } };
			}
			else
				f = _factories.value(typeInt);
			if (!f)
				return nullptr;
			page = f(nullptr);
			_paramBarPages.insert(typeInt, page);
		}
		page->setNodeId(nodeId);
		return page;
	}

	void mCurvePropPageManager::showEditorPopup(int nodeId)
	{
		if (!buildEditorPopup(nodeId))
			return;
		_popup->show();
		_popup->raise();
		_popup->activateWindow();
	}

	void mCurvePropPageManager::showEditorPopupAt(int nodeId, const QPoint& globalPos)
	{
		if (!buildEditorPopup(nodeId))
			return;
		_popup->move(globalPos);   //先落位再显示，避免先在旧位置闪现
		_popup->show();
		_popup->raise();
		_popup->activateWindow();
	}

	bool mCurvePropPageManager::buildEditorPopup(int nodeId)
	{
		CurveDocument* doc = mCurveDocController::getInstance()->document();
		CurveNode* n = doc->node(nodeId);
		if (!n || n->type == CurveNodeType::Project)
			return false;

		//弹窗专属页面优先；未注册类型回退参数栏页面（迁移期保证行为不变）
		int typeInt = static_cast<int>(n->type);
		PageFactory f = _editorFactories.value(typeInt);
		if (!f)
			f = _factories.value(typeInt);
		if (!f)
			return false;

		if (!_popup)
		{
			//以当前激活窗口（通常为主窗口）为父：有父的窗口不算"主窗口"，
			//不参与 quitOnLastWindowClosed——关闭主窗口程序即退出、弹窗随父
			//销毁；无父时主窗口关闭后弹窗会作为独立窗口滞留，进程不退出。
			//无激活窗口时退化为无父顶层窗口（✕ 走 close 仍能正确退出）
			_popup = new mPropertyEditorPopup(QApplication::activeWindow());
		}

		//页面类型变化时重建（同类型换节点只重绑id）
		if (_popupPageType != typeInt || !_popupPage)
		{
			if (_popupPage)
				_popupPage->deleteLater();
			_popupPage = f(nullptr);
			_popupPageType = typeInt;
		}

		//绑定/重绑目标节点（首次与同类型换节点都走这里）：
		//触发 loadFromModel 回填控件，并启用 commit——漏掉这步则页面
		//恒为未绑定态（_nodeId=-1），弹窗内一切编辑都会被静默丢弃
		_popupPage->setNodeId(nodeId);

		_popup->host(_popupPage);
		return true;
	}
}
