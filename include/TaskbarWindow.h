#ifndef TASKBARWINDOW_H
#define TASKBARWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QTimer>
#include <QLabel>
#include <QPixmap>
#include <windows.h>
#include "WindowInfo.h"
#include "WindowThumbnailManager.h"
#include "ThumbnailPreviewWidget.h"

QT_BEGIN_NAMESPACE
namespace Ui { class TaskbarWindow; }
QT_END_NAMESPACE

class TaskbarModel;
class TaskbarGroupManager;
class PinnedAppsManager;
class WindowThumbnailManager;
class ThumbnailPreviewWidget;
class QHBoxLayout;
class QVBoxLayout;
class QPushButton;

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
    void onButtonHoverEnter(QPushButton* button, const WindowInfo& windowInfo);
    void onButtonHoverLeave();
    void onThumbnailDelayTimeout();
    void onThumbnailClicked(HWND hwnd);

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

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
    WindowThumbnailManager *m_thumbnailManager;  // サムネイル管理
    ThumbnailPreviewWidget *m_thumbnailPreview;  // サムネイルプレビュー表示
    static TaskbarWindow* s_instance;
    
    // マウス座標ベース表示制御
    QTimer *m_mouseTrackTimer;
    QTimer *m_hideDelayTimer;  // 非表示ディレイ用タイマー
    QTimer *m_thumbnailDelayTimer;  // サムネイル表示ディレイ用タイマー
    int m_screenHeight;
    int m_taskbarHeight;
    int m_appBarHeight;
    
    // 2段表示システム
    QWidget *m_runningAppsRow;    // 起動中アプリ行
    QWidget *m_pinnedAppsRow;     // ピン留めアプリ行
    QHBoxLayout *m_runningLayout; // 起動中アプリレイアウト
    QHBoxLayout *m_pinnedLayout;  // ピン留めアプリレイアウト
    
    // サムネイル表示用データ保持
    QPushButton *m_hoverButton;
    WindowInfo m_hoverWindowInfo;
    
    // 動的レイアウト管理
    void updateAppBarHeight();
    void updateMouseThresholds();
    void setupTwoRowLayout();
    void populatePinnedAppsRow();
    void clearRunningAppButtons();
    void createRunningAppButton(const WindowInfo& window);
    
    // サムネイル関連ヘルパー
    QString thumbnailToBase64(const QPixmap& pixmap);
    
    // タスクバー表示状態検出
    bool isTaskbarVisible();
};

#endif // TASKBARWINDOW_H