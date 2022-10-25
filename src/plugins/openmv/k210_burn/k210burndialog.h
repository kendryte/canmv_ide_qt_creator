#ifndef K210BURNDIALOG_H
#define K210BURNDIALOG_H

#include <QtCore>
#include <QDialog>
#include <QFileDialog>

#include "download/k210burn.h"

namespace Ui {
class K210BurnDialog;
}

class K210BurnDialog : public QDialog
{
    Q_OBJECT

public:
    explicit K210BurnDialog(QSettings *settings, QWidget *parent = Q_NULLPTR);
    ~K210BurnDialog();

public slots:
    void keyPressEvent(QKeyEvent *e);

    void scanSerialPort(void);

private slots:
    void on_pushButtonOpen_clicked();

    void on_pushButtonDownload_clicked();

    void on_pushButtonLoadTemplate_clicked();

    void on_pushButtonSwitchMode_clicked();

    void on_pushButtonErase_clicked();

private:
    void startDownload(void);
    void stopDownload(bool forceStop = false);

    void startErase(void);
    void stopErase(bool forceStop = false);

    enum workMode {
        burnMode,
        eraseMode
    };
    workMode m_workMode = burnMode;

    enum eraseUnit {
        eraseByte,
        eraseKiB,
        eraseMiB,
    };
    eraseUnit m_eraseUnit = eraseKiB;
    burneraseType_t m_eraseType = eraseSection;

    enum eraseTemplate {
        templateNone,
        templateCanMVFs,
    };
    eraseTemplate m_eraseTemplate = templateNone;

    bool noPortListed = true;
    QTimer scanSerialTimer;

    bool burnWorking = false;
    QEventLoop burn_loop;

    bool eraseWorking = false;
    QEventLoop erase_loop;

    QSettings *m_settings;
    K210Burn *m_burn = Q_NULLPTR;

    Ui::K210BurnDialog *m_ui;
};

#endif // K210BURNDIALOG_H
