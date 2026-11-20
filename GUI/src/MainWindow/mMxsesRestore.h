#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include "mmainwindow_global.h"

//Qt
#include <QString>

#include "mMxsesFormat.h"

namespace MechMainWindow
{
	/*
	* .mxses 会话文件的采集与恢复编排（GUI 侧）。
	* 采集：从视口模型管理器收集全部模型/视口状态与相机，从视图树收集页布局
	* （含曲线图表卡片条目），从曲线文档收集整树快照，从后处理参与者收集
	* 结果文件与云图条目；
	* 恢复：清空全部模型/视图树页/曲线侧，按文件内容重建模型、视口、页结构、
	* 曲线文档与图表卡片，后处理结果文件经参与者串行异步重导入（全部完成后
	* 延迟收尾）。
	* 文件读写线程由 MechIOFile::mIOFileManager 调度，本类只做主线程装配。
	*/
	class MMAINWINDOW_EXPORT mMxsesRestore
	{
	public:
		/*
		* 保存当前会话到 .mxses 文件（采集数据后异步写盘）
		*/
		static bool saveSession(const QString& filePath);

		/*
		* 打开 .mxses 文件（异步读盘，完成后由 slot_readMxsesFinished 恢复）
		*/
		static bool openSession(const QString& filePath);

		/*
		* 读会话文件完成后的恢复入口（由 mMenuWidget 转发 readMxsesFinishedSig）
		*/
		static void slot_readMxsesFinished();

		/*
		* 写会话文件完成后的提示入口（由 mMenuWidget 转发 writeMxsesFinishedSig）
		*/
		static void slot_writeMxsesFinished();

	private:
		/*
		* 采集当前会话数据（主线程）
		*/
		static MechIOFile::MxsesFileData collectSessionData();

		/*
		* 按会话数据清空并重建整个软件状态（主线程）。
		* hasPendingPostImports 带回后处理结果文件异步重导入是否待执行
		* （调用方据此注册延迟收尾，见 slot_readMxsesFinished）
		*/
		static bool restoreFrom(const MechIOFile::MxsesFileData& data, bool& hasPendingPostImports);

		/*
		* 恢复收尾：激活视口、重放当前页布局、解除界面置灰与完成提示。
		* 无后处理导入时由 slot_readMxsesFinished 同步调用；有待导入时经
		* 参与者的完成回调在队列排空后调用（目标视口/模型取 _pending 成员）
		*/
		static void finishSessionRestore();
	};
}
