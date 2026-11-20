#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mpostwindow_global.h"

#include <QString>
#include <QVector>

namespace PostData
{
	class mPostDataBase;
	struct Compones;
}

namespace PostWindow
{
	/*
	* 统一取值请求（按需读取"帧×变量×分量"的完整参数面）：
	* 四条读取链路（OP2 等原生/VTK/老格式 mxdb/新格式 mxdb）的调用差异全部收敛到这里，
	* 调用方不再手写格式分支。coordinateName/userCoord 仅老格式 mxdb 消费（现行调用点
	* 恒 "Global"/空，字段保留对齐 readCloudData 形参面）；flattenStress 走磨平应力
	* 读取（readFlattenStress）；var=="ElementNodalForce" 的老格式 4 参特判由 provider 内部处理。
	*/
	struct ValueRequest
	{
		int frame = 0;                     //帧号
		QString var;                       //变量名
		QString compo;                     //分量名
		QString layer;                     //层名（空=跨层平均）
		QString complexMode;               //复数模式（实部/虚部/幅值/相位，空=非复数）
		QString coordinateName = QStringLiteral("Global");  //坐标系名（仅老格式 mxdb）
		QVector<double> userCoord;         //自定义坐标系参数（现行恒空）
		bool flattenStress = false;        //磨平应力读取
	};

	/*
	* 后处理按需取值统一接口（分发点收敛的适配层）：四种格式各一个适配器，工厂
	* valueProviderFor 按现行判定序（native→vtk→老mxdb→新mxdb）解析。
	* 两种取值形态（对应原两类调用点，语义差异是历史契约，迁移时逐点保持）：
	*  - publish：读数并写库（setComponeValues 发布链），云图/动画用——会覆盖
	*    "最后一次发布"云图态，消费方依赖 getComponeValues 回取；
	*  - fetch：只出参不污染云图当前显示态，查询/变形/自动测试用——native/VTK 无出参
	*    形态，实现为发布后回取快照（与原查询面板口径一致）；mxdb 两系走各自的出参重载。
	* reader 生命周期：全部经 mPost3DResultPanel 的池/注册表取（native/vtk 常驻注册表、
	* mxdb 双池），provider 本身轻量，调用方即建即毁。
	*/
	class MPOSTWINDOW_EXPORT IPostValueProvider
	{
	public:
		virtual ~IPostValueProvider() = default;

		//来源格式标识：调用方按历史契约选择失败策略（native/vtk 读失败=中止当前
		//流程[云图重绘 return/动画 return false]，mxdb 失败=报告后继续），不参与取值逻辑
		enum class SourceKind { Native, Vtk, MxdbLegacy, MxdbNew };
		virtual SourceKind sourceKind() const = 0;

		//发布式：读数写库+登记最值（失败返回 false，不产生部分写入保证由各 reader 自带）
		virtual bool publish(const ValueRequest& req) = 0;

		//出参式：只填 out 不改云图当前显示态（键域沿用各链路现行口径，归一为后续增强）
		virtual bool fetch(const ValueRequest& req, PostData::Compones& out) = 0;
	};

	/*
	* 按结果库解析取值 provider：native 注册表 → vtk 注册表 → getDataFlag()（老格式）
	* → 新格式兜底；库为空返回 nullptr。判定序与各面板现行分发一致（slot_showCloud 口径）。
	* 返回裸指针归调用方所有（即 new 即 delete，无父对象）
	*/
	MPOSTWINDOW_EXPORT IPostValueProvider* valueProviderFor(PostData::mPostDataBase* postDB);
}
