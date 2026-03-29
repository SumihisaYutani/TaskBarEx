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
    
    bool getTaskbarVisibility();
    void findTaskbarWindow();
};

#endif // TASKBARVISIBILITYMONITOR_H