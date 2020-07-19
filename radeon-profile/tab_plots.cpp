
// copyright marazmista @ 13.06.2017

#include "radeon_profile.h"
#include "ui_radeon_profile.h"
#include "dialogs/dialog_defineplot.h"

void radeon_profile::createPlots() {
    for (int i = 0; i < plotManager.schemas.count(); ++ i)
        setupPlot(plotManager.schemas.at(i));
}

void radeon_profile::on_btn_configurePlots_clicked()
{
    ui->stack_plots->setCurrentIndex(1);
}

void radeon_profile::on_btn_applySavePlotsDefinitons_clicked()
{
    plotManager.setRightGap(ui->cb_plotsRightGap->isChecked());

    for (int i = 0; i < plotManager.plots.count(); ++i) {
        plotManager.plots.at(i)->showLegend(ui->cb_showLegends->isChecked());

        if (ui->cb_overridePlotsBg->isChecked())
            plotManager.setPlotBackground(plotManager.plots.at(i), ui->frame_plotsBackground->palette().background().color());
        else
            plotManager.setPlotBackground(plotManager.plots.at(i), plotManager.schemas.at(i).background);
    }

    ui->stack_plots->setCurrentIndex(0);
    saveConfig();
}

PlotInitialValues radeon_profile::figureOutInitialScale(const PlotDefinitionSchema &pds)
{
    PlotInitialValues piv;
    if (pds.left.enabled)
        piv.left = device.gpuData.value(pds.left.dataList.keys().at(0)).value;

    if (pds.right.enabled)
        piv.right = device.gpuData.value(pds.right.dataList.keys().at(0)).value;

    return piv;
}

void radeon_profile::on_btn_addPlotDefinition_clicked()
{
    Dialog_definePlot *d = new Dialog_definePlot(this);
    d->setAvailableGPUData(device.gpuData.keys());

    if (d->exec() == QDialog::Accepted) {
        const PlotDefinitionSchema pds = d->getCreatedSchema();

        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, pds.name);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(0,Qt::Checked);

        if (plotManager.checkIfSchemaExists(pds.name)) {
            if (!askConfirmation(tr("Question"), tr("Plot definition with that name already exists. Replace?"))) {
                delete d;
                delete item;
                return;
            }
        } else
            ui->list_plotDefinitions->addTopLevelItem(item);


        plotManager.addSchema(pds);
        setupPlot(pds);
    }

    delete d;
}

void radeon_profile::on_btn_removePlotDefinition_clicked()
{
    if (!ui->list_plotDefinitions->currentItem())
        return;

    if (!askConfirmation("",tr("Remove selected plot definition?")))
        return;

    plotManager.removeSchema(ui->list_plotDefinitions->currentIndex().row());
    delete ui->list_plotDefinitions->currentItem();
}

void radeon_profile::setupPlot(const PlotDefinitionSchema &pds)
{
    RPPlot *plot = plotManager.createPlotFromSchema(pds, figureOutInitialScale(pds));
    plot->showLegend(ui->cb_showLegends->isChecked());

    if (ui->cb_overridePlotsBg->isChecked())
        plotManager.setPlotBackground(plot, ui->frame_plotsBackground->palette().background().color());

    ui->pagePlots->layout()->addWidget(plot);
}

void radeon_profile::modifyPlotSchema(const int index) {
    const PlotDefinitionSchema &editedPds = plotManager.schemas.at(index);

    Dialog_definePlot *d = new Dialog_definePlot(this);
    d->setAvailableGPUData(device.gpuData.keys());
    d->setEditedPlotSchema(editedPds);

    if (d->exec() == QDialog::Accepted) {
        PlotDefinitionSchema pds = d->getCreatedSchema();

        if (pds.name != editedPds.name) {
            QTreeWidgetItem *item = new QTreeWidgetItem();
            item->setText(0, pds.name);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(0,Qt::Checked);
            ui->list_plotDefinitions->addTopLevelItem(item);
        } else
            plotManager.removeSchema(index);

        plotManager.addSchema(pds);
        setupPlot(pds);
    }

    delete d;
}

void radeon_profile::on_btn_modifyPlotDefinition_clicked()
{
    if (ui->list_plotDefinitions->currentItem() == nullptr)
        return;

    modifyPlotSchema(ui->list_plotDefinitions->currentIndex().row());
}

void radeon_profile::on_list_plotDefinitions_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    Q_UNUSED(item);
    modifyPlotSchema(ui->list_plotDefinitions->currentIndex().row());
}

void radeon_profile::on_list_plotDefinitions_itemChanged(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    Q_UNUSED(item);

    switch (item->checkState(0)) {
        case Qt::Unchecked:
            plotManager.removePlot(ui->list_plotDefinitions->currentIndex().row());
            return;
        case Qt::Checked:
            setupPlot(plotManager.schemas.at(ui->list_plotDefinitions->currentIndex().row()));
            return;
        default:
            return;
    }
}

QColor getColor(const QColor &c) {
    QColor color = QColorDialog::getColor(c);
    if (color.isValid())
        return color;

    return c;
}

void radeon_profile::on_btn_setPlotsBackground_clicked()
{
    ui->frame_plotsBackground->setPalette(QPalette(getColor(ui->frame_plotsBackground->palette().background().color())));
}
