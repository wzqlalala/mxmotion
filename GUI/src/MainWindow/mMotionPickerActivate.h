#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mPointPickerWidget.h"
#include "mDirectionSelectorWidget.h"

#include <QList>

namespace MechMainWindow
{
    // 注意：Motion面板内严禁调用 mViewportModel::clearPick() 或
    // UpdatePreWidgetHighlight ——后者会 emit ReadyToReadContainer，
    /// 若在拾取信号(coordinateChanged/directionChanged)处理链内触发，
    /// 激活中的拾取控件会重新捕获并无条件 dataChanged，形成
    /// "信号→面板刷新→再发信号" 的无限递归（栈溢出崩溃）。
    /// 拾取会话完全交由控件内部的 mPickWidgetController 管理。

    // ===== 方向控件拾取会话状态（存数据类，照集中力/重力载荷 dirPickState_ 模式）=====
    // 面板是单例而会话属于数据对象：状态随对象保存/恢复，新建对象无记录自动回"向量"。

    /// 从方向控件抓取当前拾取会话状态写入数据成员（directionChanged 时调用）
    inline void saveDirPickState(mDirectionSelectorWidget* selector, DirectionPickState& state)
    {
        if (!selector)
            return;
        const auto mode = selector->directionMode();
        state.mode = static_cast<int>(mode);
        state.reversed = selector->isReversed();
        state.startPoint = QVariant();
        state.endPoint = QVariant();
        state.selectedObject = { MxFunctions::PickDataType::None, RenderPickResultData() };
        state.directInputDir = QVector3D();
        if (mode == mDirectionSelectorWidget::CustomCoordinate) {
            state.startPoint = QVariant::fromValue(selector->startPoint());
            state.endPoint = QVariant::fromValue(selector->endPoint());
        }
        else if (mode == mDirectionSelectorWidget::SelectObject) {
            auto* c = selector->objectPickController();
            if (c) {
                const auto& picked = c->getAllData().pickedData;
                const auto it = picked.begin();
                if (it != picked.end() && !it->second.empty())
                    state.selectedObject = { it->first, it->second.front() };
            }
        }
        else if (mode == mDirectionSelectorWidget::DirectInput) {
            QVariant dv = selector->getDirection();
            if (dv.isValid() && !dv.isNull())
                state.directInputDir = dv.value<QVector3D>();
        }
    }

    /// 按数据成员里的会话状态反显方向控件（无记录时回"向量"并写入数据矢量值）
    inline void restoreDirPickState(mDirectionSelectorWidget* selector, const DirectionPickState& state,
        const QVector3D& fallbackDir)
    {
        if (!selector)
            return;
        //记录恢复前的激活归属：setDirectionMode 会经内部 onModeChanged 激活拾取控件，
        //而本函数在 updateUIFromData（点拾取/下拉框信号链内高频触发）中被调用，
        //若放任恢复路径抢激活，会把用户正在使用的拾取控件顶掉导致无法连续拾取。
        //进入编辑时的会话激活由 restorePickSessionsOnEntry 延迟统一处理。
        const bool wasStartActive = selector->startPickController() && selector->startPickController()->isPickActive();
        const bool wasEndActive = selector->endPickController() && selector->endPickController()->isPickActive();
        const bool wasObjectActive = selector->objectPickController() && selector->objectPickController()->isPickActive();

        const bool blocked = selector->blockSignals(true);
        if (state.mode > 0) {
            const auto mode = static_cast<mDirectionSelectorWidget::DirectionMode>(state.mode);
            selector->setDirectionMode(mode);
            selector->setReversed(state.reversed);
            if (mode == mDirectionSelectorWidget::CustomCoordinate
                && state.startPoint.isValid() && !state.startPoint.isNull()
                && state.endPoint.isValid() && !state.endPoint.isNull())
            {
                selector->setCustomCoordinatePoints(
                    state.startPoint.value<QVector3D>(), state.endPoint.value<QVector3D>());
            }
            else if (mode == mDirectionSelectorWidget::SelectObject) {
                PickWidgetData data;
                if (state.selectedObject.second.result_id > 0)
                    data.pickedData[state.selectedObject.first].push_back(state.selectedObject.second);
                if (selector->objectPickController())
                    selector->objectPickController()->displayData(data);
            }
            else if (mode == mDirectionSelectorWidget::DirectInput && !state.directInputDir.isNull()) {
                selector->setDirection(state.directInputDir);
            }
        }
        else {
            //无状态记录（新建对象/历史数据）：清掉继承来的旧拾取会话回"向量"
            selector->setDirectionMode(mDirectionSelectorWidget::DirectInput);
            selector->setDirection(fallbackDir);
        }
        // 统一清空未使用的拾取控制器数据，防止上一个对象的拾取残留在控件内
        // （新建对象切"选择对象"时残留面会被当作新拾取错误地高亮并写入新对象）
        const auto finalMode = selector->directionMode();
        if (finalMode != mDirectionSelectorWidget::SelectObject) {
            if (auto* c = selector->objectPickController()) c->displayData(PickWidgetData());
        }
        if (finalMode != mDirectionSelectorWidget::CustomCoordinate) {
            if (auto* c = selector->startPickController()) c->displayData(PickWidgetData());
            if (auto* c = selector->endPickController()) c->displayData(PickWidgetData());
        }
        //还原激活归属：恢复过程若意外激活了原本未激活的拾取控件（onModeChanged 副作用），
        //立即失活归还（保留临时高亮），确保反显不干扰用户当前的拾取对象
        if (!wasStartActive && selector->startPickController() && selector->startPickController()->isPickActive())
            selector->startPickController()->setPickActive(false);
        if (!wasEndActive && selector->endPickController() && selector->endPickController()->isPickActive())
            selector->endPickController()->setPickActive(false);
        if (!wasObjectActive && selector->objectPickController() && selector->objectPickController()->isPickActive())
            selector->objectPickController()->setPickActive(false);
        selector->blockSignals(blocked);
    }

    /// 进入编辑时恢复拾取会话的可视状态（在 activateFirstVisiblePointPicker 之后调用）：
    /// - 方向控件处于"选择对象"模式且持有拾取数据：重新激活其拾取控件——
    ///   进面板即可继续拾取编辑（全局互斥下最后激活者成为当前拾取控件，
    ///   被顶下的控件按失活态保留临时高亮）；
    /// - 未被激活但持有拾取数据的点控件：按失活态重绘临时高亮，呈现已有拾取。
    inline void restorePickSessionsOnEntry(const QList<mPointPickerWidget*>& pickers,
        const QList<mDirectionSelectorWidget*>& selectors = {})
    {
        for (auto* selector : selectors)
        {
            if (!selector || !selector->isVisible() || !selector->isEnabled())
                continue;
            auto* objectController = selector->objectPickController();
            if (!objectController)
                continue;
            if (selector->directionMode() != mDirectionSelectorWidget::SelectObject)
                continue;
            if (objectController->getAllData().isEmpty())
                continue;
            objectController->setPickActive(true);
        }
        for (auto* picker : pickers)
        {
            if (!picker || !picker->pickController())
                continue;
            if (picker->pickController()->isPickActive())
                continue;
            if (picker->pickController()->getAllData().isEmpty())
                continue;
            //同数据反显并按激活态重绘高亮（失活态即临时高亮）
            picker->setPickData(picker->pickData());
        }
    }

    /// 统一激活管理（参照 mCoordinatePanel::activateFirstVisiblePicker）：
    /// - 失活所有不可见/不可用的点拾取控件（避免隐藏控件占用拾取状态）；
    /// - 按顺序激活第一个可见且可用的点拾取控件（控件内部全局互斥，激活新的自动失活其他）；
    /// - 方向控件不可见/不可用时同步失活其内部拾取（激活由方向控件按模式自管理，此处只兜底）。
    /// 注意：仅在进入编辑/类型或页面切换等可见性变化处调用，
    /// 不要放进 updateUIFromData 这类拾取信号会反复进入的函数，避免顶掉用户正在使用的方向拾取。
    inline void activateFirstVisiblePointPicker(const QList<mPointPickerWidget*>& pickers,
        const QList<mDirectionSelectorWidget*>& selectors = {})
    {
        for (auto* picker : pickers)
        {
            if (!picker || !picker->pickController())
                continue;
            if (!picker->isVisible() || !picker->isEnabled())
            {
                picker->pickController()->setPickActive(false);
                picker->clearAllHighlight();
            }
        }

        for (auto* selector : selectors)
        {
            if (!selector || (selector->isVisible() && selector->isEnabled()))
                continue;
            const QList<mPickWidgetController*> controllers = {
                selector->startPickController(), selector->endPickController(), selector->objectPickController() };
            for (auto* controller : controllers)
            {
                if (controller)
                {
                    controller->setPickActive(false);
                    controller->clearAllHighlight();
                }
            }
        }

        for (auto* picker : pickers)
        {
            if (!picker || !picker->pickController())
                continue;
            if (picker->isVisible() && picker->isEnabled())
            {
                picker->pickController()->setPickActive(true);
                break;
            }
        }
    }
}
