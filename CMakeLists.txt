cmake_minimum_required(VERSION 3.5)

project(HMIS VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets Sql REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets Sql REQUIRED)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        diagnosis.h
        diagnosis.cpp
        HMISRow.h
        Resources.qrc
        diagnoses.txt
        register.ui
        register.h
        register.cpp
        Icon.rc
)


if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    enable_language("RC")
    set (WIN32_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/Icon.rc)
endif()

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(HMIS
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        ${WIN32_RESOURCES}
    )

else()
    if(ANDROID)
        add_library(HMIS SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(HMIS WIN32
            ${PROJECT_SOURCES}
            ${WIN32_RESOURCES}
        )
    endif()
endif()

target_link_libraries(HMIS PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Sql)

set_target_properties(HMIS PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER com.yomedicalfiles.hmis
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS HMIS
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(HMIS)
endif()
