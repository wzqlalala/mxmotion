#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

//QT
#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QList>
#include <QPair>
#include <functional>

#include "mCommon_global.h"

/*
* 后处理会话参与者接口（.mxses 会话保存/打开的PostWindow侧接入通道）
* 链接方向为 PostWindow -> MainWindow 单向：MainWindow 侧的会话编排
* （mMxsesRestore）不能直接引用 PostWindow 类型，经本接口间接调用；
* 实现方（PostWindow::mPostSessionCoordinator）在 mPostBrowserWindow 构造时
* 自注册到 postSessionParticipantRef()。
* 数据交换一律用 Qt 基础类型，避免 Common 依赖 IOFile（方向相反）。
*/
namespace MechCommon
{
	class COMMON_EXPORT IPostSessionParticipant
	{
	public:
		virtual ~IPostSessionParticipant() = default;

		/*
		* 采集指定后处理模型的结果文件路径与后处理树条目块（会话保存用）。
		* modelID 非后处理模型或无数据时输出保持为空
		*/
		virtual void collectPostSessionEntry(int modelID,
			QStringList& resultFiles, QByteArray& postItemsChunk) = 0;

		/*
		* 按显式模型ID补建后处理树根（会话恢复重建后处理模型时用，
		* 等价 getPostTree()->addPostModelBrowserTree 的显式ID版）
		*/
		virtual void attachPostTree(int modelID) = 0;

		/*
		* 把后处理参数面板同步到指定视口（会话恢复收尾用：结果应用链按视口
		* 逐个级联刷新，结束后面板停留在最后一个应用的视口而非激活视口，
		* 不同步则用户随后的变量/工况切换会打到末位应用视口——等价用户
		* 点一次对应树节点的面板刷新效果，经 edit3DResult 走全量级联）
		*/
		virtual void syncPostPanelToViewport(int modelID, int viewportID) = 0;

		/*
		* 排队一个后处理模型的结果文件异步重导入（会话恢复用）。
		* 前条件：模型与视口已由编排方同步重建完成；仅排队不启动——导入收尾链
		* 按当前模型落库，编排方同步重建期间当前模型会漂移，过早启动会使
		* 收尾链读到错误模型。编排方全部重建完成后调用 kickPostRestoreQueue
		*/
		virtual void queuePostResultImport(int modelID, const QStringList& resultFiles,
			const QByteArray& postItemsChunk) = 0;

		/*
		* 启动队列（编排方在同步重建全部完成后调用一次）。导入完成后由实现方
		* 回放树条目并自动启动队内下一个，全部完成时调用
		* setPostRestoreDoneCallback 注册的回调（一次）
		*/
		virtual void kickPostRestoreQueue() = 0;

		/*
		* 队列全部完成时的回调（编排方在队列非空时注册，用于延迟收尾：
		* 最终布局重放/激活视口/解除界面置灰/完成提示）
		*/
		virtual void setPostRestoreDoneCallback(std::function<void()> onDone) = 0;

		//尚待恢复的后处理模型数（0=空闲；编排方据此决定是否延迟收尾）
		virtual int pendingPostRestoreCount() const = 0;
	};

	//全局参与者实例（PostWindow 启动期注册；未注册时为空，编排方跳过后处理接入）
	COMMON_EXPORT IPostSessionParticipant*& postSessionParticipantRef();
}
