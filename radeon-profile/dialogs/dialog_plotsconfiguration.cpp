#include "dialog_plotsconfiguration.h"
#include "ui_dialog_plotsconfiguration.h"
#include "dialog_defineplot.h"

Dialog_plotsConfiguration::Dialog_plotsConfiguration(PlotManager *pm, GPUDataContainer *data,  QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog_plotsConfiguration)
{
    ui->setupUi(this);
    plotManager = pm;
    gpuData = data;

    ui->cb_showLegends->setChecked(plotManager->generalConfig.showLegend);
    ui->cb_plotsRightGap->setChecked(plotManager->generalConfig.graphOffset);
    ui->cb_overridePlotsBg->setChecked(plotManager->generalConfig.commonPlotsBackground);
    ui->frame_plotsBackground->setAutoFillBackground(true);
    ui->frame_plotsBackground->setPalette(QPalette(plotManager->generalConfig.plotsBackground));
    ui->btn_setPlotsBackground->setEnabled(plotManager->generalConfig.commonPlotsBackground);

    for (const auto & pds : plotManager->schemas) {
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, pds.name);
        item->setCheckState(0,(pds.enabled) ? Qt::Checked : Qt::Unchecked);
        ui->list_plotDefinitions->addTopLevelItem(item);
    }
}

Dialog_plotsConfiguration::~Dialog_plotsConfiguration()
{
    delete ui;
}

void Dialog_plotsConfiguration::on_btn_cancel_clicked()
{
    this->reject();
}

void Dialog_plotsConfiguration::on_btn_removePlotDefinition_clicked()
{
    if (!ui->list_plotDefinitions->currentItem())
        return;

    if (QMessageBox::question(this, "",tr("Remove selected plot definition?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::No)
        return;

    plotManager->removeSchema(ui->list_plotDefinitions->currentIndex().row());
    delete ui->list_plotDefinitions->currentItem();
}

void Dialog_plotsConfiguration::on_btn_addPlotDefinition_clicked()
{
    Dialog_definePlot *d = new Dialog_definePlot(gpuData->keys(), this);

    if (d->exec() == QDialog::Accepted) {
        PlotDefinitionSchema pds = d->getCreatedSchema();

        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setText(0, pds.name);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(0,Qt::Checked);

        if (plotManager->findSchemaIdByName(pds.name) != -1) {
            if (QMessageBox::question(this, tr("Question"), tr("Plot definition with that name already exists. Replace?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::No) {
                delete d;
                delete item;
                return;
            }
        } else
            ui->list_plotDefinitions->addTopLevelItem(item);

        plotManager->addSchema(pds);
    }

    delete d;
}

void Dialog_plotsConfiguration::on_btn_modifyPlotDefinition_clicked()
{
    if (ui->list_plotDefinitions->currentItem() == nullptr)
        return;

    modifyPlotSchema(ui->list_plotDefinitions->currentIndex().row());
}

void Dialog_plotsConfiguration::modifyPlotSchema(const int index) {
    PlotDefinitionSchema &editedPds = plotManager->schemas[index];

    Dialog_definePlot *d = new Dialog_definePlot(gpuData->keys(), this);
    d->setEditedPlotSchema(editedPds);

    if (d->exec() == QDialog::Accepted) {
        PlotDefinitionSchema pds = d->getCreatedSchema();

        if (pds.name != editedPds.name) {
            QTreeWidgetItem *item = new QTreeWidgetItem();
            item->setText(0, pds.name);
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(0,Qt::Checked);
            ui->list_plotDefinitions->addTopLevelItem(item);

            plotManager->addSchema(pds);

        } else {
            editedPds = pds;
            editedPds.modified = true;
        }
    }

    delete d;
}

void Dialog_plotsConfiguration::on_list_plotDefinitions_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    Q_UNUSED(item);
    modifyPlotSchema(ui->list_plotDefinitions->currentIndex().row());
}

QColor getColor(const QColor &c) {
    QColor color = QColorDialog::getColor(c);
    if (color.isValid())
        return color;

    return c;
}

void Dialog_plotsConfiguration::on_btn_setPlotsBackground_clicked()
{
    ui->frame_plotsBackground->setPalette(QPalette(getColor(ui->frame_plotsBackground->palette().background().color())));
}

void Dialog_plotsConfiguration::on_btn_savePlotsConfiguration_clicked()
{
    plotManager->generalConfig.showLegend = ui->cb_showLegends->isChecked();
    plotManager->generalConfig.graphOffset = ui->cb_plotsRightGap->isChecked();
    plotManager->generalConfig.commonPlotsBackground = ui->cb_overridePlotsBg->isChecked();
    plotManager->generalConfig.plotsBackground = ui->frame_plotsBackground->palette().background().color();

    for (int i = 0; i < plotManager->schemas.count(); ++i)
        plotManager->schemas[i].enabled = ui->list_plotDefinitions->topLevelItem(i)->checkState(0) == Qt::Checked;

    this->accept();
}
