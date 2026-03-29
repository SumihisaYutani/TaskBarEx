#ifndef TASKBARVISIBILITYMONITOR_H
#define TASKBARVISIBILITYMONITOR_H

#include <QObject>
#include <QTimer>
#include <windows.h>

class TaskbarVisibilityMonitor : public QObject
{
    Q_OBJECT

public:
    explicit TaskbarVisibilityMonitor(QObject *parent = nullptr);
    ~TaskbarVisibilityMonitor();

    void startMonitoring();
    void stopMonitoring();
    bool isTaskbarVisible() const;

signals:
    void taskbarVisibilityChanged(bool visible);

private slots:
    void checkTaskbarVisibility();

private:
    QTimer* m_timer;
    bool m_lastTaskbarState;
    HWND m_taskbarHwnd;
    
    // デバウンス機能用
    bool m_pendingState;
    int m_stableCount;
    
    bool getTaskbarVisibility();
    bool getTaskbarVisibilityUsingAppBarData();
    bool getTaskbarVisibilityUsingPosition();
    void findTaskbarWindow();
};

#endif // TASKBARVISIBILITYMONITOR_H