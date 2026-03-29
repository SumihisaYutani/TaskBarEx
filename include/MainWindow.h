#pragma once

#include "Common.h"
#include "TaskbarManager.h"
#include "TaskbarGroupWidget.h"

class QSystemTrayIcon;
class QMenu;
class QAction;

/**
 * @brief アプリケーションのメインウィンドウクラス
 * 
 * TaskbarContainerを含む主要UIと、
 * システムトレイ統合、設定ダイアログなどを管理
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();
    
    // ウィンドウ管理
    void showTaskbar();
    void hideTaskbar();
    void toggleTaskbar();
    
    bool isTaskbarVisible() const;
    
protected:
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void moveEvent(QMoveEvent* event) override;
    
    // Windows固有イベント
    #ifdef Q_OS_WIN
    bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;
    void setupWindowsIntegration();
    #endif
    
private slots:
    // TaskbarManager連携
    void onApplicationsChanged();
    void onApplicationAdded(const QString& groupId);
    void onApplicationRemoved(const QString& groupId);
    void onApplicationUpdated(const QString& groupId);
    void onTaskbarError(const QString& message);
    
    // UI イベント
    void onWindowActivated(HWND hwnd);
    void onApplicationActivated(const QString& groupId);
    void onGroupContextMenu(const QString& groupId, const QPoint& globalPos);
    
    // システムトレイ
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void onTrayMenuTriggered();
    
    // メニューアクション
    void showSettings();
    void showAbout();
    void exitApplication();
    void refreshTaskbar();
    void toggleStayOnTop();
    void toggleAutoHide();
    
    // 設定変更
    void onSettingsChanged();
    void onGroupingModeChanged(GroupingMode mode);
    void onUpdateIntervalChanged(int milliseconds);
    void onFilteringChanged();
    void onUISettingsChanged();
    
private:
    // コアコンポーネント
    TaskbarManager* m_taskbarManager;
    TaskbarSettings* m_settings;
    TaskbarContainer* m_taskbarContainer;
    
    // システム統合
    QSystemTrayIcon* m_trayIcon;
    QMenu* m_trayMenu;
    
    // UIコンポーネント
    QWidget* m_centralWidget;
    QVBoxLayout* m_layout;
    QLabel* m_statusLabel;
    
    // メニューとアクション
    QMenuBar* m_menuBar;
    QMenu* m_fileMenu;
    QMenu* m_viewMenu;
    QMenu* m_settingsMenu;
    QMenu* m_helpMenu;
    
    QAction* m_refreshAction;
    QAction* m_settingsAction;
    QAction* m_exitAction;
    QAction* m_aboutAction;
    QAction* m_stayOnTopAction;
    QAction* m_autoHideAction;
    QAction* m_showTitlesAction;
    
    // 状態
    bool m_isClosingToTray;
    QPoint m_lastPosition;
    QSize m_lastSize;
    
    // Windows固有
    #ifdef Q_OS_WIN
    HWND m_hwnd;
    bool m_windowsIntegrationEnabled;
    #endif
    
    void setupUI();
    void setupMenus();
    void setupActions();
    void setupSystemTray();
    void setupStatusBar();
    void connectSignals();
    
    void updateWindowTitle();
    void updateStatusText();
    void updateTrayTooltip();
    void updateMenuStates();
    
    void applySettings();
    void saveWindowGeometry();
    void restoreWindowGeometry();
    
    void showNotification(const QString& title, const QString& message, 
                         QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::Information);
    
    QString getStatusText() const;
};

/**
 * @brief 設定ダイアログクラス
 */
class SettingsDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit SettingsDialog(TaskbarSettings* settings, QWidget* parent = nullptr);
    
public slots:
    void accept() override;
    void reject() override;
    
private slots:
    void resetToDefaults();
    void applySettings();
    void onGroupingModeChanged();
    void onUpdateIntervalChanged();
    void onIconSizeChanged();
    
private:
    TaskbarSettings* m_settings;
    TaskbarSettings* m_tempSettings; // 一時的な設定保存用
    
    // UI コンポーネント
    QTabWidget* m_tabWidget;
    
    // 一般設定タブ
    QWidget* m_generalTab;
    QComboBox* m_groupingModeCombo;
    QSpinBox* m_updateIntervalSpin;
    QCheckBox* m_stayOnTopCheck;
    QCheckBox* m_autoHideCheck;
    
    // フィルタリングタブ
    QWidget* m_filteringTab;
    QCheckBox* m_includeSystemAppsCheck;
    QCheckBox* m_includeUWPAppsCheck;
    
    // 表示設定タブ
    QWidget* m_displayTab;
    QSlider* m_iconSizeSlider;
    QLabel* m_iconSizeLabel;
    QCheckBox* m_showTitlesCheck;
    QComboBox* m_layoutCombo;
    
    // ボタン
    QPushButton* m_okButton;
    QPushButton* m_cancelButton;
    QPushButton* m_applyButton;
    QPushButton* m_resetButton;
    
    void setupUI();
    void setupGeneralTab();
    void setupFilteringTab();
    void setupDisplayTab();
    void setupButtons();
    
    void loadSettings();
    void saveSettings();
    void updateIconSizeLabel();
};