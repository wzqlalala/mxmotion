#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mSetManager.h"
#include "mMapRelationEnum.h"
#include "mMapRelationManager.h"
#include "mInfoWindow.h"
#include "AllRenderSymbolManager.h"
#include "mGeneralPickUp.h"
#include "mGetSetTypeTemplate.h"
#include "mGetSetFunction.h"
#include "mDataStatusManager.h"

//Geo
#include "mGeoManager.h"
//#include "mGeometricFeatureAttribute.h" //【OCC摘除·S10/P0-1】退役头死 include 清剿（冻结岛保档，同主线处理）
#include "PickCommonFunction.h"

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

#include "mBoundarySuperelement.h"
#include "mBoundarySuperelementPanel.h"

using namespace MechCommon;
using namespace MechPreData;

namespace MechMainWindow
{
    mBoundarySuperelementPanel::mBoundarySuperelementPanel(QWidget* parent)
        :mBoundaryPanelBase(parent)
    {
        _realtimeUpdateTimer = new QTimer(this);
        _realtimeUpdateTimer->setSingleShot(true);
        connect(_realtimeUpdateTimer, &QTimer::timeout, this, &mBoundarySuperelementPanel::doUpdateBoundarySymbolRealtime);

        initInputs();
    }

    mBoundarySuperelementPanel::~mBoundarySuperelementPanel()
    {
        clearInputs();
    }

    std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> mBoundarySuperelementPanel::getPickedData()
    {
        if (!_pickWidgetController)
            return std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>();
        return _pickWidgetController->getAllData().pickedData;
    }

    void mBoundarySuperelementPanel::initInputs()
    {
        QStringList combonames =
        {
            "约束类型",
        };

        for (int i = 0; i < 1; ++i)
        {
            singleComboBox combobox;
            combobox.initWidget(combonames[i]);
            comboBoxs.append(combobox);
        }

        // 约束类型
        comboBoxs[0].combobox->addItems({ "ASET","BNDFIX1","BNDFREE1" });
        connect(comboBoxs[0].combobox, &QComboBox::currentTextChanged,
            this, &mBoundarySuperelementPanel::slot_constraintTypeChanged);
        comboBoxs[0].combobox->blockSignals(true);
        comboBoxs[0].combobox->setCurrentIndex(0);
        comboBoxs[0].combobox->blockSignals(false);

        // 值
        names1_ = { "DOF1","DOF2","DOF3" };
        names2_ = { "DOF4","DOF5","DOF6" };
        CheckBoxsWidget checks1;
        checks1.initWidget(names1_);
        checkswid.append(checks1);
        CheckBoxsWidget checks2;
        checks2.initWidget(names2_);
        checkswid.append(checks2);
        for (auto& check : checks1.labels)
        {
            connect(check, &QCheckBox::stateChanged, this, &mBoundarySuperelementPanel::slot_checkDegreeOfFreedom);
        }
        for (auto& check : checks2.labels)
        {
            connect(check, &QCheckBox::stateChanged, this, &mBoundarySuperelementPanel::slot_checkDegreeOfFreedom);
        }

        // 通用拾取控件
        _pickWidgetController = mGeneralPickUp::createPickWidget(CBBlue);
        connect(_pickWidgetController, &mPickWidgetController::pickActiveChanged, this, [this](bool isActive) {
            if (isActive)
            {
                emit mMechGlobalSignals::getInstance()->activePickSig(5);
            }
            });
        connect(_pickWidgetController, &mPickWidgetController::dataChanged, this, [this]() {
            updateBoundarySymbolRealtime();
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

    void mBoundarySuperelementPanel::setInputsValue(std::shared_ptr<MechCaseData::mLoadBoundaryBase> boundaryData)
    {
        std::shared_ptr<MechCaseData::mBoundarySuperelement> boundaryPtr = std::dynamic_pointer_cast<MechCaseData::mBoundarySuperelement>(boundaryData);
        _boundaryData = boundaryData;
        _currentBoundary = boundaryPtr;
        if (!boundaryPtr) return;

        comboBoxs[0].combobox->blockSignals(true);
        comboBoxs[0].combobox->setCurrentText(boundaryPtr->_ConstraintType);
        comboBoxs[0].combobox->blockSignals(false);

        // 通用拾取控件持久化策略与集合列表
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

        // 拾取数据反显
        PickWidgetData displayData;
        auto& mapboundary = getCVM->getModelData()->getMapRelationManager();
        int setid = mapboundary.getMapRelation<int>({ Physical_LoadBoundary, boundaryData->getLoadBoundaryId() }, Physical_Set);
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

        // 自由度反显
        // 屏蔽控件信号：setChecked 触发 stateChanged→saveDOF 会把尚未恢复完的
        // 旧边界勾选状态写进当前 _currentBoundary，造成边界间数据串扰
        for (auto& group : checkswid)
        {
            for (auto* check : group.labels)
                check->blockSignals(true);
        }
        checkswid[0].labels[0]->setChecked(boundaryPtr->_DOF[0]);
        checkswid[0].labels[1]->setChecked(boundaryPtr->_DOF[1]);
        checkswid[0].labels[2]->setChecked(boundaryPtr->_DOF[2]);
        checkswid[1].labels[0]->setChecked(boundaryPtr->_DOF[3]);
        checkswid[1].labels[1]->setChecked(boundaryPtr->_DOF[4]);
        checkswid[1].labels[2]->setChecked(boundaryPtr->_DOF[5]);
        for (auto& group : checkswid)
        {
            for (auto* check : group.labels)
                check->blockSignals(false);
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

    void mBoundarySuperelementPanel::slot_constraintTypeChanged(QString type)
    {
        Q_UNUSED(type)
        saveConstraintType();
        updateBoundarySymbolRealtime();
    }

    void mBoundarySuperelementPanel::slot_checkDegreeOfFreedom(int state)
    {
        Q_UNUSED(state)
        saveDOF();
        updateBoundarySymbolRealtime();
    }

    QVector<QWidget*> mBoundarySuperelementPanel::getTargetInputs(FoldSection* fathertar)
    {
        fatar_ = fathertar;
        QVector<QWidget*>wds;
        wds.append(comboBoxs[0].propwid);
        if (_pickWidgetController)
        {
            wds.append(_pickWidgetController->widget());
        }
        return wds;
    }

    QVector<QWidget*> mBoundarySuperelementPanel::getValueInputs(FoldSection* fatherval)
    {
        faval_ = fatherval;
        QVector<QWidget*>wds;
        for (auto chec : checkswid)
        {
            wds.append(chec.propwid);
        }
        return wds;
    }

    void mBoundarySuperelementPanel::clearInputs()
    {
        for (auto combo : comboBoxs)
        {
            combo.clear();
        }
        comboBoxs.clear();

        for (auto chec : checkswid)
        {
            chec.clear();
        }
        checkswid.clear();

        if (_pickWidgetController)
        {
            _pickWidgetController->setPickActive(false);
            delete _pickWidgetController;
            _pickWidgetController = nullptr;
        }

        _currentBoundary.reset();

        if (_realtimeUpdateTimer)
        {
            delete _realtimeUpdateTimer;
            _realtimeUpdateTimer = nullptr;
        }
    }

    void mBoundarySuperelementPanel::saveConstraintType()
    {
        if (!_currentBoundary)
            return;
        _currentBoundary->_ConstraintType = comboBoxs[0].combobox->currentText();
        notifyBoundaryChanged();
    }

    void mBoundarySuperelementPanel::saveDOF()
    {
        if (!_currentBoundary)
            return;

        _currentBoundary->_DOF[0] = checkswid[0].labels[0]->isChecked();
        _currentBoundary->_DOF[1] = checkswid[0].labels[1]->isChecked();
        _currentBoundary->_DOF[2] = checkswid[0].labels[2]->isChecked();
        _currentBoundary->_DOF[3] = checkswid[1].labels[0]->isChecked();
        _currentBoundary->_DOF[4] = checkswid[1].labels[1]->isChecked();
        _currentBoundary->_DOF[5] = checkswid[1].labels[2]->isChecked();

        notifyBoundaryChanged();
    }

    void mBoundarySuperelementPanel::notifyBoundaryChanged()
    {
        if (!_boundaryData)
            return;
        // 仅通知外部刷新树/渲染，不再调用 storeBoundaryValue 全量保存
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_LoadBoundary, _boundaryData->getLoadBoundaryId());
    }

    void mBoundarySuperelementPanel::applyAllowedPickTypes()
    {
        if (!_pickWidgetController) return;
        // 超单元边界：几何点/边/面/实体 + 节点
        _pickWidgetController->setAllowedTypes(&pickGeoPointEdgeFaceSolidNodePart);
        _pickWidgetController->setPickType();
    }

    void mBoundarySuperelementPanel::deactivatePickAndCommit()
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

        // 切出收尾：取消挂起的防抖刷新；提交若产生新变更（定时器被重新启动）则同步冲刷
        // 一次（最终数据未变化时签名自动跳过渲染），避免切出 100ms 后再触发一次
        // 无意义的符号全量重建
        if (_realtimeUpdateTimer && _realtimeUpdateTimer->isActive())
        {
            _realtimeUpdateTimer->stop();
            doUpdateBoundarySymbolRealtime();
        }
    }

    void mBoundarySuperelementPanel::updateBoundarySymbolRealtime()
    {
        if (!_pickWidgetController || !_currentBoundary)
            return;
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;

        // 通过防抖定时器合并连续触发
        if (!_realtimeUpdateTimer)
            return;
        _realtimeUpdateTimer->start(100);
    }

    void mBoundarySuperelementPanel::doUpdateBoundarySymbolRealtime()
    {
        // 1. 从通用拾取控件内存实时汇总作用对象坐标（拾取期间尚未 commit 到映射，
        //    不能走 rendBoundary 读映射，必须直接基于控件当前数据计算）。
        QVector<QVector3D> positions;
        collectRenderPositions(positions);

        // 2. 根据当前自由度构造约束值
        QMap<uint32_t, float> value;
        if (_currentBoundary->_DOF[0]) value[1] = 1;
        if (_currentBoundary->_DOF[1]) value[2] = 2;
        if (_currentBoundary->_DOF[2]) value[3] = 3;
        if (_currentBoundary->_DOF[3]) value[4] = 4;
        if (_currentBoundary->_DOF[4]) value[5] = 5;
        if (_currentBoundary->_DOF[5]) value[6] = 6;

        // 最终数据签名：边界ID+位置集+自由度分量未变化时跳过符号赋值与
        // UpdateAllSymbol 全量重建（进入面板防抖触发/切出冲刷时数据往往与上次相同）
        size_t sig = AllRenderSymbolManager::beginRenderSignature();
        AllRenderSymbolManager::combineRenderSignature(sig, static_cast<size_t>(_boundaryData->getLoadBoundaryId()));
        AllRenderSymbolManager::combineRenderSignature(sig, positions);
        AllRenderSymbolManager::combineRenderSignature(sig, value);
        if (sig == _lastRenderSig)
        {
            return;
        }
        _lastRenderSig = sig;

        // 3. 直接赋予边界符号（key=boundary_<id>，同名覆盖；空 positions 等同清除残留符号）
        AllRenderSymbolManager::getInstance()->assignBoundary(
            _boundaryData, positions, value, QVector3D(0.0f, 255.0f, 255.0f));

        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        if (symbolController) symbolController->UpdateAllSymbol();
        getCVM->getPreRenderWidget()->updatePaint();
    }

    void mBoundarySuperelementPanel::collectRenderPositions(QVector<QVector3D>& outPositions)
    {
        if (!_pickWidgetController)
            return;

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
