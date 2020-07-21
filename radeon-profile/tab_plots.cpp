
// copyright marazmista @ 13.06.2017

#include "radeon_profile.h"
#include "ui_radeon_profile.h"
#include "dialogs/dialog_defineplot.h"
#include "dialogs/dialog_plotsconfiguration.h"

void radeon_profile::createPlots() {
    ui->widget_plots->setLayout(new QVBoxLayout(ui->widget_plots));
    ui->widget_plots->layout()->setMargin(0);
    ui->widget_plots->layout()->setSpacing(2);

    for (int i = 0; i < plotManager.schemas.count(); ++ i) {
        if (plotManager.schemas.at(i).enabled)
            setupPlot(plotManager.schemas[i]);
    }
}

void radeon_profile::on_btn_configurePlots_clicked()
{
    Dialog_plotsConfiguration *d = new Dialog_plotsConfiguration(&plotManager, &device.gpuData, this);

    if (d->exec() == QDialog::Accepted) {
        plotManager.setRightGap();

        for (int i = 0; i < plotManager.schemas.count(); ++i) {
            if (plotManager.schemas.at(i).plot == nullptr) {
                if (plotManager.schemas.at(i).enabled) {
                    setupPlot(plotManager.schemas[i]);
                    plotManager.schemas[i].plot->showLegend(plotManager.generalConfig.showLegend);

                    if (plotManager.generalConfig.commonPlotsBackground)
                        plotManager.setPlotBackground(plotManager.schemas[i].plot, plotManager.generalConfig.plotsBackground);
                }

                continue;
            }

            plotManager.schemas[i].plot->showLegend(plotManager.generalConfig.showLegend);
            if (plotManager.generalConfig.commonPlotsBackground)
                plotManager.setPlotBackground(plotManager.schemas[i].plot, plotManager.generalConfig.plotsBackground);

            if (!plotManager.schemas.at(i).enabled) {
                plotManager.removePlot(plotManager.schemas[i]);
                continue;
            }

            if (plotManager.schemas.at(i).modified) {
                plotManager.removePlot(plotManager.schemas[i]);
                setupPlot(plotManager.schemas[i]);
                plotManager.schemas[i].modified = false;
                continue;
            }
        }

        saveConfig();
    }


    delete d;
}

void radeon_profile::setupPlot(PlotDefinitionSchema &pds)
{
    plotManager.createPlotFromSchema(pds, PlotManager::figureOutInitialScale(pds, &device.gpuData));
    pds.plot->showLegend(plotManager.generalConfig.showLegend);

    if (plotManager.generalConfig.commonPlotsBackground)
        plotManager.setPlotBackground(pds.plot, plotManager.generalConfig.plotsBackground);

    ui->widget_plots->layout()->addWidget(pds.plot);
}
