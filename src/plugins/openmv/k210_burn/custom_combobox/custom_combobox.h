#ifndef CUSTOM_COMBOBOX_H
#define CUSTOM_COMBOBOX_H

#include <QtCore>
#include <QComboBox>
#include <QMouseEvent>

class CustomComboBox : public QComboBox {
    Q_OBJECT
public:
    explicit CustomComboBox(QWidget *parent = Q_NULLPTR);

signals:
    void comboBoxClicked(void);

protected:
    virtual void mousePressEvent(QMouseEvent *e);

private slots:

private:

};

#endif // CUSTOM_COMBOBOX_H
