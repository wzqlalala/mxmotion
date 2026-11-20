from conan import ConanFile
from conan.tools.cmake import cmake_layout
import platform


class RebuildGUIConan(ConanFile):
    """
    RebuildGUI 模块级 Conan 配方。
    声明主程序模块编译/运行所需的第三方依赖。
    """

    name = "rebuildgui"
    version = "2.0"
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False]}
    default_options = {
        "shared": True,
        "qt/*:shared": True,
        "opencascade/*:shared": True,
        "openscenegraph/*:shared": True,
        "openscenegraph/*:opengl_profile": "gl3",
        "openscenegraph/*:opencascade:shared": True,
        "openscenegraph/*:enable_deprecated_api": True,
        "cgminterop/*:shared": True,
        "onetbb/*:shared": True,
        "hwloc/*:shared": True,
        "qwt/*:shared": True,
        "qtads/*:shared": True,
        "qxlsx/*:shared": True,
        "ffmpeg/*:shared": True,
        # ffmpeg 原生 png 编解码器硬依赖 zlib，动画导出帧序列是 png，必须开启
        "ffmpeg/*:with_zlib": True,
        "openssl/*:shared": True,
        "muparser/*:shared": True,
        "python/*:shared": True,
        "pybind11/*:shared": True,
        "licenseshensi/*:shared": True,
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
        """目标平台是否为 Win7：优先读 profile conf（user.mxsim:win7_target，
        由 Auto_Build win7 平台层写入，支持 Win10 机器交叉编译 Win7 产物）；
        conf 缺失时回退嗅探构建机内核（真 Win7 编译机兼容路径）。
        内核 6.1 即 Windows 7：Win8.1+ 上未带 manifest 的进程 GetVersionExW
        会被改写为 6.2，因此 6.1 只会在真实 Win7 上出现；
        查询失败时回退 platform.release()。
        """
        if self.conf.get("user.mxsim:win7_target", check_type=bool):
            return True
        if platform.system() != "Windows":
            return False
        try:
            major, minor = self._get_windows_kernel_version()
        except Exception:
            return platform.release() == "7"
        return (major, minor) == (6, 1)

    def _target_platform(self):
        """目标平台配置标识：win7: Win7 线；windows: Win10 线；linux: Linux 线。"""
        if platform.system() == "Windows":
            return "win7" if self._is_win7() else "windows"
        return "linux"

    def requirements(self):
        # Qt 基础 + Web/网络组件
        self.requires("qt/5.13.2@mxsim/stable")

        # OpenCASCADE 几何内核（Windows 7.9.4；Linux 7.9.3；Win7/Linux 关闭 with_tk）
        self.requires("opencascade/7.9.5@mxsim/stable")
        self.default_options["opencascade/*:with_tk"] = False
        
        # OpenSceneGraph 渲染（Windows 3.6.5；Linux 3.6.6）
        if self._target_platform() == "linux":
            self.requires("openscenegraph/3.6.6@mxsim/stable")
        elif self._target_platform() == "win7":
            self.requires("openscenegraph/3.6.7@mxsim/stable")
        else:
            self.requires("openscenegraph/3.6.5@mxsim/stable")

        # CGMInterOP 商业 CAD 格式转换（Windows v5-6r2026，Win7 使用 v5-6r2019-2，为 Release-only FAT 包；
        # Linux 配置中无此包，跳过声明）
        if self._target_platform() == "win7":
            self.requires("cgminterop/v5-6r2019-2@mxsim/stable")
        elif self._target_platform() == "windows":
            self.requires("cgminterop/v5-6r2026@mxsim/stable")

        # TBB（oneTBB 2021+；MeSurfaceWrap 依赖 tbb::detail::r1 接口，2020.3 无此符号。
        #      Win7 不被 oneTBB 2021+ 支持，仍用 2020.3）
        if self._target_platform() == "win7":
            self.requires("onetbb/2020.3")
        else:
            self.requires("onetbb/2022.1.0@mxsim/stable")

        # Qt 生态库
        self.requires("qwt/6.2.0@mxsim/stable")
        self.requires("qtads/4.4.1@mxsim/stable")
        self.requires("qxlsx/1.5.0@mxsim/stable")

        # 多媒体/网络/解析（Windows 7.1.6；Win7 7.1.8；Linux 7.1.5 且关闭 pulse）
        if self._target_platform() == "linux":
            self.requires("ffmpeg/7.1.5@mxsim/stable")
            self.default_options["ffmpeg/*:with_pulse"] = False
        elif self._target_platform() == "win7":
            self.requires("ffmpeg/7.1.8@mxsim/stable")
        else:
            self.requires("ffmpeg/7.1.6@mxsim/stable")

        self.requires("openssl/3.6.3")
        self.requires("muparser/2.3.5")

        # MxNativeReaders 静态库的传递依赖（Basic/MxNativeReaders/conanfile.py
        # 声明 zlib/1.3.2，需在消费侧显式生成 ZLIBConfig.cmake 供 Find 模块使用；
        # ffmpeg 为预编译 FAT 包、不透出 zlib 配置，故必须显式 require）。
        # zlib 走 conan 默认静态链接（编入 PostIOFile.dll，无 zlib1.dll 运行时部署）。
        # 升级 SOP：两侧 conanfile 同步改版本 → 先重编安装 MxNativeReaders
        # → 再重跑本工程 conan install；漏改任一侧会被 FindMxNativeReaders.cmake
        # 的 configure 期校验（zlib_manifest.txt 版本比对）拦下
        self.requires("zlib/1.3.2")

        # 嵌入式 Python（Windows 3.7.17；Linux 3.8；Linux 预编译包仅 Release，由 CI/CD 工具通过 -s 强制）
        if self._target_platform() == "linux":
            self.requires("python/3.8@mxsim/stable")
        else:
            self.requires("python/3.7.17@mxsim/stable")

        self.requires("pybind11/2.7.1@mxsim/stable")

        # 深思数盾授权 SDK（仅 Windows，Win7/Win10 通用一套二进制；
        # Release-only，conan_packages.toml 中 force_build_type 强制 / 配方 compatibility 回退）
        if self._target_platform() in ("windows", "win7"):
            self.requires("licenseshensi/2.5.0@mxsim/stable")

        # 覆盖 transitive 依赖版本，避免冲突
        self.requires("freetype/2.10.4@mxsim/stable", force=True)

        # robustpredicates + rtree（2026-11-13 消费侧接线，缺陷修复：downstream-wiring）：
        # ① robustpredicates：RebuildBasicData 2026-11-12 切 conan 后其 install/include
        #    不再携带 robustPredicates.h（meshgeo-3rdparty-conan FR-04 零残留），而
        #    BasicData 安装头经五头链传递包含它——MeshEntity.h:8→MXGeoSolid.h:14、
        #    MXGeoEntity.h:7 / MXMeshFacet.h:7 / MXMeshLine.h:8 / MXMeshVertex.h:8→
        #    MXMeshParameters.h:5——GUI 全目标消费面（零 rp 符号实例化，仅头路径）
        # ② rtree：RebuildMesh install/include 的 DiscreteSurface.h:9 直接 include
        #    "rtree.h"，且被 MXMesh.h/meshFace.h/meshOptimization.h/meshTopologyOperate.h
        #    等 7 个 Mesh 安装头传递包含；meshFace.h:7→MXGeoPVTFace.h:8→rtree.h 同链
        #    （MechMainWindow 24 TU 编译实证 C1083 "rtree.h"）；header-only（仅头路径）
        self.requires("robustpredicates/2.0@mxsim/stable", transitive_headers=True)
        self.requires("rtree/20261111@mxsim/stable", transitive_headers=True)

    def layout(self):
        cmake_layout(self)
