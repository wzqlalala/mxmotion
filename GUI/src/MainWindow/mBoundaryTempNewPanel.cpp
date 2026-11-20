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

#include "PickCommonFunction.h"
#include "mBoundaryTempNew.h"
#include "mBoundaryTempNewPanel.h"
#include <QFileDialog>
#include <QFileInfo>

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

using namespace MechCommon;
using namespace MechPreData;
using namespace MechSetData;

namespace MechMainWindow
{
    mBoundaryTempNewPanel::mBoundaryTempNewPanel(QWidget* parent)
        :mBoundaryPanelBase(parent)
    {
        _realtimeUpdateTimer = new QTimer(this);
        _realtimeUpdateTimer->setSingleShot(true);
        connect(_realtimeUpdateTimer, &QTimer::timeout, this, &mBoundaryTempNewPanel::doUpdateBoundarySymbolRealtime);

        initInputs();
    }

    mBoundaryTempNewPanel::~mBoundaryTempNewPanel()
    {
        clearInputs();
    }

    std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> mBoundaryTempNewPanel::getPickedData()
    {
        if (!_pickWidgetController)
            return std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>();
        return _pickWidgetController->getAllData().pickedData;
    }

    void mBoundaryTempNewPanel::initInputs()
    {
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
                _boundaryTempNewData->setPickState(1);
            }
            else
            {
                _boundaryTempNewData->setPickState(2);
            }
            //发送更新信号
            notifyBoundaryChanged();
            });

        _pickWidgetController->setSetResolver(&resolveSetToTypedData);
        _pickWidgetController->setDefultTip("选择对象");

        applyAllowedPickTypes();

        // 主模型定义
        _mainModelFileEdit = new QLineEdit();
        _mainModelFileEdit->setFixedHeight(30);
        connect(_mainModelFileEdit, &QLineEdit::editingFinished, [this]() { saveMainModelFile(); });

        _timeStepEdit = new QLineEdit();
        _timeStepEdit->setFixedHeight(30);
        connect(_timeStepEdit, &QLineEdit::editingFinished, [this]() { saveTimeStep(); });

        _mainSetEdit = new QLineEdit();
        _mainSetEdit->setFixedHeight(30);
        _mainSetEdit->setPlaceholderText(tr(""));
        connect(_mainSetEdit, &QLineEdit::editingFinished, [this]() { saveMainSet(); });

        _mainSetCheck = new QCheckBox(tr("指定集合"));
        _mainSetCheck->setFixedHeight(30);
        connect(_mainSetCheck, &QCheckBox::toggled, this, [this](bool checked) {
            _mainSetEdit->setEnabled(checked);
            if (!checked)
            {
                _mainSetEdit->clear();
            }
            saveUseMainSet();
            saveMainSet();
            });

        _openFileBtn = new QPushButton();
        _openFileBtn->setFixedSize(30, 30);
        _openFileBtn->setIcon(QIcon(":/mMainWidgets/Icon/Case/openFile@3x.png"));
        _openFileBtn->setIconSize(QSize(20, 20));
        connect(_openFileBtn, &QPushButton::clicked, this, &mBoundaryTempNewPanel::slot_browseFileClicked);
    }

    void mBoundaryTempNewPanel::slot_browseFileClicked()
    {
        QString filePath = QFileDialog::getOpenFileName(nullptr, tr("选择文件"), QString(), tr("所有文件 (*)"));
        if (!filePath.isEmpty())
        {
            QFileInfo fi(filePath);
            _mainModelFileEdit->setText(fi.fileName());
            _mainModelFileEdit->setToolTip(filePath);
            saveMainModelFile();
        }
    }

    void mBoundaryTempNewPanel::saveMainModelFile()
    {
        if (!_boundaryTempNewData)
            return;
        QString filePath = _mainModelFileEdit->toolTip();
        if (filePath.isEmpty())
        {
            filePath = _mainModelFileEdit->text().trimmed();
        }
        _boundaryTempNewData->_mainModelFile = filePath;
        notifyBoundaryChanged();
    }

    void mBoundaryTempNewPanel::saveTimeStep()
    {
        if (!_boundaryTempNewData)
            return;
        _boundaryTempNewData->_timeStep = _timeStepEdit->text().trimmed();
        notifyBoundaryChanged();
    }

    void mBoundaryTempNewPanel::saveMainSet()
    {
        if (!_boundaryTempNewData)
            return;
        if (_mainSetCheck->isChecked())
        {
            _boundaryTempNewData->_mainSet = _mainSetEdit->text().trimmed();
        }
        else
        {
            _boundaryTempNewData->_mainSet.clear();
        }
        notifyBoundaryChanged();
    }

    void mBoundaryTempNewPanel::saveUseMainSet()
    {
        if (!_boundaryTempNewData)
            return;
        _boundaryTempNewData->_useMainSet = _mainSetCheck->isChecked();
        notifyBoundaryChanged();
    }

    void mBoundaryTempNewPanel::notifyBoundaryChanged()
    {
        if (!_boundaryData)
            return;
        // 仅通知外部刷新树/渲染，不再触发全量保存
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_LoadBoundary, _boundaryData->getLoadBoundaryId());
    }

    void mBoundaryTempNewPanel::setInputsValue(std::shared_ptr<MechCaseData::mLoadBoundaryBase> boundaryData)
    {
        std::shared_ptr<MechCaseData::mBoundaryTempNew> boundaryPtr = std::dynamic_pointer_cast<MechCaseData::mBoundaryTempNew>(boundaryData);
        _boundaryData = boundaryData;
        _boundaryTempNewData = boundaryPtr;
        if (!boundaryPtr)
        {
            return;
        }

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

        // 主模型定义反显
        QFileInfo fi(boundaryPtr->_mainModelFile);
        _mainModelFileEdit->setText(fi.fileName());
        _mainModelFileEdit->setToolTip(boundaryPtr->_mainModelFile);
        _timeStepEdit->setText(boundaryPtr->_timeStep);
        bool useSet = boundaryPtr->_useMainSet;
        _mainSetCheck->setChecked(useSet);
        _mainSetEdit->setEnabled(useSet);
        if (useSet)
        {
            _mainSetEdit->setText(boundaryPtr->_mainSet);
        }
        else
        {
            _mainSetEdit->clear();
        }
        //判断拾取控件是否为空 以设置标志位
        if (_pickWidgetController->getAllData().isEmpty())
        {
            _boundaryTempNewData->setPickState(1);
        }
        else
        {
            _boundaryTempNewData->setPickState(2);
        }
        //发送更新信号
        notifyBoundaryChanged();
        updateBoundarySymbolRealtime();
    }

    void mBoundaryTempNewPanel::applyAllowedPickTypes()
    {
        if (!_pickWidgetController) return;
        // 温度新边界：几何点/边/面/实体 + 节点
        _pickWidgetController->setAllowedTypes(&pickGeoPointEdgeFaceSolidNodePart);
        _pickWidgetController->setPickType();
    }

    QVector<QWidget*> mBoundaryTempNewPanel::getTargetInputs(FoldSection* fathertar)
    {
        fatar_ = fathertar;
        QVector<QWidget*> wds;
        if (_pickWidgetController)
        {
            wds.append(_pickWidgetController->widget());
        }
        return wds;
    }

    QVector<QWidget*> mBoundaryTempNewPanel::getValueInputs(FoldSection* fatherval)
    {
        faval_ = fatherval;
        QVector<QWidget*> wds;
        // 文件
        QWidget* fileWidget = new QWidget();
        QHBoxLayout* fileLayout = new QHBoxLayout();
        fileLayout->setMargin(0);
        fileLayout->addWidget(new PanelLabel("文件"));
        fileLayout->addWidget(_mainModelFileEdit);
        fileLayout->addWidget(_openFileBtn);
        fileWidget->setLayout(fileLayout);
        wds.append(fileWidget);
        // 时间步
        QWidget* stepWidget = new QWidget();
        QHBoxLayout* stepLayout = new QHBoxLayout();
        stepLayout->setMargin(0);
        stepLayout->addWidget(new PanelLabel("时间步"));
        stepLayout->addWidget(_timeStepEdit);
        stepWidget->setLayout(stepLayout);
        wds.append(stepWidget);
        // 指定集合
        QWidget* setWidget = new QWidget();
        QHBoxLayout* setLayout = new QHBoxLayout();
        setLayout->setMargin(0);
        setLayout->addWidget(_mainSetCheck);
        setLayout->addWidget(_mainSetEdit);
        setWidget->setLayout(setLayout);
        wds.append(setWidget);
        return wds;
    }

    std::shared_ptr<MechCaseData::mLoadBoundaryBase> mBoundaryTempNewPanel::createBoundaryData()
    {
        _boundaryTempNewData = std::make_shared<MechCaseData::mBoundaryTempNew>();
        _boundaryData = _boundaryTempNewData;
        return _boundaryTempNewData;
    }

    void mBoundaryTempNewPanel::deactivatePickAndCommit()
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

    void mBoundaryTempNewPanel::updateBoundarySymbolRealtime()
    {
        if (!_pickWidgetController || !_boundaryTempNewData)
            return;
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;

        // 通过防抖定时器合并连续触发
        if (!_realtimeUpdateTimer)
            return;
        _realtimeUpdateTimer->start(100);
    }

    void mBoundaryTempNewPanel::doUpdateBoundarySymbolRealtime()
    {
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

        // 2. 直接赋予温度新边界符号（key=boundary_<id>，同名覆盖；空 positions 等同清除残留符号）
        AllRenderSymbolManager::getInstance()->assignBoundary(
            _boundaryData, positions, QVector3D(255.0f, 165.0f, 0.0f));

        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        if (symbolController) symbolController->UpdateAllSymbol();
        getCVM->getPreRenderWidget()->updatePaint();
    }

    void mBoundaryTempNewPanel::collectRenderPositions(QVector<QVector3D>& outPositions)
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

    void mBoundaryTempNewPanel::clearInputs()
    {
        if (_pickWidgetController)
        {
            _pickWidgetController->setPickActive(false);
            delete _pickWidgetController;
            _pickWidgetController = nullptr;
        }

        if (_mainModelFileEdit)
        {
            delete _mainModelFileEdit;
            _mainModelFileEdit = nullptr;
        }
        if (_timeStepEdit)
        {
            delete _timeStepEdit;
            _timeStepEdit = nullptr;
        }
        if (_mainSetEdit)
        {
            delete _mainSetEdit;
            _mainSetEdit = nullptr;
        }
        if (_mainSetCheck)
        {
            delete _mainSetCheck;
            _mainSetCheck = nullptr;
        }
        if (_openFileBtn)
        {
            delete _openFileBtn;
            _openFileBtn = nullptr;
        }

        _boundaryTempNewData.reset();
        _boundaryData.reset();

        if (_realtimeUpdateTimer)
        {
            _realtimeUpdateTimer->stop();
            delete _realtimeUpdateTimer;
            _realtimeUpdateTimer = nullptr;
        }
    }
}
