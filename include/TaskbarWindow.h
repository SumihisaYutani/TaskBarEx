#ifndef TASKBARWINDOW_H
#define TASKBARWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QTimer>
#include <QLabel>
#include <QPixmap>
#include <windows.h>
#include "WindowInfo.h"

QT_BEGIN_NAMESPACE
namespace Ui { class TaskbarWindow; }
QT_END_NAMESPACE

class TaskbarModel;
class TaskbarGroupManager;
class TaskbarVisibilityMonitor;

class TaskbarWindow : public QMainWindow
{
    Q_OBJECT

public:
    TaskbarWindow(QWidget *parent = nullptr);
    ~TaskbarWindow();
    
    static TaskbarWindow* getInstance() { return s_instance; }

private slots:
    void updateTaskbarItems();
    void onItemClicked(QListWidgetItem *item);
    void onRefreshTriggered();
    void onShowDelayTimeout();
    void onAlwaysOnTopTriggered(bool checked);
    void onAboutTriggered();
    void onTaskbarVisibilityChanged(bool visible);
    void onApplicationFocusChanged(QWidget *old, QWidget *now);
    void onWindowFocusChanged(HWND hwnd);
    void onMouseTrackCheck();

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    void setupConnections();
    void setupTimer();
    void populateTaskbarList();
    void updateStatusBar();
    void positionAboveTaskbar();
    void clearTaskbarButtons();
    void createTaskbarButton(const WindowInfo& window);
    
    void setupWindowFocusHook();
    void cleanupWindowFocusHook();
    
    // マウスオーバー管理メソッド
    void setupMouseTracking();
    void updateVisibilityBasedOnState();
    static void CALLBACK WinEventProc(HWINEVENTHOOK hWinEventHook, DWORD event,
                                      HWND hwnd, LONG idObject, LONG idChild,
                                      DWORD dwEventThread, DWORD dwmsEventTime);

    Ui::TaskbarWindow *ui;
    QTimer *m_updateTimer;
    
    TaskbarModel *m_model;
    TaskbarGroupManager *m_groupManager;
    TaskbarVisibilityMonitor *m_visibilityMonitor;
    
    HWINEVENTHOOK m_focusHook;
    static TaskbarWindow* s_instance;
    
    // マウスオーバー管理
    bool m_isMouseOver;
    QTimer *m_mouseTrackTimer;
    
    // 表示ディレイ管理
    QTimer *m_showDelayTimer;
    
    // 固定タスクバー位置
    int m_fixedTaskbarTop;
};

#endif // TASKBARWINDOW_H