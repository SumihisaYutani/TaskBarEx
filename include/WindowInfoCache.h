#pragma once

#include "Common.h"
#include "ApplicationInfo.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

/**
 * @brief ウィンドウ情報を効率的にキャッシュするクラス
 * 
 * 頻繁なWindows API呼び出しを避けるため、
 * ウィンドウ情報をキャッシュし、変更時のみ更新する
 */
class WindowInfoCache : public QObject
{
    Q_OBJECT
    
public:
    struct WindowInfo {
        HWND hwnd = nullptr;
        QString title;
        QString executablePath;
        QString processName;
        DWORD processId = 0;
        WindowState state = WindowState::Normal;
        QRect geometry;
        QPixmap icon;
        QString appUserModelId;
        qint64 lastUpdate = 0;
        bool isValid = true;
        
        bool operator==(const WindowInfo& other) const {
            return hwnd == other.hwnd &&
                   title == other.title &&
                   executablePath == other.executablePath &&
                   state == other.state &&
                   geometry == other.geometry;
        }
        
        bool operator!=(const WindowInfo& other) const {
            return !(*this == other);
        }
    };
    
    explicit WindowInfoCache(QObject* parent = nullptr);
    ~WindowInfoCache();
    
    // キャッシュ操作
    void addWindow(HWND hwnd);
    void removeWindow(HWND hwnd);
    void updateWindow(HWND hwnd, bool force = false);
    void clearCache();
    
    // 情報取得
    WindowInfo getWindowInfo(HWND hwnd) const;
    bool hasWindow(HWND hwnd) const;
    QList<HWND> getAllWindows() const;
    QList<WindowInfo> getAllWindowInfo() const;
    
    // 検索・フィルタリング
    QList<WindowInfo> getWindowsByProcess(DWORD processId) const;
    QList<WindowInfo> getWindowsByExecutable(const QString& path) const;
    QList<WindowInfo> getVisibleWindows() const;
    WindowInfo findWindowByTitle(const QString& title) const;
    
    // 統計
    int windowCount() const { return m_cache.size(); }
    int validWindowCount() const;
    qint64 totalMemoryUsage() const;
    
    // 設定
    void setCacheTimeout(int seconds) { m_cacheTimeout = seconds; }
    int cacheTimeout() const { return m_cacheTimeout; }
    
    void setMaxCacheSize(int size) { m_maxCacheSize = size; }
    int maxCacheSize() const { return m_maxCacheSize; }
    
signals:
    void windowAdded(HWND hwnd);
    void windowRemoved(HWND hwnd);
    void windowUpdated(HWND hwnd);
    void cacheCleared();
    
public slots:
    void refreshCache();
    void cleanupExpiredEntries();
    void validateAllEntries();
    
private slots:
    void onCleanupTimer();
    
private:
    QMap<HWND, WindowInfo> m_cache;
    QTimer* m_cleanupTimer;
    
    // 設定
    int m_cacheTimeout;      // キャッシュの有効期限（秒）
    int m_maxCacheSize;      // 最大キャッシュサイズ
    
    // 内部メソッド
    WindowInfo createWindowInfo(HWND hwnd) const;
    bool isWindowInfoValid(const WindowInfo& info) const;
    void removeOldestEntries();
    qint64 getCurrentTimestamp() const;
    
    // Windows API ラッパー
    static QString getWindowTitleSafe(HWND hwnd);
    static QString getProcessPathSafe(HWND hwnd);
    static QString getProcessNameSafe(HWND hwnd);
    static DWORD getProcessIdSafe(HWND hwnd);
    static WindowState getWindowStateSafe(HWND hwnd);
    static QRect getWindowRectSafe(HWND hwnd);
    static QString getAppUserModelIdSafe(HWND hwnd);
};

/**
 * @brief プロセス情報を管理するキャッシュクラス
 */
class ProcessInfoCache : public QObject
{
    Q_OBJECT
    
public:
    struct ProcessInfo {
        DWORD processId = 0;
        QString executablePath;
        QString processName;
        QString appUserModelId;
        QPixmap icon;
        qint64 lastUpdate = 0;
        bool isValid = true;
        
        bool operator==(const ProcessInfo& other) const {
            return processId == other.processId &&
                   executablePath == other.executablePath &&
                   processName == other.processName;
        }
    };
    
    explicit ProcessInfoCache(QObject* parent = nullptr);
    
    // キャッシュ操作
    ProcessInfo getProcessInfo(DWORD processId);
    void invalidateProcess(DWORD processId);
    void clearCache();
    
    // 統計
    int processCount() const { return m_cache.size(); }
    
private:
    QMap<DWORD, ProcessInfo> m_cache;
    
    ProcessInfo createProcessInfo(DWORD processId) const;
    bool isProcessValid(DWORD processId) const;
};

/**
 * @brief アイコンキャッシュ管理クラス
 */
class IconCache : public QObject
{
    Q_OBJECT
    
public:
    explicit IconCache(QObject* parent = nullptr);
    
    // アイコン取得
    QPixmap getIcon(const QString& executablePath, int size = 0);
    QPixmap getWindowIcon(HWND hwnd, int size = 0);
    QPixmap getSystemIcon(const QString& iconName, int size = 0);
    
    // キャッシュ管理
    void clearCache();
    void removeIcon(const QString& key);
    
    // 統計
    int iconCount() const { return m_iconCache.size(); }
    qint64 memoryUsage() const;
    
    // 設定
    void setMaxCacheSize(int count) { m_maxIcons = count; }
    int maxCacheSize() const { return m_maxIcons; }
    
private:
    QMap<QString, QPixmap> m_iconCache;
    QMap<QString, qint64> m_lastAccess;
    int m_maxIcons;
    
    QString generateCacheKey(const QString& path, int size) const;
    void removeOldestIcon();
    QPixmap extractIconFromFile(const QString& path, int size) const;
    QPixmap extractIconFromWindow(HWND hwnd, int size) const;
};