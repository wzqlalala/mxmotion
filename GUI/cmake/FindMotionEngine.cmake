#-----------------------------------------------------------------------------
# 引入MotionEngine多体求解引擎（预编译 SDK，deps/MotionEngine/install）
# 引擎源码不在本源码库（源码检测只覆盖 Basic/ 与 Software/，deps/ 不在范围），
# 此处只消费预编译产物：默认用工作区 SDK 快照 deps/MotionEngine/install；
# 引擎本地联调可用 -DMotionEngine_SDK_DIR=<install路径>（或环境变量 MOTIONENGINE_SDK）
# 覆盖，例如直接指向引擎仓的 install，免同步工作区 SDK。
# （覆盖变量不能叫 MotionEngine_DIR——那是 find_package 保留变量，
#   config 搜索失败会被 CMake 写成 *-NOTFOUND 存入 cache。）
#-----------------------------------------------------------------------------

#防止重复引入
if(MOTIONENGINE_ALREADY_INCLUDED)
	return()
endif()
set(MOTIONENGINE_ALREADY_INCLUDED 1)

#设置路径
if(DEFINED MotionEngine_SDK_DIR)
	set(MotionEngine_Dir "${MotionEngine_SDK_DIR}")
elseif(DEFINED ENV{MOTIONENGINE_SDK})
	set(MotionEngine_Dir "$ENV{MOTIONENGINE_SDK}")
else()
	get_filename_component(MotionEngine_Dir "${CMAKE_SOURCE_DIR}/../../../deps/MotionEngine/install" ABSOLUTE)
endif()

set(MotionEngine_BIN_DIR "${MotionEngine_Dir}/bin")

set(MotionEngine_INC_DIRS
	"${MotionEngine_Dir}/include"
	"${MotionEngine_Dir}/include/motion"
	"${MotionEngine_Dir}/include/motion/collision/bullet"
	"${MotionEngine_Dir}/include/motion/collision/gimpact"
	"${MotionEngine_Dir}/include/motion/third_party/HACD"
	"${MotionEngine_Dir}/include/motion/third_party/HACDv2"
	"${MotionEngine_Dir}/../packages/eigen"
)

if(WIN32)
	add_library(MotionEngine::MotionEngine SHARED IMPORTED)
	set_property(TARGET MotionEngine::MotionEngine PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${MotionEngine_INC_DIRS})
	set_property(TARGET MotionEngine::MotionEngine APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE DEBUG)
	set_target_properties(MotionEngine::MotionEngine PROPERTIES
		IMPORTED_IMPLIB_RELEASE "${MotionEngine_Dir}/lib/motionengine.lib"
		IMPORTED_LOCATION_RELEASE "${MotionEngine_Dir}/bin/motionengine.dll"
		IMPORTED_IMPLIB_DEBUG "${MotionEngine_Dir}/lib/motionengined.lib"
		IMPORTED_LOCATION_DEBUG "${MotionEngine_Dir}/bin/motionengined.dll"
	)
	# 刚柔耦合 modal 模块（MotionEngine_modal，需引擎以 ENABLE_MODULE_MODAL=ON 构建并 INSTALL）
	# 【Debug/Release 分离】不带配置后缀的 IMPORTED_IMPLIB 会让 Debug 构建 also 链
	# Release 的 modal.lib——Debug(MDd/迭代器调试)与 Release(MD) 的 STL 容器布局
	# 不同，MotionFlexBodyParams 构造函数跨 DLL 导入后布局错位，运行期访问
	# attach_groups 即访问冲突（崩三天根因）。modald 由引擎 Debug+INSTALL 产出
	add_library(MotionEngine::Modal SHARED IMPORTED)
	set_property(TARGET MotionEngine::Modal PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${MotionEngine_INC_DIRS})
	set_property(TARGET MotionEngine::Modal APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE DEBUG)
	set_target_properties(MotionEngine::Modal PROPERTIES
		MAP_IMPORTED_CONFIG_RELWITHDEBINFO RELEASE
		MAP_IMPORTED_CONFIG_MINSIZEREL RELEASE
		IMPORTED_IMPLIB_RELEASE "${MotionEngine_Dir}/lib/MotionEngine_modal.lib"
		IMPORTED_LOCATION_RELEASE "${MotionEngine_Dir}/bin/MotionEngine_modal.dll"
		IMPORTED_IMPLIB_DEBUG "${MotionEngine_Dir}/lib/MotionEngine_modald.lib"
		IMPORTED_LOCATION_DEBUG "${MotionEngine_Dir}/bin/MotionEngine_modald.dll"
	)
else()
	add_library(MotionEngine::MotionEngine SHARED IMPORTED)
	set_property(TARGET MotionEngine::MotionEngine PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${MotionEngine_INC_DIRS})
	set_target_properties(MotionEngine::MotionEngine PROPERTIES
		IMPORTED_LOCATION_RELEASE "${MotionEngine_Dir}/lib/libmotionengine.so"
		IMPORTED_LOCATION_DEBUG "${MotionEngine_Dir}/lib/libmotionengined.so"
	)
	add_library(MotionEngine::Modal SHARED IMPORTED)
	set_property(TARGET MotionEngine::Modal PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${MotionEngine_INC_DIRS})
	set_target_properties(MotionEngine::Modal PROPERTIES
		IMPORTED_LOCATION "${MotionEngine_Dir}/lib/libMotionEngine_modal.so"
	)
endif()
