#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

//QT
#include <QWidget>
#include <functional>

#include "mCommon_global.h"

/*
* 后处理树dock窗口提供者（间接通道）
* 后处理树窗口实现（mPostBrowserWindow，当前暂居MainWindow，PostWindow建库后git mv
* 迁入）在其实现文件中自注册；主窗口注册"PostTree"dock时经此获取QWidget*，
* 消除对具体类型的直接依赖——文件迁移后注册方零改动，链接方向保持 Post侧->MainWindow 单向。
*/
namespace MechCommon
{
	//全局提供者（首次调用前为空；返回静态存储的可写引用）
	COMMON_EXPORT std::function<QWidget*()>& postTreeDockProvider();
	//视频窗口dock提供者（mVideoWindow，同上模式：PostWindow侧自注册，主窗口dock注册经此获取）
	COMMON_EXPORT std::function<QWidget*()>& videoDockProvider();
	//NVH工具集面板dock提供者（mPostNVHContributionPanel，同上模式：PostWindow侧自注册，
	//主窗口注册"NVHToolSet" dock经此获取——模态贡献量分析独立窗口页，落底部条
	//「函数与曲线」标签组右侧并排分栏，openspec add-nvh-toolset-window-page）
	COMMON_EXPORT std::function<QWidget*()>& nvhToolSetDockProvider();
}
