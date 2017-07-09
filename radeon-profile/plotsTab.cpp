
// copyright marazmista @ 13.06.2017

#include "radeon_profile.h"
#include "ui_radeon_profile.h"
#include "dialogs/dialog_defineplot.h"

void radeon_profile::createPlots() {
    for (const QString &name : plotManager.schemas.keys())
        setupPlot(plotManager.schemas.value(name));
}

void radeon_profile::on_btn_configurePlots_clicked()
{
    ui->stack_plots->setCurrentIndex(1);
}

void radeon_profile::on_btn_applySavePlotsDefinitons_clicked()
{
    plotManager.setRightGap(ui->cb_plotsRightGap->isChecked());

    for (const QString &pk : plotManager.plots.keys()) {
        plotManager.plots[pk]->showLegend(ui->cb_showLegends->isChecked());

        if (ui->cb_overridePlotsBg->isChecked())
            plotManager.setPlotBackground(pk, ui->frame_plotsBackground->palette().background().color());
        else
            plotManager.setPlotBackground(pk, plotManager.schemas.value(pk).background);
    }

    ui->stack_plots->setCurrentIndex(0);
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
        PlotDefinitionSchema pds = d->getCreatedSchema();

        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, pds.name);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(0,Qt::Checked);

        if (plotManager.schemas.contains(pds.name)) {
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

    plotManager.removeSchema(ui->list_plotDefinitions->currentItem()->text(0));
    delete ui->list_plotDefinitions->currentItem();
}

void radeon_profile::setupPlot(const PlotDefinitionSchema &pds)
{
    plotManager.createPlotFromSchema(pds.name, figureOutInitialScale(pds));
    plotManager.plots[pds.name]->showLegend(ui->cb_showLegends->isChecked());

    if (ui->cb_overridePlotsBg->isChecked())
        plotManager.setPlotBackground(pds.name, ui->frame_plotsBackground->palette().background().color());

    ui->pagePlots->layout()->addWidget(plotManager.plots.value(pds.name));
}

void radeon_profile::modifyPlotSchema(const QString &name) {
    PlotDefinitionSchema pds = plotManager.schemas[name];

    Dialog_definePlot *d = new Dialog_definePlot(this);
    d->setAvailableGPUData(device.gpuData.keys());
    d->setEditedPlotSchema(pds);

    if (d->exec() == QDialog::Accepted) {
        PlotDefinitionSchema pds = d->getCreatedSchema();

        if (pds.name != name) {
            QTreeWidgetItem *item = new QTreeWidgetItem();
            item->setText(0, pds.name);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(0,Qt::Checked);
        } else
            plotManager.removePlot(pds.name);

        plotManager.addSchema(pds);
        setupPlot(pds);
    }

    delete d;
}

void radeon_profile::on_btn_modifyPlotDefinition_clicked()
{
    if (ui->list_plotDefinitions->currentItem() == 0)
        return;

    modifyPlotSchema(ui->list_plotDefinitions->currentItem()->text(0));
}

void radeon_profile::on_list_plotDefinitions_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    modifyPlotSchema(item->text(0));
}

void radeon_profile::on_list_plotDefinitions_itemChanged(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);

    switch (item->checkState(0)) {
        case Qt::Unchecked:
            plotManager.removePlot(item->text(0));
            return;
        case Qt::Checked:
            setupPlot(plotManager.schemas.value(item->text(0)));
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
