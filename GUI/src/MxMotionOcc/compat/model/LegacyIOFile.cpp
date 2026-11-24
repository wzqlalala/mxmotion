#include "mIOFileManager.h"
#include "LegacyMotionContext.h"
#include "NativeMotionExchange.h"
#include "mViewportModelManager.h"
#include "mGlobalSignals.h"
namespace MechIOFile {
void mIOFileManager::slot_exportMotionJsonFile(const QString& path,bool) {
    if(!getCVM || !getCVM->getModelData()->context)return;
    auto* context=getCVM->getModelData()->context;context->flush();QString error;
    if(!MotionOcc::writeNativeMotionExchange(path,context->cadDocument(),context->document(),error))
        emit MxFunctions::mGlobalSignals::getInstance()->outputMessageSig(error,true);
}
}
