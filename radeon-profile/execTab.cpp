
// copyright marazmista @ 1.06.2014

#include "radeon_profile.h"
#include "ui_radeon_profile.h"
#include "execbin.h"

#include <QFile>
#include <QRegExp>
#include <QMessageBox>
#include <QFileDialog>
#include <QProcess>
#include <QProcessEnvironment>
#include <QInputDialog>

QStringList selectedVariableVaules,envVars;

void radeon_profile::on_btn_cancel_clicked()
{
    ui->execPages->setCurrentIndex(0);

    ui->list_variables->clear();
    ui->list_vaules->clear();
    selectedVariableVaules.clear();

    ui->txt_binary->clear();
    ui->txt_logFile->clear();
    ui->txt_profileName->clear();
    ui->txt_summary->clear();
    ui->txt_binParams->clear();
}

void radeon_profile::on_btn_modifyExecProfile_clicked()
{
    loadVariables();

    ui->label_15->setVisible(false);
    ui->cb_manualEdit->setChecked(false);

    if (ui->list_execProfiles->selectedItems().count() == 0)
        return;

    ui->txt_profileName->setText(ui->list_execProfiles->currentItem()->text(PROFILE_NAME));
    ui->txt_binary->setText(ui->list_execProfiles->currentItem()->text(BINARY));
    ui->txt_binParams->setText(ui->list_execProfiles->currentItem()->text(BINARY_PARAMS));
    ui->txt_logFile->setText(ui->list_execProfiles->currentItem()->text(LOG_FILE));
    ui->txt_summary->setText(ui->list_execProfiles->currentItem()->text(ENV_SETTINGS));
    ui->cb_appendDateTime->setChecked(((ui->list_execProfiles->currentItem()->text(LOG_FILE_DATE_APPEND) == "1") ? true : false));

    if (!ui->txt_summary->text().isEmpty())
        selectedVariableVaules = ui->txt_summary->text().split(" ");
    ui->execPages->setCurrentIndex(1);
}


void radeon_profile::on_btn_ok_clicked()
{
    if (ui->txt_profileName->text().isEmpty()) {
        QMessageBox::critical(this, tr("Error"), tr("Profile name can't be empty!"), QMessageBox::Ok);
        return;
    }

    if (ui->txt_binary->text().isEmpty()) {
        QMessageBox::critical(this, tr("Error"), tr("No binary is selected!"), QMessageBox::Ok);
        return;
    }

    // if only name given, assume it is in /usr/bin
    if (!ui->txt_binary->text().contains('/')) {
        QString tmpS = "/usr/bin/"+ui->txt_binary->text();
        if (QFile::exists(tmpS))
            ui->txt_binary->setText(tmpS);
        else {
            QMessageBox::critical(this, tr("Error"), tr("Binary not found in /usr/bin: ") + ui->txt_binary->text());
            return;
        }
    }

    int modIndex = -1;

    if (ui->list_execProfiles->selectedItems().count() != 0) {
        if (ui->list_execProfiles->currentItem()->text(PROFILE_NAME) == ui->txt_profileName->text()) {
            modIndex = ui->list_execProfiles->indexOfTopLevelItem(ui->list_execProfiles->currentItem());
            delete ui->list_execProfiles->currentItem();
        }
    }

    QTreeWidgetItem *item = new QTreeWidgetItem();
    item->setText(PROFILE_NAME, ui->txt_profileName->text());
    item->setText(BINARY, ui->txt_binary->text());
    item->setText(BINARY_PARAMS, ui->txt_binParams->text());
    item->setText(ENV_SETTINGS,ui->txt_summary->text());
    item->setText(LOG_FILE,ui->txt_logFile->text());
    item->setText(LOG_FILE_DATE_APPEND,((ui->cb_appendDateTime->isChecked()) ? "1" : "0"));

    if (modIndex == -1)
        ui->list_execProfiles->addTopLevelItem(item);
    else
        ui->list_execProfiles->insertTopLevelItem(modIndex,item);

    ui->list_execProfiles->setCurrentItem(item);

    ui->execPages->setCurrentIndex(0);
    on_btn_cancel_clicked();
}

void radeon_profile::on_btn_addExecProfile_clicked()
{
    loadVariables();
    ui->execPages->setCurrentIndex(1);
    ui->label_15->setVisible(false);
    ui->cb_manualEdit->setChecked(false);
}

void radeon_profile::on_list_variables_itemClicked(QListWidgetItem *item)
{
    ui->list_vaules->clear();

    if (item->text().contains("----")) // the separator
        return;

    if (envVars.isEmpty())
        return;

    // read variable possible values from file
    QStringList values = envVars.filter(ui->list_variables->currentItem()->text())[0].remove(ui->list_variables->currentItem()->text()+"|").split("#",QString::SkipEmptyParts);

    // if value for this variable is 'user_input' display a window for input
    if (values[0] == "user_input") {
        bool ok;
        QString input = QInputDialog::getText(this, tr("Enter value"), tr("Enter valid value for ") + ui->list_variables->currentItem()->text(), QLineEdit::Normal,"",&ok);

        // look for this variable in list
        int varIndex = selectedVariableVaules.indexOf(QRegExp(ui->list_variables->currentItem()->text()+".+",Qt::CaseInsensitive),0);
        if (!input.isEmpty() && ok) {
            // if value was ok
            if (varIndex == -1)
                // add it to list
                selectedVariableVaules.append(ui->list_variables->currentItem()->text()+"="+input);
            else
                // replace if already exists
                selectedVariableVaules[varIndex] = ui->list_variables->currentItem()->text()+"=\""+input+"\"";
        } else {
            // hehe, looks weird but check ok status is for, when input was empty, and whether user click ok or cancel, dispaly quesion
            if ((varIndex != -1) || ok) {
                if (QMessageBox::question(this, tr("Question"), tr("Remove this item?"), QMessageBox::Yes | QMessageBox::No,QMessageBox::Yes) == QMessageBox::Yes)
                    selectedVariableVaules.removeAt(varIndex);
            }
        }
        ui->txt_summary->setText(selectedVariableVaules.join(" "));
        return;
    }

    // go through list from file and check if it is selected (exists in summary)
    for (int i= 0 ; i< values.count(); i++ ) {
        // look for selected variable in list with variables and its values
        int varIndex = selectedVariableVaules.indexOf(QRegExp(ui->list_variables->currentItem()->text()+".+",Qt::CaseInsensitive),0);

        QListWidgetItem *valItem = new QListWidgetItem();
        valItem->setText(values[i]);

        // if not, add to list where from user can choose, add unchecked
        if (varIndex == -1) {
            valItem->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            valItem->setCheckState(Qt::Unchecked);
        } else {
            // if it is on list, add checked
            if (selectedVariableVaules[varIndex].contains(valItem->text()))
                valItem->setCheckState(Qt::Checked);
            else {
                // other, unchecked
                valItem->setFlags(item->flags() | Qt::ItemIsUserCheckable);
                valItem->setCheckState(Qt::Unchecked);
            }
        }
        ui->list_vaules->addItem(valItem);
    }
}

void radeon_profile::on_list_vaules_itemClicked(QListWidgetItem *item)
{
    Q_UNUSED(item)
    ui->txt_summary->clear();
    QStringList selectedValues;

    // it rebuild all selected values for current variable
    for (int i = 0; i < ui->list_vaules->count(); i++) {
        if (ui->list_vaules->item(i)->checkState() == Qt::Checked)
            selectedValues.append(ui->list_vaules->item(i)->text());

        int varIndex = selectedVariableVaules.indexOf(QRegExp(ui->list_variables->currentItem()->text()+".+",Qt::CaseInsensitive),0);
        if (varIndex != -1)
            selectedVariableVaules.removeAt(varIndex);

    }
    if (selectedValues.count() != 0)
        selectedVariableVaules.append(ui->list_variables->currentItem()->text()+"="+selectedValues.join(","));

    ui->txt_summary->setText(selectedVariableVaules.join(" "));
}

void radeon_profile::on_btn_removeExecProfile_clicked()
{
    if (ui->list_execProfiles->selectedItems().count() == 0)
        return;

   if (QMessageBox::question(this, tr("Remove"), tr("Remove this item?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
        delete ui->list_execProfiles->selectedItems()[0];
   else
       return;
}

void radeon_profile::on_btn_selectBinary_clicked()
{
    QString binaryPath = QFileDialog::getOpenFileName(this, tr("Select binary"));

    if (!binaryPath.isEmpty())
        ui->txt_binary->setText(binaryPath);
}

void radeon_profile::on_btn_selectLog_clicked()
{
    QString logFile = QFileDialog::getSaveFileName(this, tr("Select log file"), QDir::homePath()+"/"+ui->txt_profileName->text());

    if (!logFile.isEmpty())
        ui->txt_logFile->setText(logFile);
}

void radeon_profile::loadVariables() {
    QFile f(":/var/variables/envVars");
    if (f.open(QIODevice::ReadOnly)) {
        QStringList sl = QString(f.readAll()).split('\n');
        f.close();

        if (sl.isEmpty())
            return;

        envVars = sl;
        for (int i = 0; i< sl.count(); i++)
            ui->list_variables->addItem(sl[i].split('|')[0]);
    }
}

void radeon_profile::on_btn_runExecProfile_clicked()
{
    if (ui->list_execProfiles->selectedItems().count() == 0)
        return;

    QTreeWidgetItem *item = ui->list_execProfiles->currentItem();

    // sets the env for binary
    QProcessEnvironment penv;
    if (ui->cb_execSysEnv->isChecked())
        penv = QProcessEnvironment::systemEnvironment();

    QStringList variables;
    if (!item->text(ENV_SETTINGS).isEmpty()) {
            variables = item->text(ENV_SETTINGS).split(' ',QString::SkipEmptyParts);

            for (int i = 0; i < variables.count(); i++) {
                QString varible = variables[i].split('=')[0],
                        value = variables[i].split('=')[1];
                penv.insert(varible,value);
            }
    }

    execBin *exe = new execBin();
    exe->name = item->text((PROFILE_NAME));
    ui->tabs_execOutputs->addTab(exe->tab,exe->name);

    exe->setEnv(penv);

    if (QFile::exists(item->text(BINARY))) {
        exe->runBin("\""+item->text(BINARY) +"\" " +item->text(BINARY_PARAMS));
        ui->execPages->setCurrentIndex(2);

        //  check if there will be log
        if (!item->text(LOG_FILE).isEmpty()) {
            exe->logEnabled = true;
            exe->setLogFilename(item->text(LOG_FILE) +
                    ((item->text(LOG_FILE_DATE_APPEND) == "1") ? QDateTime::currentDateTime().toString("_yyyy-MM-dd_hh-mm-ss") : ""));
            exe->appendToLog("Profile: " +item->text(PROFILE_NAME) +"; App: " + item->text(BINARY) + "; Params: " + item->text(BINARY_PARAMS) + "; Env: " + item->text(ENV_SETTINGS));
            exe->appendToLog("Date and time; power level; GPU core clk; mem clk; uvd core clk; uvd decoder clk; core voltage (vddc); mem voltage (vddci); temp");
        }
        execsRunning->append(exe);
        ui->tabs_execOutputs->setCurrentIndex(ui->tabs_execOutputs->count() - 1);
    }
    else {
        QMessageBox::critical(this, tr("Error"), tr("Can't run something that not exists!"));
        delete exe;
    }
}

void radeon_profile::on_cb_manualEdit_clicked(bool checked)
{
    ui->txt_summary->setReadOnly(!checked);
    ui->label_15->setVisible(checked);
}

void radeon_profile::on_btn_viewOutput_clicked()
{
    if (ui->tabs_execOutputs->count() == 0)
        return;

    ui->execPages->setCurrentIndex(2);
}

void radeon_profile::btnBackToProfilesClicked()
{
    ui->execPages->setCurrentIndex(0);
}

void radeon_profile::on_list_execProfiles_itemDoubleClicked(QTreeWidgetItem *item, int column) {
    Q_UNUSED(column)
        switch (ui->cb_execDbcAction->currentIndex()) {
        default:
        case 0:
            if (QMessageBox::question(this, tr("Run"), tr("Run: \"") + item->text(0) + "?", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
                ui->btn_runExecProfile->click();
            break;
        case 1:
            ui->btn_modifyExecProfile->click();
            break;
        }
}
