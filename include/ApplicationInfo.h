#pragma once

#include "Common.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

/**
 * @brief アプリケーション情報を管理するクラス
 * 
 * 一つのアプリケーション（実行ファイル）に関連する
 * 複数のウィンドウ情報をまとめて管理する
 */
class ApplicationInfo
{
public:
    ApplicationInfo() = default;
    ApplicationInfo(const QString& executablePath);
    
    // 基本情報
    QString executablePath() const { return m_executablePath; }
    QString appUserModelId() const { return m_appUserModelId; }
    QString processName() const { return m_processName; }
    QString displayName() const { return m_displayName; }
    QPixmap icon() const { return m_icon; }
    
    // ウィンドウ管理
    const QList<HWND>& windows() const { return m_windows; }
    void addWindow(HWND hwnd);
    void removeWindow(HWND hwnd);
    void clearWindows();
    bool hasWindow(HWND hwnd) const;
    int windowCount() const { return m_windows.size(); }
    
    // 状態情報
    bool isValid() const;
    bool isSystemApplication() const;
    bool isUWPApplication() const;
    WindowState primaryWindowState() const;
    
    // 表示用テキスト生成
    QString getDisplayText() const;
    QString getTooltipText() const;
    
    // グループID生成（グループ化の基準）
    QString generateGroupId() const;
    
    // 設定
    void setExecutablePath(const QString& path);
    void setAppUserModelId(const QString& appId);
    void setProcessName(const QString& name);
    void setDisplayName(const QString& name);
    void setIcon(const QPixmap& icon);
    
    // 比較演算子
    bool operator==(const ApplicationInfo& other) const;
    bool operator!=(const ApplicationInfo& other) const;
    
    // デバッグ用
    QString toString() const;
    
private:
    QString m_executablePath;
    QString m_appUserModelId;
    QString m_processName;
    QString m_displayName;
    QPixmap m_icon;
    QList<HWND> m_windows;
    
    // 内部メソッド
    void updateDisplayName();
    void detectAppUserModelId();
    bool isSystemPath(const QString& path) const;
};

/**
 * @brief アプリケーション情報のファクトリークラス
 */
class ApplicationInfoFactory
{
public:
    static ApplicationInfo createFromWindow(HWND hwnd);
    static ApplicationInfo createFromProcessId(DWORD processId);
    static ApplicationInfo createFromExecutablePath(const QString& path);
    
private:
    static QString extractDisplayNameFromPath(const QString& path);
    static QString getAppUserModelIdFromProcess(DWORD processId);
    static QPixmap extractIconFromExecutable(const QString& path);
};