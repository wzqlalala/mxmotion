#include "mPredefinedFieldTempLoadPanel.h"
#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mSetManager.h"
#include "mMapRelationEnum.h"
#include "mMapRelationManager.h"
#include "mInfoWindow.h"
#include "mDataStatusManager.h"
#include "mGeneralPickUp.h"
#include "mGetSetTypeTemplate.h"
#include "mGetSetFunction.h"
#include "mAppConfiguration.h"

//Geo
#include "mGeoManager.h"
//#include "mGeometricFeatureAttribute.h" //【OCC摘除·S10/P0-1】退役头死 include 清剿（冻结岛保档，同主线处理）
#include "PickCommonFunction.h"

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/view/CCSMPreRender.h"
#include "mxr/data/SelectDataStruct.h"
#include "mxr/label/mSymbolController.h"
#include "mxr/view/mShapeRender.h"

//Basic
#include<mMixSet.h>
#include<mMeshData.h>
#include<mGeoSet.h>
#include<mNode.h>

#include "mPredefinedFieldTempLoad.h"
#include "AllRenderSymbolManager.h"
#include "mOtherFunction.h"

using namespace MechCommon;
using namespace MechPreData;
namespace MechMainWindow
{
    mPredefinedFieldTempLoadPanel::mPredefinedFieldTempLoadPanel(QWidget* parent)
        :mPredefinedFieldPanelBase(parent)
    {
        _realtimeUpdateTimer = new QTimer(this);
        _realtimeUpdateTimer->setSingleShot(true);
        connect(_realtimeUpdateTimer, &QTimer::timeout, this, &mPredefinedFieldTempLoadPanel::onRealtimeUpdateTimeout);

        initInputs();
    }

    mPredefinedFieldTempLoadPanel::~mPredefinedFieldTempLoadPanel()
    {
        clearInputs();
    }

    std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> mPredefinedFieldTempLoadPanel::getPickedData()
    {
        if (!_pickWidgetController)
            return std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>();
        return _pickWidgetController->getAllData().pickedData;
    }

    void mPredefinedFieldTempLoadPanel::updateOpenedTable()
    {
        if (!_currentField || !_discreteControl || _isSettingValue)
        {
            return;
        }

        // 独立赋值模式下，_nodeTempMap 与离散数据控件的行数据保持严格一致：
        // 表格是数据的权威来源（含拾取追加与手动删除），此处按控件当前行重建 map。
        // 键规范为 (nodeId, -1, -1)，与 mIMxFile 读入键规范逐字段对齐；
        // 已填温度值按节点 ID 保留（包括空值），跳过 key 无效（id<=0）的行。
        const auto rows = _discreteControl->rows();

        std::map<int, QVector<QVariant>> existingValues;
        for (const auto& entry : _currentField->_nodeTempMap)
        {
            const int nodeId = entry.first.result_id;
            if (existingValues.find(nodeId) == existingValues.end())
            {
                existingValues[nodeId] = entry.second;
            }
        }

        std::map<RenderPickResultData, QVector<QVariant>> newMap;
        for (const auto& row : rows)
        {
            const int nodeId = row.first.first;
            if (nodeId <= 0)
                continue;
            auto it = existingValues.find(nodeId);
            newMap[RenderPickResultData(nodeId, -1, -1)] =
                (it != existingValues.end()) ? it->second : QVector<QVariant>{ QVariant() };
        }

        _currentField->_nodeTempMap = std::move(newMap);
    }

    void mPredefinedFieldTempLoadPanel::filterPicksToNodeOnly()
    {
        if (!_pickWidgetController)
        {
            return;
        }

        // 同步通用拾取控件最新数据到 picks，避免过滤时丢失拾取/集合
        auto picks = _pickWidgetController->getAllData().pickedData;

        auto it = picks.begin();
        while (it != picks.end())
        {
            if (it->first != MxFunctions::PickDataType::Node)
            {
                it = picks.erase(it);
            }
            else
            {
                ++it;
            }
        }

        // 独立赋值/文件模式无需集合：把控件数据重设为"仅节点拾取、无集合"。
        _pickWidgetController->replacePickedDataAndClearSets(picks);
        // 独立赋值已完全剥离通用拾取：清空并抑制本控件的一切拾取高亮渲染，
        // 拾取由离散数据控件直接驱动视口选择容器，视口中不再出现蓝色高亮。
        _pickWidgetController->setCloudChartMode(true);

        updateOpenedTable();
    }

    void mPredefinedFieldTempLoadPanel::initInputs()
    {
        // 定义方式下拉框
        _defineTypeCombo.initWidget("定义方式");
        _defineTypeCombo.combobox->addItem("统一赋值");
        _defineTypeCombo.combobox->addItem("独立赋值");
        _defineTypeCombo.combobox->addItem("文件定义");
        connect(_defineTypeCombo.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mPredefinedFieldTempLoadPanel::slot_defineTypeChanged);

        // 统一赋值输入
        initValueInputs();

        // 独立赋值输入（表格行）：通用离散数据控件（入口按钮 + 锚定弹窗表格）
        _tableRowWidget = new QWidget();
        QHBoxLayout* tableRowLayout = new QHBoxLayout(_tableRowWidget);
        tableRowLayout->setContentsMargins(0, 0, 0, 0);
        tableRowLayout->setSpacing(8);

        _tableLabel = new QLabel("节点数据");
        _tableLabel->setMinimumWidth(87);
        tableRowLayout->addWidget(_tableLabel);

        mDiscreteDataControl::PickSpec nodeSpec;
        nodeSpec.typeName = QStringLiteral("节点");
        nodeSpec.idHeaders = QStringList{ QStringLiteral("节点编号") };
        nodeSpec.keyOf = [](const RenderPickResultData& d) { return qMakePair(d.result_id, -1); };
        nodeSpec.pickTypes = { MxFunctions::PickDataType::Node };
        nodeSpec.pickFilter = 5; // 视口拾取过滤器：仅节点（与 activePickSig 索引一致）
        // 拾取由离散数据控件直接驱动视口选择容器，不再经过通用拾取控件
        _discreteControl = new mDiscreteDataControl(
            QStringLiteral("节点数据"),
            { QStringLiteral("温度值") },
            &nodeSpec, _tableRowWidget);
        // 温度载荷需要渲染云图功能：显示渲染按钮，点击切换云图开关（开关仅保留在弹窗内，不再单独占行）
        _discreteControl->setRenderButtonVisible(true, QStringLiteral("云图"));
        _discreteControl->setRenderHandler([this]() {
            _cloudOn = !_cloudOn;
            if (_cloudOn)
            {
                // 打开云图前先清空视口残留的拾取选中态/高亮（含通用拾取控件写入
                // 选择容器的数据），否则首次点击云图时旧拾取高亮会遮挡云图
                _discreteControl->clearPickingSelection();
            }
            setNodeTempCloudChart(_cloudOn);
            if (!_cloudOn)
            {
                // 关闭云图后恢复离散表格的拾取高亮（开启云图时已被清空）：
                // 云图按钮在弹窗内，此刻弹窗必然打开，回种即恢复"表格所见即高亮"
                _discreteControl->seedSelectionFromRows();
            }
            });
        connect(_discreteControl, &mDiscreteDataControl::dataChanged,
            this, &mPredefinedFieldTempLoadPanel::slot_discreteDataChanged);
        connect(_discreteControl, &mDiscreteDataControl::cellEdited,
            this, &mPredefinedFieldTempLoadPanel::slot_discreteCellEdited);
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

        // 文件定义输入
        initFileInputs();

        // 通用拾取控件（作用对象）
        _pickWidgetController = mGeneralPickUp::createPickWidget(CBBlue);
        connect(_pickWidgetController, &mPickWidgetController::pickActiveChanged, this, [this](bool isActive) {
            if (isActive)
            {
                emit mMechGlobalSignals::getInstance()->activePickSig(5);
            }
            });
        connect(_pickWidgetController, &mPickWidgetController::dataChanged, this, [this]() {
            // 独立赋值模式下通用拾取控制器不再接收拾取（由离散数据控件直驱视口），
            // 其 dataChanged 只在统一赋值模式下触发，无需按方式分支处理。
            updateLoadSymbolRealtime();
            });

        _pickWidgetController->setSetResolver(&resolveSetToTypedData);
        _pickWidgetController->setDefultTip("选择对象");

        //缩放因子
        _scale.initWidget("缩放因子", false);
        _scale.labelname->setText("缩放因子");
        connect(_scale.lineeditvalue, &mCalcLineEdit::editingFinished,
            [this]() {
                saveScale();
            });
    }

    void mPredefinedFieldTempLoadPanel::initValueInputs()
    {
        _inp.initWidget("温度", false);
        connect(_inp.lineeditvalue, &mCalcLineEdit::editingFinished,
            [this]() {
                updateLoadSymbolRealtime();
                saveTemp();
            });

        // 函数->幅值曲线（非必选，加空行）
        combofunc.initWidget("幅值曲线");
        combofunc.pushbtn->setIcon(QIcon(":/mMainWidgets/Icon/BrowserTree/hanshu.png"));
        auto allFunc = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getAllFunctionID();
        combofunc.combobox->addItem("");
        _functable[""] = -1;
        for (auto funcid : allFunc)
        {
            QString funcname = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionNameByID(funcid);
            combofunc.combobox->addItem(funcname);
            _functable[funcname] = funcid;
        }
        connect(combofunc.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mPredefinedFieldTempLoadPanel::slot_funcChanged);
        connect(combofunc.pushbtn, &QPushButton::clicked, this, [this]() {
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createFunction(2);

            combofunc.combobox->blockSignals(true);
            combofunc.combobox->clear();
            _functable.clear();
            auto allFunc = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getAllFunctionID();
            combofunc.combobox->addItem("");
            _functable[""] = -1;
            for (auto funcid : allFunc)
            {
                QString funcname = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionNameByID(funcid);
                combofunc.combobox->addItem(funcname);
                _functable[funcname] = funcid;
            }
            combofunc.combobox->setCurrentIndex(allFunc.size() > 0 ? allFunc.size() : 0);
            combofunc.combobox->blockSignals(false);
            saveFunc();
            });
    }

    void mPredefinedFieldTempLoadPanel::initFileInputs()
    {
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
        connect(_fileBrowseBtn, &QPushButton::clicked, this, &mPredefinedFieldTempLoadPanel::slot_browseFile);
        fileRowLayout->addWidget(_fileBrowseBtn);

        // 保留旧控件，但实际文件模式使用 _fileRowWidget
        _filePathInput.initWidget("文件", false);
        _filePathInput.lineeditvalue->setReadOnly(true);
        _filePathInput.lineeditvalue->setFocusPolicy(Qt::NoFocus);

        _interpolateTypeCombo.initWidget("类型");
        _interpolateTypeCombo.combobox->addItem("单步插值");
        _interpolateTypeCombo.combobox->addItem("多步插值");
        connect(_interpolateTypeCombo.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mPredefinedFieldTempLoadPanel::slot_interpolateTypeChanged);

        _midNodeCheck = new QCheckBox("中间节点插值");
        connect(_midNodeCheck, &QCheckBox::stateChanged,
            this, &mPredefinedFieldTempLoadPanel::slot_midNodeInterpolateChanged);

        // 文件定义下的函数->幅值曲线（非必选，加空行）
        combofuncFile.initWidget("幅值曲线");
        combofuncFile.pushbtn->setIcon(QIcon(":/mMainWidgets/Icon/BrowserTree/hanshu.png"));
        auto allFunc = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getAllFunctionID();
        combofuncFile.combobox->addItem("");
        for (auto funcid : allFunc)
        {
            QString funcname = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionNameByID(funcid);
            combofuncFile.combobox->addItem(funcname);
        }
        connect(combofuncFile.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mPredefinedFieldTempLoadPanel::slot_funcChanged);
        connect(combofuncFile.pushbtn, &QPushButton::clicked, this, [this]() {
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createFunction(2);

            combofuncFile.combobox->blockSignals(true);
            combofuncFile.combobox->clear();
            auto allFunc = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getAllFunctionID();
            combofuncFile.combobox->addItem("");
            for (auto funcid : allFunc)
            {
                QString funcname = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionNameByID(funcid);
                combofuncFile.combobox->addItem(funcname);
            }
            combofuncFile.combobox->setCurrentIndex(allFunc.size() > 0 ? allFunc.size() : 0);
            combofuncFile.combobox->blockSignals(false);
            saveFunc();
            });
    }

    void mPredefinedFieldTempLoadPanel::slot_defineTypeChanged(int idx)
    {
        int oldDefineType = _currentField ? _currentField->_defineType : 0;

        // 切换定义方式时关闭离散数据弹窗并结束拾取，避免旧模式弹窗残留
        if (_discreteControl)
        {
            _discreteControl->closePopup();
        }

        saveWayAndClearModeFields();

        // 同步拾取类型白名单
        applyAllowedPickTypes(idx);

        // 清空布局并重新添加控件
        if (faval_)
        {
            faval_->removeAll();
            faval_->addRow(_defineTypeCombo.propwid);
            _defineTypeCombo.propwid->show();

            if (idx == 0) // 统一赋值
            {
                faval_->addRow(_inp.propwid);
                _inp.propwid->show();
                faval_->addRow(combofunc.propwid);
                combofunc.propwid->show();
            }
            else if (idx == 1) // 独立赋值
            {
                faval_->addRow(_tableRowWidget);
                _tableRowWidget->show();
                faval_->addRow(combofunc.propwid);
                combofunc.propwid->show();
            }
            else // 文件定义
            {
                saveFilePath();
                faval_->addRow(_fileRowWidget);
                _fileRowWidget->show();
                faval_->addRow(_interpolateTypeCombo.propwid);
                _interpolateTypeCombo.propwid->show();

                QWidget* checkWidget = new QWidget();
                QHBoxLayout* checkLayout = new QHBoxLayout(checkWidget);
                checkLayout->setContentsMargins(0, 0, 0, 0);
                checkLayout->addWidget(_midNodeCheck);
                checkLayout->addStretch();
                faval_->addRow(checkWidget);
                checkWidget->show();

                faval_->addRow(combofuncFile.propwid);
                combofuncFile.propwid->show();
            }

            faval_->refreshLayout();
        }

        // 离开独立赋值模式时，若云图仍开启则关闭
        if (idx != 1 && _cloudOn)
        {
            _cloudOn = false;
            setNodeTempCloudChart(false);
        }

        // 文件模式：完全隐藏拾取控件
        // 独立赋值模式：拾取由离散数据控件直接驱动，通用拾取控件不再激活
        if (_pickWidgetController)
        {
            if (idx == 2 || idx == 1)
            {
                _pickWidgetController->setPickWidgetVisible(false);
            }
            else
            {
                _pickWidgetController->setPickWidgetVisible(true);
                _pickWidgetController->setPickActive(true);
            }
        }

        // 独立赋值：隐藏集合选择按钮，强制仅节点（filterPicksToNodeOnly 内同时
        // 开启云图抑制模式，彻底关闭通用拾取控件的高亮渲染，拾取高亮完全剥离）
        if (idx == 1)
        {
            filterPicksToNodeOnly();
            if (_pickWidgetController)
            {
                _pickWidgetController->setSelectSetBtnVisible(false);
            }
        }
        else if (idx == 2)
        {
            // 文件模式：隐藏集合选择按钮，恢复正常高亮渲染
            if (_pickWidgetController)
            {
                _pickWidgetController->setSelectSetBtnVisible(false);
                _pickWidgetController->setCloudChartMode(false);
            }
        }
        else // 统一赋值：显示集合选择按钮，恢复正常高亮渲染
        {
            if (_pickWidgetController)
            {
                _pickWidgetController->setSelectSetBtnVisible(true);
                _pickWidgetController->setCloudChartMode(false);
            }
            updateLoadSymbolRealtime();
        }

        // 定义方式切换改变作用对象折叠框显隐（仅统一赋值 0 显示），折叠框数量变化需重建面板
        bool oldHasTarget = (oldDefineType == 0);
        bool newHasTarget = (idx == 0);
        if (oldHasTarget != newHasTarget)
        {
            emit sg_rebuildPanel();
        }
    }

    void mPredefinedFieldTempLoadPanel::slot_discreteDataChanged()
    {
        // 离散数据控件行变化（拾取追加/删除/清空/分量增减）：
        // 按控件当前行重建 _nodeTempMap，随后刷新实时符号并通知外部。
        // 拾取会话进行中跳过：确认拾取（完成拾取）后控件会补发一次 dataChanged，
        // 此时才执行重建/渲染/云图刷新，避免拾取界面期间反复重绘。
        if (!_currentField || (_discreteControl && _discreteControl->isPicking()))
        {
            return;
        }
        updateOpenedTable();
        updateLoadSymbolRealtime();
        // 云图处于开启状态时，表格数据变化后同步刷新云图
        if (_cloudOn)
        {
            setNodeTempCloudChart(true);
        }
        notifyLoadChanged();
    }

    void mPredefinedFieldTempLoadPanel::slot_discreteCellEdited(const QPair<int, int>& key, int valueIndex, const QString& text)
    {
        if (!_currentField || key.first <= 0)
        {
            return;
        }
        // 单点写回：仅更新/清除该节点的温度值，节点本身始终保留（避免拾取节点丢失）。
        // 温度载荷固定 1 个分量（温度值），key 规范为 result_id/-1/-1，与 mIMxFile 读入键规范一致。
        const RenderPickResultData fieldKey(key.first, -1, -1);
        QVector<QVariant>& values = _currentField->_nodeTempMap[fieldKey];
        if (values.isEmpty())
        {
            values.resize(1);
        }
        if (valueIndex == 0)
        {
            bool ok = false;
            const double v = text.toDouble(&ok);
            values[0] = (ok && !text.isEmpty()) ? QVariant(v) : QVariant();
        }
        updateLoadSymbolRealtime();
        // 云图处于开启状态时，表格数值变化后同步刷新云图
        if (_cloudOn)
        {
            setNodeTempCloudChart(true);
        }
        notifyLoadChanged();
    }

    void mPredefinedFieldTempLoadPanel::setNodeTempCloudChart(bool flag)
    {
        if (getCVM->getPreRenderWidget() && getCVM->getPreRenderWidget()->getPreRender())
        {
            // _nodeTempMap_: key=节点拾取数据 value=温度值（QVector<QVariant>，温度值在第0列）。
            // 渲染侧节点通道按 {partID, 节点连续ID} 索引：一个节点可能属于多个部件，需逐部件传入
            // 关闭云图时按接口约定传空数据（渲染侧 flag=false 时会清空全部载荷云图数据）
            std::vector<std::pair<double, std::pair<int, int>>> meshNodeIDs;
            auto meshData = getCVM->getModelData()->getMeshData();
            if (flag && _currentField)
            {
                meshNodeIDs.reserve(_currentField->_nodeTempMap.size());
                for (const auto& entry : _currentField->_nodeTempMap)
                {
                    const QVariant v = entry.second.value(0);
                    if (!v.isValid() || v.isNull())
                        continue;
                    const int realNodeId = entry.first.result_id;
                    const int contNodeId = meshData->_nodedata.nodeidmap.value(realNodeId, -1);
                    if (contNodeId < 0)
                        continue;
                    for (int partId : meshData->getPartIdsByNodeid(realNodeId))
                    {
                        meshNodeIDs.emplace_back(v.toDouble(), std::make_pair(partId, contNodeId));
                    }
                }
            }
            getCVM->getPreRenderWidget()->getPreRender()->enableMeshEleLoadCloudChart(flag, {}, meshNodeIDs);
            // 节点拾取高亮为大尺寸点渲染，密集拾取时会整片遮挡云图：
            // 统一赋值下进入/退出云图状态时切换拾取控件云图模式（清空并抑制拾取高亮，数据保留 / 恢复高亮）。
            // 独立赋值下通用拾取控件常驻抑制态（不渲染任何高亮），云图开关不改变其模式——
            // 否则退出云图时会按控件模型里反显的离散节点画失活临时高亮（蓝色残留）
            if (_pickWidgetController)
            {
                const bool discreteMode = (_currentField && _currentField->_defineType == 1);
                _pickWidgetController->setCloudChartMode(discreteMode ? true : flag);
            }
            // 显示云图时隐藏该载荷的符号渲染，关闭云图时恢复符号
            if (_currentField)
            {
                auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
                if (symbolController)
                {
                    std::vector<QString> symbolNames = { QString("predefinedfield_%1").arg(_currentField->getPredefinedFieldId()) };
                    symbolController->setSymbolShowState(symbolNames, !flag);
                    symbolController->UpdateAllSymbol();
                }
            }
            getCVM->getPreRenderWidget()->updatePaint();
        }
    }

    void mPredefinedFieldTempLoadPanel::slot_funcChanged(int idx)
    {
        Q_UNUSED(idx)
        saveFunc();
    }

    void mPredefinedFieldTempLoadPanel::slot_interpolateTypeChanged(int idx)
    {
        Q_UNUSED(idx)
        saveInterpolateInfo();
    }

    void mPredefinedFieldTempLoadPanel::slot_browseFile()
    {
        QString defaultDir = MAppConfiguration::mAppConfiguration::getInstance()->_workingDir;
        QFileInfo info(defaultDir);
        if (defaultDir.isEmpty() || !info.exists())
        {
            defaultDir = QDir(qApp->applicationDirPath() + "/../WorkingDir").absolutePath();
        }
        QString filePath = QFileDialog::getOpenFileName(
            nullptr, "选择文件", defaultDir, "Text Files (*.txt);;All Files (*)");
        if (!filePath.isEmpty())
        {
            QFileInfo fi(filePath);
            QString fileName = fi.fileName();
            _filePathEdit->setText(fileName);
            _filePathEdit->setToolTip(filePath);
            saveFilePath();
        }
    }

    void mPredefinedFieldTempLoadPanel::slot_midNodeInterpolateChanged(int state)
    {
        Q_UNUSED(state)
        saveInterpolateInfo();
    }

    void mPredefinedFieldTempLoadPanel::saveTemp()
    {
        if (!_currentField)
        {
            return;
        }
        _currentField->_temp = _inp.lineeditvalue->text().trimmed();
        notifyLoadChanged();
    }

    void mPredefinedFieldTempLoadPanel::saveWayAndClearModeFields()
    {
        if (!_currentField)
        {
            return;
        }
        _currentField->_defineType = _defineTypeCombo.combobox->currentIndex();
        // 重置所有模式字段，避免交叉污染
        _currentField->_temp = "";
        _currentField->_filePath = "";
        _currentField->_interpolateType = 0;
        _currentField->_midNodeInterpolate = false;
        _currentField->_nodeTempMap.clear();
    }

    void mPredefinedFieldTempLoadPanel::saveFilePath()
    {
        if (!_currentField)
        {
            return;
        }
        _currentField->_filePath = _filePathEdit->toolTip();
        notifyLoadChanged();
    }

    void mPredefinedFieldTempLoadPanel::saveInterpolateInfo()
    {
        if (!_currentField)
        {
            return;
        }
        _currentField->_interpolateType = _interpolateTypeCombo.combobox->currentIndex();
        _currentField->_midNodeInterpolate = _midNodeCheck ? _midNodeCheck->isChecked() : false;
        notifyLoadChanged();
    }

    void mPredefinedFieldTempLoadPanel::saveScale()
    {
        if (!_currentField)
        {
            return;
        }
        double scale = _scale.lineeditvalue->text().trimmed().toDouble();
        auto& map = getCVM->getModelData()->getMapRelationManager();
        map.setScaleByCase({ MxFunctions::PhysicalFieldType::Physical_PredefinedField, _currentField->getPredefinedFieldId() }, _caseId, scale);
        notifyLoadChanged();
    }

    void mPredefinedFieldTempLoadPanel::saveFunc()
    {
        if (!_currentField)
        {
            return;
        }
        if (_currentField->_defineType == 2) // 文件模式无函数
        {
            return;
        }
        int funcid = getFunc();
        auto& mapfield = getCVM->getModelData()->getMapRelationManager();
        mapfield.appendMapRelation({ Physical_PredefinedField, _currentField->getPredefinedFieldId() }, { Physical_Function, funcid });
        notifyLoadChanged();
    }

    void mPredefinedFieldTempLoadPanel::notifyLoadChanged()
    {
        if (!_fieldData)
        {
            return;
        }
        // 仅通知外部刷新树/渲染，不再调用 storeFieldValue 全量保存
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_PredefinedField, _fieldData->getPredefinedFieldId());
    }

    void mPredefinedFieldTempLoadPanel::applyAllowedPickTypes(int way)
    {
        if (!_pickWidgetController) return;
        if (way == 2) // 文件定义：禁用全部拾取
        {
            _pickWidgetController->setAllPickTypesDisabled(true);
        }
        else if (way == 1) // 独立赋值：仅设置白名单记录，不切换视口过滤器
                           // （拾取过滤器由离散数据控件在开始拾取时自行激活/恢复）
        {
            _pickWidgetController->setAllowedTypes(&pickOnlyNode);
        }
        else // 统一赋值(0)：几何+节点全支持
        {
            _pickWidgetController->setAllowedTypes(&pickGeoPointEdgeFaceSolidNodePart);
            _pickWidgetController->setPickType();
        }
    }

    void mPredefinedFieldTempLoadPanel::deactivatePickAndCommit()
    {
        // 面板为单例复用，切换载荷/面板时不会走析构/clearInputs，
        // 由统一退出出口（unEditPredefinedField）调用，失活并提交当前拾取数据。
        // 离散数据控件：结束拾取并关闭弹窗
        if (_discreteControl)
        {
            _discreteControl->closePopup();
        }
        // 退出温度载荷界面时删除云图显示
        if (_cloudOn)
        {
            _cloudOn = false;
            setNodeTempCloudChart(false);
        }
        // 取消挂起的防抖刷新：切出后符号显示状态已由树选中刷新统一重算，
        // 再触发一次符号全量重建（UpdateAllSymbol）纯属浪费，大模型下明显卡顿
        if (_realtimeUpdateTimer)
        {
            _realtimeUpdateTimer->stop();
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
        // 失活提交可能发出 dataChanged 重新启动防抖定时器（最近 100ms 内有拾取变更）：
        // 同步把最终数据写入符号对象（仅赋值不重建），随后切入的面板/树选中刷新
        // 会统一执行重建；切出到预定义场父节点等仍显示本符号的路径数据保持正确
        if (_realtimeUpdateTimer && _realtimeUpdateTimer->isActive())
        {
            _realtimeUpdateTimer->stop();
            doUpdateLoadSymbolRealtime(true);
        }
    }

    void mPredefinedFieldTempLoadPanel::updateLoadSymbolRealtime()
    {
        if (!_pickWidgetController || !_currentField)
            return;
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;

        // 通过防抖定时器合并连续触发（方向/大小/拾取变化），避免每输入一个字符都重算并重绘。
        if (!_realtimeUpdateTimer)
            return;
        _realtimeUpdateTimer->start(100);
    }

    void mPredefinedFieldTempLoadPanel::onRealtimeUpdateTimeout()
    {
        doUpdateLoadSymbolRealtime();
    }

    void mPredefinedFieldTempLoadPanel::doUpdateLoadSymbolRealtime(bool assignOnly)
    {
        if (!_pickWidgetController || !_currentField)
            return;
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;

        const int way = _defineTypeCombo.combobox->currentIndex();

        // 独立赋值：先按离散数据控件当前行同步 _nodeTempMap（数据源为表格，
        // 通用拾取控制器已不再接收视口拾取，不能再以它为准）
        if (way == 1)
        {
            updateOpenedTable();
        }

        // 文件模式不渲染符号
        if (way == 2)
        {
            const bool changed = AllRenderSymbolManager::getInstance()->assignPredefinedField(
                _currentField, QVector<QVector3D>(), "T", QVector3D(255.0f, 165.0f, 0.0f));
            if (changed && !assignOnly)
            {
                auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
                if (symbolController) symbolController->UpdateAllSymbol();
                getCVM->getPreRenderWidget()->updatePaint();
            }
            return;
        }

        // 1. 汇总当前数据：拾取(pickedData) + 集合(selectedSetNames 按名称展开），合并为按类型分组的 map。
        //    独立赋值已剥离通用拾取：直接以 _nodeTempMap（已同步）逐节点取坐标，
        //    不再读取 _pickWidgetController->getAllData()（控制器不再接收视口拾取，数据已停滞）。
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> merged;
        if (way == 1)
        {
            for (const auto& entry : _currentField->_nodeTempMap)
            {
                merged[PickDataType::Node].push_back(entry.first);
            }
        }
        else
        {
            const PickWidgetData allData = _pickWidgetController->getAllData();
            merged = allData.pickedData;

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
                if (auto s = dynamic_cast<mNode*>(setBase)) { auto v = s->getNodeData(); auto& d = merged[PickDataType::Node]; d.insert(d.end(), v.begin(), v.end()); }
                break;
            case MxFunctions::Element:
                if (auto s = dynamic_cast<mElementSet*>(setBase)) { auto v = s->getElementSetData(); auto& d = merged[PickDataType::Element]; d.insert(d.end(), v.begin(), v.end()); }
                break;
            case MxFunctions::ElementSurface:
                if (auto s = dynamic_cast<mElementSurface*>(setBase)) { auto v = s->getElementSurfaceData(); auto& d = merged[PickDataType::ElementFace]; d.insert(d.end(), v.begin(), v.end()); }
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
        }

        // 输入签名短路：fieldId+定义方式+汇总后的作用对象（拾取+集合展开）与
        // 上次渲染完全一致时（进入面板防抖触发、数值类编辑等场景），坐标重算
        // （几何面采样代价很高）与符号赋值全部跳过——符号只随作用对象集合变化
        size_t sig = 14695981039346656037ULL;
        auto fnvCombine = [&sig](size_t v) {
            sig ^= v;
            sig *= 1099511628211ULL;
        };
        fnvCombine(static_cast<size_t>(_currentField->getPredefinedFieldId()));
        fnvCombine(static_cast<size_t>(way));
        for (const auto& pair : merged)
        {
            fnvCombine(static_cast<size_t>(pair.first));
            fnvCombine(pair.second.size());
            for (const auto& d : pair.second)
            {
                fnvCombine(static_cast<size_t>(d.result_id));
                fnvCombine(static_cast<size_t>(d.result_id2));
                fnvCombine(static_cast<size_t>(d.part_id));
            }
        }
        if (_currentField->getPredefinedFieldId() == _lastRenderFieldId && sig == _lastRenderSignature)
        {
            return;
        }
        _lastRenderFieldId = _currentField->getPredefinedFieldId();
        _lastRenderSignature = sig;

        // 2. 按类型计算作用对象坐标
        auto meshData = getCVM->getModelData()->getMeshData();
        auto geoManager = getCVM->getModelData()->getGeoManager();
        auto geoData = getCVM->getModelData()->getGeoData();
        QVector<QVector3D> objs;
        for (const auto& pair : merged)
        {
            std::vector<int> ids;
            ids.reserve(pair.second.size());
            for (const auto& d : pair.second)
                ids.push_back(d.result_id);
            if (ids.empty()) continue;

            switch (pair.first)
            {
            case PickDataType::Node:
                for (int id : ids)
                {
                    QVector3D pos = meshData->getVectorById(id);
                    if (!pos.isNull())
                        objs.push_back(pos);
                }
                break;
            case PickDataType::GeoPoint:
            {
                auto points = geoManager->CalculateVertexAttr(ids);
                for (auto p : points) objs.push_back(p.m_coor);
                break;
            }
            case PickDataType::GeoEdge:
            {
                QVector<int> pids; for (int id : ids) pids.append(id);
                auto coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(1, pids);
                for (auto c : coords) objs.push_back(c);
                break;
            }
            case PickDataType::GeoFace:
            {
                QVector<int> pids; for (int id : ids) pids.append(id);
                auto coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(2, pids);
                for (auto c : coords) objs.push_back(c);
                break;
            }
            case PickDataType::GeoSolid:
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

        // 3. 重画该预定义场符号
        // 符号数据未变化时跳过 UpdateAllSymbol：进入面板（树选中刷新已重建过）
        // 与连续防抖触发时数据往往不变，全量重建所有可见符号在大模型下代价很高
        const bool changed = AllRenderSymbolManager::getInstance()->assignPredefinedField(
            _currentField, objs, "T", QVector3D(255.0f, 165.0f, 0.0f));

        if (changed && !assignOnly)
        {
            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            if (symbolController) symbolController->UpdateAllSymbol();
            getCVM->getPreRenderWidget()->updatePaint();
        }
    }

    void mPredefinedFieldTempLoadPanel::clearInputs()
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

        _inp.clear();
        _filePathInput.clear();
        combofunc.clear();
        combofuncFile.clear();
        _functable.clear();
        _defineTypeCombo.clear();
        _interpolateTypeCombo.clear();
        _currentField.reset();

        if (_tableRowWidget)
        {
            delete _tableRowWidget;
            _tableRowWidget = nullptr;
        }
        _tableLabel = nullptr;
        _discreteControl = nullptr; // 随 _tableRowWidget 一并删除

        if (_fileRowWidget)
        {
            delete _fileRowWidget;
            _fileRowWidget = nullptr;
        }
        _fileLabel = nullptr;
        _filePathEdit = nullptr;
        _fileBrowseBtn = nullptr;

        if (_midNodeCheck)
        {
            delete _midNodeCheck;
            _midNodeCheck = nullptr;
        }
    }

    void mPredefinedFieldTempLoadPanel::setInputsValue(std::shared_ptr<MechCaseData::mPredefinedFieldBase> fieldData)
    {
        _isSettingValue = true;

        std::shared_ptr<MechCaseData::mPredefinedFieldTempLoad> fieldPtr = std::dynamic_pointer_cast<MechCaseData::mPredefinedFieldTempLoad>(fieldData);
        _currentField = fieldPtr;
        if (!fieldPtr)
        {
            _isSettingValue = false;
            return;
        }

        // 持久化策略绑定
        if (_pickWidgetController)
        {
            auto strategy = std::make_shared<MapRelationPersistenceStrategy>(
                MxFunctions::Physical_PredefinedField,
                fieldData->getPredefinedFieldId());
            _pickWidgetController->setPersistenceStrategy(strategy);
            _pickWidgetController->setSetResolver(&resolveSetToTypedData);

            QHash<QString, QString> setToType = getSetNamesList({ SetType::GeoPoint, SetType::GeoEdge, SetType::GeoFace, SetType::GeoSolid, SetType::Node });
            _pickWidgetController->setSelectSetList(setToType);

            applyAllowedPickTypes(fieldPtr->_defineType);
        }

        // 阻塞信号
        _defineTypeCombo.combobox->blockSignals(true);
        combofunc.combobox->blockSignals(true);
        combofuncFile.combobox->blockSignals(true);
        _interpolateTypeCombo.combobox->blockSignals(true);
        if (_midNodeCheck) _midNodeCheck->blockSignals(true);

        // 反显定义方式
        _defineTypeCombo.combobox->setCurrentIndex(fieldPtr->_defineType);

        // 函数反显
        auto refreshFunc = [this](comboBoxWithPushBtn& cb) {
            cb.combobox->clear();
            _functable.clear();
            auto allFunc = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getAllFunctionID();
            cb.combobox->addItem("");
            _functable[""] = -1;
            for (auto funcid : allFunc)
            {
                QString funcname = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionNameByID(funcid);
                cb.combobox->addItem(funcname);
                _functable[funcname] = funcid;
            }
            };

        refreshFunc(combofunc);
        refreshFunc(combofuncFile);

        auto& mapfield = getCVM->getModelData()->getMapRelationManager();
        int funcid = mapfield.getMapRelation<int>({ Physical_PredefinedField, fieldData->getPredefinedFieldId() }, Physical_Function);
        if (funcid != -1)
        {
            QString funcname = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionNameByID(funcid);
            combofunc.combobox->setCurrentText(funcname);
            combofuncFile.combobox->setCurrentText(funcname);
        }
        else
        {
            combofunc.combobox->setCurrentIndex(0);
            combofuncFile.combobox->setCurrentIndex(0);
        }

        // 作用对象反显
        // 单例面板复用：无论新载荷定义方式如何，先清空离散表格残留行，
        // 避免旧载荷独立赋值数据在用户切回独立赋值时复现并污染新载荷。
        // 独立赋值模式例外：下方马上 setRows(rows) 整体替换并同步选中态，
        // 先行清空只会多一次全量选中态同步与重绘（大表格下代价可观）
        if (_discreteControl && fieldPtr->_defineType != 1)
        {
            _discreteControl->setRows({});
        }
        PickWidgetData displayData;
        int setid = mapfield.getMapRelation<int>({ Physical_PredefinedField, fieldData->getPredefinedFieldId() }, Physical_Set);
        if (setid != -1 && fieldPtr->_defineType != 2)
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
        else if (fieldPtr->_defineType == 1)
        {
            // 独立赋值：优先保留 _nodeTempMap 中的完整 key，避免用 result_id2=0 的临时 pickData 破坏通用表格的 fullKey 完整性。
            std::vector<RenderPickResultData> nodePicks;
            for (const auto& entry : fieldPtr->_nodeTempMap)
            {
                nodePicks.push_back(entry.first);
            }
            if (!nodePicks.empty())
            {
                displayData.pickedData[PickDataType::Node] = std::move(nodePicks);
            }

            // 反显到离散数据控件（节点 + 已填温度值）
            if (_discreteControl)
            {
                QVector<mDiscreteDataControl::Row> rows;
                rows.reserve(static_cast<int>(fieldPtr->_nodeTempMap.size()));
                for (const auto& entry : fieldPtr->_nodeTempMap)
                {
                    QStringList values;
                    const QVariant v = !entry.second.empty() ? entry.second.at(0) : QVariant();
                    values << ((v.isValid() && !v.isNull()) ? QString::number(v.toDouble()) : QString());
                    rows.append(qMakePair(qMakePair(entry.first.result_id, -1), values));
                }
                _discreteControl->setRows(rows);
            }
        }

        if (_pickWidgetController)
        {
            if (fieldPtr->_defineType == 1 || fieldPtr->_defineType == 2)
            {
                // 独立赋值已剥离通用拾取：仅写入数据（供 pickState/持久化一致性），
                // 全程保持失活，不激活、不渲染任何拾取高亮；
                // 文件模式同样不激活拾取控件（隐藏），云图抑制模式由 slot_defineTypeChanged 管理。
                // 入口同步隐藏控件（对齐 slot_defineTypeChanged），保证 _isVisible 标志与实际一致，
                // 即使抑制被解除，applyHighlight 的可见性守卫也会拦住高亮渲染
                _pickWidgetController->setPickWidgetVisible(false);
                _pickWidgetController->setCloudChartMode(true);
                _pickWidgetController->displayData(displayData);
            }
            else
            {
                // displayData 默认不重画高亮；若控件此前已处于激活态，setPickActive(true) 会直接返回，
                // 该情况下需显式重画一次高亮以匹配反显数据。整体保证进入面板只做一次全量高亮。
                // 旧载荷在独立赋值/文件模式下曾 setPickWidgetVisible(false)，
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
            _pickWidgetController->setSelectSetBtnVisible(fieldPtr->_defineType != 1 && fieldPtr->_defineType != 2);
        }

        if (fieldPtr->_defineType == 1)
        {
            filterPicksToNodeOnly();
        }

        // 大小
        _inp.lineeditvalue->setExpression(fieldPtr->_temp);

        // 文件定义值
        if (_filePathEdit)
        {
            QFileInfo fi(fieldPtr->_filePath);
            _filePathEdit->setText(fi.fileName());
            _filePathEdit->setToolTip(fieldPtr->_filePath);
        }
        _filePathInput.lineeditvalue->setText(fieldPtr->_filePath);
        _interpolateTypeCombo.combobox->setCurrentIndex(fieldPtr->_interpolateType);
        if (_midNodeCheck)
        {
            _midNodeCheck->setChecked(fieldPtr->_midNodeInterpolate);
        }

        // 解除信号阻塞
        _defineTypeCombo.combobox->blockSignals(false);
        combofunc.combobox->blockSignals(false);
        combofuncFile.combobox->blockSignals(false);
        _interpolateTypeCombo.combobox->blockSignals(false);
        if (_midNodeCheck) _midNodeCheck->blockSignals(false);

        auto& map = getCVM->getModelData()->getMapRelationManager();
        //缩放因子
        double scale = map.getScaleByCase({ MxFunctions::PhysicalFieldType::Physical_PredefinedField, fieldPtr->getPredefinedFieldId() }, _caseId);
        _scale.lineeditvalue->setExpression(MxFunctions::mNumberToString(scale));

        // 独立赋值模式下的表格反显已在上文按 _nodeTempMap 写入离散数据控件，
        // 此处只负责实时符号渲染（独立赋值分支内部会兜底同步 map）
        _isSettingValue = false;

        updateLoadSymbolRealtime();
    }

    QVector<QWidget*> mPredefinedFieldTempLoadPanel::getRefStateInputs(FoldSection* fathertar)
    {
        refstate_ = fathertar;
        QVector<QWidget*>wds;
        wds.append(_scale.propwid);
        return wds;
    }

    QVector<QWidget*> mPredefinedFieldTempLoadPanel::getTargetInputs(FoldSection* fathertar)
    {
        fatar_ = fathertar;
        QVector<QWidget*>wds;
        if (_pickWidgetController)
        {
            wds.append(_pickWidgetController->widget());
        }
        return wds;
    }

    QVector<QWidget*> mPredefinedFieldTempLoadPanel::getValueInputs(FoldSection* fatherval)
    {
        faval_ = fatherval;
        QVector<QWidget*>wds;

        // 定义方式下拉框始终显示在最前面
        wds.append(_defineTypeCombo.propwid);

        int defineType = _defineTypeCombo.combobox->currentIndex();

        if (defineType == 0) // 统一赋值
        {
            wds.append(_inp.propwid);
            wds.append(combofunc.propwid);
        }
        else if (defineType == 1) // 独立赋值
        {
            wds.append(_tableRowWidget);
            wds.append(combofunc.propwid);
        }
        else // 文件定义
        {
            wds.append(_fileRowWidget);
            wds.append(_interpolateTypeCombo.propwid);

            QWidget* checkWidget = new QWidget();
            QHBoxLayout* checkLayout = new QHBoxLayout(checkWidget);
            checkLayout->setContentsMargins(0, 0, 0, 0);
            checkLayout->addWidget(_midNodeCheck);
            checkLayout->addStretch();
            wds.append(checkWidget);

            wds.append(combofuncFile.propwid);
        }

        return wds;
    }

    void mPredefinedFieldTempLoadPanel::storeFieldValue(std::shared_ptr<MechCaseData::mPredefinedFieldBase> fieldData)
    {
        if (fieldData == nullptr)
        {
            return;
        }

        std::shared_ptr<MechCaseData::mPredefinedFieldTempLoad> fieldPtr = std::dynamic_pointer_cast<MechCaseData::mPredefinedFieldTempLoad>(fieldData);
        if (!fieldPtr) return;

        auto& mapfield = getCVM->getModelData()->getMapRelationManager();

        int defineType = _defineTypeCombo.combobox->currentIndex();
        fieldPtr->_defineType = defineType;

        if (defineType == 0) // 统一赋值
        {
            fieldPtr->_temp = _inp.lineeditvalue->text().trimmed();

            int funcid = getFunc();
            mapfield.appendMapRelation({ Physical_PredefinedField, fieldData->getPredefinedFieldId() }, { Physical_Function, funcid });
        }
        else if (defineType == 1) // 独立赋值
        {
            int funcid = getFunc();
            mapfield.appendMapRelation({ Physical_PredefinedField, fieldData->getPredefinedFieldId() }, { Physical_Function, funcid });
        }
        else // 文件定义
        {
            fieldPtr->_filePath = _filePathEdit->toolTip();
            fieldPtr->_interpolateType = _interpolateTypeCombo.combobox->currentIndex();
            fieldPtr->_midNodeInterpolate = _midNodeCheck ? _midNodeCheck->isChecked() : false;

            int funcid = getFunc();
            mapfield.appendMapRelation({ Physical_PredefinedField, fieldData->getPredefinedFieldId() }, { Physical_Function, funcid });
        }

        auto& map = getCVM->getModelData()->getMapRelationManager();
        //缩放因子改变
        double scale = _scale.lineeditvalue->text().trimmed().toDouble();
        map.setScaleByCase({ MxFunctions::PhysicalFieldType::Physical_PredefinedField, fieldPtr->getPredefinedFieldId() }, _caseId, scale);
    }

    int mPredefinedFieldTempLoadPanel::getFunc()
    {
        int funcid = -1;
        int defineType = _defineTypeCombo.combobox->currentIndex();
        QString funcname;
        if (defineType == 2)
        {
            funcname = combofuncFile.combobox->currentText();
        }
        else
        {
            funcname = combofunc.combobox->currentText();
        }
        if (_functable.find(funcname) != _functable.end())
        {
            funcid = _functable[funcname];
        }
        return funcid;
    }
}
