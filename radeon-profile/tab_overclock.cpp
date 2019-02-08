#include "radeon_profile.h"
#include "ui_radeon_profile.h"

#include "dialogs/dialog_sliders.h"

#include <QMessageBox>
#include <QMenu>

void loadOcTable(const FVTable &table, QTreeWidget *list, QLineSeries *series_Freq, QLineSeries *series_Voltage) {
    for (const unsigned k :  table.keys()) {
        const FreqVoltPair &fvp = table.value(k);

        series_Freq->append(k, fvp.frequency);
        series_Voltage->append(k, fvp.voltage);

        list->addTopLevelItem(new QTreeWidgetItem(QStringList() << QString::number(k)
                                                                << QString::number(fvp.frequency)
                                                                << QString::number(fvp.voltage)));
    }
}

void radeon_profile::updateOcGraphSeries(const FVTable &table, QLineSeries *series, OcSeriesType type) {

    series->clear();

    for (const unsigned k: table.keys()) {
        switch (type) {
            case OcSeriesType::MEM_FREQUENCY:
            case OcSeriesType::CORE_FREQUENCY:
                series->append(k, table.value(k).frequency);
                break;

            case OcSeriesType::MEM_VOLTAGE:
            case OcSeriesType::CORE_VOLTAGE:
                series->append(k, table.value(k).voltage);
                break;
        }
    }

}


void radeon_profile::setupOcTableOverclock() {
    ui->tw_overclock->setTabEnabled(1, true);

    loadOcTable(device.getDriverFeatures().coreTable, ui->list_coreStates,
                static_cast<QLineSeries*>(chartView_oc->chart()->series()[OcSeriesType::CORE_FREQUENCY]),
                static_cast<QLineSeries*>(chartView_oc->chart()->series()[OcSeriesType::CORE_VOLTAGE]));

    loadOcTable(device.getDriverFeatures().memTable, ui->list_memStates,
                static_cast<QLineSeries*>(chartView_oc->chart()->series()[OcSeriesType::MEM_FREQUENCY]),
                static_cast<QLineSeries*>(chartView_oc->chart()->series()[OcSeriesType::MEM_VOLTAGE]));

    auto axis_frequency = static_cast<QValueAxis*>(chartView_oc->chart()->axes(Qt::Vertical)[AxisType::FREQUENCY]);
    auto axis_volts = static_cast<QValueAxis*>(chartView_oc->chart()->axes(Qt::Vertical)[AxisType::VOLTAGE]);
    auto axis_state = static_cast<QValueAxis*>(chartView_oc->chart()->axes(Qt::Horizontal)[0]);

    axis_frequency->setMax((device.getDriverFeatures().coreRange.max > device.getDriverFeatures().memRange.max)
                             ? device.getDriverFeatures().coreRange.max + 100 : device.getDriverFeatures().memRange.max + 100);
    axis_volts->setMax(device.getDriverFeatures().voltageRange.max + 100);

    axis_state->setMax(device.getDriverFeatures().coreTable.lastKey());
    axis_state->setTickCount(device.getDriverFeatures().coreTable.keys().count());

    axis_frequency->setTickCount(8);
    axis_volts->setTickCount(8);
}

void radeon_profile::adjustState(const QTreeWidget *list, QTreeWidgetItem *item, const OCRange &frequencyRange, const OCRange &voltageRange,
                                 OcSeriesType frequencyType, OcSeriesType voltageType, const FVTable &table, const QString stateTypeString) {

    auto index = list->currentIndex().row();

    DialogSlidersConfig dialogConfig;

    dialogConfig.configureSet(DialogSet::FIRST, tr("Frequency"), "MHz", frequencyRange.min, frequencyRange.max,
                              item->text(1).toUInt());

    dialogConfig.configureSet(DialogSet::SECOND, tr("Voltage"), "mV", voltageRange.min, voltageRange.max,
                              item->text(2).toUInt());

    auto d = new Dialog_sliders(dialogConfig, tr("Adjust state"), this);

    if (d->exec() == QDialog::Rejected) {
        delete d;
        return;
    }

    item->setText(1, QString::number(d->getValue(DialogSet::FIRST)));
    item->setText(2, QString::number(d->getValue(DialogSet::SECOND)));

    if (device.currentPowerLevel != ForcePowerLevels::F_MANUAL)
        device.setForcePowerLevel(ForcePowerLevels::F_MANUAL);

    device.setOcTableValue(stateTypeString, index, FreqVoltPair(item->text(1).toUInt(), item->text(2).toUInt()));
    updateOcGraphSeries(table, static_cast<QLineSeries*>(chartView_oc->chart()->series()[frequencyType]), frequencyType);
    updateOcGraphSeries(table, static_cast<QLineSeries*>(chartView_oc->chart()->series()[voltageType]), voltageType);

    ocTableModified = true;
    delete d;
}

void radeon_profile::on_list_coreStates_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);

    adjustState(ui->list_coreStates, item, device.getDriverFeatures().coreRange, device.getDriverFeatures().voltageRange,
                OcSeriesType::CORE_FREQUENCY, OcSeriesType::CORE_VOLTAGE, device.getDriverFeatures().coreTable, "s");
}

void radeon_profile::on_list_memStates_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);

    adjustState(ui->list_memStates, item, device.getDriverFeatures().memRange, device.getDriverFeatures().voltageRange,
                OcSeriesType::MEM_FREQUENCY, OcSeriesType::MEM_VOLTAGE, device.getDriverFeatures().memTable, "m");
}

void radeon_profile::on_btn_applyOcTable_clicked()
{
    if (device.currentPowerLevel != ForcePowerLevels::F_MANUAL)
        device.setForcePowerLevel(ForcePowerLevels::F_MANUAL);

    if (ocTableModified)
        device.sendOcTableCommand("c");

    ocTableModified = false;

    if (device.getDriverFeatures().isPowerCapAvailable && device.gpuData[ValueID::POWER_CAP_CURRENT].value != ui->slider_powerCap->value())
        device.setPowerCap(ui->slider_powerCap->value());
}

void radeon_profile::on_btn_resetOcTable_clicked()
{
    device.sendOcTableCommand("r");
    device.loadOcTable();

    ocTableModified = false;
}

void radeon_profile::on_slider_powerCap_valueChanged(int value)
{
    ui->l_powerCapMax->setText(QString::number(value) + "W");
}
