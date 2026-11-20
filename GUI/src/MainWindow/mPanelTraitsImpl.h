#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

//跳转类前置声明：定义由包含本头的cpp提供（mInfoWindow.cpp等），避免与mInfoWindow.h循环include
namespace MechMainWindow
{
	class mInfoWindow;
	class mBrowserWindow;
	class mTreeView;
}

#include "mManagerPanelTraits.h"
//视口数据
#include "mViewportModelManager.h"
#include "mViewportModel.h"

//BasicData
#include "mModelData.h"
#include "mMapRelationManager.h"
#include "mLoadBoundaryManager.h"
#include "mLoadBoundaryBase.h"
#include "mLoadForce.h"
#include "mLoadPressure.h"
#include "mLoadHeatFlux.h"
#include "mLoadRadiation.h"
#include "mLoadConvection.h"
#include "mBoundaryTemp.h"
#include "mPredefinedFieldManager.h"
#include "mPredefinedFieldBase.h"
#include "mContactManager.h"
#include "mContactBase.h"
#include "mModelFixManager.h"
#include "mModelFixDataBase.h"
#include "mCaseManager.h"
#include "mCaseBase.h"
#include "mMechGlobalSignals.h"
#include "mDataStatusManager.h"

//Qt的signals宏在信号声明处展开为public，在指针成员函数地址表达式中会引发语法错误，
//此处临时关闭该宏（此文件内不声明信号，仅在表达式中取信号地址），文件末尾恢复
#pragma push_macro("signals")
#undef signals

using namespace MechCaseData;
using namespace MechContactData;
using namespace MechModelFixData;

namespace MechMainWindow
{
	/******************************* 载荷 *******************************/
	struct MMAINWINDOW_EXPORT mLoadPanelTraits : public mManagerPanelTraitsBase
	{
		static QString managerTitle() { return "载荷管理器"; }
		static QString referenceTitle() { return "载荷引用"; }
		static QString nameFilterPlaceholder() { return "筛选载荷名称"; }
		static QString nameHeader() { return "载荷名称"; }
		static QString matrixEntryText() { return "配置工况载荷"; }
		static BrowserTreeType matrixTreeType() { return LoadManagerParentItem; }

		static mLoadBoundaryManager& manager()
		{
			return getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary);
		}

		static std::vector<int> allIds() { return manager().getAllLoadID(); }
		static QString nameOf(int id) { return manager().getLoadBoundaryNameByID(id); }
		static QString typeNameOf(int id) { return manager().getLoadBoundaryTypeNameByID(id); }

		static bool isReferenced(int id, int caseID)
		{
			auto referenced = getCVM->getModelData()->getMapRelationManager().getMapRelation<std::set<int>>({ Physical_Case, caseID }, Physical_LoadBoundary);
			return referenced.count(id) > 0;
		}
		static void reference(int id, int caseID)
		{
			if (isReferenced(id, caseID))
			{
				//已引用时不重复添加（重复append会重置缩放系数）
				return;
			}
			getCVM->getModelData()->getMapRelationManager().appendMapRelation({ Physical_LoadBoundary, id }, { Physical_Case, caseID });
			mBrowserWindow::getInstance()->getBrowserTree()->addCaseLoadChildItem(caseID, id);
			getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_LoadBoundary, id);
		}
		static void unreference(int id, int caseID)
		{
			//只取消引用关系，不删除载荷数据
			getCVM->getModelData()->getMapRelationManager().deleteMapRelation({ Physical_Case, caseID }, { Physical_LoadBoundary, id });
			mBrowserWindow::getInstance()->getBrowserTree()->removeCaseLoadChildItem(caseID, id);
			getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_LoadBoundary, id);
		}

		static bool isSupportedByCase(int id, int caseID)
		{
			if (caseID == 0 || getCVM == nullptr)
			{
				return false;
			}
			mCaseBase* caseData = getCVM->getModelData()->getManager<mCaseManager>(Physical_Case).getCaseData(caseID);
			auto loadData = manager().getLoadBoundaryData(id);
			if (caseData == nullptr || loadData == nullptr)
			{
				return false;
			}
			//声学载荷不挂载在工况下载荷节点（与树创建逻辑保持一致）
			if (loadData->getLoadBoundaryType() == LOAD_IM_ACOCONCFORCE)
			{
				return false;
			}
			return mTreeView::isLoadBoundarySupportedByCase(caseData->getCaseType(), loadData->getLoadBoundaryType());
		}
		static bool isSuppressedObject(int id)
		{
			auto data = manager().getLoadBoundaryData(id);
			return data && data->isSuppressed();
		}

		//与载荷编辑面板"载荷引用状态"折叠框的显示条件保持一致
		static bool hasScaleFactor(int id)
		{
			auto loadData = manager().getLoadBoundaryData(id);
			if (loadData == nullptr)
			{
				return false;
			}
			switch (loadData->getLoadBoundaryType())
			{
			case LOAD_IM_FORCE:
			case LOAD_IM_TORQUE:
			case LOAD_IM_PRESSURE:
			case LOAD_IM_GRAVITY:
			case LOAD_IM_BOLT:
			case LOAD_IM_SURFACE:
			case LOAD_IM_HEATFLUX:
			case LOAD_IM_RADIATION:
			case LOAD_IM_CONVECTION:
			case LOAD_IM_EDGE:
				break;
			default:
				return false;
			}
			//分析场方式无大小概念，编辑面板不提供缩放因子（集中力/力矩共用mLoadForce，仅集中力支持分析场）
			auto forceData = std::dynamic_pointer_cast<mLoadForce>(loadData);
			if (forceData && forceData->getLoadBoundaryType() == LOAD_IM_FORCE && forceData->isAnalysisFieldMode_)
			{
				return false;
			}
			auto pressureData = std::dynamic_pointer_cast<mLoadPressure>(loadData);
			if (pressureData && pressureData->pressureType_ == 0 && pressureData->isAnalysisFieldMode_)
			{
				return false;
			}
			auto heatFluxData = std::dynamic_pointer_cast<mLoadHeatFlux>(loadData);
			if (heatFluxData && heatFluxData->defineType_ == 2)
			{
				return false;
			}
			auto radiationData = std::dynamic_pointer_cast<mLoadRadiation>(loadData);
			if (radiationData && radiationData->defineType_ == 2)
			{
				return false;
			}
			auto convectionData = std::dynamic_pointer_cast<mLoadConvection>(loadData);
			if (convectionData && convectionData->defineType_ == 3)
			{
				return false;
			}
			return true;
		}
		static double scaleOf(int id, int caseID)
		{
			return getCVM->getModelData()->getMapRelationManager().getScaleByCase({ Physical_LoadBoundary, id }, caseID);
		}
		static void setScale(int id, int caseID, double value)
		{
			getCVM->getModelData()->getMapRelationManager().setScaleByCase({ Physical_LoadBoundary, id }, caseID, value);
		}

		static void emitDelete(const QVector<int>& ids)
		{
			MechCommon::mMechGlobalSignals::getInstance()->deleteLoadsSig(ids);
		}
		static QList<QMetaObject::Connection> connectRefreshSignals(QObject* context, const std::function<void()>& refresh)
		{
			auto* signals = MechCommon::mMechGlobalSignals::getInstance();
			return {
				QObject::connect(signals, &MechCommon::mMechGlobalSignals::deleteLoadsSig, context, [refresh](const QVector<int>&) { refresh(); }),
				QObject::connect(signals, &MechCommon::mMechGlobalSignals::createUpdateLoadSig, context, [refresh](bool, int) { refresh(); })
			};
		}

		static void editObject(int id, int caseID) { mInfoWindow::getInstance()->editLoad(id, caseID); }
		static void selectManagerItem(int id) { mBrowserWindow::getInstance()->getBrowserTree()->selectLoadManagerLoadItem(id); }
		static void selectCaseItem(int caseID, int id) { mBrowserWindow::getInstance()->getBrowserTree()->selectCaseLoadItem(caseID, id); }
		static int firstCaseId(int id)
		{
			if (getCVM == nullptr)
			{
				return 0;
			}
			std::unordered_map<int, double> caseMap =
				getCVM->getModelData()->getMapRelationManager().getMapRelation<std::unordered_map<int, double>>({ Physical_LoadBoundary, id }, Physical_Case);
			return caseMap.empty() ? 0 : caseMap.begin()->first;
		}
	};

	/******************************* 边界 *******************************/
	struct MMAINWINDOW_EXPORT mBoundaryPanelTraits : public mManagerPanelTraitsBase
	{
		static QString managerTitle() { return "边界管理器"; }
		static QString referenceTitle() { return "边界引用"; }
		static QString nameFilterPlaceholder() { return "筛选边界名称"; }
		static QString nameHeader() { return "边界名称"; }
		static QString matrixEntryText() { return "配置工况边界"; }
		static BrowserTreeType matrixTreeType() { return BoundaryManagerParentItem; }

		static mLoadBoundaryManager& manager()
		{
			return getCVM->getModelData()->getManager<mLoadBoundaryManager>(Physical_LoadBoundary);
		}

		static std::vector<int> allIds() { return manager().getAllBoundaryID(); }
		static QString nameOf(int id) { return manager().getLoadBoundaryNameByID(id); }
		static QString typeNameOf(int id) { return manager().getLoadBoundaryTypeNameByID(id); }

		static bool isReferenced(int id, int caseID)
		{
			auto referenced = getCVM->getModelData()->getMapRelationManager().getMapRelation<std::set<int>>({ Physical_Case, caseID }, Physical_LoadBoundary);
			return referenced.count(id) > 0;
		}
		static void reference(int id, int caseID)
		{
			if (isReferenced(id, caseID))
			{
				return;
			}
			getCVM->getModelData()->getMapRelationManager().appendMapRelation({ Physical_LoadBoundary, id }, { Physical_Case, caseID });
			mBrowserWindow::getInstance()->getBrowserTree()->addCaseBoundaryChildItem(caseID, id);
			getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_LoadBoundary, id);
		}
		static void unreference(int id, int caseID)
		{
			getCVM->getModelData()->getMapRelationManager().deleteMapRelation({ Physical_Case, caseID }, { Physical_LoadBoundary, id });
			mBrowserWindow::getInstance()->getBrowserTree()->removeCaseBoundaryChildItem(caseID, id);
			getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_LoadBoundary, id);
		}

		static bool isSupportedByCase(int id, int caseID)
		{
			if (caseID == 0 || getCVM == nullptr)
			{
				return false;
			}
			mCaseBase* caseData = getCVM->getModelData()->getManager<mCaseManager>(Physical_Case).getCaseData(caseID);
			auto boundaryData = manager().getLoadBoundaryData(id);
			if (caseData == nullptr || boundaryData == nullptr)
			{
				return false;
			}
			return mTreeView::isLoadBoundarySupportedByCase(caseData->getCaseType(), boundaryData->getLoadBoundaryType());
		}
		static bool isSuppressedObject(int id)
		{
			auto data = manager().getLoadBoundaryData(id);
			return data && data->isSuppressed();
		}

		//边界编辑面板提供缩放因子（与边界"引用状态"折叠框的缩放因子控件保持一致）
		//仅位移/转角、速度、加速度、温度边界的编辑面板提供缩放因子控件，
		//固定/虚拟/超单元/位移转角(BOUNDARY_MOVE)/新温度边界不提供，引用面板不可编辑缩放系数
		static bool hasScaleFactor(int id)
		{
			auto boundaryData = manager().getLoadBoundaryData(id);
			if (boundaryData == nullptr)
			{
				return false;
			}
			switch (boundaryData->getLoadBoundaryType())
			{
			case BOUNDARY_IM_DISPLACEMENT:
			case BOUNDARY_IM_SPEED:
			case BOUNDARY_IM_ACC:
				return true;
			case BOUNDARY_IM_TEMP:
			{
				//温度边界分析场方式无大小概念，编辑面板不提供缩放因子
				auto tempData = std::dynamic_pointer_cast<mBoundaryTemp>(boundaryData);
				return !(tempData && tempData->isAnalysisFieldMode_);
			}
			default:
				return false;
			}
		}
		static double scaleOf(int id, int caseID)
		{
			return getCVM->getModelData()->getMapRelationManager().getScaleByCase({ Physical_LoadBoundary, id }, caseID);
		}
		static void setScale(int id, int caseID, double value)
		{
			getCVM->getModelData()->getMapRelationManager().setScaleByCase({ Physical_LoadBoundary, id }, caseID, value);
		}

		static void emitDelete(const QVector<int>& ids)
		{
			MechCommon::mMechGlobalSignals::getInstance()->deleteBoundarysSig(ids);
		}
		static QList<QMetaObject::Connection> connectRefreshSignals(QObject* context, const std::function<void()>& refresh)
		{
			auto* signals = MechCommon::mMechGlobalSignals::getInstance();
			return {
				QObject::connect(signals, &MechCommon::mMechGlobalSignals::deleteBoundarysSig, context, [refresh](const QVector<int>&) { refresh(); }),
				QObject::connect(signals, &MechCommon::mMechGlobalSignals::createUpdateBoundarySig, context, [refresh](bool, int) { refresh(); })
			};
		}

		static void editObject(int id, int caseID) { mInfoWindow::getInstance()->editBoundary(id, caseID); }
		static void selectManagerItem(int id) { mBrowserWindow::getInstance()->getBrowserTree()->selectBoundaryManagerBoundaryItem(id); }
		static void selectCaseItem(int caseID, int id) { mBrowserWindow::getInstance()->getBrowserTree()->selectCaseBoundaryItem(caseID, id); }
		static int firstCaseId(int id)
		{
			if (getCVM == nullptr)
			{
				return 0;
			}
			std::unordered_map<int, double> caseMap =
				getCVM->getModelData()->getMapRelationManager().getMapRelation<std::unordered_map<int, double>>({ Physical_LoadBoundary, id }, Physical_Case);
			return caseMap.empty() ? 0 : caseMap.begin()->first;
		}
	};

	/******************************* 预定义场 *******************************/
	struct MMAINWINDOW_EXPORT mPredefinedFieldPanelTraits : public mManagerPanelTraitsBase
	{
		static QString managerTitle() { return "预定义场管理器"; }
		static QString referenceTitle() { return "预定义场引用"; }
		static QString nameFilterPlaceholder() { return "筛选预定义场名称"; }
		static QString nameHeader() { return "预定义场名称"; }
		static QString matrixEntryText() { return "配置工况预定义场"; }
		static BrowserTreeType matrixTreeType() { return PredefinedFieldManagerParentItem; }

		static mPredefinedFieldManager& manager()
		{
			return getCVM->getModelData()->getManager<mPredefinedFieldManager>(Physical_PredefinedField);
		}

		static std::vector<int> allIds() { return manager().getAllPredefinedFieldID(); }
		static QString nameOf(int id) { return manager().getPredefinedFieldNameByID(id); }
		static QString typeNameOf(int id) { return manager().getPredefinedFieldTypeNameByID(id); }

		static bool isReferenced(int id, int caseID)
		{
			auto referenced = getCVM->getModelData()->getMapRelationManager().getMapRelation<std::set<int>>({ Physical_Case, caseID }, Physical_PredefinedField);
			return referenced.count(id) > 0;
		}
		static void reference(int id, int caseID)
		{
			if (isReferenced(id, caseID))
			{
				return;
			}
			getCVM->getModelData()->getMapRelationManager().appendMapRelation({ Physical_PredefinedField, id }, { Physical_Case, caseID });
			mBrowserWindow::getInstance()->getBrowserTree()->addCasePredefinedFieldChildItem(caseID, id);
			getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_PredefinedField, id);
		}
		static void unreference(int id, int caseID)
		{
			getCVM->getModelData()->getMapRelationManager().deleteMapRelation({ Physical_Case, caseID }, { Physical_PredefinedField, id });
			mBrowserWindow::getInstance()->getBrowserTree()->removeCasePredefinedFieldChildItem(caseID, id);
			getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_PredefinedField, id);
		}

		static bool isSupportedByCase(int id, int caseID)
		{
			if (caseID == 0 || getCVM == nullptr)
			{
				return false;
			}
			mCaseBase* caseData = getCVM->getModelData()->getManager<mCaseManager>(Physical_Case).getCaseData(caseID);
			auto fieldData = manager().getPredefinedFieldData(id);
			if (caseData == nullptr || fieldData == nullptr)
			{
				return false;
			}
			return mTreeView::isPredefinedFieldSupportedByCase(caseData->getCaseType(), fieldData->getPredefinedFieldType());
		}

		static void emitDelete(const QVector<int>& ids)
		{
			MechCommon::mMechGlobalSignals::getInstance()->deletePredefinedFieldsSig(ids);
		}
		static QList<QMetaObject::Connection> connectRefreshSignals(QObject* context, const std::function<void()>& refresh)
		{
			auto* signals = MechCommon::mMechGlobalSignals::getInstance();
			return {
				QObject::connect(signals, &MechCommon::mMechGlobalSignals::deletePredefinedFieldsSig, context, [refresh](const QVector<int>&) { refresh(); }),
				QObject::connect(signals, &MechCommon::mMechGlobalSignals::createUpdatePredefinedFieldSig, context, [refresh](bool, int) { refresh(); })
			};
		}

		static void editObject(int id, int caseID) { mInfoWindow::getInstance()->editPredefinedField(id, caseID); }
		static void selectManagerItem(int id) { mBrowserWindow::getInstance()->getBrowserTree()->selectPredefinedFieldManagerItem(id); }
		static void selectCaseItem(int caseID, int id) { mBrowserWindow::getInstance()->getBrowserTree()->selectCasePredefinedFieldItem(caseID, id); }
		static int firstCaseId(int id)
		{
			if (getCVM == nullptr)
			{
				return 0;
			}
			std::unordered_map<int, double> caseMap =
				getCVM->getModelData()->getMapRelationManager().getMapRelation<std::unordered_map<int, double>>({ Physical_PredefinedField, id }, Physical_Case);
			return caseMap.empty() ? 0 : caseMap.begin()->first;
		}
	};

	/******************************* 接触 *******************************/
	struct MMAINWINDOW_EXPORT mContactPanelTraits : public mManagerPanelTraitsBase
	{
		static QString managerTitle() { return "接触管理器"; }
		static QString referenceTitle() { return "接触引用"; }
		static QString nameFilterPlaceholder() { return "筛选接触名称"; }
		static QString nameHeader() { return "接触名称"; }
		static QString matrixEntryText() { return "配置工况接触"; }
		static BrowserTreeType matrixTreeType() { return ContactParentItem; }

		static mContactManager& manager()
		{
			return getCVM->getModelData()->getManager<mContactManager>(Physical_Contact);
		}

		static std::vector<int> allIds() { return manager().getAllContactID(); }
		static QString nameOf(int id)
		{
			auto data = manager().getContactData(id);
			return data ? data->getContactName() : QString();
		}
		static QString typeNameOf(int id)
		{
			auto data = manager().getContactData(id);
			return data ? data->getContactTypeStr() : QString();
		}

		static bool isReferenced(int id, int caseID)
		{
			auto referenced = getCVM->getModelData()->getMapRelationManager().getMapRelation<std::set<int>>({ Physical_Case, caseID }, Physical_Contact);
			return referenced.count(id) > 0;
		}
		static void reference(int id, int caseID)
		{
			if (isReferenced(id, caseID))
			{
				return;
			}
			getCVM->getModelData()->getMapRelationManager().appendMapRelation({ Physical_Contact, id }, { Physical_Case, caseID });
			MechCommon::mMechGlobalSignals::getInstance()->updateContactTreeView(caseID, id, 0);
		}
		static void unreference(int id, int caseID)
		{
			getCVM->getModelData()->getMapRelationManager().deleteMapRelation({ Physical_Case, caseID }, { Physical_Contact, id });
			MechCommon::mMechGlobalSignals::getInstance()->updateContactTreeView(caseID, id, 1);
		}

		//接触与工况类型无兼容性限制（与矩阵表格一致）
		static bool isSupportedByCase(int id, int caseID) { Q_UNUSED(id); Q_UNUSED(caseID); return true; }
		//除过盈接触外的接触类型强制挂载到所有工况（与矩阵表格规则一致）
		static bool isForceReferenced(int id)
		{
			auto data = manager().getContactData(id);
			return data && data->getContactType() != InterferenceContact;
		}
		static bool isSuppressedObject(int id)
		{
			auto data = manager().getContactData(id);
			return data && data->isSuppressed();
		}

		static void emitDelete(const QVector<int>& ids)
		{
			//接触无批量删除信号：逐个复用createUpdateContactSig(false, id)通知树删除
			for (int id : ids)
			{
				MechCommon::mMechGlobalSignals::getInstance()->createUpdateContactSig(false, id);
			}
		}
		static QList<QMetaObject::Connection> connectRefreshSignals(QObject* context, const std::function<void()>& refresh)
		{
			auto* signals = MechCommon::mMechGlobalSignals::getInstance();
			return {
				QObject::connect(signals, &MechCommon::mMechGlobalSignals::createUpdateContactSig, context, [refresh](bool, int) { refresh(); }),
				QObject::connect(signals, &MechCommon::mMechGlobalSignals::batchCreateContactSig, context, [refresh](std::vector<int>) { refresh(); })
			};
		}

		static void editObject(int id, int caseID)
		{
			Q_UNUSED(caseID);
			mInfoWindow::getInstance()->editContact(id);
		}
		//接触管理器/工况树暂无按id选中接口，编辑面板自身会处理树状态
		static void selectManagerItem(int id) { Q_UNUSED(id); }
		static void selectCaseItem(int caseID, int id) { Q_UNUSED(caseID); Q_UNUSED(id); }
		static int firstCaseId(int id)
		{
			if (getCVM == nullptr)
			{
				return 0;
			}
			std::unordered_map<int, double> caseMap =
				getCVM->getModelData()->getMapRelationManager().getMapRelation<std::unordered_map<int, double>>({ Physical_Contact, id }, Physical_Case);
			return caseMap.empty() ? 0 : caseMap.begin()->first;
		}
	};

	/******************************* 模型修改 *******************************/
	struct MMAINWINDOW_EXPORT mModelFixPanelTraits : public mManagerPanelTraitsBase
	{
		static QString managerTitle() { return "模型修改管理器"; }
		static QString referenceTitle() { return "模型修改引用"; }
		static QString nameFilterPlaceholder() { return "筛选模型修改名称"; }
		static QString nameHeader() { return "模型修改名称"; }
		static QString matrixEntryText() { return "配置工况模型修改"; }
		static BrowserTreeType matrixTreeType() { return ModelFixParentItem; }

		static mModelFixManager& manager()
		{
			return getCVM->getModelData()->getManager<mModelFixManager>(Physical_ModelFix);
		}

		static std::vector<int> allIds() { return manager().getAllModelFixID(); }
		static QString nameOf(int id) { return manager().getModelFixNameByID(id); }
		static QString typeNameOf(int id)
		{
			auto data = manager().getModelFixData(id);
			return data ? data->getModelFixType() : QString();
		}

		static bool isReferenced(int id, int caseID)
		{
			QList<int> ids = getCVM->getModelData()->getMapRelationManager().getMapRelation<QList<int>>({ Physical_Case, caseID }, Physical_ModelFix);
			return ids.contains(id);
		}
		static void reference(int id, int caseID)
		{
			if (isReferenced(id, caseID))
			{
				return;
			}
			//模型修改与工况为有序table映射：追加到末尾
			QList<int> ids = getCVM->getModelData()->getMapRelationManager().getMapRelation<QList<int>>({ Physical_Case, caseID }, Physical_ModelFix);
			getCVM->getModelData()->getMapRelationManager().appendTableMapRelation({ Physical_Case, caseID }, { Physical_ModelFix, id }, ids.size());
			MechCommon::mMechGlobalSignals::getInstance()->updateModelFixTreeView(caseID, id, 0);
		}
		static void unreference(int id, int caseID)
		{
			QList<int> ids = getCVM->getModelData()->getMapRelationManager().getMapRelation<QList<int>>({ Physical_Case, caseID }, Physical_ModelFix);
			int idx = ids.indexOf(id);
			if (idx >= 0)
			{
				getCVM->getModelData()->getMapRelationManager().deleteTabelMapRelation({ Physical_Case, caseID }, { Physical_ModelFix, -1 }, idx);
			}
			MechCommon::mMechGlobalSignals::getInstance()->updateModelFixTreeView(caseID, id, 1);
		}

		//模型修改与工况类型无兼容性限制（与矩阵表格一致）
		static bool isSupportedByCase(int id, int caseID) { Q_UNUSED(id); Q_UNUSED(caseID); return true; }
		static bool isSuppressedObject(int id)
		{
			auto data = manager().getModelFixData(id);
			return data && data->isSuppressed();
		}

		static void emitDelete(const QVector<int>& ids)
		{
			for (int id : ids)
			{
				MechCommon::mMechGlobalSignals::getInstance()->createUpdateModelFixSig(false, id);
			}
		}
		static QList<QMetaObject::Connection> connectRefreshSignals(QObject* context, const std::function<void()>& refresh)
		{
			auto* signals = MechCommon::mMechGlobalSignals::getInstance();
			return {
				QObject::connect(signals, &MechCommon::mMechGlobalSignals::createUpdateModelFixSig, context, [refresh](bool, int) { refresh(); })
			};
		}

		static void editObject(int id, int caseID)
		{
			Q_UNUSED(caseID);
			mInfoWindow::getInstance()->editModelFix(id);
		}
		static void selectManagerItem(int id) { Q_UNUSED(id); }
		static void selectCaseItem(int caseID, int id) { Q_UNUSED(caseID); Q_UNUSED(id); }
		static int firstCaseId(int id)
		{
			if (getCVM == nullptr)
			{
				return 0;
			}
			auto& caseMgr = getCVM->getModelData()->getManager<mCaseManager>(Physical_Case);
			for (int caseID : caseMgr.getAllCaseID())
			{
				if (isReferenced(id, caseID))
				{
					return caseID;
				}
			}
			return 0;
		}
	};
}

//恢复Qt signals宏
#pragma pop_macro("signals")
