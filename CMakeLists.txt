cmake_minimum_required(VERSION 3.16)
project(lianwall-gui VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Qt6 策略设置
if(COMMAND qt_policy)
    qt_policy(SET QTP0001 NEW)
endif()

# ============================================================================
# Qt6 依赖
# ============================================================================
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Quick
    QuickControls2
    Network
    LinguistTools
)

# ============================================================================
# 源文件
# ============================================================================
set(SOURCES
    src/main.cpp
    src/Application.cpp
    src/ConfigManager.cpp
    src/DaemonClient.cpp
    src/DaemonState.cpp
    src/WallpaperListModel.cpp
    src/ThumbnailProvider.cpp
)

set(HEADERS
    src/Application.h
    src/ConfigManager.h
    src/Constants.h
    src/DaemonTypes.h
    src/DaemonClient.h
    src/DaemonState.h
    src/WallpaperListModel.h
    src/ThumbnailProvider.h
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
set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(lianwall-gui
    URI LianwallGui
    VERSION 1.0
    RESOURCE_PREFIX /qt/qml
    QML_FILES
        qml/main.qml
        qml/Theme.qml
        qml/components/NavBar.qml
        qml/components/CountdownBar.qml
        qml/components/TagEditor.qml
        qml/pages/PlaceholderPage.qml
        qml/pages/AboutPage.qml
        qml/pages/DashboardPage.qml
        qml/pages/LibraryPage.qml
        qml/pages/SettingsPage.qml
        qml/dialogs/WallpaperDetailDialog.qml
    RESOURCES
        resources/resources.qrc
)

target_link_libraries(lianwall-gui PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Network
)

# ============================================================================
# 翻译
# ============================================================================
set(TS_FILES
    translations/lianwall-gui_en.ts
    translations/lianwall-gui_zh_CN.ts
)

qt_add_translations(lianwall-gui
    TS_FILES ${TS_FILES}
    RESOURCE_PREFIX /translations
)

# ============================================================================
# 安装
# ============================================================================
install(TARGETS lianwall-gui
    RUNTIME DESTINATION bin
)

install(FILES packaging/lianwall-gui.desktop
    DESTINATION share/applications
)

install(FILES resources/icons/lianwall.png
    DESTINATION share/icons/hicolor/scalable/apps
)
