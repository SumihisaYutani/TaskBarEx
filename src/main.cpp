#include <QApplication>
#include <QStyleFactory>
#include <QDir>
#include <QMessageBox>
#include <QWidget>
#include "TaskbarWindow.h"
#include "Logger.h"
#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#endif

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

#ifdef _WIN32
    // コンソールウィンドウを非表示にする（リリースビルドのみ）
    #ifndef _DEBUG
    if (GetConsoleWindow()) {
        ShowWindow(GetConsoleWindow(), SW_HIDE);
    }
    #endif
#endif
    
    app.setApplicationName("TaskBarEx");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("TaskBarEx");
    
    // ログシステム初期化
    LOG_INFO("TaskBarEx Application Starting...");
    LOG_INFO(QString("Application Path: %1").arg(app.applicationDirPath()));
    
    LOG_INFO("Creating TaskbarWindow...");
    TaskbarWindow* window = nullptr;
    
    try {
        LOG_INFO("=== STARTING TASKBAR WINDOW TEST MODE ===");
        
        // TaskbarWindowを作成するが、機能を段階的に有効化
        window = new TaskbarWindow();
        LOG_INFO("TaskbarWindow created successfully");
        
        // TaskbarWindowは内部ロジックに基づいて自動的に表示/非表示を制御
        // ここでは強制的にshow()やフォーカス操作を行わない
        
        LOG_INFO("Starting TaskbarWindow event loop...");
        
        app.setQuitOnLastWindowClosed(true);
        
        int result = app.exec();
        LOG_INFO(QString("TaskbarWindow event loop exited with code: %1").arg(result));
        
        delete window;
        return result;
        
    } catch (const std::exception& e) {
        QString errorMsg = QString("Exception in main: %1").arg(e.what());
        LOG_ERROR(errorMsg);
        
        // エラーダイアログを表示（デバッグ用）
        #ifdef _DEBUG
        QMessageBox::critical(nullptr, "初期化エラー", 
                             QString("アプリケーション初期化中にエラーが発生しました:\n%1").arg(e.what()));
        #endif
        
        if (window) delete window;
        return -1;
    } catch (...) {
        QString errorMsg = "Unknown exception in main";
        LOG_ERROR(errorMsg);
        
        // エラーダイアログを表示（デバッグ用）
        #ifdef _DEBUG
        QMessageBox::critical(nullptr, "初期化エラー", 
                             "アプリケーション初期化中に不明なエラーが発生しました。\n"
                             "詳細はログファイルを確認してください。");
        #endif
        
        if (window) delete window;
        return -1;
    }
}