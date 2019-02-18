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

    auto axis_frequency = static_cast<QValueAxis*>(chartView_oc->chart()->axes(Qt::Vertical)[AxisType::FREQUENCY]);
    auto axis_volts = static_cast<QValueAxis*>(chartView_oc->chart()->axes(Qt::Vertical)[AxisType::VOLTAGE]);
    auto axis_state = static_cast<QValueAxis*>(chartView_oc->chart()->axes(Qt::Horizontal)[0]);

    if (device.getDriverFeatures().isVDDCCurveAvailable) {
        loadOcTable(device.getDriverFeatures().statesTables.value(OD_VDDC_CURVE), ui->list_coreStates,
                    static_cast<QLineSeries*>(chartView_oc->chart()->series()[OcSeriesType::CORE_FREQUENCY]),
                    static_cast<QLineSeries*>(chartView_oc->chart()->series()[OcSeriesType::CORE_VOLTAGE]));

        axis_state->setMax(2);
        axis_state->setTickCount(3);

        axis_volts->setRange(device.getDriverFeatures().ocRages.value("VDDC_CURVE_VOLT[2]").min - 100,
                             device.getDriverFeatures().ocRages.value("VDDC_CURVE_VOLT[2]").max + 100);

    } else {

        loadOcTable(device.getDriverFeatures().statesTables.value(OD_SCLK), ui->list_coreStates,
                    static_cast<QLineSeries*>(chartView_oc->chart()->series()[OcSeriesType::CORE_FREQUENCY]),
                    static_cast<QLineSeries*>(chartView_oc->chart()->series()[OcSeriesType::CORE_VOLTAGE]));

        loadOcTable(device.getDriverFeatures().statesTables.value(OD_MCLK), ui->list_memStates,
                    static_cast<QLineSeries*>(chartView_oc->chart()->series()[OcSeriesType::MEM_FREQUENCY]),
                    static_cast<QLineSeries*>(chartView_oc->chart()->series()[OcSeriesType::MEM_VOLTAGE]));

        axis_state->setMax(device.getDriverFeatures().statesTables.value(OD_SCLK).lastKey());
        axis_state->setTickCount(device.getDriverFeatures().statesTables.value(OD_SCLK).keys().count());

        axis_volts->setMax(device.getDriverFeatures().ocRages.value(VDDC).max + 100);
    }

    axis_frequency->setMax((device.getDriverFeatures().ocRages.value(SCLK).max > device.getDriverFeatures().ocRages.value(MCLK).max)
                             ? device.getDriverFeatures().ocRages.value(SCLK).max + 100
                             : device.getDriverFeatures().ocRages.value(MCLK).max + 100);

    axis_frequency->setTickCount(8);
    axis_volts->setTickCount(8);
}

void radeon_profile::adjustState(const int index, QTreeWidgetItem *item, const OCRange &frequencyRange, const OCRange &voltageRange,
                                 OcSeriesType frequencyType, OcSeriesType voltageType, const QString &tableKey, const QString stateTypeString) {

    auto d = new Dialog_sliders(tr("Adjust state"), this);

    d->addSlider(tr("Frequency"), "MHz", frequencyRange.min, frequencyRange.max, item->text(1).toUInt());
    d->addSlider(tr("Voltage"), "mV", voltageRange.min, voltageRange.max, item->text(2).toUInt());

    if (d->exec() == QDialog::Rejected) {
        delete d;
        return;
    }

    item->setText(1, QString::number(d->getValue(0)));
    item->setText(2, QString::number(d->getValue(1)));

    if (device.currentPowerLevel != ForcePowerLevels::F_MANUAL)
        device.setForcePowerLevel(ForcePowerLevels::F_MANUAL);

    device.setOcTableValue(stateTypeString, tableKey, index, FreqVoltPair(item->text(1).toUInt(), item->text(2).toUInt()));
    updateOcGraphSeries(device.getDriverFeatures().statesTables.value(tableKey), static_cast<QLineSeries*>(chartView_oc->chart()->series()[frequencyType]), frequencyType);
    updateOcGraphSeries(device.getDriverFeatures().statesTables.value(tableKey), static_cast<QLineSeries*>(chartView_oc->chart()->series()[voltageType]), voltageType);

    ocTableModified = true;
    delete d;
}

void radeon_profile::on_list_coreStates_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);

    if (device.getDriverFeatures().isVDDCCurveAvailable) {
        adjustState(ui->list_coreStates->currentIndex().row(), item,  device.getDriverFeatures().ocRages.value("VDDC_CURVE_SCLK["+QString::number(ui->list_coreStates->currentIndex().row())+"]"),
                    device.getDriverFeatures().ocRages.value("VDDC_CURVE_VOLT["+QString::number(ui->list_coreStates->currentIndex().row())+"]"),
                    OcSeriesType::CORE_FREQUENCY, OcSeriesType::CORE_VOLTAGE, OD_VDDC_CURVE, "vc");
    } else
        adjustState(ui->list_coreStates->currentIndex().row(), item, device.getDriverFeatures().ocRages.value(SCLK), device.getDriverFeatures().ocRages.value(VDDC),
                    OcSeriesType::CORE_FREQUENCY, OcSeriesType::CORE_VOLTAGE, SCLK, "s");
}

void radeon_profile::on_list_memStates_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);

    adjustState(ui->list_memStates->currentIndex().row(), item, device.getDriverFeatures().ocRages.value(MCLK), device.getDriverFeatures().ocRages.value(VDDC),
                OcSeriesType::MEM_FREQUENCY, OcSeriesType::MEM_VOLTAGE, MCLK, "m");
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

void radeon_profile::on_btn_setOcRanges_clicked()
{
    auto d = new Dialog_sliders("Set ranges", this);
    auto coreRange = device.getDriverFeatures().ocRages.value(SCLK);
    auto currentRanges = device.getDriverFeatures().ocRages.value(OD_SCLK);

    d->addSlider(tr("Minimum core clock"), "MHz", coreRange.min , coreRange.max, currentRanges.min);
    d->addSlider(tr("Maximum core clock"), "MHz", coreRange.min, coreRange.max, currentRanges.max);
    d->addSlider(tr("Maximum memory clock"), "MHz", device.getDriverFeatures().ocRages.value(MCLK).min,
                 device.getDriverFeatures().ocRages.value(MCLK).max, device.getDriverFeatures().ocRages.value(OD_MCLK).max);

    if (d->exec() == QDialog::Rejected) {
        delete d;
        return;
    }

    if (d->getValue(0) != currentRanges.min)
        device.setOcRanges("s", OD_SCLK, 0, d->getValue(0));

    if (d->getValue(1) != currentRanges.max)
        device.setOcRanges("s", OD_SCLK, 1, d->getValue(1));

    if (d->getValue(2) != device.getDriverFeatures().ocRages.value(OD_MCLK).max)
        device.setOcRanges("m", OD_SCLK, 1, d->getValue(2));

    delete d;
}
