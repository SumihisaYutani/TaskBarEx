#include "TaskbarModel.h"
#include "Logger.h"
#include <QApplication>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
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
            if (isValidTaskbarWindow(window.hwnd)) {
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
    
    LOG_INFO(QString("Visible windows after filtering: %1").arg(m_visibleWindows.size()));
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
            info.title == "進行状況") {
            return;
        }
        
        DWORD processId = 0;
        GetWindowThreadProcessId(hwnd, &processId);
        info.processId = processId;
        
        info.isMinimized = IsIconic(hwnd);
        
        // 重い処理を一時的に無効化してテスト
        // info.executablePath = getExecutablePath(hwnd);
        // info.icon = getWindowIcon(hwnd);  // これが最も重い処理
        // info.appUserModelId = getAppUserModelId(hwnd);
        
        // 軽量な処理のみ実行
        info.executablePath = ""; // 空文字列
        info.icon = QPixmap(); // 空のアイコン
        info.appUserModelId = "";
        
        LOG_DEBUG(QString("Window processed (lightweight mode): %1").arg(info.title));
        
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
    // 基本的な条件チェック
    if (!isWindowVisible(hwnd) || !hasWindowTitle(hwnd)) {
        return false;
    }
    
    // ツールウィンドウは除外
    if (!isNotToolWindow(hwnd)) {
        return false;
    }
    
    // 親ウィンドウがある場合（子ウィンドウ）は除外
    HWND parent = GetParent(hwnd);
    if (parent != nullptr) {
        return false;
    }
    
    // ウィンドウスタイルをチェック
    LONG style = GetWindowLong(hwnd, GWL_STYLE);
    
    // 最小化ボタンやシステムメニューがないウィンドウは除外
    if (!(style & WS_SYSMENU)) {
        return false;
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
        return false;
    }
    
    // 最小化されている場合は復元
    if (IsIconic(hwnd)) {
        ShowWindow(hwnd, SW_RESTORE);
    }
    
    // ウィンドウをアクティブ化
    SetForegroundWindow(hwnd);
    return true;
}

QPixmap TaskbarModel::getWindowIcon(HWND hwnd)
{
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