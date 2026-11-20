#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

//QT
#include <QWidget>
#include <functional>

#include "mCommon_global.h"

/*
* 参数面板宿主接口（Info dock 容器抽象）
* mInfoWindow（居MainWindow，前处理参数面板调度者）实现本接口并在构造时注册；
* PostWindow::mPostInfoHub 调度后处理面板时经此激活Info dock并复位面板，
* 消除PostWindow对MainWindow的类型依赖——断链批次（E）关键收口之一。
*/
namespace MechCommon
{
	class IPanelHost
	{
	public:
		virtual ~IPanelHost() = default;

		/*
		* 切换 Info dock 内容部件（清空原内容并显示新面板）
		*/
		virtual void setMainWidget(QWidget* w) = 0;

		/*
		* 当前挂载的参数面板部件（无面板/实现未支持时返回 nullptr）
		* UI 截图工具（mUiShotController）经此通用获取 grab 目标，
		* 面板各自无需暴露部件访问器
		*/
		virtual QWidget* mainWidget() const { return nullptr; }

		/*
		* 参数面板总复位（清除全部已编辑面板状态；后处理侧路径拾取复位经总线连带）
		*/
		virtual void unEditInfoWindow() = 0;
	};

	//全局宿主实例（mInfoWindow构造时注册；未注册时为nullptr）
	COMMON_EXPORT IPanelHost*& panelHostRef();
}
