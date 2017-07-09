
// copyright marazmista @ 8.07.2017

#ifndef DIALOG_DEINETOPBARITEM_H
#define DIALOG_DEINETOPBARITEM_H

#include <QDialog>
#include "components/topbarcomponents.h"
#include "globalStuff.h"

namespace Ui {
class Dialog_deineTopbarItem;
}

class Dialog_deineTopbarItem : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog_deineTopbarItem(QList<ValueID> *data, const GPUConstParams *params, QWidget *parent = 0);
    ~Dialog_deineTopbarItem();

    TopbarItemDefinitionSchema getCreatedSchema();
    void setEditedSchema(TopbarItemDefinitionSchema tis);

private slots:
    void on_radio_labelPair_toggled(bool checked);
    void on_radio_largeLabel_toggled(bool checked);
    void on_radio_pie_toggled(bool checked);
    void on_combo_primaryData_currentIndexChanged(int index);
    void on_btn_cancel_clicked();
    void on_btn_save_clicked();
    void on_btn_setPrimaryColor_clicked();
    void on_btn_setSecondaryColor_clicked();

private:
    Ui::Dialog_deineTopbarItem *ui;
    TopbarItemDefinitionSchema editedSchema;
    QList<ValueID> *availableGpuData;
    const GPUConstParams *gpuParams;

    void createCombo(QComboBox *combo, const TopbarItemType type, bool emptyFirst = false);
    void createSecondaryCombo();
    TopbarItemType getItemType();
    QColor getColor(const QColor &c);
    void setSecondaryEditEnabled(bool colorEditEnabled, bool comboEnabled);
};

#endif // DIALOG_DEINETOPBARITEM_H
