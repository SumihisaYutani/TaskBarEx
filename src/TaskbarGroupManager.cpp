#include "TaskbarGroupManager.h"
#include <QFileInfo>
#include <QDebug>

TaskbarGroupManager::TaskbarGroupManager(QObject *parent)
    : QObject(parent)
    , m_groupingMode(GroupingMode::AlwaysCombine)
    , m_maxTaskbarItems(20) // デフォルトの最大アイテム数
{
}

TaskbarGroupManager::~TaskbarGroupManager()
{
}

void TaskbarGroupManager::setGroupingMode(GroupingMode mode)
{
    m_groupingMode = mode;
}

GroupingMode TaskbarGroupManager::getGroupingMode() const
{
    return m_groupingMode;
}

QVector<QVector<WindowInfo>> TaskbarGroupManager::groupWindows(const QVector<WindowInfo>& windows)
{
    QVector<QVector<WindowInfo>> groups;
    
    if (m_groupingMode == GroupingMode::NeverCombine) {
        // グループ化しない場合は各ウィンドウを個別のグループとして扱う
        for (const WindowInfo& window : windows) {
            QVector<WindowInfo> singleGroup;
            singleGroup.append(window);
            groups.append(singleGroup);
        }
        return groups;
    }
    
    // グループ化が必要かチェック
    bool shouldGroup = false;
    if (m_groupingMode == GroupingMode::AlwaysCombine) {
        shouldGroup = true;
    } else if (m_groupingMode == GroupingMode::CombineWhenFull) {
        shouldGroup = windows.size() > m_maxTaskbarItems;
    }
    
    if (!shouldGroup) {
        // グループ化しない場合
        for (const WindowInfo& window : windows) {
            QVector<WindowInfo> singleGroup;
            singleGroup.append(window);
            groups.append(singleGroup);
        }
        return groups;
    }
    
    // グループ化を実行
    QMap<QString, QVector<WindowInfo>> groupMap;
    
    for (const WindowInfo& window : windows) {
        QString groupKey = getGroupKey(window);
        groupMap[groupKey].append(window);
    }
    
    // QMapからQVectorに変換
    for (auto it = groupMap.begin(); it != groupMap.end(); ++it) {
        groups.append(it.value());
    }
    
    return groups;
}

QString TaskbarGroupManager::getGroupKey(const WindowInfo& window)
{
    // AppUserModelIDが利用可能な場合はそれを使用
    if (!window.appUserModelId.isEmpty()) {
        return window.appUserModelId;
    }
    
    // 実行ファイル名をキーとして使用
    QString executableName = extractExecutableName(window.executablePath);
    if (!executableName.isEmpty()) {
        return executableName;
    }
    
    // フォールバック: プロセスIDを文字列として使用
    return QString::number(window.processId);
}

QString TaskbarGroupManager::extractExecutableName(const QString& fullPath)
{
    if (fullPath.isEmpty()) {
        return QString();
    }
    
    QFileInfo fileInfo(fullPath);
    return fileInfo.baseName().toLower(); // 拡張子を除いた名前を小文字で返す
}

bool TaskbarGroupManager::shouldGroupWindows() const
{
    return m_groupingMode != GroupingMode::NeverCombine;
}