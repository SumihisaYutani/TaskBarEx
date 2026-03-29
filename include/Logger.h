#ifndef LOGGER_H
#define LOGGER_H

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QString>
#include <QDir>
#include <QApplication>

class Logger : public QObject
{
    Q_OBJECT

public:
    enum LogLevel {
        Debug,
        Info,
        Warning,
        Error
    };

    static Logger& instance();
    
    void log(LogLevel level, const QString& message);
    void debug(const QString& message);
    void info(const QString& message);
    void warning(const QString& message);
    void error(const QString& message);

private:
    Logger();
    ~Logger();
    
    void ensureLogDirectory();
    QString levelToString(LogLevel level);
    
    QFile m_logFile;
    QTextStream m_stream;
    QString m_logDir;
};

// ヘルパーマクロ
#define LOG_DEBUG(msg) Logger::instance().debug(msg)
#define LOG_INFO(msg) Logger::instance().info(msg)
#define LOG_WARNING(msg) Logger::instance().warning(msg)
#define LOG_ERROR(msg) Logger::instance().error(msg)

#endif // LOGGER_H