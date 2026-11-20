#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 曲线图层管理（模板应用生成的"层"注册表，单例，会话内存活）
//
// "层" = 一次模板应用生成的曲线/基准线集合（跨其生成的所有页面）。D5：
//   - 层序 = 应用序，层名默认 = 模板名（图层对话框可重命名，只改层名不动
//     成员节点 kName）；
//   - 显隐直接写成员曲线既有 kVisible（与曲线树显隐同键，图表/视图联动免费
//     获得）；层节点勾选状态由成员曲线现算（任一显=勾），不存储，杜绝双写源；
//   - 删除层 = 遍历成员 removeNode 包单个撤销宏（一次 Ctrl+Z 整体撤销；
//     撤销恢复节点但层记录不回收——v1 接受，注释见 removeLayer）；
//   - v1 不随 .mxses 会话快照持久化；文档变化不主动推送，读侧（layers/
//     validGroups/layerOfCurve）现查现剪失效成员（页面/曲线被删即剔除）。
//
// 应用管线（mTemplateApplyPipeline）在每次应用成功后 registerLayer 登记
// （任务 5.1）；validGroups 为"选择起始页面"对话框（任务 4.2）提供组枚举。
//
// D10（2026-11-02 语义三修 v3）：叠加勾选"不叠加基准线/参考曲线"时基准线
// 独立成层并按数据同一性分组——mergeDatumLayer 归置登记：迁出源=同组上一次
// 应用注册的普通曲线层（更早普通层不动）落页基准线成员 + 本次新建基准线，
// 逐条按数据等价类（除 visible/layer 外全字段比对）找层/建层：同数据并入
// 既有基准线层、异数据各新建一层（层名固定 "Datum Lines and Reference
// Curves"，树行显示号取负 "Layer -N"（-1 起随 datumNo 递增，与曲线层正编号
// 视觉分序），datumNo 本身仍为正独立分序、内部 id 仍单一序列；
// 同组可多个基准线层）；命中他组基准线层则跨组合并——成员并入、页集合并集、
// 显示号不变，层置 ownGroup（图层树升独立页面分组节点），同数据全会话至多
// 一个基准线层；findEquivalentBaseline 供管线生成前逐条判重（同数据基准线
// 已在本组页面上=本次不生成）。基准线层与普通层同权（右键重命名/删除、勾选
// 显隐）；validGroups 剔除基准线层（不作叠加/替换目标）。
// =============================================================================

#include "PostCurveGlobal.h"
#include "CurveCore/CurveDefine.h"

//QT
#include <QObject>
#include <QString>
#include <QList>
#include <QVector>

namespace PostCurve
{
	class POSTCURVE_EXPORT mCurveLayerManager : public QObject
	{
		Q_OBJECT
	public:
		static mCurveLayerManager* getInstance();

		mCurveLayerManager(const mCurveLayerManager&) = delete;
		mCurveLayerManager& operator=(const mCurveLayerManager&) = delete;

		//--- 层记录（读接口均现查文档剪除失效成员，返回即当时快照） ---
		struct LayerInfo
		{
			int id = 0;                    //层 id（会话内只增）
			QString name;                  //层名（默认=模板名，可重命名）
			QList<int> curveIds;           //成员曲线（失效成员已剔除）
			QList<int> baselineIds;        //成员基准线（随层删除；勾选显隐覆盖曲线+基准线）
			QVector<int> pageIds;          //涉及的页面（新建页或作用页，失效页已剔除）
			int datumNo = 0;               //0=普通曲线层；>0=基准线层显示号（独立计数，D10）
			bool ownGroup = false;         //基准线层跨组合并标志：树中升独立页面分组节点（v3）
		};

		//模板页面组（叠加/替换的作用目标，任务 4.2"选择起始页面"枚举用）
		struct GroupInfo
		{
			int layerId = 0;               //所属层
			QString label;                 //显示串："模板名 (Page N: 页名)"，N=页位置序（1基）
			int startPageId = -1;          //起始页 id（层 pageIds 首个存活页）
		};

		//登记一层（应用管线收尾调用）：层名=模板名，追加到层序尾，返回层 id。
		//成员 id 不校验存活（登记即应用成功瞬间的产物）
		int registerLayer(const QString& name, const QList<int>& curveIds,
			const QList<int>& baselineIds, const QVector<int>& pageIds);

		//归置登记基准线层（D10 v3，叠加勾选"不叠加基准线/参考曲线"收尾时由
		//应用管线调用）：整理范围=同组上一次应用注册的普通曲线层（登记序最新、
		//起始页相同者）落在本次页面上的基准线成员（迁出，更早普通层不动）+
		//newBaselineIds；逐条按数据等价类归置——与既有基准线层成员同数据并入
		//该层（页集合并集；跨组命中置 ownGroup 升独立页面节点，显示号不变），
		//异数据本组新建一层（datumNo 取独立计数器，内部 id 仍单一序列，同组
		//可多个基准线层）。无可归置成员不建层。返回末次归置触达的基准线层 id
		//（无可归置成员返回 0）
		int mergeDatumLayer(const QList<int>& newBaselineIds, const QVector<int>& pageIds);

		//逐条判重（D10 v2，管线生成基准线前调用）：在 pageIds 各存活页面的图表
		//基准线中，找与 spec 全等者（CurveBaselineData 除 visible/layer 外全字段
		//比对；不看层归属，手绘基准线一并参与判重）；命中返回其节点 id，未命中
		//返回 0
		int findEquivalentBaseline(const QVector<int>& pageIds,
			const CurveBaselineData& spec) const;

		//全部层（失效成员/页面剪除后；成员全空=层随文档删除一并消失，不返回）
		QList<LayerInfo> layers() const;

		//曲线所属层 id（未归属/曲线已删除返回 -1）
		int layerOfCurve(int curveId) const;

		//模板页面组枚举（失效组剔除=起始页被删；层序即应用序）
		QList<GroupInfo> validGroups() const;

		//层改名（只改层名，不动成员节点 kName）；层不存在返回 false
		bool renameLayer(int layerId, const QString& name);

		//删除层：遍历存活成员（曲线+基准线）removeNode 包单个撤销宏
		//（"删除图层"，一次 Ctrl+Z 整体撤销）；随后层记录移除——撤销恢复节点
		//但层不回来（v1 接受：删除即出列，重应用模板即重建）。层不存在返回 false
		bool removeLayer(int layerId, QString* err = nullptr);

		//层内成员批量显隐（写 kVisible，走命令栈；图层对话框勾选级联用）。
		//曲线+基准线都覆盖（D10：显隐与删除同口径）
		void setCurvesVisible(int layerId, bool visible);

		//清空全部层记录（会话重置/测试用；不动文档节点）；层 id 与基准线层
		//显示号计数一并归位（新会话从 1 重新起计，D10）
		void reset();

	private:
		explicit mCurveLayerManager(QObject* parent = nullptr);

		static mCurveLayerManager* _instance;   //单例

		struct Rec
		{
			int id = 0;
			QString name;
			QList<int> curveIds;
			QList<int> baselineIds;
			QVector<int> pageIds;
			int datumNo = 0;               //0=普通曲线层；>0=基准线层（D10）
			bool ownGroup = false;         //基准线层跨组合并标志（v3）
		};
		QVector<Rec> _layers;   //层序=应用序
		int _nextLayerId = 1;   //层 id 分配器（会话内只增）
		int _nextDatumNo = 0;   //基准线层显示号分配器（独立于 id 序，D10）
	};
}
