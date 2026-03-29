#ifndef TASKBARGROUPMANAGER_H
#define TASKBARGROUPMANAGER_H

#include <QObject>
#include <QVector>
#include <QMap>
#include <QString>
#include "WindowInfo.h"

enum class GroupingMode {
    AlwaysCombine,      // 常にグループ化（Windows 10デフォルト）
    CombineWhenFull,    // タスクバーが満杯時のみグループ化
    NeverCombine        // グループ化しない
};

class TaskbarGroupManager : public QObject
{
    Q_OBJECT

public:
    explicit TaskbarGroupManager(QObject *parent = nullptr);
    ~TaskbarGroupManager();

    void setGroupingMode(GroupingMode mode);
    GroupingMode getGroupingMode() const;
    
    QVector<QVector<WindowInfo>> groupWindows(const QVector<WindowInfo>& windows);

private:
    QString getGroupKey(const WindowInfo& window);
    QString extractExecutableName(const QString& fullPath);
    bool shouldGroupWindows() const;
    
    GroupingMode m_groupingMode;
    int m_maxTaskbarItems; // タスクバーが満杯とみなすアイテム数
};

#endif // TASKBARGROUPMANAGER_H