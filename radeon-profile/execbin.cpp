
// copyright marazmista @ 29.11.2014

#include "execbin.h"
#include "globalStuff.h"

#include <QLabel>
#include <QFileDialog>

execBin::execBin() : QProcess() {
    logEnabled = false;

    tab = new QWidget();

    ui.setupUi(tab);

    connect(this,SIGNAL(readyReadStandardOutput()),this,SLOT(execProcessReadOutput()));
    connect(this,SIGNAL(finished(int)),this,SLOT(execProcesFinished()));
    connect(this,SIGNAL(started()),this,SLOT(execProcesStart()));
    connect(ui.btn_save,SIGNAL(clicked()),this,SLOT(saveToFile()));

    setProcessChannelMode(MergedChannels);
}

execBin::~execBin() {
    qDebug() << "Deleting " << name;
    delete tab;

    if (state() == Running){
        qDebug() << name << " was still running, killing it";
        kill();
    }
}

void execBin::runBin(const QString &cmd) {
    start(cmd);
    ui.text_enviroinment->setPlainText(processEnvironment().toStringList().join(' '));
    ui.text_command->setPlainText(cmd);
}


void execBin::execProcessReadOutput() {
    QString o = readAllStandardOutput();

    if (!o.isEmpty())
        ui.text_output->appendPlainText(o);
}

void execBin::execProcesStart() {
    ui.label_status->setText(tr("Process running"));
}

void execBin::execProcesFinished() {
    if (!this->logData.logFilename.isEmpty() && this->logData.log.count() > 0) {
        QFile f(logData.logFilename);
        f.open(QIODevice::WriteOnly);
        f.write(this->logData.log.join("\n").toLatin1());
        f.close();
        this->logData.log.clear();
    }

    ui.label_status->setText(tr("Process terminated"));
}

void execBin::saveToFile() {
        QString filename = QFileDialog::getSaveFileName(0, tr("Save"), QDir::homePath()+"/output_"+this->name);
        if (!filename.isEmpty()) {
            QFile f(filename);
            f.open(QIODevice::WriteOnly);
            f.write(ui.text_output->toPlainText().toLatin1());
            f.close();
        }
}


void execBin::appendToLog(const QString &data) {
    this->logData.log.append(data);
}

void execBin::setLogFilename(const QString &name) {
    this->logData.logFilename = name;
}
