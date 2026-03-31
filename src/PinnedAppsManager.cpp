#include "PinnedAppsManager.h"
#include <QSettings>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDebug>
#include <QDateTime>
#include <QApplication>
#include <shellapi.h>
#include <shlobj.h>
#include <propvarutil.h>
#include <comdef.h>

// ログ出力関数（マクロ競合を回避）
void PinnedAppsManager::logInfo(const QString &message)
{
    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    QString logMessage = QString("[%1] %2").arg(timestamp, message);
    qDebug() << logMessage;
}

PinnedAppsManager::PinnedAppsManager(QObject *parent)
    : QObject(parent)
    , m_refreshTimer(new QTimer(this))
{
    logInfo("🔧 PinnedAppsManager初期化開始");
    
    // タイマー設定
    m_refreshTimer->setSingleShot(false);
    connect(m_refreshTimer, &QTimer::timeout, this, &PinnedAppsManager::onRefreshTimer);
    
    // 初回のピン留めアプリ検出
    refreshPinnedApps();
    
    logInfo("✅ PinnedAppsManager初期化完了");
}

QList<PinnedAppInfo> PinnedAppsManager::getPinnedApps() const
{
    return m_pinnedApps;
}

void PinnedAppsManager::refreshPinnedApps()
{
    logInfo("🔄 ピン留めアプリ一覧を更新開始");
    
    QList<PinnedAppInfo> oldList = m_pinnedApps;
    m_pinnedApps.clear();
    
    // レジストリからの検出を試行
    detectPinnedAppsFromRegistry();
    
    // ショートカットからの検出も試行（フォールバック）
    if (m_pinnedApps.isEmpty()) {
        detectPinnedAppsFromShortcuts();
    }
    
    logInfo(QString("📌 ピン留めアプリ検出完了: %1個").arg(m_pinnedApps.size()));
    
    // リストが変更された場合のみシグナル発出
    if (m_pinnedApps.size() != oldList.size()) {
        emit pinnedAppsChanged();
        logInfo("📡 ピン留めアプリ変更シグナル発出");
    }
}

void PinnedAppsManager::startAutoRefresh(int intervalMs)
{
    logInfo(QString("⏰ ピン留めアプリ自動更新開始（%1ms間隔）").arg(intervalMs));
    m_refreshTimer->setInterval(intervalMs);
    m_refreshTimer->start();
}

void PinnedAppsManager::stopAutoRefresh()
{
    logInfo("⏹️ ピン留めアプリ自動更新停止");
    m_refreshTimer->stop();
}

bool PinnedAppsManager::isAppPinned(const QString &executablePath) const
{
    QString cleanPath = QFileInfo(executablePath).absoluteFilePath().toLower();
    
    for (const auto &pinnedApp : m_pinnedApps) {
        QString pinnedPath = QFileInfo(pinnedApp.executablePath).absoluteFilePath().toLower();
        if (pinnedPath == cleanPath) {
            return true;
        }
    }
    return false;
}

void PinnedAppsManager::onRefreshTimer()
{
    refreshPinnedApps();
}

void PinnedAppsManager::detectPinnedAppsFromRegistry()
{
    logInfo("🔍 レジストリからピン留めアプリ検出開始");
    
    // Windows 10/11のタスクバーピン留め情報はレジストリとファイルシステムに分散
    // 主要な場所：
    // HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\Taskband
    
    QSettings taskbandRegistry(
        "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Taskband", 
        QSettings::Registry64Format
    );
    
    QStringList keys = taskbandRegistry.allKeys();
    logInfo(QString("📋 Taskbandレジストリキー数: %1").arg(keys.size()));
    
    for (const auto &key : std::as_const(keys)) {
        QVariant value = taskbandRegistry.value(key);
        logInfo(QString("🔑 レジストリキー: %1 = %2").arg(key, value.toString()));
    }
    
    // Windows 11の新しい場所も確認
    QSettings explorerRegistry(
        "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer", 
        QSettings::Registry64Format
    );
    
    // 現在のところ、レジストリからの直接取得は複雑なため
    // ショートカットベースの検出をメインにする
    logInfo("⚠️ レジストリからの検出は今後実装予定、ショートカット検出に切り替え");
}

void PinnedAppsManager::detectPinnedAppsFromShortcuts()
{
    logInfo("📁 ショートカットからピン留めアプリ検出開始");
    
    // Windows 10/11のタスクバーピン留めショートカットの正確な場所
    QString userProfile = QDir::homePath();
    QString pinnedPath = userProfile + "/AppData/Roaming/Microsoft/Internet Explorer/Quick Launch/User Pinned/TaskBar";
    
    logInfo(QString("📂 ピン留めショートカットフォルダ: %1").arg(pinnedPath));
    
    QDir pinnedDir(pinnedPath);
    if (!pinnedDir.exists()) {
        logInfo("❌ ピン留めフォルダが見つかりません");
        return;
    }
    
    QStringList filters;
    filters << "*.lnk";
    QFileInfoList shortcutFiles = pinnedDir.entryInfoList(filters, QDir::Files);
    
    logInfo(QString("🔗 ショートカットファイル数: %1").arg(shortcutFiles.size()));
    
    int order = 0;
    for (const auto &shortcutInfo : std::as_const(shortcutFiles)) {
        QString shortcutPath = shortcutInfo.absoluteFilePath();
        QString targetPath = resolveShortcutTarget(shortcutPath);
        
        if (!targetPath.isEmpty() && isValidExecutable(targetPath)) {
            PinnedAppInfo appInfo;
            appInfo.name = QFileInfo(targetPath).baseName();
            appInfo.executablePath = targetPath;
            appInfo.icon = extractIconFromPath(targetPath);
            appInfo.order = order++;
            appInfo.isValid = true;
            
            m_pinnedApps.append(appInfo);
            
            logInfo(QString("📌 ピン留めアプリ検出: %1 (%2)").arg(appInfo.name, targetPath));
        } else {
            logInfo(QString("❌ 無効なショートカット: %1").arg(shortcutPath));
        }
    }
}

QPixmap PinnedAppsManager::extractIconFromPath(const QString &path)
{
    if (path.isEmpty()) {
        return QPixmap();
    }
    
    // Windows APIを使ってアイコンを取得（TaskbarModel.cppと同様）
    SHFILEINFO fileInfo;
    ZeroMemory(&fileInfo, sizeof(fileInfo));
    
    DWORD_PTR result = SHGetFileInfo(
        reinterpret_cast<LPCWSTR>(path.utf16()),
        0,
        &fileInfo,
        sizeof(fileInfo),
        SHGFI_ICON | SHGFI_LARGEICON
    );
    
    if (result != 0 && fileInfo.hIcon != nullptr) {
        // HICONをQPixmapに変換
        QPixmap pixmap = QPixmap::fromImage(QImage::fromHICON(fileInfo.hIcon));
        DestroyIcon(fileInfo.hIcon);
        return pixmap.scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    
    return QPixmap();
}

QString PinnedAppsManager::resolveShortcutTarget(const QString &shortcutPath)
{
    // COM初期化
    CoInitialize(nullptr);
    
    IShellLink* psl = nullptr;
    IPersistFile* ppf = nullptr;
    QString targetPath;
    
    // IShellLinkインターフェース取得
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, 
                                  IID_IShellLink, reinterpret_cast<void**>(&psl));
    
    if (SUCCEEDED(hr)) {
        // IPersistFileインターフェース取得
        hr = psl->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&ppf));
        
        if (SUCCEEDED(hr)) {
            // ショートカットファイルをロード
            hr = ppf->Load(reinterpret_cast<LPCOLESTR>(shortcutPath.utf16()), STGM_READ);
            
            if (SUCCEEDED(hr)) {
                // ターゲットパスを取得
                wchar_t targetBuffer[MAX_PATH];
                hr = psl->GetPath(targetBuffer, MAX_PATH, nullptr, SLGP_RAWPATH);
                
                if (SUCCEEDED(hr)) {
                    targetPath = QString::fromWCharArray(targetBuffer);
                }
            }
            
            ppf->Release();
        }
        
        psl->Release();
    }
    
    CoUninitialize();
    
    return targetPath;
}

bool PinnedAppsManager::isValidExecutable(const QString &path)
{
    if (path.isEmpty()) {
        return false;
    }
    
    QFileInfo fileInfo(path);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        return false;
    }
    
    // 実行可能ファイルの拡張子チェック
    QString suffix = fileInfo.suffix().toLower();
    QStringList executableExtensions = {"exe", "com", "bat", "cmd", "msi"};
    
    return executableExtensions.contains(suffix);
}