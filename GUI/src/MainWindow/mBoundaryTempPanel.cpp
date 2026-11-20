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

#include "mBoundaryTemp.h"
#include "mBoundaryTempPanel.h"
#include "mOtherFunction.h"

using namespace MechCommon;
using namespace MechPreData;
namespace MechMainWindow
{
    mBoundaryTempPanel::mBoundaryTempPanel(QWidget* parent)
        :mBoundaryPanelBase(parent)
    {
        _realtimeUpdateTimer = new QTimer(this);
        _realtimeUpdateTimer->setSingleShot(true);
        connect(_realtimeUpdateTimer, &QTimer::timeout, this, &mBoundaryTempPanel::doUpdateBoundarySymbolRealtime);

        initInputs();
    }

    mBoundaryTempPanel::~mBoundaryTempPanel()
    {
        clearInputs();
    }

    std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> mBoundaryTempPanel::getPickedData()
    {
        if (!_pickWidgetController)
            return std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>();
        return _pickWidgetController->getAllData().pickedData;
    }

    void mBoundaryTempPanel::initInputs()
    {
        // 定义方式下拉框：0=统一赋值，1=分析场
        _defineTypeCombo.initWidget("定义方式");
        _defineTypeCombo.combobox->addItem("统一赋值");
        _defineTypeCombo.combobox->addItem("分析场");
        _defineTypeCombo.combobox->setFixedHeight(30);
        _defineTypeCombo.combobox->setCurrentIndex(0);
        connect(_defineTypeCombo.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mBoundaryTempPanel::slot_defineTypeChanged);

        // 分析场方式：已有分析场下拉框（无新建按钮，仅选择已有分析场）
        _analysisFieldCombo.initWidget("分析场");
        _analysisFieldCombo.combobox->setFixedHeight(30);
        _analysisFieldCombo.combobox->addItem("");
        _analysisFieldTable[""] = -1;
        auto allAnalysisField = getCVM->getModelData()->getManager<MechDiscreteFieldData::mFieldManager>(Physical_Field).getAllFieldID(MxFunctions::FieldType::Analysis);
        for (auto fieldid : allAnalysisField)
        {
            QString fieldname = getCVM->getModelData()->getManager<MechDiscreteFieldData::mFieldManager>(Physical_Field).getFieldNameByID(fieldid);
            _analysisFieldCombo.combobox->addItem(fieldname);
            _analysisFieldTable[fieldname] = fieldid;
        }
        connect(_analysisFieldCombo.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mBoundaryTempPanel::slot_analysisFieldChanged);

        // 温度输入框
        _inp.initWidget("温度", false);
        connect(_inp.lineeditvalue, &mCalcLineEdit::editingFinished,
            [this]() {
                saveTemp();
            });
        _inp.labelname->setText("温度");

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

        // 函数
        _functable.clear();
        combofunc.initWidget("函数");
        combofunc.pushbtn->setIcon(QIcon(":/mMainWidgets/Icon/BrowserTree/hanshu.png"));
        combofunc.pushbtn->setFixedSize(30, 30);
        combofunc.pushbtn->setIconSize(QSize(18, 18));
        combofunc.combobox->setFixedHeight(30);
        auto allFunc = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getAllFunctionID();
        _functable[""] = -1;
        combofunc.combobox->addItem("");
        for (auto funcid : allFunc)
        {
            QString funcname = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionNameByID(funcid);
            combofunc.combobox->addItem(funcname);
            _functable[funcname] = funcid;
        }
        connect(combofunc.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mBoundaryTempPanel::slot_funcChanged);
        connect(combofunc.pushbtn, &QPushButton::clicked, this, [this]() {
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createFunction(2);

            combofunc.combobox->clear();
            _functable.clear();
            auto allFunc = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getAllFunctionID();
            _functable[""] = -1;
            combofunc.combobox->addItem("");
            for (auto funcid : allFunc)
            {
                QString funcname = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionNameByID(funcid);
                combofunc.combobox->addItem(funcname);
                _functable[funcname] = funcid;
            }
            combofunc.combobox->setCurrentIndex(allFunc.size() > 0 ? allFunc.size() : 0);
            saveFunc();
            });

        // 缩放因子
        _scale.initWidget("缩放因子", false);
        _scale.labelname->setText("缩放因子");
        connect(_scale.lineeditvalue, &mCalcLineEdit::editingFinished,
            [this]() {
                saveScale();
            });
    }

    void mBoundaryTempPanel::slot_funcChanged(int idx)
    {
        Q_UNUSED(idx)
        saveFunc();
    }

    void mBoundaryTempPanel::slot_defineTypeChanged(int idx)
    {
        if (!_currentBoundary)
            return;

        _currentBoundary->isAnalysisFieldMode_ = (idx == 1);

        // 分析场：无作用对象，隐藏拾取控件并清空拾取/高亮；统一赋值恢复
        if (_pickWidgetController)
        {
            if (idx == 1)
            {
                _pickWidgetController->setPickWidgetVisible(false);
                _pickWidgetController->setCloudChartMode(true);
                _pickWidgetController->clearAllHighlight();
                PickWidgetData empty;
                _pickWidgetController->replacePickedDataAndClearSets(empty.pickedData);
            }
            else
            {
                _pickWidgetController->setPickWidgetVisible(true);
                _pickWidgetController->setCloudChartMode(false);
                _pickWidgetController->setPickActive(true);
            }
        }

        updateBoundarySymbolRealtime();
        notifyBoundaryChanged();
        // 定义方式切换导致作用对象折叠框显隐变化，通知外层重建面板
        emit sg_rebuildPanel();
    }

    void mBoundaryTempPanel::slot_analysisFieldChanged(int idx)
    {
        Q_UNUSED(idx)
        saveAnalysisField();
    }

    void mBoundaryTempPanel::clearInputs()
    {
        _inp.clear();
        _defineTypeCombo.clear();
        _analysisFieldCombo.clear();
        _analysisFieldTable.clear();

        if (_pickWidgetController)
        {
            _pickWidgetController->setPickActive(false);
            delete _pickWidgetController;
            _pickWidgetController = nullptr;
        }

        combofunc.clear();
        _functable.clear();
        _scale.clear();
        _currentBoundary.reset();

        if (_realtimeUpdateTimer)
        {
            _realtimeUpdateTimer->stop();
            delete _realtimeUpdateTimer;
            _realtimeUpdateTimer = nullptr;
        }
    }

    void mBoundaryTempPanel::setInputsValue(std::shared_ptr<MechCaseData::mLoadBoundaryBase> boundaryData)
    {
        std::shared_ptr<MechCaseData::mBoundaryTemp> boundaryPtr = std::dynamic_pointer_cast<MechCaseData::mBoundaryTemp>(boundaryData);
        _currentBoundary = boundaryPtr;
        _boundaryData = boundaryData;
        if (!boundaryPtr) return;

        // 定义方式反显
        _defineTypeCombo.combobox->blockSignals(true);
        _defineTypeCombo.combobox->setCurrentIndex(boundaryPtr->isAnalysisFieldMode_ ? 1 : 0);
        _defineTypeCombo.combobox->blockSignals(false);

        // 温度反显
        _inp.lineeditvalue->setExpression(QString::number(boundaryPtr->_temp));

        // 分析场下拉刷新与反显
        _analysisFieldCombo.combobox->blockSignals(true);
        _analysisFieldCombo.combobox->clear();
        _analysisFieldTable.clear();
        _analysisFieldCombo.combobox->addItem("");
        _analysisFieldTable[""] = -1;
        auto& analysisFieldMgr = getCVM->getModelData()->getManager<MechDiscreteFieldData::mFieldManager>(Physical_Field);
        auto allAnalysisField = analysisFieldMgr.getAllFieldID(MxFunctions::FieldType::Analysis);
        for (auto fieldid : allAnalysisField)
        {
            QString fieldname = analysisFieldMgr.getFieldNameByID(fieldid);
            _analysisFieldCombo.combobox->addItem(fieldname);
            _analysisFieldTable[fieldname] = fieldid;
        }
        {
            auto& mapboundary_ = getCVM->getModelData()->getMapRelationManager();
            int analysisFieldId = mapboundary_.getMapRelation<int>({ Physical_LoadBoundary, boundaryData->getLoadBoundaryId() }, Physical_Field, MapRelationType::MapRelation_AnalysisField);
            if (analysisFieldId != -1)
            {
                _analysisFieldCombo.combobox->setCurrentText(analysisFieldMgr.getFieldNameByID(analysisFieldId));
            }
        }
        _analysisFieldCombo.combobox->blockSignals(false);

        // 函数下拉刷新
        combofunc.combobox->blockSignals(true);

        combofunc.combobox->clear();
        _functable.clear();
        _functable[""] = -1;
        combofunc.combobox->addItem("");
        auto allFunc = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getAllFunctionID();
        for (auto funcid : allFunc)
        {
            QString funcname = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionNameByID(funcid);
            combofunc.combobox->addItem(funcname);
            _functable[funcname] = funcid;
        }

        auto& mapboundary = getCVM->getModelData()->getMapRelationManager();
        int funcid = mapboundary.getMapRelation<int>({ Physical_LoadBoundary, boundaryData->getLoadBoundaryId() }, Physical_Function);
        if (funcid != -1)
        {
            QString funcname = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionNameByID(funcid);
            combofunc.combobox->setCurrentText(funcname);
        }
        combofunc.combobox->blockSignals(false);

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
            if (boundaryPtr->isAnalysisFieldMode_)
            {
                // 分析场：无作用对象，全程保持失活，不激活、不渲染任何拾取高亮
                _pickWidgetController->setPickWidgetVisible(false);
                _pickWidgetController->setCloudChartMode(true);
                _pickWidgetController->displayData(displayData);
            }
            else
            {
                // displayData 默认不重画高亮；若控件此前已处于激活态，setPickActive(true) 会直接返回，
                // 该情况下需显式重画一次高亮以匹配反显数据。整体保证进入面板只做一次全量高亮。
                _pickWidgetController->setPickWidgetVisible(true);
                const bool wasActive = _pickWidgetController->isPickActive();
                _pickWidgetController->displayData(displayData);
                _pickWidgetController->setPickActive(true);
                if (wasActive)
                {
                    _pickWidgetController->refreshHighlight();
                }
            }
        }

        // 缩放因子反显
        double scale = mapboundary.getScaleByCase({ MxFunctions::PhysicalFieldType::Physical_LoadBoundary, boundaryData->getLoadBoundaryId() }, _caseId);
        _scale.lineeditvalue->setExpression(MxFunctions::mNumberToString(scale));
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

    QVector<QWidget*> mBoundaryTempPanel::getRefStateInputs(FoldSection* fathertar)
    {
        refstate_ = fathertar;
        QVector<QWidget*>wds;
        // 分析场方式无大小概念，不需要缩放因子（折叠框整体隐藏）
        if (_currentBoundary && _currentBoundary->isAnalysisFieldMode_)
            return wds;
        wds.append(_scale.propwid);
        return wds;
    }

    QVector<QWidget*> mBoundaryTempPanel::getTargetInputs(FoldSection* fathertar)
    {
        fatar_ = fathertar;
        QVector<QWidget*>wds;
        // 分析场方式无作用对象（折叠框整体隐藏）
        if (_currentBoundary && _currentBoundary->isAnalysisFieldMode_)
            return wds;
        if (_pickWidgetController)
        {
            wds.append(_pickWidgetController->widget());
        }
        return wds;
    }

    QVector<QWidget*> mBoundaryTempPanel::getValueInputs(FoldSection* fatherval)
    {
        faval_ = fatherval;
        QVector<QWidget*>wds;
        wds.append(_defineTypeCombo.propwid);
        if (_defineTypeCombo.combobox->currentIndex() == 1)
        {
            // 分析场：分析场下拉框 + 函数（无温度大小）
            wds.append(_analysisFieldCombo.propwid);
            wds.append(combofunc.propwid);
        }
        else
        {
            // 统一赋值：温度 + 函数
            wds.append(_inp.propwid);
            wds.append(combofunc.propwid);
        }
        return wds;
    }

    int mBoundaryTempPanel::getFunc()
    {
        int funcid = -1;
        QString funcname = combofunc.combobox->currentText();
        if (_functable.find(funcname) != _functable.end())
        {
            funcid = _functable[funcname];
        }
        return funcid;
    }

    void mBoundaryTempPanel::saveTemp()
    {
        if (!_currentBoundary)
        {
            return;
        }
        _currentBoundary->_temp = _inp.lineeditvalue->text().trimmed().toDouble();
        notifyBoundaryChanged();
    }

    void mBoundaryTempPanel::saveFunc()
    {
        if (!_currentBoundary)
        {
            return;
        }
        int funcid = getFunc();
        auto& mapboundary = getCVM->getModelData()->getMapRelationManager();
        mapboundary.appendMapRelation({ Physical_LoadBoundary, _boundaryData->getLoadBoundaryId() }, { Physical_Function, funcid });
        notifyBoundaryChanged();
    }

    void mBoundaryTempPanel::saveAnalysisField()
    {
        if (!_currentBoundary)
        {
            return;
        }
        int fieldid = -1;
        QString fieldname = _analysisFieldCombo.combobox->currentText();
        if (_analysisFieldTable.find(fieldname) != _analysisFieldTable.end())
        {
            fieldid = _analysisFieldTable[fieldname];
        }
        auto& mapboundary = getCVM->getModelData()->getMapRelationManager();
        mapboundary.appendMapRelation({ Physical_LoadBoundary, _boundaryData->getLoadBoundaryId() }, { Physical_Field, fieldid }, MapRelationType::MapRelation_AnalysisField);
        notifyBoundaryChanged();
    }

    void mBoundaryTempPanel::saveScale()
    {
        if (!_currentBoundary)
        {
            return;
        }
        double scale = _scale.lineeditvalue->text().trimmed().toDouble();
        auto& mapboundary = getCVM->getModelData()->getMapRelationManager();
        mapboundary.setScaleByCase({ MxFunctions::PhysicalFieldType::Physical_LoadBoundary, _boundaryData->getLoadBoundaryId() }, _caseId, scale);
        notifyBoundaryChanged();
    }

    void mBoundaryTempPanel::notifyBoundaryChanged()
    {
        if (!_boundaryData)
        {
            return;
        }
        // 仅通知外部刷新树/渲染，不再触发全量保存
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_LoadBoundary, _boundaryData->getLoadBoundaryId());
    }

    void mBoundaryTempPanel::applyAllowedPickTypes()
    {
        if (!_pickWidgetController) return;
        // 温度边界：几何点/边/面/实体 + 节点
        _pickWidgetController->setAllowedTypes(&pickGeoPointEdgeFaceSolidNodePart);
        _pickWidgetController->setPickType();
    }

    void mBoundaryTempPanel::deactivatePickAndCommit()
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

    void mBoundaryTempPanel::updateBoundarySymbolRealtime()
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

    void mBoundaryTempPanel::doUpdateBoundarySymbolRealtime()
    {
        // 分析场方式无作用对象，无需渲染温度边界符号（清除残留符号）
        if (_currentBoundary && _currentBoundary->isAnalysisFieldMode_)
        {
            size_t sig = AllRenderSymbolManager::beginRenderSignature();
            AllRenderSymbolManager::combineRenderSignature(sig, static_cast<size_t>(_boundaryData->getLoadBoundaryId()));
            if (sig != _lastRenderSig)
            {
                _lastRenderSig = sig;
                AllRenderSymbolManager::getInstance()->assignBoundary(
                    _boundaryData, {}, QVector3D(255.0f, 165.0f, 0.0f));
                auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
                if (symbolController) symbolController->UpdateAllSymbol();
                getCVM->getPreRenderWidget()->updatePaint();
            }
            return;
        }

        // 1. 从通用拾取控件内存实时汇总作用对象坐标（拾取期间尚未 commit 到映射，
        //    不能走 rendBoundary 读映射，必须直接基于控件当前数据计算）。
        QVector<QVector3D> positions;
        collectRenderPositions(positions);

        // 最终数据签名：边界ID+位置集未变化时跳过符号赋值与 UpdateAllSymbol 全量重建
        //（进入面板防抖触发/切出冲刷时数据往往与上次相同）
        size_t sig = AllRenderSymbolManager::beginRenderSignature();
        AllRenderSymbolManager::combineRenderSignature(sig, static_cast<size_t>(_boundaryData->getLoadBoundaryId()));
        AllRenderSymbolManager::combineRenderSignature(sig, positions);
        if (sig == _lastRenderSig)
        {
            return;
        }
        _lastRenderSig = sig;

        // 2. 直接赋予温度边界符号（key=boundary_<id>，同名覆盖；空 positions 等同清除残留符号）
        AllRenderSymbolManager::getInstance()->assignBoundary(
            _boundaryData, positions, QVector3D(255.0f, 165.0f, 0.0f));

        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        if (symbolController) symbolController->UpdateAllSymbol();
        getCVM->getPreRenderWidget()->updatePaint();
    }

    void mBoundaryTempPanel::collectRenderPositions(QVector<QVector3D>& outPositions)
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