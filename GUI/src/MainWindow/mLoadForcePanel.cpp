#include "mLoadForcePanel.h"
#include <QSet>
#include "mMechGlobalSignals.h"
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
#include "mGetSetFunction.h"

#include "mAppConfiguration.h"
#include "mDirectionSelectorWidget.h"
#include "mOtherFunction.h"

using namespace MechCommon;
using namespace MechPreData;

namespace MechMainWindow
{
    mLoadForcePanel::mLoadForcePanel(QWidget* parent)
        :mLoadPanelBase(parent)
    {
        _realtimeUpdateTimer = new QTimer(this);
        _realtimeUpdateTimer->setSingleShot(true);
        connect(_realtimeUpdateTimer, &QTimer::timeout, this, &mLoadForcePanel::onRealtimeUpdateTimeout);

        initInputs();
    }

    mLoadForcePanel::~mLoadForcePanel()
    {
        clearInputs();
    }

    void mLoadForcePanel::connectUnitSystemResponse()
    {
        connect(getCVM->getModelData()->getUnitSystemManager(), &mUnitSystemManager::sg_unitChanged, this, &mLoadForcePanel::slot_unitSystemResponse, Qt::UniqueConnection);
    }

    void mLoadForcePanel::disconnectUnitSystemResponse()
    {
        disconnect(getCVM->getModelData()->getUnitSystemManager(), &mUnitSystemManager::sg_unitChanged, this, &mLoadForcePanel::slot_unitSystemResponse);
    }

    void mLoadForcePanel::updateOpenedTable()
    {
        if (!_currentLoad || !_pickWidgetController)
        {
            return;
        }

        // 独立赋值模式下，nodeForceMap_ 与离散数据控件的行数据保持严格一致：
        // 表格是数据的权威来源（含拾取追加与手动删除），此处按控件当前行重建 map，
        // 保留原有完整 key（含 part_id）与已填分量。
        if (!_discreteControl)
        {
            return;
        }

        const auto rows = _discreteControl->rows();

        // 已填分量按节点 ID 保留；key 统一规范为 (result_id, -1, -1)，
        // 保证 slot_discreteCellEdited 可用同一规范 key 直接索引，无需线性扫描。
        std::map<int, QVector<QVariant>> existingValues;
        for (const auto& entry : _currentLoad->nodeForceMap_)
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
                (it != existingValues.end()) ? it->second : QVector<QVariant>(3);
        }

        _currentLoad->nodeForceMap_ = std::move(newMap);
    }

    void mLoadForcePanel::filterPicksToNodeOnly()
    {
        // 同步通用拾取控件最新数据到 _picks，避免过滤时丢失拾取/集合
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
        // 用用户级修改接口（置脏），切出 commit 时持久化这次"清除集合/非节点拾取"；
        // 从根上清除"已选择xx集"的残留显示。
        if (_pickWidgetController)
        {
            _pickWidgetController->replacePickedDataAndClearSets(picks);
        }
        // 独立赋值已完全剥离通用拾取：清空并抑制本控件的一切拾取高亮渲染，
        // 拾取由离散数据控件直接驱动视口选择容器，视口中不再出现蓝色高亮。
        if (_pickWidgetController)
        {
            _pickWidgetController->setCloudChartMode(true);
        }

        updateOpenedTable();
    }

    void mLoadForcePanel::slot_unitSystemResponse(UnitType type, QVector<int> idx)
    {
        Q_UNUSED(idx);
        if (!(type & UnitSystem)) return;
        if (_loadData)
            setInputsValue(_loadData);   //从载荷对象(基准值)重渲染，值+单位标签一次到位
    }

    void mLoadForcePanel::initInputs()
    {
        // ========== 新增：各向分量输入框初始化 ==========
        QStringList inpnames = { "X分量", "Y分量", "Z分量" };
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString unit = UnitSystems[sysunitidxs[0]][2];
        for (int i = 0; i < 3; ++i)
        {
            InputWidgetSet inp;
            inp.initWidget(inpnames[i], false);
            inp.labelname->setText(inpnames[i]);
            inp.lineeditvalue->setUnit(unit);
            inp.lineeditvalue->setFixedHeight(30);
            inputWidgets.append(inp);
            connect(inp.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this]() {
                    updateLoadSymbolRealtime(); // 输入变化实时刷新箭头
                    saveForceComponents();    // 单独保存当前分量
                });
        }

        // 延迟创建方向控件，首次切换到方向与大小模式时创建
        _pickWidgetController = mGeneralPickUp::createPickWidget(CBBlue);
        connect(_pickWidgetController, &mPickWidgetController::pickActiveChanged, this, [this](bool isActive) {
            if (isActive)
            {
                emit mMechGlobalSignals::getInstance()->activePickSig(5);
            }
            });
        connect(_pickWidgetController, &mPickWidgetController::dataChanged, this, [this]() {
            // 独立赋值模式下通用拾取控制器不再接收拾取（由离散数据控件直驱视口），
            // 其 dataChanged 只在各向分量/方向与大小模式下触发，无需按方式分支处理。
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

        QStringList combonames = { "定义方式" };
        for (int i = 0; i < 1; ++i)
        {
            singleComboBox combobox;
            combobox.initWidget(combonames[i]);
            combobox.combobox->setFixedHeight(30);
            comboBoxs.append(combobox);
        }

        // ========== 定义方式选项：各向分量/方向与大小/独立赋值/文件/分析场 ==========
        comboBoxs[0].combobox->addItems({ "各向分量", "方向与大小","独立赋值", "文件", "分析场" });
        connect(comboBoxs[0].combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mLoadForcePanel::slot_defineTypeChanged);

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
            this, &mLoadForcePanel::slot_analysisFieldChanged);

        _pickWidgetController->setDefultTip("选择对象");

        // 幅值曲线初始化（原有逻辑保留）
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
            this, &mLoadForcePanel::slot_funcChanged);
        connect(combofunc.pushbtn, &QPushButton::clicked, this, [this]() {
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createFunction(2);
            combofunc.combobox->blockSignals(true);
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
            combofunc.combobox->blockSignals(false);
            saveFunc();
            });

        // 坐标系初始化（原有逻辑保留）
        combocoor.initWidget("坐标系");
        combocoor.pushbtn->setIcon(QIcon(":/mMainWidgets/Icon/BrowserTree/zuobiaoxi.png"));
        combocoor.pushbtn->setFixedSize(30, 30);
        combocoor.pushbtn->setIconSize(QSize(18, 18));
        combocoor.combobox->setFixedHeight(30);
        auto allCoor = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getAllCoordinateNames();
        for (auto coorname : allCoor)
        {
            combocoor.combobox->addItem(coorname);
        }
        connect(combocoor.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mLoadForcePanel::slot_coorChanged);
        connect(combocoor.pushbtn, &QPushButton::clicked, this, [this]() {
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createCoordinate();
            combocoor.combobox->blockSignals(true);
            combocoor.combobox->clear();
            auto allCoor = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getAllCoordinateNames();
            for (auto coorname : allCoor)
            {
                combocoor.combobox->addItem(coorname);
            }
            combocoor.combobox->setCurrentIndex(allCoor.size() > 0 ? allCoor.size() : 0);
            combocoor.combobox->blockSignals(false);
            saveCoor();
            });

        // 文件选择行（原有逻辑保留）
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
        connect(_fileBrowseBtn, &QPushButton::clicked, this, &mLoadForcePanel::slot_browseFileClicked);
        fileRowLayout->addWidget(_fileBrowseBtn);

        // 独立赋值表格行：通用离散数据控件（入口按钮 + 锚定弹窗表格）
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
            { QStringLiteral("X分量"), QStringLiteral("Y分量"), QStringLiteral("Z分量") },
            &nodeSpec, _tableRowWidget);
        // 渲染云图可选：集中力沿用既有实时箭头渲染，不显示该按钮（默认隐藏）
        _discreteControl->setRenderButtonVisible(false);
        connect(_discreteControl, &mDiscreteDataControl::dataChanged,
            this, &mLoadForcePanel::slot_discreteDataChanged);
        connect(_discreteControl, &mDiscreteDataControl::cellEdited,
            this, &mLoadForcePanel::slot_discreteCellEdited);
        // 入口按钮从控件中剖分出来直接放入行布局：控件本体隐藏（其弹窗是
        // 宿主主窗口的子窗口，不受影响），按钮点击经控件转发弹窗开关，
        // 按钮可用状态不再与控件本体的显隐/禁用耦合
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

        // 缩放因子（原有逻辑保留）
        _scale.initWidget("缩放因子", false);
        _scale.labelname->setText("缩放因子");
        connect(_scale.lineeditvalue, &mCalcLineEdit::editingFinished,
            [this]() {
                saveScale();
            });
    }

    void mLoadForcePanel::ensureDirectionWidget()
    {
        if (_directionWidget)
            return;

        // 必须在已知父折叠块时创建，避免无 parent 的独立窗口短暂闪现。
        if (!faval_)
            return;

        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        _directionWidget = new mDirectionSelectorWidget(
            QStringLiteral("方向"), true, QStringLiteral("载荷大小"), false,
            UnitSystems[sysunitidxs[0]][2], false, faval_->contentWidget());
        _directionWidget->setVisible(false);

        // 使用控件默认的方向/坐标计算器（基于几何属性）
        _directionWidget->setDirectionCalculator(
            mDirectionSelectorWidget::DirectionCalculator());
        _directionWidget->setCoordinateCalculator(
            mDirectionSelectorWidget::CoordinateCalculator());

        connect(_directionWidget, &mDirectionSelectorWidget::directionChanged, this, [this]() {
            updateLoadSymbolRealtime();
            saveDirectionAndMagnitude();
            });
        connect(_directionWidget, &mDirectionSelectorWidget::magnitudeChanged, this, [this]() {
            updateLoadSymbolRealtime();
            saveDirectionAndMagnitude();
            });
        connect(_directionWidget, &mDirectionSelectorWidget::layoutRefreshRequested, this, [this]() {
            if (faval_)
            {
                faval_->refreshLayout();
            }
            });
        connect(_directionWidget, &mDirectionSelectorWidget::currentWayNoPickWidget, this, [this]() {
            //当方向控件当前方式页无拾取控件时，激活集中力面板中控件
            _pickWidgetController->setPickActive(true);
            });
    }

    void mLoadForcePanel::slot_defineTypeChanged(int idx)
    {
        // 分析场为组合框第4项，内部独立标志存储（way_ 仍为各向分量）
        const bool isFieldMode = (idx == 4);

        // 切换前移除方向控件
        if (_directionWidget && faval_ && faval_->layout())
        {
            _directionWidget->setParent(nullptr);
            faval_->layout()->removeWidget(_directionWidget);
        }

        // 切换定义方式时关闭离散数据弹窗并结束拾取，避免旧模式弹窗残留
        if (_discreteControl)
        {
            _discreteControl->closePopup();
        }

        faval_->removeAll();
        faval_->addRow(comboBoxs[0].propwid);
        comboBoxs[0].propwid->show();

        saveWayAndClearModeFields();

        // 同步拾取类型白名单
        applyAllowedPickTypes(idx);

        // 方向控件仅在方向与大小(idx==1)模式下使用，其余模式显式隐藏并清理其内部高亮
        if (_directionWidget && idx != 1)
        {
            _directionWidget->setPickWidgetVisible(false);
            _directionWidget->clearAllHighlight();
        }

        // 文件/分析场模式：完全隐藏拾取控件与方向控件，并清空所有高亮。
        // 独立赋值模式：拾取由离散数据控件直接驱动，通用拾取控件不再激活。
        // mPickWidgetController 隐藏时会根据当前激活状态自动清理拾取高亮或临时高亮。
        if (_pickWidgetController)
        {
            if (idx == 3 || idx == 2 || isFieldMode)
            {
                _pickWidgetController->setPickWidgetVisible(false);
            }
            else
            {
                _pickWidgetController->setPickWidgetVisible(true);
                _pickWidgetController->setPickActive(true);
            }
        }

        if (idx == 0) // 各向分量：XYZ输入框 + 幅值曲线 + 坐标系
        {
            faval_->addRow(inputWidgets[0].propwid);
            inputWidgets[0].propwid->show();
            faval_->addRow(inputWidgets[1].propwid);
            inputWidgets[1].propwid->show();
            faval_->addRow(inputWidgets[2].propwid);
            inputWidgets[2].propwid->show();
            faval_->addRow(combofunc.propwid);
            combofunc.propwid->show();
            faval_->addRow(combocoor.propwid);
            combocoor.propwid->show();
        }
        else if (idx == 1) // 方向与大小：方向控件 + 幅值曲线（无坐标系）
        {
            ensureDirectionWidget();
            if (_directionWidget)
            {
                _directionWidget->hide();
                faval_->addRow(_directionWidget);
                _directionWidget->show();
                _directionWidget->setPickWidgetVisible(true);
            }
            faval_->addRow(combofunc.propwid);
            combofunc.propwid->show();
        }
        else if (idx == 2) // 独立赋值：表格 + 幅值曲线 + 坐标系
        {
            faval_->addRow(_tableRowWidget);
            _tableRowWidget->show();
            faval_->addRow(combofunc.propwid);
            combofunc.propwid->show();
            faval_->addRow(combocoor.propwid);
            combocoor.propwid->show();
        }
        else if (idx == 3) // 文件
        {
            saveFilePath();
            faval_->addRow(_fileRowWidget);
            _fileRowWidget->show();
        }
        else // 分析场：分析场下拉框 + 幅值曲线 + 坐标系
        {
            faval_->addRow(_analysisFieldCombo.propwid);
            _analysisFieldCombo.propwid->show();
            faval_->addRow(combofunc.propwid);
            combofunc.propwid->show();
            faval_->addRow(combocoor.propwid);
            combocoor.propwid->show();
        }

        faval_->refreshLayout();

        // 独立赋值：隐藏集合选择按钮，强制仅节点（filterPicksToNodeOnly 内同时
        // 开启云图抑制模式，彻底关闭通用拾取控件的高亮渲染，拾取高亮完全剥离）
        if (idx == 2)
        {
            filterPicksToNodeOnly();
            if (_pickWidgetController)
            {
                _pickWidgetController->setSelectSetBtnVisible(false);
            }
        }
        else if (isFieldMode)
        {
            // 分析场：无作用对象，清空拾取并抑制高亮渲染
            if (_pickWidgetController)
            {
                _pickWidgetController->setSelectSetBtnVisible(false);
                _pickWidgetController->setCloudChartMode(true);
                _pickWidgetController->clearAllHighlight();
                PickWidgetData empty;
                _pickWidgetController->replacePickedDataAndClearSets(empty.pickedData);
            }
            updateLoadSymbolRealtime();
        }
        else // 各向分量/方向与大小/文件：显示集合选择按钮，恢复正常高亮渲染
        {
            if (_pickWidgetController)
            {
                _pickWidgetController->setSelectSetBtnVisible(true);
                _pickWidgetController->setCloudChartMode(false);
            }
            updateLoadSymbolRealtime();
        }

        // 定义方式改变属于结构性变化，由外部重建面板
        emit sg_rebuildPanel();
    }

    void mLoadForcePanel::slot_browseFileClicked()
    {
        QString defaultDir = MAppConfiguration::mAppConfiguration::getInstance()->_workingDir;
        QFileInfo info(defaultDir);
        if (defaultDir.isEmpty() || !info.exists())
        {
            defaultDir = QDir(qApp->applicationDirPath() + "/../WorkingDir").absolutePath();
        }
        QString filePath = QFileDialog::getOpenFileName(
            nullptr, "选择载荷文件", defaultDir, "Text Files (*.txt);;All Files (*)");
        if (!filePath.isEmpty())
        {
            QFileInfo fi(filePath);
            QString fileName = fi.fileName();
            _filePathEdit->setText(fileName);
            _filePathEdit->setToolTip(filePath);
            saveFilePath();
        }
    }

    void mLoadForcePanel::slot_discreteDataChanged()
    {
        // 离散数据控件行变化（拾取追加/删除/清空/分量增减）：
        // 按控件当前行重建 nodeForceMap_，随后刷新实时箭头并通知外部。
        // 拾取会话进行中跳过：确认拾取（完成拾取）后控件会补发一次 dataChanged，
        // 此时才执行重建与渲染，避免拾取界面期间反复重绘。
        if (!_currentLoad || (_discreteControl && _discreteControl->isPicking()))
        {
            return;
        }
        updateOpenedTable();
        // doUpdateLoadSymbolRealtime 内对独立赋值已含 updateOpenedTable 兜底，
        // 这里重建 map 后直接走防抖刷新即可
        updateLoadSymbolRealtime();
        notifyLoadChanged();
    }

    void mLoadForcePanel::slot_discreteCellEdited(const QPair<int, int>& key, int valueIndex, const QString& text)
    {
        if (!_currentLoad || key.first <= 0)
        {
            return;
        }
        // 单点写回：仅更新/清除该节点的该分量，节点本身始终保留。
        // 集中力固定 3 个分量（X/Y/Z），key 规范为 result_id/-1/-1。
        const RenderPickResultData loadKey(key.first, -1, -1);
        QVector<QVariant>& values = _currentLoad->nodeForceMap_[loadKey];
        if (values.size() < 3)
        {
            values.resize(3);
        }
        if (valueIndex >= 0 && valueIndex < 3)
        {
            bool ok = false;
            const double v = text.toDouble(&ok);
            values[valueIndex] = (ok && !text.isEmpty()) ? QVariant(v) : QVariant();
        }
        updateLoadSymbolRealtime();
        notifyLoadChanged();
    }

    void mLoadForcePanel::slot_funcChanged(int idx)
    {
        Q_UNUSED(idx)
        saveFunc();
    }

    void mLoadForcePanel::slot_coorChanged(int idx)
    {
        Q_UNUSED(idx)
        saveCoor();
    }

    void mLoadForcePanel::slot_analysisFieldChanged(int idx)
    {
        Q_UNUSED(idx)
        saveAnalysisField();
    }


    void mLoadForcePanel::clearInputs()
    {
        // 清理各向分量输入框
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
        if (_directionWidget)
        {
            _directionWidget->hide();
            delete _directionWidget;
            _directionWidget = nullptr;
        }
        for (auto combo : comboBoxs) combo.clear();
        comboBoxs.clear();
        combofunc.clear();
        combocoor.clear();
        _analysisFieldCombo.clear();
        _analysisFieldTable.clear();
        _functable.clear();
        _selectedItems.clear();
        _currentLoad.reset();

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
    }

    void mLoadForcePanel::clearData()
    {
        // 清空各向分量输入框
        for (auto inp : inputWidgets)
        {
            inp.lineeditvalue->setExpression("");
        }
        if (_directionWidget)
        {
            _directionWidget->setMagnitudeValue("");
        }
    }

    void mLoadForcePanel::applyAllowedPickTypes(int way)
    {
        if (!_pickWidgetController) return;
        if (way == 3 || way == 4) // 文件定义/分析场：禁用全部拾取
        {
            _pickWidgetController->setAllPickTypesDisabled(true);
        }
        else if (way == 2) // 独立赋值：仅设置白名单记录，不切换视口过滤器
                           // （拾取过滤器由离散数据控件在开始拾取时自行激活/恢复）
        {
            _pickWidgetController->setAllowedTypes(&pickOnlyNode);
        }
        else // 各向分量(0) / 方向与大小(1)：几何+节点全支持
        {
            _pickWidgetController->setAllowedTypes(&pickGeoPointEdgeFaceSolidNodePart);
            _pickWidgetController->setPickType();
        }
    }

    void mLoadForcePanel::deactivatePickAndCommit()
    {
        // 面板为单例复用，切换载荷/面板时不会走析构/clearInputs，
        // 由统一退出出口（unEditLoad）调用，失活并提交当前拾取数据。
        // 切出面板时所有拾取控件统一：失活不保留临时高亮 + 清空全部高亮（含残留的失活临时高亮）。
        // 注意：清空模型树等场景下渲染控件可能已销毁，需判空避免空指针解引用崩溃。
        const bool renderValid = (getCVM && getCVM->getPreRenderWidget());

        // 离散数据控件：结束拾取并关闭弹窗
        if (_discreteControl)
        {
            _discreteControl->closePopup();
        }

        for (auto* ctrl : { _pickWidgetController })
        {
            if (!ctrl) continue;
            // 切出/取消编辑面板：失活不保留临时高亮（置标志 false），失活提交即清空高亮。
            ctrl->setKeepHighlightOnDeactivate(false);
            // 失活提交：激活态时执行完整 commit 流程 + 清空分支 clearAllHighlight 清空高亮；
            // 已失活时幂等返回（不重复 commit、不清高亮）。
            const bool wasActive = ctrl->isPickActive();
            if(wasActive) ctrl->setPickActive(false);
            // 兜底：仅对"此前已被同面板另一控件抢活而处于失活临时高亮态"的控件清空残留。
            // 激活控件已由 setPickActive(false) 清空分支清过，此处跳过避免重复清空。
            if (renderValid && !wasActive)
            {
                ctrl->clearAllHighlight();
            }
            // 恢复默认（保留），供下次进入面板/同面板多控件切换使用。
            ctrl->setKeepHighlightOnDeactivate(true);
        }

        // 方向选择控件内部包含多个子拾取控件，切出面板时同样要失活并清理高亮，
        // 否则 s_activeController 仍指向其内部控件，其它面板激活时可能误判或残留高亮。
        if (_directionWidget)
        {
            auto deactivateChildPick = [](mPickWidgetController* ctrl) {
                if (!ctrl) return;
                const bool wasActive = ctrl->isPickActive();
                if (!wasActive) return;
                ctrl->setKeepHighlightOnDeactivate(false);
                ctrl->setPickActive(false);
                ctrl->setKeepHighlightOnDeactivate(true);
            };
            deactivateChildPick(_directionWidget->startPickController());
            deactivateChildPick(_directionWidget->endPickController());
            deactivateChildPick(_directionWidget->objectPickController());
            _directionWidget->clearAllHighlight();
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

    void mLoadForcePanel::updateLoadSymbolRealtime()
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

    void mLoadForcePanel::onRealtimeUpdateTimeout()
    {
        doUpdateLoadSymbolRealtime();
    }

    void mLoadForcePanel::doUpdateLoadSymbolRealtime()
    {
        if (!_pickWidgetController || !_currentLoad)
            return;

        const int way = comboBoxs.isEmpty() ? 0 : comboBoxs[0].combobox->currentIndex();

        // 分析场方式无作用对象，无需渲染集中力符号（清除残留箭头）
        if (way == 4)
        {
            size_t sig = AllRenderSymbolManager::beginRenderSignature();
            AllRenderSymbolManager::combineRenderSignature(sig, static_cast<size_t>(_currentLoad->getLoadBoundaryId()));
            if (sig != _lastRenderSig)
            {
                _lastRenderSig = sig;
                AllRenderSymbolManager::getInstance()->assignLoad(
                    _currentLoad, {}, "Load_Force", QVector3D(133, 19, 33));
                auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
                if (symbolController) symbolController->UpdateAllSymbol();
                getCVM->getPreRenderWidget()->updatePaint();
            }
            return;
        }

        // 独立赋值：先按离散数据控件当前行同步 nodeForceMap_（数据源为表格，
        // 通用拾取控制器已不再接收视口拾取，不能再以它为准），再合成方向。
        if (way == 2)
        {
            updateOpenedTable();
        }

        const PickWidgetData allData = _pickWidgetController->getAllData();
        auto meshData = getCVM->getModelData()->getMeshData();
        QVector<QVector3D> objs;

        // 集合展开与作用对象坐标计算仅"各向分量(0)/方向与大小(1)"需要；
        // 独立赋值(2)按表格数据逐节点取坐标、文件(3)无箭头，跳过该段全量计算。
        if (way == 0 || way == 1)
        {
            // 1. 汇总当前数据：拾取(pickedData) + 集合(selectedSetNames 按名称展开)，合并为按类型分组的 map。
            //    坐标计算与 AllRenderSymbolManager::getLoadSet 保持一致。
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

            // 2. 按类型计算作用对象坐标（参照 getLoadSet）。
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
                case PickDataType::Node:
                case PickDataType::Element:
                case PickDataType::ElementFace:
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
        }

        // 3. 方向：根据当前定义方式从输入/数据实时求有效方向。
        //    统一赋值(0)：从方向选择控件获取方向；独立赋值(2)：从 nodeForceMap_ 各节点分量合成；
        //    文件(3)：无有效方向。
        //    无有效方向时不兜底 +X，而是传空 pos_dir 清除箭头，
        //    避免"只拾取/选集合、没填力也画出 +X 箭头"的错误表现。
        QVector<std::pair<QVector3D, QVector3D>> pos_dir;

        if (way == 2) // 独立赋值：逐节点合成方向
        {
            // 独立赋值已剥离通用拾取：方向合成直接以离散数据控件行（nodeForceMap_ 已同步）为准，
            // 不再读取 _pickWidgetController->getAllData()（控制器不再接收视口拾取，数据已停滞）。
            // 同时清空共享选中容器中的网格拾取残留，避免视口持续显示拾取选中态。
            for (const auto& entry : _currentLoad->nodeForceMap_)
            {
                const QVector<QVariant>& values = entry.second;
                QVector3D nodeDir(0, 0, 0);
                if (values.size() > 0 && values[0].isValid() && !values[0].isNull()) nodeDir.setX(values[0].toDouble());
                if (values.size() > 1 && values[1].isValid() && !values[1].isNull()) nodeDir.setY(values[1].toDouble());
                if (values.size() > 2 && values[2].isValid() && !values[2].isNull()) nodeDir.setZ(values[2].toDouble());
                if (nodeDir.isNull()) continue;
                nodeDir.normalize();
                QVector3D pos = meshData->getVectorById(entry.first.result_id);
                if (pos.isNull()) continue;
                pos_dir.push_back({ pos, nodeDir });
            }

            // 拾取会话结束后的下一次刷新时，离散控件已自行清空选择容器残留，
            // 这里不再重复清理（避免与拾取会话中的选中态恢复逻辑互相干扰）。
        }
        else if (way == 0) // 各向分量：从输入框合成方向
        {
            double fx = inputWidgets[0].lineeditvalue->text().toDouble();
            double fy = inputWidgets[1].lineeditvalue->text().toDouble();
            double fz = inputWidgets[2].lineeditvalue->text().toDouble();
            QVector3D dir(fx, fy, fz);
            if (!dir.isNull() && !objs.isEmpty())
            {
                dir.normalize();
                pos_dir.reserve(objs.size());
                for (const auto& o : objs)
                    pos_dir.push_back({ o, dir });
            }
        }
        else if (way == 1) // 方向与大小：从方向控件取方向
        {
            if (_directionWidget)
            {
                QVariant dirVar = _directionWidget->getDirection();
                if (dirVar.isValid() && !dirVar.isNull())
                {
                    QVector3D dir = dirVar.value<QVector3D>();
                    if (!dir.isNull() && !objs.isEmpty())
                    {
                        dir.normalize();
                        // 渲染方向由方向控件与载荷大小符号共同决定：大小为负数时方向取反
                        bool magOk = false;
                        double mag = _directionWidget->magnitudeValue().trimmed().toDouble(&magOk);
                        if (magOk && mag < 0)
                        {
                            dir = -dir;
                        }
                        pos_dir.reserve(objs.size());
                        for (const auto& o : objs)
                            pos_dir.push_back({ o, dir });
                    }
                }
            }
        }
        // way==3 文件：pos_dir为空，清除箭头
        
        // 4. 重画该载荷的箭头符号（key=load_<id>，同名覆盖；objs 为空等同清除残留箭头）。
        // 最终数据签名：载荷ID+位置方向对未变化时跳过符号赋值与 UpdateAllSymbol
        // 全量重建（进入面板防抖触发/切出冲刷时数据往往与上次相同）
        size_t sig = AllRenderSymbolManager::beginRenderSignature();
        AllRenderSymbolManager::combineRenderSignature(sig, static_cast<size_t>(_currentLoad->getLoadBoundaryId()));
        AllRenderSymbolManager::combineRenderSignature(sig, pos_dir);
        if (sig == _lastRenderSig)
        {
            return;
        }
        _lastRenderSig = sig;

        AllRenderSymbolManager::getInstance()->assignLoad(
            _currentLoad, pos_dir, "Load_Force", QVector3D(133, 19, 33));

        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        if (symbolController) symbolController->UpdateAllSymbol();
        getCVM->getPreRenderWidget()->updatePaint();
    }

    void mLoadForcePanel::setInputsValue(std::shared_ptr<MechCaseData::mLoadBoundaryBase>loadData)
    {
        std::shared_ptr<MechCaseData::mLoadForce> forcePtr = std::dynamic_pointer_cast<MechCaseData::mLoadForce>(loadData);
        _currentLoad = forcePtr;
        if (!forcePtr) return;

        if (_directionWidget)
        {
            _directionWidget->resetToDefault();
        }
        // 持久化策略绑定
        if (_pickWidgetController)
        {
            auto strategy = std::make_shared<MapRelationPersistenceStrategy>(
                MxFunctions::Physical_LoadBoundary,
                _loadData->getLoadBoundaryId());
            _pickWidgetController->setPersistenceStrategy(strategy);
            _pickWidgetController->setSetResolver(&resolveSetToTypedData);

            QHash<QString, QString> setToType = getSetNamesList({ SetType::GeoPoint,SetType::GeoEdge, SetType::GeoFace,SetType::GeoSolid,SetType::Node });
            _pickWidgetController->setSelectSetList(setToType);

            applyAllowedPickTypes(forcePtr->isAnalysisFieldMode_ ? 4 : forcePtr->way_);
        }

        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Force, { 3,0,0,0 }, sysunitidxs);
        QString unit = UnitSystems[sysunitidxs[0]][2];

        // 更新各向分量输入框单位
        for (auto inp : inputWidgets)
        {
            inp.lineeditvalue->setUnit(unit);
            inp.lineeditvalue->setExpression("");
            inputWidgets[1].lineeditvalue->clear();// 清空界面显示文字
        }
        // 更新方向控件单位
        if (_directionWidget)
        {
            _directionWidget->setMagnitudeUnit(unit);
        }

        comboBoxs[0].combobox->blockSignals(true);
        // 分析场方式反显为组合框第4项（way_ 保持各向分量语义）
        comboBoxs[0].combobox->setCurrentIndex(forcePtr->isAnalysisFieldMode_ ? 4 : forcePtr->way_);
        comboBoxs[0].combobox->blockSignals(false);

        // 同步集合按钮显隐
        if (_pickWidgetController)
        {
            _pickWidgetController->setSelectSetBtnVisible(forcePtr->way_ != 2 && forcePtr->way_ != 3 && !forcePtr->isAnalysisFieldMode_);
        }

        // 独立赋值/文件模式不在此处 filterPicksToNodeOnly()：此时控件内还是上一次编辑的残留拾取，
        // 该调用会以残留数据全量重画高亮，并触发 dataChanged（用残留拾取重建 nodeForceMap_/表格、
        // 多发一次 notifyLoadChanged 与防抖渲染）。随后 displayData 会用当前载荷数据整体替换控件
        // 数据（独立赋值反显 nodeForceMap_ 节点、文件模式反显空数据），集合/非节点残留随之清除。

        // ========== 分模式反显 ==========
        if (forcePtr->way_ == 0) // 各向分量：反显XYZ
        {
            QVariant forcexdata = forcePtr->force_x;
            if (!forcexdata.isNull() && forcexdata.isValid())
            {
                inputWidgets[0].lineeditvalue->setExpression(QString::number(forcexdata.toDouble() * ratio));
            }
            QVariant forceydata = forcePtr->force_y;
            if (!forceydata.isNull() && forceydata.isValid())
            {
                inputWidgets[1].lineeditvalue->setExpression(QString::number(forceydata.toDouble() * ratio));
            }
            QVariant forcezdata = forcePtr->force_z;
            if (!forcezdata.isNull() && forcezdata.isValid())
            {
                inputWidgets[2].lineeditvalue->setExpression(QString::number(forcezdata.toDouble() * ratio));
            }
        }
        else if (forcePtr->way_ == 1) // 方向与大小：用结构体反显方向控件
        {
            ensureDirectionWidget();
            if (_directionWidget)
            {
                const auto& state = forcePtr->dirPickState_;

                // 先阻断 directionChanged/magnitudeChanged 信号，避免反显过程中触发保存
                bool oldSignalsBlocked = _directionWidget->blockSignals(true);

                _directionWidget->setDirectionMode(
                    static_cast<mDirectionSelectorWidget::DirectionMode>(state.mode));
                _directionWidget->setReversed(state.reversed);

                // 自定义坐标反显
                if (state.mode == mDirectionSelectorWidget::CustomCoordinate &&
                    state.startPoint.isValid() && !state.startPoint.isNull() &&
                    state.endPoint.isValid() && !state.endPoint.isNull())
                {
                    _directionWidget->setCustomCoordinatePoints(
                        state.startPoint.value<QVector3D>(),
                        state.endPoint.value<QVector3D>());
                }
                // 选择对象反显
                else if (state.mode == mDirectionSelectorWidget::SelectObject)
                {
                    PickWidgetData displayData;

                    // 有效拾取结果判空：通过 result_id 判断（id>0 视为有效拾取）
                    if (state.selectedObject.second.result_id > 0)
                    {
                        // 用 pair 的 first 作 key，second 作 value，还原 map 结构适配控件接口
                        displayData.pickedData[state.selectedObject.first].push_back(state.selectedObject.second);
                    }

                    // 调用控件统一反显接口
                    _directionWidget->objectPickController()->displayData(displayData);
                }
                // 直接输入反显
                else if (state.mode == mDirectionSelectorWidget::DirectInput)
                {
                    _directionWidget->setDirection(state.directInputDir);
                }
                // 坐标轴模式自动根据mode推断方向
                else if (forcePtr->dir_.isValid() && !forcePtr->dir_.isNull())
                {
                    _directionWidget->setDirection(forcePtr->dir_.value<QVector3D>());
                }

                // 载荷大小反显
                QVariant forcedata = forcePtr->force_;
                if (!forcedata.isNull() && forcedata.isValid())
                {
                    _directionWidget->setMagnitudeValue(QString::number(forcedata.toDouble() * ratio));
                }
                else
                {
                    _directionWidget->setMagnitudeValue("");
                }

                _directionWidget->blockSignals(oldSignalsBlocked);
            }
        }
        else if (forcePtr->way_ == 3) // 文件反显
        {
            QFileInfo fi(forcePtr->path_);
            _filePathEdit->setText(fi.fileName());
            _filePathEdit->setToolTip(forcePtr->path_);
        }

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
            auto& mapload_ = getCVM->getModelData()->getMapRelationManager();
            int analysisFieldId = mapload_.getMapRelation<int>({ Physical_LoadBoundary, loadData->getLoadBoundaryId() }, Physical_Field, MapRelationType::MapRelation_AnalysisField);
            if (analysisFieldId != -1)
            {
                QString fieldname = analysisFieldMgr.getFieldNameByID(analysisFieldId);
                _analysisFieldCombo.combobox->setCurrentText(fieldname);
            }
        }
        _analysisFieldCombo.combobox->blockSignals(false);

        // 幅值曲线+坐标系反显（原有逻辑保留）
        combofunc.combobox->blockSignals(true);
        combocoor.combobox->blockSignals(true);

        combofunc.combobox->clear();
        combocoor.combobox->clear();
        auto& mapload = getCVM->getModelData()->getMapRelationManager();
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
        int funcid = mapload.getMapRelation<int>({ Physical_LoadBoundary,loadData->getLoadBoundaryId() }, Physical_Function);
        if (funcid != -1)
        {
            QString funcname = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionNameByID(funcid);
            combofunc.combobox->setCurrentText(funcname);
        }

        auto allCoor = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getAllCoordinateNames();
        for (auto coorname : allCoor)
        {
            combocoor.combobox->addItem(coorname);
        }
        int coorid = mapload.getMapRelation<int>({ Physical_LoadBoundary,loadData->getLoadBoundaryId() }, Physical_Coordinate);
        if (coorid != -1)
        {
            QString coorname = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getCoordinateNameByID(coorid);
            combocoor.combobox->setCurrentText(coorname);
        }
        combofunc.combobox->blockSignals(false);
        combocoor.combobox->blockSignals(false);

        // 作用对象反显（原有逻辑保留）
        // 单例面板复用：无论新载荷定义方式如何，先清空离散表格残留行，
        // 避免旧载荷独立赋值数据在用户切回独立赋值时复现并污染新载荷
        if (_discreteControl)
        {
            _discreteControl->setRows({});
        }
        PickWidgetData displayData;
        int setid = mapload.getMapRelation<int>({ Physical_LoadBoundary,loadData->getLoadBoundaryId() }, Physical_Set);
        if (setid != -1 && (forcePtr->way_ == 0 || forcePtr->way_ == 1))
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
        else if (forcePtr->way_ == 2)
        {
            // 独立赋值：优先保留 nodeForceMap_ 中的完整 key（含原始 part_id），
            // 避免用 result_id2=0 的临时 pickData 破坏通用表格的 fullKey 完整性。
            for (auto it = forcePtr->nodeForceMap_.begin(); it != forcePtr->nodeForceMap_.end(); ++it)
            {
                const RenderPickResultData& key = it->first;
                displayData.pickedData[PickDataType::Node].push_back(key);
            }

            // 反显到离散数据控件（节点 + 已填分量）
            if (_discreteControl)
            {
                QVector<mDiscreteDataControl::Row> rows;
                rows.reserve(static_cast<int>(forcePtr->nodeForceMap_.size()));
                for (const auto& entry : forcePtr->nodeForceMap_)
                {
                    QStringList values;
                    for (int i = 0; i < 3; ++i)
                    {
                        const QVariant v = i < entry.second.size() ? entry.second.at(i) : QVariant();
                        values << ((v.isValid() && !v.isNull()) ? QString::number(v.toDouble()) : QString());
                    }
                    rows.append(qMakePair(qMakePair(entry.first.result_id, -1), values));
                }
                _discreteControl->setRows(rows);
            }
        }

        if (_pickWidgetController)
        {
            if (forcePtr->isAnalysisFieldMode_)
            {
                // 分析场：无作用对象，全程保持失活，不激活、不渲染任何拾取高亮
                _pickWidgetController->setPickWidgetVisible(false);
                _pickWidgetController->setCloudChartMode(true);
                _pickWidgetController->displayData(displayData);
            }
            else if (forcePtr->way_ == 2)
            {
                // 独立赋值已剥离通用拾取：仅写入数据（供 pickState/持久化一致性），
                // 全程保持失活，不激活、不渲染任何拾取高亮。
                // 入口同步隐藏控件（对齐 slot_defineTypeChanged 的 idx==2 分支），
                // 保证 _isVisible 标志与实际一致，applyHighlight 的可见性守卫才能拦住高亮渲染
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

        // 缩放因子反显
        double scale = mapload.getScaleByCase({ MxFunctions::PhysicalFieldType::Physical_LoadBoundary ,loadData->getLoadBoundaryId() }, _caseId);
        _scale.lineeditvalue->setExpression(MxFunctions::mNumberToString(scale));

        //反写完毕后对通用拾取控件进行判断
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
        QMetaObject::invokeMethod(this, &mLoadForcePanel::updateLoadSymbolRealtime, Qt::QueuedConnection);
    }

    QVector<QWidget*> mLoadForcePanel::getRefStateInputs(FoldSection* fathertar)
    {
        refstate_ = fathertar;
        QVector<QWidget*>wds;
        // 分析场方式无大小概念，不需要缩放因子（折叠框整体隐藏）
        if (_currentLoad && _currentLoad->isAnalysisFieldMode_)
            return wds;
        wds.append(_scale.propwid);
        return wds;
    }

    QVector<QWidget*> mLoadForcePanel::getTargetInputs(FoldSection* fathertar)
    {
        fatar_ = fathertar;
        QVector<QWidget*>wds;
        // 分析场方式无作用对象（折叠框整体隐藏）
        if (_currentLoad && _currentLoad->isAnalysisFieldMode_)
            return wds;
        if (_pickWidgetController)
        {
            wds.append(_pickWidgetController->widget());
        }
        return wds;
    }

    QVector<QWidget*> mLoadForcePanel::getValueInputs(FoldSection* fatherval)
    {
        faval_ = fatherval;
        QVector<QWidget*>wds;
        wds.append(comboBoxs[0].propwid);

        int defineType = comboBoxs[0].combobox->currentIndex();

        if (defineType == 0) // 各向分量
        {
            wds.append(inputWidgets[0].propwid);
            wds.append(inputWidgets[1].propwid);
            wds.append(inputWidgets[2].propwid);
            wds.append(combofunc.propwid);
            wds.append(combocoor.propwid);
        }
        else if (defineType == 1) // 方向与大小
        {
            ensureDirectionWidget();
            if (_directionWidget)
            {
                wds.append(_directionWidget);
            }
            wds.append(combofunc.propwid);
        }
        else if (defineType == 2) // 独立赋值
        {
            wds.append(_tableRowWidget);
            wds.append(combofunc.propwid);
            wds.append(combocoor.propwid);
        }
        else if (defineType == 3) // 文件
        {
            wds.append(_fileRowWidget);
        }
        else // 分析场：分析场下拉框 + 幅值曲线 + 坐标系
        {
            wds.append(_analysisFieldCombo.propwid);
            wds.append(combofunc.propwid);
            wds.append(combocoor.propwid);
        }

        return wds;
    }

    void mLoadForcePanel::saveForceComponents()
    {
        if (!_currentLoad)
        {
            return;
        }

        QVector<int> curidx = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Force, curidx, { 3,0,0,0 });

        // 读取输入文本并校验数字转换结果
        QString xStr = inputWidgets[0].lineeditvalue->text().trimmed();
        QString yStr = inputWidgets[1].lineeditvalue->text().trimmed();
        QString zStr = inputWidgets[2].lineeditvalue->text().trimmed();

        bool xOk = false, yOk = false, zOk = false;
        double fx = xStr.toDouble(&xOk);
        double fy = yStr.toDouble(&yOk);
        double fz = zStr.toDouble(&zOk);

        // 分量独立赋值：转换成功则计算单位比例后赋值，失败则设为无效空值
        if (xOk) {
            _currentLoad->force_x = fx * ratio;
        }
        else {
            _currentLoad->force_x.clear();
        }

        if (yOk) {
            _currentLoad->force_y = fy * ratio;
        }
        else {
            _currentLoad->force_y.clear();
        }

        if (zOk) {
            _currentLoad->force_z = fz * ratio;
        }
        else {
            _currentLoad->force_z.clear();
        }

        // 方向向量：仅当三个分量全部转换成功时才计算方向，否则方向置为无效
        if (xOk && yOk && zOk) {
            QVector3D dir(fx, fy, fz);
            if (!dir.isNull()) {
                dir.normalize();
                _currentLoad->dir_ = dir;
            }
            else {
                _currentLoad->dir_ = QVariant();
            }
        }
        else {
            _currentLoad->dir_ = QVariant();
        }

        notifyLoadChanged();
    }

    void mLoadForcePanel::saveWayAndClearModeFields()
    {
        if (!_currentLoad)
        {
            return;
        }
        const int idx = comboBoxs[0].combobox->currentIndex();
        // 分析场（组合框第4项）：way_ 保持各向分量语义，独立标志记录分析场方式
        _currentLoad->isAnalysisFieldMode_ = (idx == 4);
        _currentLoad->way_ = (idx == 4) ? 0 : idx;
        // 重置所有模式字段，避免交叉污染
        _currentLoad->force_ = QVariant();
        _currentLoad->force_x = QVariant();
        _currentLoad->force_y = QVariant();
        _currentLoad->force_z = QVariant();
        _currentLoad->dir_ = QVariant();
        _currentLoad->path_ = "";
        _currentLoad->dirPickState_ = DirectionPickState();
    }

    void mLoadForcePanel::applyDirectionWidgetToLoad(std::shared_ptr<MechCaseData::mLoadForce> load)
    {
        if (!load || !_directionWidget)
        {
            return;
        }

        QVector<int> curidx = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Force, curidx, { 3,0,0,0 });

        // 存大小
        QString forcestr = _directionWidget->magnitudeValue();
        if (!forcestr.isEmpty())
        {
            bool ok = false;
            double f = forcestr.trimmed().toDouble(&ok);
            if (ok)
            {
                load->force_ = f * ratio;
            }
            else
            {
                load->force_ = QVariant();
            }
        }
        else
        {
            load->force_ = QVariant();
        }

        // 存方向矢量
        QVariant dirVar = _directionWidget->getDirection();
        if (dirVar.isValid() && !dirVar.isNull())
        {
            QVector3D dir = dirVar.value<QVector3D>();
            if (!dir.isNull())
            {
                dir.normalize();
                load->dir_ = dir;
                double f = load->force_.isValid() ? load->force_.toDouble() : 0.0;
                load->force_x = f * dir.x();
                load->force_y = f * dir.y();
                load->force_z = f * dir.z();
            }
            else
            {
                load->dir_ = QVariant();
                load->force_x = QVariant();
                load->force_y = QVariant();
                load->force_z = QVariant();
            }
        }
        else
        {
            load->dir_ = QVariant();
            load->force_x = QVariant();
            load->force_y = QVariant();
            load->force_z = QVariant();
        }

        // 存方向控件完整状态到结构体
        auto& state = load->dirPickState_;
        state.mode = static_cast<int>(_directionWidget->directionMode());
        state.reversed = _directionWidget->isReversed();

        if (state.mode == mDirectionSelectorWidget::CustomCoordinate)
        {
            state.startPoint = QVariant::fromValue(_directionWidget->startPoint());
            state.endPoint = QVariant::fromValue(_directionWidget->endPoint());
        }
        else if (state.mode == mDirectionSelectorWidget::SelectObject)
        {
            PickWidgetData pickData = _directionWidget->objectPickController()->getAllData();
            if (!pickData.pickedData.empty())
            {
                auto it = pickData.pickedData.begin();
                state.selectedObject.first = it->first;
                if (!it->second.empty())
                {
                    state.selectedObject.second = it->second.front();
                }
            }
        }
        else if (state.mode == mDirectionSelectorWidget::DirectInput)
        {
            state.directInputDir = dirVar.value<QVector3D>();
        }
    }

    void mLoadForcePanel::saveDirectionAndMagnitude()
    {
        if (!_currentLoad || !_directionWidget)
        {
            return;
        }

        const int idx = comboBoxs[0].combobox->currentIndex();
        _currentLoad->isAnalysisFieldMode_ = (idx == 4);
        _currentLoad->way_ = (idx == 4) ? 0 : idx;
        applyDirectionWidgetToLoad(_currentLoad);
        notifyLoadChanged();
    }

    void mLoadForcePanel::saveScale()
    {
        if (!_currentLoad)
        {
            return;
        }
        double scale = _scale.lineeditvalue->text().trimmed().toDouble();
        auto& mapload = getCVM->getModelData()->getMapRelationManager();
        mapload.setScaleByCase({ MxFunctions::PhysicalFieldType::Physical_LoadBoundary ,_loadData->getLoadBoundaryId() }, _caseId, scale);
        notifyLoadChanged();
    }

    void mLoadForcePanel::saveFilePath()
    {
        if (!_currentLoad)
        {
            return;
        }
        _currentLoad->path_ = _filePathEdit->toolTip();
        notifyLoadChanged();
    }

    void mLoadForcePanel::saveFunc()
    {
        if (!_currentLoad)
        {
            return;
        }
        if (_currentLoad->way_ == 3 && !_currentLoad->isAnalysisFieldMode_) // 文件模式无函数
        {
            return;
        }
        int funcid = getFunc();
        auto& mapload = getCVM->getModelData()->getMapRelationManager();
        mapload.appendMapRelation({ Physical_LoadBoundary ,_loadData->getLoadBoundaryId() }, { Physical_Function,funcid });
        notifyLoadChanged();
    }

    void mLoadForcePanel::saveAnalysisField()
    {
        if (!_currentLoad)
        {
            return;
        }
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

    void mLoadForcePanel::saveCoor()
    {
        if (!_currentLoad)
        {
            return;
        }
        // 各向分量/独立赋值/分析场保存坐标系
        if (_currentLoad->way_ != 0 && _currentLoad->way_ != 2 && !_currentLoad->isAnalysisFieldMode_)
        {
            return;
        }
        int coorid = getCoor();
        auto& mapload = getCVM->getModelData()->getMapRelationManager();
        mapload.appendMapRelation({ Physical_LoadBoundary ,_loadData->getLoadBoundaryId() }, { Physical_Coordinate,coorid });
        notifyLoadChanged();
    }

    void mLoadForcePanel::notifyLoadChanged()
    {
        if (!_loadData)
        {
            return;
        }
        // 仅通知外部刷新树/渲染，不再调用 storeLoadValue 全量保存
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_LoadBoundary, _loadData->getLoadBoundaryId());
    }

    int mLoadForcePanel::getFunc()
    {
        int funcid = -1;
        QString funcname = combofunc.combobox->currentText();
        if (_functable.find(funcname) != _functable.end())
        {
            funcid = _functable[funcname];
        }
        return funcid;
    }

    int mLoadForcePanel::getCoor()
    {
        int coorid = -1;
        QString coorname = combocoor.combobox->currentText();
        coorid = getCVM->getModelData()->getManager<MechCoordinateData::mCoordinateManager>(Physical_Coordinate).getIDByCoordinateName(coorname);
        return coorid;
    }

}