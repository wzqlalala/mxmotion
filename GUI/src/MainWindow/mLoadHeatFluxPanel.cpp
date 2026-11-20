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
#include<mElementSurface.h>
#include<mElementSet.h>

#include "mLoadHeatFluxPanel.h"
#include "mGeneralPickUp.h"
#include "mGetSetTypeTemplate.h"
#include "AllRenderSymbolManager.h"
#include "mOtherFunction.h"

using namespace MechCommon;
using namespace MechPreData;

namespace MechMainWindow
{
    mLoadHeatFluxPanel::mLoadHeatFluxPanel(QWidget* parent)
        :mLoadPanelBase(parent)
    {
        _realtimeUpdateTimer = new QTimer(this);
        _realtimeUpdateTimer->setSingleShot(true);
        connect(_realtimeUpdateTimer, &QTimer::timeout, this, &mLoadHeatFluxPanel::onRealtimeUpdateTimeout);

        initInputs();
    }

    mLoadHeatFluxPanel::~mLoadHeatFluxPanel()
    {
        clearInputs();
    }

    std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> mLoadHeatFluxPanel::getPickedData()
    {
        if (!_pickWidgetController)
            return std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>();
        return _pickWidgetController->getAllData().pickedData;
    }

    namespace
    {
        MxFunctions::PickDataType heatFluxWayToSetType(int way)
        {
            if (way == 1) return MxFunctions::PickDataType::ElementFace;
            if (way == 2) return MxFunctions::PickDataType::Element;
            return MxFunctions::PickDataType::Node;
        }
    }

    void mLoadHeatFluxPanel::updateOpenedTable()
    {
        if (!_currentLoad || !_discreteControl)
            return;

        // 独立赋值模式下，cellFaceData_ 与离散数据控件的行数据保持严格一致：
        // 表格是数据的权威来源（含拾取追加与手动删除），此处按控件当前行重建 map。
        // 行 key 只有 QPair<int,int> 两个槽位，三种热流的业务 key 与 map 完整 key 构造规则：
        //   点热流(Node)：行 key (nodeId, -1)，map key (nodeId, -1, 0)（与 mIMxFile 读入一致）
        //   面热流(ElementFace)：行 key (cellId, faceId)，map key (cellId, partId, faceId)，
        //                        partId 优先保留 map 原有完整 key，否则按单元 ID 反查
        //   体热流(Element)：行 key (cellId, -1)，map key (cellId, partId, 0)，partId 规则同面热流
        const int way = _currentLoad->way_;
        const auto rows = _discreteControl->rows();

        // 模型已卸载但渲染部件仍在的中间态下 getModelData() 为空
        auto* modelData = getCVM ? getCVM->getModelData() : nullptr;
        auto meshData = modelData ? modelData->getMeshData() : nullptr;
        QHash<int, int> partIdByCellId;
        std::map<RenderPickResultData, QVector<QVariant>> newMap;

        if (way == 0) // 点热流
        {
            std::map<int, RenderPickResultData> existingKeys;
            std::map<int, QVector<QVariant>> existingValues;
            for (const auto& entry : _currentLoad->cellFaceData_)
            {
                const int nodeId = entry.first.result_id;
                if (existingKeys.find(nodeId) == existingKeys.end())
                {
                    existingKeys[nodeId] = entry.first;
                    existingValues[nodeId] = entry.second;
                }
            }

            for (const auto& row : rows)
            {
                const int nodeId = row.first.first;
                if (nodeId <= 0) continue;
                auto keyIt = existingKeys.find(nodeId);
                if (keyIt != existingKeys.end())
                {
                    newMap[keyIt->second] = existingValues[nodeId];
                }
                else
                {
                    newMap[RenderPickResultData(nodeId, -1, 0)] = { QVariant() };
                }
            }
        }
        else // 面热流 / 体热流：均含 part_id
        {
            std::map<QPair<int, int>, RenderPickResultData> existingKeys;
            std::map<QPair<int, int>, QVector<QVariant>> existingValues;
            for (const auto& entry : _currentLoad->cellFaceData_)
            {
                const QPair<int, int> bizKey = (way == 1)
                    ? qMakePair(entry.first.result_id, entry.first.result_id2)
                    : qMakePair(entry.first.result_id, -1);
                if (existingKeys.find(bizKey) == existingKeys.end())
                {
                    existingKeys[bizKey] = entry.first;
                    existingValues[bizKey] = entry.second;
                }
            }

            for (const auto& row : rows)
            {
                const int cellId = row.first.first;
                const int faceId = row.first.second;
                if (cellId <= 0) continue;
                if (way == 1 && faceId <= 0) continue;

                const QPair<int, int> bizKey = (way == 1) ? qMakePair(cellId, faceId) : qMakePair(cellId, -1);
                auto keyIt = existingKeys.find(bizKey);
                if (keyIt != existingKeys.end())
                {
                    // 保留原有完整 key（含 part_id）与已填值（含空值占位）
                    newMap[keyIt->second] = existingValues[bizKey];
                    continue;
                }

                // 新增行：按单元 ID 反查 part_id 构造完整 key（带会话级缓存），
                // 与 mIMxFile 读入键规范 (cellId, partId, faceId/0) 保持一致
                auto partIt = partIdByCellId.constFind(cellId);
                if (partIt == partIdByCellId.constEnd())
                {
                    const int partId = meshData ? meshData->getPartIdByEleId(cellId) : -1;
                    partIt = partIdByCellId.insert(cellId, partId);
                }
                newMap[RenderPickResultData(cellId, partIt.value(), way == 1 ? faceId : 0)] = { QVariant() };
            }
        }

        _currentLoad->cellFaceData_ = std::move(newMap);
    }

    // 根据当前热流类型过滤拾取数据
    void mLoadHeatFluxPanel::filterPicksByHeatFluxType()
    {
        if (!_pickWidgetController)
            return;

        // 同步通用拾取控件最新数据
        auto picks = _pickWidgetController->getAllData().pickedData;

        const int way = _currentLoad ? _currentLoad->way_ : index;
        const MxFunctions::PickDataType keepType = heatFluxWayToSetType(way);

        auto it = picks.begin();
        while (it != picks.end())
        {
            if (it->first != keepType)
                it = picks.erase(it);
            else
                ++it;
        }

        // 独立赋值模式无需集合：把控件数据重设为"仅目标类型拾取、无集合"
        _pickWidgetController->replacePickedDataAndClearSets(picks);
        // 彻底关闭通用拾取控件的高亮渲染：独立赋值的拾取高亮由离散数据控件
        // 直驱视口选择容器维护，通用控件不参与渲染
        if (_currentLoad && _currentLoad->defineType_ == 1)
        {
            _pickWidgetController->setCloudChartMode(true);
        }

        updateOpenedTable();
    }

    // 同步数据到通用拾取控件（含集合反显）
    void mLoadHeatFluxPanel::syncPicksToController()
    {
        if (!_pickWidgetController || !_currentLoad)
            return;

        PickWidgetData displayData;

        auto& mapload = getCVM->getModelData()->getMapRelationManager();
        int setid = mapload.getMapRelation<int>({ Physical_LoadBoundary, _loadData->getLoadBoundaryId() }, Physical_Set);

        // 统一赋值模式优先从集合映射反显
        if (setid != -1 && _currentLoad->defineType_ == 0)
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

        // 独立赋值模式且没有集合映射时，直接从数据类生成拾取数据
        if (_currentLoad->defineType_ == 1 && displayData.pickedData.empty() && displayData.selectedSetNames.isEmpty())
        {
            // 直接 push 数据 map 中的原始 key，禁止构造临时 RenderPickResultData
            std::vector<RenderPickResultData> pickList;
            for (const auto& entry : _currentLoad->cellFaceData_)
            {
                pickList.push_back(entry.first);
            }
            if (!pickList.empty())
            {
                displayData.pickedData[heatFluxWayToSetType(_currentLoad->way_)] = std::move(pickList);
            }
        }

        _pickWidgetController->displayData(displayData);
    }

    void mLoadHeatFluxPanel::applyAllowedPickTypes(int way)
    {
        if (!_pickWidgetController) return;

        bool independent = _currentLoad ? (_currentLoad->defineType_ == 1)
            : (_defineTypeCombo.combobox->currentIndex() == 1);

        if (way == 0) // 点热流
        {
            _pickWidgetController->setAllowedTypes(independent ? &pickOnlyNode : &pickGeoPointEdgeFaceSolidAndNode);
        }
        else if (way == 1) // 面热流
        {
            _pickWidgetController->setAllowedTypes(independent ? &pickOnlyElementSurface : &pickGeoFaceSolidAndElementSurface);
        }
        else if (way == 2) // 体热流
        {
            _pickWidgetController->setAllowedTypes(independent ? &pickOnlyElement : &pickGeoFaceSolidAndElement);
        }
        if (!independent)
        {
            _pickWidgetController->setPickType();
        }
        // 独立赋值分支不调 setPickType()：拾取过滤器由离散数据控件开始拾取时自行切换，
        // 避免无激活控件时发全局信号把视口卡死
    }
    void mLoadHeatFluxPanel::initInputs()
    {
        // 热流类型下拉框
        singleComboBox typeCombo;
        typeCombo.initWidget("热流类型");
        typeCombo.combobox->addItems({ "点热流", "面热流", "体热流" });
        typeCombo.combobox->setFixedHeight(30);
        connect(typeCombo.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mLoadHeatFluxPanel::slot_valueInpWayChanged);
        comboBoxs.append(typeCombo);

        // 定义方式下拉框
        _defineTypeCombo.initWidget("定义方式");
        _defineTypeCombo.combobox->addItem("统一赋值");
        _defineTypeCombo.combobox->addItem("独立赋值");
        _defineTypeCombo.combobox->addItem("分析场");
        _defineTypeCombo.combobox->setFixedHeight(30);
        _defineTypeCombo.combobox->setCurrentIndex(0);
        connect(_defineTypeCombo.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mLoadHeatFluxPanel::slot_defineTypeChanged);

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
            this, &mLoadHeatFluxPanel::slot_analysisFieldChanged);

        // 统一赋值：热流密度大小输入框
        _heatFluxInput.initWidget("热流大小", true);
        _heatFluxInput.labelname->setText("热流大小");
        _heatFluxInput.lineeditvalue->setFixedHeight(30);
        connect(_heatFluxInput.lineeditvalue, &mCalcLineEdit::editingFinished,
            [this]() {
                saveHeatFluxValue();
            });

        // 独立赋值表格行：通用离散数据控件（入口按钮 + 锚定弹窗表格），
        // 拾取对象/标识列随热流类型切换，由 rebuildDiscreteControl() 按当前类型创建
        _tableRowWidget = new QWidget();
        QHBoxLayout* tableLayout = new QHBoxLayout(_tableRowWidget);
        tableLayout->setContentsMargins(0, 0, 0, 0);
        tableLayout->setSpacing(8);
        _tableLabel = new QLabel("节点数据");
        _tableLabel->setMinimumWidth(87);
        tableLayout->addWidget(_tableLabel);
        rebuildDiscreteControl();

        // 通用拾取控件（替代原树形选择+按钮）
        _pickWidgetController = mGeneralPickUp::createPickWidget(CBBlue);
        connect(_pickWidgetController, &mPickWidgetController::pickActiveChanged, this, [this](bool isActive) {
            if (isActive)
            {
                const int typeIndex = comboBoxs[0].combobox->currentIndex();
                if (typeIndex == 0)
                    emit mMechGlobalSignals::getInstance()->activePickSig(5);
                else if (typeIndex == 1)
                    emit mMechGlobalSignals::getInstance()->activePickSig(7);
                else if (typeIndex == 2)
                    emit mMechGlobalSignals::getInstance()->activePickSig(8);
            }
            });
        connect(_pickWidgetController, &mPickWidgetController::dataChanged, this, [this]() {
            // 独立赋值模式下通用拾取控制器不再接收拾取（由离散数据控件直驱视口），
            // 其 dataChanged 只在统一赋值模式下触发，无需按方式分支重建 map。
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
        _pickWidgetController->setDefultTip("选择对象");

        // 缩放因子
        _scale.initWidget("缩放因子", false);
        _scale.labelname->setText("缩放因子");
        connect(_scale.lineeditvalue, &mCalcLineEdit::editingFinished,
            [this]() {
                saveScale();
            });

        // 幅值曲线
        _functable.clear();
        combofunc.initWidget("幅值曲线");
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
            this, &mLoadHeatFluxPanel::slot_funcChanged);
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
    }

    void mLoadHeatFluxPanel::rebuildDiscreteControl()
    {
        if (!_tableRowWidget || !_tableRowWidget->layout())
            return;

        // 热流类型切换时拾取对象随之变化（节点/单元面/单元），离散控件的 PickSpec
        // 无法在创建后修改，故按当前类型销毁重建；旧弹窗（若有）随控件一并析构
        if (_discreteControl)
        {
            // 先清空行：setRows 内部 syncSelectionToRows() 会清掉该控件回种到
            // 视口选择容器的选中态与高亮渲染层，避免旧类型高亮残留
            _discreteControl->setRows({});
            _discreteControl->closePopup();
            _tableRowWidget->layout()->removeWidget(_discreteControl);
            delete _discreteControl;
            _discreteControl = nullptr;
        }

        const int way = _currentLoad ? _currentLoad->way_ : index;

        mDiscreteDataControl::PickSpec pickSpec;
        QString title;
        if (way == 0) // 点热流：节点
        {
            title = QStringLiteral("节点数据");
            pickSpec.typeName = QStringLiteral("节点");
            pickSpec.idHeaders = QStringList{ QStringLiteral("节点编号") };
            pickSpec.keyOf = [](const RenderPickResultData& d) { return qMakePair(d.result_id, -1); };
            pickSpec.pickTypes = { MxFunctions::PickDataType::Node };
            pickSpec.pickFilter = 5; // 5=节点 6=单元边 7=单元面 8=单元
        }
        else if (way == 1) // 面热流：单元面（part_id 不参与行 key，由面板侧按完整 key 规则补回）
        {
            title = QStringLiteral("单元面数据");
            pickSpec.typeName = QStringLiteral("单元面");
            pickSpec.idHeaders = QStringList{ QStringLiteral("单元编号"), QStringLiteral("面编号") };
            pickSpec.keyOf = [](const RenderPickResultData& d) { return qMakePair(d.result_id, d.result_id2); };
            pickSpec.pickTypes = { MxFunctions::PickDataType::ElementFace };
            pickSpec.pickFilter = 7;
        }
        else // 体热流：单元（行 key 只放单元 ID，part_id 由面板侧按完整 key 规则补回）
        {
            title = QStringLiteral("单元数据");
            pickSpec.typeName = QStringLiteral("单元");
            pickSpec.idHeaders = QStringList{ QStringLiteral("单元编号") };
            pickSpec.keyOf = [](const RenderPickResultData& d) { return qMakePair(d.result_id, -1); };
            pickSpec.pickTypes = { MxFunctions::PickDataType::Element };
            pickSpec.pickFilter = 8;
        }

        _tableLabel->setText(title);
        // 拾取由离散数据控件直接驱动视口选择容器，不再经过通用拾取控件
        _discreteControl = new mDiscreteDataControl(
            title,
            { QStringLiteral("热流密度") },
            &pickSpec, _tableRowWidget);
        _discreteControl->setRenderButtonVisible(false);
        connect(_discreteControl, &mDiscreteDataControl::dataChanged,
            this, &mLoadHeatFluxPanel::slot_discreteDataChanged);
        connect(_discreteControl, &mDiscreteDataControl::cellEdited,
            this, &mLoadHeatFluxPanel::slot_discreteCellEdited);
        // 入口按钮从控件中剖分出来直接放入行布局，控件本体隐藏（弹窗为
        // 宿主主窗口子窗口不受影响），按钮可用状态与控件本体显隐/禁用解耦
        if (QPushButton* entry = _discreteControl->entryButton())
        {
            _discreteControl->detachEntryButton(_tableRowWidget);
            _tableRowWidget->layout()->addWidget(entry);
            _discreteControl->hide();
        }
        else
        {
            _tableRowWidget->layout()->addWidget(_discreteControl);
        }
    }

    void mLoadHeatFluxPanel::saveHeatFluxValue()
    {
        if (!_currentLoad)
            return;

        QString valStr = _heatFluxInput.lineeditvalue->text().trimmed();
        if (!valStr.isEmpty())
        {
            _currentLoad->pa_ = QVariant::fromValue<double>(valStr.toDouble());
        }
        else
        {
            _currentLoad->pa_ = QVariant();
        }
        updateLoadSymbolRealtime();
        notifyLoadChanged();
    }

    void mLoadHeatFluxPanel::saveFunc()
    {
        if (!_currentLoad)
            return;

        int funcid = getFunc();
        auto& mapload = getCVM->getModelData()->getMapRelationManager();
        mapload.appendMapRelation({ Physical_LoadBoundary, _loadData->getLoadBoundaryId() }, { Physical_Function, funcid });
        notifyLoadChanged();
    }

    void mLoadHeatFluxPanel::saveAnalysisField()
    {
        if (!_currentLoad)
            return;

        int fieldid = -1;
        QString fieldname = _analysisFieldCombo.combobox->currentText();
        if (_analysisFieldTable.find(fieldname) != _analysisFieldTable.end())
        {
            fieldid = _analysisFieldTable[fieldname];
        }
        auto& mapload = getCVM->getModelData()->getMapRelationManager();
        mapload.appendMapRelation({ Physical_LoadBoundary, _loadData->getLoadBoundaryId() }, { Physical_Field, fieldid }, MapRelationType::MapRelation_AnalysisField);
        notifyLoadChanged();
    }

    void mLoadHeatFluxPanel::saveScale()
    {
        if (!_currentLoad)
            return;

        double scale = _scale.lineeditvalue->text().trimmed().toDouble();
        auto& mapload = getCVM->getModelData()->getMapRelationManager();
        mapload.setScaleByCase({ MxFunctions::PhysicalFieldType::Physical_LoadBoundary, _loadData->getLoadBoundaryId() }, _caseId, scale);
        notifyLoadChanged();
    }

    void mLoadHeatFluxPanel::notifyLoadChanged()
    {
        if (!_loadData)
            return;
        // 仅通知外部刷新树/渲染，不再调用 storeLoadValue 全量保存
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_LoadBoundary, _loadData->getLoadBoundaryId());
    }
    void mLoadHeatFluxPanel::slot_defineTypeChanged(int idx)
    {
        if (_currentLoad)
            _currentLoad->defineType_ = idx;

        // 切换定义方式时关闭离散数据弹窗并结束拾取，避免旧模式弹窗残留
        if (_discreteControl)
        {
            _discreteControl->closePopup();
        }

        const int way = _currentLoad ? _currentLoad->way_ : index;
        if (idx == 1)
        {
            // 独立赋值：按当前热流类型重建离散数据控件（拾取对象/标识列随类型切换），
            // 避免统一切独立时控件仍保持旧类型（如点热流）的拾取配置
            rebuildDiscreteControl();
            // 隐藏通用拾取控件/集合按钮，过滤拾取为仅目标类型
            // （filterPicksByHeatFluxType 内部会关闭通用拾取的高亮渲染，
            //  拾取由离散数据控件直接驱动视口选择容器）
            if (_pickWidgetController)
            {
                _pickWidgetController->setPickWidgetVisible(false);
                _pickWidgetController->setSelectSetBtnVisible(false);
            }
            filterPicksByHeatFluxType();
        }
        else if (idx == 2)
        {
            // 分析场：无作用对象拾取与大小输入，隐藏通用拾取控件并清空拾取高亮
            if (_pickWidgetController)
            {
                _pickWidgetController->setPickWidgetVisible(false);
                _pickWidgetController->setSelectSetBtnVisible(false);
                _pickWidgetController->setCloudChartMode(true);
            }
        }
        else
        {
            // 统一赋值：恢复通用拾取控件可见/激活与正常高亮渲染
            if (_pickWidgetController)
            {
                _pickWidgetController->setPickWidgetVisible(true);
                _pickWidgetController->setSelectSetBtnVisible(true);
                _pickWidgetController->setCloudChartMode(false);
                _pickWidgetController->setPickActive(true);
            }
        }

        applyAllowedPickTypes(way);
        rebuildValueLayout();
        updateLoadSymbolRealtime();
        // 定义方式切换导致作用对象折叠框显隐变化，通知外层重建面板
        emit sg_rebuildPanel();
    }

    void mLoadHeatFluxPanel::rebuildValueLayout()
    {
        if (!faval_)
            return;

        faval_->removeAll();
        faval_->addRow(comboBoxs[0].propwid); // 热流类型
        faval_->addRow(_defineTypeCombo.propwid); // 定义方式
        comboBoxs[0].propwid->show();
        _defineTypeCombo.propwid->show();

        int defineType = _defineTypeCombo.combobox->currentIndex();

        if (defineType == 0) // 统一赋值
        {
            faval_->addRow(_heatFluxInput.propwid);
            _heatFluxInput.propwid->show();
            faval_->addRow(combofunc.propwid);
            combofunc.propwid->show();
        }
        else if (defineType == 1) // 独立赋值
        {
            faval_->addRow(_tableRowWidget);
            _tableRowWidget->show();
            faval_->addRow(combofunc.propwid);
            combofunc.propwid->show();
        }
        else // 分析场：仅分析场下拉框 + 函数
        {
            faval_->addRow(_analysisFieldCombo.propwid);
            _analysisFieldCombo.propwid->show();
            faval_->addRow(combofunc.propwid);
            combofunc.propwid->show();
        }

        faval_->refreshLayout();
    }

    void mLoadHeatFluxPanel::slot_discreteDataChanged()
    {
        // 离散数据控件行变化（拾取确认/删除/清空）：按控件当前行重建 cellFaceData_，
        // 随后刷新实时符号并通知外部。拾取会话进行中跳过：确认拾取后控件会补发一次
        // dataChanged，此时才执行重建/渲染，避免拾取期间反复重绘打断拾取节奏。
        if (!_currentLoad || (_discreteControl && _discreteControl->isPicking()))
            return;
        updateOpenedTable();
        updateLoadSymbolRealtime();
        notifyLoadChanged();
    }

    void mLoadHeatFluxPanel::slot_discreteCellEdited(const QPair<int, int>& key, int valueIndex, const QString& text)
    {
        if (!_currentLoad || key.first <= 0 || _defineTypeCombo.combobox->currentIndex() != 1)
            return;

        const int way = _currentLoad->way_;

        // 行 key 只有 QPair<int,int> 两个槽位，按业务 key 在 cellFaceData_ 中查找已有
        // 完整 key（保留原 part_id）；不存在时按单元 ID 反查 part_id 构造完整 key，
        // 与 mIMxFile 读入键规范保持一致（点：(nodeId,-1,0)，面：(cellId,partId,faceId)，体：(cellId,partId,0)）
        RenderPickResultData loadKey;
        if (way == 0)
        {
            auto it = _currentLoad->cellFaceData_.begin();
            for (; it != _currentLoad->cellFaceData_.end(); ++it)
            {
                if (it->first.result_id == key.first)
                    break;
            }
            loadKey = (it != _currentLoad->cellFaceData_.end())
                ? it->first
                : RenderPickResultData(key.first, -1, 0);
        }
        else if (way == 1)
        {
            if (key.second <= 0) return;
            auto it = _currentLoad->cellFaceData_.begin();
            for (; it != _currentLoad->cellFaceData_.end(); ++it)
            {
                if (it->first.result_id == key.first && it->first.result_id2 == key.second)
                    break;
            }
            if (it != _currentLoad->cellFaceData_.end())
            {
                loadKey = it->first;
            }
            else
            {
                auto* modelData = getCVM ? getCVM->getModelData() : nullptr;
                auto meshData = modelData ? modelData->getMeshData() : nullptr;
                const int partId = meshData ? meshData->getPartIdByEleId(key.first) : -1;
                loadKey = RenderPickResultData(key.first, partId, key.second);
            }
        }
        else
        {
            auto it = _currentLoad->cellFaceData_.begin();
            for (; it != _currentLoad->cellFaceData_.end(); ++it)
            {
                if (it->first.result_id == key.first)
                    break;
            }
            if (it != _currentLoad->cellFaceData_.end())
            {
                loadKey = it->first;
            }
            else
            {
                auto* modelData = getCVM ? getCVM->getModelData() : nullptr;
                auto meshData = modelData ? modelData->getMeshData() : nullptr;
                const int partId = meshData ? meshData->getPartIdByEleId(key.first) : -1;
                loadKey = RenderPickResultData(key.first, partId, 0);
            }
        }

        bool ok = false;
        const double v = text.toDouble(&ok);
        // 清空热流密度时保留 key，值置空 QVariant（行仍被记录，写文件时过滤空值）
        QVector<QVariant>& values = _currentLoad->cellFaceData_[loadKey];
        if (values.isEmpty())
            values.resize(1);
        if (valueIndex == 0)
            values[0] = (ok && !text.isEmpty()) ? QVariant(v) : QVariant();

        updateLoadSymbolRealtime();
        notifyLoadChanged();
    }

    void mLoadHeatFluxPanel::slot_funcChanged(int idx)
    {
        Q_UNUSED(idx)
            saveFunc();
    }

    void mLoadHeatFluxPanel::slot_analysisFieldChanged(int idx)
    {
        Q_UNUSED(idx)
            saveAnalysisField();
    }

    void mLoadHeatFluxPanel::slot_valueInpWayChanged(int idx)
    {
        index = idx;
        if (_currentLoad)
            _currentLoad->way_ = index;

        // 清空旧拾取数据，避免切换类型后残留不匹配的拾取
        if (_pickWidgetController)
        {
            PickWidgetData empty;
            _pickWidgetController->replacePickedDataAndClearSets(empty.pickedData);
        }

        // 独立赋值模式下热流类型切换：清掉旧类型离散行数据，按新类型重建控件
        // （拾取对象/标识列随类型变化，旧类型行保留下来无法正确反显/写回）
        if (_defineTypeCombo.combobox->currentIndex() == 1)
        {
            if (_currentLoad)
                _currentLoad->cellFaceData_.clear();
            rebuildDiscreteControl();
            filterPicksByHeatFluxType();
        }

        if (index == 0)
            emit mMechGlobalSignals::getInstance()->activePickSig(5);
        else if (index == 1)
            emit mMechGlobalSignals::getInstance()->activePickSig(7);
        else if (index == 2)
            emit mMechGlobalSignals::getInstance()->activePickSig(8);

        applyAllowedPickTypes(idx);
        updateLoadSymbolRealtime();
    }
    void mLoadHeatFluxPanel::clearInputs()
    {
        if (_realtimeUpdateTimer)
        {
            _realtimeUpdateTimer->stop();
            delete _realtimeUpdateTimer;
            _realtimeUpdateTimer = nullptr;
        }

        if (_pickWidgetController)
        {
            _pickWidgetController->setPickActive(false);
            delete _pickWidgetController;
            _pickWidgetController = nullptr;
        }

        _heatFluxInput.clear();

        for (auto combo : comboBoxs)
        {
            combo.clear();
        }
        comboBoxs.clear();
        _defineTypeCombo.clear();
        _analysisFieldCombo.clear();
        _analysisFieldTable.clear();
        combofunc.clear();
        _scale.clear();
        _functable.clear();

        _currentLoad.reset();

        if (_tableRowWidget)
        {
            delete _tableRowWidget;
            _tableRowWidget = nullptr;
        }
        _tableLabel = nullptr;
        _discreteControl = nullptr; // 随 _tableRowWidget 一并删除
    }

    void mLoadHeatFluxPanel::setInputsValue(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData)
    {
        _currentLoad = std::dynamic_pointer_cast<MechCaseData::mLoadHeatFlux>(loadData);
        if (!_currentLoad)
            return;

        index = _currentLoad->way_;

        // 先按当前热流类型重建独立赋值离散数据控件（拾取对象/标识列随类型切换）
        rebuildDiscreteControl();
        // 单例面板复用：无论新载荷定义方式如何，先清空离散表格残留行，
        // 避免旧载荷独立赋值数据在用户切回独立赋值时复现并污染新载荷。
        // setRows 不发 dataChanged，不会反向污染新载荷，内部还会清掉视口残留选中态。
        if (_discreteControl)
        {
            _discreteControl->setRows({});
        }

        // 设置通用拾取控件持久化策略与集合列表
        if (_pickWidgetController)
        {
            auto strategy = std::make_shared<MapRelationPersistenceStrategy>(
                MxFunctions::Physical_LoadBoundary,
                _loadData->getLoadBoundaryId());
            _pickWidgetController->setPersistenceStrategy(strategy);
            _pickWidgetController->setSetResolver(&resolveSetToTypedData);

            QHash<QString, QString> setToType;
            if (index == 0)
            {
                setToType = getSetNamesList({ SetType::GeoPoint, SetType::GeoEdge, SetType::GeoFace, SetType::GeoSolid, SetType::Node });
            }
            else if (index == 1)
            {
                setToType = getSetNamesList({ SetType::GeoFace, SetType::GeoSolid, SetType::ElementSurface });
            }
            else if (index == 2)
            {
                setToType = getSetNamesList({ SetType::GeoFace, SetType::GeoSolid, SetType::Element });
            }
            _pickWidgetController->setSelectSetList(setToType);

            applyAllowedPickTypes(_currentLoad->way_);
        }

        // 热流类型下拉框
        comboBoxs[0].combobox->blockSignals(true);
        comboBoxs[0].combobox->setCurrentIndex(index);
        comboBoxs[0].combobox->blockSignals(false);

        // 定义方式下拉框
        _defineTypeCombo.combobox->blockSignals(true);
        _defineTypeCombo.combobox->setCurrentIndex(_currentLoad->defineType_);
        _defineTypeCombo.combobox->blockSignals(false);

        // 热流密度大小反显
        _heatFluxInput.lineeditvalue->clear();
        if (_currentLoad->pa_.isValid() && !_currentLoad->pa_.isNull())
        {
            _heatFluxInput.lineeditvalue->setExpression(QString::number(_currentLoad->pa_.toDouble()));
        }

        // 幅值曲线反显
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

        auto& mapload = getCVM->getModelData()->getMapRelationManager();
        int funcid = mapload.getMapRelation<int>({ Physical_LoadBoundary, loadData->getLoadBoundaryId() }, Physical_Function);
        if (funcid != -1)
        {
            QString funcname = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionNameByID(funcid);
            combofunc.combobox->setCurrentText(funcname);
        }
        combofunc.combobox->blockSignals(false);

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
        int analysisFieldId = mapload.getMapRelation<int>({ Physical_LoadBoundary, loadData->getLoadBoundaryId() }, Physical_Field, MapRelationType::MapRelation_AnalysisField);
        if (analysisFieldId != -1)
        {
            QString fieldname = analysisFieldMgr.getFieldNameByID(analysisFieldId);
            _analysisFieldCombo.combobox->setCurrentText(fieldname);
        }
        _analysisFieldCombo.combobox->blockSignals(false);

        // 独立赋值模式：把 map 反显进离散数据控件，行 key 为业务 key
        // （点：(nodeId,-1)；面：(cellId,faceId)；体：(cellId,-1)）
        if (_currentLoad->defineType_ == 1 && _discreteControl)
        {
            QVector<mDiscreteDataControl::Row> rows;
            rows.reserve(static_cast<int>(_currentLoad->cellFaceData_.size()));
            for (const auto& entry : _currentLoad->cellFaceData_)
            {
                QStringList values;
                const QVariant v = !entry.second.isEmpty() ? entry.second.at(0) : QVariant();
                values << ((v.isValid() && !v.isNull()) ? QString::number(v.toDouble()) : QString());
                rows.append(qMakePair(qMakePair(entry.first.result_id,
                    _currentLoad->way_ == 1 ? entry.first.result_id2 : -1), values));
            }
            _discreteControl->setRows(rows);
        }

        // 通用拾取控件反显（syncPicksToController 内部 displayData 默认不重画高亮）
        const bool wasPickActive = _pickWidgetController && _pickWidgetController->isPickActive();
        syncPicksToController();

        // 独立赋值/分析场模式：通用拾取控件只写数据（供 pickState/持久化一致性），
        // 全程保持失活，不激活、不渲染任何拾取高亮（高亮由离散数据控件直驱视口）
        if (_pickWidgetController)
        {
            if (_currentLoad->defineType_ == 1)
            {
                _pickWidgetController->setPickWidgetVisible(false);
                _pickWidgetController->setSelectSetBtnVisible(false);
                _pickWidgetController->setCloudChartMode(true);
                filterPicksByHeatFluxType();
            }
            else if (_currentLoad->defineType_ == 2)
            {
                _pickWidgetController->setPickWidgetVisible(false);
                _pickWidgetController->setSelectSetBtnVisible(false);
                _pickWidgetController->setCloudChartMode(true);
            }
            else
            {
                // 旧载荷在独立赋值模式下曾 setPickWidgetVisible(false)，
                // setPickActive 在隐藏态会被静默拦截，需先恢复可见性
                _pickWidgetController->setPickWidgetVisible(true);
                _pickWidgetController->setSelectSetBtnVisible(true);
                _pickWidgetController->setPickActive(true);
                // 控件此前已激活时 setPickActive(true) 直接返回、displayData 默认不重画高亮，
                // 需显式重画一次以匹配反显数据；整体保证进入面板只做一次全量高亮
                if (wasPickActive)
                {
                    _pickWidgetController->refreshHighlight();
                }
            }
        }

        // 缩放因子反显
        double scale = mapload.getScaleByCase({ MxFunctions::PhysicalFieldType::Physical_LoadBoundary, loadData->getLoadBoundaryId() }, _caseId);
        _scale.lineeditvalue->setExpression(MxFunctions::mNumberToString(scale));

        // 分析场方式隐藏缩放因子。切换载荷进入面板时各行控件已被上一轮
        // clearFoldWidgets 摘为无父（loadPanel 稍后才重挂入折叠框并统一 show），
        // 无父 QWidget 直接 show 会作为顶层窗口短暂闪现，故无父期不做显示
        if (_currentLoad->defineType_ == 2)
        {
            _scale.propwid->hide();
        }
        else if (_scale.propwid->parentWidget())
        {
            _scale.propwid->show();
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
        // 实时渲染热流密度符号
        updateLoadSymbolRealtime();
        //发送更新信号
        notifyLoadChanged();
    }
    QVector<QWidget*> mLoadHeatFluxPanel::getRefStateInputs(FoldSection* fathertar)
    {
        refstate_ = fathertar;
        QVector<QWidget*> wds;
        // 分析场方式无大小概念，不需要缩放因子（折叠框整体隐藏）
        if (_currentLoad && _currentLoad->defineType_ == 2)
            return wds;
        wds.append(_scale.propwid);
        return wds;
    }

    QVector<QWidget*> mLoadHeatFluxPanel::getTargetInputs(FoldSection* fathertar)
    {
        fatar_ = fathertar;
        QVector<QWidget*> wds;
        // 分析场方式无作用对象（折叠框整体隐藏）
        if (_currentLoad && _currentLoad->defineType_ == 2)
            return wds;
        if (_pickWidgetController)
        {
            wds.append(_pickWidgetController->widget());
        }
        return wds;
    }

    QVector<QWidget*> mLoadHeatFluxPanel::getValueInputs(FoldSection* fatherval)
    {
        faval_ = fatherval;
        QVector<QWidget*> wds;

        wds.append(comboBoxs[0].propwid); // 热流类型
        wds.append(_defineTypeCombo.propwid); // 定义方式

        int defineType = _defineTypeCombo.combobox->currentIndex();

        if (defineType == 0) // 统一赋值
        {
            wds.append(_heatFluxInput.propwid);
            wds.append(combofunc.propwid);
        }
        else if (defineType == 1) // 独立赋值
        {
            wds.append(_tableRowWidget);
            wds.append(combofunc.propwid);
        }
        else // 分析场：仅分析场下拉框 + 函数
        {
            wds.append(_analysisFieldCombo.propwid);
            wds.append(combofunc.propwid);
        }

        return wds;
    }

    int mLoadHeatFluxPanel::getFunc()
    {
        int funcid = -1;
        QString funcname = combofunc.combobox->currentText();
        if (_functable.find(funcname) != _functable.end())
        {
            funcid = _functable[funcname];
        }
        return funcid;
    }

    void mLoadHeatFluxPanel::deactivatePickAndCommit()
    {
        // 面板为单例复用，切换载荷/面板时不会走析构/clearInputs，
        // 由统一退出出口（unEditLoad）调用，失活并提交当前拾取数据。
        const bool renderValid = (getCVM && getCVM->getPreRenderWidget());

        // 离散数据控件：结束拾取并关闭弹窗
        if (_discreteControl)
        {
            _discreteControl->closePopup();
        }

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
            doUpdateLoadSymbolRealtime();
        }
    }

    void mLoadHeatFluxPanel::updateLoadSymbolRealtime()
    {
        if (!_pickWidgetController || !_currentLoad)
            return;
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;

        // 通过防抖定时器合并连续触发（方向/大小/拾取变化），避免每输入一个字符都重算并重绘。
        if (!_realtimeUpdateTimer)
            return;
        _realtimeUpdateTimer->start(100);
    }

    void mLoadHeatFluxPanel::onRealtimeUpdateTimeout()
    {
        doUpdateLoadSymbolRealtime();
    }

    void mLoadHeatFluxPanel::doUpdateLoadSymbolRealtime()
    {
        if (!_pickWidgetController || !_currentLoad)
            return;
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;
        // 模型已卸载但渲染部件仍在的中间态下 getModelData() 为空
        auto* modelData = getCVM->getModelData();
        if (!modelData)
            return;

        // 独立赋值：先按离散数据控件当前行同步 cellFaceData_（数据源为表格，
        // 通用拾取控制器已不再接收视口拾取，不能再以它为准）
        const bool independent = (_currentLoad->defineType_ == 1);
        if (independent)
        {
            updateOpenedTable();
        }

        // 实时渲染时数据尚未持久化到集合/映射，直接使用通用拾取控件当前高亮数据
        // （独立赋值已剥离通用拾取，merged 为空，符号位置直接遍历 cellFaceData_）
        const PickWidgetData allData = _pickWidgetController->getAllData();
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> merged = allData.pickedData;
        if (independent)
        {
            merged.clear();
            auto& vec = merged[heatFluxWayToSetType(_currentLoad->way_)];
            vec.reserve(_currentLoad->cellFaceData_.size());
            for (const auto& entry : _currentLoad->cellFaceData_)
            {
                vec.push_back(entry.first);
            }
        }

        // 分析场方式无作用对象，无需渲染热流符号
        if (_currentLoad->defineType_ == 2)
            return;

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
            case MxFunctions::Element:
                if (auto s = dynamic_cast<mElementSet*>(setBase)) { auto v = s->getElementSetData(); auto& d = merged[MxFunctions::PickDataType::Element]; d.insert(d.end(), v.begin(), v.end()); }
                break;
            case MxFunctions::ElementSurface:
                if (auto s = dynamic_cast<mElementSurface*>(setBase)) { auto v = s->getElementSurfaceData(); auto& d = merged[MxFunctions::PickDataType::ElementFace]; d.insert(d.end(), v.begin(), v.end()); }
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

        QVector<QVector3D> objs;
        auto meshData = getCVM->getModelData()->getMeshData();
        auto geoManager = getCVM->getModelData()->getGeoManager();
        auto geoData = getCVM->getModelData()->getGeoData();

        for (const auto& pair : merged)
        {
            if (pair.second.empty()) continue;

            std::vector<int> ids;
            ids.reserve(pair.second.size());
            for (const auto& d : pair.second)
                ids.push_back(d.result_id);

            switch (pair.first)
            {
            case MxFunctions::PickDataType::Node:
                for (int id : ids)
                {
                    QVector3D pos = meshData->getVectorById(id);
                    if (!pos.isNull()) objs.push_back(pos);
                }
                break;
            case MxFunctions::PickDataType::Element:
                for (const auto& d : pair.second)
                {
                    QVector3D centerPos;
                    QVector<QVector3D> vertexs;
                    meshData->GetShrinkVertexByMeshID(d.result_id, centerPos, vertexs, 5);
                    objs.push_back(centerPos);
                }
                break;
            case MxFunctions::PickDataType::ElementFace:
                for (const auto& d : pair.second)
                {
                    QVector3D centerPos;
                    QVector<QVector3D> vertex;
                    meshData->GetShrinkVertexByMeshFaceID(d, centerPos, vertex);
                    objs.push_back(centerPos);
                }
                break;
            case MxFunctions::PickDataType::GeoPoint:
            {
                auto points = geoManager->CalculateVertexAttr(ids);
                for (auto p : points) objs.push_back(p.m_coor);
                break;
            }
            case MxFunctions::PickDataType::GeoEdge:
            {
                QVector<int> pids; for (int id : ids) pids.append(id);
                auto coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(1, pids);
                for (auto c : coords) objs.push_back(c);
                break;
            }
            case MxFunctions::PickDataType::GeoFace:
            {
                QVector<int> pids; for (int id : ids) pids.append(id);
                auto coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(2, pids);
                for (auto c : coords) objs.push_back(c);
                break;
            }
            case MxFunctions::PickDataType::GeoSolid:
            {
                QVector<int> pids; for (int id : ids) pids.append(id);
                auto coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(3, pids);
                for (auto c : coords) objs.push_back(c);
                break;
            }
            default:
                break;
            }
        }

        // 最终数据签名：载荷ID+位置集+热流方式未变化时跳过符号赋值与 UpdateAllSymbol
        // 全量重建（进入面板防抖触发/切出冲刷时数据往往与上次相同）
        size_t sig = AllRenderSymbolManager::beginRenderSignature();
        AllRenderSymbolManager::combineRenderSignature(sig, static_cast<size_t>(_currentLoad->getLoadBoundaryId()));
        AllRenderSymbolManager::combineRenderSignature(sig, objs);
        AllRenderSymbolManager::combineRenderSignature(sig, static_cast<size_t>(_currentLoad->way_));
        if (sig == _lastRenderSig)
        {
            return;
        }
        _lastRenderSig = sig;

        AllRenderSymbolManager::getInstance()->assignThermalLoad(_currentLoad, objs, "Load_HeatFlux", QVector3D(255.0f, 255.0f, 0.0f), _currentLoad->way_);

        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        if (symbolController) symbolController->UpdateAllSymbol();
        getCVM->getPreRenderWidget()->updatePaint();
    }
}