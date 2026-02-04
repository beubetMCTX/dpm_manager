cmake_minimum_required(VERSION 3.16)

project(dpm_manager VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(OpenCASCADE_DIR "D:/vcpkg/OPENCASCADE/INSTALL/cmake")
set(OpenCASCADE_INCLUDE_DIR "D:/vcpkg/OPENCASCADE/INSTALL/inc")
set(OpenCASCADE_LINK_DIR "D:/vcpkg/OPENCASCADE/INSTALL/win64/vc14")

find_package(OpenCASCADE CONFIG)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt6 REQUIRED COMPONENTS OpenGLWidgets)
find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

add_definitions(-DWNT)


find_package(VTK COMPONENTS
CommonCore
CommonDataModel
FiltersCore
FiltersGeneral
GUISupportQt
RenderingQt
RenderingCore
RenderingOpenGL2
InteractionStyle)
#find_package(OpenCASCADE CONFIG REQUIRED)




set(TS_FILES dpm_manager_zh_CN.ts)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        ${TS_FILES}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(dpm_manager
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        injector.h injector.cpp
        dpm_file_io.h dpm_file_io.cpp
        unit.h


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

target_link_libraries(dpm_manager PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(dpm_manager PRIVATE ${GLEW_LIBRARIES})
target_link_libraries(dpm_manager PRIVATE glfw)
target_link_libraries(dpm_manager PRIVATE OpenGL::GL)
target_link_libraries(dpm_manager PRIVATE Qt6::OpenGLWidgets)
include_directories(${OpenCASCADE_INCLUDE_DIR})

target_link_libraries(dpm_manager PRIVATE ${OpenCASCADE_LIBRARIES})

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
