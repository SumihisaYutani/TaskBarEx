cmake_minimum_required(VERSION 3.16)
project(TaskBarEx VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt6 設定
set(CMAKE_PREFIX_PATH "C:/Qt/6.10.0/mingw_64" ${CMAKE_PREFIX_PATH})
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
qt6_standard_project_setup()

# Qt Auto設定
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/ui)

# Windows API 設定
if(WIN32)
    set(CMAKE_SYSTEM_VERSION 10.0)
    add_definitions(-DUNICODE -D_UNICODE)
endif()

# インクルードディレクトリ
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party)

# ソースファイル
set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/TaskbarWindow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/TaskbarModel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/TaskbarGroupManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/TaskbarVisibilityMonitor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PinnedAppsManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/WindowThumbnailManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ThumbnailPreviewWidget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Logger.cpp
)

# ヘッダーファイル
set(HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/TaskbarWindow.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/TaskbarModel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/TaskbarGroupManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/TaskbarVisibilityMonitor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/PinnedAppsManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/WindowThumbnailManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ThumbnailPreviewWidget.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/WindowInfo.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/Logger.h
)

# UIファイル
set(UI_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/TaskbarWindow.ui
)

# リソースファイル
set(RESOURCES
    resources/icons.qrc
    resources/styles.qrc
)

# 実行ファイル作成（WIN32指定でターミナル無し）
qt6_add_executable(TaskBarEx WIN32 ${SOURCES} ${HEADERS} ${UI_FILES})
# qt6_add_executable(TaskBarEx WIN32 ${SOURCES} ${HEADERS} ${UI_FILES} ${RESOURCES})

# ヘッダーファイルをターゲットに明示的に追加してMOC処理を確実にする
target_sources(TaskBarEx PRIVATE ${HEADERS})

# Qtライブラリとリンク
target_link_libraries(TaskBarEx 
    PRIVATE
        Qt6::Core 
        Qt6::Widgets
)

# Windows固有ライブラリとリンク
if(WIN32)
    target_link_libraries(TaskBarEx 
        PRIVATE
            user32 
            kernel32 
            shell32 
            ole32 
            oleaut32
            dwmapi
            shcore
    )
endif()

# コンパイルオプション
target_compile_options(TaskBarEx PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
)

# Qt6 DLLの自動配置
if(WIN32)
    # Qt6 DLLを実行ファイルの場所にコピー（リリース版を使用）
    add_custom_command(TARGET TaskBarEx POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "C:/Qt/6.10.0/mingw_64/bin/Qt6Core.dll"
        "$<TARGET_FILE_DIR:TaskBarEx>"
        COMMENT "Copying Qt6Core.dll"
    )
    add_custom_command(TARGET TaskBarEx POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "C:/Qt/6.10.0/mingw_64/bin/Qt6Gui.dll" 
        "$<TARGET_FILE_DIR:TaskBarEx>"
        COMMENT "Copying Qt6Gui.dll"
    )
    add_custom_command(TARGET TaskBarEx POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "C:/Qt/6.10.0/mingw_64/bin/Qt6Widgets.dll"
        "$<TARGET_FILE_DIR:TaskBarEx>"
        COMMENT "Copying Qt6Widgets.dll"
    )
    # MinGWランタイムDLLもコピー
    add_custom_command(TARGET TaskBarEx POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "C:/Qt/6.10.0/mingw_64/bin/libgcc_s_seh-1.dll"
        "$<TARGET_FILE_DIR:TaskBarEx>"
        COMMENT "Copying MinGW runtime DLL"
    )
    add_custom_command(TARGET TaskBarEx POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "C:/Qt/6.10.0/mingw_64/bin/libstdc++-6.dll"
        "$<TARGET_FILE_DIR:TaskBarEx>"
        COMMENT "Copying MinGW stdlib DLL"
    )
    add_custom_command(TARGET TaskBarEx POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "C:/Qt/6.10.0/mingw_64/bin/libwinpthread-1.dll"
        "$<TARGET_FILE_DIR:TaskBarEx>"
        COMMENT "Copying MinGW pthread DLL"
    )
    
    # Qt6プラットフォームプラグインのコピー
    add_custom_command(TARGET TaskBarEx POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_FILE_DIR:TaskBarEx>/platforms"
        COMMENT "Creating platforms directory"
    )
    add_custom_command(TARGET TaskBarEx POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "C:/Qt/6.10.0/mingw_64/plugins/platforms/qwindows.dll"
        "$<TARGET_FILE_DIR:TaskBarEx>/platforms/"
        COMMENT "Copying Qt6 Windows platform plugin"
    )
    add_custom_command(TARGET TaskBarEx POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "C:/Qt/6.10.0/mingw_64/plugins/platforms/qminimal.dll"
        "$<TARGET_FILE_DIR:TaskBarEx>/platforms/"
        COMMENT "Copying Qt6 minimal platform plugin"
    )
endif()

# インストール設定
install(TARGETS TaskBarEx
    RUNTIME DESTINATION bin
)

# CPack設定
include(CPack)
set(CPACK_PACKAGE_NAME "TaskBarEx")
set(CPACK_PACKAGE_VENDOR "TaskBarEx Project")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Windows Taskbar Extension Tool")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_GENERATOR "ZIP;NSIS")