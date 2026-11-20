#include "mViewTreeFunction.h"
#include "mMechGlobalSignals.h"	//树信号挂起/恢复经总线直连（原直调mBrowserWindow树）
#include "mViewportModelManager.h"
#include "mViewportModel.h"


using namespace MechPreData;
namespace MechMainWindow
{
	void mViewTreeFunction::deleteAllViewportModel()
	{
		emit MechCommon::mMechGlobalSignals::getInstance()->suspendBrowserTreeSig();
		VMManager->deleteAllViewportModel();
		emit MechCommon::mMechGlobalSignals::getInstance()->resumeBrowserTreeSig();
	}

	void mViewTreeFunction::newViewport()
	{
		emit MechCommon::mMechGlobalSignals::getInstance()->suspendBrowserTreeSig();
		VMManager->newViewport();
		emit MechCommon::mMechGlobalSignals::getInstance()->resumeBrowserTreeSig();
	}

	void mViewTreeFunction::newViewport(MxFunctions::ViewportType type, QString name)
	{
		emit MechCommon::mMechGlobalSignals::getInstance()->suspendBrowserTreeSig();
		VMManager->newViewport(type, name);
		emit MechCommon::mMechGlobalSignals::getInstance()->resumeBrowserTreeSig();
	}

	int mViewTreeFunction::newViewportModel(bool fullScreen)
	{
		emit MechCommon::mMechGlobalSignals::getInstance()->suspendBrowserTreeSig();
		int modelID = VMManager->newViewportModel(fullScreen);
		emit MechCommon::mMechGlobalSignals::getInstance()->resumeBrowserTreeSig();
		return modelID;
	}

	int mViewTreeFunction::newPostViewportModel()
	{
		emit MechCommon::mMechGlobalSignals::getInstance()->suspendBrowserTreeSig();
		int modelID = VMManager->newPostViewportModel();
		emit MechCommon::mMechGlobalSignals::getInstance()->resumeBrowserTreeSig();
		return modelID;
	}

	void mViewTreeFunction::deleteViewportModel(int viewportModelId)
	{
		emit MechCommon::mMechGlobalSignals::getInstance()->suspendBrowserTreeSig();
		VMManager->deleteViewportModel(viewportModelId);
		emit MechCommon::mMechGlobalSignals::getInstance()->resumeBrowserTreeSig();
	}

	void mViewTreeFunction::deleteViewport(int viewportId)
	{
		emit MechCommon::mMechGlobalSignals::getInstance()->suspendBrowserTreeSig();
		VMManager->deleteViewport(viewportId);
		emit MechCommon::mMechGlobalSignals::getInstance()->resumeBrowserTreeSig();
	}

	void mViewTreeFunction::deletePostViewport(QString name, int viewportModelId)
	{
		emit MechCommon::mMechGlobalSignals::getInstance()->suspendBrowserTreeSig();
		VMManager->deletePostViewport(name, viewportModelId);
		emit MechCommon::mMechGlobalSignals::getInstance()->resumeBrowserTreeSig();
	}

	void mViewTreeFunction::deletePostViewport(int viewportModelId)
	{
		emit MechCommon::mMechGlobalSignals::getInstance()->suspendBrowserTreeSig();
		VMManager->deletePostViewport(viewportModelId);
		emit MechCommon::mMechGlobalSignals::getInstance()->resumeBrowserTreeSig();
	}

	void mViewTreeFunction::activatePreViewport()
	{
		emit MechCommon::mMechGlobalSignals::getInstance()->suspendBrowserTreeSig();
		VMManager->activatePreViewport();
		emit MechCommon::mMechGlobalSignals::getInstance()->resumeBrowserTreeSig();
	}

	void mViewTreeFunction::activatePostViewport(QString name)
	{
		emit MechCommon::mMechGlobalSignals::getInstance()->suspendBrowserTreeSig();
		VMManager->activatePostViewport(name);
		emit MechCommon::mMechGlobalSignals::getInstance()->resumeBrowserTreeSig();
	}

}