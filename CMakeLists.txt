cmake_minimum_required(VERSION 3.28)
project(PardusDikte VERSION 1.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Quick QuickControls2)
find_package(PkgConfig REQUIRED)
pkg_check_modules(WEBRTC_APM REQUIRED webrtc-audio-processing-1)
include(FetchContent)
FetchContent_Declare(
    miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio
    GIT_TAG 0.11.25
    GIT_SHALLOW ON
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/lib/miniaudio
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(miniaudio)
find_package(Vulkan QUIET COMPONENTS glslc)
if(Vulkan_FOUND AND Vulkan_glslc_FOUND)
    set(GGML_VULKAN ON CACHE BOOL "" FORCE)
else()
    set(GGML_VULKAN OFF CACHE BOOL "" FORCE)
endif()
set(WHISPER_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(WHISPER_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GGML_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    whisper
    GIT_REPOSITORY https://github.com/ggml-org/whisper.cpp
    GIT_TAG v1.9.1
    GIT_SHALLOW ON
    SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/lib/whisper
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(whisper)
set_target_properties(whisper ggml ggml-base ggml-cpu PROPERTIES INSTALL_RPATH "$ORIGIN")
if(Vulkan_FOUND AND Vulkan_glslc_FOUND)
    set_target_properties(ggml-vulkan PROPERTIES INSTALL_RPATH "$ORIGIN")
endif()
add_executable(pardus-dikte
    src/main.cpp
    src/dictation.h src/dictation.cpp
    src/recorder.h src/recorder.cpp
    src/denoise.h src/denoise.cpp
    src/transcriber.h src/transcriber.cpp
    src/keybinding.h src/keybinding.cpp
    src/audio.cpp
)
qt_add_resources(pardus-dikte "assets"
    PREFIX "/"
    FILES
        src/main.qml
)
set_source_files_properties(src/denoise.cpp PROPERTIES COMPILE_OPTIONS "-std=c++17")
target_include_directories(pardus-dikte PRIVATE
    ${CMAKE_SOURCE_DIR}/lib/whisper/include
    ${CMAKE_SOURCE_DIR}/lib/miniaudio
    ${WEBRTC_APM_INCLUDE_DIRS}
)
target_link_libraries(pardus-dikte PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Quick
    Qt6::QuickControls2
    whisper
    ${WEBRTC_APM_LIBRARIES}
    ${CMAKE_DL_LIBS}
    pthread
    m
)
set_target_properties(pardus-dikte PROPERTIES INSTALL_RPATH "$ORIGIN")
set(WHISPER_MODEL_NAME "ggml-small-q5_1.bin")
set(WHISPER_MODEL_PATH ${CMAKE_SOURCE_DIR}/${WHISPER_MODEL_NAME})
if(NOT EXISTS ${WHISPER_MODEL_PATH})
    message(STATUS "Downloading ${WHISPER_MODEL_NAME}...")
    file(DOWNLOAD
        https://huggingface.co/ggerganov/whisper.cpp/resolve/main/${WHISPER_MODEL_NAME}
        ${WHISPER_MODEL_PATH}
        SHOW_PROGRESS
    )
endif()
add_custom_command(TARGET pardus-dikte POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${WHISPER_MODEL_PATH}
        $<TARGET_FILE_DIR:pardus-dikte>/${WHISPER_MODEL_NAME}
)
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
install(TARGETS pardus-dikte
    RUNTIME DESTINATION lib/pardus-dikte
)
install(TARGETS whisper ggml ggml-base ggml-cpu
    LIBRARY DESTINATION lib/pardus-dikte NAMELINK_SKIP
)
if(Vulkan_FOUND AND Vulkan_glslc_FOUND)
    install(TARGETS ggml-vulkan
        LIBRARY DESTINATION lib/pardus-dikte NAMELINK_SKIP)
endif()
install(FILES ${WHISPER_MODEL_PATH}
    DESTINATION lib/pardus-dikte)
install(FILES ${CMAKE_SOURCE_DIR}/assets/pardus-dikte.svg
    DESTINATION share/icons/hicolor/scalable/apps)
file(WRITE ${CMAKE_BINARY_DIR}/packaging/pardus-dikte.desktop
[=[[Desktop Entry]
Type=Application
Name=Pardus Dikte
Name[tr]=Pardus Dikte
GenericName=Voice Dictation
GenericName[tr]=Sesli Yazma
Comment=Dictate speech as text into any application
Comment[tr]=Konuşmanızı herhangi bir uygulamaya metin olarak yazdırın
Icon=pardus-dikte
Exec=/usr/lib/pardus-dikte/pardus-dikte
Categories=Utility;AudioVideo;Accessibility;
Keywords=dikte;ses;konuşma;yazma;dictation;voice;speech;
]=])
install(FILES ${CMAKE_BINARY_DIR}/packaging/pardus-dikte.desktop
    DESTINATION share/applications)
file(WRITE ${CMAKE_BINARY_DIR}/packaging/postinst
[=[#!/bin/sh
set -e
if [ "$1" = "configure" ]; then
    for u in $(loginctl list-users --no-legend 2>/dev/null | awk '{print $2}'); do
        uid=$(id -u "$u" 2>/dev/null) || continue
        real_env=""
        for p in /proc/[0-9]*; do
            [ "$(stat -c %u "$p" 2>/dev/null)" = "$uid" ] || continue
            grep -q "^DBUS_SESSION_BUS_ADDRESS=" "$p/environ" 2>/dev/null || continue
            real_env=$(tr '\0' '\n' < "$p/environ" | grep -E '^(DBUS_SESSION_BUS_ADDRESS|WAYLAND_DISPLAY|DISPLAY)=')
            break
        done
        [ -n "$real_env" ] || continue
        runuser -u "$u" -- env $real_env XDG_RUNTIME_DIR=/run/user/$uid \
            /usr/lib/pardus-dikte/pardus-dikte --register >/dev/null 2>&1 || true
    done
fi
exit 0
]=])
file(WRITE ${CMAKE_BINARY_DIR}/packaging/prerm
[=[#!/bin/sh
if [ "$1" = "remove" ] || [ "$1" = "purge" ]; then
    for u in $(loginctl list-users --no-legend 2>/dev/null | awk '{print $2}'); do
        uid=$(id -u "$u" 2>/dev/null) || continue
        real_env=""
        for p in /proc/[0-9]*; do
            [ "$(stat -c %u "$p" 2>/dev/null)" = "$uid" ] || continue
            grep -q "^DBUS_SESSION_BUS_ADDRESS=" "$p/environ" 2>/dev/null || continue
            real_env=$(tr '\0' '\n' < "$p/environ" | grep -E '^(DBUS_SESSION_BUS_ADDRESS|WAYLAND_DISPLAY|DISPLAY)=')
            break
        done
        [ -n "$real_env" ] || continue
        runuser -u "$u" -- env $real_env XDG_RUNTIME_DIR=/run/user/$uid \
            /usr/lib/pardus-dikte/pardus-dikte --unregister >/dev/null 2>&1 || true
    done
fi
exit 0
]=])
file(CHMOD ${CMAKE_BINARY_DIR}/packaging/postinst ${CMAKE_BINARY_DIR}/packaging/prerm
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
    "${CMAKE_BINARY_DIR}/packaging/postinst;${CMAKE_BINARY_DIR}/packaging/prerm")
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "pardus-dikte")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Ahmet <ahmetcikko@gmail.com>")
set(CPACK_DEBIAN_PACKAGE_DEPENDS
    "libqt6core6, libqt6gui6, libqt6widgets6, libqt6qml6, libqt6quick6, libqt6quickcontrols2-6, qml6-module-qtquick, qml6-module-qtquick-window, qml6-module-qtquick-controls, qml6-module-qtquick-templates, qml6-module-qtquick-layouts, qml6-module-qtquick-dialogs, libwebrtc-audio-processing-1-3, gnome-settings-daemon-common, libglib2.0-bin, procps, dbus-user-session, pulseaudio-utils")
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "libvulkan1")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Pardus Dikte - System-wide offline voice dictation for Pardus")
include(CPack)
