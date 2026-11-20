#include "mLoadConvectionPanel.h"
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
#include "mDockLayoutManager.h"
#include "mGetSetFunction.h"
#include "mAppConfiguration.h"

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

#include <QFileDialog>
#include "mOtherFunction.h"

using namespace MechCommon;
using namespace MechPreData;

namespace MechMainWindow
{
    mLoadConvectionPanel::mLoadConvectionPanel(QWidget* parent)
        :mLoadPanelBase(parent)
    {
        _realtimeUpdateTimer = new QTimer(this);
        _realtimeUpdateTimer->setSingleShot(true);
        connect(_realtimeUpdateTimer, &QTimer::timeout, this, &mLoadConvectionPanel::onRealtimeUpdateTimeout);

        initInputs();
    }

    mLoadConvectionPanel::~mLoadConvectionPanel()
    {
        clearInputs();
    }

    std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> mLoadConvectionPanel::getPickedData()
    {
        if (!_pickWidgetController)
            return std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>();
        return _pickWidgetController->getAllData().pickedData;
    }

    void mLoadConvectionPanel::updateOpenedTable()
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

            // 新增行：按单元 ID 反查 part_id 构造完整 key，空值占位（2 分量：环境温度、对流换热系数）
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

    void mLoadConvectionPanel::filterPicksToCellFaceOnly()
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
        // 独立赋值：彻底关闭通用拾取控件的高亮渲染（拾取由离散数据控件直驱视口）
        _pickWidgetController->setCloudChartMode(true);
        updateOpenedTable();
    }

    void mLoadConvectionPanel::syncPicksToController()
    {
        if (!_pickWidgetController || !_currentLoad)
            return;

        PickWidgetData displayData;
        auto& mapload = getCVM->getModelData()->getMapRelationManager();
        int setid = mapload.getMapRelation<int>({ Physical_LoadBoundary, _loadData->getLoadBoundaryId() }, Physical_Set);

        // 统一赋值/文件模式：从集合映射反显；分析场无作用对象，跳过
        if (setid != -1 && _currentLoad->defineType_ != 1 && _currentLoad->defineType_ != 3)
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

        // 独立赋值：没有集合映射时，从数据类生成拾取数据
        if (_currentLoad->defineType_ == 1 && displayData.pickedData.empty() && displayData.selectedSetNames.isEmpty())
        {
            std::vector<RenderPickResultData> facePicks;
            for (const auto& entry : _currentLoad->cellFaceData_)
            {
                facePicks.push_back(entry.first);
            }
            if (!facePicks.empty())
            {
                displayData.pickedData[PickDataType::ElementFace] = std::move(facePicks);
            }
        }

        _pickWidgetController->displayData(displayData);
    }

    void mLoadConvectionPanel::applyAllowedPickTypes()
    {
        if (!_pickWidgetController)
            return;
        bool independent = _currentLoad ? (_currentLoad->defineType_ == 1)
            : (_defineTypeCombo.combobox->currentIndex() == 1);

        if (independent)
        {
            // 独立赋值：仅设置白名单记录，不切换视口过滤器
            // （拾取过滤器由离散数据控件在开始拾取时自行激活/恢复）
            _pickWidgetController->setAllowedTypes(&pickOnlyElementSurface);
            _pickWidgetController->setSelectSetBtnVisible(false);
        }
        else if (_currentLoad && _currentLoad->defineType_ == 3)
        {
            // 分析场：有作用对象拾取（mx文件有set关键字），正常设置拾取类型
            _pickWidgetController->setAllowedTypes(&pickGeoFaceAndElementSurface);
            _pickWidgetController->setSelectSetBtnVisible(true);
            _pickWidgetController->setPickType();
        }
        else
        {
            _pickWidgetController->setAllowedTypes(&pickGeoFaceAndElementSurface);
            _pickWidgetController->setSelectSetBtnVisible(true);
            _pickWidgetController->setPickType();
        }
    }

    void mLoadConvectionPanel::initInputs()
    {
        // 定义方式下拉框
        _defineTypeCombo.initWidget("定义方式");
        _defineTypeCombo.combobox->addItem("统一赋值");
        _defineTypeCombo.combobox->addItem("独立赋值");
        _defineTypeCombo.combobox->addItem("对流换热文件");
        _defineTypeCombo.combobox->addItem("分析场");
        _defineTypeCombo.combobox->setFixedHeight(30);
        _defineTypeCombo.combobox->setCurrentIndex(0);
        connect(_defineTypeCombo.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mLoadConvectionPanel::slot_defineTypeChanged);

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
            this, &mLoadConvectionPanel::slot_analysisFieldChanged);

        // 对流换热系数分析场下拉框（convfield）
        _convAnalysisFieldCombo.initWidget("对流换热分析场");
        _convAnalysisFieldCombo.combobox->setFixedHeight(30);
        _convAnalysisFieldCombo.combobox->addItem("");
        _convAnalysisFieldTable[""] = -1;
        for (auto fieldid : allAnalysisField)
        {
            QString fieldname = analysisFieldMgrInit.getFieldNameByID(fieldid);
            _convAnalysisFieldCombo.combobox->addItem(fieldname);
            _convAnalysisFieldTable[fieldname] = fieldid;
        }
        connect(_convAnalysisFieldCombo.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mLoadConvectionPanel::slot_convAnalysisFieldChanged);

        // 统一赋值输入框
        QStringList inpnames = { "环境温度", "对流换热系数" };
        for (int i = 0; i < 2; ++i)
        {
            InputWidgetSet inp;
            inp.initWidget(inpnames[i], true);
            inp.labelname->setText(inpnames[i]);
            inp.lineeditvalue->setFixedHeight(30);
            inputWidgets.append(inp);
            connect(inp.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this]() {
                    saveConvectionValue();
                    updateLoadSymbolRealtime();
                });
        }

        // 独立赋值表格行：通用离散数据控件（入口按钮 + 锚定弹窗表格）
        _tableRowWidget = new QWidget();
        QHBoxLayout* tableLayout = new QHBoxLayout(_tableRowWidget);
        tableLayout->setContentsMargins(0, 0, 0, 0);
        tableLayout->setSpacing(8);
        _tableLabel = new QLabel("单元面数据");
        _tableLabel->setMinimumWidth(87);
        tableLayout->addWidget(_tableLabel);

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
            { QStringLiteral("环境温度"), QStringLiteral("对流换热系数") },
            &faceSpec, _tableRowWidget);
        _discreteControl->setRenderButtonVisible(false);
        connect(_discreteControl, &mDiscreteDataControl::dataChanged,
            this, &mLoadConvectionPanel::slot_discreteDataChanged);
        connect(_discreteControl, &mDiscreteDataControl::cellEdited,
            this, &mLoadConvectionPanel::slot_discreteCellEdited);
        // 入口按钮从控件中剖分出来直接放入行布局，控件本体隐藏（弹窗为
        // 宿主主窗口子窗口不受影响），按钮可用状态与控件本体显隐/禁用解耦
        if (QPushButton* entry = _discreteControl->entryButton())
        {
            _discreteControl->detachEntryButton(_tableRowWidget);
            tableLayout->addWidget(entry, 1);
            _discreteControl->hide();
        }
        else
        {
            tableLayout->addWidget(_discreteControl);
        }

        // 文件选择行
        _fileRowWidget = new QWidget();
        QHBoxLayout* fileRowLayout = new QHBoxLayout(_fileRowWidget);
        fileRowLayout->setContentsMargins(0, 0, 0, 0);
        fileRowLayout->setSpacing(8);
        _fileLabel = new QLabel("文件");
        _fileLabel->setMinimumWidth(87);
        fileRowLayout->addWidget(_fileLabel);
        _filePathEdit = new QLineEdit();
        _filePathEdit->setReadOnly(true);
        _filePathEdit->setPlaceholderText("请选择文件...");
        fileRowLayout->addWidget(_filePathEdit);
        _fileBrowseBtn = new QPushButton("选择文件");
        connect(_fileBrowseBtn, &QPushButton::clicked, this, &mLoadConvectionPanel::slot_browseFileClicked);
        fileRowLayout->addWidget(_fileBrowseBtn);

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
            // 其 dataChanged 只在统一赋值/文件模式下触发，无需按方式分支重建 map。
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

        // 环境温度曲线
        combofunc.initWidget("环境温度曲线");
        combofunc.labelname->setMinimumWidth(100);
        combofunc.pushbtn->setIcon(QIcon(":/mMainWidgets/Icon/BrowserTree/hanshu.png"));
        combofunc.pushbtn->setFixedSize(30, 30);
        combofunc.pushbtn->setIconSize(QSize(18, 18));
        combofunc.combobox->setFixedHeight(30);
        _functable[""] = -1;
        combofunc.combobox->addItem("");
        auto allFunc = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getAllFunctionID();
        for (auto funcid : allFunc)
        {
            QString funcname = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionNameByID(funcid);
            combofunc.combobox->addItem(funcname);
            _functable[funcname] = funcid;
        }
        connect(combofunc.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mLoadConvectionPanel::slot_funcChanged);
        connect(combofunc.pushbtn, &QPushButton::clicked, this, [this]() {
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createFunction(2);
            combofunc.combobox->clear();
            _functable.clear();
            _functable[""] = -1;
            auto allFunc = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getAllFunctionID();
            for (auto funcid : allFunc)
            {
                QString funcname = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionNameByID(funcid);
                combofunc.combobox->addItem(funcname);
                _functable[funcname] = funcid;
            }
            combofunc.combobox->setCurrentIndex(allFunc.size() > 0 ? allFunc.size() : 0);
            saveFunc();
            });
        // 对流换热曲线
        combofuncConv.initWidget("对流换热曲线");
        combofuncConv.labelname->setMinimumWidth(100);
        combofuncConv.pushbtn->setIcon(QIcon(":/mMainWidgets/Icon/BrowserTree/hanshu.png"));
        combofuncConv.pushbtn->setFixedSize(30, 30);
        combofuncConv.pushbtn->setIconSize(QSize(18, 18));
        combofuncConv.combobox->setFixedHeight(30);
        _functableConv[""] = -1;
        combofuncConv.combobox->addItem("");
        for (auto funcid : allFunc)
        {
            QString funcname = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionNameByID(funcid);
            combofuncConv.combobox->addItem(funcname);
            _functableConv[funcname] = funcid;
        }
        connect(combofuncConv.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mLoadConvectionPanel::slot_funcChanged);
        connect(combofuncConv.pushbtn, &QPushButton::clicked, this, [this]() {
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createFunction(2);
            combofuncConv.combobox->clear();
            _functableConv.clear();
            _functableConv[""] = -1;
            auto allFunc = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getAllFunctionID();
            for (auto funcid : allFunc)
            {
                QString funcname = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionNameByID(funcid);
                combofuncConv.combobox->addItem(funcname);
                _functableConv[funcname] = funcid;
            }
            combofuncConv.combobox->setCurrentIndex(allFunc.size() > 0 ? allFunc.size() : 0);
            saveFuncConv();
            });

        // 缩放因子
        _scale.initWidget("缩放因子", false);
        _scale.labelname->setText("缩放因子");
        connect(_scale.lineeditvalue, &mCalcLineEdit::editingFinished,
            [this]() {
                saveScale();
            });
    }

    void mLoadConvectionPanel::saveConvectionValue()
    {
        if (!_currentLoad)
            return;
        QString strV1 = inputWidgets[0].lineeditvalue->text().trimmed();
        QString strV2 = inputWidgets[1].lineeditvalue->text().trimmed();
        _currentLoad->V1 = strV1.isEmpty() ? QVariant() : QVariant::fromValue<double>(strV1.toDouble());
        _currentLoad->V2 = strV2.isEmpty() ? QVariant() : QVariant::fromValue<double>(strV2.toDouble());
        notifyLoadChanged();
    }

    void mLoadConvectionPanel::saveFunc()
    {
        if (!_currentLoad)
            return;
        int funcid = getFunc();
        auto& mapload = getCVM->getModelData()->getMapRelationManager();
        mapload.appendMapRelation({ Physical_LoadBoundary, _loadData->getLoadBoundaryId() }, { Physical_Function, funcid });
        notifyLoadChanged();
    }

    void mLoadConvectionPanel::saveFuncConv()
    {
        if (!_currentLoad)
            return;
        int funcid = getFuncConv();
        auto& mapload = getCVM->getModelData()->getMapRelationManager();
        mapload.appendMapRelation({ Physical_LoadBoundary, _loadData->getLoadBoundaryId() }, { Physical_Function, funcid }, MapRelationType::MapRelation_convFunction);
        notifyLoadChanged();
    }

    void mLoadConvectionPanel::saveAnalysisField()
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

    void mLoadConvectionPanel::saveConvAnalysisField()
    {
        if (!_currentLoad)
            return;
        int fieldid = -1;
        QString fieldname = _convAnalysisFieldCombo.combobox->currentText();
        if (_convAnalysisFieldTable.find(fieldname) != _convAnalysisFieldTable.end())
        {
            fieldid = _convAnalysisFieldTable[fieldname];
        }
        auto& mapload = getCVM->getModelData()->getMapRelationManager();
        mapload.appendMapRelation({ Physical_LoadBoundary, _loadData->getLoadBoundaryId() }, { Physical_Field, fieldid }, MapRelationType::MapRelation_convAnalysisField);
        notifyLoadChanged();
    }

    void mLoadConvectionPanel::saveScale()
    {
        if (!_currentLoad)
            return;
        double scale = _scale.lineeditvalue->text().trimmed().toDouble();
        auto& mapload = getCVM->getModelData()->getMapRelationManager();
        mapload.setScaleByCase({ Physical_LoadBoundary, _loadData->getLoadBoundaryId() }, _caseId, scale);
        notifyLoadChanged();
    }

    void mLoadConvectionPanel::notifyLoadChanged()
    {
        if (!_loadData)
            return;
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_LoadBoundary, _loadData->getLoadBoundaryId());
    }

    void mLoadConvectionPanel::slot_defineTypeChanged(int idx)
    {
        if (_currentLoad)
            _currentLoad->defineType_ = idx;

        // 切换定义方式时关闭离散数据弹窗并结束拾取，避免旧模式弹窗残留
        if (_discreteControl)
        {
            _discreteControl->closePopup();
        }

        applyAllowedPickTypes();

        if (idx == 1)
        {
            // 独立赋值：过滤拾取为仅单元面，隐藏通用拾取控件（拾取由离散数据控件直驱视口）
            filterPicksToCellFaceOnly();
            if (_pickWidgetController)
            {
                _pickWidgetController->setPickWidgetVisible(false);
            }
        }
        else if (idx == 3)
        {
            // 分析场：需要作用对象拾取（mx文件有set关键字），恢复通用拾取控件正常显示
            if (_pickWidgetController)
            {
                _pickWidgetController->setPickWidgetVisible(true);
                _pickWidgetController->setSelectSetBtnVisible(true);
                _pickWidgetController->setCloudChartMode(false);
                _pickWidgetController->setPickActive(true);
            }
        }
        else
        {
            // 其他模式：恢复通用拾取控件可见/激活与正常高亮渲染
            if (_pickWidgetController)
            {
                _pickWidgetController->setPickWidgetVisible(true);
                _pickWidgetController->setCloudChartMode(false);
                _pickWidgetController->setPickActive(true);
            }
        }

        rebuildValueLayout();
        updateLoadSymbolRealtime();
        // 定义方式切换导致作用对象折叠框显隐变化，需重建面板
        emit sg_rebuildPanel();
    }

    void mLoadConvectionPanel::rebuildValueLayout()
    {
        if (!faval_)
            return;

        faval_->removeAll();
        faval_->addRow(_defineTypeCombo.propwid);
        _defineTypeCombo.propwid->show();

        int idx = _defineTypeCombo.combobox->currentIndex();
        if (idx == 0)
        {
            faval_->addRow(inputWidgets[0].propwid);
            inputWidgets[0].propwid->show();
            faval_->addRow(inputWidgets[1].propwid);
            inputWidgets[1].propwid->show();
            faval_->addRow(combofunc.propwid);
            combofunc.propwid->show();
            faval_->addRow(combofuncConv.propwid);
            combofuncConv.propwid->show();
        }
        else if (idx == 1)
        {
            faval_->addRow(_tableRowWidget);
            _tableRowWidget->show();
            faval_->addRow(combofunc.propwid);
            combofunc.propwid->show();
            faval_->addRow(combofuncConv.propwid);
            combofuncConv.propwid->show();
        }
        else if (idx == 2)
        {
            faval_->addRow(_fileRowWidget);
            _fileRowWidget->show();
            faval_->addRow(inputWidgets[0].propwid);
            inputWidgets[0].propwid->show();
            faval_->addRow(combofunc.propwid);
            combofunc.propwid->show();
            faval_->addRow(combofuncConv.propwid);
            combofuncConv.propwid->show();
        }
        else // 分析场：分析场 + 对流换热分析场 + 函数
        {
            faval_->addRow(_analysisFieldCombo.propwid);
            _analysisFieldCombo.propwid->show();
            faval_->addRow(_convAnalysisFieldCombo.propwid);
            _convAnalysisFieldCombo.propwid->show();
            faval_->addRow(combofunc.propwid);
            combofunc.propwid->show();
            faval_->addRow(combofuncConv.propwid);
            combofuncConv.propwid->show();
        }
        faval_->refreshLayout();
    }

    void mLoadConvectionPanel::slot_browseFileClicked()
    {
        QString defaultDir = MAppConfiguration::mAppConfiguration::getInstance()->_workingDir;
        QFileInfo info(defaultDir);
        if (defaultDir.isEmpty() || !info.exists())
        {
            defaultDir = QDir(qApp->applicationDirPath() + "/../WorkingDir").absolutePath();
        }
        QString filePath = QFileDialog::getOpenFileName(
            nullptr, "选择对流换热文件", defaultDir, "Text Files (*.txt);;All Files (*)");
        if (!filePath.isEmpty())
        {
            QFileInfo fi(filePath);
            QString fileName = fi.fileName();
            _filePathEdit->setText(fileName);
            _filePathEdit->setToolTip(filePath);
            if (_currentLoad)
            {
                _currentLoad->path_ = filePath;
            }
        }
    }

    void mLoadConvectionPanel::slot_discreteDataChanged()
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

    void mLoadConvectionPanel::slot_discreteCellEdited(const QPair<int, int>& key, int valueIndex, const QString& text)
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
        QVector<QVariant>& values = _currentLoad->cellFaceData_[loadKey];
        if (values.size() < 2)
            values.resize(2);
        if (valueIndex >= 0 && valueIndex < 2)
            values[valueIndex] = value;

        updateLoadSymbolRealtime();
        notifyLoadChanged();
    }

    void mLoadConvectionPanel::setInputsValue(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData)
    {
        _currentLoad = std::dynamic_pointer_cast<MechCaseData::mLoadConvection>(loadData);
        if (!_currentLoad)
            return;

        // 单例面板复用：无论新载荷定义方式如何，先清空离散表格残留行，
        // 避免旧载荷独立赋值数据在用户切回独立赋值时复现并污染新载荷。
        // setRows 不发 dataChanged，不会反向污染新载荷，内部还会清掉视口残留选中态。
        if (_discreteControl)
        {
            _discreteControl->setRows({});
        }

        if (_pickWidgetController)
        {
            auto strategy = std::make_shared<MapRelationPersistenceStrategy>(
                MxFunctions::Physical_LoadBoundary,
                _loadData->getLoadBoundaryId());
            _pickWidgetController->setPersistenceStrategy(strategy);
            _pickWidgetController->setSetResolver(&resolveSetToTypedData);
            _pickWidgetController->setSelectSetList(getSetNamesList({ SetType::GeoFace, SetType::GeoSolid, SetType::ElementSurface }));

            applyAllowedPickTypes();
        }

        inputWidgets[0].lineeditvalue->clear();
        inputWidgets[1].lineeditvalue->clear();
        if (_currentLoad->V1.isValid() && !_currentLoad->V1.isNull())
            inputWidgets[0].lineeditvalue->setExpression(QString::number(_currentLoad->V1.toDouble()));
        if (_currentLoad->V2.isValid() && !_currentLoad->V2.isNull())
            inputWidgets[1].lineeditvalue->setExpression(QString::number(_currentLoad->V2.toDouble()));

        _defineTypeCombo.combobox->blockSignals(true);
        _defineTypeCombo.combobox->setCurrentIndex(_currentLoad->defineType_);
        _defineTypeCombo.combobox->blockSignals(false);

        auto& mapload = getCVM->getModelData()->getMapRelationManager();

        combofunc.combobox->blockSignals(true);
        combofuncConv.combobox->blockSignals(true);

        combofunc.combobox->clear();
        combofuncConv.combobox->clear();
        _functable.clear();
        _functableConv.clear();
        _functable[""] = -1;
        _functableConv[""] = -1;
        combofunc.combobox->addItem("");
        combofuncConv.combobox->addItem("");
        auto allFunc = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getAllFunctionID();
        for (auto funcid : allFunc)
        {
            QString funcname = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionNameByID(funcid);
            combofunc.combobox->addItem(funcname);
            _functable[funcname] = funcid;
            combofuncConv.combobox->addItem(funcname);
            _functableConv[funcname] = funcid;
        }

        int funcid = mapload.getMapRelation<int>({ Physical_LoadBoundary, loadData->getLoadBoundaryId() }, Physical_Function);
        if (funcid != -1)
        {
            QString funcname = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionNameByID(funcid);
            combofunc.combobox->setCurrentText(funcname);
        }
        int funcidConv = mapload.getMapRelation<int>({ Physical_LoadBoundary, loadData->getLoadBoundaryId() }, Physical_Function, MapRelationType::MapRelation_convFunction);
        if (funcidConv != -1)
        {
            QString funcname = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionNameByID(funcidConv);
            combofuncConv.combobox->setCurrentText(funcname);
        }

        combofunc.combobox->blockSignals(false);
        combofuncConv.combobox->blockSignals(false);

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

        // 对流换热分析场下拉刷新与反显
        _convAnalysisFieldCombo.combobox->blockSignals(true);
        _convAnalysisFieldCombo.combobox->clear();
        _convAnalysisFieldTable.clear();
        _convAnalysisFieldCombo.combobox->addItem("");
        _convAnalysisFieldTable[""] = -1;
        for (auto fieldid : allAnalysisField)
        {
            QString fieldname = analysisFieldMgr.getFieldNameByID(fieldid);
            _convAnalysisFieldCombo.combobox->addItem(fieldname);
            _convAnalysisFieldTable[fieldname] = fieldid;
        }
        int convFieldId = mapload.getMapRelation<int>({ Physical_LoadBoundary, loadData->getLoadBoundaryId() }, Physical_Field, MapRelationType::MapRelation_convAnalysisField);
        if (convFieldId != -1)
        {
            QString fieldname = analysisFieldMgr.getFieldNameByID(convFieldId);
            _convAnalysisFieldCombo.combobox->setCurrentText(fieldname);
        }
        _convAnalysisFieldCombo.combobox->blockSignals(false);

        // 通用拾取控件反显（syncPicksToController 内部 displayData 默认不重画高亮）
        const bool wasPickActive = _pickWidgetController && _pickWidgetController->isPickActive();
        if (_currentLoad->defineType_ == 1)
        {
            filterPicksToCellFaceOnly();

            // 独立赋值：反显到离散数据控件（单元面 + 已填环境温度/对流换热系数），
            // 行 key 为业务 key（单元ID, 面序号）
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

            // 独立赋值已剥离通用拾取：仅写入数据（供 pickState/持久化一致性），
            // 全程保持失活，不激活、不渲染任何拾取高亮
            if (_pickWidgetController)
            {
                _pickWidgetController->setPickWidgetVisible(false);
                syncPicksToController();
            }
        }
        else if (_currentLoad->defineType_ == 3)
        {
            // 分析场：需要作用对象拾取（mx文件有set关键字），正常显示通用拾取控件
            if (_pickWidgetController)
            {
                _pickWidgetController->setPickWidgetVisible(true);
                _pickWidgetController->setSelectSetBtnVisible(true);
                _pickWidgetController->setCloudChartMode(false);
                _pickWidgetController->setPickActive(true);
                syncPicksToController();
            }
        }
        else
        {
            syncPicksToController();
            if (_pickWidgetController)
            {
                // displayData 默认不重画高亮；若控件此前已处于激活态，setPickActive(true) 会直接返回，
                // 该情况下需显式重画一次高亮以匹配反显数据。整体保证进入面板只做一次全量高亮。
                // 旧载荷在独立赋值/文件模式下曾 setPickWidgetVisible(false)，
                // setPickActive 在隐藏态会被静默拦截，需先恢复可见性
                _pickWidgetController->setPickWidgetVisible(true);
                _pickWidgetController->setCloudChartMode(false);
                _pickWidgetController->setPickActive(true);
                if (wasPickActive)
                {
                    _pickWidgetController->refreshHighlight();
                }
            }
        }

        if (_currentLoad->defineType_ == 2)
        {
            QFileInfo fi(_currentLoad->path_);
            _filePathEdit->setText(fi.fileName());
            _filePathEdit->setToolTip(_currentLoad->path_);
        }
        else
        {
            _filePathEdit->clear();
            _filePathEdit->setToolTip("");
        }

        // 分析场方式隐藏缩放因子。切换载荷进入面板时各行控件已被上一轮
        // clearFoldWidgets 摘为无父（loadPanel 稍后才重挂入折叠框并统一 show），
        // 无父 QWidget 直接 show 会作为顶层窗口短暂闪现，故无父期不做显示
        if (_currentLoad->defineType_ == 3)
        {
            _scale.propwid->hide();
        }
        else if (_scale.propwid->parentWidget())
        {
            _scale.propwid->show();
        }

        double scale = mapload.getScaleByCase({ Physical_LoadBoundary, loadData->getLoadBoundaryId() }, _caseId);
        _scale.lineeditvalue->setExpression(QString::number(scale));

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
        updateLoadSymbolRealtime();
    }

    QVector<QWidget*> mLoadConvectionPanel::getRefStateInputs(FoldSection* fathertar)
    {
        refstate_ = fathertar;
        QVector<QWidget*>wds;
        // 分析场方式无大小概念，不需要缩放因子（折叠框整体隐藏）
        if (_currentLoad && _currentLoad->defineType_ == 3)
            return wds;
        wds.append(_scale.propwid);
        return wds;
    }

    QVector<QWidget*> mLoadConvectionPanel::getTargetInputs(FoldSection* fathertar)
    {
        fatar_ = fathertar;
        QVector<QWidget*>wds;
        // 分析场方式有作用对象（mx文件有set关键字），正常显示拾取控件
        if (_pickWidgetController)
        {
            wds.append(_pickWidgetController->widget());
        }
        return wds;
    }

    QVector<QWidget*> mLoadConvectionPanel::getValueInputs(FoldSection* fatherval)
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
            wds.append(combofuncConv.propwid);
        }
        else if (defineType == 1)
        {
            wds.append(_tableRowWidget);
            wds.append(combofunc.propwid);
            wds.append(combofuncConv.propwid);
        }
        else if (defineType == 2)
        {
            wds.append(_fileRowWidget);
            wds.append(inputWidgets[0].propwid);
            wds.append(combofunc.propwid);
            wds.append(combofuncConv.propwid);
        }
        else // 分析场：分析场 + 对流换热分析场 + 函数
        {
            wds.append(_analysisFieldCombo.propwid);
            wds.append(_convAnalysisFieldCombo.propwid);
            wds.append(combofunc.propwid);
            wds.append(combofuncConv.propwid);
        }
        return wds;
    }
    void mLoadConvectionPanel::clearInputs()
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

        for (auto& inp : inputWidgets) inp.clear();
        inputWidgets.clear();

        for (auto& combo : comboBoxs) combo.clear();
        comboBoxs.clear();
        _defineTypeCombo.clear();
        _analysisFieldCombo.clear();
        _analysisFieldTable.clear();
        _convAnalysisFieldCombo.clear();
        _convAnalysisFieldTable.clear();
        combofunc.clear();
        combofuncConv.clear();
        _functable.clear();
        _functableConv.clear();
        _scale.clear();
        _currentLoad.reset();

        if (_tableRowWidget) { delete _tableRowWidget; _tableRowWidget = nullptr; }
        _tableLabel = nullptr;
        _discreteControl = nullptr; // 随 _tableRowWidget 一并删除

        if (_fileRowWidget) { delete _fileRowWidget; _fileRowWidget = nullptr; }
        _fileLabel = nullptr;
        _filePathEdit = nullptr;
        _fileBrowseBtn = nullptr;
    }

    void mLoadConvectionPanel::slot_funcChanged(int)
    {
        saveFunc();
        saveFuncConv();
    }

    void mLoadConvectionPanel::slot_analysisFieldChanged(int)
    {
        saveAnalysisField();
    }

    void mLoadConvectionPanel::slot_convAnalysisFieldChanged(int)
    {
        saveConvAnalysisField();
    }

    void mLoadConvectionPanel::deactivatePickAndCommit()
    {
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

    void mLoadConvectionPanel::updateLoadSymbolRealtime()
    {
        if (!_pickWidgetController || !_currentLoad)
            return;
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;

        if (!_realtimeUpdateTimer)
            return;
        _realtimeUpdateTimer->start(100);
    }

    void mLoadConvectionPanel::onRealtimeUpdateTimeout()
    {
        doUpdateLoadSymbolRealtime();
    }

    void mLoadConvectionPanel::doUpdateLoadSymbolRealtime()
    {
        if (!_pickWidgetController || !_currentLoad)
            return;
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;

        // 分析场方式有作用对象（mx文件有set关键字），正常渲染对流符号
        // 独立赋值：先按离散数据控件当前行同步 cellFaceData_（数据源为表格，
        // 通用拾取控制器已不再接收视口拾取，不能再以它为准），再渲染符号。
        const bool independent = (_currentLoad->defineType_ == 1);
        if (independent)
        {
            updateOpenedTable();
        }

        // 1. 汇总当前数据：拾取(pickedData) + 集合(selectedSetNames 按名称展开），合并为按类型分组的 map
        const PickWidgetData allData = _pickWidgetController->getAllData();
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> merged;
        if (!independent)
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

        // 独立赋值：已剥离通用拾取，merged 为空；
        // 符号位置直接遍历 cellFaceData_（表格为权威数据源）
        if (independent)
        {
            for (const auto& entry : _currentLoad->cellFaceData_)
            {
                QVector3D centerPos;
                QVector<QVector3D> vertex;
                meshData->GetShrinkVertexByMeshFaceID(entry.first, centerPos, vertex);
                objs.push_back(centerPos);
            }
        }
        else
        {
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
        }

        // 最终数据签名：载荷ID+位置集未变化时跳过符号赋值与 UpdateAllSymbol 全量重建
        //（进入面板防抖触发/切出冲刷时数据往往与上次相同）
        size_t sig = AllRenderSymbolManager::beginRenderSignature();
        AllRenderSymbolManager::combineRenderSignature(sig, static_cast<size_t>(_currentLoad->getLoadBoundaryId()));
        AllRenderSymbolManager::combineRenderSignature(sig, objs);
        if (sig == _lastRenderSig)
        {
            return;
        }
        _lastRenderSig = sig;

        AllRenderSymbolManager::getInstance()->assignThermalLoad(_currentLoad, objs, "Load_Convection", QVector3D(0.0f, 191.0f, 255.0f), 0);

        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        if (symbolController) symbolController->UpdateAllSymbol();
        getCVM->getPreRenderWidget()->updatePaint();
    }

    int mLoadConvectionPanel::getFunc()
    {
        int funcid = -1;
        QString funcname = combofunc.combobox->currentText();
        if (_functable.find(funcname) != _functable.end())
            funcid = _functable[funcname];
        return funcid;
    }

    int mLoadConvectionPanel::getFuncConv()
    {
        int funcid = -1;
        QString funcname = combofuncConv.combobox->currentText();
        if (_functableConv.find(funcname) != _functableConv.end())
            funcid = _functableConv[funcname];
        return funcid;
    }
}
