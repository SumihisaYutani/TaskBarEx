#ifndef TASKBARMODEL_H
#define TASKBARMODEL_H

#include <QObject>
#include <QVector>
#include <QPixmap>
#include <QString>
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
    QPixmap getWindowIcon(HWND hwnd);
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
};

#endif // TASKBARMODEL_H