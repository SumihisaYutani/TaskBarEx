#include "TaskbarWindow.h"
#include "ui_TaskbarWindow.h"
#include "TaskbarModel.h"
#include "TaskbarGroupManager.h"
#include "TaskbarVisibilityMonitor.h"
#include "Logger.h"
#include <QListWidgetItem>
#include <QPixmap>
#include <QApplication>
#include <QScreen>
#include <QMessageBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <windows.h>

// 静的メンバーの初期化
TaskbarWindow* TaskbarWindow::s_instance = nullptr;

TaskbarWindow::TaskbarWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::TaskbarWindow)  // UIファイルを再有効化
    , m_updateTimer(new QTimer(this))
    , m_model(new TaskbarModel(this))
    , m_groupManager(new TaskbarGroupManager(this))
    , m_visibilityMonitor(new TaskbarVisibilityMonitor(this))
    , m_focusHook(nullptr)
    , m_isMouseOver(false)
    , m_mouseTrackTimer(new QTimer(this))
    , m_showDelayTimer(new QTimer(this))
    , m_fixedTaskbarTop(1032)  // デフォルト値
{
    LOG_INFO("TaskbarWindow constructor started");
    
    // UIファイル読み込みをテスト
    try {
        ui->setupUi(this);
        LOG_INFO("UI setup completed");
        
        // タスクバー風の設定に変更
        setWindowTitle("TaskBarEx");
        
        // ★重要：フォーカスを取らないウィンドウフラグを設定（循環参照対策）
        setWindowFlags(Qt::Tool | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
        setAttribute(Qt::WA_ShowWithoutActivating, true);  // アクティベートなしで表示
        
        setGeometry(200, 200, 1000, 48);  // タスクバー風の高さに変更
        LOG_INFO(QString("🔍 Height after setGeometry: %1").arg(height()));
        
        // 高さを強制的に固定
        setFixedHeight(48);
        LOG_INFO(QString("🔍 Height after setFixedHeight: %1").arg(height()));
        
        // メニューバーを非表示にしてスペースを確保
        menuBar()->setVisible(false);
        
        // ウィンドウフラグを設定
        setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);
        
        // タスクバー風のスタイル
        setStyleSheet(
            "QMainWindow {"
            "    background-color: rgba(32, 32, 32, 200);"  // 半透明ダーク背景
            "    border: 1px solid rgba(255, 255, 255, 100);"
            "}"
            "QScrollArea {"
            "    background-color: transparent;"
            "    border: none;"
            "}"
        );
        
        LOG_INFO("Taskbar-style window setup completed");
        
    } catch (...) {
        LOG_ERROR("UI setup failed");
        
        // フォールバック：UI無しモード
        setWindowTitle("TaskBarEx Debug Test - UI FAILED");
        setGeometry(200, 200, 600, 80);
        setStyleSheet("background-color: purple; border: 5px solid orange;");
        
        QLabel* errorLabel = new QLabel("UI SETUP FAILED", this);
        errorLabel->setAlignment(Qt::AlignCenter);
        errorLabel->setStyleSheet("color: white; font-size: 24px; font-weight: bold;");
        setCentralWidget(errorLabel);
    }
    
    // ウィンドウが閉じられてもアプリケーションを終了させない
    setAttribute(Qt::WA_QuitOnClose, false);
    setAttribute(Qt::WA_DeleteOnClose, false);
    
    LOG_INFO("Window flags and attributes set");
    
    // 基本機能を段階的に復元 - 一つずつテスト
    LOG_INFO("Starting to restore taskbar functionality step by step...");
    
    try {
        LOG_INFO("Step 1: Setting window properties...");
        // ウィンドウ設定は上記で完了済み（重複を避けるためここでは行わない）
        
        // デバッグモードでは画面中央に表示（Qt Creatorでの確認用）
        #ifdef _DEBUG
        // スクリーン情報を取得
        QScreen *screen = QApplication::primaryScreen();
        if (screen) {
            QRect screenRect = screen->geometry();
            int centerX = (screenRect.width() - width()) / 2;
            int centerY = (screenRect.height() - height()) / 2;
            setGeometry(centerX, centerY, 1000, 48);
            LOG_INFO(QString("DEBUG MODE: Window centered at: %1,%2").arg(centerX).arg(centerY));
            LOG_INFO(QString("Screen size: %1x%2").arg(screenRect.width()).arg(screenRect.height()));
        }
        #endif
        
        LOG_INFO(QString("Window positioned at: %1,%2 %3x%4")
                 .arg(x()).arg(y()).arg(width()).arg(height()));
        
        LOG_INFO("Step 1 completed successfully");
    } catch (...) {
        LOG_ERROR("Step 1 (window properties) failed");
    }
    
    try {
        LOG_INFO("Step 2: Setting up connections...");
        setupConnections(); // 有効化
        LOG_INFO("Step 2 completed (connections enabled)");
    } catch (...) {
        LOG_ERROR("Step 2 (setupConnections) failed");
    }
    
    try {
        LOG_INFO("Step 3: Setting up timer...");
        setupTimer(); // 有効化
        LOG_INFO("Step 3 completed (timer enabled)");
    } catch (...) {
        LOG_ERROR("Step 3 (setupTimer) failed");
    }
    
    try {
        LOG_INFO("Step 4: Activating full taskbar functionality...");
        
        LOG_INFO("Step 4a: Enabling window enumeration...");
        // 初回ウィンドウ列挙を実行
        LOG_INFO("About to call m_model->updateWindowList()...");
        m_model->updateWindowList();
        populateTaskbarList();
        LOG_INFO("Initial window list populated");
        
        LOG_INFO("Step 4b: Starting automatic refresh timer...");
        // タイマーを開始
        m_updateTimer->start();
        LOG_INFO("Timer started successfully");
        
        setWindowTitle("TaskBarEx - フル機能");
        LOG_INFO("Full taskbar functionality activated");
        
    } catch (...) {
        LOG_ERROR("Step 4 failed with exception");
        setWindowTitle("TaskBarEx - 全体エラー");
    }
    
    LOG_INFO("Taskbar functionality restoration completed");
    
    try {
        LOG_INFO("Step 5: Setting up window positioning...");
        positionAboveTaskbar();
        LOG_INFO("Step 5 completed (window positioning)");
    } catch (...) {
        LOG_ERROR("Step 5 (positionAboveTaskbar) failed");
    }
    
    try {
        LOG_INFO("Step 6: Setting up focus monitoring...");
        setupWindowFocusHook();
        LOG_INFO("Step 6 completed (focus monitoring)");
    } catch (...) {
        LOG_ERROR("Step 6 (setupWindowFocusHook) failed");
    }
    
    try {
        LOG_INFO("Step 7: Setting up mouse tracking...");
        setupMouseTracking();
        LOG_INFO("Step 7 completed (mouse tracking)");
    } catch (...) {
        LOG_ERROR("Step 7 (setupMouseTracking) failed");
    }
    
    try {
        LOG_INFO("Step 8: Setting initial visibility state...");
        
        // 🔍 デバッグ用：位置テストのため一時的に表示
        LOG_INFO("🔍 DEBUG: Testing position by showing window briefly...");
        show();
        QTimer::singleShot(200, this, [this]() {
            QRect currentGeometry = geometry();
            LOG_INFO(QString("🔍 TEST Position when shown: %1,%2 %3x%4")
                     .arg(currentGeometry.x()).arg(currentGeometry.y())
                     .arg(currentGeometry.width()).arg(currentGeometry.height()));
            
            HWND hwnd = reinterpret_cast<HWND>(winId());
            if (hwnd) {
                RECT winRect;
                if (GetWindowRect(hwnd, &winRect)) {
                    LOG_INFO(QString("🔍 TEST WINDOWS API: left=%1, top=%2, right=%3, bottom=%4")
                             .arg(winRect.left).arg(winRect.top)
                             .arg(winRect.right).arg(winRect.bottom));
                }
            }
            
            // テスト後は非表示に戻す
            hide();
            LOG_INFO("🔍 DEBUG: Test completed, window hidden");
        });
        
        LOG_INFO("Step 8 completed (debug test mode)");
    } catch (...) {
        LOG_ERROR("Step 8 (debug test) failed");
    }
    
    LOG_INFO("TaskbarWindow created successfully");
}

TaskbarWindow::~TaskbarWindow()
{
    // フォーカスフックをクリーンアップ
    cleanupWindowFocusHook();
    
    // 静的インスタンスをリセット
    s_instance = nullptr;
    
    delete ui;
}

void TaskbarWindow::positionAboveTaskbar()
{
    QScreen *screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    
    LOG_INFO(QString("Screen geometry: %1,%2 %3x%4")
             .arg(screenGeometry.x()).arg(screenGeometry.y())
             .arg(screenGeometry.width()).arg(screenGeometry.height()));
    
    // タスクバーの完全表示時の固定位置を計算（動的位置は使わない）
    // 標準的なタスクバー高さ48pxで、画面下端から48px上が完全表示位置
    int standardTaskbarHeight = 48;
    int fixedTaskbarY = screenGeometry.height() - standardTaskbarHeight;
    m_fixedTaskbarTop = fixedTaskbarY;  // 固定位置を保存
    
    LOG_INFO(QString("Using FIXED taskbar position: %1 (screen height: %2, taskbar height: %3)")
             .arg(fixedTaskbarY).arg(screenGeometry.height()).arg(standardTaskbarHeight));
    
    // TaskBarExの下端をタスクバーの上端にぴったり合わせる
    int spacing = 0; // 間隔なしでタスクバー直上に配置
    int windowY = fixedTaskbarY - height() - spacing;
    int windowX = screenGeometry.left();
    int windowWidth = screenGeometry.width();
    
    // Y座標が負数にならないよう調整（画面上端を超える場合）
    if (windowY < screenGeometry.top()) {
        windowY = screenGeometry.top();
        LOG_WARNING("TaskBarEx position adjusted to avoid going above screen");
    }
    
    LOG_INFO(QString("Setting FIXED window position: %1,%2 %3x%4 (directly above taskbar)")
             .arg(windowX).arg(windowY).arg(windowWidth).arg(height()));
    
    setGeometry(windowX, windowY, windowWidth, height());
    
    // デバッグ用：実際に設定された位置を確認
    QTimer::singleShot(100, this, [this]() {
        QRect actualGeometry = geometry();
        LOG_INFO(QString("🔍 ACTUAL window position after setGeometry: %1,%2 %3x%4")
                 .arg(actualGeometry.x()).arg(actualGeometry.y())
                 .arg(actualGeometry.width()).arg(actualGeometry.height()));
        
        // Windows API でも確認
        HWND hwnd = reinterpret_cast<HWND>(winId());
        if (hwnd) {
            RECT winRect;
            if (GetWindowRect(hwnd, &winRect)) {
                LOG_INFO(QString("🔍 WINDOWS API position: left=%1, top=%2, right=%3, bottom=%4")
                         .arg(winRect.left).arg(winRect.top)
                         .arg(winRect.right).arg(winRect.bottom));
            }
        }
    });
    
    // 静的インスタンスを設定
    s_instance = this;
}

void TaskbarWindow::setupConnections()
{
    // メニューアクション
    connect(ui->actionRefresh, &QAction::triggered,
            this, &TaskbarWindow::onRefreshTriggered);
    connect(ui->actionAlwaysOnTop, &QAction::toggled,
            this, &TaskbarWindow::onAlwaysOnTopTriggered);
    connect(ui->actionAbout, &QAction::triggered,
            this, &TaskbarWindow::onAboutTriggered);
    
    // タスクバー監視機能
    connect(m_visibilityMonitor, &TaskbarVisibilityMonitor::taskbarVisibilityChanged,
            this, &TaskbarWindow::onTaskbarVisibilityChanged);
    
    // フォーカス変更監視機能は setupWindowFocusHook() で Windows API を使用
    
    // タスクバー監視開始
    m_visibilityMonitor->startMonitoring();
}

void TaskbarWindow::setupTimer()
{
    m_updateTimer->setInterval(1000); // 1秒間隔
    connect(m_updateTimer, &QTimer::timeout, 
            this, &TaskbarWindow::updateTaskbarItems);
    LOG_INFO("Timer setup completed and ready to start");
}

void TaskbarWindow::updateTaskbarItems()
{
    LOG_INFO("=== updateTaskbarItems called ===");
    m_model->updateWindowList();
    populateTaskbarList();
    updateStatusBar();
    LOG_INFO("=== updateTaskbarItems completed ===");
}

void TaskbarWindow::populateTaskbarList()
{
    LOG_INFO("Starting populateTaskbarList...");
    
    try {
        // デバッグ用：UIコンポーネントの存在確認
        LOG_INFO("Step 1: Checking UI components...");
        if (!ui) {
            LOG_ERROR("ui is NULL!");
            return;
        }
        LOG_INFO("ui is valid");
        
        if (!ui->taskbarScrollArea) {
            LOG_ERROR("taskbarScrollArea is NULL!");
            return;
        }
        LOG_INFO("taskbarScrollArea is valid");
        
        QWidget* scrollWidget = ui->taskbarScrollArea->widget();
        if (!scrollWidget) {
            LOG_ERROR("Scroll area widget is NULL!");
            return;
        }
        LOG_INFO("scrollWidget is valid");
        
        QLayout* layout = scrollWidget->layout();
        if (!layout) {
            LOG_ERROR("Scroll area layout is NULL!");
            return;
        }
        LOG_INFO("layout is valid");
    } catch (...) {
        LOG_ERROR("Exception in UI component check");
        return;
    }
    
    // レイアウトを再取得（スコープ修正）
    QWidget* scrollWidget = ui->taskbarScrollArea->widget();
    QLayout* layout = scrollWidget->layout();
    
    LOG_INFO(QString("UI components verified - ScrollArea: %1, Widget: %2, Layout: %3")
             .arg(ui->taskbarScrollArea ? "OK" : "NULL")
             .arg(scrollWidget ? "OK" : "NULL") 
             .arg(layout ? "OK" : "NULL"));
    
    try {
        LOG_INFO("Step 2: Clearing taskbar buttons...");
        clearTaskbarButtons();
        LOG_INFO("Taskbar buttons cleared");
    } catch (...) {
        LOG_ERROR("Exception in clearTaskbarButtons");
        return;
    }
    
    QVector<WindowInfo> windows;
    try {
        LOG_INFO("Step 3: Getting visible windows...");
        windows = m_model->getVisibleWindows();
        LOG_INFO(QString("Model returned %1 visible windows").arg(windows.size()));
    } catch (...) {
        LOG_ERROR("Exception in getting visible windows");
        return;
    }
    
    QVector<QVector<WindowInfo>> groups;
    try {
        LOG_INFO("Step 4: Grouping windows...");
        groups = m_groupManager->groupWindows(windows);
        LOG_INFO(QString("GroupManager created %1 groups from %2 windows").arg(groups.size()).arg(windows.size()));
    } catch (...) {
        LOG_ERROR("Exception in grouping windows");
        return;
    }
    
    try {
        LOG_INFO("Step 5: Creating buttons...");
        int buttonCount = 0;
        for (int groupIndex = 0; groupIndex < groups.size(); ++groupIndex) {
            const auto& group = groups[groupIndex];
            LOG_INFO(QString("Group %1 contains %2 windows").arg(groupIndex + 1).arg(group.size()));
            
            for (const auto& windowInfo : group) {
                LOG_INFO(QString("  - Creating button for: %1 (AppId: %2, Executable: %3)")
                         .arg(windowInfo.title)
                         .arg(windowInfo.appUserModelId)
                         .arg(windowInfo.executablePath));
                try {
                    createTaskbarButton(windowInfo);
                    buttonCount++;
                    LOG_INFO(QString("    Button created successfully for: %1").arg(windowInfo.title));
                } catch (...) {
                    LOG_ERROR(QString("    Exception creating button for: %1").arg(windowInfo.title));
                }
            }
        }
        LOG_INFO(QString("Created %1 taskbar buttons").arg(buttonCount));
    } catch (...) {
        LOG_ERROR("Exception in creating buttons");
        return;
    }
    
    // デバッグ用：最終的なレイアウトの状態をチェック
    LOG_INFO(QString("Final layout state - Item count: %1").arg(layout->count()));
    LOG_INFO(QString("ScrollArea size: %1x%2, Widget size: %3x%4")
             .arg(ui->taskbarScrollArea->width()).arg(ui->taskbarScrollArea->height())
             .arg(scrollWidget->width()).arg(scrollWidget->height()));
}

void TaskbarWindow::clearTaskbarButtons()
{
    // 既存のボタンをすべて削除
    QLayout* layout = ui->taskbarScrollArea->widget()->layout();
    if (layout) {
        QLayoutItem* item;
        while ((item = layout->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
    }
}

void TaskbarWindow::createTaskbarButton(const WindowInfo& window)
{
    LOG_INFO(QString("Creating button for window: %1").arg(window.title));
    
    QPushButton* button = new QPushButton();
    button->setToolTip(window.title);
    button->setCheckable(true);
    button->setChecked(!window.isMinimized);
    
    // Windows風タスクバーボタンサイズ設定
    button->setMinimumSize(44, 44);
    button->setMaximumSize(44, 44);
    button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    
    // アイコンとテキストの混合表示（ハングアップ対策）
    if (!window.icon.isNull() && window.icon.width() > 0 && window.icon.height() > 0) {
        try {
            // アイコンが有効な場合のみ設定
            LOG_INFO(QString("Valid icon detected - size: %1x%2").arg(window.icon.width()).arg(window.icon.height()));
            QIcon icon(window.icon);
            button->setIcon(icon);
            button->setIconSize(QSize(32, 32));  // 安全なアイコンサイズ
            button->setText(""); // テキストをクリア
            LOG_INFO("Button with icon created successfully");
        } catch (...) {
            // アイコン設定でエラーが発生した場合はテキスト表示にフォールバック
            LOG_WARNING("Icon setting failed, falling back to text");
            QString displayText = window.title.isEmpty() ? "?" : window.title.left(2).toUpper();
            button->setText(displayText);
        }
    } else {
        // アイコンが無効な場合はテキスト表示
        QString displayText = window.title.isEmpty() ? "?" : window.title.left(2).toUpper();
        button->setText(displayText);
        LOG_INFO(QString("Button created with text: '%1' for window: %2").arg(displayText).arg(window.title));
    }
    
    // Windows風タスクバーボタンスタイル（統一）
    button->setStyleSheet(
        "QPushButton {"
        "    background-color: rgba(255, 255, 255, 10);"
        "    border: 1px solid rgba(255, 255, 255, 20);"
        "    border-radius: 4px;"
        "    margin: 2px;"
        "    padding: 2px;"
        "    font-weight: bold;"
        "    color: white;"
        "    font-size: 12px;"
        "}"
        "QPushButton:hover {"
        "    background-color: rgba(255, 255, 255, 25);"
        "    border: 1px solid rgba(255, 255, 255, 40);"
        "}"
        "QPushButton:pressed {"
        "    background-color: rgba(255, 255, 255, 35);"
        "}"
        "QPushButton:checked {"
        "    background-color: rgba(0, 120, 215, 40);"
        "    border: 1px solid rgba(0, 120, 215, 60);"
        "}"
    );
    
    // HWNDをボタンに保存
    button->setProperty("hwnd", reinterpret_cast<qulonglong>(window.hwnd));
    
    // クリックイベント接続
    connect(button, &QPushButton::clicked, this, [this, button]() {
        HWND hwnd = reinterpret_cast<HWND>(button->property("hwnd").toULongLong());
        m_model->activateWindow(hwnd);
    });
    
    // レイアウトに追加
    QHBoxLayout* layout = qobject_cast<QHBoxLayout*>(ui->taskbarScrollArea->widget()->layout());
    if (layout) {
        layout->addWidget(button);
        LOG_INFO(QString("Button added to layout - Layout now has %1 items").arg(layout->count()));
        
        // デバッグ用：ボタンが見えるかチェック
        button->show();
        LOG_INFO(QString("Button visibility: %1, size: %2x%3, pos: %4,%5")
                 .arg(button->isVisible() ? "visible" : "hidden")
                 .arg(button->width()).arg(button->height())
                 .arg(button->x()).arg(button->y()));
    } else {
        LOG_ERROR("Layout not found!");
    }
    
    LOG_INFO("Button creation completed");
}

void TaskbarWindow::updateStatusBar()
{
    // ステータスバー更新（ボタン数をカウント）
    QLayout* layout = ui->taskbarScrollArea->widget()->layout();
    int count = layout ? layout->count() : 0;
    
    // ステータス表示を簡素化（タスクバー風に）
    setWindowTitle(QString("TaskBarEx - %1 apps").arg(count));
}

void TaskbarWindow::onItemClicked(QListWidgetItem *item)
{
    if (!item) return;
    
    HWND hwnd = reinterpret_cast<HWND>(item->data(Qt::UserRole).toULongLong());
    m_model->activateWindow(hwnd);
}

void TaskbarWindow::onRefreshTriggered()
{
    updateTaskbarItems();
}

void TaskbarWindow::onAlwaysOnTopTriggered(bool checked)
{
    Qt::WindowFlags flags = windowFlags();
    if (checked) {
        flags |= Qt::WindowStaysOnTopHint;
    } else {
        flags &= ~Qt::WindowStaysOnTopHint;
    }
    setWindowFlags(flags);
    show(); // フラグ変更後は再表示が必要
}

void TaskbarWindow::onAboutTriggered()
{
    QMessageBox::about(this, "TaskBarExについて",
        "TaskBarEx v1.0\n\n"
        "Windows 11のタスクバー表示を補完する\n"
        "タスクバー拡張ツールです。\n\n"
        "実行中のアプリケーション一覧を表示し、\n"
        "クリックでウィンドウをアクティブ化できます。");
}

void TaskbarWindow::onTaskbarVisibilityChanged(bool visible)
{
    LOG_INFO(QString("🔔 SIGNAL RECEIVED: onTaskbarVisibilityChanged(%1)")
             .arg(visible ? "visible" : "hidden"));
    LOG_INFO("Adjusting TaskBarEx window visibility based on new state...");
    
    // 位置は固定なので、visibilityのみ更新（頻繁な位置調整を避ける）
    updateVisibilityBasedOnState();
}

void TaskbarWindow::onApplicationFocusChanged(QWidget *old, QWidget *now)
{
    Q_UNUSED(old);
    
    LOG_INFO(QString("🔀 FOCUS CHANGED: Focus moved to %1")
             .arg(now ? now->objectName() : "null"));
    
    // TaskBarEx自体がフォーカスされた場合は何もしない
    if (now && (now == this || now->window() == this)) {
        LOG_INFO("Focus is on TaskBarEx - keeping visible");
        return;
    }
    
    // TaskBarEx以外のアプリケーションがフォーカスされた場合
    if (now && now->window() != this) {
        LOG_INFO("Focus moved to external application - updating visibility based on state...");
        updateVisibilityBasedOnState();
    }
}

void TaskbarWindow::setupWindowFocusHook()
{
    LOG_INFO("Setting up Windows focus hook...");
    
    // SetWinEventHook を使ってシステムレベルのフォーカス変更を監視
    m_focusHook = SetWinEventHook(
        EVENT_SYSTEM_FOREGROUND,     // 最小イベント
        EVENT_SYSTEM_FOREGROUND,     // 最大イベント  
        NULL,                        // モジュールハンドル
        WinEventProc,                // コールバック関数
        0,                          // プロセスID（全プロセス）
        0,                          // スレッドID（全スレッド）
        WINEVENT_OUTOFCONTEXT       // フラグ
    );
    
    if (m_focusHook) {
        LOG_INFO("✅ Windows focus hook set up successfully");
    } else {
        LOG_ERROR("❌ Failed to set up Windows focus hook");
    }
}

void TaskbarWindow::cleanupWindowFocusHook()
{
    if (m_focusHook) {
        LOG_INFO("Cleaning up Windows focus hook...");
        UnhookWinEvent(m_focusHook);
        m_focusHook = nullptr;
        LOG_INFO("✅ Windows focus hook cleaned up");
    }
}

void CALLBACK TaskbarWindow::WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event,
                                          HWND hwnd, LONG idObject, LONG idChild,
                                          DWORD dwEventThread, DWORD dwmsEventTime)
{
    Q_UNUSED(hWinEventHook);
    Q_UNUSED(idObject);
    Q_UNUSED(idChild);
    Q_UNUSED(dwEventThread);
    Q_UNUSED(dwmsEventTime);
    
    // EVENT_SYSTEM_FOREGROUND イベントのみ処理
    if (event == EVENT_SYSTEM_FOREGROUND && hwnd && s_instance) {
        // Qt のメタシステムを使ってメインスレッドで処理
        QMetaObject::invokeMethod(s_instance, "onWindowFocusChanged", 
                                  Qt::QueuedConnection, Q_ARG(HWND, hwnd));
    }
}

void TaskbarWindow::onWindowFocusChanged(HWND hwnd)
{
    // ウィンドウタイトルを取得
    wchar_t windowTitle[256];
    GetWindowTextW(hwnd, windowTitle, sizeof(windowTitle) / sizeof(wchar_t));
    QString title = QString::fromWCharArray(windowTitle);
    
    LOG_INFO(QString("🔀 SYSTEM FOCUS CHANGED: Window '%1' (HWND: 0x%2)")
             .arg(title).arg(reinterpret_cast<quintptr>(hwnd), 0, 16));
    
    // TaskBarEx自体のウィンドウかチェック
    HWND taskbarExHwnd = reinterpret_cast<HWND>(winId());
    if (hwnd == taskbarExHwnd) {
        LOG_INFO("Focus is on TaskBarEx - keeping visible");
        return;
    }
    
    // TaskBarEx以外のアプリケーションがフォーカスされた場合
    LOG_INFO("Focus moved to external application - updating visibility based on state...");
    updateVisibilityBasedOnState();
}

// マウストラッキング機能の実装
void TaskbarWindow::setupMouseTracking()
{
    LOG_INFO("Setting up mouse tracking...");
    
    // マウストラッキングを有効化
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover, true);
    
    // マウス位置チェック用のタイマーをセットアップ
    m_mouseTrackTimer->setInterval(100); // 100ms間隔でチェック
    connect(m_mouseTrackTimer, &QTimer::timeout, this, &TaskbarWindow::onMouseTrackCheck);
    m_mouseTrackTimer->start();
    
    // 表示ディレイタイマーのセットアップ
    m_showDelayTimer->setSingleShot(true); // ワンショットタイマー
    m_showDelayTimer->setInterval(300); // 300msディレイ
    connect(m_showDelayTimer, &QTimer::timeout, this, &TaskbarWindow::onShowDelayTimeout);
    
    LOG_INFO("✅ Mouse tracking and show delay timers set up successfully");
}

void TaskbarWindow::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event);
    LOG_INFO("🖱️ MOUSE ENTERED: TaskBarEx window");
    m_isMouseOver = true;
    updateVisibilityBasedOnState();
}

void TaskbarWindow::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    LOG_INFO("🖱️ MOUSE LEFT: TaskBarEx window");
    m_isMouseOver = false;
    updateVisibilityBasedOnState();
}

void TaskbarWindow::onMouseTrackCheck()
{
    // グローバルマウス位置を取得
    QPoint globalPos = QCursor::pos();
    QPoint localPos = mapFromGlobal(globalPos);
    
    // TaskBarEx内にマウスがあるかチェック
    bool mouseInTaskBarEx = rect().contains(localPos);
    
    // タスクバー領域および直上80pxでマウス検出を有効化
    bool mouseInTaskbar = false;
    QScreen *screen = QApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->geometry();
        // 実際のタスクバー位置を取得して、その周辺を検出範囲に設定
        RECT taskbarRect;
        HWND taskbarHwnd = FindWindow(L"Shell_TrayWnd", nullptr);
        int taskbarDetectionStart = m_fixedTaskbarTop - 80; // 固定タスクバー位置より80px上から検出（適切な範囲）
        
        if (taskbarHwnd && GetWindowRect(taskbarHwnd, &taskbarRect)) {
            // 実際のタスクバー位置より80px上から検出（タスクバー周辺のみ）
            taskbarDetectionStart = std::min((int)taskbarRect.top - 80, m_fixedTaskbarTop - 80);
        }

        mouseInTaskbar = (globalPos.x() >= screenGeometry.left() && 
                         globalPos.x() <= screenGeometry.right() &&
                         globalPos.y() >= taskbarDetectionStart);
        
        // デバッグ用：マウス位置の詳細情報（一時的に頻繁に出力）
        static int debugCounter = 0;
        if (debugCounter++ % 5 == 0) {  // 5回に1回ログ出力（問題特定のため）
            LOG_INFO(QString("🖱️ DEBUG: Mouse at %1,%2, TaskbarStart>=%3, InTaskbar=%4, InTaskBarEx=%5, Screen=%6x%7")
                     .arg(globalPos.x()).arg(globalPos.y())
                     .arg(taskbarDetectionStart).arg(mouseInTaskbar).arg(mouseInTaskBarEx)
                     .arg(screenGeometry.width()).arg(screenGeometry.height()));
        }
    }
    
    bool newMouseOverState = mouseInTaskBarEx || mouseInTaskbar;
    
    if (newMouseOverState != m_isMouseOver) {
        m_isMouseOver = newMouseOverState;
        QString location = mouseInTaskBarEx ? "TaskBarEx" : (mouseInTaskbar ? "Taskbar-Bottom" : "Away");
        LOG_INFO(QString("🖱️ MOUSE STATE CHANGED: %1 (location: %2)")
                 .arg(m_isMouseOver ? "OVER" : "AWAY").arg(location));
        updateVisibilityBasedOnState();
    }
}

void TaskbarWindow::updateVisibilityBasedOnState()
{
    bool taskbarVisible = m_visibilityMonitor->isTaskbarVisible();
    
    LOG_INFO(QString("🔄 STATE CHECK: Taskbar=%1, MouseOver=%2 (isVisible=%3)")
             .arg(taskbarVisible ? "visible" : "hidden")
             .arg(m_isMouseOver ? "yes" : "no")
             .arg(isVisible() ? "yes" : "no"));
    
    // ★修正ロジック: タスクバー表示時でもマウス位置を考慮
    if (taskbarVisible) {
        if (!isVisible()) {
            // タスクバー表示時：マウスがタスクバー範囲内の場合のみ表示
            if (m_isMouseOver) {
                LOG_INFO(QString("🚀 TASKBAR APPEARED - マウスオーバーのため即座に表示"));
                show();
            } else {
                LOG_INFO(QString("⏸️ TASKBAR APPEARED - マウス範囲外のため表示しない"));
            }
        } else {
            // 既に表示中：マウス位置を確認
            if (!m_isMouseOver) {
                LOG_INFO(QString("🔴 HIDING TaskBarEx (taskbar visible but mouse away)"));
                hide();
            } else {
                LOG_INFO(QString("✅ TaskBarEx already visible (taskbar visible + mouse over)"));
            }
        }
        // 非表示タイマーがあればキャンセル
        m_showDelayTimer->stop();
    } else {
        // タスクバー非表示時：ディレイ後に非表示
        if (isVisible()) {
            LOG_INFO(QString("⏱️ TASKBAR HIDDEN - starting hide delay timer"));
            m_showDelayTimer->start(); // 300msディレイ後に非表示
        } else {
            LOG_INFO(QString("TaskBarEx already hidden (taskbar hidden)"));
        }
    }
}

void TaskbarWindow::onShowDelayTimeout()
{
    // ★変更：このタイマーは非表示処理用に変更
    bool taskbarVisible = m_visibilityMonitor->isTaskbarVisible();
    
    LOG_INFO(QString("⏰ HIDE DELAY TIMEOUT - タスクバー状態再確認: %1")
             .arg(taskbarVisible ? "visible" : "hidden"));
    
    if (taskbarVisible) {
        LOG_INFO(QString("❌ HIDE CANCELLED: タスクバーが再表示されたため非表示をキャンセル"));
        return; // タスクバーが再表示された場合は非表示しない
    }
    
    LOG_INFO(QString("🔴 HIDE CONFIRMED: TaskBarExを非表示"));
    hide();
}