#include "FanCardWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPropertyAnimation>
#include <QStyle>
#include <QMouseEvent>

FanCardWidget::FanCardWidget(const QJsonObject &fan, QWidget *parent)
    : QFrame(parent)
    , m_isSelected(false)
{
    setObjectName("fanCard");
    setProperty("class", "FanCard");
    setProperty("selected", false);
    setCursor(Qt::PointingHandCursor);

    m_fanId = fan["Id"].toString();

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(6);

    QHBoxLayout *topRow = new QHBoxLayout();
    m_nameLabel = new QLabel(fan["Name"].toString(), this);
    m_nameLabel->setObjectName("fanCardName");
    topRow->addWidget(m_nameLabel);
    topRow->addStretch(1);

    m_modeBadge = new QLabel(this);
    topRow->addWidget(m_modeBadge);
    layout->addLayout(topRow);

    QHBoxLayout *midRow = new QHBoxLayout();
    m_hardwareLabel = new QLabel(fan["HardwareName"].toString(), this);
    m_hardwareLabel->setObjectName("fanCardHardware");
    midRow->addWidget(m_hardwareLabel);
    midRow->addStretch(1);

    int rpm = fan["Rpm"].toDouble();
    m_rpmLabel = new QLabel(QString("%1 RPM").arg(rpm), this);
    m_rpmLabel->setObjectName("fanCardRpm");
    midRow->addWidget(m_rpmLabel);
    layout->addLayout(midRow);

    m_progressBar = new QProgressBar(this);
    m_progressBar->setFixedHeight(4);
    m_progressBar->setTextVisible(false);
    m_progressBar->setRange(0, 100);
    layout->addWidget(m_progressBar);

    updateData(fan);
}

void FanCardWidget::updateData(const QJsonObject &fan)
{
    int rpm = fan["Rpm"].toDouble();
    int minRpm = fan["Min"].toDouble();
    int maxRpm = fan["Max"].toDouble();
    int currentDuty = fan["SpeedPercent"].toDouble();

    m_rpmLabel->setText(QString("%1 RPM").arg(rpm));

    int progressVal = 0;
    if (maxRpm > minRpm && maxRpm > 0) {
        progressVal = qBound(0, (rpm - minRpm) * 100 / (maxRpm - minRpm), 100);
    } else if (rpm > 0) {
        progressVal = qBound(0, rpm * 100 / 3000, 100);
    }

    // Smoothly animate the card progress bar value
    QPropertyAnimation *progressAnim = new QPropertyAnimation(m_progressBar, "value", this);
    progressAnim->setDuration(400);
    progressAnim->setStartValue(m_progressBar->value());
    progressAnim->setEndValue(progressVal);
    progressAnim->setEasingCurve(QEasingCurve::OutQuad);
    progressAnim->start(QAbstractAnimation::DeleteWhenStopped);

    // Update badge mode
    QString mode = fan["Mode"].toString().toLower();
    QString controlId = fan["ControlId"].toString();
    if (controlId.isEmpty()) {
        m_modeBadge->setText("MONITOR ONLY");
        m_modeBadge->setObjectName("badgeMonitor");
    } else if (mode == "software") {
        m_modeBadge->setText("MANUAL");
        m_modeBadge->setObjectName("badgeManual");
    } else {
        m_modeBadge->setText("AUTO");
        m_modeBadge->setObjectName("badgeAuto");
    }

    // Refresh styling since objectName might have changed
    m_modeBadge->style()->unpolish(m_modeBadge);
    m_modeBadge->style()->polish(m_modeBadge);
}

void FanCardWidget::setSelectedState(bool selected)
{
    m_isSelected = selected;
    setProperty("selected", selected);
    style()->unpolish(this);
    style()->polish(this);
}

void FanCardWidget::mousePressEvent(QMouseEvent *event)
{
    emit clicked(m_fanId);
    QFrame::mousePressEvent(event);
}
