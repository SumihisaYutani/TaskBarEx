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

TaskbarWindow::TaskbarWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::TaskbarWindow)  // UIファイルを再有効化
    , m_updateTimer(new QTimer(this))
    , m_model(new TaskbarModel(this))
    , m_groupManager(new TaskbarGroupManager(this))
    , m_visibilityMonitor(new TaskbarVisibilityMonitor(this))
{
    LOG_INFO("TaskbarWindow constructor started");
    
    // UIファイル読み込みをテスト
    try {
        ui->setupUi(this);
        LOG_INFO("UI setup completed");
        
        // タスクバー風の設定に変更
        setWindowTitle("TaskBarEx");
        setGeometry(200, 200, 1000, 48);  // タスクバー風の高さに変更
        
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
}

TaskbarWindow::~TaskbarWindow()
{
    delete ui;
}

void TaskbarWindow::positionAboveTaskbar()
{
    QScreen *screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    
    LOG_INFO(QString("Screen geometry: %1,%2 %3x%4")
             .arg(screenGeometry.x()).arg(screenGeometry.y())
             .arg(screenGeometry.width()).arg(screenGeometry.height()));
    
    // Windowsタスクバーの位置と高さを取得
    RECT taskbarRect;
    HWND taskbarHwnd = FindWindow(L"Shell_TrayWnd", nullptr);
    if (taskbarHwnd && GetWindowRect(taskbarHwnd, &taskbarRect)) {
        int taskbarHeight = taskbarRect.bottom - taskbarRect.top;
        int taskbarY = taskbarRect.top;
        
        LOG_INFO(QString("Taskbar found - Height: %1, Y: %2").arg(taskbarHeight).arg(taskbarY));
        
        // アプリの下端とタスクバーの上端の間に50pxの隙間を確保（テスト用）
        int windowY = taskbarY - height() - 50;
        int windowX = screenGeometry.left();
        int windowWidth = screenGeometry.width();
        
        // Y座標が負数にならないよう調整
        if (windowY < 0) {
            windowY = screenGeometry.height() - taskbarHeight - height() - 10;
        }
        
        LOG_INFO(QString("Setting window position: %1,%2 %3x%4")
                 .arg(windowX).arg(windowY).arg(windowWidth).arg(height()));
        
        setGeometry(windowX, windowY, windowWidth, height());
    } else {
        // フォールバック: 画面下部に配置
        int windowY = screenGeometry.height() - 48 - height() - 20;
        
        LOG_WARNING("Taskbar not found, using fallback positioning");
        LOG_INFO(QString("Fallback position: %1,%2 %3x%4")
                 .arg(screenGeometry.left()).arg(windowY)
                 .arg(screenGeometry.width()).arg(height()));
        
        setGeometry(screenGeometry.left(), windowY, screenGeometry.width(), height());
    }
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
    
    // アイコンを設定（空の場合はデフォルトアイコン）
    if (!window.icon.isNull()) {
        LOG_INFO(QString("Icon size: %1x%2").arg(window.icon.width()).arg(window.icon.height()));
        QIcon icon(window.icon);
        button->setIcon(icon);
        button->setIconSize(QSize(36, 36));  // より大きなアイコンサイズ
        button->setText(""); // テキストをクリア
        LOG_INFO("Button has icon - icon set successfully");
        
        // デバッグ用：アイコンが実際に設定されているかチェック
        if (!button->icon().isNull()) {
            LOG_INFO("Button icon verification: Icon is set");
        } else {
            LOG_ERROR("Button icon verification: Icon is NULL after setting!");
        }
    } else {
        // デフォルトアイコンを設定（アプリ名の最初の文字を使用）
        QString firstChar = window.title.isEmpty() ? "?" : window.title.left(1).toUpper();
        button->setText(firstChar);
        LOG_INFO("Button has text placeholder");
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
    LOG_INFO(QString("Taskbar visibility changed: %1 - Adjusting window visibility")
             .arg(visible ? "visible" : "hidden"));
    
    if (visible) {
        // タスクバーが表示されたら、TaskBarExを表示
        show();
        raise();
        LOG_INFO("TaskBarEx window shown");
    } else {
        // タスクバーが非表示になったら、TaskBarExも非表示
        hide();
        LOG_INFO("TaskBarEx window hidden");
    }
}