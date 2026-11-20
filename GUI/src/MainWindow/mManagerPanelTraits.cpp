#include "mManagerPanelTraits.h"
#include "mMechGlobalSignals.h"
#include "mLoadBoundaryManager.h"
#include "mLoadBoundaryBase.h"
#include "mPredefinedFieldManager.h"
#include "mPredefinedFieldBase.h"
#include "mContactManager.h"
#include "mContactBase.h"
#include "mModelFixManager.h"
#include "mModelFixDataBase.h"
#include "mCaseManager.h"
#include "mCaseBase.h"

using namespace MechCaseData;
using namespace MechContactData;
using namespace MechModelFixData;

namespace MechMainWindow
{
	QString mManagerPanelTraitsBase::managerTitle() { return QString(); }
	QString mManagerPanelTraitsBase::referenceTitle() { return QString(); }
	QString mManagerPanelTraitsBase::nameFilterPlaceholder() { return QString(); }
	QString mManagerPanelTraitsBase::nameHeader() { return QString(); }
	QString mManagerPanelTraitsBase::matrixEntryText() { return QString(); }
	BrowserTreeType mManagerPanelTraitsBase::matrixTreeType() { return BrowserTreeType(-1); }

	std::vector<int> mManagerPanelTraitsBase::allIds() { return {}; }
	QString mManagerPanelTraitsBase::nameOf(int id) { Q_UNUSED(id); return QString(); }
	QString mManagerPanelTraitsBase::typeNameOf(int id) { Q_UNUSED(id); return QString(); }

	bool mManagerPanelTraitsBase::isReferenced(int id, int caseID) { Q_UNUSED(id); Q_UNUSED(caseID); return false; }
	void mManagerPanelTraitsBase::reference(int id, int caseID) { Q_UNUSED(id); Q_UNUSED(caseID); }
	void mManagerPanelTraitsBase::unreference(int id, int caseID) { Q_UNUSED(id); Q_UNUSED(caseID); }

	bool mManagerPanelTraitsBase::isSupportedByCase(int id, int caseID) { Q_UNUSED(id); Q_UNUSED(caseID); return true; }
	bool mManagerPanelTraitsBase::isForceReferenced(int id) { Q_UNUSED(id); return false; }
	bool mManagerPanelTraitsBase::isSuppressedObject(int id) { Q_UNUSED(id); return false; }

	bool mManagerPanelTraitsBase::hasScaleFactor(int id) { Q_UNUSED(id); return false; }
	double mManagerPanelTraitsBase::scaleOf(int id, int caseID) { Q_UNUSED(id); Q_UNUSED(caseID); return 1.0; }
	void mManagerPanelTraitsBase::setScale(int id, int caseID, double value) { Q_UNUSED(id); Q_UNUSED(caseID); Q_UNUSED(value); }

	void mManagerPanelTraitsBase::emitDelete(const QVector<int>& ids) { Q_UNUSED(ids); }
	QList<QMetaObject::Connection> mManagerPanelTraitsBase::connectRefreshSignals(QObject* context, const std::function<void()>& refresh)
	{
		Q_UNUSED(context); Q_UNUSED(refresh);
		return {};
	}

	void mManagerPanelTraitsBase::editObject(int id, int caseID) { Q_UNUSED(id); Q_UNUSED(caseID); }
	void mManagerPanelTraitsBase::selectManagerItem(int id) { Q_UNUSED(id); }
	void mManagerPanelTraitsBase::selectCaseItem(int caseID, int id) { Q_UNUSED(caseID); Q_UNUSED(id); }

	int mManagerPanelTraitsBase::firstCaseId(int id)
	{
		//占位默认实现：实际查询由各Traits按映射存储结构覆盖
		Q_UNUSED(id);
		return 0;
	}
}
