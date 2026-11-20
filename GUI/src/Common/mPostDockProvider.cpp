#include "mPostDockProvider.h"
#include "mPanelHostProvider.h"
#include "mCenterWindowProvider.h"

namespace MechCommon
{
	std::function<QWidget*()>& postTreeDockProvider()
	{
		static std::function<QWidget*()> provider;
		return provider;
	}

	std::function<QWidget*()>& videoDockProvider()
	{
		static std::function<QWidget*()> provider;
		return provider;
	}

	std::function<QWidget*()>& nvhToolSetDockProvider()
	{
		static std::function<QWidget*()> provider;
		return provider;
	}
}

//中心窗口/工具栏提供者（mDockLayoutManager 预设重排钉正工具栏用，见头注释）
namespace MechCommon
{
	std::function<QMainWindow*()>& centerMainWindowProvider()
	{
		static std::function<QMainWindow*()> inst;
		return inst;
	}
	std::function<QToolBar*()>& viewToolBarProvider()
	{
		static std::function<QToolBar*()> inst;
		return inst;
	}
	std::function<QToolBar*()>& viewDisplayBarProvider()
	{
		static std::function<QToolBar*()> inst;
		return inst;
	}
	//动画工具栏壳（anim-tab-v13，见 mCenterWindowProvider.h 注释）
	std::function<QToolBar*()>& animToolBarProvider()
	{
		static std::function<QToolBar*()> inst;
		return inst;
	}
}

//参数面板宿主接口实例持有（mInfoWindow构造注册，见 mPanelHostProvider.h）
namespace MechCommon
{
	IPanelHost*& panelHostRef()
	{
		static IPanelHost* inst = nullptr;
		return inst;
	}
}

