cmake_minimum_required(VERSION 3.16)
project(lianwall-gui VERSION 4.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Qt6 策略设置
if(COMMAND qt_policy)
    qt_policy(SET QTP0001 NEW)
endif()

# ============================================================================
# 选项
# ============================================================================
option(EMBED_LIANWALL "Embed lianwall binary into the application" ON)
set(LIANWALL_BINARY "" CACHE FILEPATH "Path to lianwall binary to embed")

# ============================================================================
# Qt6 依赖
# ============================================================================
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Quick
    QuickControls2
    Multimedia
    Network
    Concurrent
    LinguistTools
)

# ============================================================================
# 源文件
# ============================================================================
set(SOURCES
    src/main.cpp
    src/Application.cpp
    src/ProcessManager.cpp
    src/TrayManager.cpp
    src/LianwallClient.cpp
    src/ConfigManager.cpp
    src/ThumbnailCache.cpp
    src/models/StatusModel.cpp
    src/models/WallpaperModel.cpp
    src/models/TimelineModel.cpp
)

set(HEADERS
    src/Application.h
    src/ProcessManager.h
    src/TrayManager.h
    src/LianwallClient.h
    src/ConfigManager.h
    src/ThumbnailCache.h
    src/Constants.h
    src/models/StatusModel.h
    src/models/WallpaperModel.h
    src/models/TimelineModel.h
)

# ============================================================================
# 可执行文件
# ============================================================================
qt_add_executable(lianwall-gui
    ${SOURCES}
    ${HEADERS}
)

# ============================================================================
# QML 模块
# ============================================================================
qt_add_qml_module(lianwall-gui
    URI LianwallGui
    VERSION 1.0
    RESOURCE_PREFIX /qt/qml
    QML_FILES
        qml/main.qml
        qml/components/NavBar.qml
        qml/components/WallpaperPreview.qml
        qml/components/WallpaperGrid.qml
        qml/components/WallpaperCard.qml
        qml/components/SunflowerCanvas.qml
        qml/components/TimelineView.qml
        qml/components/TimelineTrack.qml
        qml/pages/HomePage.qml
        qml/pages/LibraryPage.qml
        qml/pages/TimelinePage.qml
        qml/pages/StatusPage.qml
        qml/pages/SettingsPage.qml
        qml/pages/AboutPage.qml
        qml/dialogs/ExitDialog.qml
        qml/dialogs/FilterDialog.qml
        qml/dialogs/FileOperationDialog.qml
    RESOURCES
        resources/resources.qrc
)

target_link_libraries(lianwall-gui PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Multimedia
    Qt6::Network
    Qt6::Concurrent
)

# ============================================================================
# 内嵌 lianwall 二进制文件
# ============================================================================
if(EMBED_LIANWALL)
    # 自动查找 lianwall 二进制文件
    if(NOT LIANWALL_BINARY)
        set(_lianwall_search_paths
            "${CMAKE_SOURCE_DIR}/../lianwall/target/release/lianwall"
            "${CMAKE_SOURCE_DIR}/../lianwall/target/debug/lianwall"
            "/usr/local/bin/lianwall"
            "/usr/bin/lianwall"
        )
        foreach(_path ${_lianwall_search_paths})
            if(EXISTS "${_path}")
                set(LIANWALL_BINARY "${_path}")
                break()
            endif()
        endforeach()
    endif()

    if(LIANWALL_BINARY AND EXISTS "${LIANWALL_BINARY}")
        message(STATUS "Embedding lianwall binary: ${LIANWALL_BINARY}")
        
        # 定义编译宏
        target_compile_definitions(lianwall-gui PRIVATE EMBED_LIANWALL=1)
        
        # 复制到构建目录
        add_custom_command(TARGET lianwall-gui POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${LIANWALL_BINARY}"
                "$<TARGET_FILE_DIR:lianwall-gui>/lianwall"
            COMMENT "Copying embedded lianwall binary"
        )
        
        # 安装时也复制
        install(PROGRAMS "${LIANWALL_BINARY}"
            DESTINATION lib/lianwall-gui
            RENAME lianwall
        )
    else()
        message(WARNING "EMBED_LIANWALL is ON but lianwall binary not found!")
        message(WARNING "Set LIANWALL_BINARY to the path of lianwall executable")
        message(WARNING "Or build lianwall first: cd ../lianwall && cargo build --release")
    endif()
endif()

# ============================================================================
# 翻译文件
# ============================================================================
set(TS_FILES
    translations/lianwall-gui_en.ts
    translations/lianwall-gui_zh_CN.ts
)

qt_add_translations(lianwall-gui
    TS_FILES ${TS_FILES}
    QM_FILES_OUTPUT_VARIABLE QM_FILES
)

# ============================================================================
# 安装规则
# ============================================================================
install(TARGETS lianwall-gui DESTINATION bin)
install(FILES packaging/lianwall-gui.desktop DESTINATION share/applications)
install(FILES resources/icons/lianwall.svg DESTINATION share/icons/hicolor/scalable/apps)

# ============================================================================
# CPack 配置
# ============================================================================
set(CPACK_PACKAGE_NAME "lianwall-gui")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "GUI for LianWall wallpaper manager")
set(CPACK_PACKAGE_CONTACT "Lian <https://github.com/Yueosa>")
include(CPack)
