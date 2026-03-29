#ifndef TASKBARWINDOW_H
#define TASKBARWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QTimer>
#include <QLabel>
#include <QPixmap>
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

private slots:
    void updateTaskbarItems();
    void onItemClicked(QListWidgetItem *item);
    void onRefreshTriggered();
    void onAlwaysOnTopTriggered(bool checked);
    void onAboutTriggered();
    void onTaskbarVisibilityChanged(bool visible);

private:
    void setupConnections();
    void setupTimer();
    void populateTaskbarList();
    void updateStatusBar();
    void positionAboveTaskbar();
    void clearTaskbarButtons();
    void createTaskbarButton(const WindowInfo& window);

    Ui::TaskbarWindow *ui;
    QTimer *m_updateTimer;
    
    TaskbarModel *m_model;
    TaskbarGroupManager *m_groupManager;
    TaskbarVisibilityMonitor *m_visibilityMonitor;
};

#endif // TASKBARWINDOW_H