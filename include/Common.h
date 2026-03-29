#pragma once

#include <QtWidgets>
#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QPixmap>
#include <QIcon>
#include <QStyle>
#include <QStyleOption>
#include <QScreen>
#include <QWindow>
#include <QSettings>

// Qt Core
#include <QString>
#include <QStringList>
#include <QMap>
#include <QList>
#include <QSet>
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QStandardPaths>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#include <dwmapi.h>
#include <shobjidl_core.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <winuser.h>
#endif

// 定数定義
namespace Constants {
    // アプリケーション情報
    const QString APP_NAME = "TaskBarEx";
    const QString APP_VERSION = "1.0.0";
    const QString ORGANIZATION_NAME = "TaskBarEx Project";
    
    // UI設定
    const int DEFAULT_ICON_SIZE = 22;
    const int UPDATE_INTERVAL_MS = 1000;
    const int MAX_WINDOW_TITLE_LENGTH = 30;
    const int TASKBAR_BUTTON_SPACING = 2;
    const int TASKBAR_BUTTON_PADDING = 4;
    
    // DPIスケール対応
    const QMap<int, int> DPI_ICON_SIZES = {
        {96, 22},   // 100%
        {120, 28},  // 125%
        {144, 33},  // 150%
        {192, 44}   // 200%
    };
    
    // Windows API定数
    #ifdef Q_OS_WIN
    const DWORD PROCESS_ACCESS_FLAGS = PROCESS_QUERY_INFORMATION | PROCESS_VM_READ;
    #endif
}

// ユーティリティ関数
namespace Utils {
    // DPI対応アイコンサイズ取得
    int getTaskbarIconSize();
    
    // 文字列省略
    QString truncateText(const QString& text, int maxLength);
    
    // Windows固有関数
    #ifdef Q_OS_WIN
    QString getProcessPath(DWORD processId);
    QString getProcessName(DWORD processId);
    QString getWindowTitle(HWND hwnd);
    bool isValidTaskbarWindow(HWND hwnd);
    #endif
}

// 列挙型定義
enum class WindowState {
    Normal,
    Minimized,
    Maximized,
    Hidden
};

enum class GroupingMode {
    AlwaysCombine,      // Windows 10デフォルト
    CombineWhenFull,    // タスクバー満杯時のみ
    NeverCombine        // 常に個別表示
};

// 前方宣言
class ApplicationInfo;
class TaskbarManager;
class WindowDetector;
class TaskbarGroupWidget;
class WindowInfoCache;
class IconExtractor;