#include "Logger.h"
#include <QStandardPaths>
#include <QCoreApplication>
#include <QStringConverter>

Logger::Logger()
{
    ensureLogDirectory();
    
    // ログファイル名に日付と時刻を含めて起動ごとに別ファイル
    QString fileName = QString("TaskBarEx_%1.log")
                       .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss"));
    
    QString logFilePath = QDir(m_logDir).filePath(fileName);
    m_logFile.setFileName(logFilePath);
    
    if (m_logFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        m_stream.setDevice(&m_logFile);
        m_stream.setEncoding(QStringConverter::Utf8);
        
        // セッション開始ログ
        log(Info, QString("=== TaskBarEx Session Started - %1 ===")
                  .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
    }
}

Logger::~Logger()
{
    if (m_logFile.isOpen()) {
        log(Info, QString("=== TaskBarEx Session Ended - %1 ===")
                  .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
        m_logFile.close();
    }
}

Logger& Logger::instance()
{
    static Logger instance;
    return instance;
}

void Logger::ensureLogDirectory()
{
    // 実行ファイルと同じディレクトリにlogフォルダを作成
    QString appDir = QCoreApplication::applicationDirPath();
    m_logDir = QDir(appDir).filePath("log");
    
    QDir dir;
    if (!dir.exists(m_logDir)) {
        dir.mkpath(m_logDir);
    }
}

void Logger::log(LogLevel level, const QString& message)
{
    if (!m_logFile.isOpen()) {
        return;
    }
    
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString logLine = QString("[%1] [%2] %3")
                      .arg(timestamp, levelToString(level), message);
    
    m_stream << logLine << Qt::endl;
    m_stream.flush();
}

void Logger::debug(const QString& message)
{
    log(Debug, message);
}

void Logger::info(const QString& message)
{
    log(Info, message);
}

void Logger::warning(const QString& message)
{
    log(Warning, message);
}

void Logger::error(const QString& message)
{
    log(Error, message);
}

QString Logger::levelToString(LogLevel level)
{
    switch (level) {
        case Debug: return "DEBUG";
        case Info: return "INFO ";
        case Warning: return "WARN ";
        case Error: return "ERROR";
        default: return "UNKNW";
    }
}