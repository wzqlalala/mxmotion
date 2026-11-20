#pragma once
#pragma execution_character_set("utf-8")
#include "mmainwindow_global.h"
#include <QObject>
#include <QMap>
#include <QDebug>
#include <QShortcut>
#include <qelapsedtimer.h>

namespace MechMainWindow
{
    enum class ShortcutModule
    {
        Global,
        Geometry,
        Mesh
    };

    struct ShortcutItem
    {
        QKeySequence          keySequence;
        QString               description;
        std::function<void()> handler;
        int                   cooldownMs = 500;
        bool                  enabled = true;


    ShortcutItem() {};
    ShortcutItem(const QKeySequence& k, const QString& d, const std::function<void()>& h) {
            keySequence = k;
            description = d;
            handler = h;
    }
    };

    enum class ConflictResult
    {
        NoConflict,
        IntraModuleConflict,  // 同模块冲突 → 拒绝
        GlobalConflict,       // 与Global冲突 → 拒绝
    };

    class ShortcutManager : public QObject
    {
        Q_OBJECT

    public:
        static ShortcutManager* instance(QWidget* parent = nullptr);

        ShortcutManager(const ShortcutManager&) = delete;
        ShortcutManager& operator=(const ShortcutManager&) = delete;

        void registerShortcut(ShortcutModule       module,
            const QString& id,
            const ShortcutItem& item);

        void unregisterShortcut(ShortcutModule module, const QString& id);

        void           activateModule(ShortcutModule module);
        ShortcutModule activeModule() const { return m_activeModule; }
        QKeySequence shortcut(ShortcutModule module, const QString& id) const;

        void dumpAllShortcuts() const;

    signals:
        void moduleActivated(ShortcutModule module);

    private:
        explicit ShortcutManager(QWidget* parent);

        /**
         * @brief 冲突检测（同模块内唯一，跨业务模块允许重复）
         *
         * 规则：
         *  ① 同模块内：相同 keySequence → IntraModuleConflict
         *  ② Global  ↔ 业务模块：相同 keySequence → GlobalConflict
         *  ③ 业务模块 ↔ 业务模块：相同 keySequence → NoConflict（允许）
         *
         * @param module  待注册的模块
         * @param id      待注册的ID（用于排除自身，支持覆盖注册）
         * @param seq     待注册的按键序列
         * @param outConflictInfo 冲突详情描述（仅在有冲突时有效）
         */
        ConflictResult checkConflict(ShortcutModule     module,
            const QString& id,
            const QKeySequence& seq,
            QString& outConflictInfo) const;

        void    setModuleEnabled(ShortcutModule module, bool enabled);
        QString moduleToString(ShortcutModule module) const;

    private:
        QMap<ShortcutModule, QMap<QString, QShortcut*>>  m_shortcuts;
        QMap<ShortcutModule, QMap<QString, ShortcutItem>> m_items;

        //记录每个快捷键上次触发时间
        // key: "模块/id"
        QMap<QString, QElapsedTimer>                       m_lastTriggerTime;

        QWidget* m_parentWidget;
        ShortcutModule m_activeModule = ShortcutModule::Geometry;
    };

}