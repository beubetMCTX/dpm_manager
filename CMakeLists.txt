cmake_minimum_required(VERSION 3.16)

project(dpm_manager VERSION 0.1 LANGUAGES CXX)

option(DPM_BUILD_TESTS "Build focused regression tests" ON)
option(DPM_ENABLE_ADVANCED_ATOMIZER_PREVIEW
       "Enable experimental advanced atomizer geometry previews"
       OFF)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(OpenCASCADE_DIR "D:/vcpkg/OPENCASCADE/INSTALL/cmake" CACHE PATH
    "OpenCASCADE CMake package directory")
set(OpenCASCADE_INCLUDE_DIR "D:/vcpkg/OPENCASCADE/INSTALL/inc" CACHE PATH
    "OpenCASCADE include directory")
set(OpenCASCADE_LIB_DIR "D:/vcpkg/OPENCASCADE/INSTALL/win64/vc14/lib" CACHE PATH
    "OpenCASCADE library directory")
set(OpenCASCADE_BIN_DIR "D:/vcpkg/OPENCASCADE/INSTALL/win64/vc14/bin" CACHE PATH
    "OpenCASCADE runtime directory")

set(VTK_INCLUDEDIRS "D:/vcpkg/VTK/VTK-Install/include/vtk-9.5" CACHE PATH
    "VTK include directory")
set(VTK_DIR "D:/vcpkg/VTK/VTK-Install/lib/cmake/vtk-9.5" CACHE PATH
    "VTK CMake package directory")
set(VTK_BIN_DIR "D:/vcpkg/VTK/VTK-Install/bin" CACHE PATH
    "VTK runtime directory")

option(USE_QT "Use Qt with VTK" ON)
add_definitions(-DWNT)



find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt6 REQUIRED COMPONENTS OpenGLWidgets)
find_package(glfw3 CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(unofficial-tinyexpr CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(VTK COMPONENTS
    CommonCore
    CommonDataModel
    FiltersCore
    FiltersGeneral
    GUISupportQt
    RenderingQt
    RenderingCore
    RenderingOpenGL2
    InteractionStyle
)

find_package(OpenCASCADE COMPONENTS REQUIRED)



set(TS_FILES dpm_manager_zh_CN.ts)

set(APP_DIR src/app)
set(IO_DIR src/io)
set(MODEL_DIR src/model)
set(VIEW_DIR src/view)
set(UI_COMPONENTS_DIR src/ui/components)
set(UI_DIALOGS_DIR src/ui/dialogs)
set(UI_RESOURCES_DIR src/ui/resources)
set(COMMON_DIR src/common)

set(DPM_QT_BIN_DIR "E:/Qt/6.7.3/msvc2019_64/bin" CACHE PATH
    "Qt runtime directory used by regression tests")
set(DPM_QT_PLUGIN_DIR "E:/Qt/6.7.3/msvc2019_64/plugins" CACHE PATH
    "Qt plugin directory used by regression tests")
set(DPM_TEST_DEPENDENCY_DIR
    "${CMAKE_SOURCE_DIR}/build/Desktop_Qt_6_7_3_MSVC2022_64bit-Release"
    CACHE PATH "Additional runtime DLL directory used by regression tests")

set(PROJECT_SOURCES
        ${APP_DIR}/main.cpp
        ${APP_DIR}/mainwindow.cpp
        ${APP_DIR}/mainwindow.h
        ${APP_DIR}/mainwindow.ui
        ${TS_FILES}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(dpm_manager
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        ${MODEL_DIR}/injector.h ${MODEL_DIR}/injector.cpp
        ${IO_DIR}/dpm_file_io.h ${IO_DIR}/dpm_file_io.cpp
        ${IO_DIR}/chemkin_io.h ${IO_DIR}/chemkin_io.cpp
        ${IO_DIR}/project_session.h ${IO_DIR}/project_session.cpp
        ${MODEL_DIR}/unit.h
        ${MODEL_DIR}/unit_array.h ${MODEL_DIR}/unit_array.cpp
        ${VIEW_DIR}/occtwidget.h ${VIEW_DIR}/occtwidget.cpp
        ${IO_DIR}/base_geom_read.h ${IO_DIR}/base_geom_read.cpp
        ${COMMON_DIR}/global_functions.h
        ${COMMON_DIR}/app_config.h ${COMMON_DIR}/app_config.cpp
        ${COMMON_DIR}/unit_system.h ${COMMON_DIR}/unit_system.cpp
        ${COMMON_DIR}/runtime_debug.h ${COMMON_DIR}/runtime_debug.cpp
        ${UI_COMPONENTS_DIR}/qUI_components.h ${UI_COMPONENTS_DIR}/qUI_components.cpp
        ${UI_DIALOGS_DIR}/unit_edit_dialog.h ${UI_DIALOGS_DIR}/unit_edit_dialog.cpp ${UI_DIALOGS_DIR}/unit_edit_dialog.ui
        ${UI_DIALOGS_DIR}/unit_preferences_dialog.h ${UI_DIALOGS_DIR}/unit_preferences_dialog.cpp
        ${UI_DIALOGS_DIR}/species_color_dialog.h ${UI_DIALOGS_DIR}/species_color_dialog.cpp ${UI_DIALOGS_DIR}/species_color_dialog.ui
        ${UI_DIALOGS_DIR}/species_material_dialog.h ${UI_DIALOGS_DIR}/species_material_dialog.cpp ${UI_DIALOGS_DIR}/species_material_dialog.ui
        ${UI_RESOURCES_DIR}/ui_icons.qrc




    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET dpm_manager APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation

    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    if(ANDROID)
        add_library(dpm_manager SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(dpm_manager
            ${PROJECT_SOURCES}
        )
    endif()

    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

target_include_directories(dpm_manager PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${APP_DIR}
    ${IO_DIR}
    ${MODEL_DIR}
    ${VIEW_DIR}
    ${UI_COMPONENTS_DIR}
    ${UI_DIALOGS_DIR}
    ${COMMON_DIR}
)

target_link_libraries(dpm_manager PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(dpm_manager PRIVATE ${GLEW_LIBRARIES})
target_link_libraries(dpm_manager PRIVATE glfw)
target_link_libraries(dpm_manager PRIVATE assimp::assimp)
target_link_libraries(dpm_manager PRIVATE unofficial::tinyexpr::tinyexpr)
target_link_libraries(dpm_manager PRIVATE OpenGL::GL)
target_link_libraries(dpm_manager PRIVATE Qt6::OpenGLWidgets)
target_link_libraries(dpm_manager PRIVATE ${OpenCASCADE_LIBRARIES})
if(DPM_ENABLE_ADVANCED_ATOMIZER_PREVIEW)
    target_compile_definitions(dpm_manager PRIVATE
        DPM_ENABLE_ADVANCED_ATOMIZER_PREVIEW=1)
endif()
#target_link_libraries(dpm_manager PRIVATE TKernel;TKMath;TKG2d;TKG3d;TKGeomBase;TKBRep;TKGeomAlgo;TKTopAlgo;TKPrim;TKBO;TKShHealing;TKBool;TKHLR;TKFillet;TKOffset;TKFeat;TKMesh;TKXMesh;TKService;TKV3d;TKOpenGl;TKMeshVS;TKIVtk;TKCDF;TKLCAF;TKCAF;TKBinL;TKXmlL;TKBin;TKXml;TKStdL;TKStd;TKTObj;TKBinTObj;TKXmlTObj;TKVCAF;TKDE;TKXSBase;TKDESTEP;TKXCAF;TKDEIGES;TKDESTL;TKDEVRML;TKRWMesh;TKDECascade;TKBinXCAF;TKXmlXCAF;TKDEOBJ;TKDEGLTF;TKDEPLY;TKExpress;TKDraw;TKTopTest;TKOpenGlTest;TKViewerTest;TKXSDRAW;TKDCAF;TKXDEDRAW;TKTObjDRAW;TKQADraw;TKIVtkDraw;TKXSDRAWDE;TKXSDRAWGLTF;TKXSDRAWIGES;TKXSDRAWOBJ;TKXSDRAWPLY;TKXSDRAWSTEP;TKXSDRAWSTL;TKXSDRAWVRML)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.dpm_manager)
endif()

set_target_properties(dpm_manager PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS dpm_manager
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(dpm_manager)
endif()

if(DPM_BUILD_TESTS)
    enable_testing()
    add_executable(project_session_regression
        tests/project_session_test.cpp
        ${IO_DIR}/project_session.h ${IO_DIR}/project_session.cpp
        ${MODEL_DIR}/injector.h ${MODEL_DIR}/injector.cpp
        ${COMMON_DIR}/app_config.h ${COMMON_DIR}/app_config.cpp
        ${COMMON_DIR}/unit_system.h ${COMMON_DIR}/unit_system.cpp
        ${COMMON_DIR}/runtime_debug.h ${COMMON_DIR}/runtime_debug.cpp
    )
    target_include_directories(project_session_regression PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${IO_DIR}
        ${MODEL_DIR}
        ${COMMON_DIR}
    )
    target_link_libraries(project_session_regression PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
    target_link_libraries(project_session_regression PRIVATE ${GLEW_LIBRARIES})
    target_link_libraries(project_session_regression PRIVATE glfw)
    target_link_libraries(project_session_regression PRIVATE assimp::assimp)
    target_link_libraries(project_session_regression PRIVATE unofficial::tinyexpr::tinyexpr)
    target_link_libraries(project_session_regression PRIVATE OpenGL::GL)
    target_link_libraries(project_session_regression PRIVATE Qt6::OpenGLWidgets)
    target_link_libraries(project_session_regression PRIVATE ${OpenCASCADE_LIBRARIES})
    add_test(NAME project_session_regression COMMAND project_session_regression)

    add_executable(unit_system_regression
        tests/unit_system_test.cpp
        ${COMMON_DIR}/unit_system.h ${COMMON_DIR}/unit_system.cpp
    )
    target_include_directories(unit_system_regression PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${COMMON_DIR}
    )
    target_link_libraries(unit_system_regression PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
    add_test(NAME unit_system_regression COMMAND unit_system_regression)

    add_executable(app_config_regression
        tests/app_config_test.cpp
        ${COMMON_DIR}/app_config.h ${COMMON_DIR}/app_config.cpp
        ${COMMON_DIR}/unit_system.h ${COMMON_DIR}/unit_system.cpp
    )
    target_include_directories(app_config_regression PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${COMMON_DIR}
    )
    target_link_libraries(app_config_regression PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
    add_test(NAME app_config_regression COMMAND app_config_regression)

    add_executable(qui_line_edit_regression
        tests/qui_line_edit_test.cpp
        ${COMMON_DIR}/unit_system.h ${COMMON_DIR}/unit_system.cpp
        ${UI_COMPONENTS_DIR}/qUI_components.h ${UI_COMPONENTS_DIR}/qUI_components.cpp
    )
    target_include_directories(qui_line_edit_regression PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${COMMON_DIR}
        ${UI_COMPONENTS_DIR}
    )
    target_link_libraries(qui_line_edit_regression PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
    target_link_libraries(qui_line_edit_regression PRIVATE unofficial::tinyexpr::tinyexpr)
    add_test(NAME qui_line_edit_regression COMMAND qui_line_edit_regression)
    set_tests_properties(qui_line_edit_regression
                         PROPERTIES ENVIRONMENT
                         "QT_QPA_PLATFORM=offscreen;QT_PLUGIN_PATH=E:/Qt/6.7.3/msvc2019_64/plugins")

    add_executable(chemkin_io_regression
        tests/chemkin_io_test.cpp
        ${IO_DIR}/chemkin_io.h ${IO_DIR}/chemkin_io.cpp
    )
    target_include_directories(chemkin_io_regression PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${IO_DIR}
    )
    target_link_libraries(chemkin_io_regression PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
    add_test(NAME chemkin_io_regression COMMAND chemkin_io_regression)

    add_executable(dpm_file_io_regression
        tests/dpm_file_io_test.cpp
        ${IO_DIR}/dpm_file_io.h ${IO_DIR}/dpm_file_io.cpp
        ${MODEL_DIR}/injector.h ${MODEL_DIR}/injector.cpp
        ${COMMON_DIR}/runtime_debug.h ${COMMON_DIR}/runtime_debug.cpp
    )
    target_include_directories(dpm_file_io_regression PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${IO_DIR}
        ${MODEL_DIR}
        ${COMMON_DIR}
    )
    target_link_libraries(dpm_file_io_regression PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
    target_link_libraries(dpm_file_io_regression PRIVATE ${GLEW_LIBRARIES})
    target_link_libraries(dpm_file_io_regression PRIVATE glfw)
    target_link_libraries(dpm_file_io_regression PRIVATE assimp::assimp)
    target_link_libraries(dpm_file_io_regression PRIVATE unofficial::tinyexpr::tinyexpr)
    target_link_libraries(dpm_file_io_regression PRIVATE OpenGL::GL)
    target_link_libraries(dpm_file_io_regression PRIVATE Qt6::OpenGLWidgets)
    target_link_libraries(dpm_file_io_regression PRIVATE ${OpenCASCADE_LIBRARIES})
    add_test(NAME dpm_file_io_regression COMMAND dpm_file_io_regression)

    add_executable(base_geom_read_regression
        tests/base_geom_read_test.cpp
        ${IO_DIR}/base_geom_read.h ${IO_DIR}/base_geom_read.cpp
    )
    target_include_directories(base_geom_read_regression PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${IO_DIR}
    )
    target_link_libraries(base_geom_read_regression PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
    target_link_libraries(base_geom_read_regression PRIVATE ${OpenCASCADE_LIBRARIES})
    add_test(NAME base_geom_read_regression COMMAND base_geom_read_regression)

    add_executable(injector_geometry_regression
        tests/injector_geometry_test.cpp
        ${MODEL_DIR}/injector.h ${MODEL_DIR}/injector.cpp
        ${COMMON_DIR}/runtime_debug.h ${COMMON_DIR}/runtime_debug.cpp
    )
    target_include_directories(injector_geometry_regression PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${MODEL_DIR}
        ${COMMON_DIR}
    )
    target_link_libraries(injector_geometry_regression PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
    target_link_libraries(injector_geometry_regression PRIVATE ${GLEW_LIBRARIES})
    target_link_libraries(injector_geometry_regression PRIVATE glfw)
    target_link_libraries(injector_geometry_regression PRIVATE assimp::assimp)
    target_link_libraries(injector_geometry_regression PRIVATE unofficial::tinyexpr::tinyexpr)
    target_link_libraries(injector_geometry_regression PRIVATE OpenGL::GL)
    target_link_libraries(injector_geometry_regression PRIVATE Qt6::OpenGLWidgets)
    target_link_libraries(injector_geometry_regression PRIVATE ${OpenCASCADE_LIBRARIES})
    add_test(NAME injector_geometry_regression COMMAND injector_geometry_regression)

    add_executable(auxiliary_dialog_lifetime_regression
        tests/auxiliary_dialog_lifetime_test.cpp
        ${COMMON_DIR}/app_config.h ${COMMON_DIR}/app_config.cpp
        ${COMMON_DIR}/runtime_debug.h ${COMMON_DIR}/runtime_debug.cpp
        ${COMMON_DIR}/unit_system.h ${COMMON_DIR}/unit_system.cpp
        ${UI_COMPONENTS_DIR}/qUI_components.h ${UI_COMPONENTS_DIR}/qUI_components.cpp
        ${UI_DIALOGS_DIR}/species_color_dialog.h ${UI_DIALOGS_DIR}/species_color_dialog.cpp ${UI_DIALOGS_DIR}/species_color_dialog.ui
        ${UI_DIALOGS_DIR}/species_material_dialog.h ${UI_DIALOGS_DIR}/species_material_dialog.cpp ${UI_DIALOGS_DIR}/species_material_dialog.ui
    )
    target_include_directories(auxiliary_dialog_lifetime_regression PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${COMMON_DIR}
        ${UI_COMPONENTS_DIR}
        ${UI_DIALOGS_DIR}
    )
    target_link_libraries(auxiliary_dialog_lifetime_regression PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
    target_link_libraries(auxiliary_dialog_lifetime_regression PRIVATE unofficial::tinyexpr::tinyexpr)
    add_test(NAME auxiliary_dialog_lifetime_regression
             COMMAND auxiliary_dialog_lifetime_regression)
set_tests_properties(auxiliary_dialog_lifetime_regression
                         PROPERTIES ENVIRONMENT
                         "QT_QPA_PLATFORM=offscreen;QT_PLUGIN_PATH=E:/Qt/6.7.3/msvc2019_64/plugins")

    add_executable(unit_edit_dialog_lifetime_regression
        tests/unit_edit_dialog_lifetime_test.cpp
        ${MODEL_DIR}/injector.h ${MODEL_DIR}/injector.cpp
        ${COMMON_DIR}/app_config.h ${COMMON_DIR}/app_config.cpp
        ${COMMON_DIR}/runtime_debug.h ${COMMON_DIR}/runtime_debug.cpp
        ${COMMON_DIR}/unit_system.h ${COMMON_DIR}/unit_system.cpp
        ${UI_COMPONENTS_DIR}/qUI_components.h ${UI_COMPONENTS_DIR}/qUI_components.cpp
        ${UI_DIALOGS_DIR}/unit_edit_dialog.h ${UI_DIALOGS_DIR}/unit_edit_dialog.cpp ${UI_DIALOGS_DIR}/unit_edit_dialog.ui
    )
    target_include_directories(unit_edit_dialog_lifetime_regression PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${COMMON_DIR}
        ${MODEL_DIR}
        ${UI_COMPONENTS_DIR}
        ${UI_DIALOGS_DIR}
    )
    target_link_libraries(unit_edit_dialog_lifetime_regression PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
    target_link_libraries(unit_edit_dialog_lifetime_regression PRIVATE unofficial::tinyexpr::tinyexpr)
    target_link_libraries(unit_edit_dialog_lifetime_regression PRIVATE ${GLEW_LIBRARIES})
    target_link_libraries(unit_edit_dialog_lifetime_regression PRIVATE glfw)
    target_link_libraries(unit_edit_dialog_lifetime_regression PRIVATE assimp::assimp)
    target_link_libraries(unit_edit_dialog_lifetime_regression PRIVATE OpenGL::GL)
    target_link_libraries(unit_edit_dialog_lifetime_regression PRIVATE Qt6::OpenGLWidgets)
    target_link_libraries(unit_edit_dialog_lifetime_regression PRIVATE ${OpenCASCADE_LIBRARIES})
    add_test(NAME unit_edit_dialog_lifetime_regression
             COMMAND unit_edit_dialog_lifetime_regression)
    set_tests_properties(unit_edit_dialog_lifetime_regression
             PROPERTIES ENVIRONMENT
                         "QT_QPA_PLATFORM=offscreen;QT_PLUGIN_PATH=E:/Qt/6.7.3/msvc2019_64/plugins")

    add_executable(occtwidget_edit_history_regression
        tests/occtwidget_edit_history_test.cpp
        ${VIEW_DIR}/occtwidget.h ${VIEW_DIR}/occtwidget.cpp
        ${IO_DIR}/base_geom_read.h ${IO_DIR}/base_geom_read.cpp
        ${MODEL_DIR}/injector.h ${MODEL_DIR}/injector.cpp
        ${COMMON_DIR}/app_config.h ${COMMON_DIR}/app_config.cpp
        ${COMMON_DIR}/runtime_debug.h ${COMMON_DIR}/runtime_debug.cpp
        ${COMMON_DIR}/unit_system.h ${COMMON_DIR}/unit_system.cpp
        ${UI_COMPONENTS_DIR}/qUI_components.h ${UI_COMPONENTS_DIR}/qUI_components.cpp
        ${UI_DIALOGS_DIR}/unit_edit_dialog.h ${UI_DIALOGS_DIR}/unit_edit_dialog.cpp ${UI_DIALOGS_DIR}/unit_edit_dialog.ui
    )
    target_include_directories(occtwidget_edit_history_regression PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${VIEW_DIR}
        ${IO_DIR}
        ${MODEL_DIR}
        ${COMMON_DIR}
        ${UI_COMPONENTS_DIR}
        ${UI_DIALOGS_DIR}
    )
    target_link_libraries(occtwidget_edit_history_regression PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
    target_link_libraries(occtwidget_edit_history_regression PRIVATE ${GLEW_LIBRARIES})
    target_link_libraries(occtwidget_edit_history_regression PRIVATE glfw)
    target_link_libraries(occtwidget_edit_history_regression PRIVATE assimp::assimp)
    target_link_libraries(occtwidget_edit_history_regression PRIVATE unofficial::tinyexpr::tinyexpr)
    target_link_libraries(occtwidget_edit_history_regression PRIVATE OpenGL::GL)
    target_link_libraries(occtwidget_edit_history_regression PRIVATE Qt6::OpenGLWidgets)
    target_link_libraries(occtwidget_edit_history_regression PRIVATE ${OpenCASCADE_LIBRARIES})
    add_test(NAME occtwidget_edit_history_regression
             COMMAND occtwidget_edit_history_regression)

    # Keep CTest independent from the developer shell's PATH. The regression
    # executables link against the same Qt/OpenCASCADE/vcpkg runtime set as
    # the Release package.
    set(DPM_TEST_NAMES
        project_session_regression
        unit_system_regression
        app_config_regression
        qui_line_edit_regression
        chemkin_io_regression
        dpm_file_io_regression
        base_geom_read_regression
        injector_geometry_regression
        auxiliary_dialog_lifetime_regression
        unit_edit_dialog_lifetime_regression
        occtwidget_edit_history_regression)
    set(DPM_TEST_RUNTIME_PATH
        "${CMAKE_BINARY_DIR};${DPM_TEST_DEPENDENCY_DIR};${OpenCASCADE_BIN_DIR};${DPM_QT_BIN_DIR}")
    string(REPLACE ";" "\\;" DPM_TEST_RUNTIME_PATH_ESCAPED
           "${DPM_TEST_RUNTIME_PATH}")
    set_tests_properties(${DPM_TEST_NAMES}
                         PROPERTIES ENVIRONMENT
                         "QT_QPA_PLATFORM=offscreen;QT_PLUGIN_PATH=${DPM_QT_PLUGIN_DIR};PATH=${DPM_TEST_RUNTIME_PATH_ESCAPED}")

    # OCCTWidget creates a native WNT window and cannot be exercised with
    # Qt's offscreen platform. Keep this focused history test on Windows.
    set_tests_properties(occtwidget_edit_history_regression
                         PROPERTIES ENVIRONMENT
                         "QT_QPA_PLATFORM=windows;QT_PLUGIN_PATH=${DPM_QT_PLUGIN_DIR};PATH=${DPM_TEST_RUNTIME_PATH_ESCAPED}")
endif()



