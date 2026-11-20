#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QList>
#include <QString>

namespace PostWindow
{
	/*
	* 派生工况三功能（步序列/线性叠加/包络）的公共定义模型（单一事实源）。
	*
	* 消费方：三参数面板（编辑）、树"派生工况"分支（列表/回编）、.mxses 会话
	* （序列化往返）、引擎生成（批次A1起接入 expandDefinition）。字段为三面板
	* 私有定义结构（步序列/包络 Definition、线性叠加 LinearDef）的规范化合并；
	* 纯数据无 QObject/UI 依赖，随 mPostDerivedCaseStore 无头单测覆盖。
	*/

	enum class DerivedCaseKind
	{
		Steps = 0,          //步序列
		Superposition = 1,  //线性叠加
		Envelope = 2        //包络
	};

	//定义的启用来源行（顺序即步序列的步序；线性叠加顺序无语义）
	struct DerivedSourceRow
	{
		QString id;			//行标识：原生 "工况名|帧名"；派生 "derived:<定义id>|<子行id>"
		QString name;		//来源显示名（原生=工况名；派生=定义名）
		QString frame;		//帧文本（原生=帧名；派生=步文案/"0：包络"/"—"）
		QString scale = QStringLiteral("1");	//缩放系数（保持输入文本，校验/展开时解析）
		bool isDerived = false;				//派生工况来源
		int derivedId = -1;					//派生来源定义 id
		int frameId = 0;					//线性叠加：启用来源直存帧号（生成直读；其余 kind 为 0）
	};

	//派生工况定义
	struct DerivedCaseDefinition
	{
		int id = 0;
		DerivedCaseKind kind = DerivedCaseKind::Steps;
		QString name;
		int envelopeType = 1;		//包络类型（仅 Envelope 消费）：0 Min / 1 Max / 2 Extreme / 3 Range
		QList<DerivedSourceRow> rows;	//保存时启用的来源
		bool configured = false;	//有启用行（原型 definition.status）

		bool isValid() const { return id > 0; }
	};
}
