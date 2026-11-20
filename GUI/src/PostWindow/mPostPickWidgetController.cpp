#include <mMeshData.h>
#include "mPostPickWidgetController.h"

// =============================================================================
// 复制自 src/MainWindow/mPickWidgetController.cpp（前处理拾取控件栈，dev_base@8d278d784）
// 后处理适配（非逐字复制）：渲染窗口换 getPostRenderWidget()；捕获改读后处理共享
// 选中容器；其余逻辑（互斥/提交/高亮形态/视口切换联动）保持一致。适配点见头文件说明。
// 同步守则：前处理侧对应文件的结构/逻辑调整须对照同步到本文件（适配点除外），不得单侧漂移。
// =============================================================================

#include <QApplication>
#include <algorithm>

#include "mMechGlobalSignals.h"
#include "PickCommonFunction.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"

using namespace MechPreData;
using namespace MechCommon;
namespace PostWindow
{
    // 静态成员初始化（QPointer 控制器销毁自动置空）
    QPointer<mPostPickWidgetController> mPostPickWidgetController::s_activeController;
    int mPostPickWidgetController::s_nextInstanceId = 0;
    const PickWidgetData mPostPickWidgetController::s_emptyPickWidgetData;

    mPostPickWidgetController::mPostPickWidgetController(std::shared_ptr<mPostPickHighlightServiceBase> highlightService, mPostPickWidgetViewBase* view, QWidget* parent)
        : QObject(parent)
        , _model(std::make_unique<mPostPickWidgetDataModel>())
        , _view(view), _highlightService(highlightService)
    {
        // 初始化通用逻辑，传入颜色块前缀
        QString colorKey = _view ? _view->colorBlockKey() : "CBDefault";
        initControllerCommon(colorKey);
    }

    mPostPickWidgetController::~mPostPickWidgetController()
    {
        if (s_activeController == this)
            s_activeController = nullptr;
    }

    // ------------------------------
    // 通用初始化
    // ------------------------------
    void mPostPickWidgetController::initControllerCommon(const QString& colorKeyPrefix)
    {
        if (!_view) return;

        // 生成唯一colorKey，避免同面板多控件高亮冲突
        _uniqueColorKey = QString("%1_%2").arg(colorKeyPrefix).arg(s_nextInstanceId++);
        _highlightService->setRenderWidget(getCVM ? getCVM->getPostRenderWidget() : nullptr);

        // 连接UI信号
        connectUiSignals();

        // 监听显示事件：切出面板时高亮被统一清空（_highlightForm=None），
        // 切回面板后由显示事件自动恢复本控件应有的高亮形态
        _view->installEventFilter(this);

        // 视口/模型切换联动：多模型多视口切换时，提交落盘模型、拾取连接、高亮
        // 必须跟随视口所属模型，否则出现旧模型拾取数据写入新模型同ID载荷/边界、
        // 拾取仍监听旧视口选择容器等数据错乱
        // DirectConnection：切换前通知必须在管理器更新当前模型指针前同步完成提交
        connect(mMechGlobalSignals::getInstance(), &mMechGlobalSignals::viewportModelAboutToChange,
            this, &mPostPickWidgetController::onViewportModelAboutToChange, Qt::DirectConnection);
        connect(VMManager, &mViewportModelManager::currentViewportChanged,
            this, &mPostPickWidgetController::onViewportModelSwitched, Qt::DirectConnection);
    }

    // ------------------------------
    // 连接UI通用信号
    // ------------------------------
    void mPostPickWidgetController::connectUiSignals()
    {
        if (!_view) return;

        connect(_view, &mPostPickWidgetViewBase::activatePick, this, &mPostPickWidgetController::onUiActivated);
        // post 增补：点击已激活控件本体=停止拾取（视图基类 deactivatePick，开关语义）
        connect(_view, &mPostPickWidgetViewBase::deactivatePick, this, [this]() {
            if (_isActive)
                setPickActive(false);
            });
        connect(_view, &mPostPickWidgetViewBase::clearAllRequested, this, &mPostPickWidgetController::onUiClear);
        connect(_view, &mPostPickWidgetViewBase::selectSetChanged, this, &mPostPickWidgetController::onUiSelectSetChanged);
        connect(_view, &mPostPickWidgetViewBase::removePickRequested, this, &mPostPickWidgetController::onUiRemovePickRequested);
        connect(_view, &mPostPickWidgetViewBase::removePickRequestedByType, this, &mPostPickWidgetController::onUiRemovePickRequestedByType);
        connect(_view, &mPostPickWidgetViewBase::calcMainNodeRequested, this, &mPostPickWidgetController::onUiCalcMainNode);
    }

    void mPostPickWidgetController::setDefultTip(const QString& str, const QString& topStr)
    {
        if (_view) _view->setDefultTip(str, topStr);
    }

    void mPostPickWidgetController::setSelectSetBtnVisible(bool visible)
    {
        if (_view) _view->setSelectSetBtnVisible(visible);
    }

    void mPostPickWidgetController::replacePickedDataAndClearSets(const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& data)
    {
        if (!_model) return;
        _model->clear();
        _model->setPickedData(data);
        markDirtyAndNotify();
    }

    void mPostPickWidgetController::setPickActive(bool active)
    {
        if (_isActive == active) return;

        // 隐藏状态下不允许被激活，避免隐藏控件仍占用全局互斥或产生高亮。
        if (active && !_isVisible)
            return;

        // 禁用状态下不允许被激活（如所在折叠框未勾选），禁用控件不可响应任何操作。
        if (active && _view && !_view->widget()->isEnabled())
            return;

        // 全局互斥：激活前先失活其他控件
        if (active && s_activeController != nullptr && s_activeController != this)
        {
            s_activeController->setPickActive(false);
        }

        _isActive = active;
        if (active)
            s_activeController = this;
        else if (s_activeController == this)
            s_activeController = nullptr;

        if (_view)
            _view->setPickActive(active);

        if (active)
        {
            setPickType();
            //emit requestActivate();
            emit pickActiveChanged(true);
            applyHighlight(true);
            connectViewportSignal();
        }
        else
        {
            // 绑定数据已删（数据对象先于拾取会话销毁，如未退出编辑即删除模型对象）：
            // 跳过 commit() 与 pickActiveChanged(false)——面板失活槽普遍回写数据
            // 指针，已无安全对象可写；仅做机械摘除（全局注册已在上方解除）
            const bool alive = bindingAlive();
            if (alive)
                commit();
            deactivateMechanical();
            if (alive)
                emit pickActiveChanged(false);
        }
    }

    void mPostPickWidgetController::detachActive()
    {
        if (!_isActive)
            return;
        _isActive = false;
        if (s_activeController == this)
            s_activeController = nullptr;
        if (_view)
            _view->setPickActive(false);
        deactivateMechanical();
    }

    void mPostPickWidgetController::deactivateMechanical()
    {
        disconnectViewportSignal();
        if (_keepHighlightOnDeactivate)
            applyHighlight(false);
        else
        {
            if (_highlightService)
            {
                _highlightService->setColorKey(uniqueColorKey());
                _highlightService->clearAllHighlight();
            }
            _highlightForm = HighlightForm::None;
        }
    }

    void mPostPickWidgetController::mergeInto(std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& target, const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& source)
    {
        for (const auto& pair : source)
        {
            if (pair.second.empty()) continue;

            auto& vec = target[pair.first];
            std::unordered_set<RenderPickResultData, RenderPickResultDataHash, RenderPickResultDataEqual> seen;
            seen.reserve(vec.size() + pair.second.size());
            for (const auto& existing : vec)
                seen.insert(existing);
            for (const auto& item : pair.second)
            {
                if (seen.insert(item).second)
                    vec.push_back(item);
            }
        }
    }

    void mPostPickWidgetController::setPickWidgetVisible(bool visible)
    {
        if (_isVisible == visible)
            return;
        _isVisible = visible;

        if (_view)
        {
            // 面板切换期（clearFoldWidgets 已把控件摘为无父、loadPanel 尚未重挂）
            // 只记录可见标志不实际显示：无父 QWidget 的 setVisible(true) 会以
            // 顶层窗口形式短暂弹出（弹窗闪现），控件由宿主重挂折叠框后统一 show
            if (!visible || _view->parentWidget() != nullptr)
                _view->setVisible(visible);
        }

        if (!visible)
        {
            // 隐藏时：若处于激活态，先清空拾取高亮再失活；
            // 若处于未激活态，仅清空可能存在的临时高亮。
            if (_isActive)
            {
                clearAllHighlight();
                // 失活会触发 commit，保证已拾取数据被持久化；
                // 同时 setPickActive(false) 内部会根据 keepHighlight 逻辑处理高亮，
                // 但上面已统一清空，此处不再重复渲染。
                setPickActive(false);
            }
            else
            {
                clearAllTempHighlight();
            }
        }
    }

    void mPostPickWidgetController::setPickMode(mPickMode pickMode, bool clearWhenSwitch /*= false*/)
    {
        if (_view) _view->setPickMode(pickMode, clearWhenSwitch);
    }

    bool mPostPickWidgetController::eventFilter(QObject* watched, QEvent* event)
    {
        // 面板重新显示（控件由隐藏转为可见）时恢复高亮形态：
        // 激活态恢复正式高亮；失活但有拾取数据的恢复临时高亮。
        // 切出面板时高亮被统一 clearAllHighlight 清空（形态置 None），
        // 若不在此恢复，须逐个控件"激活再切走"才能找回临时高亮
        if (watched == _view.data() && event->type() == QEvent::Show)
        {
            if (_highlightForm == HighlightForm::None)
            {
                applyHighlight(_isActive);
            }
        }
        return QObject::eventFilter(watched, event);
    }

    void mPostPickWidgetController::setAllowedTypes(std::function<QSet<MxFunctions::PickDataType> ()> getSetype)
    {
        _getSetype = getSetype;
    }

    void mPostPickWidgetController::setPickFilter(QSet<int> filter)
    {
        _elementFilter = std::move(filter);
    }

    void mPostPickWidgetController::setAllPickTypesDisabled(bool disabled)
    {
        emit MxFunctions::mGlobalSignals::getInstance()->setPickTypesEnabledSig(
            QVector<bool>(10, !disabled));
    }

    QString mPostPickWidgetController::buildAllowedTypesTip(const QSet<MxFunctions::PickDataType>& types)
    {
        if (types.isEmpty())
            return QString();

        QStringList typeNames;
        const QVector<MxFunctions::PickDataType> order = {
            PickDataType::GeoPoint, PickDataType::GeoEdge, PickDataType::GeoFace, PickDataType::GeoSolid,
            PickDataType::Node, PickDataType::Element, PickDataType::ElementFace, PickDataType::ElementEdge,
            PickDataType::Part
        };
        for (MxFunctions::PickDataType type : order)
        {
            if (types.contains(type))
                typeNames.append(PickDataTypeToString(type));
        }
        return QString("可拾取：%1").arg(typeNames.join("、"));
    }

    void mPostPickWidgetController::displayData(const PickWidgetData& data, bool highlight)
    {
        if (!_model) return;
        _model->clear();
        _model->setSelectedSetNames(data.selectedSetNames);
        _model->setPickedData(data.pickedData);
        _highlightForm = HighlightForm::None;

        if (highlight) refreshAll();
        else refreshUI();
    }

    void mPostPickWidgetController::refreshAll()
    {
        refreshUI();
        applyHighlight(_isActive);
    }

    void mPostPickWidgetController::clearAllHighlight()
    {
        if (_highlightService)
        {
            _highlightService->setColorKey(uniqueColorKey());
            _highlightService->clearAllHighlight();
        }
        _highlightForm = HighlightForm::None;
    }

    void mPostPickWidgetController::clearAllTempHighlight()
    {
        if (_highlightService)
        {
            _highlightService->setColorKey(uniqueColorKey());
            _highlightService->clearOwnTempHighlight();
        }
        _highlightForm = HighlightForm::None;
    }

    QColor mPostPickWidgetController::highlightColor() const
    {
        return _view ? _view->colorBlockColor() : QColor("#2C4FFF");
    }

    void mPostPickWidgetController::refreshHighlight()
    {
        if (!_highlightService || !_model) return;

        const auto& data = getAllData();
        const auto setData = resolveSelectedSets(data.selectedSetNames);

        _highlightService->setColorKey(uniqueColorKey());
        _highlightService->setColor(highlightColor());
        _highlightService->clearAllHighlight();
        if (!setData.empty())
            _highlightService->highlightSet(setData, true);
        if (!data.pickedData.empty())
            _highlightService->highlightPicked(data.pickedData, true);
    }

    void mPostPickWidgetController::setSetResolver(std::function<std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>(const QString&)> resolver)
    {
        _setResolver = std::move(resolver);
    }

    void mPostPickWidgetController::setSelectSetList(const QHash<QString, QString>& setToType)
    {
        if (_view) _view->setSelectSetList(setToType);
    }

    void mPostPickWidgetController::setPersistenceStrategy(std::shared_ptr<IPostPickPersistenceStrategy> strategy)
    {
        _persistenceStrategy = std::move(strategy);
    }

    void mPostPickWidgetController::commitActivePickWidget()
    {
        // 绑定数据已删时跳过提交（无安全对象可写），QPointer 已保证控制器本体有效
        if (s_activeController && s_activeController->bindingAlive())
            s_activeController->commit();
    }

    void mPostPickWidgetController::commit()
    {
        if (!_model) return;

        const auto& allData = getAllData();
        if (!_dirty)
        {
            return;
        }

        if (allData.isEmpty())
        {
            if (_persistenceStrategy)
                _persistenceStrategy->save({});
            _dirty = false;
            return;
        }

        const bool singleSetOnly = (allData.selectedSetNames.size() == 1 && allData.pickedData.empty());
        QString singleSetName;
        if (singleSetOnly)
            singleSetName = *allData.selectedSetNames.begin();

        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> finalData;
        if (singleSetOnly)
        {
            mergeInto(finalData, resolveSet(singleSetName));
        }
        else
        {
            mergeInto(finalData, allData.pickedData);
            mergeInto(finalData, resolveSelectedSets(allData.selectedSetNames));
        }

        if (_persistenceStrategy)
        {
            if (singleSetOnly)
                _persistenceStrategy->saveSingleSet(singleSetName);
            else
                _persistenceStrategy->save(finalData);
        }

        if (singleSetOnly)
        {
            _model->setPickedData({});
        }
        else
        {
            _model->clear();
            _model->setPickedData(finalData);
        }

        _dirty = false;
        refreshUI();

        //防止存储时发送数据改变信号，导致连接删除自由节点
        if (!singleSetOnly && allData.selectedSetNames.size() > 0)
        {
            emit dataChanged();
        }
    }

    std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> mPostPickWidgetController::resolveSet(const QString& setName) const
    {
        return _setResolver ? _setResolver(setName)
            : std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>{};
    }

    std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> mPostPickWidgetController::resolveSelectedSets(const QSet<QString>& selectedSetNames) const
    {
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> result;
        if (!_setResolver) return result;

        for (const QString& setName : selectedSetNames)
        {
            const auto typedData = resolveSet(setName);
            for (const auto& pair : typedData)
            {
                auto& target = result[pair.first];
                target.insert(target.end(), pair.second.begin(), pair.second.end());
            }
        }
        return result;
    }

    void mPostPickWidgetController::setPickType()
    {
        //未设置白名单回调视为"不限制拾取类型"：不能直接返回，
        //否则沿用上一个激活控件白名单留下的工具栏置灰状态，部分拾取类型将无法激活
        QSet<MxFunctions::PickDataType> types = _getSetype ? _getSetype() : QSet<MxFunctions::PickDataType>();
        if (_view && !types.isEmpty())
        {
            _view->setRoundTooltipText(buildAllowedTypesTip(types));
        }

        QVector<bool> enabled(10, true);
        if (!types.isEmpty())
        {
            enabled.fill(false);
            if (types.contains(PickDataType::GeoPoint))        enabled[1] = true;
            if (types.contains(PickDataType::GeoEdge))         enabled[2] = true;
            if (types.contains(PickDataType::GeoFace))         enabled[3] = true;
            if (types.contains(PickDataType::GeoSolid))        enabled[4] = true;
            if (types.contains(PickDataType::Node))            enabled[5] = true;
            if (types.contains(PickDataType::ElementEdge))     enabled[6] = true;
            if (types.contains(PickDataType::ElementFace))     enabled[7] = true;
            if (types.contains(PickDataType::Element))         enabled[8] = true;
            if (types.contains(PickDataType::Part))            enabled[9] = true;
            enabled[0] = enabled[1] && enabled[2] && enabled[3] && enabled[4];
        }
        //拾取模式切换决策统一收敛在工具栏的 setPickTypesEnabledSig 处理器
        //（mCenterMainWindow）：本函数原有的"读当前视口模式→禁用则逐项重发
        //activePickSig"尾巴按白名单顺序发射（先 5-9 后 0-4），最终生效项
        //不可控，且与工具栏自身的禁用兜底互相覆盖，已随本次收敛移除
        emit MxFunctions::mGlobalSignals::getInstance()->setPickTypesEnabledSig(enabled);
    }

    void mPostPickWidgetController::renderPickedDataAsTempHighlight()
    {
        if (!_highlightService || !_model) return;

        const auto& data = getAllData();
        auto tempData = resolveSelectedSets(data.selectedSetNames);
        mergeInto(tempData, data.pickedData);

        _highlightService->setColorKey(uniqueColorKey());
        _highlightService->setColor(highlightColor());
        _highlightService->clearOwnTempHighlight();
        if (!data.pickedData.empty())
            _highlightService->highlightPicked(data.pickedData, false);
        if (!tempData.empty())
            _highlightService->highlightSet(tempData, true);
    }

    void mPostPickWidgetController::applyHighlight(bool active)
    {
        if (!_isVisible) return;

        const HighlightForm target = active ? HighlightForm::ActiveNormal
            : HighlightForm::InactiveTemp;
        if (target == _highlightForm) return;

        if (active)
            refreshHighlight();
        else
            renderPickedDataAsTempHighlight();

        _highlightForm = target;
    }

    // ------------------------------
    // UI槽函数
    // ------------------------------
    void mPostPickWidgetController::onUiActivated()
    {
        setPickActive(true);
    }

    void mPostPickWidgetController::onUiClear()
    {
        if (!_model) return;
        _model->clear();
        markDirtyAndNotify();
    }

    void mPostPickWidgetController::onUiSelectSetChanged(const QSet<QString>& selectedNames)
    {
        if (!_model) return;
        _model->setSelectedSetNames(selectedNames);
        markDirtyAndNotify();
    }

    void mPostPickWidgetController::onUiRemovePickRequested(const QList<PickDisplayItem>& items)
    {
        if (!_model) return;
        for (const auto& item : items)
        {
            if (item.source == PickDataSource::Set)
            {
                _model->removeSelectedSetName(item.sourceName);
            }
            else
            {
                _model->removePickedData(item.type, item.data);
            }
        }
        markDirtyAndNotify();
    }

    void mPostPickWidgetController::onUiRemovePickRequestedByType(MxFunctions::PickDataType type)
    {
        if (!_model) return;
        _model->clearPickedDataByType(type);
        markDirtyAndNotify();
    }

    void mPostPickWidgetController::onUiCalcMainNode()
    {
        emit calcMainNodeSig();
    }

    // ------------------------------
    // 视口拾取
    // ------------------------------
    std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>
        mPostPickWidgetController::captureViewportPickData() const
    {
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> data;

        // 后处理适配：直读共享选中容器的 MeshSelectMemoryData（前处理
        // SaveSelectMemoryData 只收 CCSMRendWidget*，后处理视口不可用）；
        // 部件拾取兜底由 MeshPart 选中项承担（前处理需区分几何/网格 part，后处理全为网格）
        auto rend = getCVM ? getCVM->getPostRenderWidget() : nullptr;
        if (!rend || !rend->getSelectionContainer())
            return data;
        auto* meshSelectData = rend->getSelectionContainer()->getMeshSelectData();

        const std::pair<MxFunctions::PickDataType, mxr::SelectionInfoType> typeTable[] = {
            { MxFunctions::PickDataType::Node,        mxr::SelectionInfoType::MeshNode },
            { MxFunctions::PickDataType::Element,     mxr::SelectionInfoType::MeshElement },
            { MxFunctions::PickDataType::ElementFace, mxr::SelectionInfoType::MeshFace },
            { MxFunctions::PickDataType::ElementEdge, mxr::SelectionInfoType::MeshLine },
            { MxFunctions::PickDataType::Part,        mxr::SelectionInfoType::MeshPart },
        };
        for (const auto& row : typeTable)
        {
            const auto& selected = meshSelectData->getSelectDataByType(row.second);
            if (selected.empty()) continue;   // 空选中不落键，保证 data.empty() 判空保护生效
            auto& vec = data[row.first];
            vec.reserve(selected.size());
            for (const auto& d : selected)
            {
                vec.push_back(MechPreData::SelectData2ResultData(row.first, d));
            }
        }

        if (_elementFilter.size() > 0)
        {
            std::vector<RenderPickResultData> filtered;
            for (const auto& pair : data)
            {
                if (pair.first != PickDataType::Element)
                    continue;

                auto* meshData = getCVM->getModelData()->getMeshData();
                for (const auto& d : pair.second)
                {
                    if (_elementFilter.contains(meshData->getDim(d)))
                    {
                        filtered.push_back(d);
                    }
                }

                data[PickDataType::Element] = filtered;
            }
        }
        return data;
    }

    void mPostPickWidgetController::onViewportPickFinished()
    {
        if (!_isActive || !_model)
            return;

        auto data = captureViewportPickData();
        if (data.empty())
        {
            // 拾取事件(右键取消拾取)把最后一个拾取从选择容器 REDUCE 移除：
            //    视口高亮已被渲染侧清空，必须同步清空模型并通知，否则出现
            //    "高亮已清、控件仍显示有拾取"的不同步；
            if (!_model->getAllData().pickedData.empty())
            {
                _model->clearPickedData();
                markDirtyAndNotify();
            }
            return;
        }

        if (pickMode() == PickOne)
        {
            // 单选模式：后拾取覆盖先拾取。共享选中容器每次提交后会同步为控件数据，
            // 因此捕获数据中会残留旧拾取（如先拾取节点，切到几何点模式再拾取时
            // 捕获数据同时含旧节点与新几何点）。先剔除控件已有数据得到本次新拾取，
            // 再以新拾取整体覆盖，避免跨类型时新拾取被单选裁剪误丢弃。
            auto oldData = _model->getAllData().pickedData;
            std::unordered_set<RenderPickResultData, RenderPickResultDataHash, RenderPickResultDataEqual> oldSet;
            for (const auto& pair : oldData)
                for (const auto& item : pair.second)
                    oldSet.insert(item);

            std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> fresh;
            for (const auto& pair : data)
            {
                for (const auto& item : pair.second)
                {
                    if (oldSet.find(item) == oldSet.end())
                        fresh[pair.first].push_back(item);
                }
            }

            // 本次点击未产生新拾取（如点击空白处或重复点击已拾取对象），保持现状
            if (fresh.empty())
                return;

            _model->clear();
            _model->setPickedData(fresh);
        }
        else
        {
            _model->setPickedData(data);
        }
        markDirtyAndNotify();
    }

    void mPostPickWidgetController::connectViewportSignal()
    {
        // 前处理同位置无 getCVM 判空（激活场景模型必在）；后处理保留同语义但补判空，
        // 模型全删后失活残留的激活链路再触发时不至于空指针解引用
        if (!getCVM || getCVM->getPostRenderWidget() == nullptr) return;
        disconnectViewportSignal();
        _viewportConnection = connect(getCVM->getPostRenderWidget()->getSelectionContainer(),&mxr::SelectionDataContainer::ReadyToReadContainer,this,&mPostPickWidgetController::onViewportPickFinished);
    }

    void mPostPickWidgetController::disconnectViewportSignal()
    {
        if (_viewportConnection)
        {
            disconnect(_viewportConnection);
            _viewportConnection = QMetaObject::Connection();
        }
    }

    // ------------------------------
    // 视口/模型切换联动
    // ------------------------------
    void mPostPickWidgetController::onViewportModelAboutToChange(int newViewportID)
    {
        // 目标视口仍属于当前模型：数据落盘模型不变，保持拾取会话不中断，
        // 切换完成后由 onViewportModelSwitched 把连接与高亮重绑到新视口
        if (newViewportID != -1 && getCVM)
        {
            int newModelID = -1;
            if (VMManager->getViewportPrivateByID(newViewportID, &newModelID)
                && newModelID == getCVM->getModelData()->getModelDataID())
            {
                return;
            }
        }

        // 跨模型切换/模型删除重建/目标视口未知：按"退出面板"语义处理（对齐各面板
        // deactivatePickAndCommit 的退出行为）。必须趁旧模型仍为当前模型时执行：
        // 提交与高亮清理内部按 getCVM 定位落盘模型与渲染窗口，切换后面板 unEdit
        // 链路的清理会重绑到新模型视口——旧视口的临时高亮清不到、提交也会写错模型
        if (this == s_activeController && _isActive)
        {
            // 激活控件：失活提交且不保留临时高亮（keep=false 分支在 _widget 仍指向
            // 旧模型视口时清空共享选中态与临时高亮层，不留蓝色临时高亮残留）
            setKeepHighlightOnDeactivate(false);
            setPickActive(false);
            setKeepHighlightOnDeactivate(true);
        }
        else if (_highlightForm == HighlightForm::InactiveTemp)
        {
            // 失活临时高亮残留（同面板内被其他控件抢占激活后留下的临时高亮）：一并清空
            clearAllHighlight();
        }
        else
        {
            return;
        }

        // 清理只改了容器数据与渲染层数据，还须立即同步重画旧视口：
        // 防抖定时器触发时当前模型已切换，doUpdateHighlight 会按
        // "非当前视口跳过"守卫跳过旧视口，旧视口画面将残留切换前的拾取高亮
        if (_highlightService)
        {
            _highlightService->flushHighlightNow();
        }
    }

    void mPostPickWidgetController::onViewportModelSwitched(int viewportID)
    {
        Q_UNUSED(viewportID);
        // 同模型视口切换且控件仍处于激活态（未走面板 unEdit/重编辑链路的场景，
        // 如切到后处理视口再切回）：保持拾取会话，连接与高亮重绑到新激活视口，
        // 避免继续监听旧视口的选择容器导致拾取无响应或读到旧视口残留选中
        if (!_isActive || !_isVisible)
            return;
        if (!getCVM || !getCVM->getPostRenderWidget())
            return;

        connectViewportSignal();
        _highlightForm = HighlightForm::None;
        applyHighlight(_isActive);
    }

    // ------------------------------
    // UI刷新
    // ------------------------------
    void mPostPickWidgetController::refreshUI()
    {
        if (!_view || !_model) return;

        const auto& allData = _model->getAllData();
        std::vector<PickDisplayItem> uiData;
        for (const auto& pick : allData.pickedData)
        {
            for (const auto& pic : pick.second)
            {
                uiData.push_back(toPickDisplayItem(pick.first, pic));
            }
        }

        _view->setCurrentData(uiData, allData.selectedSetNames);
    }

    PickDisplayItem mPostPickWidgetController::toPickDisplayItem(MxFunctions::PickDataType type, const RenderPickResultData& data) const
    {
        PickDisplayItem info;
        info.type = type;
        info.data = data;
        return info;
    }

    void mPostPickWidgetController::markDirtyAndNotify()
    {
        _dirty = true;
        enforceSinglePickLimit();
        refreshUI();
        _highlightForm = HighlightForm::None;
        applyHighlight(_isActive);
        emit dataChanged();
    }

    void mPostPickWidgetController::enforceSinglePickLimit()
    {
        if (!_model || !_view || pickMode() != PickOne)
            return;

        const auto& allData = _model->getAllData();

        // 优先保留集合，清空拾取
        if (!allData.selectedSetNames.isEmpty())
        {
            _model->clearPickedData();
            if (allData.selectedSetNames.size() > 1)
            {
                QSet<QString> firstSet;
                firstSet.insert(*allData.selectedSetNames.begin());
                _model->setSelectedSetNames(firstSet);
            }
            return;
        }

        // 仅保留最后一条拾取数据
        auto pickedData = allData.pickedData;
        if (pickedData.empty())
            return;

        MxFunctions::PickDataType lastKey = PickDataType::None;
        RenderPickResultData lastData;
        bool hasData = false;
        for (const auto& pair : pickedData)
        {
            if (!pair.second.empty())
            {
                lastKey = pair.first;
                lastData = pair.second.back();
                hasData = true;
            }
        }
        if (hasData)
        {
            _model->clearPickedData();
            _model->appendPickedData({ { lastKey, { lastData } } });
        }
    }
}
