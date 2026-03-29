#include "TaskbarVisibilityMonitor.h"
#include "Logger.h"
#include <QDebug>
#include <shellapi.h>

TaskbarVisibilityMonitor::TaskbarVisibilityMonitor(QObject *parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
    , m_lastTaskbarState(true)
    , m_taskbarHwnd(nullptr)
    , m_pendingState(true)
    , m_stableCount(0)
{
    findTaskbarWindow();
    
    connect(m_timer, &QTimer::timeout, this, &TaskbarVisibilityMonitor::checkTaskbarVisibility);
    m_timer->setInterval(350); // 350ms間隔で監視（応答性と安定性のバランス）
}

TaskbarVisibilityMonitor::~TaskbarVisibilityMonitor()
{
    stopMonitoring();
}

void TaskbarVisibilityMonitor::startMonitoring()
{
    if (m_taskbarHwnd) {
        m_lastTaskbarState = getTaskbarVisibility();
        m_timer->start();
        LOG_INFO("Taskbar visibility monitoring started");
        
        // テスト用：5秒後に強制的にシグナル送信
        QTimer::singleShot(5000, this, [this]() {
            LOG_INFO("🔧 TEST: Forcing visibility change signal (hidden)");
            emit taskbarVisibilityChanged(false);
        });
        
        // テスト用：10秒後に再度強制シグナル送信
        QTimer::singleShot(10000, this, [this]() {
            LOG_INFO("🔧 TEST: Forcing visibility change signal (visible)");
            emit taskbarVisibilityChanged(true);
        });
    } else {
        LOG_WARNING("Cannot start monitoring - taskbar window not found");
    }
}

void TaskbarVisibilityMonitor::stopMonitoring()
{
    m_timer->stop();
    LOG_INFO("Taskbar visibility monitoring stopped");
}

bool TaskbarVisibilityMonitor::isTaskbarVisible() const
{
    return m_lastTaskbarState;
}

void TaskbarVisibilityMonitor::checkTaskbarVisibility()
{
    if (!m_taskbarHwnd) {
        findTaskbarWindow();
        if (!m_taskbarHwnd) return;
    }
    
    bool currentState = getTaskbarVisibility();
    
    // ★修正: 表示検出は即座に通知、非表示検出のみdebounce適用
    if (currentState && currentState != m_lastTaskbarState) {
        // タスクバー表示は即座に通知（応答性重視）
        LOG_INFO(QString("🚀 IMMEDIATE SHOW: タスクバー表示を即座に通知 (%1 → %2)")
                 .arg(m_lastTaskbarState ? "visible" : "hidden")
                 .arg("visible"));
        m_lastTaskbarState = currentState;
        emit taskbarVisibilityChanged(currentState);
        m_stableCount = 0; // カウンタリセット
        return;
    }
    
    // 状態に変化がない場合（表示継続中など）
    if (currentState == m_lastTaskbarState) {
        static int noChangeCount = 0;
        if (noChangeCount++ % 28 == 0) { // 約10秒間隔でログ
            LOG_INFO(QString("No change: Taskbar state remains %1")
                     .arg(currentState ? "visible" : "hidden"));
        }
        return; // 変化がないので何もしない
    }
    
    // 非表示検出はdebounce適用（誤検知防止）
    if (currentState == m_pendingState) {
        m_stableCount++;
        if (m_stableCount < 2) {
            return; // まだ安定していない
        }
    } else {
        m_pendingState = currentState;
        m_stableCount = 1;
        return; // 新しい状態の最初の検出
    }
    
    // 安定した状態が確認されたので、変更を通知（非表示のみ）
    if (currentState != m_lastTaskbarState) {
        LOG_INFO(QString("✅ STABLE STATE CONFIRMED: %1 → %2 (after %3 checks)")
                 .arg(m_lastTaskbarState ? "visible" : "hidden")
                 .arg(currentState ? "visible" : "hidden")
                 .arg(m_stableCount));
        
        m_lastTaskbarState = currentState;
        emit taskbarVisibilityChanged(currentState);
    } else {
        // デバッグ用：状態に変化がない場合も時々ログ出力
        static int noChangeCount = 0;
        noChangeCount++;
        if (noChangeCount % 28 == 0) { // 350ms × 28 ≈ 10秒間隔
            LOG_INFO(QString("No change: Taskbar state remains %1 (check #%2)")
                     .arg(currentState ? "visible" : "hidden")
                     .arg(noChangeCount));
        }
    }
}

bool TaskbarVisibilityMonitor::getTaskbarVisibility()
{
    if (!m_taskbarHwnd) return true;
    
    // 方法1: AppBarDataを使った検出
    bool method1Result = getTaskbarVisibilityUsingAppBarData();
    
    // 方法2: 既存の位置ベース検出
    bool method2Result = getTaskbarVisibilityUsingPosition();
    
    // デバッグ用：両方の結果を比較
    static int methodCompareCount = 0;
    methodCompareCount++;
    if (methodCompareCount % 20 == 0) {
        LOG_INFO(QString("METHODS COMPARISON: AppBarData=%1, Position=%2")
                 .arg(method1Result ? "visible" : "hidden")
                 .arg(method2Result ? "visible" : "hidden"));
    }
    
    // Position方式の結果を優先（AppBarDataは設定のみ）
    return method2Result;
}

bool TaskbarVisibilityMonitor::getTaskbarVisibilityUsingAppBarData()
{
    // AppBarDataを使用してタスクバーの状態を取得
    APPBARDATA abd;
    ZeroMemory(&abd, sizeof(APPBARDATA));
    abd.cbSize = sizeof(APPBARDATA);
    abd.hWnd = m_taskbarHwnd;
    
    // タスクバーの状態を取得
    UINT state = (UINT)SHAppBarMessage(ABM_GETSTATE, &abd);
    
    bool isAutoHide = (state & ABS_AUTOHIDE) != 0;
    bool isVisible = true;
    
    if (isAutoHide) {
        // 自動非表示が有効な場合、現在の表示状態をチェック
        // タスクバーウィンドウの実際の可視状態を確認
        isVisible = IsWindowVisible(m_taskbarHwnd);
        
        // さらに詳細チェック：ウィンドウスタイルを確認
        LONG windowStyle = GetWindowLong(m_taskbarHwnd, GWL_STYLE);
        bool hasVisibleStyle = (windowStyle & WS_VISIBLE) != 0;
        
        // より詳細な動的状態検出テスト
        RECT rect;
        GetWindowRect(m_taskbarHwnd, &rect);
        
        // ウィンドウの透明度チェック
        BYTE alpha = 255;
        DWORD layered = GetWindowLong(m_taskbarHwnd, GWL_EXSTYLE);
        bool hasLayered = (layered & WS_EX_LAYERED) != 0;
        if (hasLayered) {
            GetLayeredWindowAttributes(m_taskbarHwnd, nullptr, &alpha, nullptr);
        }
        
        // Z-Order位置の確認
        HWND topWindow = GetTopWindow(GetDesktopWindow());
        bool isTopMost = (topWindow == m_taskbarHwnd);
        
        // デバッグログ - より多くの状態情報
        static int detailedDebugCount = 0;
        detailedDebugCount++;
        if (detailedDebugCount % 10 == 0) { // より頻繁に出力
            LOG_INFO(QString("DETAILED: AutoHide=%1, IsWindowVisible=%2, Style=%3, Alpha=%4, ZOrder=%5, Rect=%6,%7")
                     .arg(isAutoHide ? "yes" : "no")
                     .arg(isVisible ? "yes" : "no")
                     .arg(hasVisibleStyle ? "vis" : "hid")
                     .arg(alpha)
                     .arg(isTopMost ? "top" : "behind")
                     .arg(rect.top).arg(rect.bottom));
        }
        
        // 透明度ベースの判定を試行
        if (hasLayered && alpha < 50) {
            LOG_INFO("DETECTION: Taskbar appears hidden due to low alpha");
            return false;
        }
        
        return isVisible && hasVisibleStyle;
    }
    
    return true; // 自動非表示無効の場合は常に表示
}

bool TaskbarVisibilityMonitor::getTaskbarVisibilityUsingPosition()
{
    // ★改良：位置ベースでタスクバー可視性を判定（IsWindowVisibleに依存しない）
    bool visible = true; // デフォルトは表示状態と仮定
    
    // タスクバーの実際の位置とサイズを取得
    RECT taskbarRect;
    if (!GetWindowRect(m_taskbarHwnd, &taskbarRect)) {
        return true; // 取得失敗時は表示と仮定
    }
    
    // 画面サイズを取得
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    
    // ★修正：タスクバーが通常表示位置より上にある場合は隠れていると判定  
    if (taskbarRect.top < screenHeight - 60) {
        // タスクバーが画面下端から60px以上上にある場合は隠れていると判定
        // 通常: ~1078, 隠れ: ~1032 なので screenHeight-60 = 1020 が閾値
        visible = false;
        
        static int hiddenLogCount = 0;
        if (hiddenLogCount++ % 10 == 0) {
            LOG_INFO(QString("📍 POSITION CHECK: Taskbar hidden - screen=%1x%2, taskbar=%3,%4-%5,%6")
                     .arg(screenWidth).arg(screenHeight)
                     .arg(taskbarRect.left).arg(taskbarRect.top)
                     .arg(taskbarRect.right).arg(taskbarRect.bottom));
        }
    } else {
        // タスクバーが画面付近にある場合は表示状態
        static int visibleLogCount = 0;
        if (visibleLogCount++ % 20 == 0) {
            LOG_INFO(QString("📍 POSITION CHECK: Taskbar visible - screen=%1x%2, taskbar=%3,%4-%5,%6")
                     .arg(screenWidth).arg(screenHeight)
                     .arg(taskbarRect.left).arg(taskbarRect.top)
                     .arg(taskbarRect.right).arg(taskbarRect.bottom));
        }
    }
    
    // タスクバーの座標変化をリアルタイム監視
    static RECT lastRect = {0, 0, 0, 0};
    static bool firstCheck = true;
    
    RECT rect;
    if (GetWindowRect(m_taskbarHwnd, &rect)) {
        // 座標が変化した場合は即座にログ出力
        if (firstCheck || 
            rect.left != lastRect.left || rect.top != lastRect.top ||
            rect.right != lastRect.right || rect.bottom != lastRect.bottom) {
            
            int width = rect.right - rect.left;
            int height = rect.bottom - rect.top;
            LOG_INFO(QString("REALTIME: Taskbar position changed - IsVisible: %1, Size: %2x%3, Rect: %4,%5-%6,%7")
                     .arg(visible ? "true" : "false")
                     .arg(width).arg(height)
                     .arg(rect.left).arg(rect.top).arg(rect.right).arg(rect.bottom));
            
            lastRect = rect;
            firstCheck = false;
        }
    }
    
    // Windows 11の自動非表示対応の改良された検出
    if (visible) {
        RECT rect;
        if (GetWindowRect(m_taskbarHwnd, &rect)) {
            // タスクバーが画面外に移動している場合も非表示と判定
            int width = rect.right - rect.left;
            int height = rect.bottom - rect.top;
            
            if (width <= 0 || height <= 0) {
                visible = false;
            }
            
            // 画面の境界をチェック
            HMONITOR monitor = MonitorFromWindow(m_taskbarHwnd, MONITOR_DEFAULTTONEAREST);
            MONITORINFO mi;
            mi.cbSize = sizeof(MONITORINFO);
            if (GetMonitorInfo(monitor, &mi)) {
                // 改良されたタスクバー自動非表示検出
                // Windows 11では、タスクバーが画面外に移動することで非表示になる
                
                // デバッグ用：画面とタスクバーの位置関係を出力
                static int positionDebugCount = 0;
                positionDebugCount++;
                if (positionDebugCount % 40 == 0) { // 10秒間隔
                    LOG_INFO(QString("DEBUG: Screen: %1,%2-%3,%4, Taskbar: %5,%6-%7,%8")
                             .arg(mi.rcMonitor.left).arg(mi.rcMonitor.top)
                             .arg(mi.rcMonitor.right).arg(mi.rcMonitor.bottom)
                             .arg(rect.left).arg(rect.top).arg(rect.right).arg(rect.bottom));
                }
                
                // Windows 11の実際の動作に基づく自動非表示検出
                // 判定基準：タスクバー上端（rect.top）の位置で判定
                
                bool isAutoHidden = false;
                
                // 通常表示時の基準位置を1078px（観測された通常時のY座標）とする
                // 非表示時は1032px（46pxの差）
                const int normalTopPosition = 1078;
                const int hiddenTopPosition = 1032;
                
                // 実際の座標による判定（より正確）
                if (rect.top <= hiddenTopPosition) {
                    isAutoHidden = true;
                    static int hideCount = 0;
                    if (hideCount < 5) {
                        LOG_INFO(QString("DEBUG: Auto-hide detected - taskbar at hidden position (top: %1)")
                                 .arg(rect.top));
                        hideCount++;
                    }
                }
                
                // デバッグ用：位置変化を監視
                static int topPositionDebugCount = 0;
                topPositionDebugCount++;
                if (topPositionDebugCount % 20 == 0) {
                    LOG_INFO(QString("DEBUG: Taskbar top position: %1 (normal: %2, hidden: %3, current: %4)")
                             .arg(rect.top).arg(normalTopPosition)
                             .arg(hiddenTopPosition).arg(isAutoHidden ? "hidden" : "visible"));
                }
                
                if (isAutoHidden) {
                    visible = false;
                }
                
                // 完全に画面外の場合
                if (rect.bottom <= mi.rcMonitor.top || 
                    rect.top >= mi.rcMonitor.bottom ||
                    rect.right <= mi.rcMonitor.left || 
                    rect.left >= mi.rcMonitor.right) {
                    visible = false;
                }
            }
        }
    }
    
    return visible;
}

void TaskbarVisibilityMonitor::findTaskbarWindow()
{
    m_taskbarHwnd = FindWindow(L"Shell_TrayWnd", nullptr);
    if (m_taskbarHwnd) {
        LOG_INFO("Taskbar window found for monitoring");
    } else {
        LOG_WARNING("Taskbar window not found");
    }
}