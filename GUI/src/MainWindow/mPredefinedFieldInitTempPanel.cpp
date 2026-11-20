#include "mPredefinedFieldInitTempPanel.h"
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

#include "mPredefinedFieldInitTemp.h"
#include "AllRenderSymbolManager.h"

using namespace MechCommon;
using namespace MechPreData;
namespace MechMainWindow
{
    mPredefinedFieldInitTempPanel::mPredefinedFieldInitTempPanel(QWidget* parent)
        :mPredefinedFieldPanelBase(parent)
    {
        _realtimeUpdateTimer = new QTimer(this);
        _realtimeUpdateTimer->setSingleShot(true);
        connect(_realtimeUpdateTimer, &QTimer::timeout, this, &mPredefinedFieldInitTempPanel::onRealtimeUpdateTimeout);

        initInputs();
    }

    mPredefinedFieldInitTempPanel::~mPredefinedFieldInitTempPanel()
    {
        clearInputs();
    }

    std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> mPredefinedFieldInitTempPanel::getPickedData()
    {
        if (!_pickWidgetController)
            return std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>();
        return _pickWidgetController->getAllData().pickedData;
    }

    void mPredefinedFieldInitTempPanel::updateOpenedTable()
    {
        if (!_currentField || !_pickWidgetController || !_discreteControl)
        {
            return;
        }

        // 独立赋值模式下，_nodeTempMap 与离散数据控件的行数据保持严格一致：
        // 表格是数据的权威来源（含拾取追加与手动删除），此处按控件当前行重建 map，
        // 保留已填温度值；key 统一规范为 (result_id, -1, -1)，与 mIMxFile 读入键一致。
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

    void mPredefinedFieldInitTempPanel::filterPicksToNodeOnly()
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

        // 独立赋值模式无需集合：把控件数据重设为"仅节点拾取、无集合"。
        _pickWidgetController->replacePickedDataAndClearSets(picks);
        // 独立赋值已完全剥离通用拾取：开启云图抑制模式，彻底关闭通用拾取控件的高亮渲染，
        // 拾取由离散数据控件直接驱动视口选择容器。
        _pickWidgetController->setCloudChartMode(true);

        updateOpenedTable();
    }

    void mPredefinedFieldInitTempPanel::initInputs()
    {
        // 定义方式下拉框
        _defineTypeCombo.initWidget("定义方式");
        _defineTypeCombo.combobox->addItem("统一赋值");
        _defineTypeCombo.combobox->addItem("独立赋值");
        connect(_defineTypeCombo.combobox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &mPredefinedFieldInitTempPanel::slot_defineTypeChanged);

        // 统一赋值温度输入
        _inp.initWidget("初始温度", false);
        connect(_inp.lineeditvalue, &mCalcLineEdit::editingFinished,
            [this]() {
                updateLoadSymbolRealtime();
                saveTemp();
            });

        // 独立赋值表格行：通用离散数据控件（入口按钮 + 锚定弹窗表格）
        _tableRowWidget = new QWidget();
        QHBoxLayout* tableRowLayout = new QHBoxLayout(_tableRowWidget);
        tableRowLayout->setContentsMargins(0, 0, 0, 0);
        tableRowLayout->setSpacing(8);

        _tableLabel = new QLabel("初始温度");
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
            QStringLiteral("初始温度"),
            { QStringLiteral("温度值") },
            &nodeSpec, _tableRowWidget);
        // 初始温度沿用既有实时符号渲染，不显示渲染按钮（默认隐藏）
        _discreteControl->setRenderButtonVisible(false);
        connect(_discreteControl, &mDiscreteDataControl::dataChanged,
            this, &mPredefinedFieldInitTempPanel::slot_discreteDataChanged);
        connect(_discreteControl, &mDiscreteDataControl::cellEdited,
            this, &mPredefinedFieldInitTempPanel::slot_discreteCellEdited);
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
            // 其 dataChanged 只在统一赋值模式下触发，无需按方式分支重建表格。
            updateLoadSymbolRealtime();
            });

        _pickWidgetController->setSetResolver(&resolveSetToTypedData);
        _pickWidgetController->setDefultTip("选择对象");
    }

    void mPredefinedFieldInitTempPanel::slot_defineTypeChanged(int idx)
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

        // 独立赋值模式：拾取由离散数据控件直接驱动，通用拾取控件不再激活。
        if (_pickWidgetController)
        {
            if (idx == 1)
            {
                _pickWidgetController->setPickWidgetVisible(false);
            }
            else
            {
                _pickWidgetController->setPickWidgetVisible(true);
                _pickWidgetController->setPickActive(true);
            }
        }

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
            }
            else // 独立赋值
            {
                faval_->addRow(_tableRowWidget);
                _tableRowWidget->show();
            }

            faval_->refreshLayout();
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
        else // 统一赋值：显示集合选择按钮，恢复正常高亮渲染
        {
            if (_pickWidgetController)
            {
                _pickWidgetController->setSelectSetBtnVisible(true);
                _pickWidgetController->setCloudChartMode(false);
            }
            updateLoadSymbolRealtime();
        }

        // 独立赋值不显示作用对象折叠框，定义方式切换导致折叠框数量变化，由外部重建面板
        emit sg_rebuildPanel();
    }

    void mPredefinedFieldInitTempPanel::slot_discreteDataChanged()
    {
        // 离散数据控件行变化（拾取追加/删除/清空/分量增减）：
        // 按控件当前行重建 _nodeTempMap，随后刷新实时符号并通知外部。
        // 拾取会话进行中跳过：确认拾取（完成拾取）后控件会补发一次 dataChanged，
        // 此时才执行重建与渲染，避免拾取界面期间反复重绘。
        if (!_currentField || (_discreteControl && _discreteControl->isPicking()))
        {
            return;
        }
        updateOpenedTable();
        updateLoadSymbolRealtime();
        notifyLoadChanged();
    }

    void mPredefinedFieldInitTempPanel::slot_discreteCellEdited(const QPair<int, int>& key, int valueIndex, const QString& text)
    {
        if (!_currentField || key.first <= 0)
        {
            return;
        }
        // 单点写回：仅更新/清除该节点的温度值，节点本身始终保留。
        // 初始温度为单分量，key 规范为 result_id/-1/-1，与 mIMxFile 读入键一致。
        const RenderPickResultData fieldKey(key.first, -1, -1);
        QVector<QVariant>& values = _currentField->_nodeTempMap[fieldKey];
        if (values.size() < 1)
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
        notifyLoadChanged();
    }

    void mPredefinedFieldInitTempPanel::slot_tempChanged()
    {
        saveTemp();
    }

    void mPredefinedFieldInitTempPanel::saveTemp()
    {
        if (!_currentField)
        {
            return;
        }
        _currentField->_temp = _inp.lineeditvalue->text().trimmed();
        notifyLoadChanged();
    }

    void mPredefinedFieldInitTempPanel::saveWayAndClearModeFields()
    {
        if (!_currentField)
        {
            return;
        }
        _currentField->_defineType = _defineTypeCombo.combobox->currentIndex();
        // 重置所有模式字段，避免交叉污染
        _currentField->_temp = "";
        _currentField->_nodeTempMap.clear();
    }

    void mPredefinedFieldInitTempPanel::notifyLoadChanged()
    {
        if (!_fieldData)
        {
            return;
        }
        // 仅通知外部刷新树/渲染，不再调用 storeFieldValue 全量保存
        getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_PredefinedField, _fieldData->getPredefinedFieldId());
    }

    void mPredefinedFieldInitTempPanel::applyAllowedPickTypes(int way)
    {
        if (!_pickWidgetController) return;
        if (way == 1) // 独立赋值：仅设置白名单记录，不切换视口过滤器
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

    void mPredefinedFieldInitTempPanel::deactivatePickAndCommit()
    {
        // 面板为单例复用，切换载荷/面板时不会走析构/clearInputs，
        // 由统一退出出口（unEditPredefinedField）调用，失活并提交当前拾取数据。
        const bool renderValid = (getCVM && getCVM->getPreRenderWidget());

        // 离散数据控件：结束拾取并关闭弹窗
        if (_discreteControl)
        {
            _discreteControl->closePopup();
        }

        // 取消挂起的防抖刷新：切出后符号显示状态已由树选中刷新统一重算，
        // 再触发一次符号全量重建（UpdateAllSymbol）纯属浪费，大模型下明显卡顿
        if (_realtimeUpdateTimer)
        {
            _realtimeUpdateTimer->stop();
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

        // 失活提交可能发出 dataChanged 重新启动防抖定时器（最近 100ms 内有拾取变更）：
        // 同步把最终数据写入符号对象（仅赋值不重建），随后切入的面板/树选中刷新
        // 会统一执行重建；切出到预定义场父节点等仍显示本符号的路径数据保持正确
        if (_realtimeUpdateTimer && _realtimeUpdateTimer->isActive())
        {
            _realtimeUpdateTimer->stop();
            doUpdateLoadSymbolRealtime(true);
        }
    }

    void mPredefinedFieldInitTempPanel::updateLoadSymbolRealtime()
    {
        if (!_pickWidgetController || !_currentField)
            return;
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;

        // 通过防抖定时器合并连续触发（输入/拾取变化），避免每输入一个字符都重算并重绘。
        if (!_realtimeUpdateTimer)
            return;
        _realtimeUpdateTimer->start(100);
    }

    void mPredefinedFieldInitTempPanel::onRealtimeUpdateTimeout()
    {
        doUpdateLoadSymbolRealtime();
    }

    void mPredefinedFieldInitTempPanel::doUpdateLoadSymbolRealtime(bool assignOnly)
    {
        if (!_pickWidgetController || !_currentField)
            return;
        if (!getCVM || !getCVM->getPreRenderWidget())
            return;

        const int way = _defineTypeCombo.combobox->currentIndex();

        // 独立赋值：先按离散数据控件当前行同步 _nodeTempMap（数据源为表格，
        // 通用拾取控制器已不再接收视口拾取，不能再以它为准），再取坐标渲染。
        if (way == 1)
        {
            updateOpenedTable();
        }

        // 1. 汇总当前数据：拾取(pickedData) + 集合(selectedSetNames 按名称展开），合并为按类型分组的 map。
        const PickWidgetData allData = _pickWidgetController->getAllData();
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> merged;
        if (way == 0)
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
        // 集合展开仅统一赋值需要；独立赋值以表格数据（_nodeTempMap）为准
        if (way == 0)
        {
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
        }

        // 独立赋值：直接以表格数据（_nodeTempMap）为准取节点坐标，
        // 不再遍历 _pickWidgetController->getAllData()（控制器数据已停滞）。
        if (way == 1)
        {
            auto& vec = merged[MxFunctions::PickDataType::Node];
            vec.reserve(_currentField->_nodeTempMap.size());
            for (const auto& entry : _currentField->_nodeTempMap)
            {
                vec.push_back(entry.first);
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
            case MxFunctions::PickDataType::Node:
                for (int id : ids)
                {
                    QVector3D pos = meshData->getVectorById(id);
                    if (!pos.isNull())
                        objs.push_back(pos);
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

        // 3. 重画该预定义场符号（初始温度使用空标识）
        // 符号数据未变化时跳过 UpdateAllSymbol：进入面板（树选中刷新已重建过）
        // 与连续防抖触发时数据往往不变，全量重建所有可见符号在大模型下代价很高
        const bool changed = AllRenderSymbolManager::getInstance()->assignPredefinedField(
            _currentField, objs, "", QVector3D(255.0f, 165.0f, 0.0f));

        if (changed && !assignOnly)
        {
            auto symbolController = getCVM->getPreRenderWidget()->GetSymbolController();
            if (symbolController) symbolController->UpdateAllSymbol();
            getCVM->getPreRenderWidget()->updatePaint();
        }
    }

    void mPredefinedFieldInitTempPanel::clearInputs()
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
        _defineTypeCombo.clear();
        _currentField.reset();

        if (_tableRowWidget)
        {
            delete _tableRowWidget;
            _tableRowWidget = nullptr;
        }
        _tableLabel = nullptr;
        _discreteControl = nullptr; // 随 _tableRowWidget 一并删除
    }

    void mPredefinedFieldInitTempPanel::setInputsValue(std::shared_ptr<MechCaseData::mPredefinedFieldBase> fieldData)
    {
        _isSettingValue = true;

        std::shared_ptr<MechCaseData::mPredefinedFieldInitTemp> fieldPtr = std::dynamic_pointer_cast<MechCaseData::mPredefinedFieldInitTemp>(fieldData);
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

        // 反显定义方式
        _defineTypeCombo.combobox->setCurrentIndex(fieldPtr->_defineType);

        // 反显温度值
        _inp.lineeditvalue->setExpression(fieldPtr->_temp);

        // 作用对象反显
        // 单例面板复用：无论新载荷定义方式如何，先清空离散表格残留行，
        // 避免旧载荷独立赋值数据在用户切回独立赋值时复现并污染新载荷
        if (_discreteControl)
        {
            _discreteControl->setRows({});
        }
        PickWidgetData displayData;
        auto& mapfield = getCVM->getModelData()->getMapRelationManager();
        int setid = mapfield.getMapRelation<int>({ Physical_PredefinedField, fieldData->getPredefinedFieldId() }, Physical_Set);
        if (setid != -1 && fieldPtr->_defineType == 0)
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
                displayData.pickedData[MxFunctions::PickDataType::Node] = std::move(nodePicks);
            }

            // 反显到离散数据控件（节点 + 已填温度值）
            if (_discreteControl)
            {
                QVector<mDiscreteDataControl::Row> rows;
                rows.reserve(static_cast<int>(fieldPtr->_nodeTempMap.size()));
                for (const auto& entry : fieldPtr->_nodeTempMap)
                {
                    const QVariant v = entry.second.isEmpty() ? QVariant() : entry.second.first();
                    QStringList values;
                    values << ((v.isValid() && !v.isNull()) ? QString::number(v.toDouble()) : QString());
                    rows.append(qMakePair(qMakePair(entry.first.result_id, -1), values));
                }
                _discreteControl->setRows(rows);
            }
        }

        if (_pickWidgetController)
        {
            if (fieldPtr->_defineType == 1)
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
            _pickWidgetController->setSelectSetBtnVisible(fieldPtr->_defineType != 1);
        }

        if (fieldPtr->_defineType == 1)
        {
            filterPicksToNodeOnly();
        }

        // 解除信号阻塞
        _defineTypeCombo.combobox->blockSignals(false);

        _isSettingValue = false;

        updateOpenedTable();
        updateLoadSymbolRealtime();
    }

    QVector<QWidget*> mPredefinedFieldInitTempPanel::getTargetInputs(FoldSection* fathertar)
    {
        fatar_ = fathertar;
        QVector<QWidget*>wds;
        if (_pickWidgetController)
        {
            wds.append(_pickWidgetController->widget());
        }
        return wds;
    }

    QVector<QWidget*> mPredefinedFieldInitTempPanel::getValueInputs(FoldSection* fatherval)
    {
        faval_ = fatherval;
        QVector<QWidget*>wds;

        // 定义方式下拉框始终显示在最前面
        wds.append(_defineTypeCombo.propwid);

        int defineType = _defineTypeCombo.combobox->currentIndex();

        if (defineType == 0) // 统一赋值
        {
            wds.append(_inp.propwid);
        }
        else // 独立赋值
        {
            wds.append(_tableRowWidget);
        }

        return wds;
    }

    void mPredefinedFieldInitTempPanel::storeFieldValue(std::shared_ptr<MechCaseData::mPredefinedFieldBase> fieldData)
    {
        if (fieldData == nullptr)
        {
            return;
        }

        std::shared_ptr<MechCaseData::mPredefinedFieldInitTemp> fieldPtr = std::dynamic_pointer_cast<MechCaseData::mPredefinedFieldInitTemp>(fieldData);
        if (!fieldPtr) return;

        int defineType = _defineTypeCombo.combobox->currentIndex();
        fieldPtr->_defineType = defineType;

        if (defineType == 0) // 统一赋值
        {
            fieldPtr->_temp = _inp.lineeditvalue->text().trimmed();
        }
        // 独立赋值模式下 _nodeTempMap 由表格实时维护，无需在此全量保存
    }
}
