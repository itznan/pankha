#include "SmoothScrollFilter.h"
#include <QScrollBar>
#include <QPropertyAnimation>
#include <QWheelEvent>

SmoothScrollFilter::SmoothScrollFilter(QScrollBar *scrollBar, QObject *parent)
    : QObject(parent)
    , m_scrollBar(scrollBar)
    , m_animation(nullptr)
    , m_targetValue(0)
{
    if (m_scrollBar) {
        m_targetValue = m_scrollBar->value();
    }
}

bool SmoothScrollFilter::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj);
    if (event->type() == QEvent::Wheel) {
        if (m_scrollBar && m_scrollBar->maximum() > 0) {
            QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);
            int delta = wheelEvent->angleDelta().y();
            
            int scrollAmount = (delta / 120) * 100;
            
            int target = (m_animation && m_animation->state() == QPropertyAnimation::Running) ? m_targetValue : m_scrollBar->value();
            target -= scrollAmount;
            target = qBound(m_scrollBar->minimum(), target, m_scrollBar->maximum());
            
            m_targetValue = target;
            
            if (!m_animation) {
                m_animation = new QPropertyAnimation(m_scrollBar, "value", this);
                m_animation->setDuration(250);
                m_animation->setEasingCurve(QEasingCurve::OutCubic);
            }
            
            m_animation->stop();
            m_animation->setStartValue(m_scrollBar->value());
            m_animation->setEndValue(m_targetValue);
            m_animation->start();
            
            return true;
        }
    }
    return QObject::eventFilter(obj, event);
}
