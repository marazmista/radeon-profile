#ifndef DIALOG_PLOTSCONFIGURATION_H
#define DIALOG_PLOTSCONFIGURATION_H

#include <QDialog>
#include <components/rpplot.h>
#include "globalStuff.h"

namespace Ui {
class Dialog_plotsConfiguration;
}

class Dialog_plotsConfiguration : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog_plotsConfiguration(PlotManager *pm, GPUDataContainer *data,  QWidget *parent = nullptr);
    ~Dialog_plotsConfiguration();

private slots:
    void on_btn_cancel_clicked();
    void on_btn_removePlotDefinition_clicked();
    void on_btn_addPlotDefinition_clicked();
    void on_btn_modifyPlotDefinition_clicked();
    void on_list_plotDefinitions_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void on_btn_setPlotsBackground_clicked();
    void on_btn_savePlotsConfiguration_clicked();

private:
    void modifyPlotSchema(const int index);
    void setupPlot(PlotDefinitionSchema &pds);

    Ui::Dialog_plotsConfiguration *ui;
    PlotManager *plotManager;
    GPUDataContainer *gpuData;
};

#endif // DIALOG_PLOTSCONFIGURATION_H
