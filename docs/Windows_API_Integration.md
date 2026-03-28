# Windows API統合詳細資料

## Windows APIを使ったタスクバー情報取得

### 主要API関数

#### EnumWindows
```cpp
BOOL EnumWindows(
  WNDENUMPROC lpEnumFunc,  // コールバック関数
  LPARAM      lParam       // ユーザー定義データ
);

static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    if (IsWindowVisible(hwnd) && GetWindowTextLength(hwnd) > 0) {
        // ウィンドウ処理
    }
    return TRUE;
}
```

#### アプリケーションアイコン取得
```cpp
// 方法1: SHGetFileInfo（推奨）
QPixmap getApplicationIcon(const QString& executablePath) {
    SHFILEINFO shfi = { 0 };
    DWORD_PTR result = SHGetFileInfo(
        executablePath.toStdWString().c_str(),
        0, &shfi, sizeof(shfi),
        SHGFI_ICON | SHGFI_LARGEICON
    );
    
    if (result && shfi.hIcon) {
        QPixmap pixmap = QPixmap::fromImage(
            QImage::fromHICON(shfi.hIcon)
        );
        DestroyIcon(shfi.hIcon); // メモリリーク防止
        return pixmap;
    }
    return QPixmap();
}

// 方法2: ウィンドウから直接取得
HICON getWindowIcon(HWND hwnd) {
    HICON hIcon = (HICON)SendMessage(hwnd, WM_GETICON, ICON_BIG, 0);
    if (!hIcon) {
        hIcon = (HICON)SendMessage(hwnd, WM_GETICON, ICON_SMALL, 0);
    }
    if (!hIcon) {
        hIcon = (HICON)GetClassLongPtr(hwnd, GCLP_HICON);
    }
    return hIcon;
}
```

### プロセス情報取得

#### 実行ファイルパス取得
```cpp
QString getExecutablePath(HWND hwnd) {
    DWORD processId;
    GetWindowThreadProcessId(hwnd, &processId);
    
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (!hProcess) return QString();
    
    wchar_t exePath[MAX_PATH];
    DWORD size = MAX_PATH;
    if (QueryFullProcessImageName(hProcess, 0, exePath, &size)) {
        CloseHandle(hProcess);
        return QString::fromWCharArray(exePath);
    }
    
    CloseHandle(hProcess);
    return QString();
}
```

#### AppUserModelID取得
```cpp
QString getAppUserModelId(HWND hwnd) {
    DWORD processId;
    GetWindowThreadProcessId(hwnd, &processId);
    
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processId);
    if (!hProcess) return QString();
    
    PWSTR appId = nullptr;
    if (SUCCEEDED(GetCurrentProcessExplicitAppUserModelID(&appId))) {
        QString result = QString::fromWCharArray(appId);
        CoTaskMemFree(appId);
        CloseHandle(hProcess);
        return result;
    }
    
    CloseHandle(hProcess);
    return QString();
}
```

## メモリ管理

### 重要な注意点
```cpp
// HICONリソースは必ず解放する
HICON hIcon = getWindowIcon(hwnd);
if (hIcon) {
    // 使用後
    DestroyIcon(hIcon);
}

// AppUserModelIDの文字列はCoTaskMemFreeで解放
PWSTR appId = nullptr;
if (SUCCEEDED(GetCurrentProcessExplicitAppUserModelID(&appId))) {
    QString result = QString::fromWCharArray(appId);
    CoTaskMemFree(appId); // 必須
}

// GetIconInfo使用時
ICONINFO iconInfo;
if (GetIconInfo(hIcon, &iconInfo)) {
    // 作成されたビットマップを削除
    if (iconInfo.hbmColor) DeleteObject(iconInfo.hbmColor);
    if (iconInfo.hbmMask) DeleteObject(iconInfo.hbmMask);
}
```

## エラーハンドリング

### 一般的なエラーパターン
```cpp
// プロセスアクセス権限不足
HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processId);
if (!hProcess) {
    // UAC昇格が必要な場合やシステムプロセス
    return QString();
}

// ウィンドウハンドルの無効化
if (!IsWindow(hwnd)) {
    // ウィンドウが既に破棄されている
    return;
}

// アイコン取得失敗時のフォールバック
QPixmap icon = getApplicationIcon(execPath);
if (icon.isNull()) {
    // デフォルトアイコンを使用
    icon = QApplication::style()->standardIcon(QStyle::SP_ComputerIcon).pixmap(22, 22);
}
```

## パフォーマンス最適化

### 効率的な実装パターン
```cpp
class WindowInfoCache {
private:
    QMap<HWND, WindowInfo> cache;
    QTimer* updateTimer;
    
public:
    void updateCache() {
        QSet<HWND> currentWindows;
        
        // 新しいウィンドウリストを取得
        EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
            auto* windowSet = reinterpret_cast<QSet<HWND>*>(lParam);
            if (IsWindowVisible(hwnd) && GetWindowTextLength(hwnd) > 0) {
                windowSet->insert(hwnd);
            }
            return TRUE;
        }, (LPARAM)&currentWindows);
        
        // キャッシュ更新（差分のみ）
        for (auto it = cache.begin(); it != cache.end();) {
            if (!currentWindows.contains(it.key())) {
                it = cache.erase(it); // 削除されたウィンドウ
            } else {
                ++it;
            }
        }
        
        // 新しいウィンドウを追加
        for (HWND hwnd : currentWindows) {
            if (!cache.contains(hwnd)) {
                cache[hwnd] = createWindowInfo(hwnd);
            }
        }
    }
};
```

## Qt6統合のベストプラクティス

### HICONからQPixmapへの変換
```cpp
// Qt6でのHICON変換
QPixmap convertHIconToPixmap(HICON hIcon) {
    if (!hIcon) return QPixmap();
    
    QPixmap pixmap = QPixmap::fromImage(QImage::fromHICON(hIcon));
    
    // DPI対応リサイズ
    int targetSize = getTaskbarIconSize();
    if (!pixmap.isNull() && pixmap.size() != QSize(targetSize, targetSize)) {
        pixmap = pixmap.scaled(targetSize, targetSize, 
                               Qt::KeepAspectRatio, 
                               Qt::SmoothTransformation);
    }
    
    return pixmap;
}
```

### DPI対応
```cpp
int getTaskbarIconSize() {
    int dpi = GetDpiForSystem();
    if (dpi <= 96) return 22;      // 100%スケール
    if (dpi <= 120) return 28;     // 125%スケール  
    if (dpi <= 144) return 33;     // 150%スケール
    return 44;                     // 200%スケール
}
```