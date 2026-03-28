# アプリケーショングループ化実装詳細

## Windows 10のグループ化仕様

### グループ化の3つのモード

#### 1. Always combine, hide labels（デフォルト）
- 同一アプリは1つのアイコンにまとめられる
- 複数インスタンスは枠線で表示
- ラベルなし

#### 2. Combine when taskbar is full
- 通常は個別表示
- タスクバーが満杯になると自動的にグループ化
- ラベル付きボタン表示

#### 3. Never combine
- 各ウィンドウが独立したボタンで表示
- 同一アプリは隣接配置される
- 完全な分離は不可

### グループ化の判定基準

1. **AppUserModelID**（最優先）
2. **実行ファイルの完全パス**
3. **プロセス名**

## 実装アーキテクチャ

### データ構造

```cpp
struct AppInfo {
    QString executablePath;
    QString appUserModelId;
    QString processName;
    QList<HWND> windows;
    QPixmap icon;
    QString displayName;
};

class ApplicationDetector {
private:
    QMap<QString, AppInfo> appGroups;
    
public:
    QString generateGroupId(HWND hwnd);
    AppInfo getApplicationInfo(HWND hwnd);
    void updateApplications();
};
```

### グループID生成ロジック

```cpp
QString ApplicationDetector::generateGroupId(HWND hwnd) {
    // 1. AppUserModelIDを優先
    QString appId = getAppUserModelId(hwnd);
    if (!appId.isEmpty()) {
        return QString("appid:%1").arg(appId);
    }
    
    // 2. 実行ファイルパス
    QString execPath = getExecutablePath(hwnd);
    if (!execPath.isEmpty()) {
        return QString("path:%1").arg(execPath);
    }
    
    // 3. プロセス名（フォールバック）
    QString processName = getProcessName(hwnd);
    return QString("process:%1").arg(processName);
}
```

### アプリケーション検出

```cpp
void ApplicationDetector::updateApplications() {
    QMap<QString, AppInfo> newGroups;
    
    // すべての可視ウィンドウを列挙
    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        auto* detector = reinterpret_cast<ApplicationDetector*>(lParam);
        
        if (IsWindowVisible(hwnd) && GetWindowTextLength(hwnd) > 0) {
            QString groupId = detector->generateGroupId(hwnd);
            
            if (!newGroups.contains(groupId)) {
                AppInfo info;
                info.executablePath = detector->getExecutablePath(hwnd);
                info.appUserModelId = detector->getAppUserModelId(hwnd);
                info.processName = detector->getProcessName(hwnd);
                info.icon = detector->getApplicationIcon(hwnd);
                info.displayName = detector->getDisplayName(info);
                newGroups[groupId] = info;
            }
            
            newGroups[groupId].windows.append(hwnd);
        }
        return TRUE;
    }, (LPARAM)this);
    
    // 古いグループと比較して変更を検出
    detectChanges(appGroups, newGroups);
    appGroups = newGroups;
}
```

## UI実装

### グループ化ウィジェット

```cpp
class TaskbarGroupWidget : public QWidget {
    Q_OBJECT
    
private:
    AppInfo appInfo;
    QPushButton* mainButton;
    QWidget* subWindowsWidget;
    QVBoxLayout* layout;
    bool isExpanded = false;
    
public:
    TaskbarGroupWidget(const AppInfo& info, QWidget* parent = nullptr);
    void updateAppInfo(const AppInfo& info);
    
private:
    void setupMainButton();
    void setupSubWindows();
    QString getMainButtonText();
    
private slots:
    void onMainButtonClicked();
    void onSubWindowClicked(HWND hwnd);
    
signals:
    void windowActivated(HWND hwnd);
};
```

### メインボタンの実装

```cpp
void TaskbarGroupWidget::setupMainButton() {
    mainButton = new QPushButton(this);
    mainButton->setIcon(appInfo.icon);
    mainButton->setIconSize(QSize(getTaskbarIconSize(), getTaskbarIconSize()));
    mainButton->setText(getMainButtonText());
    mainButton->setToolTip(appInfo.displayName);
    
    // Windows 10スタイル
    mainButton->setStyleSheet(R"(
        QPushButton {
            text-align: left;
            padding: 4px 8px;
            border: 1px solid transparent;
            border-radius: 2px;
            background: transparent;
            min-height: 24px;
        }
        QPushButton:hover {
            background: #e5f3ff;
            border: 1px solid #cce8ff;
        }
        QPushButton:pressed {
            background: #cce8ff;
        }
    )");
    
    connect(mainButton, &QPushButton::clicked, 
            this, &TaskbarGroupWidget::onMainButtonClicked);
}
```

### サブウィンドウ表示

```cpp
void TaskbarGroupWidget::setupSubWindows() {
    if (appInfo.windows.size() <= 1) {
        subWindowsWidget->setVisible(false);
        return;
    }
    
    // 既存のサブウィンドウをクリア
    QLayout* subLayout = subWindowsWidget->layout();
    if (subLayout) {
        QLayoutItem* item;
        while ((item = subLayout->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
    } else {
        subLayout = new QVBoxLayout(subWindowsWidget);
        subLayout->setContentsMargins(16, 0, 0, 0);
        subLayout->setSpacing(1);
    }
    
    // 各ウィンドウのサブボタンを作成
    for (HWND hwnd : appInfo.windows) {
        wchar_t title[256];
        GetWindowText(hwnd, title, 256);
        QString windowTitle = QString::fromWCharArray(title);
        
        auto subButton = new QPushButton(windowTitle, subWindowsWidget);
        subButton->setObjectName("subWindow");
        subButton->setStyleSheet(R"(
            QPushButton#subWindow {
                text-align: left;
                padding: 2px 8px;
                border: none;
                background: transparent;
                font-size: 12px;
                min-height: 20px;
            }
            QPushButton#subWindow:hover {
                background: #e5f3ff;
            }
        )");
        
        connect(subButton, &QPushButton::clicked, [this, hwnd]() {
            emit windowActivated(hwnd);
        });
        
        subLayout->addWidget(subButton);
    }
}
```

### ボタンクリック処理

```cpp
void TaskbarGroupWidget::onMainButtonClicked() {
    if (appInfo.windows.size() == 1) {
        // 単一ウィンドウの場合は直接アクティベート
        emit windowActivated(appInfo.windows.first());
    } else if (appInfo.windows.size() > 1) {
        // 複数ウィンドウの場合は展開/折りたたみ
        isExpanded = !isExpanded;
        subWindowsWidget->setVisible(isExpanded);
        updateExpandIcon();
    }
}

void TaskbarGroupWidget::updateExpandIcon() {
    if (appInfo.windows.size() > 1) {
        QString arrowIcon = isExpanded ? "▼" : "▶";
        QString buttonText = QString("%1 %2").arg(arrowIcon, getMainButtonText());
        mainButton->setText(buttonText);
    }
}
```

### 表示テキスト生成

```cpp
QString TaskbarGroupWidget::getMainButtonText() {
    if (appInfo.windows.size() == 1) {
        // 単一ウィンドウの場合はタイトル表示
        wchar_t title[256];
        GetWindowText(appInfo.windows.first(), title, 256);
        QString windowTitle = QString::fromWCharArray(title);
        
        // 長すぎる場合は省略
        if (windowTitle.length() > 30) {
            windowTitle = windowTitle.left(27) + "...";
        }
        return windowTitle;
    } else {
        // 複数ウィンドウの場合はアプリ名 + 個数
        QString appName = QFileInfo(appInfo.executablePath).baseName();
        if (appName.isEmpty()) {
            appName = appInfo.displayName;
        }
        return QString("%1 (%2)").arg(appName).arg(appInfo.windows.size());
    }
}
```

## 特別なケースの処理

### システムアプリケーション
```cpp
bool isSystemApplication(const QString& executablePath) {
    QString systemPath = QDir::toNativeSeparators(QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation));
    QString windowsPath = "C:\\Windows\\";
    
    return executablePath.startsWith(windowsPath, Qt::CaseInsensitive) ||
           executablePath.startsWith(systemPath, Qt::CaseInsensitive);
}
```

### UWPアプリケーション
```cpp
bool isUWPApplication(HWND hwnd) {
    // UWPアプリは特別なAppUserModelIDを持つ
    QString appId = getAppUserModelId(hwnd);
    return appId.contains("!App", Qt::CaseInsensitive) && 
           appId.contains("_", Qt::CaseInsensitive);
}
```

### ダイアログウィンドウの除外
```cpp
bool shouldIncludeWindow(HWND hwnd) {
    // 親ウィンドウがある場合は除外（ダイアログなど）
    HWND parent = GetParent(hwnd);
    if (parent != nullptr) {
        return false;
    }
    
    // ツールウィンドウスタイルは除外
    LONG_PTR exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
    if (exStyle & WS_EX_TOOLWINDOW) {
        return false;
    }
    
    // 通常のアプリケーションウィンドウのみ含める
    LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
    return (style & WS_OVERLAPPEDWINDOW) != 0;
}
```