#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 属性页基类
//
// 一个属性页绑定一个节点id，实现"控件 <-> 模型"的双向同步闭环：
//
//   模型 -> 控件（回填）：
//     setNodeId(nodeId) 切换目标节点并调用 loadFromModel()；
//     模型属性变化（含撤销/重做/其他视图修改，经 sig_nodePropertyChanged）
//     时自动重新 loadFromModel()；
//
//   控件 -> 模型（提交）：
//     子类把控件信号 connect 到 commit(属性名, 值)；
//     commit 经控制器走命令栈（可撤销），随后模型信号又触发回填，
//     因控件赋同值不再发信号，链路自然收敛，不会震荡。
//
// 防回环机制：_loading 在 loadFromModel() 期间置位，屏蔽 commit——
// 程序赋值（回填）不会被误认为用户修改。
//
// 宿主复用：同一套属性页部件可宿主在参数栏（mInfoWindow，按类型缓存单实例，
// 切换节点复用）或浮动属性编辑器（独立实例）中，两个实例经上述信号机制
// 自动保持一致。
//
// 子类编写约定：
//   1. buildUi() 创建控件（推荐用 mCurveFoldSection 分组）；
//   2. 实现 loadFromModel()：从控制器 document() 读 nodeProperty 回填控件；
//   3. 每个控件信号 -> commit(CurveProp::kXxx, 值)。
//
// 基类说明：继承 CommonWidgets 的参数面板基座 mPanelBase（与前处理/后处理
// 面板同一基类）——构造即应用参数栏控件状态样式，三方参数栏样式由
// 继承关系保证一致，不靠各侧自觉。
// =============================================================================

#include "../PostCurveGlobal.h"

//QT
#include <QVariant>
#include "mParamPanelBase.h"   //参数面板基座（CommonWidgets，三方共用）

namespace PostCurve
{
	class POSTCURVE_EXPORT mCurvePropPageBase : public MechMainWindow::mPanelBase
	{
		Q_OBJECT
	public:
		explicit mCurvePropPageBase(QWidget* parent = nullptr);

		int nodeId() const;    //当前绑定的节点id（-1表示未绑定）
		//切换目标节点并回填：_loading 保护下调用 loadFromModel()。
		//参数栏复用同一页面实例切换同类型节点时就是走这里
		void setNodeId(int nodeId);

		//从模型回填控件（子类实现：读 nodeProperty 设到各控件）
		virtual void loadFromModel() = 0;

	protected:
		//控件变更 -> 命令栈提交（属性名见 CurveProp）。
		//_loading 期间（回填中）或未绑定节点时静默忽略
		void commit(const QString& prop, const QVariant& value);
		//模型属性变化回调（本节点任何属性变化 -> 重新回填；
		//子类可重写做更细的按属性增量刷新）
		virtual void onNodePropertyChanged(int nodeId, const QString& prop);

		int _nodeId = -1;            //当前绑定节点id
		bool _loading = false;       //回填标志：loadFromModel 期间屏蔽 commit，防止回环
	};
}
