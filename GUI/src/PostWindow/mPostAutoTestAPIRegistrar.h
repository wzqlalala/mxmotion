#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QObject>
#include <QVariant>

#include "mpostwindow_global.h"
#include "mAPIServerTypes.h"

namespace PostWindow
{
	/*
	* 后处理自动化测试 API 注册器（mx-autotest 平台的 L1 数据抽取原语）
	*
	* 定位：只提供"驱动原语"（开结果文件/列变量/取数值/截图），不写业务用例——
	* 业务用例全部在外层 mx-autotest 的 JSON 步骤脚本里（-e script.json 经
	* mAPIScriptExecutor 执行，每步 apiKey=下述 API 名）。
	*
	* 注册时机：mAppInitializer::initScriptExecutor（脚本模式启动时），
	* 经 getAPISchedulingServer 注册；全部为只读/低副作用操作。
	*
	* API 清单（命名遵循 mAPIServerTypes.h 规范 API_类名_功能名）：
	*   API_Result_OpenFiles      files（多值）——打开结果文件并建后处理树
	*                             完成语义 postImportIdle（由 mAppInitializer 挂策略）
	*   API_Result_ListVariables  ——当前后处理数据库的帧号/变量名清单（脚本探查用）
	*   API_Result_QueryValues    variable[必需]，frame/component/layer/complex/nodes/maxDump[可选]
	*                             ——按四分支读取协议（native→vtk→mxdb新→mxdb旧，
	*                             与 mPostQueryPanel::readVarCompones 同口径）取
	*                             Compones 值，返回 JSON 摘要+指定节点值（金样本对比用）
	*   API_Scene_Shot            out[必需]，target=panel|window[可选]，settleMs[可选]
	*                             ——截当前 Info dock 主部件或整窗 PNG（脚本内截图步骤）
	*/
	class MPOSTWINDOW_EXPORT mPostAutoTestAPIRegistrar : public QObject
	{
		Q_OBJECT
	public:
		static mPostAutoTestAPIRegistrar* getInstance();

		/*
		* 幂等注册全部测试 API 到 API 调度服务器。
		* 返回 false 表示调度服务器不可用（极少发生，调用方据此给出脚本启动错误）。
		*/
		bool ensureRegistered();

		/*
		* postImportIdle 完成条件探针：最近一次 API_Result_OpenFiles 之后是否
		* 已收到 finishedReadPostFileSig（先接线再触发导入，见 onResultOpenFiles）。
		* 多文件场景首个信号即视为完成（v1 约定，典型单文件够用）。
		*/
		bool importSettled() const;

	private:
		explicit mPostAutoTestAPIRegistrar(QObject* parent = nullptr);

		QVariant onResultOpenFiles(const APIServer::APIParams& params, QString& error);
		QVariant onResultListVariables(const APIServer::APIParams& params, QString& error);
		QVariant onResultQueryValues(const APIServer::APIParams& params, QString& error);
		QVariant onSceneShot(const APIServer::APIParams& params, QString& error);

		bool _registered = false;
		qint64 _importStartMs = 0;
		qint64 _importFinishMs = 0;
	};
}
