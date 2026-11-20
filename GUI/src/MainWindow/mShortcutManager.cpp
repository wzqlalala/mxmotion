#include "mShortcutManager.h"
#include <QDebug>
#include <stdexcept>
#include <QShortcut>


namespace MechMainWindow
{
    // 全局单例指针
    static ShortcutManager* g_shortcutManager = nullptr;
    ShortcutManager* ShortcutManager::instance(QWidget* parent)
    {
        if (!g_shortcutManager) {
            g_shortcutManager = new ShortcutManager(parent);
        }
        return g_shortcutManager;
    }

    ShortcutManager::ShortcutManager(QWidget* parent)
        : QObject(parent)
        , m_parentWidget(parent)
    {
        if (!parent)
            throw std::runtime_error("ShortcutManager: 首次初始化必须传入有效的父窗口指针");

        qDebug() << "[ShortcutManager] 初始化完成";
    }

    // ----------------------------------------------------------------
    //  冲突检测（核心修改）
    // ----------------------------------------------------------------
    ConflictResult ShortcutManager::checkConflict(ShortcutModule      module,
        const QString& id,
        const QKeySequence& seq,
        QString& outInfo) const
    {
        for (auto mIt = m_items.cbegin(); mIt != m_items.cend(); ++mIt)
        {
            for (auto iIt = mIt.value().cbegin(); iIt != mIt.value().cend(); ++iIt)
            {
                // 跳过自身（支持覆盖注册）
                if (mIt.key() == module && iIt.key() == id)
                    continue;

                // 按键不同，直接跳过
                if (iIt.value().keySequence != seq)
                    continue;

                // ===== 以下：按键相同，判断冲突类型 =====

                // 规则①：同模块内按键重复 → 拒绝
                if (mIt.key() == module)
                {
                    outInfo = QString("同模块[%1]内已有 ID[%2] 使用了按键[%3]")
                        .arg(moduleToString(module))
                        .arg(iIt.key())
                        .arg(seq.toString());
                    return ConflictResult::IntraModuleConflict;
                }

                // 规则②：Global ↔ 业务模块 → 拒绝
                //   情况A：待注册是业务模块，已有Global模块注册了相同按键
                //   情况B：待注册是Global，已有业务模块注册了相同按键
                const bool newIsGlobal = (module == ShortcutModule::Global);
                const bool existingIsGlobal = (mIt.key() == ShortcutModule::Global);

                if (newIsGlobal || existingIsGlobal)
                {
                    outInfo = QString("按键[%1]在 Global模块[ID:%2] 与 业务模块[%3/ID:%4] 间冲突"
                        "（Global始终enabled，会产生按键竞争）")
                        .arg(seq.toString())
                        .arg(existingIsGlobal ? iIt.key() : id)
                        .arg(moduleToString(existingIsGlobal ? module : mIt.key()))
                        .arg(existingIsGlobal ? id : iIt.key());
                    return ConflictResult::GlobalConflict;
                }

                // 规则③：业务模块 ↔ 业务模块，按键相同 → 允许（NoConflict）
                // 因为同一时刻只有一个业务模块处于 enabled 状态
                // 此处仅输出提示，不阻止注册
                qDebug() << QString("[ShortcutManager] 提示：按键[%1] 在模块[%2/ID:%3]"
                    " 和模块[%4/ID:%5] 中共用（跨模块允许）")
                    .arg(seq.toString())
                    .arg(moduleToString(module)).arg(id)
                    .arg(moduleToString(mIt.key())).arg(iIt.key());
            }
        }

        return ConflictResult::NoConflict;
    }

    // ----------------------------------------------------------------
    //  注册快捷键
    // ----------------------------------------------------------------
    void ShortcutManager::registerShortcut(ShortcutModule      module,
        const QString& id,
        const ShortcutItem& item)
    {
        QString conflictInfo;
        const ConflictResult result = checkConflict(module, id,
            item.keySequence, conflictInfo);
        switch (result)
        {
        case ConflictResult::IntraModuleConflict:
            qWarning() << "[ShortcutManager] 注册拒绝（同模块冲突）：" << conflictInfo;
            return;
        case ConflictResult::GlobalConflict:
            qWarning() << "[ShortcutManager] 注册拒绝（Global冲突）：" << conflictInfo;
            return;
        case ConflictResult::NoConflict:
            break;
        }

        if (m_shortcuts[module].contains(id))
        {
            delete m_shortcuts[module][id];
            m_shortcuts[module].remove(id);
        }

        auto* shortcut = new QShortcut(item.keySequence,
            m_parentWidget,
            nullptr,
            nullptr,
            Qt::WindowShortcut);

        const bool shouldEnable = (module == ShortcutModule::Global)
            || (module == m_activeModule);
        shortcut->setEnabled(shouldEnable && item.enabled);

        //构造冷却计时器的 key
        const QString timerKey = QString("%1/%2")
            .arg(moduleToString(module))
            .arg(id);

        // 初始化计时器（start 后 elapsed 从 0 开始，首次触发必然通过）
        m_lastTriggerTime[timerKey].start();

        connect(shortcut, &QShortcut::activated,
            this, [handler = item.handler,
            cooldownMs = item.cooldownMs,
            timerKey,
            module, id,
            this]()
            {
                //冷却时间检查
                if (cooldownMs > 0)
                {
                    QElapsedTimer& timer = m_lastTriggerTime[timerKey];
                    if (timer.elapsed() < cooldownMs)
                    {
                        qDebug() << QString("[ShortcutManager] 冷却中，忽略触发 模块[%1] ID[%2]"
                            " 剩余冷却[%3ms]")
                            .arg(moduleToString(module))
                            .arg(id)
                            .arg(cooldownMs - timer.elapsed());
                        return;
                    }
                    //重置计时器
                    timer.restart();
                }

                qDebug() << QString("[ShortcutManager] 触发 模块[%1] ID[%2]")
                    .arg(moduleToString(module)).arg(id);

                if (handler)
                    handler();
            });

        m_shortcuts[module][id] = shortcut;
        m_items[module][id] = item;

        qDebug() << QString("[ShortcutManager] 注册成功 模块[%1] ID[%2] 按键[%3]"
            " enabled[%4] cooldown[%5ms]")
            .arg(moduleToString(module))
            .arg(id)
            .arg(item.keySequence.toString())
            .arg(shouldEnable ? "true" : "false")
            .arg(item.cooldownMs);
    }

    // ----------------------------------------------------------------
    //  激活模块（真正在Qt层面 enable/disable）
    // ----------------------------------------------------------------
    void ShortcutManager::activateModule(ShortcutModule module)
    {
        if (module == ShortcutModule::Global)
        {
            qWarning() << "[ShortcutManager] Global 模块无需手动激活，始终有效";
            return;
        }

        if (m_activeModule == module)
        {
            qDebug() << "[ShortcutManager] 模块已处于激活状态:" << moduleToString(module);
            return;
        }

        const ShortcutModule prev = m_activeModule;
        m_activeModule = module;

        // 禁用上一个业务模块
        setModuleEnabled(prev, false);
        qDebug() << "[ShortcutManager] 禁用模块:" << moduleToString(prev);

        // 启用新模块
        setModuleEnabled(module, true);
        qDebug() << "[ShortcutManager] 激活模块:" << moduleToString(module);

        emit moduleActivated(module);
    }

    void ShortcutManager::setModuleEnabled(ShortcutModule module, bool enabled)
    {
        if (!m_shortcuts.contains(module))
            return;

        for (auto it = m_shortcuts[module].begin();
            it != m_shortcuts[module].end(); ++it)
        {
            const bool itemEnabled = m_items[module][it.key()].enabled;
            it.value()->setEnabled(enabled && itemEnabled);
        }
    }

    void ShortcutManager::unregisterShortcut(ShortcutModule module, const QString& id)
    {
        if (!m_shortcuts.contains(module) || !m_shortcuts[module].contains(id))
        {
            qWarning() << QString("[ShortcutManager] 注销失败，未找到 模块[%1] ID[%2]")
                .arg(moduleToString(module)).arg(id);
            return;
        }

        delete m_shortcuts[module][id];
        m_shortcuts[module].remove(id);
        m_items[module].remove(id);

        //同步清理计时器
        const QString timerKey = QString("%1/%2")
            .arg(moduleToString(module))
            .arg(id);
        m_lastTriggerTime.remove(timerKey);

        qDebug() << QString("[ShortcutManager] 注销 模块[%1] ID[%2]")
            .arg(moduleToString(module)).arg(id);
    }

    QKeySequence ShortcutManager::shortcut(ShortcutModule module, const QString& id) const
    {
        const auto moduleIt = m_items.constFind(module);
        if (moduleIt == m_items.cend())
            return QKeySequence();
        const auto itemIt = moduleIt.value().constFind(id);
        return itemIt == moduleIt.value().cend() ? QKeySequence() : itemIt.value().keySequence;
    }

    void ShortcutManager::dumpAllShortcuts() const
    {
        qDebug() << "\n========== 已注册快捷键 ==========";
        for (auto mIt = m_items.cbegin(); mIt != m_items.cend(); ++mIt)
        {
            qDebug() << "【" << moduleToString(mIt.key()) << "】";
            for (auto iIt = mIt.value().cbegin(); iIt != mIt.value().cend(); ++iIt)
            {
                const bool active = m_shortcuts[mIt.key()][iIt.key()]->isEnabled();
                qDebug() << QString("  %-20s %-15s %-12s %s")
                    .arg(iIt.key())
                    .arg(iIt.value().keySequence.toString())
                    .arg(active ? "[enabled] " : "[disabled]")
                    .arg(iIt.value().description);
            }
        }
        qDebug() << "===================================\n";
    }

    QString ShortcutManager::moduleToString(ShortcutModule module) const
    {
        switch (module)
        {
        case ShortcutModule::Global:      return "Global";
        case ShortcutModule::Geometry:    return "Geometry";
        case ShortcutModule::Mesh:        return "Mesh";
        default:                          return "Unknown";
        }
    }

}