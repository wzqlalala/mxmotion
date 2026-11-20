#include "mLoadEdgePanel.h"
#include <QSet>
#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mMapRelationManager.h"
#include "mUnitSystemManager.h"
#include "mInfoWindow.h"
#include "AllRenderSymbolManager.h"
#include "mGeneralPickUp.h"
#include "mDataStatusManager.h"
#include "mGeoManager.h"
//#include "mGeometricFeatureAttribute.h" //【OCC摘除·S10/P0-1】退役头死 include 清剿（拉 mOCCDocument OCC 链；冻结岛保档，同参照处理）
#include "mMeshData.h"
#include "mAppConfiguration.h"
#include "mGetSetFunction.h"
#include "mGetSetTypeTemplate.h"
#include "mMixSet.h"

using namespace MechCommon;
using namespace MechPreData;

namespace MechMainWindow
{
    mLoadEdgePanel::mLoadEdgePanel(QWidget* parent)
        : mLoadPanelBase(parent)
    {
        _realtimeUpdateTimer = new QTimer(this);
        _realtimeUpdateTimer->setSingleShot(true);
        connect(_realtimeUpdateTimer, &QTimer::timeout, this, &mLoadEdgePanel::onRealtimeUpdateTimeout);

        initInputs();
    }

    mLoadEdgePanel::~mLoadEdgePanel()
    {
        clearInputs();
    }

    void mLoadEdgePanel::connectUnitSystemResponse()
    {
        connect(getCVM->getModelData()->getUnitSystemManager(), &mUnitSystemManager::sg_unitChanged,
            this, &mLoadEdgePanel::slot_unitSystemResponse, Qt::UniqueConnection);
    }

    void mLoadEdgePanel::disconnectUnitSystemResponse()
    {
        disconnect(getCVM->getModelData()->getUnitSystemManager(), &mUnitSystemManager::sg_unitChanged,
            this, &mLoadEdgePanel::slot_unitSystemResponse);
    }

    void mLoadEdgePanel::initInputs()
    {
        // ========== 1. 通用拾取控件（作用对象） ==========
        _pickWidgetController = mGeneralPickUp::createPickWidget(CBBlue);
        connect(_pickWidgetController, &mPickWidgetController::pickActiveChanged, this, [this](bool isActive) {
            if (isActive) emit mMechGlobalSignals::getInstance()->activePickSig(6);
            });
        connect(_pickWidgetController, &mPickWidgetController::dataChanged, this, [this]() {
            updateLoadSymbolRealtime();

            // 拾取状态标记
            if (_pickWidgetController->getAllData().isEmpty())
            {
                _currentEdge->setPickState(1);
            }
            else
            {
                _currentEdge->setPickState(2);
            }
            notifyEdgeChanged();
            });
        _pickWidgetController->setSetResolver(&resolveSetToTypedData);
        _pickWidgetController->setDefultTip("选择线/面/单元线对象");
        //边载荷拾取过滤：几何线仅保留几何面上的线，单元线仅保留二维单元的边
        _pickWidgetController->setEdgeLoadPickFilter(true);

        // ========== 2. 分布类型下拉框 ==========
        _comboDistribution.initWidget("分布类型");
        _comboDistribution.combobox->setFixedHeight(30);
        _comboDistribution.combobox->addItem("均匀分布");
        connect(_comboDistribution.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mLoadEdgePanel::slot_distributionChanged);

        // ========== 3. 定义方式下拉框 ==========
        _comboDefineWay.initWidget("定义方式");
        _comboDefineWay.combobox->setFixedHeight(30);
        _comboDefineWay.combobox->addItem("统一赋值");
        connect(_comboDefineWay.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mLoadEdgePanel::slot_defineWayChanged);

        // ========== 4. 方向模式下拉框 ==========
        _comboDirectionMode.initWidget("方向模式");
        _comboDirectionMode.combobox->setFixedHeight(30);
        _comboDirectionMode.combobox->addItems({ "面内法向", "面外法向", "边切向", "通用" });
        connect(_comboDirectionMode.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mLoadEdgePanel::slot_directionModeChanged);

        // ========== 5. 载荷值输入框 ==========
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString unit = UnitSystems[sysunitidxs[0]][2]; // 边载荷单位沿用力/长度单位体系
        _sizeInput.initWidget("载荷值", false);
        _sizeInput.labelname->setText("载荷值");
        _sizeInput.lineeditvalue->setUnit(unit);
        _sizeInput.lineeditvalue->setFixedHeight(30);
        connect(_sizeInput.lineeditvalue, &mCalcLineEdit::editingFinished, this, [this]() {
            updateLoadSymbolRealtime();
            saveSize();
            });

        // ========== 6. 幅值曲线下拉框 ==========
        _functable.clear();
        combofunc.initWidget("幅值曲线");
        combofunc.pushbtn->setIcon(QIcon(":/mMainWidgets/Icon/BrowserTree/hanshu.png"));
        combofunc.pushbtn->setFixedSize(30, 30);
        combofunc.pushbtn->setIconSize(QSize(18, 18));
        combofunc.combobox->setFixedHeight(30);

        auto allFunc = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getAllFunctionID();
        _functable[""] = -1;
        combofunc.combobox->addItem("");
        for (auto funcid : allFunc) {
            QString funcname = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionNameByID(funcid);
            combofunc.combobox->addItem(funcname);
            _functable[funcname] = funcid;
        }
        connect(combofunc.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mLoadEdgePanel::slot_funcChanged);
        connect(combofunc.pushbtn, &QPushButton::clicked, this, [this]() {
            mInfoWindow::getInstance()->unEditInfoWindow();
            mInfoWindow::getInstance()->createFunction(2);

            combofunc.combobox->blockSignals(true);
            combofunc.combobox->clear();
            _functable.clear();

            auto allFunc = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getAllFunctionID();
            _functable[""] = -1;
            combofunc.combobox->addItem("");
            for (auto funcid : allFunc) {
                QString funcname = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionNameByID(funcid);
                combofunc.combobox->addItem(funcname);
                _functable[funcname] = funcid;
            }
            combofunc.combobox->setCurrentIndex(allFunc.size() > 0 ? allFunc.size() : 0);
            combofunc.combobox->blockSignals(false);
            saveFunc();
            });

        // ========== 7. 缩放因子（载荷引用状态） ==========
        _scale.initWidget("缩放因子", false);
        _scale.labelname->setText("缩放因子");
        connect(_scale.lineeditvalue, &mCalcLineEdit::editingFinished, this, [this]() {
            saveScale();
            });
    }

    void mLoadEdgePanel::ensureDirectionWidget()
    {
        if (_directionWidget) return;
        if (!_faval) return;

        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        _directionWidget = new mDirectionSelectorWidget(
            QStringLiteral("通用方向"), false, QStringLiteral(""), false,
            UnitSystems[sysunitidxs[0]][2], false, _faval->contentWidget());
        _directionWidget->setVisible(false);

        // 通用模式下方向控件方向参与符号渲染，方向变化时刷新
        connect(_directionWidget, &mDirectionSelectorWidget::directionChanged, this, [this]() {
            if (_currentEdge) applyDirectionWidgetToEdge(_currentEdge);
            updateLoadSymbolRealtime();
            notifyEdgeChanged();
            });
        connect(_directionWidget, &mDirectionSelectorWidget::layoutRefreshRequested, this, [this]() {
            if (_faval) _faval->refreshLayout();
            });
        //方向控件切到无拾取方式时，回退激活主拾取控件
        connect(_directionWidget, &mDirectionSelectorWidget::currentWayNoPickWidget, this, [this]() {
            if (_pickWidgetController) {
                _pickWidgetController->setPickActive(true);
            }
            });
    }

    QVector<QWidget*> mLoadEdgePanel::getRefStateInputs(FoldSection* fathertar)
    {
        Q_UNUSED(fathertar);
        QVector<QWidget*> wds;
        wds.append(_scale.propwid);
        return wds;
    }

    QVector<QWidget*> mLoadEdgePanel::getTargetInputs(FoldSection* fathertar)
    {
        Q_UNUSED(fathertar);
        QVector<QWidget*> wds;
        if (_pickWidgetController) {
            wds.append(_pickWidgetController->widget());
        }
        return wds;
    }

    QVector<QWidget*> mLoadEdgePanel::getValueInputs(FoldSection* fatherval)
    {
        _faval = fatherval;
        QVector<QWidget*> wds;

        wds.append(_comboDistribution.propwid);
        wds.append(_comboDefineWay.propwid);
        wds.append(_comboDirectionMode.propwid);

        // 通用模式下追加方向控件
        int dirMode = _comboDirectionMode.combobox->currentIndex();
        if (dirMode == 3) { // 通用 = 索引3
            ensureDirectionWidget();
            if (_directionWidget) {
                wds.append(_directionWidget);
            }
        }

        wds.append(_sizeInput.propwid);
        wds.append(combofunc.propwid);
        return wds;
    }

    void mLoadEdgePanel::setInputsValue(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData)
    {
        std::shared_ptr<mLoadEdge> edgePtr = std::dynamic_pointer_cast<mLoadEdge>(loadData);
        _currentEdge = edgePtr;
        if (!edgePtr) return;

        // 拾取控件持久化策略绑定
        if (_pickWidgetController) {
            auto strategy = std::make_shared<MapRelationPersistenceStrategy>(
                MxFunctions::Physical_LoadBoundary, _loadData->getLoadBoundaryId());
            _pickWidgetController->setPersistenceStrategy(strategy);
            _pickWidgetController->setSetResolver(&resolveSetToTypedData);

            QHash<QString, QString> setToType = getSetNamesList({
                SetType::GeoEdge, SetType::GeoFace, SetType::ElementEdge });
            _pickWidgetController->setSelectSetList(setToType);
            applyAllowedPickTypes();
        }

        // 单位初始化
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        QString unit = UnitSystems[sysunitidxs[0]][2];
        _sizeInput.lineeditvalue->setUnit(unit);

        // ========== 批量阻塞信号反显 ==========
        _comboDistribution.combobox->blockSignals(true);
        _comboDefineWay.combobox->blockSignals(true);
        _comboDirectionMode.combobox->blockSignals(true);
        combofunc.combobox->blockSignals(true);

        // 分布类型反显
        int distIdx = _comboDistribution.combobox->findText(edgePtr->distributionType_);
        _comboDistribution.combobox->setCurrentIndex(distIdx >= 0 ? distIdx : 0);

        // 定义方式反显
        int wayIdx = _comboDefineWay.combobox->findText(edgePtr->defineWay_);
        _comboDefineWay.combobox->setCurrentIndex(wayIdx >= 0 ? wayIdx : 0);

        // 方向模式反显
        int dirIdx = _comboDirectionMode.combobox->findText(edgePtr->directionMode_);
        _comboDirectionMode.combobox->setCurrentIndex(dirIdx >= 0 ? dirIdx : 0);

        // 通用模式：反显方向控件
        if (dirIdx == 3) {
            ensureDirectionWidget();
            if (_directionWidget) {
                bool oldBlock = _directionWidget->blockSignals(true);
                const auto& state = edgePtr->dirPickState_;

                _directionWidget->setDirectionMode(
                    static_cast<mDirectionSelectorWidget::DirectionMode>(state.mode));
                _directionWidget->setReversed(state.reversed);

                if (state.mode == mDirectionSelectorWidget::CustomCoordinate &&
                    state.startPoint.isValid() && !state.startPoint.isNull() &&
                    state.endPoint.isValid() && !state.endPoint.isNull()) {
                    _directionWidget->setCustomCoordinatePoints(
                        state.startPoint.value<QVector3D>(),
                        state.endPoint.value<QVector3D>());
                }
                else if (state.mode == mDirectionSelectorWidget::DirectInput) {
                    _directionWidget->setDirection(state.directInputDir);
                }

                _directionWidget->blockSignals(oldBlock);
            }
        }

        // 载荷值反显：对象存基准值{3,0,0,0}，按当前系统换算显示
        if (!edgePtr->size_.isEmpty()) {
            bool ok = false;
            double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Force, { 3,0,0,0 }, sysunitidxs);
            double v = edgePtr->size_.toDouble(&ok);
            _sizeInput.lineeditvalue->setExpression(ok
                ? MxFunctions::mNumberToString(v * ratio)
                : edgePtr->size_);
        }
        else {
            _sizeInput.lineeditvalue->setExpression("");
        }

        // 幅值曲线反显
        auto& mapload = getCVM->getModelData()->getMapRelationManager();
        int funcid = mapload.getMapRelation<int>(
            { Physical_LoadBoundary, loadData->getLoadBoundaryId() }, Physical_Function);
        if (funcid != -1) {
            QString funcname = getCVM->getModelData()->getManager<MechFunctionData::mFunctionManager>(Physical_Function).getFunctionNameByID(funcid);
            combofunc.combobox->setCurrentText(funcname);
        }
        else {
            combofunc.combobox->setCurrentIndex(0);
        }

        // 解除信号阻塞
        _comboDistribution.combobox->blockSignals(false);
        _comboDefineWay.combobox->blockSignals(false);
        _comboDirectionMode.combobox->blockSignals(false);
        combofunc.combobox->blockSignals(false);

        // ========== 作用对象反显 ==========
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

        // displayData 默认不重画高亮；若控件此前已处于激活态，setPickActive(true) 会直接返回，
        // 该情况下需显式重画一次高亮以匹配反显数据。整体保证进入面板只做一次全量高亮。
        if (_pickWidgetController) {
            _pickWidgetController->setPickWidgetVisible(true);
            const bool wasActive = _pickWidgetController->isPickActive();
            _pickWidgetController->displayData(displayData);
            _pickWidgetController->setPickActive(true);
            if (wasActive) {
                _pickWidgetController->refreshHighlight();
            }
        }

        // 反写完毕后判断拾取控件是否为空 以设置标志位
        if (_pickWidgetController->getAllData().isEmpty()) {
            _currentEdge->setPickState(1);
        }
        else {
            _currentEdge->setPickState(2);
        }
        notifyEdgeChanged();

        // 缩放因子反显
        double scale = mapload.getScaleByCase({ MxFunctions::Physical_LoadBoundary, loadData->getLoadBoundaryId() }, _caseId);
        _scale.lineeditvalue->setExpression(MxFunctions::mNumberToString(scale));

        // 通用模式下，方向控件拾取默认可见
        if (dirIdx == 3 && _directionWidget) {
            _directionWidget->setPickWidgetVisible(true);
        }
        
        // 延迟刷新符号
        QMetaObject::invokeMethod(this, &mLoadEdgePanel::updateLoadSymbolRealtime, Qt::QueuedConnection);
    }

    void mLoadEdgePanel::applyAllowedPickTypes()
    {
        if (!_pickWidgetController) return;
        _pickWidgetController->setAllowedTypes(&pickGeoEdgeFaceSolidAndElement);
        _pickWidgetController->setPickType();
    }

    void mLoadEdgePanel::slot_distributionChanged(int idx)
    {
        Q_UNUSED(idx);
        saveDistribution();
    }

    void mLoadEdgePanel::slot_defineWayChanged(int idx)
    {
        Q_UNUSED(idx);
        saveDefineWay();
    }

    void mLoadEdgePanel::slot_directionModeChanged(int idx)
    {
        // 动态调整值折叠框布局：通用模式显示方向控件，其他隐藏
        if (_faval)
        {
            _faval->removeAll();
            _faval->addRow(_comboDistribution.propwid);
            _comboDistribution.propwid->show();
            _faval->addRow(_comboDefineWay.propwid);
            _comboDefineWay.propwid->show();
            _faval->addRow(_comboDirectionMode.propwid);
            _comboDirectionMode.propwid->show();

            if (idx == 3)
            { // 通用
                ensureDirectionWidget();
                if (_directionWidget)
                {
                    _faval->addRow(_directionWidget);
                    _directionWidget->show();
                    _directionWidget->setPickWidgetVisible(true); // 显示方向控件内部拾取

                    //首次进入通用模式时控件默认X轴但不发directionChanged，
                    //主动同步默认方向，保证符号按当前方向渲染
                    if (_currentEdge && !_currentEdge->dir_.isValid())
                    {
                        applyDirectionWidgetToEdge(_currentEdge);
                    }
                }
            }
            else
            {
                if (_directionWidget)
                {
                    _directionWidget->hide();
                    _directionWidget->clearAllHighlight();
                    _directionWidget->setPickWidgetVisible(false); // 隐藏方向控件内部拾取
                }
                // 回退激活主拾取控件
                if (_pickWidgetController)
                {
                    _pickWidgetController->setPickActive(true);
                }
            }

            _faval->addRow(_sizeInput.propwid);
            _sizeInput.propwid->show();
            _faval->addRow(combofunc.propwid);
            combofunc.propwid->show();
            _faval->refreshLayout();
        }

        saveDirectionMode();
        updateLoadSymbolRealtime();
    }

    void mLoadEdgePanel::slot_sizeChanged()
    {
        saveSize();
        updateLoadSymbolRealtime();
    }

    void mLoadEdgePanel::slot_funcChanged(int idx)
    {
        Q_UNUSED(idx);
        saveFunc();
    }

    void mLoadEdgePanel::slot_unitSystemResponse(UnitType type, QVector<int> idx)
    {
        Q_UNUSED(idx);
        if (!(type & UnitSystem)) return;
        if (_loadData)
            setInputsValue(_loadData);   //从载荷对象(基准值)重渲染，值+单位标签一次到位
    }

    void mLoadEdgePanel::saveDistribution()
    {
        if (!_currentEdge) return;
        _currentEdge->distributionType_ = _comboDistribution.combobox->currentText();
        notifyEdgeChanged();
    }

    void mLoadEdgePanel::saveDefineWay()
    {
        if (!_currentEdge) return;
        _currentEdge->defineWay_ = _comboDefineWay.combobox->currentText();
        notifyEdgeChanged();
    }

    void mLoadEdgePanel::saveDirectionMode()
    {
        if (!_currentEdge) return;
        _currentEdge->directionMode_ = _comboDirectionMode.combobox->currentText();
        notifyEdgeChanged();
    }

    void mLoadEdgePanel::saveSize()
    {
        if (!_currentEdge) return;
        QString text = _sizeInput.lineeditvalue->text().trimmed();
        bool ok = false;
        QVector<int> sysunitidxs = getCVM->getModelData()->getUnitSystemManager()->getCurUnitsIndex();
        double ratio = getCVM->getModelData()->getUnitSystemManager()->getUnitSystemFromToRatio(Unit::Force, { 3,0,0,0 }, sysunitidxs);
        double v = text.toDouble(&ok);
        //显示值(当前单位制) → 基准值{3,0,0,0}存储；非数值文本原样保留
        _currentEdge->size_ = ok
            ? MxFunctions::mNumberToString(v * ratio)
            : text;
        notifyEdgeChanged();
    }

    void mLoadEdgePanel::saveFunc()
    {
        if (!_currentEdge) return;
        int funcid = getFunc();
        auto& mapload = getCVM->getModelData()->getMapRelationManager();
        mapload.appendMapRelation(
            { Physical_LoadBoundary ,_loadData->getLoadBoundaryId() },
            { Physical_Function, funcid });
        notifyEdgeChanged();
    }

    void mLoadEdgePanel::saveScale()
    {
        if (!_currentEdge) return;
        double scale = _scale.lineeditvalue->text().trimmed().toDouble();
        auto& mapload = getCVM->getModelData()->getMapRelationManager();
        mapload.setScaleByCase({ MxFunctions::PhysicalFieldType::Physical_LoadBoundary, _loadData->getLoadBoundaryId() }, _caseId, scale);
        notifyEdgeChanged();
    }

    void mLoadEdgePanel::applyDirectionWidgetToEdge(std::shared_ptr<mLoadEdge> edge)
    {
        if (!edge || !_directionWidget) return;

        // 保存方向矢量
        QVariant dirVar = _directionWidget->getDirection();
        if (dirVar.isValid() && !dirVar.isNull()) {
            QVector3D dir = dirVar.value<QVector3D>();
            if (!dir.isNull()) {
                dir.normalize();
                edge->dir_ = dir;
            }
            else {
                edge->dir_ = QVariant();
            }
        }
        else {
            edge->dir_ = QVariant();
        }

        // 保存控件UI状态
        auto& state = edge->dirPickState_;
        state.mode = static_cast<int>(_directionWidget->directionMode());
        state.reversed = _directionWidget->isReversed();

        if (state.mode == mDirectionSelectorWidget::CustomCoordinate) {
            state.startPoint = QVariant::fromValue(_directionWidget->startPoint());
            state.endPoint = QVariant::fromValue(_directionWidget->endPoint());
        }
        else if (state.mode == mDirectionSelectorWidget::DirectInput) {
            state.directInputDir = dirVar.value<QVector3D>();
        }
    }

    void mLoadEdgePanel::notifyEdgeChanged()
    {
        if (!_loadData) return;
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_LoadBoundary, _loadData->getLoadBoundaryId());
    }

    int mLoadEdgePanel::getFunc()
    {
        int funcid = -1;
        QString funcname = combofunc.combobox->currentText();
        if (_functable.find(funcname) != _functable.end()) {
            funcid = _functable[funcname];
        }
        return funcid;
    }

    void mLoadEdgePanel::updateLoadSymbolRealtime()
    {
        if (!_pickWidgetController || !_currentEdge) return;
        if (!getCVM || !getCVM->getPreRenderWidget()) return;
        if (!_realtimeUpdateTimer) return;

        _realtimeUpdateTimer->start(100); // 100ms防抖
    }

    void mLoadEdgePanel::onRealtimeUpdateTimeout()
    {
        doUpdateLoadSymbolRealtime();
    }

    void mLoadEdgePanel::doUpdateLoadSymbolRealtime()
    {
        if (!_pickWidgetController || !_currentEdge) return;

        const PickWidgetData allData = _pickWidgetController->getAllData();
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> merged = allData.pickedData;

        // 集合展开（与集中力逻辑一致）
        auto& setManager = getCVM->getModelData()->getManager<MechSetData::mSetManager>(Physical_Set);
        auto appendIds = [](MxFunctions::PickDataType type, const std::vector<int>& ids,
            std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& dst) {
                auto& vec = dst[type];
                for (int id : ids) vec.emplace_back(id, -1, -1);
            };

        for (const QString& setName : allData.selectedSetNames) {
            mSetBase* setBase = setManager.getSetData(setName);
            if (!setBase) continue;
            if (auto mixSet = dynamic_cast<MechSetData::mMixSet*>(setBase)) {
                for (const auto& inner : mixSet->getMixSetData()) {
                    auto& vec = merged[inner.first];
                    vec.insert(vec.end(), inner.second.begin(), inner.second.end());
                }
                continue;
            }
            switch (setBase->getSetType()) {
            case MxFunctions::GeoEdge:
                if (auto s = dynamic_cast<mGeoSet*>(setBase)) appendIds(SetTypeToPickDataType(setBase->getSetType()), s->getGeoSetData(), merged);
                break;
            case MxFunctions::GeoFace:
                if (auto s = dynamic_cast<mGeoSet*>(setBase)) appendIds(SetTypeToPickDataType(setBase->getSetType()), s->getGeoSetData(), merged);
                break;
            case MxFunctions::ElementEdge:
                if (auto s = dynamic_cast<mElementSurface*>(setBase)) {
                    auto v = s->getElementSurfaceData();
                    auto& d = merged[PickDataType::ElementEdge];
                    d.insert(d.end(), v.begin(), v.end());
                }
                break;
            default: break;
            }
        }

        // ========== 计算各作用边的位置与方向 ==========
        // 作用对象：几何线/几何面/单元线；方向随方向模式下拉框：
        // 面内法向/面外法向/边切向使用计算方向，通用使用方向控件方向
        QVector3D generalDir;
        if (_currentEdge->dir_.isValid() && !_currentEdge->dir_.isNull()) {
            generalDir = _currentEdge->dir_.value<QVector3D>();
        }

        QVector<std::pair<QVector3D, QVector3D>> pos_dir =
            AllRenderSymbolManager::calcEdgeLoadPosDir(merged, _comboDirectionMode.combobox->currentIndex(), generalDir);

        // 渲染方向与载荷值符号关联：载荷值为负时方向取反；无值/非数字默认正方向
        bool sizeOk = false;
        double sizeVal = _sizeInput.lineeditvalue->text().trimmed().toDouble(&sizeOk);
        if (sizeOk && sizeVal < 0)
        {
            for (auto& pd : pos_dir)
            {
                pd.second = -pd.second;
            }
        }

        // 绘制边载荷符号。
        // 最终数据签名：载荷ID+位置方向对未变化时跳过符号赋值与 UpdateAllSymbol
        // 全量重建（进入面板防抖触发/切出冲刷时数据往往与上次相同）
        size_t sig = AllRenderSymbolManager::beginRenderSignature();
        AllRenderSymbolManager::combineRenderSignature(sig, static_cast<size_t>(_currentEdge->getLoadBoundaryId()));
        AllRenderSymbolManager::combineRenderSignature(sig, pos_dir);
        if (sig == _lastRenderSig)
        {
            return;
        }
        _lastRenderSig = sig;

        AllRenderSymbolManager::getInstance()->assignLoad(
            _currentEdge, pos_dir, "Load_Edge", QVector3D(33, 150, 243));

        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        if (symbolController) symbolController->UpdateAllSymbol();
        getCVM->getPreRenderWidget()->updatePaint();
    }

    void mLoadEdgePanel::deactivatePickAndCommit()
    {
        const bool renderValid = (getCVM && getCVM->getPreRenderWidget());
        if (_pickWidgetController) {
            _pickWidgetController->setKeepHighlightOnDeactivate(false);
            bool wasActive = _pickWidgetController->isPickActive();
            if (wasActive) _pickWidgetController->setPickActive(false);
            if (renderValid && !wasActive) {
                _pickWidgetController->clearAllHighlight();
            }
            _pickWidgetController->setKeepHighlightOnDeactivate(true);
        }

        // 方向控件内部拾取失活
        if (_directionWidget) {
            auto deactivateChild = [](mPickWidgetController* ctrl) {
                if (!ctrl) return;
                if (!ctrl->isPickActive()) return;
                ctrl->setKeepHighlightOnDeactivate(false);
                ctrl->setPickActive(false);
                ctrl->setKeepHighlightOnDeactivate(true);
                };
            deactivateChild(_directionWidget->startPickController());
            deactivateChild(_directionWidget->endPickController());
            deactivateChild(_directionWidget->objectPickController());
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

    void mLoadEdgePanel::clearData()
    {
        _sizeInput.lineeditvalue->setExpression("");
        if (_directionWidget) {
            _directionWidget->resetToDefault();
        }
    }

    void mLoadEdgePanel::clearInputs()
    {
        if (_pickWidgetController) {
            _pickWidgetController->setPickActive(false);
            delete _pickWidgetController;
            _pickWidgetController = nullptr;
        }
        if (_directionWidget) {
            _directionWidget->hide();
            delete _directionWidget;
            _directionWidget = nullptr;
        }

        _comboDistribution.clear();
        _comboDefineWay.clear();
        _comboDirectionMode.clear();
        combofunc.clear();
        _functable.clear();
        _sizeInput.clear();
        _currentEdge.reset();
        _faval = nullptr;
    }
}