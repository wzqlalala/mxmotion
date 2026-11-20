#include "mMechGlobalSignals.h"
#include <QMetaObject>
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mSetManager.h"
#include "mMapRelationEnum.h"
#include "mMapRelationManager.h"
#include "mUnitSystemManager.h"
#include "mInfoWindow.h"
#include "PickCommonFunction.h"
#include "mWorkingConditions.h"
#include "mDockLayoutManager.h"
#include "mGetSetFunction.h"
#include "mGeneralPickUp.h"
#include "mGetSetTypeTemplate.h"
#include "mDataStatusManager.h"
#include "AllRenderSymbolManager.h"

//Geo
#include "mGeoManager.h"

//Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/label/mSymbolController.h"

//Basic
#include<mMixSet.h>
#include<mMeshData.h>
#include<mGeoSet.h>
#include<mNode.h>
#include<mElementSet.h>

#include "mRotatePhysicalPowerPanel.h"
#include "mOtherFunction.h"

using namespace MechCommon;
using namespace MechPreData;

namespace MechMainWindow
{
    const QString mRotatePhysicalPowerPanel::s_dirArrowRenderKey = QStringLiteral("rotate_physical_power_dir_arrow");
    const QVector3D mRotatePhysicalPowerPanel::s_dirArrowColor = QVector3D(19.0f, 133.0f, 246.0f);

    mRotatePhysicalPowerPanel::mRotatePhysicalPowerPanel(QWidget* parent)
        :mLoadPanelBase(parent)
    {
        initInputs();
    }

    mRotatePhysicalPowerPanel::~mRotatePhysicalPowerPanel()
    {
        clearInputs();
    }

    void mRotatePhysicalPowerPanel::connectUnitSystemResponse()
    {
        connect(getCVM->getModelData()->getUnitSystemManager(), &CommonData::mUnitSystemManager::sg_unitChanged,
            this, &mRotatePhysicalPowerPanel::slot_unitSystemResponse, Qt::UniqueConnection);
    }

    void mRotatePhysicalPowerPanel::disconnectUnitSystemResponse()
    {
        disconnect(getCVM->getModelData()->getUnitSystemManager(), &CommonData::mUnitSystemManager::sg_unitChanged,
            this, &mRotatePhysicalPowerPanel::slot_unitSystemResponse);
    }

    std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> mRotatePhysicalPowerPanel::getPickedData()
    {
        if (!_pickWidgetController)
            return std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>();
        return _pickWidgetController->getAllData().pickedData;
    }

    void mRotatePhysicalPowerPanel::filterPicksToElementOnly()
    {
        if (!_pickWidgetController)
            return;
        // 同步通用拾取控件最新数据到 picks，避免过滤时丢失拾取/集合
        auto picks = _pickWidgetController->getAllData().pickedData;

        auto it = picks.begin();
        while (it != picks.end())
        {
            if (it->first != PickDataType::Element)
                it = picks.erase(it);
            else
                ++it;
        }

        _pickWidgetController->replacePickedDataAndClearSets(picks);
        // 独立赋值已完全剥离通用拾取：开启云图抑制模式，彻底关闭通用拾取控件的高亮渲染
        _pickWidgetController->setCloudChartMode(true);
        updateOpenedTable();
    }

    void mRotatePhysicalPowerPanel::updateOpenedTable()
    {
        if (!_currentLoad || !_discreteControl)
            return;

        // 独立赋值模式下，_elementCoorMap 与离散数据控件的行数据保持严格一致：
        // 表格是数据的权威来源（含拾取追加与手动删除），此处按控件当前行重建 map。
        // map key 为 (elementId, partId, -1)：part_id 优先保留 map 原有完整 key，
        // 查不到再按单元 ID 反查（会话级缓存），与 mIMxFile 读入键规范对齐。
        const auto rows = _discreteControl->rows();

        std::map<int, RenderPickResultData> existingKeys;
        std::map<int, QVector<QVariant>> existingValues;
        for (const auto& entry : _currentLoad->_elementCoorMap)
        {
            const int elemId = entry.first.result_id;
            if (existingKeys.find(elemId) == existingKeys.end())
            {
                existingKeys[elemId] = entry.first;
                existingValues[elemId] = entry.second;
            }
        }

        auto meshData = (getCVM && getCVM->getModelData()) ? getCVM->getModelData()->getMeshData() : nullptr;
        QHash<int, int> partIdCache;
        auto resolvePartId = [&](int elemId) -> int {
            if (!meshData) return -1;
            auto it = partIdCache.find(elemId);
            if (it == partIdCache.end())
                it = partIdCache.insert(elemId, meshData->getPartIdByEleId(elemId));
            return it.value();
        };

        std::map<RenderPickResultData, QVector<QVariant>> newMap;
        for (const auto& row : rows)
        {
            const int elemId = row.first.first;
            if (elemId <= 0)
                continue;
            auto keyIt = existingKeys.find(elemId);
            if (keyIt != existingKeys.end())
            {
                newMap[keyIt->second] = existingValues[elemId];
            }
            else
            {
                newMap[RenderPickResultData(elemId, resolvePartId(elemId), -1)] = QVector<QVariant>(3);
            }
        }

        _currentLoad->_elementCoorMap = std::move(newMap);
    }

    void mRotatePhysicalPowerPanel::slot_rotateTypeChanged(int idx)
    {
        if (_currentLoad)
            _currentLoad->_rotateType = idx;

        // 根据类型切换方向控件 magnitude 标签和单位
        if (_directionWidget)
        {
            bool isCentrifugal = (idx == 0);
            _directionWidget->setMagnitudeLabel(isCentrifugal ? QStringLiteral("转速") : QStringLiteral("大小"));
            if (isCentrifugal)
            {
                QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
                _directionWidget->setMagnitudeUnit(CommonData::SpeedSystem[sysunitidxs[2]]);
                _directionWidget->setMagnitudeUnitVisible(true);
            }
            else
            {
                _directionWidget->setMagnitudeUnit("");
                _directionWidget->setMagnitudeUnitVisible(false);
            }
        }

        // 类型切换属于结构性变化，仍通过 sg_rebuildPanel 由外部槽统一整体保存
        emit sg_rebuildPanel();
    }

    void mRotatePhysicalPowerPanel::slot_defineTypeChanged(int idx)
    {
        if (_currentLoad)
            _currentLoad->_defineType = idx;

        // 切换定义方式时关闭离散数据弹窗并结束拾取，避免旧模式弹窗残留
        if (_discreteControl)
            _discreteControl->closePopup();

        // 同步拾取类型白名单
        applyAllowedPickTypes();

        if (idx == 1)
        {
            filterPicksToElementOnly();
        }

        rebuildValueLayout();

        // 定义方式切换影响作用对象折叠框显隐（独立赋值不显示），通知外部整体重建面板
        emit sg_rebuildPanel();
    }

    void mRotatePhysicalPowerPanel::rebuildValueLayout()
    {
        if (!faval_) return;

        int defineType = _defineTypeCombo.combobox->currentIndex();

        // 切换布局前先把方向控件从旧父布局移除，避免 faval_ 重建后野指针/重复添加
        if (_directionWidget && faval_->layout())
        {
            _directionWidget->setParent(nullptr);
            faval_->layout()->removeWidget(_directionWidget);
        }

        faval_->removeAll();
        faval_->addRow(_rotateTypeCombo.propwid); // 类型
        _rotateTypeCombo.propwid->show();

        if (defineType == 0)
        {
            // 统一赋值：方向控件已含大小/转速 + 函数
            ensureDirectionWidget();
            if (_directionWidget)
            {
                _directionWidget->hide();
                faval_->addRow(_directionWidget); // 方向（含大小/转速）
                _directionWidget->show();
            }
            faval_->addRow(_defineTypeCombo.propwid); // 定义方式
            _defineTypeCombo.propwid->show();
            faval_->addRow(combofunc.propwid); // 函数
            combofunc.propwid->show();
        }
        else
        {
            // 独立赋值：定义方式 + 表格按钮 + 函数（方向控件仅在统一赋值使用）
            faval_->addRow(_defineTypeCombo.propwid); // 定义方式
            _defineTypeCombo.propwid->show();
            faval_->addRow(_tableRowWidget); // 表格按钮
            _tableRowWidget->show();
            faval_->addRow(combofunc.propwid); // 函数
            combofunc.propwid->show();

            if (_directionWidget)
            {
                _directionWidget->setPickWidgetVisible(false);
                _directionWidget->clearAllHighlight();
            }
        }
        faval_->refreshLayout();

        // 独立赋值：隐藏通用拾取控件与集合按钮（拾取由离散数据控件直接驱动视口）；
        // 统一赋值：恢复通用拾取控件可见性与正常高亮渲染
        if (defineType == 1)
        {
            if (_pickWidgetController)
            {
                _pickWidgetController->setPickWidgetVisible(false);
                _pickWidgetController->setSelectSetBtnVisible(false);
            }
        }
        else
        {
            if (_pickWidgetController)
            {
                _pickWidgetController->setPickWidgetVisible(true);
                _pickWidgetController->setPickActive(true);
                _pickWidgetController->setSelectSetBtnVisible(true);
                _pickWidgetController->setCloudChartMode(false);
            }
        }
    }

    void mRotatePhysicalPowerPanel::slot_discreteDataChanged()
    {
        // 离散数据控件行变化（拾取追加/删除/清空）：
        // 按控件当前行重建 _elementCoorMap，随后刷新实时符号并通知外部。
        // 拾取会话进行中跳过：确认拾取（完成拾取）后控件会补发一次 dataChanged，
        // 此时才执行重建与渲染，避免拾取期间反复重绘。
        if (!_currentLoad || (_discreteControl && _discreteControl->isPicking()))
            return;
        updateOpenedTable();
        updateLoadSymbolRealtime();
        notifyLoadChanged();
    }

    void mRotatePhysicalPowerPanel::slot_discreteCellEdited(const QPair<int, int>& key, int valueIndex, const QString& text)
    {
        if (!_currentLoad || key.first <= 0)
            return;
        if (valueIndex < 0 || valueIndex > 2)
            return;

        // 单点写回：仅更新该单元的该分量。map key 为 (elementId, partId, -1)：
        // part_id 优先沿用 map 原有完整 key（含文件读入的原始 part_id），
        // 查不到再按单元 ID 反查（会话级缓存），避免构造出与 map 中不等价的键。
        const int elemId = key.first;
        int partId = -1;
        bool foundKey = false;
        for (const auto& entry : _currentLoad->_elementCoorMap)
        {
            if (entry.first.result_id == elemId)
            {
                partId = entry.first.part_id;
                foundKey = true;
                break;
            }
        }
        if (!foundKey && getCVM && getCVM->getModelData())
        {
            if (auto meshData = getCVM->getModelData()->getMeshData())
                partId = meshData->getPartIdByEleId(elemId);
        }

        const RenderPickResultData loadKey(elemId, partId, -1);
        QVector<QVariant>& values = _currentLoad->_elementCoorMap[loadKey];
        if (values.size() < 3)
            values.resize(3);

        // 分量为字符串：空串置空；数值列按 double 解析；起点/方向点列按 "x, y, z" 解析
        if (valueIndex == 0)
        {
            bool ok = false;
            const double v = text.toDouble(&ok);
            values[0] = (ok && !text.isEmpty()) ? QVariant(v) : QVariant();
        }
        else
        {
            QVariant vecVar;
            if (!text.trimmed().isEmpty())
            {
                const QStringList parts = text.split(",");
                if (parts.size() >= 3)
                {
                    vecVar = QVariant::fromValue(QVector3D(
                        parts[0].trimmed().toDouble(),
                        parts[1].trimmed().toDouble(),
                        parts[2].trimmed().toDouble()));
                }
            }
            values[valueIndex] = vecVar;
        }

        updateLoadSymbolRealtime();
        notifyLoadChanged();
    }

    void mRotatePhysicalPowerPanel::slot_funcChanged(int idx)
    {
        Q_UNUSED(idx)
        saveFunc();
    }

    void mRotatePhysicalPowerPanel::initInputs()
    {
        // 类型
        _rotateTypeCombo.initWidget("类型");
        _rotateTypeCombo.combobox->addItems({ "离心力", "旋转加速度" });
        _rotateTypeCombo.combobox->setCurrentIndex(0);
        connect(_rotateTypeCombo.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mRotatePhysicalPowerPanel::slot_rotateTypeChanged);

        // 定义方式
        _defineTypeCombo.initWidget("定义方式");
        _defineTypeCombo.combobox->addItem("统一赋值");
        _defineTypeCombo.combobox->addItem("独立赋值");
        _defineTypeCombo.combobox->setCurrentIndex(0);
        connect(_defineTypeCombo.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mRotatePhysicalPowerPanel::slot_defineTypeChanged);


        // 通用拾取控件
        _pickWidgetController = mGeneralPickUp::createPickWidget(CBBlue);
        connect(_pickWidgetController, &mPickWidgetController::pickActiveChanged, this, [this](bool isActive) {
            if (isActive)
            {
                emit mMechGlobalSignals::getInstance()->activePickSig(8);
            }
            });
        connect(_pickWidgetController, &mPickWidgetController::dataChanged, this, [this]() {
            // 独立赋值模式下通用拾取控制器不再接收拾取（由离散数据控件直驱视口），
            // 其 dataChanged 只在统一赋值模式下触发，无需按方式分支处理。
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

        // 表格行控件：通用离散数据控件（入口按钮 + 锚定弹窗表格）
        _tableRowWidget = new QWidget();
        QHBoxLayout* tableLayout = new QHBoxLayout(_tableRowWidget);
        tableLayout->setContentsMargins(0, 0, 0, 0);
        tableLayout->setSpacing(8);
        _tableLabel = new QLabel("单元数据");
        _tableLabel->setMinimumWidth(87);
        tableLayout->addWidget(_tableLabel);

        mDiscreteDataControl::PickSpec eleSpec;
        eleSpec.typeName = QStringLiteral("单元");
        eleSpec.idHeaders = QStringList{ QStringLiteral("单元编号") };
        eleSpec.keyOf = [](const RenderPickResultData& d) { return qMakePair(d.result_id, -1); };
        eleSpec.pickTypes = { MxFunctions::PickDataType::Element };
        eleSpec.pickFilter = 8; // 视口拾取过滤器：仅单元（与 activePickSig 索引一致）
        // 拾取由离散数据控件直接驱动视口选择容器，不再经过通用拾取控件
        _discreteControl = new mDiscreteDataControl(
            QStringLiteral("单元数据"),
            { QStringLiteral("值"), QStringLiteral("起始点"), QStringLiteral("方向点") },
            &eleSpec, _tableRowWidget);
        // 旋转体力沿用既有实时符号渲染，不显示渲染按钮（默认隐藏）
        _discreteControl->setRenderButtonVisible(false);
        connect(_discreteControl, &mDiscreteDataControl::dataChanged,
            this, &mRotatePhysicalPowerPanel::slot_discreteDataChanged);
        connect(_discreteControl, &mDiscreteDataControl::cellEdited,
            this, &mRotatePhysicalPowerPanel::slot_discreteCellEdited);
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

        _functable.clear();
        // 函数
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
            this, &mRotatePhysicalPowerPanel::slot_funcChanged);
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

        // 缩放因子
        _scale.initWidget("缩放因子", false);
        _scale.labelname->setText("缩放因子");
        connect(_scale.lineeditvalue, &mCalcLineEdit::editingFinished,
            [this]() {
                saveScale();
            });
    }

    void mRotatePhysicalPowerPanel::ensureDirectionWidget()
    {
        if (_directionWidget)
            return;

        // 必须在已知父折叠块时创建，避免无 parent 的独立窗口短暂闪现或父窗口已被释放
        if (!faval_)
            return;

        // 方向选择控件：统一赋值下替代原起始点/方向点参考
        // 默认模式为 X 轴（无默认方向选项），默认起点/方向点对应 000 / 100
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        _directionWidget = new mDirectionSelectorWidget(
            QStringLiteral("方向"), true, QStringLiteral("转速"), true,
            CommonData::SpeedSystem[sysunitidxs[2]], false, faval_->contentWidget());
        _directionWidget->setVisible(false);
        _directionWidget->setDirectionMode(mDirectionSelectorWidget::XAxis);

        // 默认计算回调
        _directionWidget->setDirectionCalculator(mDirectionSelectorWidget::DirectionCalculator());
        _directionWidget->setCoordinateCalculator(mDirectionSelectorWidget::CoordinateCalculator());

        connect(_directionWidget, &mDirectionSelectorWidget::directionChanged, this, [this]() {
            updateLoadSymbolRealtime();
            saveUniformValue();
            });
        connect(_directionWidget, &mDirectionSelectorWidget::magnitudeChanged, this, [this]() {
            updateLoadSymbolRealtime();
            saveUniformValue();
            });
        connect(_directionWidget, &mDirectionSelectorWidget::layoutRefreshRequested, this, [this]() {
            if (faval_)
            {
                faval_->refreshLayout();
            }
            });
        connect(_directionWidget, &mDirectionSelectorWidget::currentWayNoPickWidget, this, [this]() {
            // 方向控件当前方式页无拾取控件时，激活面板中外层通用拾取控件
            if (_pickWidgetController)
            {
                _pickWidgetController->setPickActive(true);
            }
            });
    }

    void mRotatePhysicalPowerPanel::saveUniformValue()
    {
        if (!_currentLoad || !_directionWidget)
            return;

        QVariant valueData = QVariant();
        QString valueStr = _directionWidget->magnitudeValue().trimmed();
        if (!valueStr.isEmpty())
        {
            double value = valueStr.toDouble();
            // 离心力类型时，将用户输入的当前转速单位值转换为内部 rad/s 存储
            if (_rotateTypeCombo.combobox->currentIndex() == 0)
            {
                QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
                double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Speed, sysunitidxs, { 2,0,0,0 });
                valueData = QVariant::fromValue<double>(value * ratio);
            }
            else //旋转体力时无需单位转换
            {
                valueData = QVariant::fromValue<double>(value);
            }
        }
        _currentLoad->_uniformValue = valueData;

        // 方向控件状态实时保存，保证切换类型/面板后反显一致
        saveDirectionWidgetState();

        notifyLoadChanged();
    }

    void mRotatePhysicalPowerPanel::saveDirectionWidgetState()
    {
        if (!_currentLoad || !_directionWidget)
            return;

        // 1. 保存方向分解结果：起点(方向控件起点) + 方向点(起点 + 方向)
        QVariant dirVar = _directionWidget->getDirection();
        QVector3D dir = dirVar.isValid() && !dirVar.isNull() ? dirVar.value<QVector3D>() : QVector3D(1, 0, 0);
        if (dir.isNull())
            dir = QVector3D(1, 0, 0);

        // 自定义坐标模式下使用用户设置的实际起点；其余模式沿用原点作为旋转轴参考点
        if (_directionWidget->directionMode() == mDirectionSelectorWidget::CustomCoordinate)
        {
            _currentLoad->_startPoint1 = _directionWidget->startPoint();
        }
        else
        {
            _currentLoad->_startPoint1 = QVector3D(0, 0, 0);
        }
        _currentLoad->_directPoint2 = _currentLoad->_startPoint1 + dir;

        // 2. 保存方向控件完整 UI 状态
        auto& state = _currentLoad->_dirPickState;
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
            state.directInputDir = dir;
        }
    }

    void mRotatePhysicalPowerPanel::saveFunc()
    {
        if (!_currentLoad)
            return;

        int funcid = getFunc();
        if (funcid == -1)
            return;

        auto& mapload = getCVM->getModelData()->getMapRelationManager();
        mapload.appendMapRelation({ Physical_LoadBoundary ,_loadData->getLoadBoundaryId() }, { Physical_Function,funcid });
        notifyLoadChanged();
    }

    void mRotatePhysicalPowerPanel::saveScale()
    {
        if (!_currentLoad)
            return;

        double scale = _scale.lineeditvalue->text().trimmed().toDouble();
        auto& mapload = getCVM->getModelData()->getMapRelationManager();
        mapload.setScaleByCase({ MxFunctions::PhysicalFieldType::Physical_LoadBoundary ,_loadData->getLoadBoundaryId() }, _caseId, scale);
        notifyLoadChanged();
    }

    void mRotatePhysicalPowerPanel::notifyLoadChanged()
    {
        if (!_loadData)
            return;
        // 仅通知外部刷新树/渲染，不再调用 storeLoadValue 全量保存
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_LoadBoundary, _loadData->getLoadBoundaryId());
    }

    void mRotatePhysicalPowerPanel::clearInputs()
    {
        if (_directionWidget)
        {
            _directionWidget->clearAllHighlight();
            delete _directionWidget;
            _directionWidget = nullptr;
        }

        if (_pickWidgetController)
        {
            _pickWidgetController->setPickActive(false);
            delete _pickWidgetController;
            _pickWidgetController = nullptr;
        }

        combofunc.clear();
        _scale.clear();
        _rotateTypeCombo.clear();
        _defineTypeCombo.clear();
        _functable.clear();

        if (_tableRowWidget)
        {
            delete _tableRowWidget;
            _tableRowWidget = nullptr;
        }
        _tableLabel = nullptr;
        _discreteControl = nullptr; // 随 _tableRowWidget 一并删除

        disconnectUnitSystemResponse();
        _currentLoad.reset();
    }

    void mRotatePhysicalPowerPanel::setInputsValue(std::shared_ptr<MechCaseData::mLoadBoundaryBase>loadData)
    {
        _currentLoad = std::dynamic_pointer_cast<MechCaseData::mRotatePhysicalPower>(loadData);
        if (!_currentLoad) return;

        std::shared_ptr<MechCaseData::mRotatePhysicalPower> loadPtr = _currentLoad;

        // 类型
        _rotateTypeCombo.combobox->blockSignals(true);
        _rotateTypeCombo.combobox->setCurrentIndex(loadPtr->_rotateType);
        _rotateTypeCombo.combobox->blockSignals(false);

        // 定义方式
        _defineTypeCombo.combobox->blockSignals(true);
        _defineTypeCombo.combobox->setCurrentIndex(loadPtr->_defineType);
        _defineTypeCombo.combobox->blockSignals(false);

        // 方向控件仅在统一赋值模式下创建/使用，避免独立赋值模式下反显到不存在的野控件
        if (loadPtr->_defineType == 0)
        {
            ensureDirectionWidget();
        }

        // 方向控件反显：阻断信号避免反显过程中触发保存
        if (_directionWidget)
        {
            bool oldSignalsBlocked = _directionWidget->blockSignals(true);
            _directionWidget->resetToDefault();

            const auto& state = loadPtr->_dirPickState;
            _directionWidget->setDirectionMode(
                static_cast<mDirectionSelectorWidget::DirectionMode>(state.mode));
            _directionWidget->setReversed(state.reversed);

            // 自定义坐标反显：优先用持久化的 _startPoint1/_directPoint2 反推起点终点，
            // 避免 state 中的 QVariant 失效后落到兜底分支被重置为 X 轴。
            if (state.mode == mDirectionSelectorWidget::CustomCoordinate)
            {
                QVector3D startPt = loadPtr->_startPoint1;
                QVector3D endPt = loadPtr->_directPoint2;
                if (state.startPoint.isValid() && !state.startPoint.isNull())
                    startPt = state.startPoint.value<QVector3D>();
                if (state.endPoint.isValid() && !state.endPoint.isNull())
                    endPt = state.endPoint.value<QVector3D>();
                _directionWidget->setCustomCoordinatePoints(startPt, endPt);
            }
            // 选择对象反显
            else if (state.mode == mDirectionSelectorWidget::SelectObject)
            {
                PickWidgetData displayData;
                if (state.selectedObject.second.result_id > 0)
                {
                    displayData.pickedData[state.selectedObject.first].push_back(state.selectedObject.second);
                }
                _directionWidget->objectPickController()->displayData(displayData);
            }
            // 直接输入反显
            else if (state.mode == mDirectionSelectorWidget::DirectInput)
            {
                _directionWidget->setDirection(state.directInputDir);
            }
            // 坐标轴/默认方向：兜底，用分解后的方向点推断
            else if (!loadPtr->_directPoint2.isNull())
            {
                QVector3D dir = loadPtr->_directPoint2;
                dir.normalize();
                QVector3D absDir(qAbs(dir.x()), qAbs(dir.y()), qAbs(dir.z()));
                if (state.mode == mDirectionSelectorWidget::XAxis ||
                    state.mode == mDirectionSelectorWidget::YAxis ||
                    state.mode == mDirectionSelectorWidget::ZAxis)
                {
                    _directionWidget->setReversed(dir[static_cast<int>(state.mode) - 1] < 0);
                }
                else
                {
                    _directionWidget->setDirectionMode(mDirectionSelectorWidget::XAxis);
                    _directionWidget->setReversed(dir.x() < 0);
                }
            }
            else
            {
                _directionWidget->setDirectionMode(mDirectionSelectorWidget::XAxis);
            }

            // 根据类型刷新 magnitude 标签和单位
            bool isCentrifugal = (loadPtr->_rotateType == 0);
            _directionWidget->setMagnitudeLabel(isCentrifugal ? QStringLiteral("转速") : QStringLiteral("大小"));
            QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
            QString speedUnit = isCentrifugal ? CommonData::SpeedSystem[sysunitidxs[2]] : QString("");
            _directionWidget->setMagnitudeUnit(speedUnit);
            _directionWidget->setMagnitudeUnitVisible(isCentrifugal);

            // 数值回显
            QVariant valueData = loadPtr->_uniformValue;
            QString valueStr = "";
            if (!valueData.isNull() && valueData.isValid())
            {
                double rads = valueData.value<double>();
                if (isCentrifugal)
                {
                    double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Speed, { 2,0,0,0 }, sysunitidxs);
                    valueStr = MxFunctions::mNumberToString(rads * ratio);
                }
                else
                {
                    valueStr = MxFunctions::mNumberToString(rads);
                }
            }
            _directionWidget->setMagnitudeValue(valueStr);

            _directionWidget->blockSignals(oldSignalsBlocked);
        }

        // 函数下拉刷新：阻断信号避免 clear/addItem/setCurrentText 过程中触发保存
        combofunc.combobox->blockSignals(true);
        combofunc.combobox->clear();
        auto& mapload = getCVM->getModelData()->getMapRelationManager();
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
        int funcid = mapload.getMapRelation<int>({ Physical_LoadBoundary,loadData->getLoadBoundaryId() }, Physical_Function);
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
                _loadData->getLoadBoundaryId());
            _pickWidgetController->setPersistenceStrategy(strategy);
            _pickWidgetController->setSetResolver(&resolveSetToTypedData);

            QHash<QString, QString> setToType = getSetNamesList({ SetType::GeoPoint, SetType::GeoEdge, SetType::GeoFace, SetType::GeoSolid, SetType::Node, SetType::Element });
            _pickWidgetController->setSelectSetList(setToType);

            applyAllowedPickTypes();
        }

        // 拾取数据反显
        // 单例面板复用：无论新载荷定义方式如何，先清空离散表格残留行，
        // 避免旧载荷独立赋值数据在用户切回独立赋值时复现并污染新载荷
        if (_discreteControl)
        {
            _discreteControl->setRows({});
        }
        PickWidgetData displayData;
        int setid = mapload.getMapRelation<int>({ Physical_LoadBoundary,loadData->getLoadBoundaryId() }, Physical_Set);
        if (setid != -1 && loadPtr->_defineType == 0)
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
        // 独立赋值模式：直接从数据类生成拾取数据，并反显到离散数据控件
        if (loadPtr->_defineType == 1)
        {
            for (const auto& entry : loadPtr->_elementCoorMap)
            {
                // 直接 push 数据 map 中的原始完整 key（含原始 part_id），禁止重新构造临时 RenderPickResultData
                displayData.pickedData[PickDataType::Element].push_back(entry.first);
            }

            // 反显到离散数据控件（单元 + 已填分量）
            if (_discreteControl)
            {
                QVector<mDiscreteDataControl::Row> rows;
                rows.reserve(static_cast<int>(loadPtr->_elementCoorMap.size()));
                for (const auto& entry : loadPtr->_elementCoorMap)
                {
                    QStringList values;
                    const QVariant val = entry.second.size() > 0 ? entry.second.at(0) : QVariant();
                    values << ((val.isValid() && !val.isNull()) ? QString::number(val.toDouble()) : QString());
                    for (int i = 1; i < 3; ++i)
                    {
                        const QVariant v = i < entry.second.size() ? entry.second.at(i) : QVariant();
                        if (v.isValid() && !v.isNull())
                        {
                            const QVector3D p = v.value<QVector3D>();
                            values << QStringLiteral("%1, %2, %3").arg(p.x()).arg(p.y()).arg(p.z());
                        }
                        else
                        {
                            values << QString();
                        }
                    }
                    rows.append(qMakePair(qMakePair(entry.first.result_id, -1), values));
                }
                _discreteControl->setRows(rows);
            }
        }

        if (_pickWidgetController)
        {
            if (loadPtr->_defineType == 1)
            {
                // 独立赋值已剥离通用拾取：仅写入数据（供 pickState/持久化一致性），
                // 全程保持失活，不激活、不渲染任何拾取高亮。
                // 入口同步隐藏控件（对齐 slot_defineTypeChanged），保证 _isVisible 与实际一致，
                // applyHighlight 的可见性守卫才能拦住高亮渲染
                _pickWidgetController->setPickWidgetVisible(false);
                _pickWidgetController->setCloudChartMode(true);
                _pickWidgetController->displayData(displayData);
            }
            else
            {
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
            _pickWidgetController->setSelectSetBtnVisible(loadPtr->_defineType != 1);
        }

        if (loadPtr->_defineType == 1)
        {
            filterPicksToElementOnly();
        }

        // 缩放因子回填
        double scale = mapload.getScaleByCase({ MxFunctions::PhysicalFieldType::Physical_LoadBoundary,loadData->getLoadBoundaryId() }, _caseId);
        _scale.lineeditvalue->setExpression(MxFunctions::mNumberToString(scale));

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
        // 数据反显完成后刷新一次符号/方向箭头渲染
        // 直接调用时面板尚未完成布局/显示，渲染上下文可能未就绪，
        // 使用 QueuedConnection 延迟到事件循环下一阶段执行，确保首次进入面板也能正确渲染。
        QMetaObject::invokeMethod(this, &mRotatePhysicalPowerPanel::updateLoadSymbolRealtime, Qt::QueuedConnection);
    }

    QVector<QWidget*> mRotatePhysicalPowerPanel::getRefStateInputs(FoldSection* fathertar)
    {
        refstate_ = fathertar;
        QVector<QWidget*>wds;
        wds.append(_scale.propwid);
        return wds;
    }

    QVector<QWidget*> mRotatePhysicalPowerPanel::getTargetInputs(FoldSection* fathertar)
    {
        fatar_ = fathertar;
        QVector<QWidget*>wds;
        if (_pickWidgetController)
        {
            wds.append(_pickWidgetController->widget());
        }
        return wds;
    }

    QVector<QWidget*> mRotatePhysicalPowerPanel::getValueInputs(FoldSection* fatherval)
    {
        faval_ = fatherval;

        QVector<QWidget*>wds;

        wds.append(_rotateTypeCombo.propwid); // 类型

        int defineType = _defineTypeCombo.combobox->currentIndex();
        if (defineType == 0)
        {
            // 统一赋值：方向控件已含大小/转速 + 函数
            ensureDirectionWidget();
            if (_directionWidget)
            {
                wds.append(_directionWidget); // 方向（含大小/转速）
            }
            wds.append(_defineTypeCombo.propwid); // 定义方式
            wds.append(combofunc.propwid);
        }
        else
        {
            // 独立赋值：定义方式 + 表格按钮 + 函数（方向控件仅在统一赋值使用）
            wds.append(_defineTypeCombo.propwid); // 定义方式
            wds.append(_tableRowWidget);
            wds.append(combofunc.propwid);
            if (_directionWidget)
            {
                _directionWidget->hide();
            }
        }
        return wds;
    }

    int mRotatePhysicalPowerPanel::getFunc()
    {
        int funcid = -1;
        QString funcname = combofunc.combobox->currentText();
        if (_functable.find(funcname) != _functable.end())
        {
            funcid = _functable[funcname];
        }
        return funcid;
    }

    void mRotatePhysicalPowerPanel::applyAllowedPickTypes()
    {
        if (!_pickWidgetController) return;
        if (_defineTypeCombo.combobox->currentIndex() == 1)
        {
            // 独立赋值：仅设置白名单记录，不切换视口过滤器
            // （拾取过滤器由离散数据控件在开始拾取时自行激活/恢复）
            _pickWidgetController->setAllowedTypes(&pickOnlyElement);
            // 集合列表也同步限制为仅单元类型集合
            QHash<QString, QString> setToType = getSetNamesList({ SetType::Element });
            _pickWidgetController->setSelectSetList(setToType);
        }
        else
        {
            _pickWidgetController->setAllowedTypes(&pickGeoFaceSolidAndElement);
            // 集合列表同步为全部类型，便于后续统一调整
            QHash<QString, QString> setToType = getSetNamesList({ SetType::GeoFace, SetType::GeoSolid,SetType::Element });
            _pickWidgetController->setSelectSetList(setToType);
            _pickWidgetController->setPickType();
        }
    }

    void mRotatePhysicalPowerPanel::deactivatePickAndCommit()
    {
        // 面板为单例复用，切换载荷/面板时不会走析构/clearInputs，
        // 由统一退出出口（unEditLoad）调用，失活并提交当前拾取数据。
        const bool renderValid = (getCVM && getCVM->getPreRenderWidget());

        // 离散数据控件：结束拾取并关闭弹窗
        if (_discreteControl)
            _discreteControl->closePopup();

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

        // 方向选择控件内部包含多个子拾取控件，切出面板时同样要失活并清理高亮
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
    }

    void mRotatePhysicalPowerPanel::updateLoadSymbolRealtime()
    {
        if (!_pickWidgetController || !_currentLoad)
            return;
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;

        const int way = _defineTypeCombo.combobox->currentIndex();

        // 独立赋值：先按离散数据控件当前行同步 _elementCoorMap（数据源为表格，
        // 通用拾取控制器已不再接收视口拾取，不能再以它为准）
        if (way == 1)
        {
            updateOpenedTable();
        }

        auto meshData = getCVM->getModelData()->getMeshData();
        QVector<QVector3D> objs;

        // 集合展开与作用对象坐标计算仅统一赋值(0)需要；
        // 独立赋值(1)按表格数据逐单元取坐标，跳过该段全量计算。
        if (way == 0)
        {
            // 1. 汇总当前数据：拾取(pickedData) + 集合(selectedSetNames 按名称展开），合并为按类型分组的 map。
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
                    if (auto s = dynamic_cast<mNode*>(setBase)) { auto v = s->getNodeData(); auto& d = merged[PickDataType::Node]; d.insert(d.end(), v.begin(), v.end()); }
                    break;
                case MxFunctions::Element:
                    if (auto s = dynamic_cast<mElementSet*>(setBase)) { auto v = s->getElementSetData(); auto& d = merged[PickDataType::Element]; d.insert(d.end(), v.begin(), v.end()); }
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

            // 2. 按类型计算作用对象坐标。
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
        else // 独立赋值：逐单元取坐标（数据源为 _elementCoorMap，与离散表格行同步）
        {
            for (const auto& entry : _currentLoad->_elementCoorMap)
            {
                QVector3D pos = meshData->getVectorById(entry.first.result_id);
                if (!pos.isNull())
                    objs.push_back(pos);
            }
        }

        // 3. 方向：统一赋值从数据取方向；独立赋值不绘制整体方向符号
        QVector<std::pair<QVector3D, QVector3D>> pos_dir;

        if (way == 0 && _currentLoad && !_currentLoad->_directPoint2.isNull())
        {
            QVector3D dir = _currentLoad->_directPoint2 - _currentLoad->_startPoint1;
            if (!dir.isNull() && !objs.isEmpty())
            {
                dir.normalize();
                pos_dir.reserve(objs.size());
                for (const auto& o : objs)
                    pos_dir.push_back({ o, dir });
            }
        }

        // 4. 重画该载荷的旋转体力符号。
        AllRenderSymbolManager::getInstance()->assignLoad(
            _currentLoad, pos_dir, "Load_RotatePhysicalPower", QVector3D(133, 19, 33));

        // 5. 起点到方向点的方向箭头实时渲染
        updateDirectionArrowRender();

        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        if (symbolController) symbolController->UpdateAllSymbol();
        getCVM->getPreRenderWidget()->updatePaint();
    }


    void mRotatePhysicalPowerPanel::updateDirectionArrowRender()
    {
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;
        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        if (!symbolController)
            return;

        // 清理旧箭头
        symbolController->deleteSymbol({ s_dirArrowRenderKey });

        // 统一赋值且方向有效时绘制从原点指向方向点的集中力样式箭头
        const int way = _defineTypeCombo.combobox->currentIndex();
        if (way != 0 || !_directionWidget)
            return;

        QVariant dirVar = _directionWidget->getDirection();
        QVector3D dir = dirVar.isValid() && !dirVar.isNull() ? dirVar.value<QVector3D>() : QVector3D(1, 0, 0);
        if (dir.lengthSquared() < 1e-12f)
            return;

        // 方向箭头：尾部位于起始点，头部指向当前方向。
        // 起点使用持久化的旋转轴起始点 _startPoint1；
        // 方向直接从方向控件获取，避免 saveUniformValue 更新 _directPoint2 之前出现延迟一帧的问题。
        QVector<std::pair<QVector3D, QVector3D>> pos_dir;
        QVector3D startPoint = _currentLoad->_startPoint1;
        if (!dir.isNull())
        {
            pos_dir.push_back({ startPoint, dir });
        }

        const auto& arrow = symbolController->getOrCreateSymbolData<Arrow_Font_Load_Implicit>(s_dirArrowRenderKey);
        arrow->_pos_dir = pos_dir;
        arrow->_txt = "";
        arrow->_color = s_dirArrowColor;
        arrow->_depthState = false;
        arrow->_isFixedPixel = false;
        arrow->_size = 30.0;
        symbolController->createSymbol(arrow);

        symbolController->symbolSelfAdaptiveSize(getCVM->getPreRenderWidget(), { s_dirArrowRenderKey }, 0.05);
    }

    void mRotatePhysicalPowerPanel::slot_unitSystemResponse(UnitType type, QVector<int> idx)
    {
        if (!_directionWidget || !(type & SpeedUnit))
            return;
        if (_loadData)
            setInputsValue(_loadData);
    }
}