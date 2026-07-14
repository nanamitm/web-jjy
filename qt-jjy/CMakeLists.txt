cmake_minimum_required(VERSION 3.21)

project(QtJjySimulator VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.6 REQUIRED COMPONENTS Quick Multimedia)

qt_standard_project_setup(REQUIRES 6.6)

qt_add_executable(QtJjySimulator
    main.cpp
    jjyframebuilder.cpp
    jjyframebuilder.h
    jjyaudiodevice.cpp
    jjyaudiodevice.h
    jjycontroller.cpp
    jjycontroller.h
)

if(WIN32)
    target_sources(QtJjySimulator PRIVATE app.rc)
endif()

qt_add_qml_module(QtJjySimulator
    URI JjySimulator
    VERSION 1.0
    QML_FILES
        Main.qml
)

target_link_libraries(QtJjySimulator
    PRIVATE Qt6::Quick Qt6::Multimedia
)

set_target_properties(QtJjySimulator PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

if(ANDROID)
    set_property(TARGET QtJjySimulator PROPERTY
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    set_property(TARGET QtJjySimulator PROPERTY
        QT_ANDROID_APP_ICON "@drawable/jjy_icon")
endif()

include(GNUInstallDirs)
install(TARGETS QtJjySimulator
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
