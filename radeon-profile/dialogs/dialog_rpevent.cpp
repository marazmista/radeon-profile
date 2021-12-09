#include "dialog_rpevent.h"
#include "ui_dialog_rpevent.h"
#include "radeon_profile.h"
#include "globalStuff.h"

#include <QLineEdit>
#include <QMap>
#include <QMessageBox>
#include <QFileDialog>

Dialog_RPEvent::Dialog_RPEvent(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog_RPEvent)
{
    ui->setupUi(this);
    setFixedSize(size());

    ui->spin_fixedFanSpeed->setVisible(false);
}

void Dialog_RPEvent::setFeatures(const GPUDataContainer &gpuData, const DriverFeatures &features, const QList<QString> &profiles) {
    for (const ValueID::Instance instance : features.tempSensors) {
        const ValueID id(ValueID::TEMPERATURE_CURRENT, instance);
        ui->combo_sensorInstance->addItem(globalStuff::getNameOfValueID(id), QVariant(instance));
    }

    switch (features.currentPowerMethod) {
        case PowerMethod::DPM:
            ui->combo_powerLevelChange->addItems(globalStuff::createPowerLevelCombo(features.sysInfo.module));
            break;
        case PowerMethod::PROFILE:
            ui->combo_powerLevelChange->setVisible(false);
            ui->l_powerLevel->setVisible(false);

            ui->l_profile->setText(tr("Set Profile to:"));
            break;
        default:
            break;
    }

    ui->combo_powerProfileChange->addItems([&features]()
    { QStringList profilesList;
        for (auto &p : features.powerProfiles)
            profilesList.append(p.name);
        return profilesList;
    }());

    ui->combo_fanChange->addItem(tr("Auto"));
    ui->combo_fanChange->addItem(tr("Fixed speed"));

    for (QString p : profiles)
        ui->combo_fanChange->addItem(p);

    if (!gpuData.contains(ValueID::FAN_SPEED_PERCENT)) {
        ui->combo_fanChange->setVisible(false);
        ui->l_fan->setVisible(false);
    }
}

Dialog_RPEvent::~Dialog_RPEvent()
{
    delete ui;
}

void Dialog_RPEvent::on_btn_cancel_clicked()
{
    this->reject();
}

void Dialog_RPEvent::on_btn_save_clicked()
{
    if (ui->edt_eventName->text().isEmpty())
        return;

    this->setResult(QDialog::Accepted);

    switch (ui->combo_eventTrigger->currentIndex()) {
        case 0:
            createdEvent.type = RPEventType::TEMPERATURE;
            createdEvent.sensorInstance = ui->combo_sensorInstance->currentData().value<ValueID::Instance>();
            break;
        case 1:
            createdEvent.type = RPEventType::BINARY;

            if (ui->edt_binary->text().isEmpty()) {
                QMessageBox::information(this, "", tr("Selected trigger type is Binary, so the binary field cannot be empty."),QMessageBox::Ok);
                return;
            }
            break;
    }

    createdEvent.enabled = ui->cb_enabled->isChecked();
    createdEvent.name = ui->edt_eventName->text();

    createdEvent.activationBinary = ui->edt_binary->text();
    createdEvent.activationTemperature = ui->spin_tempActivate->value();

    createdEvent.powerProfileChange = (ui->combo_powerProfileChange->currentIndex() == 0) ? "" : QString::number(ui->combo_powerProfileChange->currentIndex() - 1);
    createdEvent.powerLevelChange = (ui->combo_powerLevelChange->currentIndex() == 0) ? "" : ui->combo_powerLevelChange->currentText();

    createdEvent.fixedFanSpeedChange = ui->spin_fixedFanSpeed->value();
    createdEvent.fanComboIndex = ui->combo_fanChange->currentIndex();

    if (ui->combo_fanChange->currentIndex() > 1)
        createdEvent.fanProfileNameChange = ui->combo_fanChange->currentText();

    this->accept();
}

RPEvent Dialog_RPEvent::getCreatedEvent() {
    return createdEvent;
}

void Dialog_RPEvent::setEditedEvent(const RPEvent &rpe) {
    createdEvent = rpe;

    ui->combo_eventTrigger->setCurrentIndex(rpe.type);
    ui->combo_sensorInstance->setCurrentIndex(ui->combo_sensorInstance->findData(rpe.sensorInstance));
    ui->cb_enabled->setChecked(rpe.enabled);
    ui->edt_eventName->setText(rpe.name);
    ui->spin_tempActivate->setValue(rpe.activationTemperature);
    ui->edt_binary->setText(rpe.activationBinary);
    ui->combo_powerProfileChange->setCurrentIndex(rpe.powerProfileChange.isEmpty() ? 0 : rpe.powerProfileChange.toInt() + 1);
    ui->combo_powerLevelChange->setCurrentText(rpe.powerLevelChange);

    if (rpe.fanComboIndex > 1)
        ui->combo_fanChange->setCurrentText(rpe.fanProfileNameChange);
    else
        ui->combo_fanChange->setCurrentIndex(rpe.fanComboIndex);

    ui->spin_fixedFanSpeed->setValue(rpe.fixedFanSpeedChange);
}

void Dialog_RPEvent::on_combo_fanChange_currentIndexChanged(int index)
{
    ui->spin_fixedFanSpeed->setVisible(index == 2);
}

void Dialog_RPEvent::on_combo_eventTrigger_currentIndexChanged(int index)
{
    ui->combo_sensorInstance->setVisible(index == 0);
    ui->label_sensorInstance->setVisible(index == 0);
}

void Dialog_RPEvent::on_btn_setBinary_clicked()
{
    QString binaryPath = QFileDialog::getOpenFileName(this, tr("Select binary"),
                                                      (!ui->edt_binary->text().isEmpty()) ? QFileInfo(ui->edt_binary->text()).absoluteFilePath() : "");

    if (!binaryPath.isEmpty())
        ui->edt_binary->setText(binaryPath);
}
