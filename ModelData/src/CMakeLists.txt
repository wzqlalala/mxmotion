IF (WIN32)
    add_compile_options(/utf-8)
else()
    add_compile_options(-finput-charset=utf-8 -fexec-charset=utf-8)
endif()

set(SOVERSION ${PROJECT_VERSION} ${PROJECT_VERSION_MAJOR} ${PROJECT_VERSION_MINOR} ${PROJECT_VERSION_PATCH})

#-----------------------------------------------------------------------------
# 所有项目
#-----------------------------------------------------------------------------
# 查找所有子目录（绝对路径）
file(GLOB all_subdirs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*)

set(_libraries "")
foreach(entry ${all_subdirs})
    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${entry}")
        list(APPEND _libraries "${entry}")
    endif()
endforeach()

#-----------------------------------------------------------------------------
# 处理程序的RPATH
#-----------------------------------------------------------------------------
file(RELATIVE_PATH _rel ${CMAKE_INSTALL_PREFIX}/${INSTALL_BINDIR} ${CMAKE_INSTALL_PREFIX})

if(_APPLE_)
	set(_rpath "@loader_path/${_rel}")
else()
	set(_rpath "\$ORIGIN:\$ORIGIN/${_rel}")
endif()

file(TO_NATIVE_PATH "${_rpath}/${INSTALL_LIBDIR}" _lib_rpath)


foreach(_library ${_libraries})
	
	add_subdirectory(${_library})
	
	set_target_properties(${_library}
		PROPERTIES 
			MACOSX_RPATH ON
			SKIP_BUILD_RPATH OFF
			BUILD_WITH_INSTALL_RPATH ON
			INSTALL_RPATH "${_lib_rpath}"
			INSTALL_RPATH_USE_LINK_PATH ON
			#POSITION_INDEPENDENT_CODE 1
			#OUTPUT_NAME $<LOWER_CASE:${_library}>
			#DEBUG_POSTFIX "_d"
			VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR}
			FOLDER Modules
	)

endforeach()


#-----------------------------------------------------------------------------
# 安装MxMesh，定义项目的安装目录
#-----------------------------------------------------------------------------
install(
	TARGETS ${_libraries}
    EXPORT ${PROJECT_NAME}Targets
	ARCHIVE DESTINATION ${INSTALL_LIBDIR} COMPONENT lib
	RUNTIME DESTINATION ${INSTALL_BINDIR} COMPONENT bin
)

install(EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Config.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake
)
