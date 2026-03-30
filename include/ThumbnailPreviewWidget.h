#ifndef THUMBNAILPREVIEWWIDGET_H
#define THUMBNAILPREVIEWWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPixmap>
#include <QTimer>
#include <QVBoxLayout>
#include <Windows.h>

class ThumbnailPreviewWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ThumbnailPreviewWidget(QWidget *parent = nullptr);
    ~ThumbnailPreviewWidget();

    // サムネイル表示
    void showThumbnail(const QPixmap &thumbnail, const QString &title, const QPoint &position, HWND hwnd = nullptr);
    void hideThumbnail();
    
    // 表示状態
    bool isVisible() const;

signals:
    void thumbnailClicked(HWND hwnd);

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private slots:
    void onAutoHideTimeout();

private:
    void setupWidget();
    void updatePosition(const QPoint &position);

    QLabel *m_thumbnailLabel;
    QLabel *m_titleLabel;
    QVBoxLayout *m_layout;
    QTimer *m_autoHideTimer;
    HWND m_currentHwnd;  // クリック時フォーカス用HWND保存
    
    static const int THUMBNAIL_WIDTH = 300;   // 200 -> 300 に拡大
    static const int THUMBNAIL_HEIGHT = 225;  // 150 -> 225 に拡大（4:3比率維持）
    static const int WIDGET_MARGIN = 12;      // マージンも拡大
    static const int AUTO_HIDE_DELAY = 3000; // 3秒で自動非表示
};

#endif // THUMBNAILPREVIEWWIDGET_H