#include "mBoundaryDisplacementPanel.h"

#include "mMechGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mSetManager.h"
#include "mMapRelationEnum.h"
#include "mMapRelationManager.h"
#include "mInfoWindow.h"
#include "mDockLayoutManager.h"
#include "AllRenderSymbolManager.h"
#include "mGeneralPickUp.h"
#include "mGetSetTypeTemplate.h"
#include "mGetSetFunction.h"
#include "mDataStatusManager.h"

//Geo
#include "mGeoManager.h"
//#include "mGeometricFeatureAttribute.h" //【OCC摘除·S10/P0-1】退役头死 include 清剿（冻结岛保档，同主线处理）
#include "PickCommonFunction.h"

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

#include "mBoundaryDisplacement.h"
#include "mOtherFunction.h"

using namespace MechCommon;
using namespace MechPreData;

namespace MechMainWindow
{
    mBoundaryDisplacementPanel::mBoundaryDisplacementPanel(QWidget* parent)
        :mBoundaryPanelBase(parent)
    {
        _realtimeUpdateTimer = new QTimer(this);
        _realtimeUpdateTimer->setSingleShot(true);
        connect(_realtimeUpdateTimer, &QTimer::timeout, this, &mBoundaryDisplacementPanel::doUpdateBoundarySymbolRealtime);

        initInputs();
    }

    mBoundaryDisplacementPanel::~mBoundaryDisplacementPanel()
    {
        clearInputs();
    }

    std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> mBoundaryDisplacementPanel::getPickedData()
    {
        if (!_pickWidgetController)
            return std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>();
        return _pickWidgetController->getAllData().pickedData;
    }

    void mBoundaryDisplacementPanel::initInputs()
    {
        QStringList checknames = {
            "U1",
            "U2",
            "U3",
            "UR1",
            "UR2",
            "UR3",
        };

        // 定义方式
        _comboDefinitionMode.initWidget("定义方式");
        _comboDefinitionMode.combobox->addItem("各向分量");
        _comboDefinitionMode.combobox->addItem("独立赋值");
        connect(_comboDefinitionMode.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mBoundaryDisplacementPanel::slot_definitionModeChanged);

        // 各自由度分量输入框
        for (int i = 0; i < 6; ++i)
        {
            InputWithCheckBox widgetSet;
            widgetSet.initWidget();
            widgetSet.checkbox->setChecked(false);
            widgetSet.lineeditvalue->setDisabled(true);

            // 输入完成：单独保存分量 + 刷新实时渲染
            connect(widgetSet.lineeditvalue, &mCalcLineEdit::editingFinished,
                [this]() {
                    saveComponents();
                    updateBoundarySymbolRealtime();
                });

            // checkbox勾选状态变化：单独保存
            connect(widgetSet.checkbox, &QCheckBox::stateChanged,
                [=](int state) {
                    bool isChecked = (state == Qt::Checked);
                    widgetSet.lineeditvalue->setDisabled(!isChecked);
                    if (!isChecked)
                    {
                        widgetSet.lineeditvalue->setExpression("");
                    }
                    saveComponents();
                    updateBoundarySymbolRealtime();
                });

            widgetSet.checkbox->setText(checknames[i]);
            InputWidgets.append(widgetSet);
        }

        // 通用拾取控件
        _pickWidgetController = mGeneralPickUp::createPickWidget(CBBlue);
        connect(_pickWidgetController, &mPickWidgetController::pickActiveChanged, this, [this](bool isActive) {
            if (isActive)
            {
                emit mMechGlobalSignals::getInstance()->activePickSig(5);
            }
            });
        connect(_pickWidgetController, &mPickWidgetController::dataChanged, this, [this]() {
            // 独立赋值模式下通用拾取控制器不再接收拾取（由离散数据控件直驱视口），
            // 其 dataChanged 只在各向分量模式下触发，无需按方式分支处理。
            updateBoundarySymbolRealtime();
            //判断拾取控件是否为空 以设置标志位
            if (_pickWidgetController->getAllData().isEmpty())
            {
                _currentBoundary->setPickState(1);
            }
            else
            {
                _currentBoundary->setPickState(2);
            }
            //发送更新信号
            notifyBoundaryChanged();
            });

        _pickWidgetController->setSetResolver(&resolveSetToTypedData);
        _pickWidgetController->setDefultTip("选择对象");

        _functable.clear();

        // 幅值曲线
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
            this, &mBoundaryDisplacementPanel::slot_funcChanged);
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
            { QStringLiteral("U1"), QStringLiteral("U2"), QStringLiteral("U3"),
              QStringLiteral("UR1"), QStringLiteral("UR2"), QStringLiteral("UR3") },
            &nodeSpec, _tableRowWidget);
        _discreteControl->setRenderButtonVisible(false);
        connect(_discreteControl, &mDiscreteDataControl::dataChanged,
            this, &mBoundaryDisplacementPanel::slot_discreteDataChanged);
        connect(_discreteControl, &mDiscreteDataControl::cellEdited,
            this, &mBoundaryDisplacementPanel::slot_discreteCellEdited);
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

        // 缩放因子
        _scale.initWidget("缩放因子", false);
        _scale.labelname->setText("缩放因子");
        connect(_scale.lineeditvalue, &mCalcLineEdit::editingFinished,
            [this]() {
                saveScale();
            });
    }

    void mBoundaryDisplacementPanel::updateModeWidgets(int mode)
    {
        if (!faval_)
        {
            return;
        }
        faval_->removeAll();
        // 定义方式
        faval_->addRow(_comboDefinitionMode.propwid);
        _comboDefinitionMode.propwid->show();

        // 0=各向分量：显示6个分量输入
        if (mode == 0)
        {
            for (auto& inp : InputWidgets)
            {
                faval_->addRow(inp.propwid);
                inp.propwid->show();
            }
        }
        // 1=独立赋值：显示表格行
        else
        {
            faval_->addRow(_tableRowWidget);
            _tableRowWidget->show();
        }

        // 幅值曲线
        faval_->addRow(combofunc.propwid);
        combofunc.propwid->show();

        faval_->refreshLayout();
    }

    void mBoundaryDisplacementPanel::slot_definitionModeChanged(int idx)
    {
        // 切换定义方式时关闭离散数据弹窗并结束拾取，避免旧模式弹窗残留
        if (_discreteControl)
        {
            _discreteControl->closePopup();
        }

        // 独立赋值：仅允许节点拾取，过滤并隐藏集合选择按钮；
        // 通用拾取控件隐藏且拾取由离散数据控件直接驱动，不再激活
        if (idx == 1)
        {
            filterPicksToNodeOnly();
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
                _pickWidgetController->setSelectSetBtnVisible(true);
                _pickWidgetController->setPickActive(true);
                _pickWidgetController->setCloudChartMode(false);
            }
        }

        saveDefinitionMode();
        applyAllowedPickTypes(idx);
        updateModeWidgets(idx);
        updateBoundarySymbolRealtime();

        // 定义方式改变导致作用对象折叠框显隐变化，由外部重建面板
        emit sg_rebuildPanel();
    }

    void mBoundaryDisplacementPanel::slot_funcChanged(int idx)
    {
        Q_UNUSED(idx)
            saveFunc();
    }
    void mBoundaryDisplacementPanel::filterPicksToNodeOnly()
    {
        // 同步通用拾取控件最新数据到 picks，避免过滤时丢失拾取/集合
        auto picks = _pickWidgetController ? _pickWidgetController->getAllData().pickedData
            : std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>();

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

        // 独立赋值模式无需集合：把控件数据重设为"仅节点拾取、无集合"，
        // 从根上清除"已选择xx集"的残留显示
        if (_pickWidgetController)
        {
            _pickWidgetController->replacePickedDataAndClearSets(picks);
        }
        // 独立赋值已完全剥离通用拾取：抑制本控件的一切拾取高亮渲染，
        // 拾取由离散数据控件直接驱动视口选择容器
        if (_pickWidgetController)
        {
            _pickWidgetController->setCloudChartMode(true);
        }

        updateOpenedTable();
    }

    void mBoundaryDisplacementPanel::setInputsValue(std::shared_ptr<MechCaseData::mLoadBoundaryBase> boundaryData)
    {
        std::shared_ptr<MechCaseData::mBoundaryDisplacement> boundaryPtr =
            std::dynamic_pointer_cast<MechCaseData::mBoundaryDisplacement>(boundaryData);

        _boundaryData = boundaryData;
        _currentBoundary = boundaryPtr;
        if (!boundaryPtr) return;

        int mode = boundaryPtr->_definitionMode;

        _comboDefinitionMode.combobox->blockSignals(true);
        _comboDefinitionMode.combobox->setCurrentIndex(mode);
        _comboDefinitionMode.combobox->blockSignals(false);

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

        auto& mapboundary = getCVM->getModelData()->getMapRelationManager();
        int funcid = mapboundary.getMapRelation<int>({ Physical_LoadBoundary, boundaryData->getLoadBoundaryId() }, Physical_Function);
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
                boundaryData->getLoadBoundaryId());
            _pickWidgetController->setPersistenceStrategy(strategy);
            _pickWidgetController->setSetResolver(&resolveSetToTypedData);

            QHash<QString, QString> setToType = getSetNamesList({ SetType::GeoPoint, SetType::GeoEdge, SetType::GeoFace, SetType::GeoSolid, SetType::Node });
            _pickWidgetController->setSelectSetList(setToType);

            applyAllowedPickTypes(mode);
        }

        // 拾取数据反显
        // 单例面板复用：无论新边界定义方式如何，先清空离散表格残留行，
        // 避免旧边界独立赋值数据在用户切回独立赋值时复现并污染新边界
        if (_discreteControl)
        {
            _discreteControl->setRows({});
        }
        PickWidgetData displayData;
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
        // 独立赋值模式且没有集合映射时，直接从数据类生成拾取数据（供 pickState/持久化一致性）
        if (mode == 1 && displayData.pickedData.empty() && displayData.selectedSetNames.isEmpty())
        {
            std::vector<RenderPickResultData> nodePicks;
            for (const auto& entry : boundaryPtr->_independentTable)
            {
                nodePicks.push_back(entry.first);
            }
            if (!nodePicks.empty())
            {
                displayData.pickedData[MxFunctions::PickDataType::Node] = std::move(nodePicks);
            }
        }

        // 独立赋值：把 map 反显进离散数据控件（节点 + 已填分量）
        if (mode == 1 && _discreteControl)
        {
            QVector<mDiscreteDataControl::Row> rows;
            rows.reserve(static_cast<int>(boundaryPtr->_independentTable.size()));
            for (const auto& entry : boundaryPtr->_independentTable)
            {
                QStringList values;
                for (int i = 0; i < 6; ++i)
                {
                    const QVariant v = i < entry.second.size() ? entry.second.at(i) : QVariant();
                    values << ((v.isValid() && !v.isNull()) ? QString::number(v.toDouble()) : QString());
                }
                rows.append(qMakePair(qMakePair(entry.first.result_id, -1), values));
            }
            _discreteControl->setRows(rows);
        }

        if (_pickWidgetController)
        {
            if (mode == 1)
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
                // displayData 默认不重画高亮；若控件此前已处于激活态，setPickActive(true) 会直接返回，
                // 该情况下需显式重画一次高亮以匹配反显数据。整体保证进入面板只做一次全量高亮。
                // 旧边界在独立赋值模式下曾 setPickWidgetVisible(false)，
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
            _pickWidgetController->setSelectSetBtnVisible(mode != 1);
        }

        // 独立赋值：过滤为仅节点拾取，并隐藏通用拾取控件（拾取由离散数据控件驱动）
        if (mode == 1)
        {
            filterPicksToNodeOnly();
            if (_pickWidgetController)
            {
                _pickWidgetController->setPickWidgetVisible(false);
            }
        }

        // 各自由度分量反显
        // 屏蔽控件信号：setChecked 触发 stateChanged→saveComponents 会把
        // 尚未恢复完的旧边界控件状态写进当前 _currentBoundary，造成边界间数据串扰
        auto setComp = [&](int idx, const QVariant& data) {
            bool valid = (!data.isNull() && data.isValid());
            InputWidgets[idx].checkbox->blockSignals(true);
            InputWidgets[idx].lineeditvalue->blockSignals(true);
            InputWidgets[idx].checkbox->setChecked(valid);
            InputWidgets[idx].lineeditvalue->setDisabled(!valid);
            InputWidgets[idx].lineeditvalue->setExpression(valid ? QString::number(data.toDouble()) : "");
            InputWidgets[idx].checkbox->blockSignals(false);
            InputWidgets[idx].lineeditvalue->blockSignals(false);
            };
        setComp(0, boundaryPtr->_U1);
        setComp(1, boundaryPtr->_U2);
        setComp(2, boundaryPtr->_U3);
        setComp(3, boundaryPtr->_UR1);
        setComp(4, boundaryPtr->_UR2);
        setComp(5, boundaryPtr->_UR3);

        // 缩放因子
        double scale = mapboundary.getScaleByCase({ MxFunctions::PhysicalFieldType::Physical_LoadBoundary, boundaryData->getLoadBoundaryId() }, _caseId);
        _scale.lineeditvalue->setExpression(MxFunctions::mNumberToString(scale));

        updateOpenedTable();
        //判断拾取控件是否为空 以设置标志位
        if (_pickWidgetController->getAllData().isEmpty())
        {
            _currentBoundary->setPickState(1);
        }
        else
        {
            _currentBoundary->setPickState(2);
        }
        //发送更新信号
        notifyBoundaryChanged();
        updateBoundarySymbolRealtime();
    }

    QVector<QWidget*> mBoundaryDisplacementPanel::getRefStateInputs(FoldSection* fathertar)
    {
        refstate_ = fathertar;
        QVector<QWidget*>wds;
        wds.append(_scale.propwid);
        return wds;
    }

    QVector<QWidget*> mBoundaryDisplacementPanel::getTargetInputs(FoldSection* fathertar)
    {
        fatar_ = fathertar;
        QVector<QWidget*>wds;
        if (_pickWidgetController)
        {
            wds.append(_pickWidgetController->widget());
        }
        return wds;
    }

    QVector<QWidget*> mBoundaryDisplacementPanel::getValueInputs(FoldSection* fatherval)
    {
        faval_ = fatherval;
        QVector<QWidget*> wds;
        wds.append(_comboDefinitionMode.propwid);

        int mode = _comboDefinitionMode.combobox->currentIndex();
        if (mode == 0)
        {
            for (auto& inp : InputWidgets)
            {
                wds.append(inp.propwid);
            }
        }
        else
        {
            wds.append(_tableRowWidget);
        }
        wds.append(combofunc.propwid);
        return wds;
    }

    void mBoundaryDisplacementPanel::saveDefinitionMode()
    {
        if (!_currentBoundary)
            return;
        _currentBoundary->_definitionMode = _comboDefinitionMode.combobox->currentIndex();
        notifyBoundaryChanged();
    }

    void mBoundaryDisplacementPanel::saveComponents()
    {
        if (!_currentBoundary)
            return;
        auto getComp = [&](int idx) -> QVariant {
            if (InputWidgets[idx].checkbox->isChecked())
            {
                QString txt = InputWidgets[idx].lineeditvalue->text().trimmed();
                if (!txt.isEmpty())
                    return QVariant(txt.toDouble());
            }
            return QVariant();
            };

        _currentBoundary->_U1 = getComp(0);
        _currentBoundary->_U2 = getComp(1);
        _currentBoundary->_U3 = getComp(2);
        _currentBoundary->_UR1 = getComp(3);
        _currentBoundary->_UR2 = getComp(4);
        _currentBoundary->_UR3 = getComp(5);

        notifyBoundaryChanged();
    }

    void mBoundaryDisplacementPanel::saveScale()
    {
        if (!_boundaryData)
            return;
        double scale = _scale.lineeditvalue->text().trimmed().toDouble();
        auto& mapboundary = getCVM->getModelData()->getMapRelationManager();
        mapboundary.setScaleByCase({ MxFunctions::PhysicalFieldType::Physical_LoadBoundary, _boundaryData->getLoadBoundaryId() }, _caseId, scale);
        notifyBoundaryChanged();
    }

    void mBoundaryDisplacementPanel::saveFunc()
    {
        if (!_boundaryData)
            return;
        int funcid = getFunc();
        auto& mapboundary = getCVM->getModelData()->getMapRelationManager();
        mapboundary.appendMapRelation({ Physical_LoadBoundary, _boundaryData->getLoadBoundaryId() }, { Physical_Function, funcid });
        notifyBoundaryChanged();
    }

    void mBoundaryDisplacementPanel::notifyBoundaryChanged()
    {
        if (!_boundaryData)
            return;
        // 仅通知外部刷新树/渲染，不再调用 storeBoundaryValue 全量保存
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_LoadBoundary, _boundaryData->getLoadBoundaryId());
    }

    void mBoundaryDisplacementPanel::slot_discreteDataChanged()
    {
        // 离散数据控件行变化（拾取追加/删除/清空/分量增减）：
        // 按控件当前行重建 _independentTable，随后刷新实时符号并通知外部。
        // 拾取会话进行中跳过：确认拾取（完成拾取）后控件会补发一次 dataChanged，
        // 此时才执行重建与渲染，避免拾取期间反复重绘。
        if (!_currentBoundary || (_discreteControl && _discreteControl->isPicking()))
        {
            return;
        }
        updateOpenedTable();
        updateBoundarySymbolRealtime();
        notifyBoundaryChanged();
    }

    void mBoundaryDisplacementPanel::slot_discreteCellEdited(const QPair<int, int>& key, int valueIndex, const QString& text)
    {
        if (!_currentBoundary || key.first <= 0)
        {
            return;
        }
        // 单点写回：仅更新/清除该节点的该分量，节点本身始终保留。
        // 位移转角固定 6 个分量（U1/U2/U3/UR1/UR2/UR3），key 规范为 result_id/-1/-1。
        const RenderPickResultData boundaryKey(key.first, -1, -1);
        QVector<QVariant>& values = _currentBoundary->_independentTable[boundaryKey];
        if (values.size() < 6)
        {
            values.resize(6);
        }
        if (valueIndex >= 0 && valueIndex < 6)
        {
            bool ok = false;
            const double v = text.toDouble(&ok);
            values[valueIndex] = (ok && !text.isEmpty()) ? QVariant(v) : QVariant();
        }
        updateBoundarySymbolRealtime();
        notifyBoundaryChanged();
    }

    void mBoundaryDisplacementPanel::updateOpenedTable()
    {
        if (!_currentBoundary)
        {
            return;
        }

        // 独立赋值模式下，_independentTable 与离散数据控件的行数据保持严格一致：
        // 表格是数据的权威来源（含拾取追加与手动删除），此处按控件当前行重建 map，
        // 保留已填分量。
        if (!_discreteControl)
        {
            return;
        }

        const auto rows = _discreteControl->rows();

        // 已填分量按节点 ID 保留；key 统一规范为 (result_id, -1, -1)，
        // 保证 slot_discreteCellEdited 可用同一规范 key 直接索引，且与
        // mIMxFile 读入键规范（nodeId, -1, -1）逐字段一致。
        std::map<int, QVector<QVariant>> existingValues;
        for (const auto& entry : _currentBoundary->_independentTable)
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
                (it != existingValues.end()) ? it->second : QVector<QVariant>(6);
        }

        _currentBoundary->_independentTable = std::move(newMap);
    }

    int mBoundaryDisplacementPanel::getFunc()
    {
        int funcid = -1;
        QString funcname = combofunc.combobox->currentText();
        if (_functable.find(funcname) != _functable.end())
        {
            funcid = _functable[funcname];
        }
        return funcid;
    }

    void mBoundaryDisplacementPanel::applyAllowedPickTypes(int mode)
    {
        if (!_pickWidgetController) return;
        if (mode == 1)
        {
            // 独立赋值：仅设置白名单记录，不切换视口过滤器
            // （拾取过滤器由离散数据控件在开始拾取时自行激活/恢复）
            _pickWidgetController->setAllowedTypes(&pickOnlyNode);
        }
        else
        {
            // 各向分量：几何点/边/面/体 + 节点
            _pickWidgetController->setAllowedTypes(&pickGeoPointEdgeFaceSolidNodePart);
            _pickWidgetController->setPickType();
        }
    }

    void mBoundaryDisplacementPanel::collectRenderPositions(QVector<QVector3D>& outPositions)
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
                if (auto s = dynamic_cast<mNode*>(setBase)) { auto v = s->getNodeData(); auto& d = merged[PickDataType::Node]; d.insert(d.end(), v.begin(), v.end()); }
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
            case PickDataType::Node:
                for (int id : ids)
                {
                    QVector3D pos = meshData->getVectorById(id);
                    if (!pos.isNull())
                        outPositions.push_back(pos);
                }
                break;
            case PickDataType::GeoPoint:
            {
                auto points = geoManager->CalculateVertexAttr(ids);
                for (auto p : points) outPositions.push_back(p.m_coor);
                break;
            }
            case PickDataType::GeoEdge:
            {
                QVector<int> pids; for (int id : ids) pids.append(id);
                auto coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(1, pids);
                for (auto c : coords) outPositions.push_back(c);
                break;
            }
            case PickDataType::GeoFace:
            {
                QVector<int> pids; for (int id : ids) pids.append(id);
                auto coords = getGEOServer.geoCalculate().getAllPointCoordinateByID(2, pids);
                for (auto c : coords) outPositions.push_back(c);
                break;
            }
            case PickDataType::GeoSolid:
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

    void mBoundaryDisplacementPanel::updateBoundarySymbolRealtime()
    {
        if (!_pickWidgetController || !_currentBoundary)
            return;
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;

        // 通过防抖定时器合并连续触发
        if (!_realtimeUpdateTimer)
            return;
        _realtimeUpdateTimer->start(100);
    }

    void mBoundaryDisplacementPanel::doUpdateBoundarySymbolRealtime()
    {
        const int mode = _comboDefinitionMode.combobox->currentIndex();
        auto render = AllRenderSymbolManager::getInstance();

        // 独立赋值：先按离散数据控件当前行同步 _independentTable（数据源为表格，
        // 通用拾取控制器已不再接收视口拾取，不能再以它为准）
        if (mode == 1)
        {
            updateOpenedTable();
        }

        if (mode == 0)
        {
            // 各向分量：所有选中位置共用同一组分量
            QVector<QVector3D> positions;
            collectRenderPositions(positions);

            QMap<uint32_t, float> value;
            auto addComp = [&](int idx, int key) {
                if (InputWidgets[idx].checkbox->isChecked())
                {
                    QString txt = InputWidgets[idx].lineeditvalue->text().trimmed();
                    if (!txt.isEmpty())
                        value[key] = txt.toFloat();
                }
                };
            addComp(0, 1);
            addComp(1, 2);
            addComp(2, 3);
            addComp(3, 4);
            addComp(4, 5);
            addComp(5, 6);

            // 最终数据签名：边界ID+模式+位置集+分量值未变化时跳过符号赋值与
            // UpdateAllSymbol 全量重建（进入面板防抖触发/切出冲刷时数据往往与上次相同）
            size_t sig = AllRenderSymbolManager::beginRenderSignature();
            AllRenderSymbolManager::combineRenderSignature(sig, static_cast<size_t>(_boundaryData->getLoadBoundaryId()));
            AllRenderSymbolManager::combineRenderSignature(sig, static_cast<size_t>(mode));
            AllRenderSymbolManager::combineRenderSignature(sig, positions);
            AllRenderSymbolManager::combineRenderSignature(sig, value);
            if (sig == _lastRenderSig)
            {
                return;
            }
            _lastRenderSig = sig;

            // 清除独立赋值可能留下的分量 symbol，再创建标准 symbol
            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            if (symbolController)
            {
                std::vector<QString> symbolsToDelete = AllRenderSymbolManager::getBoundarySymbolNames(_boundaryData->getLoadBoundaryId());
                symbolController->deleteSymbol(symbolsToDelete);
            }

            if (!positions.isEmpty() && !value.isEmpty())
            {
                // 与集中力相同的箭头符号：U1/UR1->X、U2/UR2->Y、U3/UR3->Z，方向随输入数值正负翻转
                render->assignBoundaryDisplacement(_boundaryData, positions, value, QVector3D(255.0f, 165.0f, 0.0f));
            }
        }
        else
        {
            // 独立赋值：按节点-分量精确渲染，统一使用 AllRenderSymbolManager 接口
            if (_currentBoundary)
            {
                // 最终数据签名：边界ID+模式+独立表未变化时跳过符号赋值与全量重建
                size_t sig = AllRenderSymbolManager::beginRenderSignature();
                AllRenderSymbolManager::combineRenderSignature(sig, static_cast<size_t>(_boundaryData->getLoadBoundaryId()));
                AllRenderSymbolManager::combineRenderSignature(sig, static_cast<size_t>(mode));
                AllRenderSymbolManager::combineRenderSignature(sig, _currentBoundary->_independentTable);
                if (sig == _lastRenderSig)
                {
                    return;
                }
                _lastRenderSig = sig;

                render->assignBoundaryIndependentDisplacement(_boundaryData, _currentBoundary->_independentTable, QVector3D(255.0f, 165.0f, 0.0f));
            }
        }

        auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
        if (symbolController) symbolController->UpdateAllSymbol();
        getCVM->getPreRenderWidget()->updatePaint();
    }

    void mBoundaryDisplacementPanel::deactivatePickAndCommit()
    {
        // 面板为单例复用，切换边界/面板时不会走析构/clearInputs，
        // 由统一退出出口（unEditBoundary）调用，失活并提交当前拾取数据。
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
            doUpdateBoundarySymbolRealtime();
        }
    }

    std::map<RenderPickResultData, QVector<QVariant>> mBoundaryDisplacementPanel::convertIndependentTable(
        const QMap<int, QMap<int, QVariant>>& cache)
    {
        std::map<RenderPickResultData, QVector<QVariant>> data;
        for (auto it = cache.begin(); it != cache.end(); ++it)
        {
            int nodeId = it.key();
            const QMap<int, QVariant>& dirMap = it.value();
            QVector<QVariant> values(6);
            for (auto dirIt = dirMap.begin(); dirIt != dirMap.end(); ++dirIt)
            {
                int dof = dirIt.key();
                if (dof >= 1 && dof <= 6)
                    values[dof - 1] = dirIt.value();
            }
            data[RenderPickResultData(nodeId, -1, -1)] = std::move(values);
        }
        return data;
    }

    QMap<int, QMap<int, QVariant>> mBoundaryDisplacementPanel::convertIndependentTable(
        const std::map<RenderPickResultData, QVector<QVariant>>& data)
    {
        QMap<int, QMap<int, QVariant>> cache;
        for (const auto& entry : data)
        {
            int nodeId = entry.first.result_id;
            QMap<int, QVariant>& dirMap = cache[nodeId];
            const QVector<QVariant>& values = entry.second;
            for (int i = 0; i < values.size() && i < 6; ++i)
            {
                if (values[i].isValid() && !values[i].isNull())
                {
                    dirMap[i + 1] = values[i];
                }
            }
        }
        return cache;
    }

    void mBoundaryDisplacementPanel::clearInputs()
    {
        for (auto inp : InputWidgets)
        {
            inp.clear();
        }
        InputWidgets.clear();

        if (_pickWidgetController)
        {
            _pickWidgetController->setPickActive(false);
            delete _pickWidgetController;
            _pickWidgetController = nullptr;
        }

        _currentBoundary.reset();

        _comboDefinitionMode.clear();
        _functable.clear();

        if (_tableRowWidget)
        {
            delete _tableRowWidget;
            _tableRowWidget = nullptr;
        }
        _tableLabel = nullptr;
        _discreteControl = nullptr; // 随 _tableRowWidget 一并删除

        _scale.clear();

        if (_realtimeUpdateTimer)
        {
            delete _realtimeUpdateTimer;
            _realtimeUpdateTimer = nullptr;
        }
    }
}