// copyright marazmista @ 29.12.2013

// this file contains functions for handle gui events, clicks and others

#include "radeon_profile.h"
#include "ui_radeon_profile.h"

#include <QTimer>
#include <QColorDialog>
#include <QClipboard>
#include <QInputDialog>
#include <QMessageBox>
#include <QFileDialog>


//===================================
// === GUI events === //
// == menu forcePowerLevel
void radeon_profile::forceAuto() {
    ui->combo_pLevel->setCurrentIndex(F_AUTO);
}

void radeon_profile::forceLow() {
    ui->combo_pLevel->setCurrentIndex(F_LOW);
}

void radeon_profile::forceHigh() {
    ui->combo_pLevel->setCurrentIndex(F_HIGH);
}

// == buttons for forcePowerLevel
void radeon_profile::on_btn_forceAuto_clicked()
{
    logEvent "btn_forceAuto clicked, changing power level";
    forceAuto();
}

void radeon_profile::on_btn_forceHigh_clicked()
{
    logEvent "btn_forceHigh clicked, changing power level";
    forceHigh();
}

void radeon_profile::on_btn_forceLow_clicked()
{
    logEvent "btn_forceLow clicked, changing power level";
    forceLow();
}

// == fan control
void radeon_profile::on_btn_pwmFixedApply_clicked()
{
    logEvent "btn_pwmFixedApply clicked, applying fixed fan speed";
    device->setPwmValue(static_cast<ushort>(ui->fanSpeedSlider->value()));
}

void radeon_profile::on_btn_pwmFixed_clicked()
{
    logEvent "btn_pwmFixed clicked, switching to fixed fan speed";
    ui->fanModesTabs->setCurrentWidget(ui->page_fixed);

    device->setPwmManualControl(true);
    device->setPwmValue(static_cast<ushort>(ui->fanSpeedSlider->value()));
}

void radeon_profile::on_btn_pwmAuto_clicked()
{
    logEvent "btn_pwmAuto clicked, switching to automatic fan speed";
    device->setPwmManualControl(false);
    ui->fanModesTabs->setCurrentWidget(ui->page);
}

void radeon_profile::on_btn_pwmProfile_clicked()
{
    logEvent "btn_pwmProfile clicked, switching to fan speed steps";
    ui->fanModesTabs->setCurrentWidget(ui->page_profile);

    device->setPwmManualControl(true);

    adjustFanSpeed(true);
}

void radeon_profile::on_combo_pProfile_currentIndexChanged(int index) {
    logEvent "combo_pProfile index changed, changing power profile";

    if(index != -1){ // Any profile is selected in combo_pProfile
        if (device->features.pm != DPM)
            index += 3; // frist three in enum is dpm so we need to increase

        device->setPowerProfile(static_cast<powerProfiles>(index));
    }
}

void radeon_profile::on_combo_pLevel_currentIndexChanged(int index) {
    logEvent "combo_pLevel current index changed, changing power level";

    if(index != -1) // Any profile is selected in combo_pLevel
        device->setForcePowerLevel(static_cast<forcePowerLevels>(index));
}

// == others
void radeon_profile::on_btn_dpmBattery_clicked() {
    logEvent "btn_dpmBattery clicked, changing power profile";
    ui->combo_pProfile->setCurrentIndex(BATTERY);
}

void radeon_profile::on_btn_dpmBalanced_clicked() {
    logEvent "btn_dpmBalanced clicked, changing power profile";
    ui->combo_pProfile->setCurrentIndex(BALANCED);
}

void radeon_profile::on_btn_dpmPerformance_clicked() {
    logEvent "btn_dpmPerformance clicked, changing power profile";
    ui->combo_pProfile->setCurrentIndex(PERFORMANCE);
}

void radeon_profile::resetMinMax() { device->gpuTemeperatureData.min = 0; device->gpuTemeperatureData.max = 0; }

void radeon_profile::on_timeSlider_valueChanged(int value) {
    logEvent "timeSlider value changed, changing graphs range";
    if(value > 0)
        rangeX = static_cast<ushort>(value);
}

void radeon_profile::on_cb_showFreqGraph_toggled(const bool &checked)
{
    logEvent "cb_showFreqGraph toggled";
    ui->plotClocks->setVisible(checked);
}

void radeon_profile::on_cb_showTempsGraph_toggled(const bool &checked)
{
    logEvent "cb_showTempsGraph toggled";
    ui->plotTemp->setVisible(checked);
}

void radeon_profile::on_cb_showVoltsGraph_toggled(const bool &checked)
{
    logEvent "cb_showVoltsGraph toggled";
    ui->plotVolts->setVisible(checked);
}

void radeon_profile::resetGraphs() {
    ui->plotClocks->yAxis->setRange(startClocksScaleL,startClocksScaleH);
    ui->plotVolts->yAxis->setRange(startVoltsScaleL,startVoltsScaleH);
    ui->plotTemp->yAxis->setRange(10,20);
}

void radeon_profile::showLegend(const bool &checked) {
    ui->plotClocks->legend->setVisible(checked);
    ui->plotVolts->legend->setVisible(checked);
    ui->plotClocks->replot();
    ui->plotVolts->replot();
}

void radeon_profile::setGraphOffset(const bool &checked) {
    graphOffset = (checked) ? 20 : 0;
}

void radeon_profile::changeEvent(QEvent *event)
{
    logEvent "changeEvent " << event->type();
    if(event->type() == QEvent::WindowStateChange && ui->cb_minimizeTray->isChecked()) {
        if(isMinimized())
            this->hide();

        event->ignore();
    }
    if (event->type() == QEvent::Close && ui->cb_closeTray) {
        this->hide();
        event->ignore();
    }
}

void radeon_profile::on_combo_gpus_currentIndexChanged(int index)
{
    logEvent "combo_gpus current index changed, selecting gpu";

    if(index != -1){ // Any profile is selected in combo_gpus
        device->changeGPU(static_cast<ushort>(index));
        setupUiEnabledFeatures(device->features);
        timerEvent(0);
        refreshBtnClicked(true);
    }
}

void radeon_profile::iconActivated(QSystemTrayIcon::ActivationReason reason) {
    if(reason == QSystemTrayIcon::Trigger){
        if (isHidden()) {
            this->setWindowFlags(Qt::Window);
            showNormal();
        } else
            hide();
    }
}

void radeon_profile::showEvent(QShowEvent *event){
    logEvent "Showing window";
    event->accept();
}

void radeon_profile::hideEvent(QHideEvent *event){
    logEvent "Hiding window";
    event->accept();
    show->setVisible(true);
}

void radeon_profile::closeEvent(QCloseEvent *e) {
    logEvent "closeEvent";

    if (ui->cb_closeTray->isChecked() && !closeFromTrayMenu) {
        qDebug() << "Closing to tray menu";
        this->hide();
        e->ignore();
    } else {
        //Check if a process is still running
        bool exit = true;
        for(int i=0; exit && (i < execsRunning.size()); i++){
            execBin * process = execsRunning.at(i);
            if(process->state() == QProcess::Running)
                exit = askConfirmation(tr("Quit"), process->name + tr(" is still running, exit anyway?"));
        }

        if( ! exit){
            e->ignore();
            return;
        }
        qDebug() << "Closing application";
        e->accept();

        killTimer(timerID);
        this->hide();
        saveConfig();

        if (device->features.pwmAvailable) {
            qDebug() << "Disabling pwm and saving fan profiles";
            device->setPwmManualControl(false);
            saveFanProfiles();
            QApplication::processEvents(QEventLoop::AllEvents, 50); // Wait for the daemon to disable pwm
        }

        qDebug() << "Closing";
        QApplication::quit();
    }
}

void radeon_profile::closeFromTray() {
    closeFromTrayMenu = true;
    this->close();
}

void radeon_profile::on_spin_lineThick_valueChanged(int arg1)
{
    logEvent "spin_lineThick value changed";
    Q_UNUSED(arg1);
    setupGraphsStyle();
}

void radeon_profile::on_spin_timerInterval_valueChanged(int arg1)
{
    logEvent "spin_timerInterval value changed";
    killTimer(timerID);
    timerID = startTimer(arg1 * 1000);
}

void radeon_profile::on_cb_graphs_toggled(bool checked)
{
    ui->graphTab->setEnabled(checked);
}

void radeon_profile::on_cb_gpuData_toggled(bool checked)
{
    logEvent "cb_gpuData toggled, applying changes";
    // Enable/Disable linked checkboxes
    ui->cb_graphs->setEnabled(checked);
    ui->cb_stats->setEnabled(checked);

    resetStats();


    // Enable/Disable the GPU data and stats list
    if (!checked) {
        ui->l_minMaxTemp->setText(tr("GPU data is disabled"));

        ui->list_stats->insertTopLevelItem(0, new QTreeWidgetItem(QStringList() << "" << "" << "" << tr("GPU data is disabled")));
        ui->list_currentGPUData->clear();
        ui->list_currentGPUData->addTopLevelItem(new QTreeWidgetItem(QStringList() << tr("GPU data is disabled")));
    }
    setupUiEnabledFeatures(device->features);

    // Enable/Disable pwm profile control
    const bool enableProfile = checked && device->features.pwmAvailable;
    if(ui->btn_pwmProfile->isChecked())
        device->setPwmManualControl(enableProfile);

    // Enable/Disable daemon auto update
    configureDaemonAutoRefresh((checked && ui->cb_daemonAutoRefresh->isChecked()), ui->spin_timerInterval->value());
}

void radeon_profile::refreshBtnClicked(bool onlyCardDependant){
    logEvent "Refresh button clicked, refreshing info";

    ui->list_glxinfo->clear();
    ui->list_glxinfo->addItems(device->getGLXInfo(ui->combo_gpus->currentText()));

    if( ! onlyCardDependant){
        ui->list_connectors->clear();
        ui->list_connectors->addTopLevelItems(device->getCardConnectors());
        ui->list_connectors->expandToDepth(2);

        ui->list_modInfo->clear();
        ui->list_modInfo->addTopLevelItems(device->getModuleInfo());
    }

    resetStats();
}

void radeon_profile::on_graphColorsList_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    logEvent "graphColorsList item double clicked";
    Q_UNUSED(column);
    QColor c = QColorDialog::getColor(item->backgroundColor(1));
    if (c.isValid()) {
        item->setBackgroundColor(1,c);
        // apply colors
        setupGraphsStyle();
    }
}

void radeon_profile::on_cb_stats_toggled(bool checked)
{
    logEvent "cb_stats toggled";
    ui->tab_stats->setEnabled(checked);

    // reset stats data
    resetStats();
    if (!checked)
        ui->list_stats->insertTopLevelItem(0, new QTreeWidgetItem(QStringList() << "" << "" << "" << tr("Stats disabled")));
}

void radeon_profile::copyGlxInfoToClipboard() {
    QString clip;
    for (int i = 0; i < ui->list_glxinfo->count(); i++)
        clip += ui->list_glxinfo->item(i)->text() + "\n";

    QApplication::clipboard()->setText(clip);
}

void radeon_profile::copyConnectorsToClipboard(){
    QString clip;

    for(QTreeWidgetItem *item : ui->list_connectors->selectedItems()) // For each item
        clip += item->text(0).simplified() + '\t' + item->text(1).simplified() + '\n';

    QApplication::clipboard()->setText(clip);
}

void radeon_profile::resetStats() {
    statsTickCounter = 0;
    pmStats.clear();
    ui->list_stats->clear();
}

void radeon_profile::on_cb_alternateRow_toggled(bool checked) {
    logEvent "cb_alternateRow toggled";
    ui->list_currentGPUData->setAlternatingRowColors(checked);
    ui->list_glxinfo->setAlternatingRowColors(checked);
    ui->list_modInfo->setAlternatingRowColors(checked);
    ui->list_connectors->setAlternatingRowColors(checked);
    ui->list_stats->setAlternatingRowColors(checked);
    ui->list_execProfiles->setAlternatingRowColors(checked);
    ui->list_variables->setAlternatingRowColors(checked);
    ui->list_vaules->setAlternatingRowColors(checked);
}

void radeon_profile::on_chProfile_clicked()
{
    logEvent "cb_alternateRow toggled";
    bool ok;
    QStringList profiles;
    profiles << profile_auto << profile_default << profile_high << profile_mid << profile_low;

    QString selection = QInputDialog::getItem(this, tr("Select new power profile"), tr("Profile selection"), profiles,0,false,&ok);

    if (ok) {
        if (selection == profile_default)
            device->setPowerProfile(DEFAULT);
        else if (selection == profile_auto)
            device->setPowerProfile(AUTO);
        else if (selection == profile_high)
            device->setPowerProfile(HIGH);
        else if (selection == profile_mid)
            device->setPowerProfile(MID);
        else if (selection == profile_low)
            device->setPowerProfile(LOW);
    }
}

void radeon_profile::on_btn_reconfigureDaemon_clicked()
{
    logEvent "btn_reconfigureDaemon clicked, reconfiguring daemon";
    configureDaemonAutoRefresh(ui->cb_daemonAutoRefresh->isChecked(), ui->spin_timerInterval->value());
}

void radeon_profile::on_tabs_execOutputs_tabCloseRequested(int index)
{
    logEvent "tabs_execOutputs tab close requested";
    if (execsRunning.at(index)->state() != QProcess::Running || askConfirmation(tr("Process running"), tr("Process is still running. Close tab?"))){

        ui->tabs_execOutputs->removeTab(index);
        execsRunning.removeAt(index);

        if (ui->tabs_execOutputs->count() == 0)
            btnBackToProfilesClicked();
    }
}


void radeon_profile::on_btn_fanInfo_clicked()
{
    QMessageBox::information(this,
                             tr("Fan control information"),
                             tr("Don't overheat your card! Be careful! Don't use this if you don't know what you're doing! \n\nHovewer, looks like card won't apply too low values due its internal protection. \n\nClosing application will restore fan control to Auto. If application crashes, last fan value will remain, so you have been warned!"));
}

void radeon_profile::on_btn_addFanStep_clicked()
{
    logEvent "btn_addFanStep clicked";
    const short temperature = static_cast<short>(askNumber(0, minFanStepsTemp, maxFanStepsTemp, tr("Temperature (°C)")));
    if (temperature == -1) // User clicked Cancel
        return;

    if (fanSteps.contains(temperature)) // A step with this temperature already exists
        QMessageBox::warning(this, tr("Error"), tr("This step already exists. To edit it double click it"));
    else { // This step does not exist, proceed
        const int fanSpeed = askNumber(0, minFanStepsSpeed, maxFanStepsSpeed, tr("Speed [%] (20-100)"));
        if (fanSpeed == -1) // User clicked Cancel
            return;

        addFanStep(temperature,static_cast<ushort>(fanSpeed));

    }
}

void radeon_profile::on_btn_removeFanStep_clicked()
{
    logEvent "btn_removeFanStep clicked, removing fan step";
    QTreeWidgetItem *current = ui->list_fanSteps->currentItem();

    if (ui->list_fanSteps->indexOfTopLevelItem(current) == 0 || ui->list_fanSteps->indexOfTopLevelItem(current) == ui->list_fanSteps->topLevelItemCount()-1) {
        // The selected item is the first or the last, it can't be deleted
        QMessageBox::warning(this, tr("Error"), tr("You can't delete the first and the last item"));
        return;
    }

    // The selected item can be removed, remove it
    short temperature = current->text(0).toShort();

    fanSteps.remove(temperature);
    adjustFanSpeed(true);

    // Remove the step from the list and from the graph
    delete current;
    ui->plotFanProfile->graph(0)->removeData(temperature);
    ui->plotFanProfile->replot();
}


void radeon_profile::on_list_fanSteps_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    logEvent "list_fanSteps double clicked, editing fan step";
    if (ui->list_fanSteps->indexOfTopLevelItem(item) == 0 || ui->list_fanSteps->indexOfTopLevelItem(item) == ui->list_fanSteps->topLevelItemCount()-1) {
        // The selected item is the first or the last, it can't be edited
        QMessageBox::warning(this, tr("Error"), tr("You can't edit the first and the last item"));
        return;
    }

    const int oldTemp = item->text(0).toInt(), oldSpeed = item->text(1).toInt();
    int newTemp, newSpeed;

    if(column == 0){ // The user wants to change the temperature
        newTemp = askNumber(oldTemp, minFanStepsTemp, maxFanStepsTemp, tr("Temperature (°C)"));
        if(newTemp == -1) // User clicked cancel
            return;

        newSpeed = oldSpeed;
        fanSteps.remove(static_cast<short>(oldTemp));
        delete item;
        ui->plotFanProfile->graph(0)->removeData(oldTemp);
    } else { // The user wants to change the speed
        newTemp = oldTemp;
        newSpeed = askNumber(oldSpeed, minFanStepsSpeed, maxFanStepsSpeed, tr("Speed [%] (20-100)"));
        if(newSpeed == -1) // User clicked cancel
            return;
    }

    // addFanStep() will check the validity of newSpeed and overwrite the current step
    addFanStep(static_cast<short>(newTemp), static_cast<ushort>(newSpeed));
}

int radeon_profile::askNumber(const int value, const int min, const int max, const QString label) {
    bool ok;
    int number = QInputDialog::getInt(this,"",label,value,min,max,1, &ok);

    if (!ok)
        return -1;

    return number;
}

void radeon_profile::on_fanSpeedSlider_valueChanged(int value)
{
    if(device->features.pwmMaxSpeed != 0) // Prevent division by zero
        ui->labelFixedSpeed->setText(QString().setNum((static_cast<float>(value) / device->features.pwmMaxSpeed) * 100,'f',0));
}

//========

void radeon_profile::on_cb_enableOverclock_toggled(const bool enable){
    logEvent "cb_enableOverclock toggled, enabling overclock interface";
    const bool GPUOcOk = enable && device->features.GPUoverClockAvailable,
            memoryOcOk = enable && device->features.memoryOverclockAvailable;

    ui->label_GPUoc->setEnabled(GPUOcOk);
    ui->slider_GPUoverclock->setEnabled(GPUOcOk);
    ui->label_GPUoverclock->setEnabled(GPUOcOk);

    ui->label_memoryOc->setEnabled(memoryOcOk);
    ui->slider_memoryOverclock->setEnabled(memoryOcOk);
    ui->label_memoryOverclock->setEnabled(memoryOcOk);

    ui->btn_applyOverclock->setEnabled(enable);
    ui->cb_overclockAtLaunch->setEnabled(enable);

    if(enable)
        qDebug() << "Enabling overclock";
    else {
        qDebug() << "Disabling overclock";
        device->overclockGPU(0);
        device->overclockMemory(0);
    }
}

void radeon_profile::on_btn_applyOverclock_clicked(){
    logEvent "btn_applyOverclock clicked, applying overclock";
    if( ! device->overclockGPU(ui->slider_GPUoverclock->value()))
        QMessageBox::warning(this, tr("Error"), tr("An error occurred, GPU overclock failed"));

    if( ! device->overclockMemory(ui->slider_memoryOverclock->value()))
        QMessageBox::warning(this, tr("Error"), tr("An error occurred, memory overclock failed"));
}

void radeon_profile::on_slider_GPUoverclock_valueChanged(const int value){
    ui->label_GPUoverclock->setText(QString::number(value) + '%');
}

void radeon_profile::on_slider_memoryOverclock_valueChanged(const int value){
    ui->label_memoryOverclock->setText(QString::number(value) + '%');
}

void radeon_profile::on_cb_showAlwaysGpuSelector_toggled(const bool checked){
    logEvent "cb_showAlwaysGpuSelector toggled, changing visibility";
    const bool show = checked || (ui->combo_gpus->count() > 2);
    ui->combo_gpus->setVisible(show);
    ui->label->setVisible(show);
}

void radeon_profile::on_cb_showCombo_toggled(const bool checked){
    logEvent "cb_showCombo toggled, changing visibility";
    ui->combo_pProfile->setVisible(checked);
    ui->combo_pLevel->setVisible(checked);
}

void radeon_profile::on_mainTabs_currentChanged(int index){
    logEvent "mainTabs current tab changed, refreshing";
    if(ui->mainTabs->indexOf(ui->infoTab) == index && ui->list_currentGPUData->isEnabled())
        // currentGPUData is not updated when not visible, update it when infoTab is chosen
        refreshUI();
    else if(ui->mainTabs->indexOf(ui->graphTab) == index && ui->graphTab->isEnabled())
        // graphTab tabs are not replotted when not visible, replot them when graphTab is chosen
        replotGraphs();
}

void radeon_profile::on_btn_exportGraph_clicked(){
    logEvent "btn_exportGraph clicked, exporting graphs";
    exportDialog.show();
}

void radeon_profile::exportSingleGraph(QAbstractButton * format, QCustomPlot * graph, const QString & path){
    graph->replot();

    if(format == exportUi.rb_bmp)
        graph->saveBmp(path + ".bmp");
    else if(format == exportUi.rb_jpg)
        graph->saveJpg(path + ".jpg");
    else if(format == exportUi.rb_pdf)
        graph->savePdf(path + ".pdf");
    else if(format == exportUi.rb_png)
        graph->savePng(path + ".png");
}

void radeon_profile::exportGraphs(){
    qDebug() << "Exporting graphs";
    QAbstractButton * rb = exportFormat.checkedButton();
    const QString directory = exportUi.directory->text();

    if(exportUi.cb_temperature->isChecked())
        exportSingleGraph(rb, ui->plotTemp, directory+"/temperature");

    if(exportUi.cb_frequency->isChecked())
        exportSingleGraph(rb, ui->plotClocks, directory+"/frequency");

    if(exportUi.cb_volts->isChecked())
        exportSingleGraph(rb, ui->plotVolts, directory+"/volts");
}

void radeon_profile::changeExportDirectory(){
    logEvent "Changing export directory";
    exportUi.directory->setText(QFileDialog::getExistingDirectory(this, "Export directory", exportUi.directory->text()));
}

void radeon_profile::on_list_execProfiles_itemSelectionChanged(){
    logEvent "Item selected in list_variables, enabling buttons";
    ui->btn_modifyExecProfile->setEnabled(true);
    ui->btn_removeExecProfile->setEnabled(true);
    ui->btn_runExecProfile->setEnabled(true);
    ui->btn_viewOutput->setEnabled(true);
}
