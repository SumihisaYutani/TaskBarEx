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
    void onShowDelayTimeout();  // 互換性維持のため残存
    void onHideDelayTimeout();  // 互換性維持のため残存
    void onAlwaysOnTopTriggered(bool checked);
    void onAboutTriggered();
    void checkMousePosition();

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
    
    // 新しいマウス座標ベース表示制御
    void setupMouseTracking();

    Ui::TaskbarWindow *ui;
    QTimer *m_updateTimer;
    
    TaskbarModel *m_model;
    TaskbarGroupManager *m_groupManager;
    static TaskbarWindow* s_instance;
    
    // マウス座標ベース表示制御
    QTimer *m_mouseTrackTimer;
    int m_screenHeight;
    int m_taskbarHeight;
    int m_appBarHeight;
};

#endif // TASKBARWINDOW_H