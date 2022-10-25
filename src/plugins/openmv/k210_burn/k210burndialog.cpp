#include "k210burndialog.h"
#include "ui_k210burndialog.h"

#include <QKeyEvent>
#include <QSerialPortInfo>

#include "custom_combobox/custom_combobox.h"

#define BURN_SETTINGS_GROUP     "K210Burn"
#define BURN_LAST_FILE_PATH     "BurnLastPath"

K210BurnDialog::K210BurnDialog(QSettings *settings, QWidget *parent) :
    m_settings(settings),
    QDialog(parent),
    m_ui(new Ui::K210BurnDialog)
{
// qDebug() << __FUNCTION__ << __LINE__;

    setWindowFlags(Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint);
    setFocus(Qt::ActiveWindowFocusReason);

    m_ui->setupUi(this);

    m_settings->beginGroup(QStringLiteral(BURN_SETTINGS_GROUP));
    QString lastFileName = m_settings->value(QStringLiteral(BURN_LAST_FILE_PATH), QString()).toString();
    if(!lastFileName.isEmpty())
        m_ui->lineEditFileName->setText(lastFileName);
    m_settings->endGroup();

    QStringList baudList;
    baudList.append(QString(QStringLiteral("115200")));
    baudList.append(QString(QStringLiteral("921600")));
    baudList.append(QString(QStringLiteral("1500000")));
    baudList.append(QString(QStringLiteral("2000000")));
    baudList.append(QString(QStringLiteral("3000000")));
    m_ui->comboBoxBaud->addItems(baudList);
    m_ui->comboBoxBaud->setCurrentIndex(2); // default 1500000.

    QStringList eraseModeList;
    eraseModeList.append(tr("Section Erase"));
    eraseModeList.append(tr("FullChip Erase"));
    m_ui->comboBoxEraseMode->addItems(eraseModeList);
    m_ui->comboBoxEraseMode->setCurrentIndex(0);

    QStringList eraseSizeList;
    eraseSizeList.append(QString(QStringLiteral("Byte")));
    eraseSizeList.append(QString(QStringLiteral("KiB")));
    eraseSizeList.append(QString(QStringLiteral("MiB")));
    m_ui->comboBoxEraseSize->addItems(eraseSizeList);
    m_ui->comboBoxEraseSize->setCurrentIndex(1);

    QStringList eraseTempLateList;
    eraseTempLateList.append(QString());
    eraseTempLateList.append(tr("CanMV FileSystem"));
    m_ui->comboBoxEraseTemplate->addItems(eraseTempLateList);
    m_ui->comboBoxEraseTemplate->setCurrentIndex(0);

    connect(m_ui->comboBoxEraseMode, &QComboBox::currentTextChanged, this, [this] (const QString &text) {
        if(text == tr("Section Erase")) {
            m_eraseType = eraseSection;

            m_ui->lineEditEraseAddr->setEnabled(true);
            m_ui->lineEditEraseSize->setEnabled(true);
            m_ui->comboBoxEraseSize->setEnabled(true);
            m_ui->comboBoxEraseTemplate->setEnabled(true);
            m_ui->pushButtonLoadTemplate->setEnabled(true);
        } else if(text == tr("FullChip Erase")) {
            m_eraseType = eraseFullChip;

            m_ui->lineEditEraseAddr->setEnabled(false);
            m_ui->lineEditEraseSize->setEnabled(false);
            m_ui->comboBoxEraseSize->setEnabled(false);
            m_ui->comboBoxEraseTemplate->setEnabled(false);
            m_ui->pushButtonLoadTemplate->setEnabled(false);
        }
    });

    connect(m_ui->comboBoxEraseTemplate, &QComboBox::currentTextChanged, this, [this] (const QString &text) {
        if(text == tr("CanMV FileSystem")) {
            m_eraseTemplate = templateCanMVFs;
        } else if(text == QString()) {
            m_eraseTemplate = templateNone;
        }
    });

    connect(m_ui->comboBoxEraseSize, &QComboBox::currentTextChanged, this, [this] (const QString &text) {
        if(text == QString(QStringLiteral("Byte"))) {
            m_eraseUnit = eraseByte;
        }else if(text == QString(QStringLiteral("KiB"))) {
            m_eraseUnit = eraseKiB;
        } else {
            m_eraseUnit = eraseMiB;
        }
    });

    connect(&scanSerialTimer, &QTimer::timeout, this, &K210BurnDialog::scanSerialPort);
    scanSerialTimer.start(500);

    connect(m_ui->comboBoxPort, &CustomComboBox::comboBoxClicked, this, [this] {
        noPortListed = true;
        scanSerialTimer.start(200);
    });

    on_pushButtonSwitchMode_clicked();
}

K210BurnDialog::~K210BurnDialog()
{
// qDebug() << __FUNCTION__ << __LINE__;

    if(m_burn)
    {
        m_burn->burnReleasePort();
        delete m_burn;
        m_burn = Q_NULLPTR;
    }

    delete m_ui;
}

void K210BurnDialog::keyPressEvent(QKeyEvent *e)
{
    switch (e->key())
    {
        case Qt::Key_Escape:
            break;
        default:
            QDialog::keyPressEvent(e);
            break;
    }
}

void K210BurnDialog::scanSerialPort(void)
{
// qDebug() << __FUNCTION__ << __LINE__;

    if(noPortListed)
    {
        QStringList stringList;

        m_ui->comboBoxPort->clear();

        foreach(QSerialPortInfo port, QSerialPortInfo::availablePorts())
        {
            stringList.append(port.portName());
        }

        if(stringList.size() > 0)
        {
            noPortListed = false;

            m_ui->comboBoxPort->addItems(stringList);
            scanSerialTimer.stop();
        }
    }
}

void K210BurnDialog::on_pushButtonOpen_clicked()
{
    QFileInfo fileInfo = QFileInfo(m_ui->lineEditFileName->text());

    m_settings->beginGroup(QStringLiteral(BURN_SETTINGS_GROUP));

    QString openFileName = QFileDialog::getOpenFileName(this,
                        QObject::tr("Select file"),
                        m_settings->value(QStringLiteral(BURN_LAST_FILE_PATH), QDir::homePath()).toString(),
                        QObject::tr("Firmwares (*.bin *.kmodel *.kfpkg);;All Files (*.*)"));

    if(!openFileName.isEmpty())
    {
        QString fileName;

        if(Utils::HostOsInfo::isWindowsHost() && (openFileName[1] != QLatin1Char(':')))
        {
            fileName = QString(QStringLiteral("//wsl$")) + openFileName;
        }
        else
        {
            fileName = openFileName;
        }

        m_settings->setValue(QStringLiteral(BURN_LAST_FILE_PATH), fileName);

        m_ui->lineEditFileName->setText(fileName);
    }

    m_settings->endGroup();
}

void K210BurnDialog::startDownload(void)
{
// qDebug() << __FUNCTION__ << __LINE__;

    burnWorking = true;
    m_ui->pushButtonDownload->setText(tr("Cancel"));

    m_ui->progressBar->setVisible(false);
    m_ui->progressBar->setValue(0);

    m_ui->progressLabel->setVisible(true);
    m_ui->progressLabel->setText(tr("Start Download..."));

    m_ui->groupBoxFileInfo->setEnabled(false);
    m_ui->groupBoxUartInfo->setEnabled(false);
    m_ui->pushButtonSwitchMode->setEnabled(false);
}

void K210BurnDialog::stopDownload(bool forceStop)
{
// qDebug() << __FUNCTION__ << __LINE__;

    burnWorking = false;
    m_ui->pushButtonDownload->setText(tr("Download"));

    m_burn->burnReleasePort();

    disconnect(m_burn, &K210Burn::openResult, 0, 0);
    disconnect(m_burn, &K210Burn::burnStoped, 0, 0);

    disconnect(m_burn, &K210Burn::burnRamProcess, 0, 0);
    disconnect(m_burn, &K210Burn::burnRamResult, 0, 0);

    disconnect(m_burn, &K210Burn::burnFlashProcess, 0, 0);
    disconnect(m_burn, &K210Burn::burnFlashResult, 0, 0);

    m_ui->groupBoxFileInfo->setEnabled(true);
    m_ui->groupBoxUartInfo->setEnabled(true);
    m_ui->pushButtonSwitchMode->setEnabled(true);

    m_ui->progressLabel->setVisible(true);

    m_ui->progressBar->setVisible(false);
    m_ui->progressBar->setValue(0);

    if(forceStop) {
        m_ui->progressLabel->setText(tr("Stoped."));
    }
}

void K210BurnDialog::on_pushButtonDownload_clicked()
{
    if(burnWorking)
    {
        if(m_burn)
        {
            QEventLoop loop;
            connect(m_burn, &K210Burn::burnStoped, &loop, &QEventLoop::quit);

            m_burn->burnStop();
            m_ui->progressLabel->setText(tr("Stopping..."));

            loop.exec();
        }

        if(burn_loop.isRunning()) {
            burn_loop.exit(-1);
        }
    }
    else
    {
        if(!m_burn) {
            m_burn = new K210Burn(this);
        }

        startDownload();

        QString fileName = m_ui->lineEditFileName->text();
        QString deviceName = m_ui->comboBoxPort->currentText();

        int burnBaud = m_ui->comboBoxBaud->currentText().toInt();

        if(fileName.size() <= 0)
        {
            m_ui->progressLabel->setText(tr("Please Select a File."));

            stopDownload();
            return;
        }

        if(deviceName.size() <= 0)
        {
            m_ui->progressLabel->setText(tr("Please Select a Device."));

            stopDownload();
            return;
        }

        bool ok;
        int burnAddr = m_ui->lineEditFileAddress->text().toLower().toInt(&ok, 0);

        if((false == ok) || (burnAddr < 0) || (burnAddr > (16 * 1000 * 1000)))
        {
            stopDownload();
            m_ui->progressLabel->setText(tr("Please Spec the correct Address"));

            return;
        }

        // qDebug() << "fileName" << fileName;
        // qDebug() << "deviceName" << deviceName;
        // qDebug() << "burnAddr" << burnAddr;
        // qDebug() << "burnBaud" << burnBaud;

        QList <burnFirmwareInfo_t> files = parseFirmware(fileName, burnAddr, false /* aesEncrypt */, QByteArray()/* aesKey */, false/* DioMode */);

        if(files.isEmpty())
        {
            m_ui->progressLabel->setText(tr("Parse files Failed."));

            stopDownload();
            return;
        }

        int loopExitCode = int();
        bool openSucc = false;
        bool *openSuccPtr = &openSucc;

        ///////////////////////////////////
        // burn bootloader
        ///////////////////////////////////

        connect(m_burn, &K210Burn::openResult, this, [this, openSuccPtr] (burnErrcode_t errCode, const QString &errorMessage) {
            // qDebug() << "open result" << errorMessage;

            if(errCode != ERROR_NONE) {
                *openSuccPtr = false;
            } else {
                *openSuccPtr = true;
            }

            if(errorMessage.isEmpty()) {
                m_ui->progressLabel->setText(QString(QStringLiteral("Open Port Failed.")));
            } else {
                m_ui->progressLabel->setText(errorMessage);
            }
        });

        connect(m_burn, &K210Burn::openResult, &burn_loop, &QEventLoop::quit);

        m_burn->open(deviceName);

        if((-1 == burn_loop.exec()) || (false == openSucc))
        {
            // qDebug() << __FUNCTION__ << __LINE__;
            stopDownload();

            return;
        }

        m_ui->progressLabel->setVisible(false);
        m_ui->progressBar->setVisible(true);

        connect(m_burn, &K210Burn::burnRamProcess, this, [this] (int total, int current) {
            // qDebug() << "total" << total << "current" << current;

            if(total != m_ui->progressBar->maximum()) {
                m_ui->progressBar->setMaximum(total);
            }

            if(current != m_ui->progressBar->value()) {
                m_ui->progressBar->setValue(current);
            }
        });

        connect(m_burn, &K210Burn::burnRamResult, this, [this, openSuccPtr] (burnErrcode_t errCode, const QString &errorMessage) {
            // qDebug() << "burnRamResult result" << errorMessage;

            if(errCode != ERROR_NONE) {
                *openSuccPtr = false;
            } else {
                *openSuccPtr = true;
            }

            m_ui->progressLabel->setText(errorMessage);
        });

        connect(m_burn, &K210Burn::burnRamResult, &burn_loop, &QEventLoop::quit);

        openSucc = false;
        // m_burn->burnRam(0, QString(QStringLiteral(":/burn/download/isp.bin")));
        m_burn->burnRam(0, QString(QStringLiteral(":/openmv/k210_burn/download/isp.bin")));

        loopExitCode = burn_loop.exec();

        if((-1 == loopExitCode) || (false == openSucc))
        {
            // qDebug() << __FUNCTION__ << __LINE__;

            stopDownload((loopExitCode == -1));

            return;
        }

        ///////////////////////////////////
        // burn flash
        ///////////////////////////////////
        disconnect(m_burn, &K210Burn::burnRamProcess, 0, 0);
        disconnect(m_burn, &K210Burn::burnRamResult, 0, 0);

        // qDebug() << "Wait stub start.";

        QThread::msleep(100); // wait for bootloader start.

        m_ui->progressLabel->setVisible(false);
        m_ui->progressBar->setVisible(true);

        connect(m_burn, &K210Burn::burnFlashProcess, this, [this] (int total, int current) {
            // qDebug() << "total" << total << "current" << current;

            if(total != m_ui->progressBar->maximum()) {
                m_ui->progressBar->setMaximum(total);
            }

            if(current != m_ui->progressBar->value()) {
                m_ui->progressBar->setValue(current);
            }
        });

        connect(m_burn, &K210Burn::burnFlashResult, this, [this, openSuccPtr] (burnErrcode_t errCode, const QString &errorMessage) {
            // qDebug() << "burnFlashResult result" << errorMessage;

            if(errCode != ERROR_NONE) {
                *openSuccPtr = false;
            } else {
                *openSuccPtr = true;
            }

            m_ui->progressLabel->setText(errorMessage);
        });

        connect(m_burn, &K210Burn::burnFlashResult, &burn_loop, &QEventLoop::quit);

        m_burn->burnFlash(burnBaud, files);

        loopExitCode = burn_loop.exec();

        if((-1 == loopExitCode) || (false == openSucc))
        {
            // qDebug() << __FUNCTION__ << __LINE__;

            stopDownload((loopExitCode == -1));

            return;
        }

        stopDownload();
    }
}

void K210BurnDialog::on_pushButtonLoadTemplate_clicked()
{
    if(templateCanMVFs == m_eraseTemplate) {
        /*
            Addr 0xD00000
            Size 3
            Unit MiB
        */
       m_ui->lineEditEraseAddr->setText(QStringLiteral("0xD00000"));
       m_ui->lineEditEraseSize->setText(QStringLiteral("3"));
       m_ui->comboBoxEraseSize->setCurrentText(QStringLiteral("MiB"));
       m_ui->comboBoxEraseMode->setCurrentText(tr("Section Erase"));
    }

    m_ui->comboBoxEraseTemplate->setCurrentText(QString());
}

void K210BurnDialog::on_pushButtonSwitchMode_clicked()
{
    if(m_workMode == burnMode) {
        m_workMode = eraseMode;

        m_ui->progressBar->setVisible(false);

        m_ui->groupBoxFileInfo->setVisible(true);
        m_ui->groupBoxEraseSettings->setVisible(false);

        m_ui->pushButtonErase->setVisible(false);
        m_ui->pushButtonDownload->setVisible(true);

        m_ui->progressLabel->setText(tr("Click to Start Download..."));
        m_ui->pushButtonSwitchMode->setText(tr("Switch To Erase"));

        m_ui->groupBoxUartInfo->setGeometry(QRect(10, 120, 491, 121));

        m_ui->progressBar->setGeometry(QRect(10, 260, 491, 23));
        m_ui->progressLabel->setGeometry(QRect(10, 260, 491, 23));
        m_ui->pushButtonSwitchMode->setGeometry(QRect(10, 300, 121, 51));
        m_ui->pushButtonDownload->setGeometry(QRect(140, 300, 361, 51));

        setMinimumSize(QSize(512, 370));
        setMaximumSize(QSize(512, 370));
        resize(512, 370);
    } else { /* eraseMode */
        m_workMode = burnMode;

        m_ui->progressBar->setVisible(false);

        m_ui->groupBoxFileInfo->setVisible(false);
        m_ui->groupBoxEraseSettings->setVisible(true);

        m_ui->pushButtonErase->setVisible(true);
        m_ui->pushButtonDownload->setVisible(false);

        m_ui->progressLabel->setText(tr("Click to Start Erase..."));
        m_ui->pushButtonSwitchMode->setText(tr("Switch To Burn"));

        m_ui->groupBoxUartInfo->setGeometry(QRect(10, 220, 491, 121));

        m_ui->progressBar->setGeometry(QRect(10, 360, 491, 23));
        m_ui->progressLabel->setGeometry(QRect(10, 360, 491, 23));
        m_ui->pushButtonSwitchMode->setGeometry(QRect(10, 400, 121, 51));
        m_ui->pushButtonDownload->setGeometry(QRect(140, 400, 361, 51));

        setMinimumSize(QSize(512, 470));
        setMaximumSize(QSize(512, 470));
        resize(512, 470);
    }
}

void K210BurnDialog::startErase(void)
{
    eraseWorking = true;
    m_ui->pushButtonErase->setText(tr("Cancel"));

    m_ui->progressBar->setVisible(false);
    m_ui->progressBar->setValue(0);

    m_ui->progressLabel->setVisible(true);
    m_ui->progressLabel->setText(tr("Start Erase..."));

    m_ui->groupBoxEraseSettings->setEnabled(false);
    m_ui->groupBoxUartInfo->setEnabled(false);
    m_ui->pushButtonSwitchMode->setEnabled(false);
}

void K210BurnDialog::stopErase(bool forceStop)
{
    eraseWorking = false;
    m_ui->pushButtonErase->setText(tr("Erase"));

    m_burn->burnReleasePort();

    disconnect(m_burn, &K210Burn::openResult, 0, 0);
    disconnect(m_burn, &K210Burn::burnStoped, 0, 0);

    disconnect(m_burn, &K210Burn::burnRamProcess, 0, 0);
    disconnect(m_burn, &K210Burn::burnRamResult, 0, 0);

    disconnect(m_burn, &K210Burn::burnEraseResult, 0, 0);

    m_ui->groupBoxEraseSettings->setEnabled(true);
    m_ui->groupBoxUartInfo->setEnabled(true);
    m_ui->pushButtonSwitchMode->setEnabled(true);

    m_ui->progressLabel->setVisible(true);

    m_ui->progressBar->setVisible(false);
    m_ui->progressBar->setValue(0);

    if(forceStop) {
        m_ui->progressLabel->setText(tr("Stoped."));
    }
}

void K210BurnDialog::on_pushButtonErase_clicked()
{
    if(eraseWorking)
    {
        if(m_burn)
        {
            QEventLoop loop;
            connect(m_burn, &K210Burn::burnStoped, &loop, &QEventLoop::quit);

            m_burn->burnStop();
            m_ui->progressLabel->setText(tr("Stopping..."));

            loop.exec();
        }

        if(erase_loop.isRunning()) {
            erase_loop.exit(-1);
        }
    }
    else
    {
        if(!m_burn) {
            m_burn = new K210Burn(this);
        }

        startErase();

        QString deviceName = m_ui->comboBoxPort->currentText();

        if(deviceName.size() <= 0)
        {
            m_ui->progressLabel->setText(tr("Please Select a Device."));

            stopErase();
            return;
        }

        int eraseAddr = 0, eraseSize = 0;

        if(eraseSection == m_eraseType)
        {
            bool ok;

            eraseAddr = m_ui->lineEditEraseAddr->text().toLower().toInt(&ok, 0);

            if((false == ok) || (eraseAddr < 0) || (eraseAddr > (16 * 1000 * 1000)))
            {
                stopErase();
                m_ui->progressLabel->setText(tr("Please Spec the correct Erase Address"));

                return;
            }

            eraseSize = m_ui->lineEditEraseSize->text().toLower().toInt(&ok, 0);

            if(eraseKiB == m_eraseUnit) {
                eraseSize *= 1024;
            } else if(eraseMiB == m_eraseUnit) {
                eraseSize *= 1024 * 1024;
            }

            if((false == ok) || (eraseSize < 0) || (eraseSize > (16 * 1000 * 1000)))
            {
                stopErase();
                m_ui->progressLabel->setText(tr("Please Spec the correct Erase Size"));

                return;
            }
        }

        int loopExitCode = int();
        bool openSucc = false;
        bool *openSuccPtr = &openSucc;

        ///////////////////////////////////
        // burn bootloader
        ///////////////////////////////////

        connect(m_burn, &K210Burn::openResult, this, [this, openSuccPtr] (burnErrcode_t errCode, const QString &errorMessage) {
            if(errCode != ERROR_NONE) {
                *openSuccPtr = false;
            } else {
                *openSuccPtr = true;
            }

            if(errorMessage.isEmpty()) {
                m_ui->progressLabel->setText(QString(QStringLiteral("Open Port Failed.")));
            } else {
                m_ui->progressLabel->setText(errorMessage);
            }
        });

        connect(m_burn, &K210Burn::openResult, &erase_loop, &QEventLoop::quit);

        m_burn->open(deviceName);

        if((-1 == erase_loop.exec()) || (false == openSucc))
        {
            stopErase();
            return;
        }

        m_ui->progressLabel->setVisible(false);
        m_ui->progressBar->setVisible(true);

        connect(m_burn, &K210Burn::burnRamProcess, this, [this] (int total, int current) {
            if(total != m_ui->progressBar->maximum()) {
                m_ui->progressBar->setMaximum(total);
            }

            if(current != m_ui->progressBar->value()) {
                m_ui->progressBar->setValue(current);
            }
        });

        connect(m_burn, &K210Burn::burnRamResult, this, [this, openSuccPtr] (burnErrcode_t errCode, const QString &errorMessage) {
            if(errCode != ERROR_NONE) {
                *openSuccPtr = false;
            } else {
                *openSuccPtr = true;
            }

            m_ui->progressLabel->setText(errorMessage);
        });

        connect(m_burn, &K210Burn::burnRamResult, &erase_loop, &QEventLoop::quit);

        openSucc = false;
        m_burn->burnRam(0, QString(QStringLiteral(":/openmv/k210_burn/download/isp.bin")));

        loopExitCode = erase_loop.exec();

        if((-1 == loopExitCode) || (false == openSucc))
        {
            stopErase((loopExitCode == -1));

            return;
        }

        ///////////////////////////////////
        // erase flash
        ///////////////////////////////////
        disconnect(m_burn, &K210Burn::burnRamProcess, 0, 0);
        disconnect(m_burn, &K210Burn::burnRamResult, 0, 0);

        QThread::msleep(100); // wait for bootloader start.

        m_ui->progressBar->setVisible(false);
        m_ui->progressLabel->setVisible(true);
        m_ui->progressLabel->setText(tr("Wating Erase..."));
        
        connect(m_burn, &K210Burn::burnEraseResult, this, [this, openSuccPtr] (burnErrcode_t errCode, const QString &errorMessage) {
            if(errCode != ERROR_NONE) {
                *openSuccPtr = false;
            } else {
                *openSuccPtr = true;
            }

            m_ui->progressLabel->setText(errorMessage);
        });

        connect(m_burn, &K210Burn::burnEraseResult, &erase_loop, &QEventLoop::quit);

        m_burn->burnErase(m_eraseType, eraseAddr, eraseSize);

        loopExitCode = erase_loop.exec();

        if((-1 == loopExitCode) || (false == openSucc))
        {
            stopErase((loopExitCode == -1));
            return;
        }

        stopErase();
    }
}
