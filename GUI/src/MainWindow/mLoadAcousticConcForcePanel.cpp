#include "mLoadAcousticConcForcePanel.h"
#include "mMechGlobalSignals.h"
#include "mDataStatusManager.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mSetManager.h"
#include "mMapRelationEnum.h"
#include "mMapRelationManager.h"
#include "mUnitSystemManager.h"
#include "mInfoWindow.h"
#include "PickCommonFunction.h"
#include "mWorkingConditions.h"
#include "mGetSetFunction.h"
#include "mGeneralPickUp.h"
#include "mGetSetTypeTemplate.h"
#include "AllRenderSymbolManager.h"

//Geo
#include "mGeoManager.h"
//#include "mGeometricFeatureAttribute.h" //【OCC摘除·S10/P0-1】退役头死 include 清剿（冻结岛保档，同主线处理）

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
#include "mxr/label/mSymbolController.h"
#include "mxr/view/mShapeRender.h"

//Basic
#include<mMixSet.h>
#include<mMeshData.h>
#include<mGeoSet.h>
#include<mNode.h>

#include "mLoadBoltPanel.h"
#include "mOtherFunction.h"

using namespace MechCommon;
using namespace MechPreData;

namespace MechMainWindow
{
    mLoadAcousticConcForcePanel::mLoadAcousticConcForcePanel(QWidget* parent)
        :mLoadPanelBase(parent)
    {
        initInputs();
    }

    mLoadAcousticConcForcePanel::~mLoadAcousticConcForcePanel()
    {
        clearInputs();
    }

    void mLoadAcousticConcForcePanel::initInputs()
    {
        // 大小输入框
        InputWidgetSet inp;
        inp.initWidget("大小", false);
        inp.labelname->setText("大小");
        inp.labelunit->setVisible(false);
        inputWidgets.append(inp);
        connect(inp.lineeditvalue, &mCalcLineEdit::editingFinished,
            this, &mLoadAcousticConcForcePanel::slot_valueChanged);

        // 通用拾取控件：声学载荷仅允许几何点、节点
        _pickWidgetController = mGeneralPickUp::createOneDataWidget(CBBlue);
        connect(_pickWidgetController, &mPickWidgetController::pickActiveChanged, this, [this](bool isActive) {
            if (isActive)
            {
                emit mMechGlobalSignals::getInstance()->activePickSig(5);
            }
            });
        connect(_pickWidgetController, &mPickWidgetController::dataChanged, this, [this]() {
            updateLoadSymbolRealtime();
            //判断拾取控件是否为空 以设置标志位
            if (_pickWidgetController->getAllData().isEmpty())
            {
                _currentLoad->setPickState(1);
            }
            else
            {
                _currentLoad->setPickState(2);
            }
            //发送更新信号
            notifyLoadChanged();
            });

        _pickWidgetController->setSetResolver(&resolveSetToTypedData);
        _pickWidgetController->setAllowedTypes(&pickGeoPointAndNode);
        _pickWidgetController->setDefultTip("选择对象");
    }

    void mLoadAcousticConcForcePanel::clearInputs()
    {
        for (auto inp : inputWidgets)
        {
            inp.clear();
        }
        inputWidgets.clear();

        if (_pickWidgetController)
        {
            _pickWidgetController->setPickActive(false);
            delete _pickWidgetController;
            _pickWidgetController = nullptr;
        }

        _currentLoad.reset();
    }

    void mLoadAcousticConcForcePanel::setInputsValue(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData)
    {
        std::shared_ptr<MechCaseData::mLoadAcousticConcForce> acousticPtr = std::dynamic_pointer_cast<MechCaseData::mLoadAcousticConcForce>(loadData);
        _currentLoad = acousticPtr;
        if (!acousticPtr) return;

        // 大小反显
        QString valueStr = "";
        if (!acousticPtr->value_.isNull() && acousticPtr->value_.isValid())
        {
            valueStr = MxFunctions::mNumberToString(acousticPtr->value_.toDouble());
        }
        inputWidgets[0].lineeditvalue->setExpression(valueStr);

        // 通用拾取控件持久化策略与集合列表
        if (_pickWidgetController)
        {
            auto strategy = std::make_shared<MapRelationPersistenceStrategy>(
                MxFunctions::Physical_LoadBoundary,
                _loadData->getLoadBoundaryId());
            _pickWidgetController->setPersistenceStrategy(strategy);
            _pickWidgetController->setSetResolver(&resolveSetToTypedData);

            QHash<QString, QString> setToType = getSetNamesList({ SetType::GeoPoint, SetType::Node });
            setToType = getOneDataSet(setToType);
            _pickWidgetController->setSelectSetList(setToType);
            _pickWidgetController->setPickType();
        }

        // 作用对象反显
        PickWidgetData displayData;
        auto& mapload = getCVM->getModelData()->getMapRelationManager();
        int setid = mapload.getMapRelation<int>({ Physical_LoadBoundary,loadData->getLoadBoundaryId() }, Physical_Set);
        if (setid != -1)
        {
            mSetBase* set = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set).getSetData(setid);
            if (set && set->getInternal())
            {
                auto mixset = dynamic_cast<mMixSet*>(set);
                if (mixset) displayData.pickedData = mixset->getMixSetData();
            }
            else if (set)
            {
                displayData.selectedSetNames = { set->getSetName() };
            }
        }

        if (_pickWidgetController)
        {
            // displayData 默认不重画高亮；若控件此前已处于激活态，setPickActive(true) 会直接返回，
            // 该情况下需显式重画一次高亮以匹配反显数据。整体保证进入面板只做一次全量高亮。
            const bool wasActive = _pickWidgetController->isPickActive();
            _pickWidgetController->displayData(displayData);
            _pickWidgetController->setPickActive(true);
            if (wasActive)
            {
                _pickWidgetController->refreshHighlight();
            }
        }

        //判断拾取控件是否为空 以设置标志位
        if (_pickWidgetController->getAllData().isEmpty())
        {
            _currentLoad->setPickState(1);
        }
        else
        {
            _currentLoad->setPickState(2);
        }
        //发送更新信号
        notifyLoadChanged();
        // 延迟刷新符号
        QMetaObject::invokeMethod(this, &mLoadAcousticConcForcePanel::updateLoadSymbolRealtime, Qt::QueuedConnection);
    }

    QVector<QWidget*> mLoadAcousticConcForcePanel::getTargetInputs(FoldSection* fathertar)
    {
        fatar_ = fathertar;
        QVector<QWidget*>wds;
        if (_pickWidgetController)
        {
            wds.append(_pickWidgetController->widget());
        }
        return wds;
    }

    QVector<QWidget*> mLoadAcousticConcForcePanel::getValueInputs(FoldSection* fatherval)
    {
        faval_ = fatherval;
        QVector<QWidget*> wds;
        wds.append(inputWidgets[0].propwid);
        return wds;
    }

    std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> mLoadAcousticConcForcePanel::getPickedData()
    {
        if (!_pickWidgetController)
            return std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>();
        return _pickWidgetController->getAllData().pickedData;
    }

    void mLoadAcousticConcForcePanel::slot_valueChanged()
    {
        saveValue();
    }

    void mLoadAcousticConcForcePanel::saveValue()
    {
        if (!_currentLoad)
            return;

        QString strV = inputWidgets[0].lineeditvalue->text().trimmed();
        if (!strV.isEmpty())
        {
            _currentLoad->value_ = QVariant::fromValue<double>(strV.toDouble());
        }
        else
        {
            _currentLoad->value_ = QVariant();
        }

        notifyLoadChanged();
    }

    void mLoadAcousticConcForcePanel::notifyLoadChanged()
    {
        if (!_loadData)
            return;

        // 仅通知外部刷新树/渲染，不再调用 storeLoadValue 全量保存
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_LoadBoundary, _loadData->getLoadBoundaryId());
    }

    void mLoadAcousticConcForcePanel::deactivatePickAndCommit()
    {
        // 面板为单例复用，切换载荷/面板时不会走析构/clearInputs，
        // 由统一退出出口（unEditLoad）调用，失活并提交当前拾取数据。
        const bool renderValid = (getCVM && getCVM->getPreRenderWidget());
        if (_pickWidgetController)
        {
            _pickWidgetController->setKeepHighlightOnDeactivate(false);
            const bool wasActive = _pickWidgetController->isPickActive();
            if (wasActive) _pickWidgetController->setPickActive(false);
            if (renderValid && !wasActive)
            {
                _pickWidgetController->clearAllHighlight();
            }
            _pickWidgetController->setKeepHighlightOnDeactivate(true);
        }
    }

    void mLoadAcousticConcForcePanel::updateLoadSymbolRealtime()
    {
        if (!_pickWidgetController || !_currentLoad)
            return;
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;

        QVector<QVector3D> positions = collectRenderPositions();

        // 使用 AllRenderSymbolManager 实时渲染声学载荷符号
        AllRenderSymbolManager::getInstance()->assignThermalLoad(
            _currentLoad, positions, "", QVector3D(255.0f, 255.0f, 0.0f));

        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        if (symbolController) symbolController->UpdateAllSymbol();
        getCVM->getPreRenderWidget()->updatePaint();
    }

    QVector<QVector3D> mLoadAcousticConcForcePanel::collectRenderPositions()
    {
        QVector<QVector3D> objs;

        if (!_pickWidgetController)
            return objs;

        const PickWidgetData allData = _pickWidgetController->getAllData();
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> merged = allData.pickedData;

        auto& setManager = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set);
        for (const QString& setName : allData.selectedSetNames)
        {
            mSetBase* setBase = setManager.getSetData(setName);
            if (!setBase) continue;
            if (auto mixSet = dynamic_cast<MechSetData::mMixSet*>(setBase))
            {
                for (const auto& inner : mixSet->getMixSetData())
                {
                    auto& vec = merged[inner.first];
                    vec.insert(vec.end(), inner.second.begin(), inner.second.end());
                }
                continue;
            }
            switch (setBase->getSetType())
            {
            case MxFunctions::Node:
                if (auto s = dynamic_cast<mNode*>(setBase)) { auto v = s->getNodeData(); auto& d = merged[PickDataType::Node]; d.insert(d.end(), v.begin(), v.end()); }
                break;
            case MxFunctions::GeoPoint:
                if (auto s = dynamic_cast<mGeoSet*>(setBase)) { auto v = s->getGeoSetData(); auto& d = merged[PickDataType::GeoPoint]; d.insert(d.end(), v.begin(), v.end()); }
                break;
            default:
                break;
            }
        }

        auto meshData = getCVM->getModelData()->getMeshData();
        auto geoManager = getCVM->getModelData()->getGeoManager();
        for (const auto& pair : merged)
        {
            if (pair.second.empty()) continue;

            switch (pair.first)
            {
            case PickDataType::Node:
                for (const auto& d : pair.second)
                {
                    QVector3D pos = meshData->getVectorById(d.result_id);
                    if (!pos.isNull())
                        objs.push_back(pos);
                }
                break;
            case PickDataType::GeoPoint:
            {
                std::vector<int> ids;
                ids.reserve(pair.second.size());
                for (const auto& d : pair.second)
                    ids.push_back(d.result_id);
                auto points = geoManager->CalculateVertexAttr(ids);
                for (auto p : points) objs.push_back(p.m_coor);
                break;
            }
            default:
                break;
            }
        }

        return objs;
    }
}