cmake_minimum_required(VERSION 3.22)
project(LCALauncher VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -----------------------------------------------------------------------------
# Cross-Platform Qt 6 Path Resolution
# -----------------------------------------------------------------------------
# Allow user/CI to set QTDIR or CMAKE_PREFIX_PATH
if(DEFINED ENV{QTDIR})
    list(PREPEND CMAKE_PREFIX_PATH "$ENV{QTDIR}")
endif()

if(APPLE)
    list(PREPEND CMAKE_PREFIX_PATH 
        "/opt/homebrew/opt/qt" 
        "/opt/homebrew/opt/qt@6"
        "/usr/local/opt/qt"
        "/usr/local/opt/qt@6"
    )
elseif(WIN32)
    # Check default Windows Qt online installer locations
    file(GLOB WIN_QT_DIRS "C:/Qt/6.*/msvc2022_64" "C:/Qt/6.*/mingw_64")
    if(WIN_QT_DIRS)
        list(PREPEND CMAKE_PREFIX_PATH ${WIN_QT_DIRS})
    endif()
endif()

# Fix macOS 14+ / Xcode 15+ removal of deprecated AGL framework
if(APPLE)
    if(NOT TARGET WrapOpenGL::WrapOpenGL)
        add_library(WrapOpenGL::WrapOpenGL INTERFACE IMPORTED)
        set_target_properties(WrapOpenGL::WrapOpenGL PROPERTIES
            INTERFACE_LINK_LIBRARIES "-framework OpenGL"
        )
    endif()
endif()

find_package(Qt6 REQUIRED COMPONENTS 
    Core 
    Gui 
    Quick 
    Qml 
    QuickControls2 
    Network 
    Concurrent
)

qt_standard_project_setup(REQUIRES 6.5)
qt_policy(SET QTP0004 NEW)

# -----------------------------------------------------------------------------
# Platform-Specific Sources
# -----------------------------------------------------------------------------
set(PLATFORM_SOURCES "")
if(WIN32)
    set(PLATFORM_SOURCES "windows/LCALauncher.rc")
endif()

# -----------------------------------------------------------------------------
# Executable Target
# -----------------------------------------------------------------------------
qt_add_executable(LCALauncher
    src/main.cpp
    ${PLATFORM_SOURCES}
    
    # Core - Utils
    src/core/utils/FileUtils.h
    src/core/utils/FileUtils.cpp
    src/core/utils/HashUtils.h
    src/core/utils/HashUtils.cpp
    src/core/utils/UrlUtils.h
    src/core/utils/ArchiveUtils.h
    src/core/utils/ArchiveUtils.cpp

    # Core - Client
    src/core/client/ClientDownloader.h
    src/core/client/ClientDownloader.cpp

    # Core - Auth
    src/core/auth/UserSession.h
    src/core/auth/AuthService.h
    src/core/auth/AuthService.cpp

    # Core - Java
    src/core/java/JavaDetector.h
    src/core/java/JavaDetector.cpp
    src/core/java/JavaDownloader.h
    src/core/java/JavaDownloader.cpp

    # Core - Downloader
    src/core/downloader/DownloadTask.h
    src/core/downloader/DownloadManager.h
    src/core/downloader/DownloadManager.cpp

    # Core - Sync
    src/core/sync/SyncEngine.h
    src/core/sync/SyncEngine.cpp

    # Core - Launch (Minecraft 1.21.1 NeoForge)
    src/core/launch/ArgumentBuilder.h
    src/core/launch/ArgumentBuilder.cpp
    src/core/launch/GameProcess.h
    src/core/launch/GameProcess.cpp

    # Core - Server (Ping & Status)
    src/core/server/MinecraftPinger.h
    src/core/server/MinecraftPinger.cpp

    # Bridge (C++ <-> QML)
    src/bridge/AuthBridge.h
    src/bridge/AuthBridge.cpp
    src/bridge/SettingsBridge.h
    src/bridge/SettingsBridge.cpp
    src/bridge/LauncherBridge.h
    src/bridge/LauncherBridge.cpp
    src/bridge/ServerStatusBridge.h
    src/bridge/ServerStatusBridge.cpp
)

qt_add_qml_module(LCALauncher
    URI LCALauncher
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/components/LCAButton.qml
        qml/components/LCAProgressBar.qml
        qml/components/LCATextField.qml
        qml/views/MainView.qml
        qml/views/LoginView.qml
        qml/views/SettingsView.qml
        qml/views/ConsoleView.qml
    RESOURCES
        assets/logo.png
)

target_include_directories(LCALauncher PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(LCALauncher PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
    Qt6::QuickControls2
    Qt6::Network
    Qt6::Concurrent
)

# -----------------------------------------------------------------------------
# OS-Specific Target Properties
# -----------------------------------------------------------------------------
if(APPLE)
    # Strip deprecated -framework AGL if injected transitively
    get_target_property(_target_libs LCALauncher LINK_LIBRARIES)
    if(_target_libs)
        list(REMOVE_ITEM _target_libs "-framework AGL")
        set_target_properties(LCALauncher PROPERTIES LINK_LIBRARIES "${_target_libs}")
    endif()

    set_target_properties(LCALauncher PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "LCALauncher"
        MACOSX_BUNDLE_GUI_IDENTIFIER "org.lcaproject.LCALauncher"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    )
elseif(WIN32)
    set_target_properties(LCALauncher PROPERTIES
        WIN32_EXECUTABLE TRUE
        OUTPUT_NAME "LCALauncher"
    )
else() # Linux
    set_target_properties(LCALauncher PROPERTIES
        OUTPUT_NAME "lcalauncher"
    )
endif()

# -----------------------------------------------------------------------------
# Installation & Packaging (CPack)
# -----------------------------------------------------------------------------
include(GNUInstallDirs)

install(TARGETS LCALauncher
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(UNIX AND NOT APPLE)
    install(FILES linux/org.lcaproject.LCALauncher.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
    )
endif()

set(CPACK_PACKAGE_NAME "LCALauncher")
set(CPACK_PACKAGE_VENDOR "LCA Project")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Minecraft 1.21.1 NeoForge Custom Launcher")

if(APPLE)
    set(CPACK_GENERATOR "DragNDrop;TGZ")
elseif(WIN32)
    set(CPACK_GENERATOR "ZIP;NSIS")
else()
    set(CPACK_GENERATOR "TGZ;DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "LCA Team")
endif()

include(CPack)
