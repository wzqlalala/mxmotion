#pragma once
#pragma execution_character_set("utf-8")
#include "mmainwindow_global.h"
#include <QObject>
#include <QKeySequence>
#include <vector>
#include "mShortcutManager.h"

namespace MechMainWindow
{
    /**
  * @brief 模块快捷键注册器基类（接口）
  *
  * 每个业务模块继承此类，实现自己的快捷键注册逻辑。
  * 遵循开闭原则：新增模块只需新增子类，无需修改管理器。
  */
    class mModuleShortcutRegistrar
    {
    public:
        virtual ~mModuleShortcutRegistrar() = default;

        /**
         * @brief 向 ShortcutManager 注册本模块所有快捷键
         */
        virtual void registerAll() = 0;

        /**
         * @brief 注销本模块所有快捷键（模块卸载时调用）
         */
        virtual void unregisterAll() = 0;

        virtual ShortcutModule module() const = 0;
    };
}