#include "radeon_profile.h"
#include "ui_radeon_profile.h"

#include "dialogs/dialog_sliders.h"

#include <QMessageBox>
#include <QMenu>

bool tableHasBeenModified = true;

FVTable listToMap(const QTreeWidget *list) {
    FVTable fvt;
    for (auto i = 0; i < list->topLevelItemCount(); ++i)
        fvt.insert(i, FreqVoltPair(list->topLevelItem(i)->text(1).toUInt(), list->topLevelItem(i)->text(2).toUInt()));

    return fvt;
}

void radeon_profile::loadDefaultOcTables(const DriverFeatures &features)
{
    if (features.isVDDCCurveAvailable) {
        loadListFromOcProfile(features.currentStatesTables.value(OD_VDDC_CURVE), ui->list_coreStates);
    } else {
        loadListFromOcProfile(features.currentStatesTables.value(OD_SCLK), ui->list_coreStates);
        loadListFromOcProfile(features.currentStatesTables.value(OD_MCLK), ui->list_memStates);
    }

    ocProfiles.insert("default", createOcProfile());
}

void radeon_profile::loadListFromOcProfile(const FVTable &table, QTreeWidget *list) {
    for (auto k : table.keys()) {
        list->addTopLevelItem(new QTreeWidgetItem(QStringList() << QString::number(k)
                              << QString::number(table.value(k).frequency)
                              << QString::number(table.value(k).voltage)));
    }
}

void radeon_profile::createOcGraphSeriesFromList(const QTreeWidget *list, QLineSeries *seriesClocks, QLineSeries *seriesVoltages) {

    seriesClocks->clear();
    seriesVoltages->clear();

    for (auto i = 0; i < list->topLevelItemCount(); ++i) {
        seriesClocks->append(i, list->topLevelItem(i)->text(1).toUInt());
        seriesVoltages->append(i, list->topLevelItem(i)->text(2).toUInt());
    }
}

void radeon_profile::adjustState(QTreeWidgetItem *item, const OCRange &frequencyRange, const OCRange &voltageRange) {

    auto d = new Dialog_sliders(tr("Adjust state"), this);

    d->addSlider(tr("Frequency"), "MHz", frequencyRange.min, frequencyRange.max, item->text(1).toUInt());
    d->addSlider(tr("Voltage"), "mV", voltageRange.min, voltageRange.max, item->text(2).toUInt());

    if (d->exec() == QDialog::Rejected) {
        delete d;
        return;
    }

    item->setText(1, QString::number(d->getValue(0)));
    item->setText(2, QString::number(d->getValue(1)));

    tableHasBeenModified = true;
    ui->l_ocProfileUnsavedInficator->setVisible(true);
    delete d;
}

void radeon_profile::on_list_coreStates_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);

    if (device.getDriverFeatures().isVDDCCurveAvailable) {
        adjustState(item,  device.getDriverFeatures().ocRages.value("VDDC_CURVE_SCLK["+QString::number(ui->list_coreStates->currentIndex().row())+"]"),
                    device.getDriverFeatures().ocRages.value("VDDC_CURVE_VOLT["+QString::number(ui->list_coreStates->currentIndex().row())+"]"));
    } else
        adjustState(item, device.getDriverFeatures().ocRages.value(SCLK), device.getDriverFeatures().ocRages.value(VDDC));


    createOcGraphSeriesFromList(ui->list_coreStates, static_cast<QLineSeries*>(chartView_oc->chart()->series()[OcSeriesType::CORE_FREQUENCY]),
            static_cast<QLineSeries*>(chartView_oc->chart()->series()[OcSeriesType::CORE_VOLTAGE]));
}

void radeon_profile::on_list_memStates_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);

    adjustState(item, device.getDriverFeatures().ocRages.value(MCLK), device.getDriverFeatures().ocRages.value(VDDC));

    createOcGraphSeriesFromList(ui->list_memStates, static_cast<QLineSeries*>(chartView_oc->chart()->series()[OcSeriesType::MEM_FREQUENCY]),
            static_cast<QLineSeries*>(chartView_oc->chart()->series()[OcSeriesType::MEM_VOLTAGE]));
}

void radeon_profile::on_btn_applyOcTable_clicked()
{
    if (ui->l_ocProfileUnsavedInficator->isVisible()) {
        if (!askConfirmation("", tr("Cannot apply unsaved profile. Do you want to save it?")))
            return;

        on_btn_saveOcProfile_clicked();
    }

    setCurrentOcProfile(ui->combo_ocProfiles->currentText());
}

void radeon_profile::on_btn_resetOcTable_clicked()
{
    device.sendOcTableCommand("r");
    device.readOcTableAndRanges();

    auto &ocp = ocProfiles[ui->combo_ocProfiles->currentText()];
    ocp.tables = device.getDriverFeatures().currentStatesTables;
    createOcProfileListsAndGraph(ui->combo_ocProfiles->currentText());

    ui->l_ocProfileUnsavedInficator->setVisible(true);
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
        device.setOcRanges("m", OD_MCLK, 1, d->getValue(2));

    delete d;
}


OCProfile radeon_profile::createOcProfile() {
    OCProfile ocp;

    ocp.powerCap = ui->slider_powerCap->value();

    if (device.getDriverFeatures().isVDDCCurveAvailable)
        ocp.tables.insert(OD_VDDC_CURVE, listToMap(ui->list_coreStates));
    else {
        ocp.tables.insert(OD_SCLK, listToMap(ui->list_coreStates));
        ocp.tables.insert(OD_MCLK, listToMap(ui->list_memStates));
    }

    return ocp;
}

void radeon_profile::on_btn_saveOcProfile_clicked()
{
    ocProfiles.insert(ui->combo_ocProfiles->currentText(), createOcProfile());
    saveConfig();

    ui->l_ocProfileUnsavedInficator->setVisible(false);
}

void radeon_profile::on_btn_saveOcProfileAs_clicked()
{
    QString name =  QInputDialog::getText(this, "", tr("Overclock profile name:"));

    if (name.isEmpty())
        return;

    if (ocProfiles.contains(name)) {
        QMessageBox::information(this, "", tr("Cannot add another profile with the same name that already exists."), QMessageBox::Ok);
        return;
    }

    ocProfiles.insert(name, createOcProfile());
    ui->combo_ocProfiles->addItem(name);
    ui->combo_ocProfiles->setCurrentText(name);

    saveConfig();
    createOcProfilesMenu(true);
    ui->btn_ocProfileControl->menu()->actions()[findCurrentMenuIndex(ui->btn_ocProfileControl->menu(), name)]->setChecked(true);
    ui->l_ocProfileUnsavedInficator->setVisible(false);
}

void radeon_profile::createOcProfileListsAndGraph(const QString &arg1)
{
    ui->list_coreStates->clear();
    ui->list_memStates->clear();

    const auto ocp = ocProfiles.value(arg1);

    ui->slider_powerCap->setValue(ocp.powerCap);

    auto axis_frequency = static_cast<QValueAxis*>(chartView_oc->chart()->axes(Qt::Vertical)[AxisType::FREQUENCY]);
    auto axis_volts = static_cast<QValueAxis*>(chartView_oc->chart()->axes(Qt::Vertical)[AxisType::VOLTAGE]);
    auto axis_state = static_cast<QValueAxis*>(chartView_oc->chart()->axes(Qt::Horizontal)[0]);

    if (device.getDriverFeatures().isVDDCCurveAvailable) {
        loadListFromOcProfile(ocp.tables.value(OD_VDDC_CURVE), ui->list_coreStates);

        axis_state->setMax(2);
        axis_state->setTickCount(3);

        axis_volts->setRange(device.getDriverFeatures().ocRages.value("VDDC_CURVE_VOLT[2]").min - 100,
                             device.getDriverFeatures().ocRages.value("VDDC_CURVE_VOLT[2]").max + 100);

    } else {
        loadListFromOcProfile(ocp.tables.value(OD_SCLK), ui->list_coreStates);
        loadListFromOcProfile(ocp.tables.value(OD_MCLK), ui->list_memStates);

        createOcGraphSeriesFromList(ui->list_memStates, static_cast<QLineSeries*>(chartView_oc->chart()->series()[OcSeriesType::MEM_FREQUENCY]),
                static_cast<QLineSeries*>(chartView_oc->chart()->series()[OcSeriesType::MEM_VOLTAGE]));

        axis_state->setMax(device.getDriverFeatures().currentStatesTables.value(OD_SCLK).lastKey());
        axis_state->setTickCount(device.getDriverFeatures().currentStatesTables.value(OD_SCLK).keys().count());

        axis_volts->setMax(device.getDriverFeatures().ocRages.value(VDDC).max + 100);
    }

    createOcGraphSeriesFromList(ui->list_coreStates, static_cast<QLineSeries*>(chartView_oc->chart()->series()[OcSeriesType::CORE_FREQUENCY]),
            static_cast<QLineSeries*>(chartView_oc->chart()->series()[OcSeriesType::CORE_VOLTAGE]));


    axis_frequency->setMax((device.getDriverFeatures().ocRages.value(SCLK).max > device.getDriverFeatures().ocRages.value(MCLK).max)
                             ? device.getDriverFeatures().ocRages.value(SCLK).max + 100
                             : device.getDriverFeatures().ocRages.value(MCLK).max + 100);

    axis_frequency->setTickCount(8);
    axis_volts->setTickCount(8);

    ui->l_ocProfileUnsavedInficator->setVisible(false);
    tableHasBeenModified = true;
}

void radeon_profile::on_btn_removeOcProfile_clicked()
{
    if (ui->combo_ocProfiles->currentText() == "default") {
        QMessageBox::information(this, "", tr("Cannot remove default profile."), QMessageBox::Ok);
        return;
    }

    if (!askConfirmation("", tr("Remove profile: ")+ui->combo_ocProfiles->currentText() + "?"))
        return;

    ocProfiles.remove(ui->combo_ocProfiles->currentText());
    ui->combo_ocProfiles->removeItem(ui->combo_ocProfiles->currentIndex());
    createOcProfilesMenu(true);

    setCurrentOcProfile("default");
    saveConfig();
}

void radeon_profile::setCurrentOcProfile(const QString &name) {
    const auto &ocp = ocProfiles.value(name);

    if (tableHasBeenModified) {
        if (ui->combo_pLevel->currentText() != level_manual)
            device.setForcePowerLevel(level_manual);

        if (device.getDriverFeatures().isVDDCCurveAvailable)
            device.setOcTable("vc", ocp.tables.value(OD_VDDC_CURVE));
        else {
            device.setOcTable("s", ocp.tables.value(OD_SCLK));
            device.setOcTable("m", ocp.tables.value(OD_MCLK));
        }

        device.sendOcTableCommand("c");
    }

    if (device.getDriverFeatures().isPowerCapAvailable)
        device.setPowerCap(ocp.powerCap);

    ui->l_currentOcProfile->setText(name);
    ui->btn_ocProfileControl->menu()->actions()[findCurrentMenuIndex(ui->btn_ocProfileControl->menu(), name)]->setChecked(true);
    ui->btn_ocProfileControl->setText(name);

    tableHasBeenModified = false;

    // refresh states table after overclock
    device.refreshPowerPlayTables();
    updateFrequencyStatesTables();
}

void radeon_profile::powerCapValueChange(int value)
{
    Q_UNUSED(value)

    ui->l_ocProfileUnsavedInficator->setVisible(true);
}

void radeon_profile::percentOverclockToggled(bool toggle) {
    if (!device.isInitialized())
        return;

    if (toggle)
        applyOc();
    else
        device.resetOverclock();
}

void radeon_profile::frequencyControlToggled(bool toggle)
{
    if (!device.isInitialized())
        return;

    if (toggle)
        applyFrequencyTables();
    else
        device.resetFrequencyControlStates();
}

void radeon_profile::applyOc()
{
    device.setOverclockValue(device.getDriverFiles().sysFs.pp_sclk_od, ui->slider_ocSclk->value());
    device.setOverclockValue(device.getDriverFiles().sysFs.pp_mclk_od, ui->slider_ocMclk->value());

    // refresh states table after overclock
    device.refreshPowerPlayTables();
    updateFrequencyStatesTables();
}

void radeon_profile::applyFrequencyTables() {
    bool allCheckedCore = true, allCheckedMem = true;

    enabledFrequencyStatesCore = "";
    enabledFrequencyStatesMem = "";

    for (int i = 0; i < ui->list_freqStatesCore->count(); ++i) {
        if (ui->list_freqStatesCore->item(i)->checkState() == Qt::Checked)
            enabledFrequencyStatesCore.append(QString::number(i) + " ");
        else
            allCheckedCore = false;
    }

    if (enabledFrequencyStatesCore.isEmpty()) {
        QMessageBox::warning(this, "", "At least one core state has to be enabled.");
        return;
    }

    for (int i = 0; i < ui->list_freqStatesMem->count(); ++i) {
        if (ui->list_freqStatesMem->item(i)->checkState() == Qt::Checked)
            enabledFrequencyStatesMem.append(QString::number(i) + " ");
        else
            allCheckedMem = false;
    }

    if (enabledFrequencyStatesMem.isEmpty()) {
        QMessageBox::warning(this, "", "At least one memory state has to be enabled.");
        return;
    }


    if ((!allCheckedCore || !allCheckedMem) && ui->combo_pLevel->currentText() != level_manual)
        device.setForcePowerLevel(level_manual);


    device.setManualFrequencyControlStates(device.getDriverFiles().sysFs.pp_dpm_sclk, enabledFrequencyStatesCore);
    device.setManualFrequencyControlStates(device.getDriverFiles().sysFs.pp_dpm_mclk, enabledFrequencyStatesMem);
}

void radeon_profile::on_btn_applyStatesAndOc_clicked() {
    if (ui->group_oc->isChecked())
        applyOc();

    if (ui->group_freq->isChecked())
        applyFrequencyTables();

}
void radeon_profile::ocProfilesMenuActionClicked(QAction *a) {
    if (a->isSeparator())
        return;

    if (a->text() == ui->l_currentOcProfile->text())
        return;

    tableHasBeenModified = true;
    setCurrentOcProfile(a->text());
}

void radeon_profile::loadFrequencyStatesTables()
{
    for (const QString &s : device.getDriverFeatures().sclkTable) {
        QListWidgetItem *item = new QListWidgetItem(ui->list_freqStatesCore);
        item->setCheckState((enabledFrequencyStatesCore.contains(s.left(1))) ? Qt::Checked : Qt::Unchecked);
        item->setText(s);
        ui->list_freqStatesCore->addItem(item);
    }

    for (const QString &s : device.getDriverFeatures().mclkTable) {
        QListWidgetItem *item = new QListWidgetItem(ui->list_freqStatesMem);
        item->setCheckState((enabledFrequencyStatesMem.contains(s.left(1))) ? Qt::Checked : Qt::Unchecked);
        item->setText(s);
        ui->list_freqStatesMem->addItem(item);
    }
}

void radeon_profile::updateFrequencyStatesTables()
{
    for (int i = 0; i < ui->list_freqStatesCore->count(); ++i)
        ui->list_freqStatesCore->item(i)->setText(device.getDriverFeatures().sclkTable.at(i));

    for (int i = 0; i < ui->list_freqStatesMem->count(); ++i)
        ui->list_freqStatesMem->item(i)->setText(device.getDriverFeatures().mclkTable.at(i));
}
