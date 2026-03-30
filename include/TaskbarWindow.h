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
class PinnedAppsManager;
class QHBoxLayout;
class QVBoxLayout;

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
    PinnedAppsManager *m_pinnedManager;  // ピン留めアプリ管理
    static TaskbarWindow* s_instance;
    
    // マウス座標ベース表示制御
    QTimer *m_mouseTrackTimer;
    QTimer *m_hideDelayTimer;  // 非表示ディレイ用タイマー
    int m_screenHeight;
    int m_taskbarHeight;
    int m_appBarHeight;
    
    // 2段表示システム
    QWidget *m_runningAppsRow;    // 起動中アプリ行
    QWidget *m_pinnedAppsRow;     // ピン留めアプリ行
    QHBoxLayout *m_runningLayout; // 起動中アプリレイアウト
    QHBoxLayout *m_pinnedLayout;  // ピン留めアプリレイアウト
    
    // 動的レイアウト管理
    void updateAppBarHeight();
    void updateMouseThresholds();
    void setupTwoRowLayout();
    void populatePinnedAppsRow();
    void clearRunningAppButtons();
    void createRunningAppButton(const WindowInfo& window);
};

#endif // TASKBARWINDOW_H