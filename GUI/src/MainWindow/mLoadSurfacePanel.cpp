#include "mLoadSurfacePanel.h"
#include "mLoadSurface.h"
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
#include "mGeoManager.h"
//#include "mGeometricFeatureAttribute.h" //【OCC摘除·S10/P0-1】退役头死 include 清剿（冻结岛保档，同主线处理）
#include "AllRenderSymbolManager.h"
#include "mGeneralPickUp.h"
#include "mGetSetTypeTemplate.h"
#include "mGetSetFunction.h"
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"
#include "mxr/label/mSymbolController.h"
#include "mxr/view/mShapeRender.h"
#include "mAppConfiguration.h"
#include "mDockLayoutManager.h"
#include "mGetSetTypeTemplate.h"

#include <mMixSet.h>
#include <mMeshData.h>
#include <mGeoSet.h>
#include <mNode.h>
#include <mElementSurface.h>
#include <mElementSet.h>
#include <QToolTip>
#include <QRegularExpression>
#include <QFileDialog>
#include <QSet>
#include "mAppConfiguration.h"
#include "mGetSetFunction.h"
#include "mOtherFunction.h"

using namespace MechCommon;
using namespace MechPreData;

namespace MechMainWindow
{
    mLoadSurfacePanel::mLoadSurfacePanel(QWidget* parent)
        :mLoadPanelBase(parent)
    {
        initInputs();
    }

    mLoadSurfacePanel::~mLoadSurfacePanel()
    {
        clearInputs();
    }

    void mLoadSurfacePanel::connectUnitSystemResponse()
    {
        connect(getCVM->getModelData()->getUnitSystemManager(), &mUnitSystemManager::sg_unitChanged,
            this, &mLoadSurfacePanel::slot_unitSystemResponse, Qt::UniqueConnection);
    }

    void mLoadSurfacePanel::disconnectUnitSystemResponse()
    {
        disconnect(getCVM->getModelData()->getUnitSystemManager(), &mUnitSystemManager::sg_unitChanged,
            this, &mLoadSurfacePanel::slot_unitSystemResponse);
    }

    std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> mLoadSurfacePanel::getPickedData()
    {
        if (!_pickWidgetController)
            return std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>();
        return _pickWidgetController->getAllData().pickedData;
    }

    void mLoadSurfacePanel::updateOpenedTable()
    {
        if (!_currentLoad || !_discreteControl)
            return;

        // 独立赋值模式下，surfaceForceMap_ 与离散数据控件的行数据保持严格一致：
        // 表格是数据的权威来源（含拾取追加与手动删除），此处按控件当前行重建 map。
        // 行 key 为业务 key（单元ID, 面序号），重建时优先保留 surfaceForceMap_ 中原有
        // 完整 key（含 part_id）与已填值；新行按单元 ID 反查 part_id 构造完整 key，
        // 与 mIMxFile 读入键规范 (cellId, partId, faceId) 保持一致。
        const auto rows = _discreteControl->rows();

        std::map<QPair<int, int>, RenderPickResultData> existingKeys;
        std::map<QPair<int, int>, QVector<QVariant>> existingValues;
        for (const auto& entry : _currentLoad->surfaceForceMap_)
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

            // 新增行：按单元 ID 反查 part_id 构造完整 key，4 列空值占位（压强值 + v1 + v2 + v3）
            auto partIt = partIdByCellId.constFind(cellId);
            if (partIt == partIdByCellId.constEnd())
            {
                const int partId = meshData ? meshData->getPartIdByEleId(cellId) : -1;
                partIt = partIdByCellId.insert(cellId, partId);
            }
            newMap[RenderPickResultData(cellId, partIt.value(), faceId)] = QVector<QVariant>(4);
        }

        _currentLoad->surfaceForceMap_ = std::move(newMap);
    }

    void mLoadSurfacePanel::filterPicksToElementSurfaceOnly()
    {
        if (!_pickWidgetController)
            return;

        // 同步通用拾取控件最新数据到 picks，避免过滤时丢失拾取/集合
        auto picks = _pickWidgetController->getAllData().pickedData;

        auto it = picks.begin();
        while (it != picks.end())
        {
            if (it->first != MxFunctions::PickDataType::ElementFace)
            {
                it = picks.erase(it);
            }
            else
            {
                ++it;
            }
        }

        // 独立赋值模式无需集合：把控件数据重设为"仅单元面拾取、无集合"
        _pickWidgetController->replacePickedDataAndClearSets(picks);
        // 彻底关闭通用拾取的高亮渲染（拾取由离散数据控件直接驱动视口选择容器）
        _pickWidgetController->setCloudChartMode(true);

        updateOpenedTable();
    }

    void mLoadSurfacePanel::slot_unitSystemResponse(UnitType type, QVector<int> idx)
    {
        Q_UNUSED(idx);
        if (!(type & UnitSystem)) return;
        if (_loadData)
            setInputsValue(_loadData);   //从载荷对象(基准值)重渲染，值+单位标签一次到位
    }

    void mLoadSurfacePanel::initInputs()
    {
        // 单位列表
        _pressureUnits = QStringList{
            "Pa",                // 1  m-kg-N-s
            "dyne/cm²",          // 2  cm-g-dyne-s
            "MPa",               // 3  mm-kg-N-s
            "MPa",               // 4  mm-t-N-s
            "MPa",               // 5  mm-dat-N-s
            "kPa",               // 6  vm-kg-vN-s
            "lbf/ft²",           // 7  ft-lbm-lbf-s
            "psi"                // 8  in-lbm-lbf-s
        };

        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString unit = _pressureUnits[sysunitidxs[0]];

        QStringList combonames = { "分布类型", "定义方式", "方向模式" };
        for (int i = 0; i < 3; ++i)
        {
            singleComboBox combobox;
            combobox.initWidget(combonames[i]);
            combobox.combobox->setFixedHeight(30);
            comboBoxs.append(combobox);
        }

        // 分布类型
        comboBoxs[0].combobox->addItem("均匀分布");
        connect(comboBoxs[0].combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mLoadSurfacePanel::slot_distributeTypeChanged);

        // 定义方式
        comboBoxs[1].combobox->addItems({ "统一赋值", "独立赋值" });
        connect(comboBoxs[1].combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mLoadSurfacePanel::slot_defineTypeChanged);

        // 方向模式
        comboBoxs[2].combobox->addItems({ "表面切向", "通用" });
        connect(comboBoxs[2].combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mLoadSurfacePanel::slot_dirModeChanged);

        // 统一赋值：载荷值输入框
        _loadValueInput.initWidget("载荷值", false);
        _loadValueInput.labelname->setText("载荷值");
        _loadValueInput.lineeditvalue->setUnit(unit);
        _loadValueInput.lineeditvalue->setFixedHeight(30);
        connect(_loadValueInput.lineeditvalue, &mCalcLineEdit::editingFinished,
            [this]() {
                saveLoadValue();
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
            this, &mLoadSurfacePanel::slot_funcChanged);
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
        // 面板侧在重建/反显 map 时按 surfaceForceMap_ 原有完整 key 回填 part_id
        faceSpec.keyOf = [](const RenderPickResultData& d) { return qMakePair(d.result_id, d.result_id2); };
        faceSpec.pickTypes = { MxFunctions::PickDataType::ElementFace };
        faceSpec.pickFilter = 7; // 视口拾取过滤器：仅单元面（与 activePickSig 索引一致）
        // 拾取由离散数据控件直接驱动视口选择容器，不再经过通用拾取控件
        _discreteControl = new mDiscreteDataControl(
            QStringLiteral("单元面数据"),
            { QStringLiteral("压强值"), QStringLiteral("v1"), QStringLiteral("v2"), QStringLiteral("v3") },
            &faceSpec, _tableRowWidget);
        _discreteControl->setRenderButtonVisible(false);
        connect(_discreteControl, &mDiscreteDataControl::dataChanged,
            this, &mLoadSurfacePanel::slot_discreteDataChanged);
        connect(_discreteControl, &mDiscreteDataControl::cellEdited,
            this, &mLoadSurfacePanel::slot_discreteCellEdited);
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

        // 通用拾取控件（替代原树形选择+按钮）
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
            // 拾取/集合变化时实时刷新表面载荷符号
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
            [this]() { saveScale(); });
    }

    void mLoadSurfacePanel::ensureDirectionWidget()
    {
        if (_directionWidget)
            return;

        if (!faval_)
            return;

        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString unit = _pressureUnits[sysunitidxs[0]];

        _directionWidget = new mDirectionSelectorWidget(
            QStringLiteral("参考方向"), false, QString(), false, QString(), false, faval_->contentWidget());
        _directionWidget->setVisible(false);

        // 使用控件默认的方向/坐标计算器
        _directionWidget->setDirectionCalculator(
            mDirectionSelectorWidget::DirectionCalculator());
        _directionWidget->setCoordinateCalculator(
            mDirectionSelectorWidget::CoordinateCalculator());

        connect(_directionWidget, &mDirectionSelectorWidget::directionChanged, this, [this]() {
            updateLoadSymbolRealtime();
            saveLoadValue();
            });
        connect(_directionWidget, &mDirectionSelectorWidget::layoutRefreshRequested, this, [this]() {
            if (faval_)
            {
                faval_->refreshLayout();
            }
            });
        connect(_directionWidget, &mDirectionSelectorWidget::currentWayNoPickWidget, this, [this]() {
            // 方向控件当前方式页无拾取控件时，激活表面载荷面板中外层拾取控件
            if (_pickWidgetController)
            {
                _pickWidgetController->setPickActive(true);
            }
            });
    }

    void mLoadSurfacePanel::saveLoadValue()
    {
        if (!_currentLoad)
            return;

        QVector<int> curidx = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, curidx, { 3,0,0,0 });

        // 载荷值
        QString valStr = _loadValueInput.lineeditvalue->text().trimmed();
        if (!valStr.isEmpty())
        {
            _currentLoad->loadValue_ = QVariant::fromValue<double>(valStr.toDouble() * ratio);
        }
        else
        {
            _currentLoad->loadValue_ = QVariant();
        }

        // 参考方向/载荷值变化时实时刷新符号
        applyDirectionWidgetToLoad(_currentLoad);
        updateLoadSymbolRealtime();
        notifyLoadChanged();
    }

    void mLoadSurfacePanel::applyDirectionWidgetToLoad(std::shared_ptr<MechCaseData::mLoadSurface> load)
    {
        if (!load || !_directionWidget)
        {
            return;
        }

        // 保存方向矢量
        QVariant dirVar = _directionWidget->getDirection();
        if (dirVar.isValid() && !dirVar.isNull())
        {
            QVector3D dir = dirVar.value<QVector3D>();
            if (!dir.isNull())
            {
                dir.normalize();
            }
        }

        // 保存方向控件完整 UI 状态
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

    void mLoadSurfacePanel::restoreDirectionWidgetFromLoad(std::shared_ptr<MechCaseData::mLoadSurface> load)
    {
        if (!load || !_directionWidget)
        {
            return;
        }

        const auto& state = load->dirPickState_;

        // 先阻断 directionChanged 信号，避免反显过程中触发保存
        bool oldSignalsBlocked = _directionWidget->blockSignals(true);

        _directionWidget->resetToDefault();

        int mode = state.mode;
        bool reversed = state.reversed;

        // 如果 state.mode 为 0（DefaultDirection，未设置过），则根据 directInputDir 推断一个合理的方向控件模式
        if (mode == mDirectionSelectorWidget::DefaultDirection && !state.directInputDir.isNull())
        {
            QVector3D dir = state.directInputDir;
            dir.normalize();
            QVector3D absDir(qAbs(dir.x()), qAbs(dir.y()), qAbs(dir.z()));
            if (absDir.x() >= absDir.y() && absDir.x() >= absDir.z())
            {
                mode = mDirectionSelectorWidget::XAxis;
                reversed = (dir.x() < 0);
            }
            else if (absDir.y() >= absDir.x() && absDir.y() >= absDir.z())
            {
                mode = mDirectionSelectorWidget::YAxis;
                reversed = (dir.y() < 0);
            }
            else if (absDir.z() >= absDir.x() && absDir.z() >= absDir.y())
            {
                mode = mDirectionSelectorWidget::ZAxis;
                reversed = (dir.z() < 0);
            }
            else
            {
                mode = mDirectionSelectorWidget::DirectInput;
            }
        }

        _directionWidget->setDirectionMode(static_cast<mDirectionSelectorWidget::DirectionMode>(mode));
        _directionWidget->setReversed(reversed);

        if (mode == mDirectionSelectorWidget::CustomCoordinate &&
            state.startPoint.isValid() && !state.startPoint.isNull() &&
            state.endPoint.isValid() && !state.endPoint.isNull())
        {
            _directionWidget->setCustomCoordinatePoints(
                state.startPoint.value<QVector3D>(),
                state.endPoint.value<QVector3D>());
        }
        else if (mode == mDirectionSelectorWidget::SelectObject)
        {
            PickWidgetData displayData;
            if (state.selectedObject.second.result_id > 0)
            {
                displayData.pickedData[state.selectedObject.first].push_back(state.selectedObject.second);
            }
            _directionWidget->objectPickController()->displayData(displayData);
        }
        else if (mode == mDirectionSelectorWidget::DirectInput)
        {
            if (!state.directInputDir.isNull())
                _directionWidget->setDirection(state.directInputDir);
            else
                _directionWidget->setDirection(QVector3D(1, 0, 0));
        }

        _directionWidget->blockSignals(oldSignalsBlocked);
    }

    void mLoadSurfacePanel::saveDirMode()
    {
        if (!_currentLoad)
            return;
        _currentLoad->dirMode_ = comboBoxs[2].combobox->currentIndex();
        notifyLoadChanged();
    }

    void mLoadSurfacePanel::saveFunc()
    {
        if (!_currentLoad)
            return;

        int funcid = getFunc();
        auto& mapload = getCVM->getModelData()->getMapRelationManager();
        mapload.appendMapRelation({ Physical_LoadBoundary, _loadData->getLoadBoundaryId() }, { Physical_Function, funcid });

        notifyLoadChanged();
    }

    void mLoadSurfacePanel::saveScale()
    {
        if (!_currentLoad)
            return;

        double scale = _scale.lineeditvalue->text().trimmed().toDouble();
        auto& mapload = getCVM->getModelData()->getMapRelationManager();
        mapload.setScaleByCase({ MxFunctions::PhysicalFieldType::Physical_LoadBoundary, _loadData->getLoadBoundaryId() }, _caseId, scale);

        notifyLoadChanged();
    }

    void mLoadSurfacePanel::notifyLoadChanged()
    {
        if (!_loadData)
            return;
        // 仅通知外部刷新树/渲染，不再调用 storeLoadValue 全量保存
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_LoadBoundary, _loadData->getLoadBoundaryId());
    }

    void mLoadSurfacePanel::slot_distributeTypeChanged(int idx)
    {
        if (_currentLoad)
            _currentLoad->distributeType_ = idx;
        // 分布类型目前只有一项，切换不涉及其它逻辑
    }

    void mLoadSurfacePanel::slot_defineTypeChanged(int idx)
    {
        if (_currentLoad)
            _currentLoad->way_ = idx;

        // 切换定义方式时关闭离散数据弹窗并结束拾取，避免旧模式弹窗残留
        if (_discreteControl)
        {
            _discreteControl->closePopup();
        }

        if (idx == 1) // 独立赋值
        {
            // 独立赋值：过滤拾取为仅单元面（内部会关闭通用拾取的高亮渲染），
            // 并隐藏通用拾取控件（拾取由离散数据控件直接驱动视口选择容器）
            filterPicksToElementSurfaceOnly();
            if (_pickWidgetController)
            {
                _pickWidgetController->setPickWidgetVisible(false);
            }
        }
        else if (_pickWidgetController && idx != -1)
        {
            // 统一赋值：恢复通用拾取控件可见/激活与正常高亮渲染
            _pickWidgetController->setPickWidgetVisible(true);
            _pickWidgetController->setCloudChartMode(false);
            _pickWidgetController->setPickActive(true);
        }

        // 独立赋值切换前需先隐藏并清理方向控件内部拾取状态，避免与外层拾取控件互斥冲突。
        if (_directionWidget)
        {
            if (idx == 1)
            {
                _directionWidget->setPickWidgetVisible(false);
                _directionWidget->clearAllHighlight();
            }
            else
            {
                _directionWidget->setPickWidgetVisible(true);
            }
        }

        applyAllowedPickTypes(idx);
        rebuildValueLayout();
        // 定义方式切换影响"作用对象"折叠框显隐（独立赋值不显示），需重建外层面板
        emit sg_rebuildPanel();
    }

    void mLoadSurfacePanel::rebuildValueLayout()
    {
        if (!faval_)
            return;

        faval_->removeAll();
        faval_->addRow(comboBoxs[0].propwid);
        comboBoxs[0].propwid->show();
        faval_->addRow(comboBoxs[1].propwid);
        comboBoxs[1].propwid->show();

        int defineType = comboBoxs[1].combobox->currentIndex();

        if (defineType == 0) // 统一赋值
        {
            faval_->addRow(comboBoxs[2].propwid);
            comboBoxs[2].propwid->show();
            ensureDirectionWidget();
            if (_directionWidget)
            {
                _directionWidget->hide();
                faval_->addRow(_directionWidget);
                _directionWidget->show();
                _directionWidget->setPickWidgetVisible(true);
            }
            faval_->addRow(_loadValueInput.propwid);
            _loadValueInput.propwid->show();
            faval_->addRow(combofunc.propwid);
            combofunc.propwid->show();
        }
        else // 独立赋值
        {
            faval_->addRow(comboBoxs[2].propwid);
            comboBoxs[2].propwid->show();
            faval_->addRow(_tableRowWidget);
            _tableRowWidget->show();
            faval_->addRow(combofunc.propwid);
            combofunc.propwid->show();
        }

        faval_->refreshLayout();

        // 独立赋值模式：隐藏集合选择按钮，强制仅单元面
        if (defineType == 1)
        {
            if (_pickWidgetController)
            {
                _pickWidgetController->setSelectSetBtnVisible(false);
            }
            // 切换到独立赋值后立即按表格 v1v2v3 刷新符号，避免残留统一赋值的箭头
            updateLoadSymbolRealtime();
        }
        else
        {
            if (_pickWidgetController)
            {
                _pickWidgetController->setSelectSetBtnVisible(true);
            }
            updateLoadSymbolRealtime();
        }
    }

    void mLoadSurfacePanel::slot_dirModeChanged(int idx)
    {
        if (_currentLoad)
            _currentLoad->dirMode_ = idx;
        saveDirMode();
    }

    void mLoadSurfacePanel::slot_funcChanged(int idx)
    {
        Q_UNUSED(idx)
            saveFunc();
    }

    void mLoadSurfacePanel::slot_discreteDataChanged()
    {
        // 离散数据控件行变化（拾取追加/删除/清空）：
        // 按控件当前行重建 surfaceForceMap_，随后刷新实时符号并通知外部。
        // 拾取会话进行中跳过：确认拾取（完成拾取）后控件会补发一次 dataChanged，
        // 此时才执行重建/渲染，避免拾取界面期间反复重绘。
        if (!_currentLoad || (_discreteControl && _discreteControl->isPicking()))
            return;
        updateOpenedTable();
        updateLoadSymbolRealtime();
        notifyLoadChanged();
    }

    void mLoadSurfacePanel::slot_discreteCellEdited(const QPair<int, int>& key, int valueIndex, const QString& text)
    {
        if (!_currentLoad || key.first <= 0)
            return;

        // 行 key 只有（单元ID, 面序号）两个槽位，按业务 key 在 surfaceForceMap_ 中查找
        // 已有完整 key（保留原 part_id）；不存在时按单元 ID 反查 part_id 构造完整 key，
        // 与 mIMxFile 读入键规范 (cellId, partId, faceId) 保持一致。
        auto it = _currentLoad->surfaceForceMap_.begin();
        for (; it != _currentLoad->surfaceForceMap_.end(); ++it)
        {
            if (it->first.result_id == key.first && it->first.result_id2 == key.second)
                break;
        }

        RenderPickResultData loadKey;
        if (it != _currentLoad->surfaceForceMap_.end())
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
        // 清空分量时保留 key，值置空 QVariant（行仍被记录，写文件时空值保留逗号占位）
        QVector<QVariant>& values = _currentLoad->surfaceForceMap_[loadKey];
        if (values.size() < 4)
            values.resize(4);
        if (valueIndex >= 0 && valueIndex < 4)
            values[valueIndex] = value;

        updateLoadSymbolRealtime();
        notifyLoadChanged();
    }

    void mLoadSurfacePanel::applyAllowedPickTypes(int defineType)
    {
        if (!_pickWidgetController) return;

        if (defineType == 1)
        {
            // 独立赋值：仅设置白名单记录，不切换视口过滤器
            // （拾取过滤器由离散数据控件在开始拾取时自行激活/恢复）
            _pickWidgetController->setAllowedTypes(&pickOnlyElementSurface);
        }
        else
        {
            // 统一赋值：几何面 + 单元面
            _pickWidgetController->setAllowedTypes(&pickGeoFaceAndElementSurface);
            _pickWidgetController->setPickType();
        }
    }

    void mLoadSurfacePanel::deactivatePickAndCommit()
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

    void mLoadSurfacePanel::clearInputs()
    {
        _loadValueInput.clear();

        if (_directionWidget)
        {
            _directionWidget->hide();
            delete _directionWidget;
            _directionWidget = nullptr;
        }

        if (_pickWidgetController)
        {
            _pickWidgetController->setPickActive(false);
            delete _pickWidgetController;
            _pickWidgetController = nullptr;
        }

        for (auto combo : comboBoxs) combo.clear();
        comboBoxs.clear();
        combofunc.clear();
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

    void mLoadSurfacePanel::clearData()
    {
        _loadValueInput.lineeditvalue->setExpression("");
        if (_directionWidget)
        {
            _directionWidget->resetToDefault();
        }
    }

    void mLoadSurfacePanel::setInputsValue(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData)
    {
        std::shared_ptr<MechCaseData::mLoadSurface> surfPtr = std::dynamic_pointer_cast<MechCaseData::mLoadSurface>(loadData);
        _currentLoad = surfPtr;
        if (!surfPtr) return;

        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Stress, { 3,0,0,0 }, sysunitidxs);
        QString unit = _pressureUnits[sysunitidxs[0]];

        // 载荷值
        _loadValueInput.lineeditvalue->setUnit(unit);
        _loadValueInput.lineeditvalue->clear();
        if (surfPtr->loadValue_.isValid() && !surfPtr->loadValue_.isNull())
        {
            double val = surfPtr->loadValue_.toDouble() * ratio;
            _loadValueInput.lineeditvalue->setExpression(QString::number(val));
        }

        // 下拉框
        comboBoxs[0].combobox->blockSignals(true);
        comboBoxs[0].combobox->setCurrentIndex(surfPtr->distributeType_);
        comboBoxs[0].combobox->blockSignals(false);

        comboBoxs[1].combobox->blockSignals(true);
        comboBoxs[1].combobox->setCurrentIndex(surfPtr->way_);
        comboBoxs[1].combobox->blockSignals(false);

        comboBoxs[2].combobox->blockSignals(true);
        comboBoxs[2].combobox->setCurrentIndex(surfPtr->dirMode_);
        comboBoxs[2].combobox->blockSignals(false);

        // 幅值曲线刷新
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

        // 方向控件反显（统一赋值模式）
        ensureDirectionWidget();
        if (_directionWidget)
        {
            restoreDirectionWidgetFromLoad(surfPtr);
        }

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

            applyAllowedPickTypes(surfPtr->way_);
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

        // 独立赋值模式且没有集合映射时，直接从数据类生成拾取数据
        if (surfPtr->way_ == 1 && displayData.pickedData.empty() && displayData.selectedSetNames.isEmpty())
        {
            std::vector<RenderPickResultData> facePicks;
            for (const auto& entry : surfPtr->surfaceForceMap_)
            {
                facePicks.push_back(entry.first);
            }
            if (!facePicks.empty())
            {
                displayData.pickedData[MxFunctions::PickDataType::ElementFace] = std::move(facePicks);
            }

            // 反显到离散数据控件（单元面 + 已填分量），行 key 为业务 key（单元ID, 面序号）
            if (_discreteControl)
            {
                QVector<mDiscreteDataControl::Row> rows;
                rows.reserve(static_cast<int>(surfPtr->surfaceForceMap_.size()));
                for (const auto& entry : surfPtr->surfaceForceMap_)
                {
                    QStringList values;
                    for (int i = 0; i < 4; ++i)
                    {
                        const QVariant v = i < entry.second.size() ? entry.second.at(i) : QVariant();
                        values << ((v.isValid() && !v.isNull()) ? QString::number(v.toDouble()) : QString());
                    }
                    rows.append(qMakePair(qMakePair(entry.first.result_id, entry.first.result_id2), values));
                }
                _discreteControl->setRows(rows);
            }
        }

        if (_pickWidgetController)
        {
            if (surfPtr->way_ == 1)
            {
                // 独立赋值已剥离通用拾取：仅写入数据（供 pickState/持久化一致性），
                // 全程保持失活，不激活、不渲染任何拾取高亮
                _pickWidgetController->setPickWidgetVisible(false);
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
            _pickWidgetController->setSelectSetBtnVisible(surfPtr->way_ != 1);
        }

        if (surfPtr->way_ == 1)
        {
            filterPicksToElementSurfaceOnly();
        }

        // 缩放因子回填
        double scale = mapload.getScaleByCase({ MxFunctions::PhysicalFieldType::Physical_LoadBoundary, loadData->getLoadBoundaryId() }, _caseId);
        _scale.lineeditvalue->setExpression(MxFunctions::mNumberToString(scale));

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
        QMetaObject::invokeMethod(this, &mLoadSurfacePanel::updateLoadSymbolRealtime, Qt::QueuedConnection);
    }

    QVector<QWidget*> mLoadSurfacePanel::getRefStateInputs(FoldSection* fathertar)
    {
        refstate_ = fathertar;
        QVector<QWidget*> wds;
        wds.append(_scale.propwid);
        return wds;
    }

    QVector<QWidget*> mLoadSurfacePanel::getTargetInputs(FoldSection* fathertar)
    {
        fatar_ = fathertar;
        QVector<QWidget*> wds;
        if (_pickWidgetController)
        {
            wds.append(_pickWidgetController->widget());
        }
        return wds;
    }

    QVector<QWidget*> mLoadSurfacePanel::getValueInputs(FoldSection* fatherval)
    {
        faval_ = fatherval;
        QVector<QWidget*> wds;

        wds.append(comboBoxs[0].propwid);
        wds.append(comboBoxs[1].propwid);

        int defineType = comboBoxs[1].combobox->currentIndex();

        if (defineType == 0)
        {
            wds.append(comboBoxs[2].propwid);
            ensureDirectionWidget();
            if (_directionWidget)
            {
                wds.append(_directionWidget);
            }
            wds.append(_loadValueInput.propwid);
            wds.append(combofunc.propwid);
        }
        else
        {
            wds.append(comboBoxs[2].propwid);
            wds.append(_tableRowWidget);
            wds.append(combofunc.propwid);
        }

        return wds;
    }

    int mLoadSurfacePanel::getFunc()
    {
        int funcid = -1;
        QString funcname = combofunc.combobox->currentText();
        if (_functable.find(funcname) != _functable.end())
        {
            funcid = _functable[funcname];
        }
        return funcid;
    }

    void mLoadSurfacePanel::updateLoadSymbolRealtime()
    {
        if (!_pickWidgetController || !_currentLoad)
            return;
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;

        const int defineType = _currentLoad->way_;

        // 独立赋值：先按离散数据控件当前行同步 surfaceForceMap_（数据源为表格，
        // 通用拾取控制器已不再接收视口拾取，不能再以它为准），再合成方向。
        if (defineType == 1)
        {
            updateOpenedTable();
        }

        // 1. 汇总当前数据：拾取(pickedData) + 集合(selectedSetNames 按名称展开），合并为按类型分组的 map。
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

        QVector<std::pair<QVector3D, QVector3D>> pos_dir;

        // 统一赋值：渲染方向与载荷值符号关联，载荷值为负时取反；无值/非数字默认正方向
        bool negativeLoad = false;
        if (defineType == 0)
        {
            bool valOk = false;
            double val = _loadValueInput.lineeditvalue->text().trimmed().toDouble(&valOk);
            negativeLoad = (valOk && val < 0);
        }

        // 独立赋值：已剥离通用拾取，merged 为空；
        // 符号位置/方向直接遍历 surfaceForceMap_（表格为权威数据源），按表格 v1v2v3 合成方向。
        if (defineType == 1)
        {
            for (const auto& entry : _currentLoad->surfaceForceMap_)
            {
                QVector3D centerPos;
                QVector3D normal;
                meshData->GetOutVectorByMeshFaceID(entry.first, normal, centerPos);
                if (centerPos.isNull()) continue;

                // 方向：v1/v2/v3 任一有效即可构成方向；无效分量以 0 补齐
                QVector3D dir;
                const QVector<QVariant>& vals = entry.second;
                if (vals.size() >= 4)
                {
                    bool ok1 = false, ok2 = false, ok3 = false;
                    float v1 = vals[1].toFloat(&ok1);
                    float v2 = vals[2].toFloat(&ok2);
                    float v3 = vals[3].toFloat(&ok3);
                    if (ok1 || ok2 || ok3)
                    {
                        dir = QVector3D(
                            ok1 ? v1 : 0.0f,
                            ok2 ? v2 : 0.0f,
                            ok3 ? v3 : 0.0f);
                        // 该面压强值为负时方向取反；无值默认正方向
                        if (!dir.isNull())
                        {
                            bool ok0 = false;
                            float p = vals[0].toFloat(&ok0);
                            if (ok0 && p < 0)
                            {
                                dir = -dir;
                            }
                        }
                    }
                }

                if (dir.isNull()) continue;
                pos_dir.push_back({ centerPos, dir });
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
                    QVector3D normal;
                    meshData->GetOutVectorByMeshFaceID(
                        RenderPickResultData(d.result_id, d.part_id, d.result_id2),
                        normal, centerPos);
                    if (centerPos.isNull()) continue;

                    // 统一赋值使用方向控件（独立赋值已在前面的分支单独处理）
                    QVector3D dir;
                    if (_directionWidget)
                    {
                        QVariant dirVar = _directionWidget->getDirection();
                        if (dirVar.isValid() && !dirVar.isNull())
                        {
                            dir = dirVar.value<QVector3D>();
                        }
                    }
                    // 载荷值为负时方向取反
                    if (negativeLoad && !dir.isNull())
                    {
                        dir = -dir;
                    }

                    if (dir.isNull()) continue;
                    pos_dir.push_back({ centerPos, dir });
                }
                break;
            }
            case MxFunctions::PickDataType::GeoFace:
            {
                // 统一赋值才走到这里（独立赋值已剥离通用拾取，merged 为空且上面单独分支处理）
                std::vector<int> ids;
                ids.reserve(pair.second.size());
                for (const auto& d : pair.second)
                    ids.push_back(d.result_id);

                auto points = geoManager->CalculateFaceAttr(ids);
                auto coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(2, QVector<int>::fromStdVector(ids));
                for (int i = 0; i < coords.size() && i < points.size(); ++i)
                {
                    QVector3D dir;
                    if (_directionWidget)
                    {
                        QVariant dirVar = _directionWidget->getDirection();
                        if (dirVar.isValid() && !dirVar.isNull())
                        {
                            dir = dirVar.value<QVector3D>();
                        }
                    }
                    // 载荷值为负时方向取反
                    if (negativeLoad && !dir.isNull())
                    {
                        dir = -dir;
                    }

                    if (dir.isNull()) continue;
                    pos_dir.push_back({ coords[i], dir });
                }
                break;
            }
            default:
                break;
            }
            }
        } // end else (defineType != 1)

        // 3. 重画该载荷的表面载荷符号。
        AllRenderSymbolManager::getInstance()->assignLoad(
            _currentLoad, pos_dir, "Load_Surface", QVector3D(60.0f, 180.0f, 220.0f));

        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        if (symbolController) symbolController->UpdateAllSymbol();
        getCVM->getPreRenderWidget()->updatePaint();
    }
}
