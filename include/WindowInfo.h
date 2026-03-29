#ifndef WINDOWINFO_H
#define WINDOWINFO_H

#include <QString>
#include <QPixmap>
#include <windows.h>

struct WindowInfo
{
    HWND hwnd;
    QString title;
    QPixmap icon;
    QString executablePath;
    DWORD processId;
    bool isMinimized;
    QString appUserModelId;
    
    WindowInfo()
        : hwnd(nullptr)
        , processId(0)
        , isMinimized(false)
    {
    }
    
    bool operator==(const WindowInfo& other) const {
        return hwnd == other.hwnd;
    }
    
    bool operator!=(const WindowInfo& other) const {
        return !(*this == other);
    }
};

#endif // WINDOWINFO_H