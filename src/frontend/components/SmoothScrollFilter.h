#ifndef SMOOTHSCROLLFILTER_H
#define SMOOTHSCROLLFILTER_H

#include <QObject>

class QScrollBar;
class QPropertyAnimation;

class SmoothScrollFilter : public QObject
{
    Q_OBJECT
public:
    SmoothScrollFilter(QScrollBar *scrollBar, QObject *parent = nullptr);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    QScrollBar *m_scrollBar;
    QPropertyAnimation *m_animation;
    int m_targetValue;
};

#endif // SMOOTHSCROLLFILTER_H
