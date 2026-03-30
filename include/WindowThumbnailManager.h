#ifndef WINDOWTHUMBNAILMANAGER_H
#define WINDOWTHUMBNAILMANAGER_H

#include <QObject>
#include <QPixmap>
#include <QTimer>
#include <QHash>
#include <Windows.h>
#include <dwmapi.h>
#include "WindowInfo.h"

class WindowThumbnailManager : public QObject
{
    Q_OBJECT

public:
    explicit WindowThumbnailManager(QObject *parent = nullptr);
    ~WindowThumbnailManager();

    // ウィンドウサムネイル取得
    QPixmap captureWindowThumbnail(HWND hwnd, const QSize &targetSize = QSize(200, 150));
    
    // 複数ウィンドウのサムネイル取得（同一グループ用）
    QList<QPixmap> captureGroupThumbnails(const QList<WindowInfo> &windows, const QSize &targetSize = QSize(160, 120));
    
    // サムネイル品質設定
    void setThumbnailQuality(int quality); // 1-100
    
    // キャッシュ制御
    void clearCache();
    void setCacheEnabled(bool enabled);

signals:
    void thumbnailReady(HWND hwnd, const QPixmap &thumbnail);
    void thumbnailFailed(HWND hwnd, const QString &error);

private slots:
    void onCacheCleanupTimer();

private:
    // Windows API サムネイル取得
    QPixmap captureWindowDWM(HWND hwnd, const QSize &targetSize);
    QPixmap captureWindowPrintWindow(HWND hwnd, const QSize &targetSize);
    QPixmap captureWindowBitBlt(HWND hwnd, const QSize &targetSize);
    
    // ヘルパー関数
    bool isWindowValidForCapture(HWND hwnd);
    QSize getOptimalThumbnailSize(HWND hwnd, const QSize &targetSize);
    QPixmap scaleAndCleanThumbnail(const QPixmap &source, const QSize &targetSize);
    QPixmap convertHBitmapToQPixmap(HBITMAP hBitmap);  // Qt6対応HBITMAP変換
    
    // キャッシュ管理
    struct ThumbnailCache {
        QPixmap thumbnail;
        qint64 timestamp;
        QSize size;
    };
    QHash<HWND, ThumbnailCache> m_cache;
    QTimer *m_cacheCleanupTimer;
    bool m_cacheEnabled;
    int m_cacheTimeout; // ミリ秒
    int m_thumbnailQuality;
    
    // ログ出力ヘルパー
    void logInfo(const QString &message);
};

#endif // WINDOWTHUMBNAILMANAGER_H