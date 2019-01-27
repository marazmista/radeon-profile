#include "radeon_profile.h"
#include "ui_radeon_profile.h"

#include <QMessageBox>
#include <QMenu>
#include <QInputDialog>


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

QString displayRange(unsigned min, unsigned max) {
    return " [" + QString::number(min) + " - " + QString::number(max) + "]";
}

void radeon_profile::on_list_coreStates_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    OcSeriesType type;

    switch (column) {
        case 0:
            return;

        case 1: {
            int newFrequency = askNumber(item->text(column).toInt(), device.getDriverFeatures().coreRange.min, device.getDriverFeatures().coreRange.max,
                                         tr("Core frequency") + displayRange(device.getDriverFeatures().coreRange.min, device.getDriverFeatures().coreRange.max));

            if (newFrequency == -1)
                return;

            item->setText(column, QString::number(newFrequency));
            type = OcSeriesType::CORE_FREQUENCY;
            break;
        }
        case 2: {
            int newVoltage = askNumber(item->text(column).toInt(), device.getDriverFeatures().voltageRange.min, device.getDriverFeatures().voltageRange.max,
                                       tr("Core voltage") + displayRange(device.getDriverFeatures().voltageRange.min, device.getDriverFeatures().voltageRange.max));

            if (newVoltage == -1)
                return;

            item->setText(column, QString::number(newVoltage));
            type = OcSeriesType::CORE_VOLTAGE;
            break;
        }
    }

    if (device.currentPowerLevel != ForcePowerLevels::F_MANUAL)
        device.setForcePowerLevel(ForcePowerLevels::F_MANUAL);

    device.setOcTableValue("s", ui->list_coreStates->currentIndex().row(), FreqVoltPair(item->text(1).toUInt(), item->text(2).toUInt()));
    updateOcGraphSeries(device.getDriverFeatures().coreTable, static_cast<QLineSeries*>(chartView_oc->chart()->series()[type]), type);

    ocTableModified = true;
}

void radeon_profile::on_list_memStates_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    OcSeriesType type;

    switch (column) {
        case 0:
            return;

        case 1: {
            int newFrequency = askNumber(item->text(column).toInt(), device.getDriverFeatures().memRange.min, device.getDriverFeatures().memRange.max,
                                         tr("Memory frequency") + displayRange(device.getDriverFeatures().memRange.min, device.getDriverFeatures().memRange.max));

            if (newFrequency == -1)
                return;

            item->setText(column, QString::number(newFrequency));
            type = OcSeriesType::MEM_FREQUENCY;
            break;
        }
        case 2: {
            int newVoltage = askNumber(item->text(column).toInt(), device.getDriverFeatures().voltageRange.min, device.getDriverFeatures().voltageRange.max,
                                       tr("Memory voltage") + displayRange(device.getDriverFeatures().voltageRange.min, device.getDriverFeatures().voltageRange.max));

            if (newVoltage == -1)
                return;

            item->setText(column, QString::number(newVoltage));
            type = OcSeriesType::MEM_VOLTAGE;
            break;
        }
    }

    if (device.currentPowerLevel != ForcePowerLevels::F_MANUAL)
        device.setForcePowerLevel(ForcePowerLevels::F_MANUAL);

    device.setOcTableValue("m", ui->list_memStates->currentIndex().row(), FreqVoltPair(item->text(1).toUInt(), item->text(2).toUInt()));
    updateOcGraphSeries(device.getDriverFeatures().coreTable, static_cast<QLineSeries*>(chartView_oc->chart()->series()[type]), type);
    ocTableModified = true;
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
