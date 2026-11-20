if(MxPre_WIN)
	#获取qmake.exe的绝对路径，存储到变量_qmake_executable
	get_target_property(_qmake_executable Qt5::qmake IMPORTED_LOCATION)
	#获取qmake.exe所在文件夹的绝对路径，存储到变量_qt_bin_dir
	get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)

	#将QT和第三方的动态库复制到目标文件夹
	#MESSAGE(STATUS "3.MxPre_Runtimes_Libraries = ${MxPre_Runtimes_Libraries}")
	#foreach(_lib ${MxPre_Runtimes_Libraries})
	#	add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
	#		COMMAND ${CMAKE_COMMAND} -E copy 	#执行CMake命令，CMAKE_COMMAND表示CMake的绝对路径
	#			$<TARGET_FILE:${_lib}>
	#		$<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>
	#	)
	#endforeach()
	
else()
	get_target_property(_qmake_executable Qt5::qmake IMPORTED_LOCATION)
	get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)

	#将QT和第三方的动态库复制到目标文件夹
	MESSAGE(STATUS "3.MxPre_Runtimes_Libraries = ${MxPre_Runtimes_Libraries}")
	foreach(_lib ${MxPre_Runtimes_Libraries})
		add_custom_command(TARGET MxMech POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy 	#执行CMake命令，CMAKE_COMMAND表示CMake的绝对路径
				$<TARGET_FILE:${_lib}>
			$<TARGET_FILE_DIR:MxMech>
		)
	endforeach()
	
endif()