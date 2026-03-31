#include "WindowThumbnailManager.h"
#include <QApplication>
#include <QScreen>
#include <QDateTime>
#include <QDebug>
#include <QBitmap>
#include <QPainter>
#include <QWidget>

// Windows API ヘッダー
#include <dwmapi.h>
#include <wingdi.h>

// DWM ライブラリリンク（CMakeのtarget_link_librariesで設定済み）
// #pragma comment(lib, "dwmapi.lib")  // MinGWでは無効なため削除

void WindowThumbnailManager::logInfo(const QString &message)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    QString logMessage = QString("[%1] %2").arg(timestamp, message);
    qDebug() << logMessage;
}

WindowThumbnailManager::WindowThumbnailManager(QObject *parent)
    : QObject(parent)
    , m_cacheCleanupTimer(new QTimer(this))
    , m_cacheEnabled(true)
    , m_cacheTimeout(30000)  // 30秒キャッシュ
    , m_thumbnailQuality(95) // 95%品質（高画質）
{
    logInfo("🖼️ WindowThumbnailManager初期化開始");
    
    // キャッシュクリーンアップタイマー設定
    m_cacheCleanupTimer->setInterval(60000); // 1分間隔
    m_cacheCleanupTimer->setSingleShot(false);
    connect(m_cacheCleanupTimer, &QTimer::timeout, this, &WindowThumbnailManager::onCacheCleanupTimer);
    m_cacheCleanupTimer->start();
    
    logInfo("✅ WindowThumbnailManager初期化完了");
}

WindowThumbnailManager::~WindowThumbnailManager()
{
    clearCache();
    logInfo("🗑️ WindowThumbnailManager破棄完了");
}

QPixmap WindowThumbnailManager::captureWindowThumbnail(HWND hwnd, const QSize &targetSize)
{
    if (!hwnd || !isWindowValidForCapture(hwnd)) {
        logInfo(QString("❌ 無効なウィンドウハンドル: %1").arg(reinterpret_cast<quintptr>(hwnd)));
        return QPixmap();
    }
    
    // キャッシュ確認
    if (m_cacheEnabled && m_cache.contains(hwnd)) {
        const ThumbnailCache &cache = m_cache[hwnd];
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
        if ((currentTime - cache.timestamp) < m_cacheTimeout && cache.size == targetSize) {
            logInfo(QString("📋 キャッシュからサムネイル取得: HWND=%1").arg(reinterpret_cast<quintptr>(hwnd)));
            return cache.thumbnail;
        }
    }
    
    logInfo(QString("🖼️ ウィンドウサムネイル取得開始: HWND=%1").arg(reinterpret_cast<quintptr>(hwnd)));
    
    QPixmap thumbnail;
    
    // 複数の方法を試行（優先度順）
    
    // 1. DWM API を試行（Windows Vista以降）
    thumbnail = captureWindowDWM(hwnd, targetSize);
    if (!thumbnail.isNull()) {
        logInfo("✅ DWM APIでサムネイル取得成功");
    } else {
        logInfo("⚠️ DWM API失敗、PrintWindowを試行");
        
        // 2. PrintWindow API を試行
        thumbnail = captureWindowPrintWindow(hwnd, targetSize);
        if (!thumbnail.isNull()) {
            logInfo("✅ PrintWindowでサムネイル取得成功");
        } else {
            logInfo("⚠️ PrintWindow失敗、BitBltを試行");
            
            // 3. BitBlt を試行（最後の手段）
            thumbnail = captureWindowBitBlt(hwnd, targetSize);
            if (!thumbnail.isNull()) {
                logInfo("✅ BitBltでサムネイル取得成功");
            } else {
                logInfo("❌ 全ての手段でサムネイル取得失敗");
                emit thumbnailFailed(hwnd, "All capture methods failed");
                return QPixmap();
            }
        }
    }
    
    // キャッシュに保存
    if (m_cacheEnabled && !thumbnail.isNull()) {
        ThumbnailCache cache;
        cache.thumbnail = thumbnail;
        cache.timestamp = QDateTime::currentMSecsSinceEpoch();
        cache.size = targetSize;
        m_cache[hwnd] = cache;
        logInfo(QString("💾 サムネイルをキャッシュに保存: HWND=%1").arg(reinterpret_cast<quintptr>(hwnd)));
    }
    
    if (!thumbnail.isNull()) {
        emit thumbnailReady(hwnd, thumbnail);
    }
    
    return thumbnail;
}

QPixmap WindowThumbnailManager::captureWindowDWM(HWND hwnd, const QSize &targetSize)
{
    // DWM Thumbnail APIを使用
    // 注意: この実装は簡略化されており、実際のDWM thumbnail登録は複雑
    
    logInfo("🔍 DWM APIサムネイル取得を試行");
    
    // DWMが有効かチェック
    BOOL dwmEnabled = FALSE;
    HRESULT hr = DwmIsCompositionEnabled(&dwmEnabled);
    if (FAILED(hr) || !dwmEnabled) {
        logInfo("❌ DWMが無効または利用不可");
        return QPixmap();
    }
    
    // PrintWindowにフォールバック（DWM直接アクセスは複雑なため）
    return captureWindowPrintWindow(hwnd, targetSize);
}

QPixmap WindowThumbnailManager::captureWindowPrintWindow(HWND hwnd, const QSize &targetSize)
{
    logInfo("🖨️ PrintWindow APIサムネイル取得を試行");
    
    // ウィンドウサイズ取得
    RECT windowRect;
    if (!GetWindowRect(hwnd, &windowRect)) {
        logInfo("❌ ウィンドウサイズ取得失敗");
        return QPixmap();
    }
    
    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;
    
    if (windowWidth <= 0 || windowHeight <= 0) {
        logInfo("❌ 無効なウィンドウサイズ");
        return QPixmap();
    }
    
    // デバイスコンテキスト作成
    HDC windowDC = GetWindowDC(hwnd);
    HDC memDC = CreateCompatibleDC(windowDC);
    
    if (!windowDC || !memDC) {
        logInfo("❌ デバイスコンテキスト作成失敗");
        if (memDC) DeleteDC(memDC);
        if (windowDC) ReleaseDC(hwnd, windowDC);
        return QPixmap();
    }
    
    // ビットマップ作成
    HBITMAP bitmap = CreateCompatibleBitmap(windowDC, windowWidth, windowHeight);
    if (!bitmap) {
        logInfo("❌ ビットマップ作成失敗");
        DeleteDC(memDC);
        ReleaseDC(hwnd, windowDC);
        return QPixmap();
    }
    
    // ビットマップ選択
    HGDIOBJ oldBitmap = SelectObject(memDC, bitmap);
    
    // PrintWindow でウィンドウをビットマップにコピー
    BOOL result = PrintWindow(hwnd, memDC, PW_CLIENTONLY | PW_RENDERFULLCONTENT);
    
    QPixmap pixmap;
    
    if (result) {
        // ビットマップをQPixmapに変換（Qt6対応）
        pixmap = convertHBitmapToQPixmap(bitmap);
        
        if (!pixmap.isNull()) {
            // ターゲットサイズにスケール
            pixmap = scaleAndCleanThumbnail(pixmap, targetSize);
            logInfo(QString("✅ PrintWindow成功: %1x%2 -> %3x%4")
                   .arg(windowWidth).arg(windowHeight)
                   .arg(pixmap.width()).arg(pixmap.height()));
        }
    } else {
        logInfo("❌ PrintWindow失敗");
    }
    
    // クリーンアップ
    SelectObject(memDC, oldBitmap);
    DeleteObject(bitmap);
    DeleteDC(memDC);
    ReleaseDC(hwnd, windowDC);
    
    return pixmap;
}

QPixmap WindowThumbnailManager::captureWindowBitBlt(HWND hwnd, const QSize &targetSize)
{
    logInfo("🎯 BitBlt APIサムネイル取得を試行");
    
    // ウィンドウサイズ取得
    RECT windowRect;
    if (!GetWindowRect(hwnd, &windowRect)) {
        return QPixmap();
    }
    
    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;
    
    // スクリーンキャプチャでウィンドウ領域を取得
    QScreen *screen = QApplication::primaryScreen();
    if (!screen) {
        logInfo("❌ プライマリスクリーン取得失敗");
        return QPixmap();
    }
    
    QPixmap screenshot = screen->grabWindow(0, windowRect.left, windowRect.top, 
                                           windowWidth, windowHeight);
    
    if (screenshot.isNull()) {
        logInfo("❌ スクリーンキャプチャ失敗");
        return QPixmap();
    }
    
    // スケールして返す
    QPixmap scaled = scaleAndCleanThumbnail(screenshot, targetSize);
    logInfo(QString("✅ BitBlt成功: %1x%2 -> %3x%4")
           .arg(windowWidth).arg(windowHeight)
           .arg(scaled.width()).arg(scaled.height()));
    
    return scaled;
}

QList<QPixmap> WindowThumbnailManager::captureGroupThumbnails(const QList<WindowInfo> &windows, const QSize &targetSize)
{
    QList<QPixmap> thumbnails;
    
    logInfo(QString("📸 グループサムネイル取得開始: %1個のウィンドウ").arg(windows.size()));
    
    for (const WindowInfo &window : windows) {
        QPixmap thumbnail = captureWindowThumbnail(window.hwnd, targetSize);
        thumbnails.append(thumbnail);
    }
    
    logInfo(QString("✅ グループサムネイル取得完了: %1個成功").arg(thumbnails.size()));
    
    return thumbnails;
}

bool WindowThumbnailManager::isWindowValidForCapture(HWND hwnd)
{
    if (!hwnd || !IsWindow(hwnd)) {
        return false;
    }
    
    // 非表示ウィンドウは除外
    if (!IsWindowVisible(hwnd)) {
        return false;
    }
    
    // 最小化ウィンドウも取得可能とする（サムネイル目的）
    
    return true;
}

QSize WindowThumbnailManager::getOptimalThumbnailSize(HWND hwnd, const QSize &targetSize)
{
    RECT windowRect;
    if (!GetWindowRect(hwnd, &windowRect)) {
        return targetSize;
    }
    
    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;
    
    // アスペクト比を保持してサイズ計算
    double aspectRatio = static_cast<double>(windowWidth) / windowHeight;
    
    QSize optimalSize = targetSize;
    
    if (aspectRatio > 1.0) {
        // 横長
        optimalSize.setHeight(static_cast<int>(targetSize.width() / aspectRatio));
    } else {
        // 縦長
        optimalSize.setWidth(static_cast<int>(targetSize.height() * aspectRatio));
    }
    
    return optimalSize;
}

QPixmap WindowThumbnailManager::scaleAndCleanThumbnail(const QPixmap &source, const QSize &targetSize)
{
    if (source.isNull()) {
        return QPixmap();
    }
    
    // 高品質スケーリング処理
    QPixmap scaled;
    
    // 元画像がターゲットサイズより大きい場合は高品質にスケールダウン
    if (source.width() > targetSize.width() || source.height() > targetSize.height()) {
        // マルチステップスケーリングで品質向上
        QPixmap intermediate = source;
        QSize currentSize = source.size();
        
        // 段階的にスケールダウン（50%ずつ）
        while (currentSize.width() > targetSize.width() * 2 || currentSize.height() > targetSize.height() * 2) {
            currentSize = QSize(currentSize.width() / 2, currentSize.height() / 2);
            intermediate = intermediate.scaled(currentSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        
        // 最終的にターゲットサイズにスケール
        scaled = intermediate.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    } else {
        // 拡大の場合は通常のスケーリング
        scaled = source.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    
    return scaled;
}

QPixmap WindowThumbnailManager::convertHBitmapToQPixmap(HBITMAP hBitmap)
{
    if (!hBitmap) {
        return QPixmap();
    }
    
    // ビットマップ情報取得
    BITMAP bitmap;
    if (!GetObject(hBitmap, sizeof(BITMAP), &bitmap)) {
        logInfo("❌ ビットマップ情報取得失敗");
        return QPixmap();
    }
    
    // デバイスコンテキスト作成
    HDC hdc = GetDC(nullptr);
    HDC memDC = CreateCompatibleDC(hdc);
    
    if (!memDC) {
        ReleaseDC(nullptr, hdc);
        return QPixmap();
    }
    
    // BITMAPINFO 構造体設定
    BITMAPINFO bmi;
    memset(&bmi, 0, sizeof(BITMAPINFO));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = bitmap.bmWidth;
    bmi.bmiHeader.biHeight = -bitmap.bmHeight; // 上下反転防止
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    
    // ピクセルデータ取得用バッファ
    void* pixelData = nullptr;
    int dataSize = bitmap.bmWidth * bitmap.bmHeight * 4;
    pixelData = malloc(dataSize);
    
    if (!pixelData) {
        DeleteDC(memDC);
        ReleaseDC(nullptr, hdc);
        return QPixmap();
    }
    
    // ビットマップデータ取得
    int result = GetDIBits(memDC, hBitmap, 0, bitmap.bmHeight, pixelData, &bmi, DIB_RGB_COLORS);
    
    QPixmap pixmap;
    if (result) {
        // QImageを経由してQPixmapに変換
        QImage image(static_cast<uchar*>(pixelData), bitmap.bmWidth, bitmap.bmHeight, 
                    bitmap.bmWidth * 4, QImage::Format_ARGB32);
        
        // BGRAからRGBAに変換（Windows は BGRA フォーマット）
        image = image.rgbSwapped();
        
        pixmap = QPixmap::fromImage(image);
        logInfo(QString("✅ HBITMAP→QPixmap変換成功: %1x%2").arg(bitmap.bmWidth).arg(bitmap.bmHeight));
    } else {
        logInfo("❌ GetDIBits失敗");
    }
    
    // クリーンアップ
    free(pixelData);
    DeleteDC(memDC);
    ReleaseDC(nullptr, hdc);
    
    return pixmap;
}

void WindowThumbnailManager::setThumbnailQuality(int quality)
{
    m_thumbnailQuality = qBound(1, quality, 100);
    logInfo(QString("🎨 サムネイル品質設定: %1%").arg(m_thumbnailQuality));
}

void WindowThumbnailManager::clearCache()
{
    m_cache.clear();
    logInfo("🗑️ サムネイルキャッシュクリア完了");
}

void WindowThumbnailManager::setCacheEnabled(bool enabled)
{
    m_cacheEnabled = enabled;
    if (!enabled) {
        clearCache();
    }
    logInfo(QString("💾 キャッシュ有効状態: %1").arg(enabled ? "有効" : "無効"));
}

void WindowThumbnailManager::onCacheCleanupTimer()
{
    if (!m_cacheEnabled) {
        return;
    }
    
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    QList<HWND> expiredKeys;
    
    // 期限切れキャッシュを検索
    for (auto it = m_cache.constBegin(); it != m_cache.constEnd(); ++it) {
        if ((currentTime - it.value().timestamp) >= m_cacheTimeout) {
            expiredKeys.append(it.key());
        }
    }
    
    // 期限切れキャッシュを削除
    for (HWND hwnd : expiredKeys) {
        m_cache.remove(hwnd);
    }
    
    if (!expiredKeys.isEmpty()) {
        logInfo(QString("🧹 期限切れキャッシュ削除: %1個").arg(expiredKeys.size()));
    }
}