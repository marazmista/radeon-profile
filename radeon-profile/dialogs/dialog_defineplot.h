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

    PlotDefinitionSchema getCreatedSchema();
    void setEditedPlotSchema(const PlotDefinitionSchema &pds);
    void setAvailableGPUData(const QList<ValueID> &gpu);

    ~Dialog_definePlot();

private slots:
    void on_list_leftData_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void on_btn_setBackground_clicked();
    void on_btn_leftScaleColor_clicked();
    void on_btn_rightScaleColor_clicked();
    void on_list_rightData_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void on_cb_enableLeftScale_clicked(bool checked);
    void on_cb_enableRightScale_clicked(bool checked);
    void on_list_leftData_itemChanged(QTreeWidgetItem *item, int column);
    void on_list_rightData_itemChanged(QTreeWidgetItem *item, int column);
    void on_btn_save_clicked();
    void on_btn_cancel_clicked();

private:
    Ui::Dialog_definePlot *ui;
    PlotDefinitionSchema schema;
    QList<ValueID> availableGPUData;
    QMap<int, ValueID> listRelationToValueID;

    void createStyleCombo(QComboBox *combo);
    QStringList createUnitCombo();
    QList<QTreeWidgetItem* > createList();
    QList<QTreeWidgetItem* > createDataListFromUnit(ValueUnit u);
    void init();
    QColor getColor(const QColor &c = Qt::black);
    void addSelectedItemToSchema(int itemIndex, QTreeWidgetItem *item , QMap<ValueID, QColor> &schemaDataList);
    void loadListFromSchema(QTreeWidget *list, QMap<ValueID, QColor> selected);
};

#endif // DIALOG_DEFINEPLOT_H
