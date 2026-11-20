from conan import ConanFile
from conan.tools.cmake import cmake_layout
import platform


class RebuildModelDataConan(ConanFile):
    name = "rebuildmodeldata"
    version = "2.0"
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False]}
    default_options = {
        "shared": True,
        "qt/*:shared": True,
        "freetype/*:shared": True,
    }
    generators = "CMakeDeps", "CMakeToolchain"
    exports_sources = "CMakeLists.txt", "src/*", "cmake/*"

    @staticmethod
    def _get_windows_kernel_version():
        """通过 GetVersionExW 查询 Windows 内核版本号 (major, minor)。"""
        import ctypes

        class OSVERSIONINFOEXW(ctypes.Structure):
            _fields_ = [
                ("dwOSVersionInfoSize", ctypes.c_ulong),
                ("dwMajorVersion", ctypes.c_ulong),
                ("dwMinorVersion", ctypes.c_ulong),
                ("dwBuildNumber", ctypes.c_ulong),
                ("dwPlatformId", ctypes.c_ulong),
                ("szCSDVersion", ctypes.c_wchar * 128),
                ("wServicePackMajor", ctypes.c_ushort),
                ("wServicePackMinor", ctypes.c_ushort),
                ("wSuiteMask", ctypes.c_ushort),
                ("wProductType", ctypes.c_ubyte),
                ("wReserved", ctypes.c_ubyte),
            ]

        osvi = OSVERSIONINFOEXW()
        osvi.dwOSVersionInfoSize = ctypes.sizeof(OSVERSIONINFOEXW)
        success = ctypes.windll.kernel32.GetVersionExW(ctypes.byref(osvi))
        if not success:
            raise RuntimeError("GetVersionExW failed to query windows version")
        return osvi.dwMajorVersion, osvi.dwMinorVersion

    def _is_win7(self):
        """内核版本 6.1 即 Windows 7。
        Win8.1+ 上未带 manifest 的进程 GetVersionExW 会被改写为 6.2，
        因此 6.1 只会在真实 Win7 上出现；查询失败时回退 platform.release()。
        """
        if platform.system() != "Windows":
            return False
        try:
            major, minor = self._get_windows_kernel_version()
        except Exception:
            return platform.release() == "7"
        return (major, minor) == (6, 1)

    def _target_platform(self):
        """目标平台配置标识，与三份 conan_packages.toml 对应：
        windows: Auto_Config/conan_packages.toml（默认）
        win7:    CI-CD_Tool_Win7_Config/conan_packages.toml
        linux:   CI-CD_Tool_Linux_Config/conan_packages.toml
        """
        if platform.system() == "Windows":
            return "win7" if self._is_win7() else "windows"
        return "linux"

    def requirements(self):
        self.requires("qt/5.13.2@mxsim/stable")
        #【OCC摘除·S11】opencascade 依赖移除：GeoEngineData pimpl 化后本仓零 OCC
        self.requires("freetype/2.10.4@mxsim/stable", force=True)

    def layout(self):
        cmake_layout(self)
