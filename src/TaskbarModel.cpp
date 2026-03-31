#include "TaskbarModel.h"
#include "Logger.h"
#include <QApplication>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <algorithm>
#include <shellapi.h>
#include <tlhelp32.h>
#include <psapi.h>

TaskbarModel::TaskbarModel(QObject *parent)
    : QObject(parent)
{
}

TaskbarModel::~TaskbarModel()
{
}

void TaskbarModel::updateWindowList()
{
    m_windows.clear();
    m_visibleWindows.clear();
    
    // 無効になったウィンドウの順序情報をクリーンアップ
    QList<HWND> invalidHwnds;
    for (auto it = m_windowOrder.begin(); it != m_windowOrder.end(); ++it) {
        if (!IsWindow(it.key())) {
            invalidHwnds.append(it.key());
        }
    }
    for (HWND hwnd : invalidHwnds) {
        m_windowOrder.remove(hwnd);
        LOG_DEBUG(QString("Removed invalid window from order map: HWND=%1")
                  .arg(reinterpret_cast<quintptr>(hwnd)));
    }
    
    LOG_INFO("Starting window enumeration...");
    
    BOOL result = EnumWindows(enumWindowsProc, reinterpret_cast<LPARAM>(this));
    if (!result) {
        DWORD error = GetLastError();
        LOG_ERROR(QString("EnumWindows failed with error: %1").arg(error));
    } else {
        LOG_INFO("EnumWindows completed successfully");
    }
    
    LOG_INFO(QString("Total windows found: %1").arg(m_windows.size()));
    
    LOG_INFO("Starting filtering process...");
    // フィルタリングして表示対象のウィンドウのみ抽出
    int filterCount = 0;
    for (const WindowInfo& window : m_windows) {
        filterCount++;
        LOG_INFO(QString("Filtering window %1/%2: %3").arg(filterCount).arg(m_windows.size()).arg(window.title));
        
        try {
            bool isMTGA = window.title.contains("MTGA", Qt::CaseInsensitive);
            bool isBrownDust = window.title.contains("BrownDust", Qt::CaseInsensitive);
            
            if (isMTGA) {
                LOG_INFO(QString("🎯 MTGAフィルター呼び出し開始: %1").arg(window.title));
            }
            
            if (isBrownDust) {
                LOG_INFO(QString("🎯 BrownDustフィルター呼び出し開始: %1").arg(window.title));
            }
            
            bool filterResult = isValidTaskbarWindow(window.hwnd);
            
            if (isMTGA) {
                LOG_INFO(QString("🎯 MTGAフィルター結果: %1").arg(filterResult ? "PASS" : "FAIL"));
            }
            
            if (isBrownDust) {
                LOG_INFO(QString("🎯 BrownDustフィルター結果: %1").arg(filterResult ? "PASS" : "FAIL"));
            }
            
            if (filterResult) {
                m_visibleWindows.append(window);
                LOG_INFO(QString("Window passed filter: %1").arg(window.title));
            } else {
                LOG_INFO(QString("Window filtered out: %1 (visible: %2, hasTitle: %3)")
                         .arg(window.title)
                         .arg(isWindowVisible(window.hwnd) ? "yes" : "no")
                         .arg(hasWindowTitle(window.hwnd) ? "yes" : "no"));
            }
        } catch (...) {
            LOG_ERROR(QString("Exception while filtering window: %1").arg(window.title));
        }
    }
    LOG_INFO("Filtering process completed");
    
    // ウィンドウを固定順序でソート（初回検出順序を維持）
    std::sort(m_visibleWindows.begin(), m_visibleWindows.end(), [this](const WindowInfo& a, const WindowInfo& b) {
        int orderA = m_windowOrder.value(a.hwnd, 999999);  // 未登録は最後
        int orderB = m_windowOrder.value(b.hwnd, 999999);
        return orderA < orderB;
    });
    LOG_INFO("Windows sorted by fixed order");
    
    LOG_INFO(QString("Visible windows after filtering and sorting: %1").arg(m_visibleWindows.size()));
    LOG_INFO("About to emit windowListUpdated signal...");
    emit windowListUpdated();
    LOG_INFO("windowListUpdated signal emitted successfully!");
}

const QVector<WindowInfo>& TaskbarModel::getVisibleWindows() const
{
    return m_visibleWindows;
}

BOOL CALLBACK TaskbarModel::enumWindowsProc(HWND hwnd, LPARAM lParam)
{
    try {
        TaskbarModel* model = reinterpret_cast<TaskbarModel*>(lParam);
        if (model) {
            model->addWindow(hwnd);
        }
        return TRUE;
    } catch (...) {
        LOG_ERROR("Exception in enumWindowsProc");
        return TRUE;
    }
}

void TaskbarModel::addWindow(HWND hwnd)
{
    if (!hwnd || !IsWindow(hwnd)) {
        return;
    }
    
    // ウィンドウの初回検出順序を記録（固定化のため）
    if (!m_windowOrder.contains(hwnd)) {
        m_windowOrder[hwnd] = m_nextOrderIndex++;
        LOG_DEBUG(QString("New window order assigned: HWND=%1, Order=%2")
                  .arg(reinterpret_cast<quintptr>(hwnd)).arg(m_windowOrder[hwnd]));
    }
    
    WindowInfo info;
    info.hwnd = hwnd;
    
    try {
        info.title = getWindowTitle(hwnd);
        
        // 空のタイトルや不要なウィンドウを除外
        if (info.title.isEmpty() || 
            info.title.contains("overlay", Qt::CaseInsensitive) ||
            info.title.contains("dvr", Qt::CaseInsensitive) ||
            info.title == "_q_titlebar" ||
            info.title == "QTreeViewThemeHelperWindow" ||
            info.title.contains("DesktopWindowXamlSource") ||
            info.title.contains("WinEventWindow") ||
            info.title.contains("GDI+ Window") ||
            info.title.contains("Firefox Media Keys") ||
            info.title.contains("Battery Watcher") ||
            info.title.contains(".NET-BroadcastEventWindow") ||
            info.title == "Hidden Window" ||
            info.title == "RemoteApp" ||
            info.title.contains("wslhost.exe") ||
            info.title == "タスクの切り替え" ||
            info.title == "バッテリ メーター" ||
            info.title == "Network Flyout" ||
            info.title == "システム トレイ オーバーフロー ウィンドウ。" ||
            info.title == "Windows 入力エクスペリエンス" ||
            info.title == "進行状況" ||
            info.title.contains("Shell Experience Host", Qt::CaseInsensitive) ||
            info.title.contains("ShellExperienceHost", Qt::CaseInsensitive) ||
            // アイコン抽出で問題を起こすウィンドウを一時的に除外
            info.title.contains("セットアップ") ||
            info.title.contains("Setup") ||
            info.title.contains("InnoSetupLdrWindow") ||
            info.title.contains("Default IME") ||
            info.title.contains("IME") ||
            info.title.contains("入力") ||
            info.title.contains("Input")) {
            LOG_DEBUG(QString("Skipping problematic window: %1").arg(info.title));
            return;
        }
        
        DWORD processId = 0;
        GetWindowThreadProcessId(hwnd, &processId);
        info.processId = processId;
        
        // Shell Experience Hostプロセス名チェック
        if (processId > 0) {
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
            if (hProcess) {
                TCHAR processName[MAX_PATH];
                if (GetModuleBaseName(hProcess, NULL, processName, MAX_PATH)) {
                    QString procName = QString::fromWCharArray(processName);
                    if (procName.contains("ShellExperienceHost", Qt::CaseInsensitive)) {
                        CloseHandle(hProcess);
                        LOG_DEBUG(QString("Skipping Shell Experience Host process: %1").arg(procName));
                        return;
                    }
                }
                CloseHandle(hProcess);
            }
        }
        
        info.isMinimized = IsIconic(hwnd);
        
        // 重い処理を一時的に無効化してテスト
        // info.executablePath = getExecutablePath(hwnd);
        // info.icon = getWindowIcon(hwnd);  // これが最も重い処理
        // info.appUserModelId = getAppUserModelId(hwnd);
        
        // 段階的にアイコン処理を復旧（安全モード）
        info.executablePath = ""; // パス取得は一旦無効化
        info.appUserModelId = ""; // AppUserModelId取得は一旦無効化
        
        // アイコン取得を有効化（強化された例外処理付き）
        try {
            LOG_DEBUG(QString("Starting icon extraction for: %1").arg(info.title));
            info.icon = getWindowIcon(hwnd, false); // アイコン取得を有効化
            if (!info.icon.isNull()) {
                LOG_DEBUG(QString("Window processed (with safe icon): %1").arg(info.title));
            } else {
                LOG_DEBUG(QString("Window processed (no icon): %1").arg(info.title));
            }
            LOG_DEBUG(QString("Icon extraction completed for: %1").arg(info.title));
        } catch (const std::exception& e) {
            LOG_WARNING(QString("Icon extraction exception for %1: %2").arg(info.title).arg(e.what()));
            info.icon = QPixmap(); // 空のアイコン
            LOG_DEBUG(QString("Window processed (icon failed, fallback): %1").arg(info.title));
        } catch (...) {
            LOG_WARNING(QString("Icon extraction failed for: %1 (unknown exception)").arg(info.title));
            info.icon = QPixmap(); // 空のアイコン
            LOG_DEBUG(QString("Window processed (icon failed, fallback): %1").arg(info.title));
        }
        
        m_windows.append(info);
        
        LOG_INFO(QString("Added window: %1 (PID: %2)")
                  .arg(info.title)
                  .arg(info.processId));
                  
    } catch (...) {
        LOG_ERROR(QString("Exception in addWindow for HWND: %1")
                  .arg(reinterpret_cast<quintptr>(hwnd), 0, 16));
    }
}

bool TaskbarModel::isValidTaskbarWindow(HWND hwnd)
{
    wchar_t title[256];
    GetWindowTextW(hwnd, title, sizeof(title) / sizeof(wchar_t));
    QString windowTitle = QString::fromWCharArray(title);
    
    // アプリケーション詳細デバッグ
    bool isVSCode = windowTitle.contains("Visual Studio Code", Qt::CaseInsensitive);
    bool isMTGA = windowTitle.contains("MTGA", Qt::CaseInsensitive);
    bool isBrownDust = windowTitle.contains("BrownDust", Qt::CaseInsensitive);
    
    if (isVSCode) {
        LOG_INFO(QString("🔍 VS Code詳細チェック開始: %1").arg(windowTitle));
    }
    
    if (isMTGA) {
        LOG_INFO(QString("🃏 MTGA詳細チェック開始: %1").arg(windowTitle));
    }
    
    if (isBrownDust) {
        LOG_INFO(QString("🎮 BrownDust詳細チェック開始: %1").arg(windowTitle));
    }
    
    // 基本的な条件チェック
    bool visible = isWindowVisible(hwnd);
    bool hasTitle = hasWindowTitle(hwnd);
    
    if (isMTGA) {
        LOG_INFO(QString("🔍 MTGA基本条件: visible=%1, hasTitle=%2").arg(visible).arg(hasTitle));
    }
    
    if (isBrownDust) {
        LOG_INFO(QString("🔍 BrownDust基本条件: visible=%1, hasTitle=%2").arg(visible).arg(hasTitle));
    }
    if (!visible || !hasTitle) {
        if (isVSCode) {
            LOG_INFO(QString("❌ VS Code基本チェック失敗: visible=%1, hasTitle=%2").arg(visible).arg(hasTitle));
        }
        if (isMTGA) {
            LOG_INFO(QString("❌ MTGA基本チェック失敗: visible=%1, hasTitle=%2").arg(visible).arg(hasTitle));
        }
        if (isBrownDust) {
            LOG_INFO(QString("❌ BrownDust基本チェック失敗: visible=%1, hasTitle=%2").arg(visible).arg(hasTitle));
        }
        return false;
    }
    
    // ツールウィンドウは除外
    bool notToolWindow = isNotToolWindow(hwnd);
    
    if (isMTGA) {
        LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
        LOG_INFO(QString("🔍 MTGAツールウィンドウチェック: notToolWindow=%1, ExStyle=0x%2")
                 .arg(notToolWindow).arg(exStyle, 0, 16));
    }
    
    if (isBrownDust) {
        LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
        LOG_INFO(QString("🔍 BrownDustツールウィンドウチェック: notToolWindow=%1, ExStyle=0x%2")
                 .arg(notToolWindow).arg(exStyle, 0, 16));
    }
    
    if (!notToolWindow) {
        if (isVSCode) {
            LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
            LOG_INFO(QString("❌ VS CodeはツールウィンドウでNGExStyle=0x%1 (WS_EX_TOOLWINDOW=%2)")
                     .arg(exStyle, 0, 16).arg((exStyle & WS_EX_TOOLWINDOW) ? "true" : "false"));
        }
        if (isMTGA) {
            LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
            LOG_INFO(QString("❌ MTGAはツールウィンドウでNG ExStyle=0x%1 (WS_EX_TOOLWINDOW=%2)")
                     .arg(exStyle, 0, 16).arg((exStyle & WS_EX_TOOLWINDOW) ? "true" : "false"));
        }
        if (isBrownDust) {
            LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
            LOG_INFO(QString("❌ BrownDustはツールウィンドウでNG ExStyle=0x%1 (WS_EX_TOOLWINDOW=%2)")
                     .arg(exStyle, 0, 16).arg((exStyle & WS_EX_TOOLWINDOW) ? "true" : "false"));
        }
        return false;
    }
    
    // 親ウィンドウがある場合（子ウィンドウ）は除外
    HWND parent = GetParent(hwnd);
    
    if (isMTGA) {
        LOG_INFO(QString("🔍 MTGA親ウィンドウチェック: parent=0x%1").arg(reinterpret_cast<quintptr>(parent), 0, 16));
    }
    
    if (isBrownDust) {
        LOG_INFO(QString("🔍 BrownDust親ウィンドウチェック: parent=0x%1").arg(reinterpret_cast<quintptr>(parent), 0, 16));
    }
    
    if (parent != nullptr) {
        if (isVSCode) {
            LOG_INFO(QString("❌ VS Codeは子ウィンドウでNG parent=0x%1").arg(reinterpret_cast<quintptr>(parent), 0, 16));
        }
        if (isMTGA) {
            LOG_INFO(QString("❌ MTGAは子ウィンドウでNG parent=0x%1").arg(reinterpret_cast<quintptr>(parent), 0, 16));
        }
        if (isBrownDust) {
            LOG_INFO(QString("❌ BrownDustは子ウィンドウでNG parent=0x%1").arg(reinterpret_cast<quintptr>(parent), 0, 16));
        }
        return false;
    }
    
    // ウィンドウスタイルをチェック
    LONG style = GetWindowLong(hwnd, GWL_STYLE);
    
    // WS_SYSMENUチェックを緩和（VS Code等のモダンアプリは持たない場合がある）
    // 代わりに最小化・最大化ボタンの存在をチェック
    bool hasMinMaxButtons = (style & WS_MINIMIZEBOX) || (style & WS_MAXIMIZEBOX);
    bool hasSysMenu = (style & WS_SYSMENU);
    
    // システムメニューまたは最小化/最大化ボタンのいずれかがあればOK
    // ただし、ゲームアプリケーション（style=-0x6c000000）の場合は例外とする
    bool isGameStyle = (style == -0x6c000000);
    
    if (!hasSysMenu && !hasMinMaxButtons && !isGameStyle) {
        if (isVSCode) {
            LOG_INFO(QString("❌ VS Codeはシステムメニューも最小化/最大化ボタンもなしでNG style=0x%1").arg(style, 0, 16));
        }
        if (isMTGA) {
            LOG_INFO(QString("❌ MTGAはシステムメニューも最小化/最大化ボタンもなしでNG style=0x%1").arg(style, 0, 16));
        }
        if (isBrownDust) {
            LOG_INFO(QString("❌ BrownDustはシステムメニューも最小化/最大化ボタンもなしでNG style=0x%1").arg(style, 0, 16));
        }
        return false;
    }
    
    if (isGameStyle) {
        LOG_INFO(QString("🎮 ゲームアプリケーションスタイル検出 - フィルター通過許可: %1").arg(windowTitle));
    }
    
    if (isVSCode) {
        LOG_INFO(QString("ℹ️ VS Code ウィンドウスタイル: sysMenu=%1, minMax=%2, style=0x%3")
                 .arg(hasSysMenu).arg(hasMinMaxButtons).arg(style, 0, 16));
    }
    
    if (isMTGA) {
        LOG_INFO(QString("ℹ️ MTGA ウィンドウスタイル: sysMenu=%1, minMax=%2, style=0x%3")
                 .arg(hasSysMenu).arg(hasMinMaxButtons).arg(style, 0, 16));
    }
    
    if (isBrownDust) {
        LOG_INFO(QString("ℹ️ BrownDust ウィンドウスタイル: sysMenu=%1, minMax=%2, style=0x%3")
                 .arg(hasSysMenu).arg(hasMinMaxButtons).arg(style, 0, 16));
    }
    
    if (isVSCode) {
        LOG_INFO("✅ VS Code全チェック通過！");
    }
    
    if (isMTGA) {
        LOG_INFO("✅ MTGA全チェック通過！");
    }
    
    if (isBrownDust) {
        LOG_INFO("✅ BrownDust全チェック通過！");
    }
    
    return true;
}

bool TaskbarModel::isWindowVisible(HWND hwnd)
{
    return IsWindowVisible(hwnd);
}

bool TaskbarModel::hasWindowTitle(HWND hwnd)
{
    wchar_t title[256];
    int length = GetWindowTextW(hwnd, title, sizeof(title) / sizeof(wchar_t));
    return length > 0;
}

bool TaskbarModel::isNotToolWindow(HWND hwnd)
{
    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    return !(exStyle & WS_EX_TOOLWINDOW);
}

bool TaskbarModel::activateWindow(HWND hwnd)
{
    if (!hwnd || !IsWindow(hwnd)) {
        LOG_WARNING(QString("⚠️ 無効なウィンドウハンドル: %1").arg(reinterpret_cast<quintptr>(hwnd)));
        return false;
    }
    
    // ウィンドウタイトルを取得して詳細ログ
    wchar_t windowTitle[256];
    GetWindowTextW(hwnd, windowTitle, 256);
    QString title = QString::fromWCharArray(windowTitle);
    
    LOG_INFO(QString("🎯 ウィンドウアクティベート開始: HWND=%1, Title=%2")
             .arg(reinterpret_cast<quintptr>(hwnd)).arg(title));
    
    // 現在のウィンドウ状態を確認
    bool isMinimized = IsIconic(hwnd);
    bool isVisible = IsWindowVisible(hwnd);
    HWND currentForeground = GetForegroundWindow();
    
    LOG_INFO(QString("📊 ウィンドウ状態: 最小化=%1, 可視=%2, 現在のフォアグラウンド=%3")
             .arg(isMinimized ? "はい" : "いいえ")
             .arg(isVisible ? "はい" : "いいえ")
             .arg(reinterpret_cast<quintptr>(currentForeground)));
    
    // 最小化されている場合は復元
    if (isMinimized) {
        LOG_INFO("🔄 最小化ウィンドウを復元中");
        ShowWindow(hwnd, SW_RESTORE);
        Sleep(50); // 復元処理を待つ
    }
    
    // ウィンドウをアクティブ化
    LOG_INFO("🚀 SetForegroundWindow実行中");
    BOOL result = SetForegroundWindow(hwnd);
    
    // 結果確認
    HWND newForeground = GetForegroundWindow();
    bool success = (newForeground == hwnd);
    
    LOG_INFO(QString("✅ アクティベート結果: API戻り値=%1, 新フォアグラウンド=%2, 成功=%3")
             .arg(result ? "成功" : "失敗")
             .arg(reinterpret_cast<quintptr>(newForeground))
             .arg(success ? "はい" : "いいえ"));
    
    if (!success) {
        LOG_WARNING(QString("⚠️ ウィンドウアクティベート失敗: 対象=%1, 実際のフォアグラウンド=%2")
                   .arg(reinterpret_cast<quintptr>(hwnd))
                   .arg(reinterpret_cast<quintptr>(newForeground)));
    }
    
    return success;
}

QPixmap TaskbarModel::getWindowIcon(HWND hwnd, bool safeMode)
{
    // 安全モードの場合は、アイコン取得を完全にスキップ
    if (safeMode) {
        LOG_DEBUG("Safe mode: skipping icon extraction");
        return QPixmap(); // 空のPixmapを返す
    }
    
    // 通常モード：アイコン取得を実行
    LOG_DEBUG("Normal mode: attempting icon extraction");
    
    // まずウィンドウから直接アイコンを取得
    HICON hIcon = reinterpret_cast<HICON>(SendMessage(hwnd, WM_GETICON, ICON_BIG, 0));
    if (!hIcon) {
        hIcon = reinterpret_cast<HICON>(SendMessage(hwnd, WM_GETICON, ICON_SMALL, 0));
    }
    if (!hIcon) {
        hIcon = reinterpret_cast<HICON>(GetClassLongPtr(hwnd, GCLP_HICON));
    }
    
    // ウィンドウから取得できない場合は実行ファイルから取得
    if (!hIcon) {
        QString exePath = getExecutablePath(hwnd);
        if (!exePath.isEmpty()) {
            return extractIcon(exePath);
        }
    }
    
    // HICONからQPixmapに変換（安全な変換）
    if (hIcon) {
        try {
            // HICONが有効かチェック
            ICONINFO iconInfo;
            if (GetIconInfo(hIcon, &iconInfo)) {
                // リソースを解放
                if (iconInfo.hbmColor) DeleteObject(iconInfo.hbmColor);
                if (iconInfo.hbmMask) DeleteObject(iconInfo.hbmMask);
                
                // Qt6のfromHICONは安全でない場合があるため、try-catchで保護
                QImage image = QImage::fromHICON(hIcon);
                if (!image.isNull()) {
                    return QPixmap::fromImage(image);
                }
            }
        } catch (...) {
            // HICONからの変換でエラーが発生した場合はログに記録
            LOG_WARNING("Failed to convert HICON to QPixmap - invalid icon handle");
        }
    }
    
    return QPixmap(); // 空のPixmapを返す
}

QString TaskbarModel::getWindowTitle(HWND hwnd)
{
    wchar_t title[256];
    int length = GetWindowTextW(hwnd, title, sizeof(title) / sizeof(wchar_t));
    if (length > 0) {
        return QString::fromWCharArray(title);
    }
    return QString();
}

QString TaskbarModel::getExecutablePath(HWND hwnd)
{
    DWORD processId;
    GetWindowThreadProcessId(hwnd, &processId);
    
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (!hProcess) {
        return QString();
    }
    
    wchar_t exePath[MAX_PATH];
    DWORD pathLength = MAX_PATH;
    
    if (QueryFullProcessImageNameW(hProcess, 0, exePath, &pathLength)) {
        CloseHandle(hProcess);
        return QString::fromWCharArray(exePath);
    }
    
    CloseHandle(hProcess);
    return QString();
}

QPixmap TaskbarModel::extractIcon(const QString& executablePath)
{
    if (executablePath.isEmpty()) {
        return QPixmap();
    }
    
    SHFILEINFOW fileInfo;
    ZeroMemory(&fileInfo, sizeof(fileInfo));
    
    DWORD_PTR result = SHGetFileInfoW(
        reinterpret_cast<const wchar_t*>(executablePath.utf16()),
        0,
        &fileInfo,
        sizeof(fileInfo),
        SHGFI_ICON | SHGFI_LARGEICON
    );
    
    if (result && fileInfo.hIcon) {
        QPixmap pixmap;
        try {
            // HICONが有効かチェック
            ICONINFO iconInfo;
            if (GetIconInfo(fileInfo.hIcon, &iconInfo)) {
                // リソースを解放
                if (iconInfo.hbmColor) DeleteObject(iconInfo.hbmColor);
                if (iconInfo.hbmMask) DeleteObject(iconInfo.hbmMask);
                
                // 安全な変換
                QImage image = QImage::fromHICON(fileInfo.hIcon);
                if (!image.isNull()) {
                    pixmap = QPixmap::fromImage(image);
                }
            }
        } catch (...) {
            // エラーが発生した場合はログに記録
            LOG_WARNING(QString("Failed to extract icon from: %1").arg(executablePath));
        }
        
        DestroyIcon(fileInfo.hIcon);
        return pixmap;
    }
    
    return QPixmap();
}

QString TaskbarModel::getAppUserModelId(HWND hwnd)
{
    // AppUserModelIDを取得する実装
    // Windowsの場合、プロパティストアから取得することが多いが、
    // シンプルな実装として実行ファイル名を返す
    QString execPath = getExecutablePath(hwnd);
    if (!execPath.isEmpty()) {
        QFileInfo fileInfo(execPath);
        return fileInfo.baseName().toLower();
    }
    return QString();
}