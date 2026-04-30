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

// PinnedAppsManager からの型前方宣言
struct PinnedAppInfo;

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
    void onGroupButtonHoverEnter(QPushButton* button);
    void onButtonHoverLeave();
    void onGroupThumbnailDisplay();
    void onThumbnailDelayTimeout();
    void onThumbnailClicked(HWND hwnd);

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void setupConnections();
    void setupTimer();
    void populateTaskbarList();
    void updateStatusBar();
    void positionAboveTaskbar();
    void clearTaskbarButtons();
    void createTaskbarButton(const WindowInfo& window);
    void createGroupButton(const QVector<WindowInfo>& group);
    
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
    int m_screenHeight;
    int m_taskbarHeight;
    int m_appBarHeight;
    
    // 多段表示システム
    QWidget *m_runningAppsContainer;    // 起動中アプリコンテナ
    QWidget *m_pinnedAppsContainer;     // ピン留めアプリコンテナ
    QVBoxLayout *m_runningRowsLayout;   // 起動中アプリの段レイアウト
    QVBoxLayout *m_pinnedRowsLayout;    // ピン留めアプリの段レイアウト
    
    // 動的レイアウト管理設定
    static const int ICON_SIZE = 36;           // アイコンサイズ
    static const int ICON_SPACING = 2;         // アイコン間隔
    static const int ROW_HEIGHT = 40;          // 1段の高さ
    static const int ROW_MARGIN = 4;           // 段の上下マージン
    static const int SECTION_SPACING = 2;     // セクション間隔
    static const int LABEL_WIDTH = 45;         // セクションラベル幅
    static const int CONTAINER_MARGIN = 8;     // コンテナ左右マージン
    
    // 画面幅ベース動的アイコン数計算
    int m_maxIconsPerRow;                      // 画面幅に基づく1段あたり最大アイコン数
    int m_availableWidth;                      // 利用可能な幅
    
    // 段数・列数情報
    int m_runningRowCount;     // 起動中アプリの段数
    int m_pinnedRowCount;      // ピン留めアプリの段数
    int m_totalRowCount;       // 総段数
    
    // サムネイル表示用データ保持
    QPushButton *m_hoverButton;
    WindowInfo m_hoverWindowInfo;
    
    // 多段レイアウト管理
    void setupMultiRowLayout();
    void updateAppBarHeight();
    void updateMouseThresholds();
    void calculateRowCounts();
    void populateRunningAppsRows(const QVector<WindowInfo>& windows);
    void populatePinnedAppsRows();
    void clearAllAppButtons();
    void createAppButtonInRow(const WindowInfo& window, QHBoxLayout* rowLayout, bool isPinned = false);
    void createPinnedAppButtonInRow(const PinnedAppInfo& appInfo, QHBoxLayout* rowLayout);
    QHBoxLayout* createNewRow(QVBoxLayout* parentLayout, const QString& sectionName);
    
    // 画面幅ベース動的計算
    void calculateMaxIconsPerRow();
    int calculateRequiredRows(int itemCount) const;
    void updateLayoutForScreenSize();
    
    // 旧2段システム互換性（廃止予定）
    void setupTwoRowLayout();  // 廃止予定
    void populatePinnedAppsRow(); // 廃止予定
    void clearRunningAppButtons(); // 廃止予定
    void createRunningAppButton(const WindowInfo& window); // 廃止予定
    
    // サムネイル関連ヘルパー
    QString thumbnailToBase64(const QPixmap& pixmap);
    
    // タスクバー表示状態検出
    bool isTaskbarVisible();
};

#endif // TASKBARWINDOW_H