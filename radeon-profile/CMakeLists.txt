cmake_minimum_required(VERSION 3.19)

project(radeon-profile
    VERSION 20200824
)

find_package(Qt5 REQUIRED COMPONENTS
    Core
    Widgets
    Network
    Charts
    LinguistTools
)

find_package(X11 REQUIRED)

find_library(LIBDRM drm REQUIRED)

add_executable(${PROJECT_NAME}
    main.cpp
    radeon_profile.cpp
    uiElements.cpp
    uiEvents.cpp
    gpu.cpp
    dxorg.cpp
    settings.cpp
    daemonComm.cpp
    ioctlHandler.cpp
    ioctl_radeon.cpp
    ioctl_amdgpu.cpp
    execbin.cpp
    dialogs/dialog_defineplot.cpp
    dialogs/dialog_rpevent.cpp
    dialogs/dialog_topbarcfg.cpp
    dialogs/dialog_deinetopbaritem.cpp
    tab_events.cpp
    tab_plots.cpp
    tab_fanControl.cpp
    tab_exec.cpp
    tab_overclock.cpp
    dialogs/dialog_sliders.cpp
    dialogs/dialog_plotsconfiguration.cpp
)

target_sources(${PROJECT_NAME} PRIVATE
    radeon_profile.h
    gpu.h
    dxorg.h
    globalStuff.h
    daemonComm.h
    execbin.h
    rpevent.h
    ioctlHandler.h
    components/rpplot.h
    components/pieprogressbar.h
    components/topbarcomponents.h
    dialogs/dialog_defineplot.h
    dialogs/dialog_rpevent.h
    dialogs/dialog_topbarcfg.h
    dialogs/dialog_deinetopbaritem.h
    dialogs/dialog_sliders.h
    dialogs/dialog_plotsconfiguration.h
    components/slider.h
    radeon_profile.ui
    components/pieprogressbar.ui
    components/slider.ui
    dialogs/dialog_defineplot.ui
    dialogs/dialog_plotsconfiguration.ui
    dialogs/dialog_rpevent.ui
    dialogs/dialog_topbarcfg.ui
    dialogs/dialog_deinetopbaritem.ui
    dialogs/dialog_sliders.ui
    radeon-resource.qrc
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt5::Core
    Qt5::Widgets
    Qt5::Network
    Qt5::Charts
    X11::X11
    X11::Xrandr
    ${LIBDRM}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<CONFIG:Release>:QT_NO_DEBUG_OUTPUT>
)

install(TARGETS ${PROJECT_NAME}
    DESTINATION bin
)

add_subdirectory(translations)
add_subdirectory(extra)
