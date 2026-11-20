#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mSetManager.h"
#include "mMapRelationEnum.h"
#include "mMapRelationManager.h"
#include "mInfoWindow.h"
#include "PickCommonFunction.h"
#include "mWorkingConditions.h"
#include "AllRenderSymbolManager.h"
#include "mGeneralPickUp.h"
#include "mGetSetTypeTemplate.h"
#include "mGetSetFunction.h"
#include "mDataStatusManager.h"

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

#include "mBoundaryFixed.h"
#include "mBoundaryFixedPanel.h"

using namespace MechCommon;
using namespace MechPreData;

namespace MechMainWindow
{
    mBoundaryFixedPanel::mBoundaryFixedPanel(QWidget* parent)
        :mBoundaryPanelBase(parent)
    {
        initInputs();
    }

    mBoundaryFixedPanel::~mBoundaryFixedPanel()
    {
        clearInputs();
    }

    std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> mBoundaryFixedPanel::getPickedData()
    {
        if (!_pickWidgetController)
            return std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>();
        return _pickWidgetController->getAllData().pickedData;
    }

    void mBoundaryFixedPanel::initInputs()
    {
        // 固定约束类型
        QStringList combonames = { "固定约束类型" };
        for (int i = 0; i < 1; ++i)
        {
            singleComboBox combobox;
            combobox.initWidget(combonames[i]);
            comboBoxs.append(combobox);
        }

        comboBoxs[0].combobox->addItems({
            "完全固定",
            "铰链",
            "关于X轴对称",
            "关于Y轴对称",
            "关于Z轴对称",
            "关于X轴反对称",
            "关于Y轴反对称",
            "关于Z轴反对称"
            });
        connect(comboBoxs[0].combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mBoundaryFixedPanel::slot_valueFixTypeChanged);
        comboBoxs[0].combobox->blockSignals(true);
        comboBoxs[0].combobox->setCurrentIndex(0);
        comboBoxs[0].combobox->blockSignals(false);

        _label.initWidget("自由度");
        _label.labelname->setText("自由度");
        _label.labelvalue->setText("U1=U2=U3=UR1=UR2=UR3=0");

        // 通用拾取控件
        _pickWidgetController = mGeneralPickUp::createPickWidget(CBBlue);
        connect(_pickWidgetController, &mPickWidgetController::pickActiveChanged, this, [this](bool isActive) {
            if (isActive)
            {
                emit mMechGlobalSignals::getInstance()->activePickSig(5);
            }
            });
        connect(_pickWidgetController, &mPickWidgetController::dataChanged, this, [this]() {
            // dataChanged 在拾取过程中会多次触发；用单次触发 + 延迟调用确保
            // 控件内部模型已最终写入（ PickOne 模式在 onViewportPickFinished 中
            // 先 clear 再 setPickedData，立即渲染可能读到空数据）。
            QMetaObject::invokeMethod(this, &mBoundaryFixedPanel::updateBoundarySymbolRealtime, Qt::QueuedConnection);
            //判断拾取控件是否为空 以设置标志位
            if (_pickWidgetController->getAllData().isEmpty())
            {
                _currentBoundary->setPickState(1);
            }
            else
            {
                _currentBoundary->setPickState(2);
            }
            //发送更新信号
            notifyBoundaryChanged();
            });

        _pickWidgetController->setSetResolver(&resolveSetToTypedData);
        _pickWidgetController->setDefultTip("选择对象");

        applyAllowedPickTypes();
    }

    void mBoundaryFixedPanel::slot_valueFixTypeChanged(int idx)
    {
        QStringList labels = {
            "U1=U2=U3=UR1=UR2=UR3=0",   // 完全固定
            "U1=U2=U3=0",                // 铰链
            "U1=UR2=UR3=0",              // 关于X轴对称
            "U2=UR1=UR3=0",              // 关于Y轴对称
            "U3=UR1=UR2=0",              // 关于Z轴对称
            "U2=U3=UR1=0",               // 关于X轴反对称
            "U1=U3=UR2=0",               // 关于Y轴反对称
            "U1=U2=UR3=0"                // 关于Z轴反对称
        };
        if (idx >= 0 && idx < labels.size())
        {
            _label.labelvalue->setText(labels[idx]);
        }

        saveFixedType();
        updateBoundarySymbolRealtime();
    }

    void mBoundaryFixedPanel::clearInputs()
    {
        for (auto combo : comboBoxs)
        {
            combo.clear();
        }
        comboBoxs.clear();
        _label.clear();

        if (_pickWidgetController)
        {
            _pickWidgetController->setPickActive(false);
            delete _pickWidgetController;
            _pickWidgetController = nullptr;
        }

        _currentBoundary.reset();
    }

    void mBoundaryFixedPanel::setInputsValue(std::shared_ptr<MechCaseData::mLoadBoundaryBase> boundaryData)
    {
        std::shared_ptr<MechCaseData::mBoundaryFixed> boundaryPtr = std::dynamic_pointer_cast<MechCaseData::mBoundaryFixed>(boundaryData);
        _currentBoundary = boundaryPtr;
        if (!boundaryPtr) return;

        int typeIndex = boundaryPtr->_fixedType;

        comboBoxs[0].combobox->blockSignals(true);
        comboBoxs[0].combobox->setCurrentIndex(typeIndex);
        comboBoxs[0].combobox->blockSignals(false);

        QStringList labels = {
            "U1=U2=U3=UR1=UR2=UR3=0",
            "U1=U2=U3=0",
            "U1=UR2=UR3=0",
            "U2=UR1=UR3=0",
            "U3=UR1=UR2=0",
            "U2=U3=UR1=0",
            "U1=U3=UR2=0",
            "U1=U2=UR3=0"
        };
        if (typeIndex >= 0 && typeIndex < labels.size())
        {
            _label.labelvalue->setText(labels[typeIndex]);
        }

        // 持久化策略绑定
        if (_pickWidgetController)
        {
            auto strategy = std::make_shared<MapRelationPersistenceStrategy>(
                MxFunctions::Physical_LoadBoundary,
                boundaryData->getLoadBoundaryId());
            _pickWidgetController->setPersistenceStrategy(strategy);
            _pickWidgetController->setSetResolver(&resolveSetToTypedData);

            QHash<QString, QString> setToType = getSetNamesList({ SetType::GeoPoint, SetType::GeoEdge, SetType::GeoFace, SetType::GeoSolid, SetType::Node });
            _pickWidgetController->setSelectSetList(setToType);

            applyAllowedPickTypes();
        }

        // 反显集合
        PickWidgetData displayData;
        auto& mapboundary = getCVM->getModelData()->getMapRelationManager();
        int setid = mapboundary.getMapRelation<int>({ Physical_LoadBoundary,boundaryData->getLoadBoundaryId() }, Physical_Set);
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
            _currentBoundary->setPickState(1);
        }
        else
        {
            _currentBoundary->setPickState(2);
        }
        //发送更新信号
        notifyBoundaryChanged();
        updateBoundarySymbolRealtime();
    }

    QVector<QWidget*> mBoundaryFixedPanel::getTargetInputs(FoldSection* fathertar)
    {
        fatar_ = fathertar;
        QVector<QWidget*>wds;
        if (_pickWidgetController)
        {
            wds.append(_pickWidgetController->widget());
        }
        return wds;
    }

    QVector<QWidget*> mBoundaryFixedPanel::getValueInputs(FoldSection* fatherval)
    {
        faval_ = fatherval;
        QVector<QWidget*>wds;
        wds.append(comboBoxs[0].propwid);
        wds.append(_label.propwid);
        return wds;
    }

    void mBoundaryFixedPanel::saveFixedType()
    {
        if (!_currentBoundary)
        {
            return;
        }
        _currentBoundary->_fixedType = comboBoxs[0].combobox->currentIndex();
        notifyBoundaryChanged();
    }

    void mBoundaryFixedPanel::notifyBoundaryChanged()
    {
        if (!_boundaryData)
        {
            return;
        }
        // 仅通知外部刷新树/渲染，不再调用 storeBoundaryValue 全量保存
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_LoadBoundary, _boundaryData->getLoadBoundaryId());
    }

    void mBoundaryFixedPanel::applyAllowedPickTypes()
    {
        if (!_pickWidgetController) return;
        // 固定约束：几何点/边/面/实体 + 节点
        _pickWidgetController->setAllowedTypes(&pickGeoPointEdgeFaceSolidNodePart);
        _pickWidgetController->setPickType();
    }

    void mBoundaryFixedPanel::deactivatePickAndCommit()
    {
        // 面板为单例复用，切换边界/面板时不会走析构/clearInputs，
        // 由统一退出出口（unEditBoundary）调用，失活并提交当前拾取数据。
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

    void mBoundaryFixedPanel::updateBoundarySymbolRealtime()
    {
        if (!_pickWidgetController || !_currentBoundary)
            return;
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;

        // 1. 从通用拾取控件内存实时汇总作用对象坐标（拾取期间尚未 commit 到映射，
        //    不能走 rendBoundary 读映射，必须直接基于控件当前数据计算）。
        QVector<QVector3D> positions;
        collectRenderPositions(positions);

        // 2. 根据当前固定约束类型构造自由度约束值
        QMap<uint32_t, float> value;
        switch (_currentBoundary->_fixedType)
        {
        case 0: value = { {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}, {6, 0} }; break; // 完全固定
        case 1: value = { {1, 0}, {2, 0}, {3, 0} }; break;                         // 铰链
        case 2: value = { {1, 0}, {5, 0}, {6, 0} }; break;                         // 关于X轴对称
        case 3: value = { {2, 0}, {4, 0}, {6, 0} }; break;                         // 关于Y轴对称
        case 4: value = { {3, 0}, {4, 0}, {5, 0} }; break;                         // 关于Z轴对称
        case 5: value = { {2, 0}, {3, 0}, {4, 0} }; break;                         // 关于X轴反对称
        case 6: value = { {1, 0}, {3, 0}, {5, 0} }; break;                         // 关于Y轴反对称
        case 7: value = { {1, 0}, {2, 0}, {6, 0} }; break;                         // 关于Z轴反对称
        default: break;
        }

        // 3. 直接赋予边界符号（key=boundary_<id>，同名覆盖；空 positions 等同清除残留符号）
        AllRenderSymbolManager::getInstance()->assignBoundary(
            _boundaryData, positions, value, QVector3D(255.0f, 165.0f, 0.0f));

        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        if (symbolController) symbolController->UpdateAllSymbol();
        getCVM->getPreRenderWidget()->updatePaint();
    }

    void mBoundaryFixedPanel::collectRenderPositions(QVector<QVector3D>& outPositions)
    {
        const PickWidgetData allData = _pickWidgetController->getAllData();
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> merged = allData.pickedData;

        auto& setManager = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set);
        auto appendIds = [](MxFunctions::PickDataType type, const std::vector<int>& ids,
            std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& dst) {
            auto& vec = dst[type];
            for (int id : ids)
                vec.emplace_back(id, -1, -1);
            };

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
                if (auto s = dynamic_cast<mNode*>(setBase)) { auto v = s->getNodeData(); auto& d = merged[MxFunctions::PickDataType::Node]; d.insert(d.end(), v.begin(), v.end()); }
                break;
            case MxFunctions::GeoPoint:
            case MxFunctions::GeoEdge:
            case MxFunctions::GeoFace:
            case MxFunctions::GeoSolid:
                if (auto s = dynamic_cast<mGeoSet*>(setBase)) appendIds(SetTypeToPickDataType(setBase->getSetType()), s->getGeoSetData(), merged);
                break;
            default:
                break;
            }
        }

        auto meshData = getCVM->getModelData()->getMeshData();
        auto geoManager = getCVM->getModelData()->getGeoManager();
        auto geoData = getCVM->getModelData()->getGeoData();

        for (const auto& pair : merged)
        {
            std::vector<int> ids;
            ids.reserve(pair.second.size());
            for (const auto& d : pair.second)
                ids.push_back(d.result_id);
            if (ids.empty()) continue;

            switch (pair.first)
            {
            case MxFunctions::PickDataType::Node:
                for (int id : ids)
                {
                    QVector3D pos = meshData->getVectorById(id);
                    if (!pos.isNull())
                        outPositions.push_back(pos);
                }
                break;
            case MxFunctions::PickDataType::GeoPoint:
            {
                auto points = geoManager->CalculateVertexAttr(ids);
                for (auto p : points) outPositions.push_back(p.m_coor);
                break;
            }
            case MxFunctions::PickDataType::GeoEdge:
            {
                QVector<int> pids; for (int id : ids) pids.append(id);
                auto coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(1, pids);
                for (auto c : coords) outPositions.push_back(c);
                break;
            }
            case MxFunctions::PickDataType::GeoFace:
            {
                QVector<int> pids; for (int id : ids) pids.append(id);
                auto coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(2, pids);
                for (auto c : coords) outPositions.push_back(c);
                break;
            }
            case MxFunctions::PickDataType::GeoSolid:
            {
                QVector<int> pids; for (int id : ids) pids.append(id);
                auto coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(3, pids);
                for (auto c : coords) outPositions.push_back(c);
                break;
            }
            default:
                break;
            }
        }
    }
}