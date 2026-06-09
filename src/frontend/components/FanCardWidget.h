#ifndef FANCARDWIDGET_H
#define FANCARDWIDGET_H

#include <QFrame>
#include <QLabel>
#include <QProgressBar>
#include <QJsonObject>

class FanCardWidget : public QFrame
{
    Q_OBJECT
public:
    FanCardWidget(const QJsonObject &fan, QWidget *parent = nullptr);
    void updateData(const QJsonObject &fan);
    void setSelectedState(bool selected);
    QString fanId() const { return m_fanId; }

signals:
    void clicked(const QString &fanId);

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    QString m_fanId;
    QLabel *m_nameLabel;
    QLabel *m_hardwareLabel;
    QLabel *m_rpmLabel;
    QLabel *m_modeBadge;
    QProgressBar *m_progressBar;
    bool m_isSelected;
};

#endif // FANCARDWIDGET_H
