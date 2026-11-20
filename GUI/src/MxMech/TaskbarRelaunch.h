#pragma once
// =============================================================================
// TaskbarRelaunch.h —— Windows 任务栏固定/重启动属性设置
//
// 背景：Launcher(MechLauncher.exe) 与拥有窗口的 MxMech.exe 是两个进程，
// 用户"固定到任务栏"时 Windows 默认固定的是 MxMech.exe，再点击时绕过
// Launcher 导致其构造的运行环境失效。通过给主窗口设置
//   - AppUserModelID（与安装器快捷方式一致）
//   - System.AppUserModel.RelaunchCommand（指向 MechLauncher.exe）
// 使任务栏固定项的重新启动始终经过 Launcher。
// =============================================================================

#include <QWidget>

namespace TaskbarRelaunch {

// 进程级 AUMID：必须在任何窗口创建之前调用（main 最早期），
// 决定任务栏按钮/固定项的应用身份；重复调用安全。
void SetProcessAppUserModelId();

// 给主窗口设置任务栏重启动属性。
// - 重启动命令自动定位 *Launcher.exe（exe 目录及上层目录，名字因项目而异）；
//   找不到时仅设 AUMID；
// - 重复调用安全（仅设置窗口属性，不影响业务）；
// - 非 Windows 平台为空实现，调用方无需条件编译。
void ApplyForMainWindow(QWidget* mainWindow);

} // namespace TaskbarRelaunch
