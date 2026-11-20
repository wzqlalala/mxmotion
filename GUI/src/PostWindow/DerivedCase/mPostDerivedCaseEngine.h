#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mpostwindow_global.h"
#include "DerivedCase/mPostDerivedCaseTypes.h"

#include <QHash>
#include <QList>
#include <QPointer>
#include <QString>

class QWidget;

namespace PostData
{
	class mPostDataBase;
}

namespace PostWindow
{
	/*
	* 派生工况生成引擎（2026-11-12 派生工况开发计划批次A1：步序列先行，包络
	* 批次B 接入同款骨架）。
	*
	* 职责：store 定义 → expandDefinition 展开 → 逐来源解析帧号 → 四分支读数
	* （native 懒加载 / vtk / 旧格式 mIMxdbFile1 / mMxdbThread，范式同
	* slot_generateLinear）→ 值×缩放 → mPostDataBase::appendDerivedFrame 物化
	* 为派生工况帧（几何克隆自源帧）。
	*
	* 口径（计划 D5 修订，HyperView 行为）：不建独立视口——物化帧登记进 PostData
	* 派生工况（getAllCaseNames 排除、getDerivedCaseNames 枚举），生成面板随后经
	* 云图面板工况下拉切换查看/帧滑条播放；引擎不做任何视口/下拉操作（尾段刷新
	* 归面板）。
	*/
	class MPOSTWINDOW_EXPORT mPostDerivedCaseEngine
	{
	public:
		//生成上下文（面板 resolveContext 后组装；变量身份取当前云图视口 guidata）
		struct GenerateContext
		{
			QPointer<PostData::mPostDataBase> postDB;	//目标结果库（读数与物化落点）
			QString postPathName;						//结果文件路径（读数分支判定）
			QString postName;							//结果数据名（旧格式 mIMxdbFile1 读数用）
			QString var;								//云图变量名（非 SolidColor）
			QString compo;								//分量名（可空）
			QString layer;								//层名（可空）
			QString complexMode;						//复数模式（可空）
			QWidget* progressParent = nullptr;			//进度框父窗口（生成面板）
		};

		/*
		* 步序列生成：展开 def 至原生来源 → 逐行解析帧号+读数+缩放 →
		* appendDerivedFrame（工况名=def.name，帧名"i: 源工况/源帧"）。
		* allDefs 传该模型桶全 kind 定义集（expandDefinition 纯函数入参）。
		* 再生成语义：先 removeDerivedCase(def.name) 清旧帧（旧帧墓碑、帧号不复用）。
		* 返回 false 时 errMsg 回带原因（上下文无效/待配置/来源不可用/读数失败/
		* 用户取消）；失败与取消均已回滚本次登记的全部派生帧。
		*/
		static bool generateSteps(const DerivedCaseDefinition& def,
			const QList<DerivedCaseDefinition>& allDefs, const GenerateContext& ctx,
			QString* errMsg = nullptr);

		/********************包络归约（批次B；纯函数，无头单测覆盖）********************/

		/*
		* 流式归约运行态：调用方在逐帧循环内持本结构喂入（帧数据读入即弃，
		* 内存 O(1) 帧 × 2 数组，不做 N 帧驻留）。min/max 双数组为 Min/Max/
		* Extreme/Range 四类型共同基底；来源帧索引（喂入序 0 基）随极值更新，
		* 供后续"极值来源可视化"增强消费（信息标签/探测探针）。
		*/
		struct EnvelopeAccumulator
		{
			QHash<int, double> minByKey;		//每键运行最小
			QHash<int, double> maxByKey;		//每键运行最大
			QHash<int, int> minSourceByKey;		//每键最小值来源帧（喂入序）
			QHash<int, int> maxSourceByKey;		//每键最大值来源帧（喂入序）
			int fedFrames = 0;					//已喂入帧数
			bool keysIntersected = false;		//喂入中发生过键域收缩（帧键域不一致提示用）
		};

		/*
		* 喂入一帧（缩放后键值）：首帧建立键域；后续帧与运行域取交集（不在本帧的
		* 键剔除并置 keysIntersected，避免脏数据静默），域内键更新运行极值。
		* 返回 false=交集已空（含首帧空数据），归约无可行结果。
		*/
		static bool envelopeFeed(EnvelopeAccumulator& acc, const QHash<int, double>& frameValues);

		/*
		* 按类型产出最终包络值：0 Min / 1 Max（默认）/ 2 Extreme（绝对值最大者取
		* **原值**，非绝对值；|max|==|min| 并列取 max 侧）/ 3 Range（极差）——
		* 与 DerivedCaseDefinition::envelopeType 对应。
		*/
		static QHash<int, double> envelopeReduce(const EnvelopeAccumulator& acc, int envelopeType);

		/*
		* 包络生成：展开 def → 逐行解析帧号+读数+缩放 → 流式喂入归约 → 单帧
		* appendDerivedFrame（工况名=def.name，帧名"0: 包络"，几何克隆自首个成功
		* 喂入的源帧）。键域收缩经输出窗提示（非致命）；其余口径同 generateSteps
		* （再生成清旧帧/进度可取消/失败与取消回滚）。
		*/
		static bool generateEnvelope(const DerivedCaseDefinition& def,
			const QList<DerivedCaseDefinition>& allDefs, const GenerateContext& ctx,
			QString* errMsg = nullptr);

		/*
		* 四分支读数器（批次C 自线性叠加 slot_generateLinear 收敛提取，三生成链
		* 共用）：native 懒加载 / vtk / 旧格式 mIMxdbFile1（含 ElementNodalForce）/
		* mMxdbThread，reader 经云图面板懒加载表获取。任一分支失败落输出窗并返回
		* false（比线性叠加旧"旧格式失败仅报告继续"更严——坏数据不再静默进结果）。
		*/
		static bool readSourceFrame(const GenerateContext& ctx, int frameId);

	private:
		/*
		* 结果变量身份串：镜像 mPostDataBase::addResultValue 的 _resultVarNames
		* 登记格式（varName[+" "+compoName]，mPostDataBase.cpp:1943 附近）——
		* slot_showCloud 读缓存与派生帧变量守卫按此串比对，三处格式改动须同步
		* （另两处：mPostDataBase 登记侧、mPost3DResult::expectedResultVarName）。
		*/
		static QString resultVarIdentity(const QString& varstr, const QString& compostr);
	};
}
