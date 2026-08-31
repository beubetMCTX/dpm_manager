cmake_minimum_required(VERSION 3.16)

project(dpm_manager VERSION 0.1 LANGUAGES CXX)

include(CTest)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(OpenCASCADE_DIR "D:/vcpkg/OPENCASCADE/INSTALL/cmake")
set(OpenCASCADE_INCLUDE_DIR "D:/vcpkg/OPENCASCADE/INSTALL/inc")
set(OpenCASCADE_LIB_DIR "D:/vcpkg/OPENCASCADE/INSTALL/win64/vc14/lib")
set(OpenCASCADE_BIN_DIR "D:/vcpkg/OPENCASCADE/INSTALL/win64/vc14/bin")

set(VTK_INCLUDEDIRS "D:/vcpkg/VTK/VTK-Install/include/vtk-9.5")
set(VTK_DIR "D:/vcpkg/VTK/VTK-Install/lib/cmake/vtk-9.5")
set(VTK_BIN_DIR "D:/vcpkg/VTK/VTK-Install/bin")

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
        ${MODEL_DIR}/unit.h
        ${VIEW_DIR}/occtwidget.h ${VIEW_DIR}/occtwidget.cpp
        ${IO_DIR}/base_geom_read.h ${IO_DIR}/base_geom_read.cpp
        ${COMMON_DIR}/global_functions.h
        ${COMMON_DIR}/app_config.h ${COMMON_DIR}/app_config.cpp
        ${COMMON_DIR}/runtime_debug.h ${COMMON_DIR}/runtime_debug.cpp
        ${UI_COMPONENTS_DIR}/qUI_components.h ${UI_COMPONENTS_DIR}/qUI_components.cpp
        ${UI_DIALOGS_DIR}/unit_edit_dialog.h ${UI_DIALOGS_DIR}/unit_edit_dialog.cpp ${UI_DIALOGS_DIR}/unit_edit_dialog.ui
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
#target_link_libraries(dpm_manager PRIVATE TKernel;TKMath;TKG2d;TKG3d;TKGeomBase;TKBRep;TKGeomAlgo;TKTopAlgo;TKPrim;TKBO;TKShHealing;TKBool;TKHLR;TKFillet;TKOffset;TKFeat;TKMesh;TKXMesh;TKService;TKV3d;TKOpenGl;TKMeshVS;TKIVtk;TKCDF;TKLCAF;TKCAF;TKBinL;TKXmlL;TKBin;TKXml;TKStdL;TKStd;TKTObj;TKBinTObj;TKXmlTObj;TKVCAF;TKDE;TKXSBase;TKDESTEP;TKXCAF;TKDEIGES;TKDESTL;TKDEVRML;TKRWMesh;TKDECascade;TKBinXCAF;TKXmlXCAF;TKDEOBJ;TKDEGLTF;TKDEPLY;TKExpress;TKDraw;TKTopTest;TKOpenGlTest;TKViewerTest;TKXSDRAW;TKDCAF;TKXDEDRAW;TKTObjDRAW;TKQADraw;TKIVtkDraw;TKXSDRAWDE;TKXSDRAWGLTF;TKXSDRAWIGES;TKXSDRAWOBJ;TKXSDRAWPLY;TKXSDRAWSTEP;TKXSDRAWSTL;TKXSDRAWVRML)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.dpm_manager)
endif()

option(BUILD_IO_SMOKE_TEST "Build the parser smoke-test executable" ON)
if(BUILD_IO_SMOKE_TEST)
    add_executable(io_smoke_test
        tests/io_smoke_test.cpp
        ${IO_DIR}/dpm_file_io.cpp
        ${IO_DIR}/chemkin_io.cpp
        ${MODEL_DIR}/injector.cpp
        ${MODEL_DIR}/injector.h
        ${MODEL_DIR}/unit.h
        ${COMMON_DIR}/runtime_debug.cpp
        ${COMMON_DIR}/runtime_debug.h
    )

    target_include_directories(io_smoke_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${IO_DIR}
        ${MODEL_DIR}
        ${COMMON_DIR}
    )

    target_link_libraries(io_smoke_test PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        ${OpenCASCADE_LIBRARIES}
    )

    set(DPM_SMOKE_DPM_FILE "" CACHE FILEPATH
        "DPM file used by the parser smoke test")
    set(DPM_SMOKE_CHEMKIN_FILE "" CACHE FILEPATH
        "Chemkin file used by the parser smoke test")
    if(BUILD_TESTING AND EXISTS "${DPM_SMOKE_DPM_FILE}" AND
       EXISTS "${DPM_SMOKE_CHEMKIN_FILE}")
        add_test(NAME io_smoke_test
            COMMAND io_smoke_test
                "${DPM_SMOKE_DPM_FILE}"
                "${DPM_SMOKE_CHEMKIN_FILE}")
    endif()
endif()

option(BUILD_COMPONENT_SMOKE_TEST "Build the reusable UI component smoke-test executable" ON)
if(BUILD_COMPONENT_SMOKE_TEST)
    add_executable(component_smoke_test
        tests/component_smoke_test.cpp
        ${UI_COMPONENTS_DIR}/qUI_components.cpp
        ${UI_COMPONENTS_DIR}/qUI_components.h
    )

    target_include_directories(component_smoke_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${UI_COMPONENTS_DIR}
    )

    target_link_libraries(component_smoke_test PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        unofficial::tinyexpr::tinyexpr
    )

    if(BUILD_TESTING)
        add_test(NAME component_smoke_test COMMAND component_smoke_test)
    endif()
endif()

option(BUILD_CONFIG_SMOKE_TEST "Build the configuration round-trip smoke-test executable" ON)
if(BUILD_CONFIG_SMOKE_TEST)
    add_executable(config_smoke_test
        tests/config_smoke_test.cpp
        ${COMMON_DIR}/app_config.cpp
        ${COMMON_DIR}/app_config.h
    )

    target_include_directories(config_smoke_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${COMMON_DIR}
    )

    target_link_libraries(config_smoke_test PRIVATE
        Qt${QT_VERSION_MAJOR}::Core
        Qt${QT_VERSION_MAJOR}::Gui
    )

    set_target_properties(config_smoke_test PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/config_smoke_runtime"
    )

    if(BUILD_TESTING)
        add_test(NAME config_smoke_test COMMAND config_smoke_test)
    endif()
endif()

option(BUILD_UNIT_EDITOR_SMOKE_TEST "Build the unit editor synchronization smoke-test executable" ON)
if(BUILD_UNIT_EDITOR_SMOKE_TEST)
    add_executable(unit_editor_smoke_test
        tests/unit_editor_smoke_test.cpp
        ${MODEL_DIR}/injector.cpp
        ${MODEL_DIR}/injector.h
        ${MODEL_DIR}/unit.h
        ${COMMON_DIR}/runtime_debug.cpp
        ${COMMON_DIR}/runtime_debug.h
        ${UI_COMPONENTS_DIR}/qUI_components.cpp
        ${UI_COMPONENTS_DIR}/qUI_components.h
        ${UI_DIALOGS_DIR}/unit_edit_dialog.cpp
        ${UI_DIALOGS_DIR}/unit_edit_dialog.h
        ${UI_DIALOGS_DIR}/unit_edit_dialog.ui
        ${UI_RESOURCES_DIR}/ui_icons.qrc
    )

    target_include_directories(unit_editor_smoke_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${MODEL_DIR}
        ${COMMON_DIR}
        ${UI_COMPONENTS_DIR}
        ${UI_DIALOGS_DIR}
    )

    target_link_libraries(unit_editor_smoke_test PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        unofficial::tinyexpr::tinyexpr
        ${OpenCASCADE_LIBRARIES}
    )

    if(BUILD_TESTING)
        add_test(NAME unit_editor_smoke_test COMMAND unit_editor_smoke_test)
    endif()
endif()

option(BUILD_SHUTDOWN_SMOKE_TEST "Build the auxiliary-dialog shutdown smoke-test executable" ON)
if(BUILD_SHUTDOWN_SMOKE_TEST)
    add_executable(shutdown_smoke_test
        tests/shutdown_smoke_test.cpp
        ${APP_DIR}/mainwindow.cpp
        ${APP_DIR}/mainwindow.h
        ${APP_DIR}/mainwindow.ui
        ${MODEL_DIR}/injector.cpp
        ${MODEL_DIR}/injector.h
        ${MODEL_DIR}/unit.h
        ${IO_DIR}/dpm_file_io.cpp
        ${IO_DIR}/dpm_file_io.h
        ${IO_DIR}/chemkin_io.cpp
        ${IO_DIR}/chemkin_io.h
        ${IO_DIR}/base_geom_read.cpp
        ${IO_DIR}/base_geom_read.h
        ${VIEW_DIR}/occtwidget.cpp
        ${VIEW_DIR}/occtwidget.h
        ${COMMON_DIR}/app_config.cpp
        ${COMMON_DIR}/app_config.h
        ${COMMON_DIR}/runtime_debug.cpp
        ${COMMON_DIR}/runtime_debug.h
        ${UI_COMPONENTS_DIR}/qUI_components.cpp
        ${UI_COMPONENTS_DIR}/qUI_components.h
        ${UI_DIALOGS_DIR}/unit_edit_dialog.cpp
        ${UI_DIALOGS_DIR}/unit_edit_dialog.h
        ${UI_DIALOGS_DIR}/unit_edit_dialog.ui
        ${UI_DIALOGS_DIR}/species_color_dialog.cpp
        ${UI_DIALOGS_DIR}/species_color_dialog.h
        ${UI_DIALOGS_DIR}/species_color_dialog.ui
        ${UI_DIALOGS_DIR}/species_material_dialog.cpp
        ${UI_DIALOGS_DIR}/species_material_dialog.h
        ${UI_DIALOGS_DIR}/species_material_dialog.ui
        ${UI_RESOURCES_DIR}/ui_icons.qrc
    )

    target_include_directories(shutdown_smoke_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${APP_DIR}
        ${IO_DIR}
        ${MODEL_DIR}
        ${VIEW_DIR}
        ${UI_COMPONENTS_DIR}
        ${UI_DIALOGS_DIR}
        ${COMMON_DIR}
    )

    target_link_libraries(shutdown_smoke_test PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt6::OpenGLWidgets
        unofficial::tinyexpr::tinyexpr
        glfw
        assimp::assimp
        ${GLEW_LIBRARIES}
        OpenGL::GL
        ${OpenCASCADE_LIBRARIES}
    )

    if(BUILD_TESTING)
        add_test(NAME shutdown_smoke_test COMMAND shutdown_smoke_test)
    endif()
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



