# Reuse the uploaded GUI and data classes. Only missing SDK services are
# supplied by compat; no alternate property-panel implementation is generated.
set(original_data_dir "${PROJECT_SOURCE_DIR}/ModelData/src/MechMotionData")
file(GLOB original_motion_data CONFIGURE_DEPENDS "${original_data_dir}/*.cpp")
add_library(MotionOccOriginalData STATIC ${original_motion_data})
target_compile_definitions(MotionOccOriginalData PUBLIC BUILD_STATIC MX_MOTION_OCC_COMPAT)
target_include_directories(MotionOccOriginalData PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/compat" "${original_data_dir}")
target_link_libraries(MotionOccOriginalData PUBLIC Qt5::Gui)

set(original_gui_sources)
foreach(name IN ITEMS Body BodyBatch Joint Coupling Drive Spring VectorForce Contact Marker Sensor Controller
        ControlInput Scenario AutoFlex ElectroImport ElectroExport Hydraulic RL Mechanism Transmission MechElecHyd Animation Curve LoadExport LoadImport)
    list(APPEND original_gui_sources "${window_dir}/mMotion${name}Panel.cpp" "${window_dir}/mMotion${name}Panel.h")
endforeach()
foreach(name IN ITEMS mPickWidgetViewBase mPickWidgetView mDirectPickWidgetView mGeneralPickUp
        mPointPickerWidget mDirectionSelectorWidget mMotionRefMap mMotionJointSymbolHelper
        mMotionVectorSymbolHelper mMotionExprBuilderWidget mMotionFlexMxExport mMotionResultQueryService mMotionSensorEvaluator mWorkingDirPanel)
    list(APPEND original_gui_sources "${window_dir}/${name}.cpp" "${window_dir}/${name}.h")
endforeach()
foreach(name IN ITEMS mParamPanelBase mParamBarStyle mRoundTooltip mRoundListWidget)
    list(APPEND original_gui_sources "${gui_root}/src/CommonWidgets/${name}.cpp" "${gui_root}/src/CommonWidgets/${name}.h")
endforeach()
file(GLOB_RECURSE original_compat_sources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/compat/model/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/compat/picking/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/compat/render/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/compat/plot/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/compat/solver/*.cpp")
add_library(MotionOccOriginalGui STATIC ${original_gui_sources} ${original_compat_sources}
    "${foundation_dir}/mCurveWindow.cpp" "${foundation_dir}/mCurveWindow.h"
    "${gui_root}/src/IOFile/mIMotionJsonFile.cpp" "${gui_root}/src/IOFile/mIMotionJsonFile.h"
    "${gui_root}/src/IOFile/mOMotionJsonFile.cpp" "${gui_root}/src/IOFile/mOMotionJsonFile.h"
    compat/mCalcLineEdit.h compat/mGroupBox.h compat/mGlobalSignals.h compat/mMechGlobalSignals.h
    compat/mUnitSystemManager.h compat/mViewportModel.h compat/mViewportModelManager.h
    compat/model/LegacyMotionContext.h compat/picking/LegacyPickContext.h
    compat/picking/OccPickController.h compat/render/LegacyRender.h
    compat/solver/LegacyMotionSolverThread.h
    compat/mPlotView.h compat/mPlotViewStyleWidget.h)
set_target_properties(MotionOccOriginalData MotionOccOriginalGui PROPERTIES AUTOMOC ON)
target_include_directories(MotionOccOriginalGui PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/compat" "${CMAKE_CURRENT_SOURCE_DIR}/compat/model"
    "${CMAKE_CURRENT_SOURCE_DIR}" "${window_dir}" "${common_dir}" "${foundation_dir}"
    "${gui_root}/src/CommonWidgets" "${gui_root}/src/IOFile")
target_link_libraries(MotionOccOriginalGui PUBLIC MotionOccOriginalData MotionOccRigid MotionEngine::Core
    Qt5::Widgets Qt5::Svg Qt5::PrintSupport Qwt::Qwt)
target_compile_options(MotionOccOriginalData PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
target_compile_options(MotionOccOriginalGui PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8> $<$<CXX_COMPILER_ID:MSVC>:/bigobj>)
