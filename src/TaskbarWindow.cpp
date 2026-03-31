#include "TaskbarWindow.h"
#include "ui_TaskbarWindow.h"
#include "TaskbarModel.h"
#include "TaskbarGroupManager.h"
#include "PinnedAppsManager.h"
// TaskbarVisibilityMonitor削除済み
#include "Logger.h"
#include <QListWidgetItem>
#include <QPixmap>
#include <QApplication>
#include <QScreen>
#include <QMessageBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QProcess>
#include <QBuffer>
#include <QEvent>
#include <QHoverEvent>
#include <windows.h>

// WindowInfoをQVariantで使用できるように登録
Q_DECLARE_METATYPE(WindowInfo)

// 静的メンバーの初期化
TaskbarWindow* TaskbarWindow::s_instance = nullptr;

TaskbarWindow::TaskbarWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::TaskbarWindow)  // UIファイルを再有効化
    , m_updateTimer(new QTimer(this))
    , m_model(new TaskbarModel(this))
    , m_groupManager(new TaskbarGroupManager(this))
    , m_pinnedManager(new PinnedAppsManager(this))  // ピン留めアプリ管理初期化
    , m_thumbnailManager(new WindowThumbnailManager(this))  // サムネイル管理初期化
    , m_thumbnailPreview(new ThumbnailPreviewWidget(nullptr))  // サムネイルプレビュー初期化
    , m_mouseTrackTimer(new QTimer(this))
    , m_hideDelayTimer(new QTimer(this))  // 非表示ディレイタイマー初期化
    , m_thumbnailDelayTimer(new QTimer(this))  // サムネイル表示ディレイタイマー初期化
    , m_screenHeight(0)
    , m_taskbarHeight(48)  // 標準タスクバー高さ
    , m_appBarHeight(48)   // アプリバー高さ
    , m_runningAppsRow(nullptr)
    , m_pinnedAppsRow(nullptr)
    , m_runningLayout(nullptr)
    , m_pinnedLayout(nullptr)
    , m_hoverButton(nullptr)
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
        
        // メニューバーとステータスバーを完全に無効化してスペースを確保
        menuBar()->setVisible(false);
        if (statusBar()) {
            statusBar()->setVisible(false);
            statusBar()->setSizeGripEnabled(false);
        }
        
        // 高さを固定（後でpositionAboveTaskbar()で位置設定）
        setFixedHeight(m_appBarHeight);
        LOG_INFO(QString("🔍 Height set to: %1").arg(m_appBarHeight));
        
        // centralwidgetのマージンを最小化
        if (centralWidget() && centralWidget()->layout()) {
            centralWidget()->layout()->setContentsMargins(0, 0, 0, 0);
            centralWidget()->layout()->setSpacing(0);
        }
        
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
        LOG_INFO("Step 2.5: Setting up two-row layout...");
        setupTwoRowLayout(); // 2段レイアウト設定
        LOG_INFO("Step 2.5 completed (two-row layout enabled)");
    } catch (...) {
        LOG_ERROR("Step 2.5 (setupTwoRowLayout) failed");
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
        LOG_INFO("Step 6: Setting up mouse tracking...");
        setupMouseTracking();
        LOG_INFO("Step 6 completed (mouse tracking)");
    } catch (...) {
        LOG_ERROR("Step 6 (setupMouseTracking) failed");
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
    // 静的インスタンスをリセット
    s_instance = nullptr;
    
    delete ui;
}

void TaskbarWindow::positionAboveTaskbar()
{
    QScreen *screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    
    // 画面情報を保存
    m_screenHeight = screenGeometry.height();
    
    LOG_INFO(QString("画面情報: %1x%2 (タスクバー高さ:%3, アプリバー高さ:%4)")
             .arg(screenGeometry.width()).arg(m_screenHeight)
             .arg(m_taskbarHeight).arg(m_appBarHeight));
    
    // アプリバーを画面下端に配置（タスクバー直上）
    int windowY = m_screenHeight - m_taskbarHeight - m_appBarHeight;
    int windowX = screenGeometry.left();
    int windowWidth = screenGeometry.width();
    
    LOG_INFO(QString("アプリバー位置: %1,%2 %3x%4")
             .arg(windowX).arg(windowY).arg(windowWidth).arg(m_appBarHeight));
    
    setGeometry(windowX, windowY, windowWidth, m_appBarHeight);
    
    // 実際に設定されたサイズを確認
    QTimer::singleShot(100, this, [this]() {
        QRect actualGeometry = geometry();
        LOG_INFO(QString("🔍 実際のアプリバー位置: %1,%2 %3x%4")
                 .arg(actualGeometry.x()).arg(actualGeometry.y())
                 .arg(actualGeometry.width()).arg(actualGeometry.height()));
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
    
    // サムネイルクリック時のフォーカス処理
    connect(m_thumbnailPreview, &ThumbnailPreviewWidget::thumbnailClicked,
            this, &TaskbarWindow::onThumbnailClicked);
    
    // タスクバー監視機能は削除済み
}

void TaskbarWindow::setupTimer()
{
    m_updateTimer->setInterval(1000); // 1秒間隔
    connect(m_updateTimer, &QTimer::timeout, 
            this, &TaskbarWindow::updateTaskbarItems);
            
    // サムネイル表示ディレイタイマーのセットアップ（応答性向上）
    m_thumbnailDelayTimer->setSingleShot(true);
    m_thumbnailDelayTimer->setInterval(300);  // 300ms ディレイ（応答性向上）
    connect(m_thumbnailDelayTimer, &QTimer::timeout,
            this, &TaskbarWindow::onThumbnailDelayTimeout);
    
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
    LOG_INFO("Starting populateTaskbarList (2段レイアウト対応)...");
    
    // 2段レイアウトが設定されているかチェック
    if (!m_runningLayout) {
        LOG_ERROR("❌ 起動中アプリレイアウトが初期化されていません");
        return;
    }
    
    try {
        LOG_INFO("Step 1: 2段レイアウトコンポーネント確認...");
        LOG_INFO("✅ 起動中アプリレイアウト使用可能");
    } catch (...) {
        LOG_ERROR("Exception in 2段レイアウト component check");
        return;
    }
    
    try {
        LOG_INFO("Step 2: 起動中アプリボタンをクリア...");
        clearRunningAppButtons();
        LOG_INFO("起動中アプリボタンクリア完了");
    } catch (...) {
        LOG_ERROR("Exception in clearing running app buttons");
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
        LOG_INFO("Step 5: Creating running app buttons (2段レイアウト)...");
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
                    createRunningAppButton(windowInfo);
                    buttonCount++;
                    LOG_INFO(QString("    Button created successfully for: %1").arg(windowInfo.title));
                } catch (...) {
                    LOG_ERROR(QString("    Exception creating button for: %1").arg(windowInfo.title));
                }
            }
        }
        LOG_INFO(QString("Created %1 running app buttons").arg(buttonCount));
        
        // 右端にスペーサー追加
        m_runningLayout->addStretch();
        
    } catch (...) {
        LOG_ERROR("Exception in creating running app buttons");
        return;
    }
    
    // デバッグ用：最終的なレイアウトの状態をチェック
    LOG_INFO(QString("Final 2段レイアウト state - Running apps count: %1").arg(m_runningLayout->count()));
    
    // アプリバー高さは2段レイアウトで86pxに固定済み（updateAppBarHeight呼び出し削除）
    // updateAppBarHeight();
    
    LOG_INFO("✅ populateTaskbarList (2段レイアウト対応) 完了");
}

void TaskbarWindow::clearTaskbarButtons()
{
    // 2段レイアウト用のclearRunningAppButtons()に委譲
    clearRunningAppButtons();
}

void TaskbarWindow::createTaskbarButton(const WindowInfo& window)
{
    // 2段レイアウト用のcreateRunningAppButton()に委譲
    createRunningAppButton(window);
}

void TaskbarWindow::updateStatusBar()
{
    // 2段レイアウト対応のステータスバー更新
    int runningCount = m_runningLayout ? m_runningLayout->count() - 1 : 0; // ラベル分を引く
    int pinnedCount = m_pinnedLayout ? m_pinnedLayout->count() - 1 : 0;   // ラベル分を引く
    
    // ステータス表示を簡素化（タスクバー風に）
    setWindowTitle(QString("TaskBarEx - %1 apps, %2 pinned").arg(runningCount).arg(pinnedCount));
    
    LOG_INFO(QString("📊 ステータス更新: 起動中=%1, ピン留め=%2").arg(runningCount).arg(pinnedCount));
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

// onTaskbarVisibilityChanged関数は削除済み（マウス座標ベースに変更）

// onApplicationFocusChanged関数は削除済み（マウス座標ベースに変更）

// setupWindowFocusHook関数は削除済み（マウス座標ベースに変更）

// cleanupWindowFocusHook関数は削除済み（マウス座標ベースに変更）

// WinEventProc関数は削除済み（マウス座標ベースに変更）

// onWindowFocusChanged関数は削除済み（マウス座標ベースに変更）

// マウストラッキング機能の実装
void TaskbarWindow::setupMouseTracking()
{
    LOG_INFO("マウス座標ベース表示制御を開始...");
    
    // 画面情報をログ出力
    LOG_INFO(QString("画面高さ: %1, タスクバー高さ: %2, アプリバー高さ: %3")
             .arg(m_screenHeight).arg(m_taskbarHeight).arg(m_appBarHeight));
    
    // マウス位置チェックタイマー設定
    m_mouseTrackTimer->setInterval(50); // 50ms間隔でチェック
    connect(m_mouseTrackTimer, &QTimer::timeout, this, &TaskbarWindow::checkMousePosition);
    m_mouseTrackTimer->start();
    LOG_INFO("🕒 マウストラッキングタイマー開始（50ms間隔）");
    
    // 非表示ディレイタイマー設定
    m_hideDelayTimer->setSingleShot(true); // 一回のみ実行
    m_hideDelayTimer->setInterval(500); // 0.5秒ディレイ
    connect(m_hideDelayTimer, &QTimer::timeout, this, [this]() {
        LOG_INFO("⏱️ 非表示ディレイ完了 - TaskBarExを非表示");
        hide();
    });
    LOG_INFO("⏱️ 非表示ディレイタイマー設定完了（0.5秒ディレイ）");
    
    // 初期マウス位置をチェック
    QPoint initialPos = QCursor::pos();
    LOG_INFO(QString("🖱️ 初期マウス位置: %1,%2").arg(initialPos.x()).arg(initialPos.y()));
    
    // 初期状態は非表示
    hide();
    LOG_INFO("👻 初期状態：ウィンドウを非表示に設定");
    
    LOG_INFO("✅ マウス座標ベース表示制御を設定完了");
}

void TaskbarWindow::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event);
    // イベント処理は無効（マウス座標チェックのcheckMousePositionで処理）
}

void TaskbarWindow::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    // イベント処理は無効（マウス座標チェックのcheckMousePositionで処理）
}

// タスクバー表示状態検出（フルスクリーンゲーム対応）
bool TaskbarWindow::isTaskbarVisible()
{
    HWND taskbarHandle = FindWindow(L"Shell_TrayWnd", nullptr);
    if (!taskbarHandle) {
        LOG_INFO("⚠️ タスクバーハンドルが見つかりません - デフォルトで非表示を返します");
        return false; // タスクバーが見つからない場合は非表示とする
    }
    
    // タスクバーの位置を取得
    RECT taskbarRect;
    if (!GetWindowRect(taskbarHandle, &taskbarRect)) {
        return false; // 位置取得失敗時は非表示とする
    }
    
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    
    // フルスクリーンアプリ検出
    HWND foregroundWindow = GetForegroundWindow();
    bool isFullscreenApp = false;
    
    if (foregroundWindow) {
        RECT foregroundRect;
        if (GetWindowRect(foregroundWindow, &foregroundRect)) {
            // フォアグラウンドウィンドウがフルスクリーンかチェック
            // 最大化ウィンドウ(-8px境界)とフルスクリーンアプリ(0px境界)を区別
            isFullscreenApp = (foregroundRect.left == 0 && 
                             foregroundRect.top == 0 && 
                             foregroundRect.right == GetSystemMetrics(SM_CXSCREEN) && 
                             foregroundRect.bottom == screenHeight);
            
            // フォアグラウンドウィンドウのタイトル取得（デバッグ用）
            wchar_t windowTitle[256];
            GetWindowText(foregroundWindow, windowTitle, sizeof(windowTitle)/sizeof(wchar_t));
            QString title = QString::fromWCharArray(windowTitle);
            
            // デバッグログ（頻度制限）
            static int fgDebugCounter = 0;
            if (fgDebugCounter++ % 30 == 0) { // 30回に1回ログ出力
                LOG_INFO(QString("🖼️ フォアグラウンド: %1, Rect=(%2,%3,%4,%5), フルスクリーン判定=%6")
                         .arg(title)
                         .arg(foregroundRect.left).arg(foregroundRect.top)
                         .arg(foregroundRect.right).arg(foregroundRect.bottom)
                         .arg(isFullscreenApp ? "true" : "false"));
            }
        }
    }
    
    // タスクバーの実際の可視性判定
    bool isWindowVisible = IsWindowVisible(taskbarHandle);
    bool isInScreenArea = (taskbarRect.top < screenHeight) && (taskbarRect.bottom > screenHeight - 50);
    
    // フルスクリーンアプリが起動中の場合は、タスクバーを非表示とみなす
    bool actuallyVisible = isWindowVisible && isInScreenArea && !isFullscreenApp;
    
    // デバッグログ（頻度制限）
    static int debugCounter = 0;
    if (debugCounter++ % 50 == 0) { // 50回に1回ログ出力
        LOG_INFO(QString("🔍 タスクバー詳細: top=%1, bottom=%2, screenH=%3, WindowVisible=%4, フルスクリーン=%5, 結果=%6")
                 .arg(taskbarRect.top)
                 .arg(taskbarRect.bottom)
                 .arg(screenHeight)
                 .arg(isWindowVisible ? "true" : "false")
                 .arg(isFullscreenApp ? "true" : "false")
                 .arg(actuallyVisible ? "true" : "false"));
    }
    
    return actuallyVisible;
}

// 新しいマウス座標ベース表示制御（タスクバー表示状態をAND条件で追加）
void TaskbarWindow::checkMousePosition()
{
    // タイマー動作の確認（最初の数回のみログ出力）
    static int timerCounter = 0;
    timerCounter++;
    if (timerCounter <= 5) {
        LOG_INFO(QString("⏰ checkMousePosition() 呼び出し回数: %1").arg(timerCounter));
    }
    
    // フルスクリーンアプリ検出（タスクバー抑止状態の確認）
    bool taskbarSuppressed = !isTaskbarVisible();
    
    // グローバルマウス位置を取得
    QPoint globalPos = QCursor::pos();
    int mouseY = globalPos.y();
    
    // 新マウス座標ベース表示制御仕様（フルスクリーンゲーム対応）
    // 表示判定：画面下端から10px以内 AND タスクバーが抑止されていない
    int showThreshold = m_screenHeight - 10;
    bool shouldShow = (mouseY >= showThreshold) && !taskbarSuppressed;
    
    // 非表示判定：アプリバー上端より上 OR タスクバーが抑止されている
    int hideThreshold = m_screenHeight - m_taskbarHeight - m_appBarHeight;  // 動的計算
    bool shouldHide = (mouseY < hideThreshold) || taskbarSuppressed;
    
    // ログ出力（デバッグ用、頻度制限）
    static int logCounter = 0;
    if (logCounter++ % 20 == 0) {  // 20回に1回ログ出力
        LOG_INFO(QString("🖱️ マウス座標Y=%1, 表示閾値=%2, 非表示閾値=%3, タスクバー抑止=%4, shouldShow=%5, shouldHide=%6")
                 .arg(mouseY)
                 .arg(showThreshold)
                 .arg(hideThreshold)
                 .arg(taskbarSuppressed ? "true" : "false")
                 .arg(shouldShow ? "true" : "false")
                 .arg(shouldHide ? "true" : "false"));
    }
    
    // 表示制御
    if (shouldShow && !isVisible()) {
        // 非表示ディレイタイマーが動作中なら停止
        if (m_hideDelayTimer->isActive()) {
            m_hideDelayTimer->stop();
            LOG_INFO("⏹️ 非表示ディレイタイマー停止（表示条件が満たされました）");
        }
        LOG_INFO(QString("🚀 SHOW: マウス画面下端到達 (マウスY=%1, 表示閾値=%2, タスクバー抑止=%3)")
                 .arg(mouseY).arg(showThreshold).arg(taskbarSuppressed ? "false" : "true"));
        show();
    } else if (shouldHide && isVisible()) {
        // 非表示ディレイタイマーがまだ動作していない場合のみ開始
        if (!m_hideDelayTimer->isActive()) {
            QString reason = taskbarSuppressed ? "タスクバー抑止" : "マウス上部移動";
            LOG_INFO(QString("⏱️ HIDE DELAY START: %1のため0.5秒後に非表示 (マウスY=%2, 非表示閾値=%3)")
                     .arg(reason).arg(mouseY).arg(hideThreshold));
            m_hideDelayTimer->start();
        }
    } else if (!shouldHide && isVisible() && m_hideDelayTimer->isActive()) {
        // 非表示条件が解除された場合、ディレイタイマーを停止
        m_hideDelayTimer->stop();
        LOG_INFO(QString("⏹️ HIDE DELAY CANCELED: 非表示条件が解除 (マウスY=%1, 非表示閾値=%2)")
                 .arg(mouseY).arg(hideThreshold));
    }
}

// 古いupdateVisibilityBasedOnState関数は削除済み（マウス座標ベースに変更）

// onShowDelayTimeout関数は削除済み（マウス座標ベースに変更）
void TaskbarWindow::onShowDelayTimeout()
{
    // 使用しない（互換性維持のためスロットのみ残す）
}

// onHideDelayTimeout関数は削除済み（マウス座標ベースに変更）
void TaskbarWindow::onHideDelayTimeout()
{
    // 使用しない（互換性維持のためスロットのみ残す）
}

// ========== 2段表示システム実装 ==========

void TaskbarWindow::setupTwoRowLayout()
{
    LOG_INFO("🏗️ 2段レイアウト設定開始");
    
    // メインコンテナウィジェット取得
    QWidget *centralWidget = this->centralWidget();
    if (!centralWidget) {
        LOG_ERROR("❌ centralWidget が見つかりません");
        return;
    }
    
    // 既存のレイアウトを削除（もしあれば）
    if (centralWidget->layout()) {
        QLayoutItem *item;
        while ((item = centralWidget->layout()->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }
        delete centralWidget->layout();
    }
    
    // メインの垂直レイアウト作成
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(2, 2, 2, 2);  // 小さなマージン
    mainLayout->setSpacing(1);  // 段間の境界線用スペース
    
    // === 上段: 起動中アプリケーション行 ===
    m_runningAppsRow = new QWidget();
    m_runningAppsRow->setFixedHeight(40);  // 1段の高さ
    m_runningAppsRow->setStyleSheet("background-color: rgba(40, 40, 40, 220); border-radius: 3px;");
    
    m_runningLayout = new QHBoxLayout(m_runningAppsRow);
    m_runningLayout->setContentsMargins(4, 2, 4, 2);
    m_runningLayout->setSpacing(2);
    m_runningLayout->setAlignment(Qt::AlignLeft);
    
    // ラベル追加（デバッグ用）
    QLabel *runningLabel = new QLabel("起動中:");
    runningLabel->setObjectName("runningLabel");  // オブジェクト名設定
    runningLabel->setStyleSheet("color: white; font-size: 10px; font-weight: bold;");
    runningLabel->setFixedWidth(45);
    m_runningLayout->addWidget(runningLabel);
    
    // === 下段: ピン留めアプリケーション行 ===
    m_pinnedAppsRow = new QWidget();
    m_pinnedAppsRow->setFixedHeight(40);  // 1段の高さ
    m_pinnedAppsRow->setStyleSheet("background-color: rgba(60, 60, 60, 200); border-radius: 3px;");
    
    m_pinnedLayout = new QHBoxLayout(m_pinnedAppsRow);
    m_pinnedLayout->setContentsMargins(4, 2, 4, 2);
    m_pinnedLayout->setSpacing(2);
    m_pinnedLayout->setAlignment(Qt::AlignLeft);
    
    // ラベル追加（デバッグ用）
    QLabel *pinnedLabel = new QLabel("ピン留め:");
    pinnedLabel->setObjectName("pinnedLabel");  // オブジェクト名設定
    pinnedLabel->setStyleSheet("color: white; font-size: 10px; font-weight: bold;");
    pinnedLabel->setFixedWidth(45);
    m_pinnedLayout->addWidget(pinnedLabel);
    
    // メインレイアウトに行を追加
    mainLayout->addWidget(m_runningAppsRow);
    mainLayout->addWidget(m_pinnedAppsRow);
    
    // 初期のアプリバー高さ更新（2段分）
    m_appBarHeight = 86;  // 40px × 2 + マージン・境界線
    setFixedHeight(m_appBarHeight);
    
    LOG_INFO(QString("✅ 2段レイアウト設定完了 - 高さ: %1px").arg(m_appBarHeight));
    
    // ピン留めアプリマネージャー接続（再有効化）
    connect(m_pinnedManager, &PinnedAppsManager::pinnedAppsChanged, 
            this, &TaskbarWindow::populatePinnedAppsRow);
    
    // 初期のピン留めアプリ表示（再有効化）
    populatePinnedAppsRow();
    
    // ピン留めアプリの自動更新開始（5秒間隔）
    m_pinnedManager->startAutoRefresh(5000);
    LOG_INFO("🔄 ピン留めアプリ自動更新開始（5秒間隔）");
    
    // マウス閾値も更新
    updateMouseThresholds();
}

void TaskbarWindow::populatePinnedAppsRow()
{
    LOG_INFO("📌 ピン留めアプリ行を更新開始");
    
    if (!m_pinnedLayout) {
        LOG_ERROR("❌ ピン留めレイアウトが初期化されていません");
        return;
    }
    
    // 既存のピン留めアイコンをクリア（ラベル以外）
    for (int i = m_pinnedLayout->count() - 1; i >= 0; --i) {
        QLayoutItem *item = m_pinnedLayout->itemAt(i);
        if (item && item->widget()) {
            QWidget *widget = item->widget();
            // ラベル（"ピン留め:"）以外を削除（QPushButton、QLabel等）
            if (widget->objectName() != "pinnedLabel") {
                m_pinnedLayout->removeWidget(widget);
                widget->deleteLater();
            }
        } else if (item && !item->widget()) {
            // スペーサー（QSpacerItem）も削除
            m_pinnedLayout->removeItem(item);
            delete item;
        }
    }
    
    // ピン留めアプリを追加
    QList<PinnedAppInfo> pinnedApps = m_pinnedManager->getPinnedApps();
    LOG_INFO(QString("📌 ピン留めアプリ数: %1").arg(pinnedApps.size()));
    
    for (const PinnedAppInfo &appInfo : pinnedApps) {
        if (!appInfo.isValid) {
            continue;
        }
        
        // ピン留めアプリボタン作成（クリック可能なQPushButton）
        QPushButton *pinnedButton = new QPushButton();
        pinnedButton->setFixedSize(36, 36);
        pinnedButton->setFlat(true);  // フラットボタン
        pinnedButton->setStyleSheet(
            "QPushButton {"
            "    background-color: rgba(80, 80, 80, 150);"
            "    border-radius: 4px;"
            "    border: 1px solid rgba(100, 100, 100, 100);"
            "    color: white;"
            "    font-size: 10px;"
            "    font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "    background-color: rgba(100, 100, 100, 180);"
            "    border: 1px solid rgba(140, 140, 140, 180);"
            "}"
            "QPushButton:pressed {"
            "    background-color: rgba(0, 120, 215, 180);"
            "    border: 1px solid rgba(0, 120, 215, 255);"
            "}"
        );
        
        if (!appInfo.icon.isNull()) {
            QIcon icon(appInfo.icon);
            pinnedButton->setIcon(icon);
            pinnedButton->setIconSize(QSize(32, 32));
            pinnedButton->setToolTip(appInfo.name);
        } else {
            // アイコンがない場合はテキスト表示
            pinnedButton->setText(appInfo.name.left(2).toUpper());
            pinnedButton->setToolTip(appInfo.name);
        }
        
        // 実行可能ファイルパスを保存してクリックイベント設定
        pinnedButton->setProperty("executablePath", appInfo.executablePath);
        connect(pinnedButton, &QPushButton::clicked, this, [this, pinnedButton]() {
            QString execPath = pinnedButton->property("executablePath").toString();
            if (!execPath.isEmpty()) {
                // 実行可能ファイルを起動
                QProcess::startDetached(execPath);
                LOG_INFO(QString("📌 ピン留めアプリを起動: %1").arg(execPath));
            }
        });
        
        m_pinnedLayout->addWidget(pinnedButton);
        
        LOG_INFO(QString("📌 ピン留めアイコン追加: %1").arg(appInfo.name));
    }
    
    // 右端にスペーサー追加
    m_pinnedLayout->addStretch();
    
    LOG_INFO("✅ ピン留めアプリ行更新完了");
}

void TaskbarWindow::updateAppBarHeight()
{
    // 動的な高さ計算を一時的に無効化（固定高さに戻す）
    int newHeight = 48;  // 固定1段高さ
    
    if (newHeight != m_appBarHeight) {
        m_appBarHeight = newHeight;
        setFixedHeight(m_appBarHeight);
        updateMouseThresholds();
        
        LOG_INFO(QString("📏 アプリバー高さ固定: %1px").arg(m_appBarHeight));
    }
}

void TaskbarWindow::updateMouseThresholds()
{
    // マウス座標閾値（動的計算）
    if (m_screenHeight > 0) {
        int hideThreshold = m_screenHeight - m_taskbarHeight - m_appBarHeight;
        int showThreshold = m_screenHeight - 10;
        
        LOG_INFO(QString("🖱️ マウス閾値更新 - 非表示: Y<%1, 表示: Y>=%2 (アプリバー高さ:%3px)")
                 .arg(hideThreshold).arg(showThreshold).arg(m_appBarHeight));
    }
}

// ========== 起動中アプリ管理機能 ==========

void TaskbarWindow::clearRunningAppButtons()
{
    if (!m_runningLayout) {
        LOG_ERROR("❌ 起動中アプリレイアウトが初期化されていません");
        return;
    }
    
    LOG_INFO("🧹 起動中アプリボタンをクリア開始");
    
    // 既存のボタンをすべて削除（ラベル以外）
    for (int i = m_runningLayout->count() - 1; i >= 0; --i) {
        QLayoutItem *item = m_runningLayout->itemAt(i);
        if (item && item->widget()) {
            QWidget *widget = item->widget();
            // ラベル（"起動中:"）以外を削除（QPushButton、QLabel等）
            if (widget->objectName() != "runningLabel") {
                m_runningLayout->removeWidget(widget);
                widget->deleteLater();
            }
        } else if (item && !item->widget()) {
            // スペーサー（QSpacerItem）も削除
            m_runningLayout->removeItem(item);
            delete item;
        }
    }
    
    LOG_INFO("✅ 起動中アプリボタンクリア完了");
}

void TaskbarWindow::createRunningAppButton(const WindowInfo& window)
{
    if (!m_runningLayout) {
        LOG_ERROR("❌ 起動中アプリレイアウトが初期化されていません");
        return;
    }
    
    // 起動中アプリボタン作成（クリック可能なQPushButton）
    QPushButton *appButton = new QPushButton();
    appButton->setFixedSize(36, 36);
    appButton->setFlat(true);  // フラットボタン
    appButton->setStyleSheet(
        "QPushButton {"
        "    background-color: rgba(40, 40, 40, 180);"
        "    border-radius: 4px;"
        "    border: 1px solid rgba(80, 80, 80, 150);"
        "    color: white;"
        "    font-size: 10px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: rgba(60, 60, 60, 200);"
        "    border: 1px solid rgba(120, 120, 120, 180);"
        "}"
        "QPushButton:pressed {"
        "    background-color: rgba(0, 120, 215, 180);"
        "    border: 1px solid rgba(0, 120, 215, 255);"
        "}"
    );
    
    // ツールチップ用のタイトル準備（フォルダパス短縮）
    QString toolTipTitle = window.title;
    if (toolTipTitle.contains('\\') || toolTipTitle.contains('/')) {
        QFileInfo fileInfo(toolTipTitle);
        if (fileInfo.isAbsolute()) {
            QString shortTitle = fileInfo.fileName();
            if (shortTitle.isEmpty()) {
                shortTitle = fileInfo.absolutePath().split(QDir::separator()).last();
            }
            toolTipTitle = shortTitle;
        }
    }
    
    // アイコン設定
    if (!window.icon.isNull()) {
        QIcon icon(window.icon);
        appButton->setIcon(icon);
        appButton->setIconSize(QSize(32, 32));
        appButton->setToolTip(toolTipTitle);
    } else {
        // アイコンがない場合はテキスト表示
        QString displayText = window.title.left(3).toUpper();
        if (displayText.isEmpty()) {
            displayText = window.executablePath.split('/').last().left(3).toUpper();
        }
        appButton->setText(displayText);
        appButton->setToolTip(window.title.isEmpty() ? window.executablePath : toolTipTitle);
    }
    
    // HWNDをボタンに保存してクリックイベント設定
    appButton->setProperty("hwnd", reinterpret_cast<qulonglong>(window.hwnd));
    
    // WindowInfoをボタンに保存（ホバーイベント用）
    appButton->setProperty("windowInfo", QVariant::fromValue(window));
    
    // ホバーイベントを有効化
    appButton->setAttribute(Qt::WA_Hover, true);
    
    // イベントフィルターを追加してホバーイベントをキャッチ
    appButton->installEventFilter(this);
    
    connect(appButton, &QPushButton::clicked, this, [this, appButton]() {
        // ボタンクリック開始ログ
        QString buttonText = appButton->text();
        HWND hwnd = reinterpret_cast<HWND>(appButton->property("hwnd").toULongLong());
        LOG_INFO(QString("🖱️ ボタンクリック検出: '%1' (HWND=%2)").arg(buttonText).arg(reinterpret_cast<quintptr>(hwnd)));
        
        // サムネイル競合回避: クリック時に全てのサムネイル処理を即座停止
        if (m_thumbnailDelayTimer->isActive()) {
            m_thumbnailDelayTimer->stop();
            LOG_INFO("⚡ ボタンクリック - サムネイルディレイタイマー即座停止");
        }
        
        // サムネイルプレビューも即座に非表示
        if (m_thumbnailPreview && m_thumbnailPreview->isVisible()) {
            m_thumbnailPreview->hideThumbnail();
            LOG_INFO("⚡ ボタンクリック - サムネイルプレビュー即座非表示");
        }
        
        // ホバー状態をクリア
        m_hoverButton = nullptr;
        
        if (!hwnd) {
            LOG_WARNING("⚠️ 無効なHWND - ボタンクリック処理中断");
            return;
        }
        
        // TaskbarModel経由でアクティベート（詳細ログ付き）
        bool success = m_model->activateWindow(hwnd);
        if (success) {
            LOG_INFO("✅ ウィンドウアクティベート成功");
        } else {
            LOG_WARNING("❌ ウィンドウアクティベート失敗");
        }
    });
    
    // レイアウトに追加
    m_runningLayout->addWidget(appButton);
    
    LOG_INFO(QString("✅ 起動中アプリボタン追加: %1").arg(window.title.isEmpty() ? window.executablePath : window.title));
}

void TaskbarWindow::createGroupButton(const QVector<WindowInfo>& group)
{
    if (group.isEmpty()) return;
    
    // グループの代表ウィンドウ（最初のウィンドウ）を使用してボタンを作成
    const WindowInfo& representative = group[0];
    
    QPushButton *groupButton = new QPushButton(m_runningAppsRow);
    groupButton->setFixedSize(48, 48);
    groupButton->setFlat(true);
    groupButton->setStyleSheet(
        "QPushButton {"
        "    background-color: rgba(255, 255, 255, 10);"
        "    border: 1px solid rgba(255, 255, 255, 30);"
        "    border-radius: 4px;"
        "    margin: 1px;"
        "}"
        "QPushButton:hover {"
        "    background-color: rgba(255, 255, 255, 20);"
        "    border: 1px solid rgba(255, 255, 255, 50);"
        "}"
        "QPushButton:pressed {"
        "    background-color: rgba(255, 255, 255, 30);"
        "}"
    );
    
    // 代表ウィンドウのアイコンを使用
    if (!representative.icon.isNull()) {
        QIcon icon(representative.icon);
        groupButton->setIcon(icon);
        groupButton->setIconSize(QSize(32, 32));
        
        // グループサイズをツールチップに表示
        QString appName = representative.title.contains(" - ") ? 
            representative.title.split(" - ").last() : representative.title;
        groupButton->setToolTip(QString("%1 (%2個のウィンドウ)").arg(appName).arg(group.size()));
    }
    
    // グループ情報をボタンに保存
    QVariantList groupData;
    for (const auto& window : group) {
        groupData.append(QVariant::fromValue(window));
    }
    groupButton->setProperty("groupWindows", groupData);
    groupButton->setProperty("isGroupButton", true);
    
    // ホバーイベントを有効化
    groupButton->setAttribute(Qt::WA_Hover, true);
    groupButton->installEventFilter(this);
    
    // クリックイベント：グループ内ウィンドウのサムネイル一覧表示
    connect(groupButton, &QPushButton::clicked, this, [this, groupButton]() {
        QVariantList groupData = groupButton->property("groupWindows").toList();
        LOG_INFO(QString("🖱️ グループボタンクリック検出: %1個のウィンドウ").arg(groupData.size()));
        
        // TODO: ここでサムネイル一覧を表示する
        // 現在は最初のウィンドウをアクティベートする簡易実装
        if (!groupData.isEmpty()) {
            WindowInfo firstWindow = groupData[0].value<WindowInfo>();
            bool success = m_model->activateWindow(firstWindow.hwnd);
            LOG_INFO(QString("✅ グループの最初のウィンドウをアクティベート: %1").arg(firstWindow.title));
        }
    });
    
    // レイアウトに追加
    m_runningLayout->addWidget(groupButton);
    
    // グループボタンの視覚的な区別のためのマーカー（複数ウィンドウ表示）
    if (group.size() > 1) {
        // ボタンの右下に小さな数字バッジを表示（将来の実装）
    }
    
    LOG_INFO(QString("✅ グループボタン追加: %1 (%2個のウィンドウ)")
             .arg(representative.title.contains(" - ") ? 
                  representative.title.split(" - ").last() : representative.title)
             .arg(group.size()));
}

void TaskbarWindow::onButtonHoverEnter(QPushButton* button, const WindowInfo& windowInfo)
{
    LOG_INFO(QString("🖱️ ボタンホバー開始: %1").arg(windowInfo.title));
    
    // ホバー情報を保存してタイマーを開始
    m_hoverButton = button;
    m_hoverWindowInfo = windowInfo;
    
    // 既存のタイマーをリセット
    if (m_thumbnailDelayTimer->isActive()) {
        m_thumbnailDelayTimer->stop();
    }
    
    // 300ms後にサムネイル表示（応答性向上）
    m_thumbnailDelayTimer->start();
}

void TaskbarWindow::onButtonHoverLeave()
{
    LOG_INFO("🖱️ ボタンホバー終了");
    
    // ディレイタイマーを即座停止
    if (m_thumbnailDelayTimer->isActive()) {
        m_thumbnailDelayTimer->stop();
        LOG_INFO("⚡ ホバー離脱 - サムネイルディレイタイマー即座停止");
    }
    
    // カスタムサムネイルプレビューを即座非表示
    if (m_thumbnailPreview && m_thumbnailPreview->isVisible()) {
        m_thumbnailPreview->hideThumbnail();
        LOG_INFO("⚡ ホバー離脱 - サムネイルプレビュー即座非表示");
    }
    
    // ホバー情報をクリア
    m_hoverButton = nullptr;
}

void TaskbarWindow::onGroupButtonHoverEnter(QPushButton* button)
{
    if (!button) return;
    
    QVariantList groupData = button->property("groupWindows").toList();
    if (groupData.isEmpty()) {
        LOG_WARNING("⚠️ グループボタンにウィンドウデータが設定されていません");
        return;
    }
    
    // グループの最初のウィンドウを代表として使用
    WindowInfo representative = groupData[0].value<WindowInfo>();
    
    LOG_INFO(QString("🖱️ グループボタンホバー開始: %1 (%2個のウィンドウ)")
             .arg(representative.title).arg(groupData.size()));
    
    // 既存のタイマーをクリア
    if (m_thumbnailDelayTimer->isActive()) {
        m_thumbnailDelayTimer->stop();
        LOG_INFO("⏹️ 既存のサムネイルタイマーを停止");
    }
    
    // ホバー状態を設定
    m_hoverButton = button;
    m_hoverWindowInfo = representative;  // 代表ウィンドウ情報を保存
    
    // タイマー開始（グループの場合も同じディレイ）
    m_thumbnailDelayTimer->start();
    LOG_INFO("⏰ グループサムネイル表示タイマー開始");
}

void TaskbarWindow::onThumbnailDelayTimeout()
{
    if (!m_hoverButton) {
        LOG_WARNING("⚠️ タイマータイムアウト時にホバーボタンが無効");
        return;
    }
    
    // 通常ボタンの処理
    LOG_INFO(QString("⏰ サムネイル表示タイマータイムアウト: %1").arg(m_hoverWindowInfo.title));
    
    // サムネイル取得とカスタムプレビュー表示（高画質・大サイズ）
    QSize highQualitySize(450, 338); // 1.5倍サイズで高画質に（300*1.5=450, 225*1.5=337.5≒338）
    QPixmap thumbnail = m_thumbnailManager->captureWindowThumbnail(m_hoverWindowInfo.hwnd, highQualitySize);
    
    // ボタンの位置を取得してプレビュー表示位置を計算
    QPoint buttonPos = m_hoverButton->mapToGlobal(QPoint(0, 0));
    QPoint previewPos = QPoint(buttonPos.x() + m_hoverButton->width() / 2, 
                              buttonPos.y());
    
    // ボタン位置デバッグログ
    LOG_INFO(QString("🔍 ボタン位置詳細: ローカル位置(%1,%2), グローバル位置(%3,%4), TaskBarEx位置(%5,%6)")
             .arg(m_hoverButton->x()).arg(m_hoverButton->y())
             .arg(buttonPos.x()).arg(buttonPos.y())
             .arg(this->x()).arg(this->y()));
    
    // フォルダの場合はタイトルを短縮（パス→フォルダ名のみ）
    QString displayTitle = m_hoverWindowInfo.title;
    if (displayTitle.contains('\\') || displayTitle.contains('/')) {
        // パス区切り文字が含まれている場合はフォルダ名のみを抽出
        QFileInfo fileInfo(displayTitle);
        if (fileInfo.isAbsolute()) {
            displayTitle = fileInfo.fileName();
            if (displayTitle.isEmpty()) {
                // ルートディレクトリの場合
                displayTitle = fileInfo.absolutePath().split(QDir::separator()).last();
            }
            LOG_INFO(QString("📁 フォルダタイトル短縮: %1 → %2")
                     .arg(m_hoverWindowInfo.title).arg(displayTitle));
        }
    }
    
    // カスタムサムネイルプレビューを表示（HWNDも渡してクリック機能有効化）
    m_thumbnailPreview->showThumbnail(thumbnail, displayTitle, previewPos, m_hoverWindowInfo.hwnd);
    
    if (!thumbnail.isNull()) {
        LOG_INFO("✅ カスタムサムネイルプレビュー表示完了");
    } else {
        LOG_WARNING("⚠️ サムネイル取得失敗、プレースホルダー表示");
    }
}

void TaskbarWindow::onGroupThumbnailDisplay()
{
    if (!m_hoverButton) return;
    
    QVariantList groupData = m_hoverButton->property("groupWindows").toList();
    if (groupData.isEmpty()) {
        LOG_WARNING("⚠️ グループデータが空です");
        return;
    }
    
    LOG_INFO(QString("🖼️ グループサムネイル表示開始: %1個のウィンドウ").arg(groupData.size()));
    
    // とりあえず最初のウィンドウのサムネイルを表示（簡易実装）
    WindowInfo firstWindow = groupData[0].value<WindowInfo>();
    
    // サムネイル取得
    QSize highQualitySize(450, 338);
    QPixmap thumbnail = m_thumbnailManager->captureWindowThumbnail(firstWindow.hwnd, highQualitySize);
    
    // ボタンの位置を取得
    QPoint buttonPos = m_hoverButton->mapToGlobal(QPoint(0, 0));
    QPoint previewPos = QPoint(buttonPos.x() + m_hoverButton->width() / 2, buttonPos.y());
    
    // グループ情報をタイトルに追加
    QString groupTitle = QString("%1 (グループ: %2個)")
                        .arg(firstWindow.title.contains(" - ") ? 
                             firstWindow.title.split(" - ").last() : firstWindow.title)
                        .arg(groupData.size());
    
    // サムネイルプレビュー表示
    m_thumbnailPreview->showThumbnail(thumbnail, groupTitle, previewPos, firstWindow.hwnd);
    
    LOG_INFO("✅ グループサムネイルプレビュー表示完了");
}

QString TaskbarWindow::thumbnailToBase64(const QPixmap& pixmap)
{
    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    pixmap.save(&buffer, "PNG");
    return ba.toBase64();
}

bool TaskbarWindow::eventFilter(QObject *watched, QEvent *event)
{
    QPushButton *button = qobject_cast<QPushButton*>(watched);
    if (!button) {
        return QMainWindow::eventFilter(watched, event);
    }
    
    switch (event->type()) {
    case QEvent::HoverEnter:
        // 通常のボタン（単一ウィンドウ）のみ処理
        if (button->property("windowInfo").isValid()) {
            WindowInfo windowInfo = button->property("windowInfo").value<WindowInfo>();
            onButtonHoverEnter(button, windowInfo);
        }
        return false;  // イベントを継続処理
        
    case QEvent::HoverLeave:
        onButtonHoverLeave();
        return false;  // イベントを継続処理
        
    default:
        break;
    }
    
    return QMainWindow::eventFilter(watched, event);
}

void TaskbarWindow::onThumbnailClicked(HWND hwnd)
{
    if (!hwnd) {
        LOG_WARNING("⚠️ サムネイルクリック時に無効なHWND");
        return;
    }
    
    LOG_INFO(QString("🖱️ サムネイルクリック - ウィンドウアクティベート: HWND=%1")
             .arg(reinterpret_cast<quintptr>(hwnd)));
    
    // ウィンドウをアクティベート（ボタンクリックと同じ処理）
    if (IsIconic(hwnd)) {
        ShowWindow(hwnd, SW_RESTORE);  // 最小化されている場合は復元
        LOG_INFO("🔄 最小化ウィンドウを復元");
    }
    
    SetForegroundWindow(hwnd);
    LOG_INFO("✅ サムネイルクリックによるウィンドウアクティベート完了");
}