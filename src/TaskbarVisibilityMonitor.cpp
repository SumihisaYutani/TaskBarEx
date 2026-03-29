#include "TaskbarVisibilityMonitor.h"
#include "Logger.h"
#include <QDebug>

TaskbarVisibilityMonitor::TaskbarVisibilityMonitor(QObject *parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
    , m_lastTaskbarState(true)
    , m_taskbarHwnd(nullptr)
{
    findTaskbarWindow();
    
    connect(m_timer, &QTimer::timeout, this, &TaskbarVisibilityMonitor::checkTaskbarVisibility);
    m_timer->setInterval(250); // 250ms間隔で監視
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
    
    if (currentState != m_lastTaskbarState) {
        m_lastTaskbarState = currentState;
        LOG_INFO(QString("Taskbar visibility changed: %1").arg(currentState ? "visible" : "hidden"));
        emit taskbarVisibilityChanged(currentState);
    }
}

bool TaskbarVisibilityMonitor::getTaskbarVisibility()
{
    if (!m_taskbarHwnd) return true;
    
    // タスクバーウィンドウの可視性をチェック
    bool visible = IsWindowVisible(m_taskbarHwnd);
    
    // さらに詳細なチェック：ウィンドウ位置とサイズ
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
            POINT cursor;
            GetCursorPos(&cursor);
            HMONITOR monitor = MonitorFromPoint(cursor, MONITOR_DEFAULTTONEAREST);
            MONITORINFO mi;
            mi.cbSize = sizeof(MONITORINFO);
            if (GetMonitorInfo(monitor, &mi)) {
                // タスクバーが画面境界の外にある場合
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