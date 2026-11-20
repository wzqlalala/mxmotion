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
#include "mGeneralPickUp.h"
#include "mGetSetTypeTemplate.h"
#include "AllRenderSymbolManager.h"

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

#include "mLoadGravityPanel.h"
#include "mDirectionSelectorWidget.h"
#include "mOtherFunction.h"

using namespace MechCommon;
using namespace MechPreData;
namespace MechMainWindow
{
    mLoadGravityPanel::mLoadGravityPanel(QWidget* parent)
        :mLoadPanelBase(parent)
    {
        initInputs();
    }

    mLoadGravityPanel::~mLoadGravityPanel()
    {
        clearInputs();
    }

    void mLoadGravityPanel::initInputs()
    {
        // ========== 各向分量输入框初始化 ==========
        QStringList inpnames = {
            "V1",
            "V2",
            "V3",
        };
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString unit = UnitSystems[sysunitidxs[0]][6];
        for (int i = 0; i < 3; ++i)
        {
            InputWidgetSet inp;
            inp.initWidget(inpnames[i], false);
            inp.lineeditvalue->setUnit(unit);
            inp.labelname->setText(inpnames[i]);
            inp.lineeditvalue->setFixedHeight(30);
            inputWidgets.append(inp);
            connect(inp.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this]() {
                    saveGravityComponents();
                });

        }

        // 定义方式下拉框
        singleComboBox defineType;
        defineType.initWidget("定义方式");
        defineType.combobox->setFixedHeight(30);
        defineType.combobox->addItems({ "各向分量", "方向与大小" });
        connect(defineType.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mLoadGravityPanel::slot_defineTypeChanged);
        comboBoxs.append(defineType);

        // 缩放因子
        _scale.initWidget("缩放因子", false);
        _scale.labelname->setText("缩放因子");
        connect(_scale.lineeditvalue, &mCalcLineEdit::editingFinished,
            [this]() {
                saveScale();
            });

        // 通用拾取控件（作用对象）
        _pickWidgetController = mGeneralPickUp::createPickWidget(CBBlue);
        connect(_pickWidgetController, &mPickWidgetController::pickActiveChanged, this, [this](bool isActive) {
            if (isActive)
            {
                emit mMechGlobalSignals::getInstance()->activePickSig(5);
            }
            });
        connect(_pickWidgetController, &mPickWidgetController::dataChanged, this, [this]() {
            // 拾取变化时仅通知外部刷新，不触发全量保存
            notifyLoadChanged();
            });

        _pickWidgetController->setSetResolver(&resolveSetToTypedData);
        applyAllowedPickTypes();
        _pickWidgetController->setDefultTip("选择对象");

        // 默认禁用拾取控件，只有在局部施加折叠框激活时才启用
        if (_pickWidgetController)
        {
            _pickWidgetController->setAllPickTypesDisabled(true);
        }
    }

    void mLoadGravityPanel::ensureDirectionWidget()
    {
        if (_directionWidget)
            return;

        // 必须在已知父折叠块时创建，避免无 parent 的独立窗口短暂闪现。
        if (!faval_)
            return;

        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        // 重力加速度大小单位：长度/时间^2，这里沿用原 UnitSystems[...][6] 的加速度单位显示。
        QString unit = UnitSystems[sysunitidxs[0]][6];
        _directionWidget = new mDirectionSelectorWidget(
            QStringLiteral("方向"), true, QStringLiteral("重力大小"), false,
            unit, false, faval_->contentWidget());
        _directionWidget->setVisible(false);

        // 使用控件默认的方向/坐标计算器（基于几何属性）
        _directionWidget->setDirectionCalculator(
            mDirectionSelectorWidget::DirectionCalculator());
        _directionWidget->setCoordinateCalculator(
            mDirectionSelectorWidget::CoordinateCalculator());

        connect(_directionWidget, &mDirectionSelectorWidget::directionChanged, this, [this]() {
            saveDirectionAndMagnitude();
            });
        connect(_directionWidget, &mDirectionSelectorWidget::magnitudeChanged, this, [this]() {
            saveDirectionAndMagnitude();
            });
        connect(_directionWidget, &mDirectionSelectorWidget::layoutRefreshRequested, this, [this]() {
            if (faval_)
            {
                faval_->refreshLayout();
            }
            });
        connect(_directionWidget, &mDirectionSelectorWidget::currentWayNoPickWidget, this, [this]() {
            // 方向控件当前方式页无拾取控件时，激活重力面板中控件
            if (_pickWidgetController && _pickWidgetEnabled)
            {
                _pickWidgetController->setPickActive(true);
            }
            });
    }

    void mLoadGravityPanel::slot_defineTypeChanged(int idx)
    {
        // 切换前移除方向控件
        if (_directionWidget && faval_ && faval_->layout())
        {
            _directionWidget->setParent(nullptr);
            faval_->layout()->removeWidget(_directionWidget);
        }

        if (faval_)
        {
            faval_->removeAll();
            faval_->addRow(comboBoxs[0].propwid);
            comboBoxs[0].propwid->show();
        }

        saveWayAndClearModeFields();

        if (idx == 0) // 各向分量：XYZ输入框
        {
            if (faval_)
            {
                faval_->addRow(inputWidgets[0].propwid);
                inputWidgets[0].propwid->show();
                faval_->addRow(inputWidgets[1].propwid);
                inputWidgets[1].propwid->show();
                faval_->addRow(inputWidgets[2].propwid);
                inputWidgets[2].propwid->show();
            }
        }
        else if (idx == 1) // 方向与大小：方向控件
        {
            ensureDirectionWidget();
            if (_directionWidget && faval_)
            {
                _directionWidget->hide();
                faval_->addRow(_directionWidget);
                _directionWidget->show();
                _directionWidget->setPickWidgetVisible(true);
            }
        }

        if (faval_)
        {
            faval_->refreshLayout();
        }
    }

    void mLoadGravityPanel::saveWayAndClearModeFields()
    {
        if (!_currentLoad)
        {
            return;
        }
        _currentLoad->way_ = comboBoxs[0].combobox->currentIndex();
        // 重置所有模式字段，避免交叉污染
        _currentLoad->magnitude_ = QVariant();
        _currentLoad->dir_ = QVariant();
        _currentLoad->V1 = QVariant();
        _currentLoad->V2 = QVariant();
        _currentLoad->V3 = QVariant();
        _currentLoad->dirPickState_ = DirectionPickState();
    }

    void mLoadGravityPanel::applyDirectionWidgetToLoad(std::shared_ptr<MechCaseData::mLoadGravity> load)
    {
        if (!load || !_directionWidget)
        {
            return;
        }

        QVector<int> curidx = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Length, curidx, { 3,0,0,0 });

        // 存大小
        QString magstr = _directionWidget->magnitudeValue();
        if (!magstr.isEmpty())
        {
            bool ok = false;
            double m = magstr.trimmed().toDouble(&ok);
            if (ok)
            {
                load->magnitude_ = m * ratio;
            }
            else
            {
                load->magnitude_ = QVariant();
            }
        }
        else
        {
            load->magnitude_ = QVariant();
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
                double m = load->magnitude_.isValid() ? load->magnitude_.toDouble() : 0.0;
                load->V1 = m * dir.x();
                load->V2 = m * dir.y();
                load->V3 = m * dir.z();
            }
            else
            {
                load->dir_ = QVariant();
                load->V1 = QVariant();
                load->V2 = QVariant();
                load->V3 = QVariant();
            }
        }
        else
        {
            load->dir_ = QVariant();
            load->V1 = QVariant();
            load->V2 = QVariant();
            load->V3 = QVariant();
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

    void mLoadGravityPanel::saveDirectionAndMagnitude()
    {
        if (!_currentLoad || !_directionWidget)
        {
            return;
        }

        applyDirectionWidgetToLoad(_currentLoad);
        notifyLoadChanged();
    }

    void mLoadGravityPanel::setInputsValue(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData)
    {
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Length, { 3,0,0,0 }, sysunitidxs);
        QString unit = UnitSystems[sysunitidxs[0]][6];
        std::shared_ptr<MechCaseData::mLoadGravity> gravityPtr = std::dynamic_pointer_cast<MechCaseData::mLoadGravity>(loadData);
        if (!gravityPtr) return;
        _currentLoad = gravityPtr;

        if (_directionWidget)
        {
            _directionWidget->resetToDefault();
        }

        // 定义方式反显
        comboBoxs[0].combobox->blockSignals(true);
        comboBoxs[0].combobox->setCurrentIndex(gravityPtr->way_);
        comboBoxs[0].combobox->blockSignals(false);

        // 缩放因子
        auto& mapload = getCVM->getModelData()->getMapRelationManager();
        double scale = mapload.getScaleByCase({ MxFunctions::PhysicalFieldType::Physical_LoadBoundary,loadData->getLoadBoundaryId() }, _caseId);
        _scale.lineeditvalue->setExpression(MxFunctions::mNumberToString(scale));

        // 各向分量输入框：先清空
        for (auto inp : inputWidgets)
        {
            inp.lineeditvalue->setUnit(unit);
            inp.lineeditvalue->setExpression("");
        }

        // 方向控件单位
        if (_directionWidget)
        {
            _directionWidget->setMagnitudeUnit(unit);
        }

        // 分模式反显
        if (gravityPtr->way_ == 0) // 各向分量：反显XYZ
        {
            QVariant v1data = gravityPtr->V1;
            if (!v1data.isNull() && v1data.isValid())
            {
                inputWidgets[0].lineeditvalue->setExpression(QString::number(v1data.toDouble() * ratio));
            }
            QVariant v2data = gravityPtr->V2;
            if (!v2data.isNull() && v2data.isValid())
            {
                inputWidgets[1].lineeditvalue->setExpression(QString::number(v2data.toDouble() * ratio));
            }
            QVariant v3data = gravityPtr->V3;
            if (!v3data.isNull() && v3data.isValid())
            {
                inputWidgets[2].lineeditvalue->setExpression(QString::number(v3data.toDouble() * ratio));
            }
        }
        else if (gravityPtr->way_ == 1) // 方向与大小：用结构体反显方向控件
        {
            ensureDirectionWidget();
            if (_directionWidget)
            {
                const auto& state = gravityPtr->dirPickState_;

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
                else if (gravityPtr->dir_.isValid() && !gravityPtr->dir_.isNull())
                {
                    _directionWidget->setDirection(gravityPtr->dir_.value<QVector3D>());
                }

                // 加速度大小反显
                QVariant magdata = gravityPtr->magnitude_;
                if (!magdata.isNull() && magdata.isValid())
                {
                    _directionWidget->setMagnitudeValue(QString::number(magdata.toDouble() * ratio));
                }
                else
                {
                    _directionWidget->setMagnitudeValue("");
                }

                _directionWidget->blockSignals(oldSignalsBlocked);
            }
        }

        // 持久化策略绑定
        if (_pickWidgetController)
        {
            auto strategy = std::make_shared<MapRelationPersistenceStrategy>(
                MxFunctions::Physical_LoadBoundary,
                _loadData->getLoadBoundaryId());
            _pickWidgetController->setPersistenceStrategy(strategy);
            _pickWidgetController->setSetResolver(&resolveSetToTypedData);

            QHash<QString, QString> setToType = getSetNamesList({ SetType::GeoFace, SetType::GeoSolid, SetType::Element });
            _pickWidgetController->setSelectSetList(setToType);

            // 根据局部施加状态决定拾取是否可用
            setPickEnabled(gravityPtr->isCornerAddChecked);
        }

        // 作用对象反显
        PickWidgetData displayData;
        int setid = mapload.getMapRelation<int>({ Physical_LoadBoundary,loadData->getLoadBoundaryId() }, Physical_Set);
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
            _pickWidgetController->displayData(displayData);
            // 必须紧随 setPickEnabled 之后，确保激活状态与局部施加勾选严格一致
            setPickEnabled(gravityPtr->isCornerAddChecked);
        }
    }

    QVector<QWidget*> mLoadGravityPanel::getRefStateInputs(FoldSection* fathertar)
    {
        refstate_ = fathertar;
        QVector<QWidget*>wds;
        wds.append(_scale.propwid);
        return wds;
    }

    QVector<QWidget*> mLoadGravityPanel::getValueInputs(FoldSection* fatherval)
    {
        faval_ = fatherval;
        QVector<QWidget*> wds;
        wds.append(comboBoxs[0].propwid);

        int defineType = comboBoxs[0].combobox->currentIndex();
        if (defineType == 0) // 各向分量
        {
            wds.append(inputWidgets[0].propwid);
            wds.append(inputWidgets[1].propwid);
            wds.append(inputWidgets[2].propwid);
        }
        else if (defineType == 1) // 方向与大小
        {
            ensureDirectionWidget();
            if (_directionWidget)
            {
                wds.append(_directionWidget);
            }
        }
        return wds;
    }

    QVector<QWidget*> mLoadGravityPanel::getTargetInputs(FoldSection* fatherval)
    {
        fatar_ = fatherval;
        QVector<QWidget*> wds;
        if (_pickWidgetController)
        {
            wds.append(_pickWidgetController->widget());
        }
        return wds;
    }

    void mLoadGravityPanel::setPickEnabled(bool enabled)
    {
        if (!_pickWidgetController) return;

        // 记录当前局部施加状态，供事件过滤器识别是否允许鼠标点击
        _pickWidgetEnabled = enabled;

        QWidget* pickWidget = _pickWidgetController->widget();

        // 整体启用/禁用拾取控件：未勾选局部施加时置灰且不可交互
        pickWidget->setEnabled(enabled);

        // 显式设置鼠标事件穿透：禁用时不接收鼠标事件
        pickWidget->setAttribute(Qt::WA_TransparentForMouseEvents, !enabled);

        // 禁用/启用子控件
        auto* pickView = qobject_cast<mPickWidgetViewBase*>(pickWidget);
        if (pickView)
        {
            pickView->setPickWidgetEnabled(enabled);
        }

        if (enabled)
        {
            applyAllowedPickTypes();
            _pickWidgetController->setPickActive(true);
        }
        else
        {
            // 关闭局部施加：失活并清空拾取数据
            _pickWidgetController->setAllPickTypesDisabled(true);
            if (_pickWidgetController->isPickActive())
            {
                _pickWidgetController->setKeepHighlightOnDeactivate(false);
                _pickWidgetController->setPickActive(false);
                _pickWidgetController->setKeepHighlightOnDeactivate(true);
            }
            _pickWidgetController->clearAllHighlight();
            PickWidgetData empty;
            _pickWidgetController->replacePickedDataAndClearSets(empty.pickedData);
        }
    }

    bool mLoadGravityPanel::eventFilter(QObject* watched, QEvent* event)
    {
        if (!_pickWidgetEnabled && _pickWidgetController)
        {
            QWidget* pickWidget = _pickWidgetController->widget();
            if (pickWidget && (watched == pickWidget || pickWidget->isAncestorOf(qobject_cast<QWidget*>(watched))))
            {
                // 局部施加未勾选时，直接吞掉拾取控件内的鼠标事件
                if (event->type() == QEvent::MouseButtonPress ||
                    event->type() == QEvent::MouseButtonRelease ||
                    event->type() == QEvent::MouseButtonDblClick ||
                    event->type() == QEvent::MouseMove)
                {
                    return true;
                }
            }
        }
        return mLoadPanelBase::eventFilter(watched, event);
    }

    void mLoadGravityPanel::applyAllowedPickTypes()
    {
        if (!_pickWidgetController) return;
        _pickWidgetController->setAllowedTypes(&pickGeoPointEdgeFaceSolidNodePart);
        _pickWidgetController->setPickType();
    }

    void mLoadGravityPanel::saveGravityComponents()
    {
        if (!_currentLoad) return;

        QVector<int> curidx = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Length, curidx, { 3,0,0,0 });

        QString strV1 = inputWidgets[0].lineeditvalue->text().trimmed();
        QString strV2 = inputWidgets[1].lineeditvalue->text().trimmed();
        QString strV3 = inputWidgets[2].lineeditvalue->text().trimmed();

        _currentLoad->V1 = strV1.isEmpty() ? QVariant() : QVariant::fromValue<double>(strV1.toDouble() * ratio);
        _currentLoad->V2 = strV2.isEmpty() ? QVariant() : QVariant::fromValue<double>(strV2.toDouble() * ratio);
        _currentLoad->V3 = strV3.isEmpty() ? QVariant() : QVariant::fromValue<double>(strV3.toDouble() * ratio);

        QVector3D dir(strV1.isEmpty() ? 0.0 : strV1.toDouble(),
            strV2.isEmpty() ? 0.0 : strV2.toDouble(),
            strV3.isEmpty() ? 0.0 : strV3.toDouble());
        if (!dir.isNull())
        {
            dir.normalize();
            _currentLoad->dir_ = dir;
        }
        else
        {
            _currentLoad->dir_ = QVariant();
        }

        notifyLoadChanged();
    }

    void mLoadGravityPanel::saveScale()
    {
        if (!_currentLoad) return;

        double scale = _scale.lineeditvalue->text().trimmed().toDouble();
        auto& mapload = getCVM->getModelData()->getMapRelationManager();
        mapload.setScaleByCase({ MxFunctions::PhysicalFieldType::Physical_LoadBoundary ,_loadData->getLoadBoundaryId() }, _caseId, scale);

        notifyLoadChanged();
    }

    void mLoadGravityPanel::notifyLoadChanged()
    {
        if (!_loadData) return;

        // 仅通知外部刷新树/渲染，不再调用 storeLoadValue 全量保存
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_LoadBoundary, _loadData->getLoadBoundaryId());
    }

    void mLoadGravityPanel::connectUnitSystemResponse()
    {
        connect(getCVM->getModelData()->getUnitSystemManager(), &mUnitSystemManager::sg_unitChanged, this, &mLoadGravityPanel::slot_unitSystemResponse, Qt::UniqueConnection);
    }

    void mLoadGravityPanel::disconnectUnitSystemResponse()
    {
        disconnect(getCVM->getModelData()->getUnitSystemManager(), &mUnitSystemManager::sg_unitChanged, this, &mLoadGravityPanel::slot_unitSystemResponse);
    }

    std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> mLoadGravityPanel::getPickedData()
    {
        if (!_pickWidgetController)
            return std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>();
        return _pickWidgetController->getAllData().pickedData;
    }

    void mLoadGravityPanel::clearInputs()
    {
        for (auto& inp : inputWidgets) inp.clear();
        inputWidgets.clear();

        for (auto& combo : comboBoxs) combo.clear();
        comboBoxs.clear();

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
        _currentLoad.reset();
    }

    void mLoadGravityPanel::slot_unitSystemResponse(UnitType type, QVector<int> idx)
    {
        Q_UNUSED(idx);
        if (!(type & UnitSystem)) return;
        if (_loadData)
            setInputsValue(_loadData);   //从载荷对象(基准值)重渲染，值+单位标签一次到位
    }

    void mLoadGravityPanel::deactivatePickAndCommit()
    {
        // 面板为单例复用，切换载荷/面板时不会走析构/clearInputs，
        // 由统一退出出口（unEditLoad）调用，失活并提交当前拾取数据。
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
}
