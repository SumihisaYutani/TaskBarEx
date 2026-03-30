#include "ThumbnailPreviewWidget.h"
#include "Logger.h"
#include <QPainter>
#include <QApplication>
#include <QScreen>
#include <QHBoxLayout>
#include <QMouseEvent>

ThumbnailPreviewWidget::ThumbnailPreviewWidget(QWidget *parent)
    : QWidget(parent)
    , m_thumbnailLabel(new QLabel(this))
    , m_titleLabel(new QLabel(this))
    , m_layout(new QVBoxLayout(this))
    , m_autoHideTimer(new QTimer(this))
    , m_currentHwnd(nullptr)
{
    LOG_INFO("🖼️ ThumbnailPreviewWidget 初期化開始");
    setupWidget();
    LOG_INFO("✅ ThumbnailPreviewWidget 初期化完了");
}

ThumbnailPreviewWidget::~ThumbnailPreviewWidget()
{
    LOG_INFO("🗑️ ThumbnailPreviewWidget 破棄");
}

void ThumbnailPreviewWidget::setupWidget()
{
    // ウィジェット設定
    setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground, true);
    setAttribute(Qt::WA_ShowWithoutActivating, true);
    setFixedSize(THUMBNAIL_WIDTH + WIDGET_MARGIN * 2, THUMBNAIL_HEIGHT + 50 + WIDGET_MARGIN * 2);
    
    // レイアウト設定
    m_layout->setContentsMargins(WIDGET_MARGIN, WIDGET_MARGIN, WIDGET_MARGIN, WIDGET_MARGIN);
    m_layout->setSpacing(5);
    
    // サムネイルラベル設定
    m_thumbnailLabel->setFixedSize(THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT);
    m_thumbnailLabel->setAlignment(Qt::AlignCenter);
    m_thumbnailLabel->setStyleSheet(
        "QLabel {"
        "    background-color: rgba(30, 30, 30, 200);"
        "    border: 1px solid rgba(100, 100, 100, 150);"
        "    border-radius: 4px;"
        "}"
    );
    m_thumbnailLabel->setScaledContents(true);
    
    // タイトルラベル設定
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_titleLabel->setWordWrap(true);
    m_titleLabel->setMaximumHeight(40);
    m_titleLabel->setStyleSheet(
        "QLabel {"
        "    background-color: rgba(50, 50, 50, 200);"
        "    color: white;"
        "    font-size: 11px;"
        "    font-weight: bold;"
        "    padding: 4px;"
        "    border-radius: 4px;"
        "    border: 1px solid rgba(100, 100, 100, 150);"
        "}"
    );
    
    // レイアウトに追加
    m_layout->addWidget(m_thumbnailLabel);
    m_layout->addWidget(m_titleLabel);
    
    // 自動非表示タイマー設定
    m_autoHideTimer->setSingleShot(true);
    m_autoHideTimer->setInterval(AUTO_HIDE_DELAY);
    connect(m_autoHideTimer, &QTimer::timeout, this, &ThumbnailPreviewWidget::onAutoHideTimeout);
    
    // 初期状態は非表示
    hide();
}

void ThumbnailPreviewWidget::showThumbnail(const QPixmap &thumbnail, const QString &title, const QPoint &position, HWND hwnd)
{
    LOG_INFO(QString("🖼️ サムネイル表示: %1").arg(title));
    
    // HWNDを保存（クリック時のフォーカス用）
    m_currentHwnd = hwnd;
    
    // サムネイル設定
    if (!thumbnail.isNull()) {
        // 高画質変換設定
        QPixmap scaledThumbnail = thumbnail.scaled(THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT, 
                                                 Qt::KeepAspectRatio, Qt::SmoothTransformation);
        m_thumbnailLabel->setPixmap(scaledThumbnail);
        LOG_INFO(QString("✅ サムネイル画像設定完了: %1x%2").arg(thumbnail.width()).arg(thumbnail.height()));
    } else {
        // サムネイルがない場合はプレースホルダー表示
        QPixmap placeholder(THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT);
        placeholder.fill(Qt::darkGray);
        QPainter painter(&placeholder);
        painter.setPen(Qt::white);
        painter.setFont(QFont("Arial", 12, QFont::Bold));
        painter.drawText(placeholder.rect(), Qt::AlignCenter, "No Preview");
        m_thumbnailLabel->setPixmap(placeholder);
        LOG_INFO("⚠️ サムネイル無し、プレースホルダー表示");
    }
    
    // タイトル設定
    QString displayTitle = title.length() > 50 ? title.left(47) + "..." : title;
    m_titleLabel->setText(displayTitle);
    
    // 位置調整
    updatePosition(position);
    
    // 表示
    show();
    raise();
    
    // 自動非表示タイマー開始
    m_autoHideTimer->start();
    
    LOG_INFO("✅ サムネイルプレビュー表示完了");
}

void ThumbnailPreviewWidget::hideThumbnail()
{
    LOG_INFO("🔴 サムネイルプレビュー非表示");
    
    // タイマー停止
    m_autoHideTimer->stop();
    
    // 非表示
    hide();
}

bool ThumbnailPreviewWidget::isVisible() const
{
    return QWidget::isVisible();
}

void ThumbnailPreviewWidget::updatePosition(const QPoint &position)
{
    QScreen *screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    
    int x = position.x() - width() / 2;  // 中央配置
    int y = position.y() - height() - 10;  // ボタンの上に表示
    
    // 画面境界チェック
    if (x < 0) x = 0;
    if (x + width() > screenGeometry.width()) x = screenGeometry.width() - width();
    if (y < 0) y = position.y() + 30;  // ボタンの下に表示
    if (y + height() > screenGeometry.height()) y = screenGeometry.height() - height();
    
    move(x, y);
    LOG_INFO(QString("📍 サムネイルウィジェット位置設定: (%1, %2)").arg(x).arg(y));
}

void ThumbnailPreviewWidget::enterEvent(QEnterEvent *event)
{
    Q_UNUSED(event)
    // マウスが乗った時は自動非表示タイマーを停止
    m_autoHideTimer->stop();
}

void ThumbnailPreviewWidget::leaveEvent(QEvent *event)
{
    Q_UNUSED(event)
    // マウスが離れた時は自動非表示タイマーを再開
    m_autoHideTimer->start();
}

void ThumbnailPreviewWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    
    // 背景を半透明で描画
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QBrush(QColor(0, 0, 0, 150)));
    painter.setPen(QPen(QColor(100, 100, 100, 200), 1));
    painter.drawRoundedRect(rect().adjusted(1, 1, -1, -1), 6, 6);
}

void ThumbnailPreviewWidget::onAutoHideTimeout()
{
    LOG_INFO("⏰ サムネイル自動非表示タイマータイムアウト");
    hideThumbnail();
}

void ThumbnailPreviewWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_currentHwnd) {
        LOG_INFO(QString("🖱️ サムネイルクリック - ウィンドウフォーカス: HWND=%1")
                 .arg(reinterpret_cast<quintptr>(m_currentHwnd)));
        
        // サムネイルクリック時のフォーカス処理
        emit thumbnailClicked(m_currentHwnd);
        
        // サムネイルを非表示
        hideThumbnail();
    }
    
    QWidget::mousePressEvent(event);
}