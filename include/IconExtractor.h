#pragma once

#include "Common.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#include <commoncontrols.h>
#endif

/**
 * @brief Windows APIを使用してアプリケーションアイコンを抽出するクラス
 * 
 * 複数の方法でアイコンを取得し、DPI対応とキャッシング機能を提供
 */
class IconExtractor : public QObject
{
    Q_OBJECT
    
public:
    enum class IconSize {
        Small = 16,
        Medium = 32,
        Large = 48,
        ExtraLarge = 256,
        Auto = 0  // DPI自動調整
    };
    
    enum class ExtractionMethod {
        SHGetFileInfo,      // 推奨：ファイルアイコン取得
        ExtractIcon,        // 実行ファイルから直接抽出
        WindowIcon,         // ウィンドウアイコン取得
        SystemImageList,    // システムイメージリスト
        Auto               // 自動選択
    };
    
    explicit IconExtractor(QObject* parent = nullptr);
    ~IconExtractor();
    
    // メインのアイコン取得メソッド
    QPixmap extractIcon(const QString& executablePath, 
                       IconSize size = IconSize::Auto,
                       ExtractionMethod method = ExtractionMethod::Auto);
    
    QPixmap extractWindowIcon(HWND hwnd, 
                             IconSize size = IconSize::Auto);
    
    QPixmap extractSystemIcon(const QString& iconName, 
                             IconSize size = IconSize::Auto);
    
    // 一括抽出
    QMap<IconSize, QPixmap> extractAllSizes(const QString& executablePath);
    
    // DPI対応
    QPixmap extractIconForDPI(const QString& executablePath, int dpi);
    int getCurrentDPI() const;
    int getOptimalIconSize(int dpi) const;
    
    // キャッシング
    void enableCaching(bool enabled) { m_cachingEnabled = enabled; }
    bool isCachingEnabled() const { return m_cachingEnabled; }
    void clearCache();
    int cacheSize() const;
    
    // 設定
    void setDefaultMethod(ExtractionMethod method) { m_defaultMethod = method; }
    ExtractionMethod defaultMethod() const { return m_defaultMethod; }
    
    void setQualityMode(bool highQuality) { m_highQuality = highQuality; }
    bool isHighQualityMode() const { return m_highQuality; }
    
signals:
    void iconExtracted(const QString& path, const QPixmap& icon);
    void extractionFailed(const QString& path, const QString& error);
    
public slots:
    void preloadIcons(const QStringList& executablePaths);
    void refreshIconCache();
    
private:
    struct IconCacheEntry {
        QPixmap pixmap;
        qint64 timestamp;
        QString method;
        int accessCount;
    };
    
    QMap<QString, IconCacheEntry> m_iconCache;
    bool m_cachingEnabled;
    ExtractionMethod m_defaultMethod;
    bool m_highQuality;
    
    // 抽出メソッド実装
    QPixmap extractUsingSHGetFileInfo(const QString& path, int size);
    QPixmap extractUsingExtractIcon(const QString& path, int size);
    QPixmap extractUsingWindowIcon(HWND hwnd, int size);
    QPixmap extractUsingSystemImageList(const QString& path, int size);
    
    // ヘルパーメソッド
    QString generateCacheKey(const QString& path, int size, const QString& method) const;
    int iconSizeToPixels(IconSize size) const;
    IconSize pixelsToIconSize(int pixels) const;
    ExtractionMethod selectBestMethod(const QString& path) const;
    
    // Windows API ヘルパー
    #ifdef Q_OS_WIN
    QPixmap hIconToQPixmap(HICON hIcon, int size = 0) const;
    HICON extractHIcon(const QString& path, int size, ExtractionMethod method);
    HICON getWindowHIcon(HWND hwnd, bool large = true);
    void destroyIcon(HICON hIcon) const;
    
    // システムイメージリスト操作
    HIMAGELIST getSystemImageList(bool large = true);
    int getSystemImageListIndex(const QString& path);
    QPixmap extractFromImageList(HIMAGELIST imageList, int index, int size);
    #endif
    
    // エラーハンドリング
    void logError(const QString& message) const;
    QString getLastWindowsError() const;
};

/**
 * @brief アイコン関連のユーティリティ関数
 */
namespace IconUtils {
    // アイコンサイズ変換
    QPixmap scaleIcon(const QPixmap& icon, int targetSize, 
                     Qt::TransformationMode mode = Qt::SmoothTransformation);
    
    // アイコン品質向上
    QPixmap enhanceIcon(const QPixmap& icon);
    QPixmap addIconShadow(const QPixmap& icon);
    QPixmap addIconBorder(const QPixmap& icon, const QColor& color);
    
    // アイコン検証
    bool isValidIcon(const QPixmap& icon);
    bool isHighDPIIcon(const QPixmap& icon);
    
    // デフォルトアイコン
    QPixmap getDefaultApplicationIcon(int size = 32);
    QPixmap getDefaultFileIcon(int size = 32);
    QPixmap getDefaultFolderIcon(int size = 32);
    
    // システムアイコン名
    extern const QString ICON_APPLICATION;
    extern const QString ICON_FOLDER;
    extern const QString ICON_FILE;
    extern const QString ICON_EXECUTABLE;
    extern const QString ICON_DOCUMENT;
    extern const QString ICON_ERROR;
    extern const QString ICON_WARNING;
    extern const QString ICON_INFORMATION;
}

/**
 * @brief 非同期アイコン抽出器
 */
class AsyncIconExtractor : public QObject
{
    Q_OBJECT
    
public:
    explicit AsyncIconExtractor(QObject* parent = nullptr);
    
    void extractIconAsync(const QString& executablePath, 
                         IconExtractor::IconSize size = IconExtractor::IconSize::Auto);
    
    void extractMultipleIconsAsync(const QStringList& paths);
    
    bool isBusy() const { return m_extractionCount > 0; }
    int pendingExtractions() const { return m_extractionCount; }
    
signals:
    void iconReady(const QString& path, const QPixmap& icon);
    void extractionFinished();
    void allExtractionsFinished();
    
private slots:
    void onIconExtracted(const QString& path, const QPixmap& icon);
    void onExtractionFailed(const QString& path, const QString& error);
    
private:
    IconExtractor* m_extractor;
    int m_extractionCount;
    QStringList m_pendingPaths;
    
    void processNextExtraction();
};