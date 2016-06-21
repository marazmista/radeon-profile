
// copyright marazmista @ 29.11.2014

// class for handling executed process from exec page and all stuff
// regarding this process

#ifndef EXECBIN_H
#define EXECBIN_H

#include "ui_exec_bin.h"

#include <QProcess>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>
#include <QString>
#include <QLabel>

class execBin : public QProcess {
    Q_OBJECT
public:
    execBin();
    ~execBin() {
        delete tab;
    }

    void runBin(const QString &cmd);
    void appendToLog(const QString &data);
    void setLogFilename(const QString &name);

    QString name;
    QWidget *tab;
    bool logEnabled;

private slots:
    void execProcessReadOutput();
    void execProcesStart();
    void execProcesFinished();
    void saveToFile();

private:
    Ui::execBin ui;

    struct {
        QString logFilename, launchTime;
        QStringList log;
    } logData;
};

#endif // EXECBIN_H
