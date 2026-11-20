#include "mLoadRadiationPanel.h"
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
#include "AllRenderSymbolManager.h"
#include "mGeneralPickUp.h"
#include "mGetSetTypeTemplate.h"
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

#include "mLoadRadiationPanel.h"
#include "mOtherFunction.h"

using namespace MechCommon;
using namespace MechPreData;

namespace MechMainWindow
{
    mLoadRadiationPanel::mLoadRadiationPanel(QWidget* parent)
        :mLoadPanelBase(parent)
    {
        initInputs();
    }

    mLoadRadiationPanel::~mLoadRadiationPanel()
    {
        clearInputs();
    }

    std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> mLoadRadiationPanel::getPickedData()
    {
        if (!_pickWidgetController)
            return std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>();
        return _pickWidgetController->getAllData().pickedData;
    }

    void mLoadRadiationPanel::filterPicksValid()
    {
        if (!_pickWidgetController)
            return;

        auto picks = _pickWidgetController->getAllData().pickedData;
        auto it = picks.begin();
        while (it != picks.end())
        {
            if (it->first != MxFunctions::PickDataType::ElementFace)
                it = picks.erase(it);
            else
                ++it;
        }

        _pickWidgetController->replacePickedDataAndClearSets(picks);
        updateOpenedTable();
    }

    void mLoadRadiationPanel::updateOpenedTable()
    {
        if (!_currentLoad || !_discreteControl)
            return;

        // 独立赋值模式下，cellFaceData_ 与离散数据控件的行数据保持严格一致：
        // 表格是数据的权威来源（含拾取追加与手动删除），此处按控件当前行重建 map。
        // 行 key 为业务 key（单元ID, 面序号），重建时优先保留 cellFaceData_ 中原有
        // 完整 key（含 part_id）与已填值；新行按单元 ID 反查 part_id 构造完整 key，
        // 与 mIMxFile 读入键规范 (cellId, partId, faceId) 保持一致。
        const auto rows = _discreteControl->rows();

        std::map<QPair<int, int>, RenderPickResultData> existingKeys;
        std::map<QPair<int, int>, QVector<QVariant>> existingValues;
        for (const auto& entry : _currentLoad->cellFaceData_)
        {
            QPair<int, int> bizKey(entry.first.result_id, entry.first.result_id2);
            if (existingKeys.find(bizKey) == existingKeys.end())
            {
                existingKeys[bizKey] = entry.first;
                existingValues[bizKey] = entry.second;
            }
        }

        auto meshData = getCVM->getModelData()->getMeshData();
        QHash<int, int> partIdByCellId;
        std::map<RenderPickResultData, QVector<QVariant>> newMap;
        for (const auto& row : rows)
        {
            const int cellId = row.first.first;
            const int faceId = row.first.second;
            if (cellId <= 0 || faceId <= 0)
                continue;

            const QPair<int, int> bizKey(cellId, faceId);
            auto keyIt = existingKeys.find(bizKey);
            if (keyIt != existingKeys.end())
            {
                // 保留原有完整 key（含 part_id）与已填值（含空值占位）
                newMap[keyIt->second] = existingValues[bizKey];
                continue;
            }

            // 新增行：按单元 ID 反查 part_id 构造完整 key，空值占位
            auto partIt = partIdByCellId.constFind(cellId);
            if (partIt == partIdByCellId.constEnd())
            {
                const int partId = meshData ? meshData->getPartIdByEleId(cellId) : -1;
                partIt = partIdByCellId.insert(cellId, partId);
            }
            newMap[RenderPickResultData(cellId, partIt.value(), faceId)] = { QVariant(), QVariant() };
        }

        _currentLoad->cellFaceData_ = std::move(newMap);
    }

    void mLoadRadiationPanel::applyAllowedPickTypes(int defineType)
    {
        if (!_pickWidgetController) return;
        if (defineType == 1)
        {
            // 独立赋值：仅设置白名单记录，不切换视口过滤器
            // （拾取过滤器由离散数据控件在开始拾取时自行激活/恢复）
            _pickWidgetController->setAllowedTypes(&pickOnlyElementSurface);
        }
        else if (defineType == 2)
        {
            // 分析场：无拾取，仅设置白名单记录，不切换视口过滤器
            _pickWidgetController->setAllowedTypes(&pickOnlyElementSurface);
        }
        else
        {
            // 统一赋值：几何面 + 单元面
            _pickWidgetController->setAllowedTypes(&pickGeoFaceAndElementSurface);
            _pickWidgetController->setPickType();
        }
    }

    void mLoadRadiationPanel::saveV1V2()
    {
        if (!_currentLoad)
            return;

        QString strV1 = inputWidgets[0].lineeditvalue->text().trimmed();
        QString strV2 = inputWidgets[1].lineeditvalue->text().trimmed();
        _currentLoad->V1 = strV1.isEmpty() ? QVariant() : QVariant::fromValue<double>(strV1.toDouble());
        _currentLoad->V2 = strV2.isEmpty() ? QVariant() : QVariant::fromValue<double>(strV2.toDouble());

        notifyLoadChanged();
    }

    void mLoadRadiationPanel::saveFunc()
    {
        if (!_currentLoad)
            return;
        int funcid = getFunc();
        auto& mapload = getCVM->getModelData()->getMapRelationManager();
        mapload.appendMapRelation({ Physical_LoadBoundary, _loadData->getLoadBoundaryId() }, { Physical_Function, funcid });
        notifyLoadChanged();
    }

    void mLoadRadiationPanel::saveAnalysisField()
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

    void mLoadRadiationPanel::saveScale()
    {
        if (!_currentLoad)
            return;
        double scale = _scale.lineeditvalue->text().trimmed().toDouble();
        auto& mapload = getCVM->getModelData()->getMapRelationManager();
        mapload.setScaleByCase({ MxFunctions::PhysicalFieldType::Physical_LoadBoundary, _loadData->getLoadBoundaryId() }, _caseId, scale);
        notifyLoadChanged();
    }

    void mLoadRadiationPanel::notifyLoadChanged()
    {
        if (!_loadData)
            return;
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_LoadBoundary, _loadData->getLoadBoundaryId());
    }
    void mLoadRadiationPanel::initInputs()
    {
        // 定义方式下拉框
        _defineTypeCombo.initWidget("定义方式");
        _defineTypeCombo.combobox->addItems({ "统一赋值", "独立赋值", "分析场" });
        _defineTypeCombo.combobox->setMinimumWidth(100);
       // _defineTypeCombo.pushbtn->hide();
        _defineTypeCombo.combobox->setCurrentIndex(0);
        connect(_defineTypeCombo.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mLoadRadiationPanel::slot_defineTypeChanged);

        // 分析场方式：已有分析场下拉框（无新建按钮，仅选择已有分析场）
        _analysisFieldCombo.initWidget("分析场");
        _analysisFieldCombo.combobox->setFixedHeight(30);
        _analysisFieldCombo.combobox->addItem("");
        _analysisFieldTable[""] = -1;
        auto& analysisFieldMgrInit = getCVM->getModelData()->getManager<MechDiscreteFieldData::mFieldManager>(Physical_Field);
        auto allAnalysisField = analysisFieldMgrInit.getAllFieldID(MxFunctions::FieldType::Analysis);
        for (auto fieldid : allAnalysisField)
        {
            QString fieldname = analysisFieldMgrInit.getFieldNameByID(fieldid);
            _analysisFieldCombo.combobox->addItem(fieldname);
            _analysisFieldTable[fieldname] = fieldid;
        }
        connect(_analysisFieldCombo.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mLoadRadiationPanel::slot_analysisFieldChanged);

        // 环境温度、热辐射系数
        QStringList inpnames = { "环境温度", "热辐射系数" };
        for (int i = 0; i < 2; ++i)
        {
            InputWidgetSet inp;
            inp.initWidget(inpnames[i], true);
            inp.labelname->setText(inpnames[i]);
            inp.labelunit->setVisible(false);
            inputWidgets.append(inp);
            connect(inp.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this]() {
                    saveV1V2();
                });
        }

        // 独立赋值表格行：通用离散数据控件（入口按钮 + 锚定弹窗表格）
        _tableRowWidget = new QWidget();
        QHBoxLayout* tableRowLayout = new QHBoxLayout(_tableRowWidget);
        tableRowLayout->setContentsMargins(0, 0, 0, 0);
        tableRowLayout->setSpacing(8);
        _tableLabel = new QLabel("单元面数据");
        _tableLabel->setMinimumWidth(87);
        tableRowLayout->addWidget(_tableLabel);

        mDiscreteDataControl::PickSpec faceSpec;
        faceSpec.typeName = QStringLiteral("单元面");
        faceSpec.idHeaders = QStringList{ QStringLiteral("单元编号"), QStringLiteral("面编号") };
        // 业务 key = (单元ID, 面序号)：part_id 不参与行 key，
        // 面板侧在重建/反显 map 时按 cellFaceData_ 原有完整 key 回填 part_id
        faceSpec.keyOf = [](const RenderPickResultData& d) { return qMakePair(d.result_id, d.result_id2); };
        faceSpec.pickTypes = { MxFunctions::PickDataType::ElementFace };
        faceSpec.pickFilter = 7; // 视口拾取过滤器：仅单元面（与 activePickSig 索引一致）
        // 拾取由离散数据控件直接驱动视口选择容器，不再经过通用拾取控件
        _discreteControl = new mDiscreteDataControl(
            QStringLiteral("单元面数据"),
            { QStringLiteral("环境温度"), QStringLiteral("热辐射系数") },
            &faceSpec, _tableRowWidget);
        _discreteControl->setRenderButtonVisible(false);
        connect(_discreteControl, &mDiscreteDataControl::dataChanged,
            this, &mLoadRadiationPanel::slot_discreteDataChanged);
        connect(_discreteControl, &mDiscreteDataControl::cellEdited,
            this, &mLoadRadiationPanel::slot_discreteCellEdited);
        // 入口按钮从控件中剖分出来直接放入行布局，控件本体隐藏（弹窗为
        // 宿主主窗口子窗口不受影响），按钮可用状态与控件本体显隐/禁用解耦
        if (QPushButton* entry = _discreteControl->entryButton())
        {
            _discreteControl->detachEntryButton(_tableRowWidget);
            tableRowLayout->addWidget(entry, 1);
            _discreteControl->hide();
        }
        else
        {
            tableRowLayout->addWidget(_discreteControl);
        }

        // 通用拾取控件
        _pickWidgetController = mGeneralPickUp::createPickWidget(CBBlue);
        connect(_pickWidgetController, &mPickWidgetController::pickActiveChanged, this, [this](bool isActive) {
            if (isActive)
            {
                emit mMechGlobalSignals::getInstance()->activePickSig(7);
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

        // 函数
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
            this, &mLoadRadiationPanel::slot_funcChanged);
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
    void mLoadRadiationPanel::slot_defineTypeChanged(int idx)
    {
        if (_currentLoad)
            _currentLoad->defineType_ = idx;

        // 切换定义方式时关闭离散数据弹窗并结束拾取，避免旧模式弹窗残留
        if (_discreteControl)
        {
            _discreteControl->closePopup();
        }

        if (idx == 1)
        {
            // 独立赋值：过滤拾取为仅单元面，并彻底关闭通用拾取控件的高亮渲染
            // （拾取由离散数据控件直接驱动视口选择容器）
            filterPicksValid();
            if (_pickWidgetController)
            {
                _pickWidgetController->setPickWidgetVisible(false);
                _pickWidgetController->setCloudChartMode(true);
            }
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
                _pickWidgetController->setCloudChartMode(false);
                _pickWidgetController->setPickActive(true);
            }
        }

        applyAllowedPickTypes(idx);

        faval_->removeAll();
        faval_->addRow(_defineTypeCombo.propwid);
        _defineTypeCombo.propwid->show();

        if (idx == 0)
        {
            // 统一赋值：输入框 + 函数
            faval_->addRow(inputWidgets[0].propwid);
            inputWidgets[0].propwid->show();
            faval_->addRow(inputWidgets[1].propwid);
            inputWidgets[1].propwid->show();
            faval_->addRow(combofunc.propwid);
            combofunc.propwid->show();
        }
        else if (idx == 1)
        {
            // 独立赋值：表格 + 函数
            faval_->addRow(_tableRowWidget);
            _tableRowWidget->show();
            faval_->addRow(combofunc.propwid);
            combofunc.propwid->show();
        }
        else
        {
            // 分析场：仅分析场下拉框 + 函数
            faval_->addRow(_analysisFieldCombo.propwid);
            _analysisFieldCombo.propwid->show();
            faval_->addRow(combofunc.propwid);
            combofunc.propwid->show();
        }
        faval_->refreshLayout();

        // 独立赋值/分析场模式：隐藏集合选择按钮，强制仅单元面
        if (_pickWidgetController)
        {
            _pickWidgetController->setSelectSetBtnVisible(idx == 0);
        }

        // 定义方式切换后立即按新模式刷新符号（统一/独立按各自取值渲染）
        updateLoadSymbolRealtime();

        // 定义方式切换属于结构性变化，仍通过 sg_rebuildPanel 由外部槽统一整体保存
        emit sg_rebuildPanel();
    }

    void mLoadRadiationPanel::slot_discreteDataChanged()
    {
        // 离散数据控件行变化（拾取追加/删除/清空）：
        // 按控件当前行重建 cellFaceData_，随后刷新实时符号并通知外部。
        // 拾取会话进行中跳过：确认拾取（完成拾取）后控件会补发一次 dataChanged，
        // 此时才执行重建/渲染，避免拾取期间反复重绘。
        if (!_currentLoad || (_discreteControl && _discreteControl->isPicking()))
            return;
        updateOpenedTable();
        updateLoadSymbolRealtime();
        notifyLoadChanged();
    }

    void mLoadRadiationPanel::slot_discreteCellEdited(const QPair<int, int>& key, int valueIndex, const QString& text)
    {
        if (!_currentLoad || key.first <= 0)
            return;

        // 行 key 只有（单元ID, 面序号）两个槽位，按业务 key 在 cellFaceData_ 中查找
        // 已有完整 key（保留原 part_id）；不存在时按单元 ID 反查 part_id 构造完整 key，
        // 与 mIMxFile 读入键规范 (cellId, partId, faceId) 保持一致。
        auto it = _currentLoad->cellFaceData_.begin();
        for (; it != _currentLoad->cellFaceData_.end(); ++it)
        {
            if (it->first.result_id == key.first && it->first.result_id2 == key.second)
                break;
        }

        RenderPickResultData loadKey;
        if (it != _currentLoad->cellFaceData_.end())
        {
            loadKey = it->first;
        }
        else
        {
            auto meshData = getCVM->getModelData()->getMeshData();
            const int partId = meshData ? meshData->getPartIdByEleId(key.first) : -1;
            loadKey = RenderPickResultData(key.first, partId, key.second);
        }

        bool ok = false;
        const double v = text.toDouble(&ok);
        const QVariant value = (ok && !text.isEmpty()) ? QVariant(v) : QVariant();
        // 清空数值时保留 key，值置空 QVariant（行仍被记录，写文件时过滤空值）
        QVector<QVariant>& values = _currentLoad->cellFaceData_[loadKey];
        if (values.size() < 2)
            values.resize(2);
        if (valueIndex >= 0 && valueIndex < 2)
            values[valueIndex] = value;

        updateLoadSymbolRealtime();
        notifyLoadChanged();
    }

    void mLoadRadiationPanel::slot_funcChanged(int idx)
    {
        Q_UNUSED(idx)
            saveFunc();
    }

    void mLoadRadiationPanel::slot_analysisFieldChanged(int idx)
    {
        Q_UNUSED(idx)
            saveAnalysisField();
    }
    void mLoadRadiationPanel::clearInputs()
    {
        for (auto& inp : inputWidgets) inp.clear();
        inputWidgets.clear();

        if (_pickWidgetController)
        {
            _pickWidgetController->setPickActive(false);
            delete _pickWidgetController;
            _pickWidgetController = nullptr;
        }

        _defineTypeCombo.clear();
        _analysisFieldCombo.clear();
        _analysisFieldTable.clear();
        combofunc.clear();
        _functable.clear();
        _scale.clear();
        _currentLoad.reset();

        if (_tableRowWidget)
        {
            delete _tableRowWidget;
            _tableRowWidget = nullptr;
        }
        _tableLabel = nullptr;
        _discreteControl = nullptr; // 随 _tableRowWidget 一并删除
    }

    void mLoadRadiationPanel::setInputsValue(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData)
    {
        _currentLoad = std::dynamic_pointer_cast<MechCaseData::mLoadRadiation>(loadData);
        if (!_currentLoad)
            return;

        // V1、V2 反显
        QString V1str = _currentLoad->V1.isNull() ? "" : QString::number(_currentLoad->V1.toDouble());
        QString V2str = _currentLoad->V2.isNull() ? "" : QString::number(_currentLoad->V2.toDouble());
        inputWidgets[0].lineeditvalue->setExpression(V1str);
        inputWidgets[1].lineeditvalue->setExpression(V2str);

        // 回填定义方式
        _defineTypeCombo.combobox->blockSignals(true);
        _defineTypeCombo.combobox->setCurrentIndex(_currentLoad->defineType_);
        _defineTypeCombo.combobox->blockSignals(false);

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

        // 通用拾取控件持久化策略与集合列表
        if (_pickWidgetController)
        {
            auto strategy = std::make_shared<MapRelationPersistenceStrategy>(
                MxFunctions::Physical_LoadBoundary,
                _loadData->getLoadBoundaryId());
            _pickWidgetController->setPersistenceStrategy(strategy);
            _pickWidgetController->setSetResolver(&resolveSetToTypedData);

            QHash<QString, QString> setToType = getSetNamesList({ SetType::ElementSurface, SetType::GeoFace });
            _pickWidgetController->setSelectSetList(setToType);

            applyAllowedPickTypes(_currentLoad->defineType_);
        }

        // 拾取数据反显
        // 单例面板复用：无论新载荷定义方式如何，先清空离散表格残留行，
        // 避免旧载荷独立赋值数据在用户切回独立赋值时复现并污染新载荷。
        // setRows 不发 dataChanged，不会反向污染新载荷，内部还会清掉视口残留选中态。
        if (_discreteControl)
        {
            _discreteControl->setRows({});
        }
        PickWidgetData displayData;
        if (_currentLoad->defineType_ == 2)
        {
            // 分析场：无拾取数据，displayData 保持空
        }
        else if (_currentLoad->defineType_ == 1)
        {
            // 独立赋值：直接从 cellFaceData_ 生成拾取数据，不依赖集合映射
            std::vector<RenderPickResultData> facePicks;
            for (const auto& entry : _currentLoad->cellFaceData_)
            {
                facePicks.push_back(entry.first);
            }
            if (!facePicks.empty())
            {
                displayData.pickedData[MxFunctions::PickDataType::ElementFace] = std::move(facePicks);
            }

            // 反显到离散数据控件（单元面 + 已填环境温度/热辐射系数），行 key 为业务 key（单元ID, 面序号）
            if (_discreteControl)
            {
                QVector<mDiscreteDataControl::Row> rows;
                rows.reserve(static_cast<int>(_currentLoad->cellFaceData_.size()));
                for (const auto& entry : _currentLoad->cellFaceData_)
                {
                    QStringList values;
                    for (int i = 0; i < 2; ++i)
                    {
                        const QVariant v = i < entry.second.size() ? entry.second.at(i) : QVariant();
                        values << ((v.isValid() && !v.isNull()) ? QString::number(v.toDouble()) : QString());
                    }
                    rows.append(qMakePair(qMakePair(entry.first.result_id, entry.first.result_id2), values));
                }
                _discreteControl->setRows(rows);
            }
        }
        else
        {
            int setid = mapload.getMapRelation<int>({ Physical_LoadBoundary, loadData->getLoadBoundaryId() }, Physical_Set);
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
        }

        if (_pickWidgetController)
        {
            if (_currentLoad->defineType_ == 1)
            {
                // 独立赋值已剥离通用拾取：仅写入数据（供 pickState/持久化一致性），
                // 全程保持失活，不激活、不渲染任何拾取高亮
                _pickWidgetController->setPickWidgetVisible(false);
                _pickWidgetController->setCloudChartMode(true);
                _pickWidgetController->displayData(displayData);
            }
            else if (_currentLoad->defineType_ == 2)
            {
                // 分析场：无作用对象，仅写入空数据保持 pickState 一致，不激活、不渲染
                _pickWidgetController->setPickWidgetVisible(false);
                _pickWidgetController->setSelectSetBtnVisible(false);
                _pickWidgetController->setCloudChartMode(true);
                _pickWidgetController->displayData(displayData);
            }
            else
            {
                // displayData 默认不重画高亮；若控件此前已处于激活态，setPickActive(true) 会直接返回，
                // 该情况下需显式重画一次高亮以匹配反显数据。整体保证进入面板只做一次全量高亮。
                // 旧载荷在独立赋值模式下曾 setPickWidgetVisible(false)，
                // setPickActive 在隐藏态会被静默拦截，需先恢复可见性
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

        // 同步集合按钮显隐
        if (_pickWidgetController)
        {
            _pickWidgetController->setSelectSetBtnVisible(_currentLoad->defineType_ == 0);
        }

        // 缩放因子回填
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

        updateOpenedTable();
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
        // 延迟刷新符号：等折叠块布局完成后（控件尺寸/渲染上下文就绪）再渲染，
        // 避免 setInputsValue 阶段 _pickWidgetController 尚未显示导致符号位置计算失败。
        QMetaObject::invokeMethod(this, &mLoadRadiationPanel::updateLoadSymbolRealtime, Qt::QueuedConnection);
    }
    QVector<QWidget*> mLoadRadiationPanel::getRefStateInputs(FoldSection* fathertar)
    {
        refstate_ = fathertar;
        QVector<QWidget*>wds;
        // 分析场方式无大小概念，不需要缩放因子（折叠框整体隐藏）
        if (_currentLoad && _currentLoad->defineType_ == 2)
            return wds;
        wds.append(_scale.propwid);
        return wds;
    }

    QVector<QWidget*> mLoadRadiationPanel::getTargetInputs(FoldSection* fathertar)
    {
        fatar_ = fathertar;
        QVector<QWidget*>wds;
        // 分析场方式无作用对象（折叠框整体隐藏）
        if (_currentLoad && _currentLoad->defineType_ == 2)
            return wds;
        if (_pickWidgetController)
        {
            wds.append(_pickWidgetController->widget());
        }
        return wds;
    }

    QVector<QWidget*> mLoadRadiationPanel::getValueInputs(FoldSection* fatherval)
    {
        faval_ = fatherval;
        QVector<QWidget*> wds;

        wds.append(_defineTypeCombo.propwid);
        int defineType = _defineTypeCombo.combobox->currentIndex();

        if (defineType == 0)
        {
            wds.append(inputWidgets[0].propwid);
            wds.append(inputWidgets[1].propwid);
            wds.append(combofunc.propwid);
        }
        else if (defineType == 1)
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

    int mLoadRadiationPanel::getFunc()
    {
        int funcid = -1;
        QString funcname = combofunc.combobox->currentText();
        if (_functable.find(funcname) != _functable.end())
        {
            funcid = _functable[funcname];
        }
        return funcid;
    }

    void mLoadRadiationPanel::deactivatePickAndCommit()
    {
        // 离散数据控件：结束拾取并关闭弹窗
        if (_discreteControl)
        {
            _discreteControl->closePopup();
        }

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
    void mLoadRadiationPanel::updateLoadSymbolRealtime()
    {
        if (!_pickWidgetController || !_currentLoad)
            return;
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;

        // 分析场方式无作用对象，无需渲染热辐射符号
        if (_currentLoad->defineType_ == 2)
            return;

        const int defineType = _defineTypeCombo.combobox->currentIndex();

        // 独立赋值：先按离散数据控件当前行同步 cellFaceData_（数据源为表格，
        // 通用拾取控制器已不再接收视口拾取，不能再以它为准），再合成符号位置。
        if (defineType == 1)
        {
            updateOpenedTable();
        }

        // 1. 汇总当前数据：拾取(pickedData) + 集合(selectedSetNames 按名称展开），合并为按类型分组的 map。
        // 独立赋值已剥离通用拾取，pickedData 停滞，merged 保持为空，符号位置直接遍历 cellFaceData_。
        const PickWidgetData allData = _pickWidgetController->getAllData();
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> merged;
        if (defineType != 1)
        {
            merged = allData.pickedData;
        }

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
            case MxFunctions::ElementSurface:
                if (auto s = dynamic_cast<mElementSurface*>(setBase)) { auto v = s->getElementSurfaceData(); auto& d = merged[MxFunctions::PickDataType::ElementFace]; d.insert(d.end(), v.begin(), v.end()); }
                break;
            case MxFunctions::GeoFace:
            case MxFunctions::GeoSolid:
                if (auto s = dynamic_cast<mGeoSet*>(setBase)) appendIds(SetTypeToPickDataType(setBase->getSetType()), s->getGeoSetData(), merged);
                break;
            default:
                break;
            }
        }

        // 2. 按类型计算作用对象坐标。
        auto meshData = getCVM->getModelData()->getMeshData();
        auto geoManager = getCVM->getModelData()->getGeoManager();
        auto geoData = getCVM->getModelData()->getGeoData();
        QVector<QVector3D> objs;
        if (defineType == 1)
        {
            // 独立赋值：符号位置直接遍历 cellFaceData_（表格为权威数据源）
            for (const auto& entry : _currentLoad->cellFaceData_)
            {
                QVector3D centerPos;
                QVector<QVector3D> vertex;
                meshData->GetShrinkVertexByMeshFaceID(entry.first, centerPos, vertex);
                if (!centerPos.isNull())
                    objs.push_back(centerPos);
            }
        }
        else
        {
            for (const auto& pair : merged)
            {
                if (pair.second.empty()) continue;

            switch (pair.first)
            {
            case MxFunctions::PickDataType::ElementFace:
            {
                for (const auto& d : pair.second)
                {
                    QVector3D centerPos;
                    QVector<QVector3D> vertex;
                    meshData->GetShrinkVertexByMeshFaceID(RenderPickResultData(d.result_id, d.part_id, d.result_id2), centerPos, vertex);
                    if (!centerPos.isNull())
                        objs.push_back(centerPos);
                }
                break;
            }
            case MxFunctions::PickDataType::GeoFace:
            {
                std::vector<int> ids;
                ids.reserve(pair.second.size());
                for (const auto& d : pair.second)
                    ids.push_back(d.result_id);
                auto points = geoManager->CalculateFaceAttr(ids);
                auto coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(2, QVector<int>::fromStdVector(ids));
                for (auto c : coords) objs.push_back(c);
                break;
            }
            case MxFunctions::PickDataType::GeoSolid:
            {
                std::vector<int> ids;
                ids.reserve(pair.second.size());
                for (const auto& d : pair.second)
                    ids.push_back(d.result_id);
                auto coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(3, QVector<int>::fromStdVector(ids));
                for (auto c : coords) objs.push_back(c);
                break;
            }
            default:
                break;
            }
            }
        }

        // 3. 重画该载荷的热辐射符号。
        AllRenderSymbolManager::getInstance()->assignThermalLoad(
            _currentLoad, objs, "Load_Radiation", QVector3D(255.0f, 97.0f, 0.0f));

        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        if (symbolController) symbolController->UpdateAllSymbol();
        getCVM->getPreRenderWidget()->updatePaint();
    }
}