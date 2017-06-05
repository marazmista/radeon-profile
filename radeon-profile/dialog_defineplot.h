#ifndef DIALOG_DEFINEPLOT_H
#define DIALOG_DEFINEPLOT_H

#include <QDialog>
#include "globalStuff.h"
#include "components/rpplot.h"

namespace Ui {
class Dialog_definePlot;
}

class Dialog_definePlot : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog_definePlot(QWidget *parent = 0);
    explicit Dialog_definePlot(const PlotDefinitionSchema &s, QWidget *parent = 0);

    PlotDefinitionSchema getCreatedSchema();

    ~Dialog_definePlot();

private slots:

    void on_combo_leftUnit_currentIndexChanged(int index);

    void on_combo_rightUnit_currentIndexChanged(int index);

    void on_buttonBox_accepted();

    void on_tree_leftData_itemChanged(QTreeWidgetItem *item, int column);

    void on_tree_leftData_itemDoubleClicked(QTreeWidgetItem *item, int column);

    void on_btn_setBackground_clicked();

    void on_btn_leftScaleColor_clicked();

    void on_btn_rightScaleColor_clicked();

    void on_tree_rightData_itemDoubleClicked(QTreeWidgetItem *item, int column);

private:
    Ui::Dialog_definePlot *ui;
    PlotDefinitionSchema schema;
    GpuDataContainer refernceGpuData;

    QStringList penStyles = { tr("Solid line"), tr("Dash line"), tr("Dot Line") };

    void createStyleCombo();
    QStringList createUnitCombo();
    QList<QTreeWidgetItem* > createList(QStringList titles);
    QList<QTreeWidgetItem* > createDataListFromUnit(ValueUnit u);
    void init();
    QColor getColor(const QColor &c = Qt::black);

};

#endif // DIALOG_DEFINEPLOT_H
