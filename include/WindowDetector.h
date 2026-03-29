#pragma once

#include "Common.h"
#include "ApplicationInfo.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

/**
 * @brief Windows APIを使用してシステムのウィンドウを検出・監視するクラス
 * 
 * EnumWindowsを使用してタスクバーに表示されるべきウィンドウを列挙し、
 * アプリケーション情報として整理する
 */
class WindowDetector : public QObject
{
    Q_OBJECT
    
public:
    explicit WindowDetector(QObject* parent = nullptr);
    
    // ウィンドウ検出
    QMap<QString, ApplicationInfo> detectApplications();
    QList<HWND> detectVisibleWindows();
    
    // フィルタリング設定
    void setIncludeSystemApps(bool include) { m_includeSystemApps = include; }
    void setIncludeUWPApps(bool include) { m_includeUWPApps = include; }
    void setMinimumWindowSize(const QSize& size) { m_minimumWindowSize = size; }
    
    bool includeSystemApps() const { return m_includeSystemApps; }
    bool includeUWPApps() const { return m_includeUWPApps; }
    QSize minimumWindowSize() const { return m_minimumWindowSize; }
    
    // ウィンドウ情報取得
    static ApplicationInfo getApplicationInfoFromWindow(HWND hwnd);
    static QString getWindowTitle(HWND hwnd);
    static QString getProcessPath(HWND hwnd);
    static DWORD getProcessId(HWND hwnd);
    static WindowState getWindowState(HWND hwnd);
    static QRect getWindowRect(HWND hwnd);
    
    // 検証メソッド
    static bool isValidTaskbarWindow(HWND hwnd);
    static bool isSystemWindow(HWND hwnd);
    static bool isToolWindow(HWND hwnd);
    static bool hasParentWindow(HWND hwnd);
    
signals:
    void applicationsDetected(const QMap<QString, ApplicationInfo>& applications);
    void windowAdded(HWND hwnd);
    void windowRemoved(HWND hwnd);
    void windowTitleChanged(HWND hwnd, const QString& newTitle);
    
public slots:
    void startMonitoring();
    void stopMonitoring();
    void refreshApplications();
    
private slots:
    void onDetectionTimer();
    
private:
    QTimer* m_detectionTimer;
    QMap<QString, ApplicationInfo> m_lastApplications;
    QSet<HWND> m_lastWindows;
    
    // 設定
    bool m_includeSystemApps;
    bool m_includeUWPApps;
    QSize m_minimumWindowSize;
    
    // 内部メソッド
    void compareApplications(const QMap<QString, ApplicationInfo>& oldApps,
                           const QMap<QString, ApplicationInfo>& newApps);
    void compareWindows(const QSet<HWND>& oldWindows, 
                       const QSet<HWND>& newWindows);
    
    // Windows API コールバック
    #ifdef Q_OS_WIN
    static BOOL CALLBACK enumWindowsProc(HWND hwnd, LPARAM lParam);
    #endif
    
    struct EnumData {
        WindowDetector* detector;
        QMap<QString, ApplicationInfo>* applications;
    };
};

/**
 * @brief ウィンドウ監視用のユーティリティクラス
 */
class WindowMonitor : public QObject
{
    Q_OBJECT
    
public:
    explicit WindowMonitor(QObject* parent = nullptr);
    
    // イベントフック管理
    void installHooks();
    void removeHooks();
    
    bool isHooksInstalled() const { return m_hooksInstalled; }
    
signals:
    void windowCreated(HWND hwnd);
    void windowDestroyed(HWND hwnd);
    void windowShown(HWND hwnd);
    void windowHidden(HWND hwnd);
    void windowMoved(HWND hwnd);
    void windowTitleChanged(HWND hwnd);
    
private:
    bool m_hooksInstalled;
    
    #ifdef Q_OS_WIN
    HWINEVENTHOOK m_createHook;
    HWINEVENTHOOK m_destroyHook;
    HWINEVENTHOOK m_showHook;
    HWINEVENTHOOK m_hideHook;
    HWINEVENTHOOK m_moveHook;
    HWINEVENTHOOK m_nameChangeHook;
    
    // Windows イベントコールバック
    static void CALLBACK winEventProc(HWINEVENTHOOK hWinEventHook,
                                    DWORD event,
                                    HWND hwnd,
                                    LONG idObject,
                                    LONG idChild,
                                    DWORD dwEventThread,
                                    DWORD dwmsEventTime);
    
    static WindowMonitor* s_instance;
    #endif
    
    void processWindowEvent(DWORD event, HWND hwnd);
};