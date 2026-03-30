#ifndef PINNEDAPPSMANAGER_H
#define PINNEDAPPSMANAGER_H

#include <QObject>
#include <QStringList>
#include <QPixmap>
#include <QTimer>
#include <Windows.h>

struct PinnedAppInfo {
    QString name;          // アプリケーション名
    QString executablePath; // 実行可能ファイルパス
    QPixmap icon;          // アイコン
    int order;             // タスクバーでの順序
    bool isValid;          // 有効なピン留めかどうか
    
    PinnedAppInfo() : order(-1), isValid(false) {}
};

class PinnedAppsManager : public QObject
{
    Q_OBJECT

public:
    explicit PinnedAppsManager(QObject *parent = nullptr);
    
    // ピン留めアプリ一覧の取得
    QList<PinnedAppInfo> getPinnedApps() const;
    
    // ピン留めアプリの更新
    void refreshPinnedApps();
    
    // 自動更新の開始・停止
    void startAutoRefresh(int intervalMs = 5000); // 5秒間隔
    void stopAutoRefresh();
    
    // 特定アプリがピン留めされているかチェック
    bool isAppPinned(const QString &executablePath) const;

signals:
    void pinnedAppsChanged();

private slots:
    void onRefreshTimer();

private:
    // Windows API を使ったピン留めアプリ検出
    void detectPinnedAppsFromRegistry();
    void detectPinnedAppsFromShortcuts();
    
    // アイコン取得
    QPixmap extractIconFromPath(const QString &path);
    
    // ユーティリティ関数
    QString resolveShortcutTarget(const QString &shortcutPath);
    bool isValidExecutable(const QString &path);
    
    QList<PinnedAppInfo> m_pinnedApps;
    QTimer *m_refreshTimer;
    
    // ログ出力用（プライベート関数名を変更してマクロ競合を回避）
    void logInfo(const QString &message);
};

#endif // PINNEDAPPSMANAGER_H