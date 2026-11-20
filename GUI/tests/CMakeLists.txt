# 回归测试聚合:各模块测试目录逐个挂入(根 CMakeLists 经
# tests/CMakeLists.txt 存在性判断决定是否注册 ctest)
add_subdirectory(PostWindow)
add_subdirectory(PostCurveMath)
add_subdirectory(PostCurveAscii)
add_subdirectory(PostCurve3D)
add_subdirectory(PostCurveDocName)
# PostCurve 三维基准面（R41-T41-2）模型层无头自测
add_subdirectory(PostCurveDatumPlane)
