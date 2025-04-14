cmake_minimum_required(VERSION 3.30)

project(HMIS VERSION 2.2.2 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Use C++20 (or C++2a fallback) for designated initializers
if(MSVC)
    add_compile_options(/std:c++20)
else()
    # Check GCC or Clang version
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # GCC 10+ and Clang 10+ support -std=c++20
        if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 10.0)
            add_compile_options(-std=c++20)
        else()
            # Older versions use -std=c++2a (C++20 draft)
            add_compile_options(-std=c++2a -fconcepts)
        endif()
    else()
        # Default to -std=c++20 for other compilers (e.g., newer GCC/Clang)
        add_compile_options(-std=c++20)
    endif()
endif()

# Set the C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Widgets Sql Charts REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Widgets Sql Charts REQUIRED)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.hpp
    mainwindow.ui
    HMISRow.hpp
    Resources.qrc
    diagnoses.txt
    register.ui
    register.hpp
    register.cpp
    databaseOptions.hpp
    database.hpp
    database.cpp
    charts.hpp
    Icon.rc
)


if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    enable_language("RC")
    set(WIN32_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/Icon.rc)
endif()

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(HMIS MANUAL_FINALIZATION ${PROJECT_SOURCES} ${WIN32_RESOURCES})
else()
    if(ANDROID)
        add_library(HMIS SHARED ${PROJECT_SOURCES})
    else()
        add_executable(HMIS WIN32 ${PROJECT_SOURCES} ${WIN32_RESOURCES})
    endif()
endif()

target_link_libraries(HMIS PRIVATE 
    Qt${QT_VERSION_MAJOR}::Core 
    Qt${QT_VERSION_MAJOR}::Widgets 
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Charts
)

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
