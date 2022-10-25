#include "custom_combobox.h"

CustomComboBox::CustomComboBox(QWidget *parent) : QComboBox(parent)
{

}

void CustomComboBox::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        emit comboBoxClicked();
    }
    QComboBox::mousePressEvent(event);
}
