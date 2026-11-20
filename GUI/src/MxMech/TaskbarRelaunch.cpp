// =============================================================================
// TaskbarRelaunch.cpp —— Windows 任务栏固定/重启动属性设置（实现）
//
// 参考：Microsoft Learn "AppUserModelIDs" / "System.AppUserModel.RelaunchCommand"
// 时序要求：在窗口 winId()（原生 HWND 已创建）之后调用；AUMID 最后设置。
// =============================================================================

#include "TaskbarRelaunch.h"

// 平台隔离：任务栏机制为 Windows 独有，非 Windows 编译为空实现，
// 调用方（mAppInitializer 等）无需任何条件编译。
#if defined(_WIN32) || defined(_WIN64)

#include <windows.h>
#include <shobjidl.h>
#include <propkey.h>
#include <propvarutil.h>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>

#include <string>

#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Propsys.lib")
#pragma comment(lib, "Shlwapi.lib")

namespace {

// 应用用户模型 ID：全公司/全产品稳定唯一，不带版本、不带路径。
// 安装器开始菜单/桌面快捷方式应写入同一 AUMID。
// 值由 MxMech/CMakeLists.txt 按 CurrentModel 注入（MX_APP_AUMID/MX_APP_DISPLAY_NAME），
// 此处仅作漏注入时的编译兜底
#ifndef MX_APP_AUMID
#define MX_APP_AUMID L"Hnmx.MxSim.Generic"
#endif
#ifndef MX_APP_DISPLAY_NAME
#define MX_APP_DISPLAY_NAME L"MxSim"
#endif
constexpr wchar_t kAppUserModelId[] = MX_APP_AUMID;
constexpr wchar_t kDisplayName[] = MX_APP_DISPLAY_NAME;

std::wstring FindLauncherPath()
{
	// 启动程序名因项目而异（*Launcher.exe 模式），且通常位于 exe 上层目录
	// （打包结构：<root>/<XxxLauncher.exe> + <root>/Application/<主程序>.exe）。
	// 依次在 exe 目录、上层目录按 "*Launcher.exe" 过滤查找（Windows 下不区分大小写）。
	const QString exeDir = QCoreApplication::applicationDirPath();
	const QString searchDirs[] = { exeDir, exeDir + QStringLiteral("/..") };
	for (const QString& dir : searchDirs) {
		const QFileInfoList entries = QDir(dir).entryInfoList(
			QStringList() << QStringLiteral("*Launcher.exe"),
			QDir::Files);
		if (!entries.isEmpty())
			return QDir::toNativeSeparators(entries.first().absoluteFilePath())
				.toStdWString();
	}
	return std::wstring();
}

bool SetStringProperty(IPropertyStore* store, REFPROPERTYKEY key, const std::wstring& value)
{
	PROPVARIANT pv;
	PropVariantInit(&pv);
	HRESULT hr = InitPropVariantFromString(value.c_str(), &pv);
	if (SUCCEEDED(hr))
		hr = store->SetValue(key, pv);
	PropVariantClear(&pv);
	return SUCCEEDED(hr);
}

} // namespace

namespace TaskbarRelaunch {

void SetProcessAppUserModelId()
{
	// 返回值仅在没有窗口创建前调用才可能成功；失败静默（不影响业务）
	SetCurrentProcessExplicitAppUserModelID(kAppUserModelId);
}

void ApplyForMainWindow(QWidget* mainWindow)
{
	if (!mainWindow)
		return;

	HWND hwnd = reinterpret_cast<HWND>(mainWindow->winId()); // 强制创建原生窗口
	if (!hwnd)
		return;

	IPropertyStore* store = nullptr;
	if (FAILED(SHGetPropertyStoreForWindow(hwnd, IID_PPV_ARGS(&store))) || !store)
		return;

	const std::wstring launcher = FindLauncherPath();

	bool ok = false;
	if (!launcher.empty()) {
		// 重启动命令：固定到任务栏后再点击，始终先启动 Launcher
		ok = SetStringProperty(store, PKEY_AppUserModel_RelaunchCommand,
			L"\"" + launcher + L"\"");
		SetStringProperty(store, PKEY_AppUserModel_RelaunchDisplayNameResource,
			kDisplayName);
		SetStringProperty(store, PKEY_AppUserModel_RelaunchIconResource,
			launcher + L",0");
	}
	// AUMID 必须最后设置；无 Launcher 时也设置（保证窗口分组稳定）
	SetStringProperty(store, PKEY_AppUserModel_ID, kAppUserModelId);

	if (ok || !launcher.empty())
		store->Commit();
	store->Release();
}

} // namespace TaskbarRelaunch

#else // 非 Windows：空实现

namespace TaskbarRelaunch {

void SetProcessAppUserModelId()
{
}

void ApplyForMainWindow(QWidget*)
{
}

} // namespace TaskbarRelaunch

#endif // _WIN32
