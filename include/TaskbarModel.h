#ifndef TASKBARMODEL_H
#define TASKBARMODEL_H

#include <QObject>
#include <QVector>
#include <QPixmap>
#include <QString>
#include <QMap>
#include <windows.h>
#include "WindowInfo.h"

class TaskbarModel : public QObject
{
    Q_OBJECT

public:
    explicit TaskbarModel(QObject *parent = nullptr);
    ~TaskbarModel();

    void updateWindowList();
    const QVector<WindowInfo>& getVisibleWindows() const;
    
    bool activateWindow(HWND hwnd);
    QPixmap getWindowIcon(HWND hwnd, bool safeMode = false);
    QString getWindowTitle(HWND hwnd);
    
signals:
    void windowListUpdated();

private:
    static BOOL CALLBACK enumWindowsProc(HWND hwnd, LPARAM lParam);
    
    bool isValidTaskbarWindow(HWND hwnd);
    bool isWindowVisible(HWND hwnd);
    bool hasWindowTitle(HWND hwnd);
    bool isNotToolWindow(HWND hwnd);
    
    void addWindow(HWND hwnd);
    QPixmap extractIcon(const QString& executablePath);
    QString getExecutablePath(HWND hwnd);
    QString getAppUserModelId(HWND hwnd);
    
    QVector<WindowInfo> m_windows;
    QVector<WindowInfo> m_visibleWindows;
    
    // ウィンドウ順序固定化用
    QMap<HWND, int> m_windowOrder;  // HWND → 初回検出順序のマップ
    int m_nextOrderIndex = 0;       // 次に割り当てる順序番号
};

#endif // TASKBARMODEL_H