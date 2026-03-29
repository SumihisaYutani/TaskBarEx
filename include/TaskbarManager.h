#pragma once

#include "Common.h"
#include "ApplicationInfo.h"
#include "WindowDetector.h"
#include "WindowInfoCache.h"
#include "IconExtractor.h"

/**
 * @brief タスクバー管理の中核クラス
 * 
 * WindowDetectorからの情報を受け取り、アプリケーショングループ化を管理し、
 * UIコンポーネントに適切な形でデータを提供する
 */
class TaskbarManager : public QObject
{
    Q_OBJECT
    
public:
    explicit TaskbarManager(QObject* parent = nullptr);
    ~TaskbarManager();
    
    // 基本操作
    void initialize();
    void start();
    void stop();
    bool isRunning() const { return m_isRunning; }
    
    // アプリケーション情報
    const QMap<QString, ApplicationInfo>& applications() const { return m_applications; }
    ApplicationInfo getApplication(const QString& groupId) const;
    QStringList getApplicationGroupIds() const;
    
    // グループ化設定
    void setGroupingMode(GroupingMode mode);
    GroupingMode groupingMode() const { return m_groupingMode; }
    
    // 更新間隔設定
    void setUpdateInterval(int milliseconds);
    int updateInterval() const;
    
    // フィルタリング設定
    void setIncludeSystemApps(bool include);
    void setIncludeUWPApps(bool include);
    bool includeSystemApps() const;
    bool includeUWPApps() const;
    
    // ウィンドウ操作
    void activateWindow(HWND hwnd);
    void minimizeWindow(HWND hwnd);
    void closeWindow(HWND hwnd);
    void activateApplication(const QString& groupId);
    
    // 統計情報
    int totalApplications() const { return m_applications.size(); }
    int totalWindows() const;
    
signals:
    void applicationsChanged();
    void applicationAdded(const QString& groupId);
    void applicationRemoved(const QString& groupId);
    void applicationUpdated(const QString& groupId);
    void windowActivated(HWND hwnd);
    void error(const QString& message);
    
public slots:
    void refresh();
    void forceRefresh();
    
private slots:
    void onApplicationsDetected(const QMap<QString, ApplicationInfo>& applications);
    void onWindowAdded(HWND hwnd);
    void onWindowRemoved(HWND hwnd);
    void onWindowTitleChanged(HWND hwnd, const QString& newTitle);
    
private:
    // コンポーネント
    WindowDetector* m_windowDetector;
    WindowInfoCache* m_windowCache;
    IconExtractor* m_iconExtractor;
    
    // データ
    QMap<QString, ApplicationInfo> m_applications;
    
    // 設定
    GroupingMode m_groupingMode;
    bool m_isRunning;
    
    // 内部メソッド
    void processApplicationChanges(const QMap<QString, ApplicationInfo>& newApplications);
    void updateApplicationIcons();
    QString generateGroupId(const ApplicationInfo& appInfo) const;
    ApplicationInfo mergeApplicationInfo(const ApplicationInfo& existing, 
                                       const ApplicationInfo& update) const;
    
    // グループ化ロジック
    QMap<QString, ApplicationInfo> applyGrouping(const QMap<QString, ApplicationInfo>& apps) const;
    bool shouldGroup(const ApplicationInfo& app1, const ApplicationInfo& app2) const;
    ApplicationInfo combineApplications(const ApplicationInfo& app1, 
                                      const ApplicationInfo& app2) const;
};

/**
 * @brief タスクバー設定を管理するクラス
 */
class TaskbarSettings : public QObject
{
    Q_OBJECT
    
public:
    explicit TaskbarSettings(QObject* parent = nullptr);
    
    // 設定の読み書き
    void loadSettings();
    void saveSettings();
    void resetToDefaults();
    
    // グループ化設定
    GroupingMode groupingMode() const { return m_groupingMode; }
    void setGroupingMode(GroupingMode mode);
    
    // 更新間隔
    int updateInterval() const { return m_updateInterval; }
    void setUpdateInterval(int milliseconds);
    
    // フィルタリング
    bool includeSystemApps() const { return m_includeSystemApps; }
    void setIncludeSystemApps(bool include);
    
    bool includeUWPApps() const { return m_includeUWPApps; }
    void setIncludeUWPApps(bool include);
    
    // UI設定
    int iconSize() const { return m_iconSize; }
    void setIconSize(int size);
    
    bool showWindowTitles() const { return m_showWindowTitles; }
    void setShowWindowTitles(bool show);
    
    bool autoHide() const { return m_autoHide; }
    void setAutoHide(bool hide);
    
    // ウィンドウ設定
    QPoint windowPosition() const { return m_windowPosition; }
    void setWindowPosition(const QPoint& position);
    
    QSize windowSize() const { return m_windowSize; }
    void setWindowSize(const QSize& size);
    
    bool stayOnTop() const { return m_stayOnTop; }
    void setStayOnTop(bool onTop);
    
signals:
    void settingsChanged();
    void groupingModeChanged(GroupingMode mode);
    void updateIntervalChanged(int milliseconds);
    void filteringChanged();
    void uiSettingsChanged();
    
private:
    QSettings* m_settings;
    
    // 設定値
    GroupingMode m_groupingMode;
    int m_updateInterval;
    bool m_includeSystemApps;
    bool m_includeUWPApps;
    int m_iconSize;
    bool m_showWindowTitles;
    bool m_autoHide;
    QPoint m_windowPosition;
    QSize m_windowSize;
    bool m_stayOnTop;
    
    // 定数
    static const QString GROUP_GENERAL;
    static const QString GROUP_FILTERING;
    static const QString GROUP_UI;
    static const QString GROUP_WINDOW;
};