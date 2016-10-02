
// copyright marazmista @ 29.11.2014

#include "execbin.h"
#include "globalStuff.h"

#include <QLabel>
#include <QFileDialog>

execBin::execBin() : QObject() {
    p = new QProcess();
    mainLay = new QVBoxLayout();
    btnLay = new QHBoxLayout();
    tab = new QWidget();
    output = new QPlainTextEdit();
    cmd = new QPlainTextEdit();
    lStatus = new QLabel();
    btnSave = new QPushButton();

    setupTab();

    connect(p,SIGNAL(readyReadStandardOutput()),this,SLOT(execProcessReadOutput()));
    connect(p,SIGNAL(finished(int)),this,SLOT(execProcesFinished()));
    connect(p,SIGNAL(started()),this,SLOT(execProcesStart()));
    connect(btnSave,SIGNAL(clicked()),this,SLOT(saveToFile()));
}

void execBin::setupTab() {
    output->setReadOnly(true);
    cmd->setReadOnly(true);
    cmd->setMaximumHeight(60);
    cmd->setFont(QFont("monospace", 8));
    output->setFont(QFont("monospace", 8));
    btnLay->setAlignment(Qt::AlignRight);
    QFont f;
    f.setBold(true);
    lStatus->setFont(f);
    p->setProcessChannelMode(QProcess::MergedChannels);


    this->btnSave->setText(tr("Save output to file"));

    // just two labels out of nowhere, forget it
    QLabel *l1 = new QLabel();
    l1->setText(tr("Command"));
    QLabel *l2 = new QLabel();
    l2->setText(tr("Output"));

    btnLay->addWidget(lStatus);
    btnLay->addWidget(btnSave);

    mainLay->addWidget(l1);
    mainLay->addWidget(cmd);
    mainLay->addWidget(l2);
    mainLay->addWidget(output);
    mainLay->addLayout(btnLay);
    tab->setLayout(mainLay);
}

void execBin::runBin(const QString &cmd) {
    p->start(cmd);
    this->cmd->setPlainText(p->processEnvironment().toStringList().join(" ") +" "+ cmd);
}

void execBin::setEnv(const QProcessEnvironment &env) {
    this->p->setProcessEnvironment(env);
}

void execBin::execProcessReadOutput() {
    QString o = this->p->readAllStandardOutput();

    if (!o.isEmpty())
        this->output->appendPlainText(o);
}

void execBin::execProcesStart() {
    this->lStatus->setText(tr("Process state: running"));
}

void execBin::execProcesFinished() {
    if (!this->logData.logFilename.isEmpty() && this->logData.log.count() > 0) {
        QFile f(logData.logFilename);
        f.open(QIODevice::WriteOnly);
        f.write(this->logData.log.join("\n").toLatin1());
        f.close();
        this->logData.log.clear();
    }

    this->lStatus->setText(tr("Process state: not running"));
}

void execBin::saveToFile() {
        QString filename = QFileDialog::getSaveFileName(0, tr("Save"), QDir::homePath()+"/output_"+this->name);
        if (!filename.isEmpty()) {
            QFile f(filename);
            f.open(QIODevice::WriteOnly);
            f.write(this->output->toPlainText().toLatin1());
            f.close();
        }
}

QProcess::ProcessState execBin::getExecState() {
    return this->p->state();
}

void execBin::appendToLog(const QString &data) {
    this->logData.log.append(data);
}

void execBin::setLogFilename(const QString &name) {
    this->logData.logFilename = name;
}
