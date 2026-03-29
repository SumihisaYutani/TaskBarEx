#pragma once

#include "Common.h"
#include "ApplicationInfo.h"

/**
 * @brief 個別のアプリケーショングループを表示するウィジェット
 * 
 * 一つのアプリケーション（複数ウィンドウを含む可能性あり）を
 * Windows 10タスクバーのスタイルで表示する
 */
class TaskbarGroupWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit TaskbarGroupWidget(const ApplicationInfo& appInfo, QWidget* parent = nullptr);
    
    // アプリケーション情報
    const ApplicationInfo& applicationInfo() const { return m_appInfo; }
    void setApplicationInfo(const ApplicationInfo& appInfo);
    
    // 表示状態
    bool isExpanded() const { return m_isExpanded; }
    void setExpanded(bool expanded);
    
    // スタイル設定
    void setIconSize(int size);
    int iconSize() const { return m_iconSize; }
    
    void setShowWindowTitles(bool show);
    bool showWindowTitles() const { return m_showWindowTitles; }
    
signals:
    void windowActivated(HWND hwnd);
    void applicationActivated(const QString& groupId);
    void contextMenuRequested(const QPoint& globalPos);
    
protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    
private slots:
    void onMainButtonClicked();
    void onSubWindowClicked();
    void onExpandToggled();
    
private:
    ApplicationInfo m_appInfo;
    bool m_isExpanded;
    bool m_isHovered;
    int m_iconSize;
    bool m_showWindowTitles;
    
    // UIコンポーネント
    QVBoxLayout* m_layout;
    QPushButton* m_mainButton;
    QWidget* m_subWindowsContainer;
    QVBoxLayout* m_subWindowsLayout;
    QList<QPushButton*> m_subWindowButtons;
    
    // 内部メソッド
    void setupUI();
    void setupMainButton();
    void setupSubWindows();
    void updateMainButton();
    void updateSubWindows();
    void clearSubWindows();
    
    QString getMainButtonText() const;
    QString getMainButtonTooltip() const;
    void updateExpandIcon();
    void applyTaskbarStyle();
    void applyHoverEffect(bool hovered);
    
    // 静的スタイル
    static QString getMainButtonStyleSheet();
    static QString getSubButtonStyleSheet();
    static QString getContainerStyleSheet();
};

/**
 * @brief 複数のTaskbarGroupWidgetを管理するコンテナウィジェット
 */
class TaskbarContainer : public QScrollArea
{
    Q_OBJECT
    
public:
    explicit TaskbarContainer(QWidget* parent = nullptr);
    
    // グループ管理
    void addGroup(const ApplicationInfo& appInfo);
    void removeGroup(const QString& groupId);
    void updateGroup(const ApplicationInfo& appInfo);
    void clearGroups();
    
    TaskbarGroupWidget* getGroup(const QString& groupId) const;
    QList<TaskbarGroupWidget*> getAllGroups() const;
    int groupCount() const;
    
    // 表示設定
    void setIconSize(int size);
    void setShowWindowTitles(bool show);
    void setGroupingMode(GroupingMode mode);
    
    // レイアウト設定
    void setHorizontalLayout(bool horizontal);
    bool isHorizontalLayout() const { return m_horizontalLayout; }
    
signals:
    void windowActivated(HWND hwnd);
    void applicationActivated(const QString& groupId);
    void groupContextMenuRequested(const QString& groupId, const QPoint& globalPos);
    
protected:
    void wheelEvent(QWheelEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    
private slots:
    void onWindowActivated(HWND hwnd);
    void onApplicationActivated(const QString& groupId);
    void onContextMenuRequested(const QPoint& globalPos);
    
private:
    QWidget* m_contentWidget;
    QBoxLayout* m_contentLayout;
    QMap<QString, TaskbarGroupWidget*> m_groups;
    
    // 設定
    bool m_horizontalLayout;
    int m_iconSize;
    bool m_showWindowTitles;
    GroupingMode m_groupingMode;
    
    void setupLayout();
    void updateLayout();
    void sortGroups();
    
    static bool groupLessThan(TaskbarGroupWidget* a, TaskbarGroupWidget* b);
};

/**
 * @brief タスクバーウィジェット用のコンテキストメニュー
 */
class TaskbarContextMenu : public QMenu
{
    Q_OBJECT
    
public:
    explicit TaskbarContextMenu(const ApplicationInfo& appInfo, QWidget* parent = nullptr);
    
signals:
    void activateRequested(HWND hwnd);
    void minimizeRequested(HWND hwnd);
    void maximizeRequested(HWND hwnd);
    void closeRequested(HWND hwnd);
    void propertiesRequested(const QString& executablePath);
    
private:
    ApplicationInfo m_appInfo;
    
    void setupSingleWindowMenu(HWND hwnd);
    void setupMultiWindowMenu();
    void addSeparatorIfNeeded();
};