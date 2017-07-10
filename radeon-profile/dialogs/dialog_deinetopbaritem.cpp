
// copyright marazmista @ 8.07.2017

#include "dialog_deinetopbaritem.h"
#include "ui_dialog_deinetopbaritem.h"

Dialog_deineTopbarItem::Dialog_deineTopbarItem(QList<ValueID> *data, const GPUConstParams *params, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog_deineTopbarItem)
{
    ui->setupUi(this);

    availableGpuData = data;
    gpuParams = params;

    on_radio_labelPair_toggled(true);

    ui->frame_primaryColor->setAutoFillBackground(true);
    ui->frame_secondaryColor->setAutoFillBackground(true);

    ui->frame_primaryColor->setPalette(QPalette(this->palette().foreground().color()));
    ui->frame_secondaryColor->setPalette(QPalette(this->palette().foreground().color()));
}

Dialog_deineTopbarItem::~Dialog_deineTopbarItem()
{
    delete ui;
}

void Dialog_deineTopbarItem::setEditedSchema(TopbarItemDefinitionSchema tis) {
    editedSchema = tis;

    switch (tis.type) {
        case TopbarItemType::LABEL_PAIR:
            on_radio_labelPair_toggled(true);
            ui->radio_labelPair->setChecked(true);
            break;
        case TopbarItemType::LARGE_LABEL:
            on_radio_largeLabel_toggled(true);
            ui->radio_largeLabel->setChecked(true);
            break;
        case TopbarItemType::PIE:
            on_radio_pie_toggled(true);
            ui->radio_pie->setChecked(true);
            break;
    }

    ui->combo_primaryData->setCurrentIndex(ui->combo_primaryData->findData(QVariant::fromValue(tis.primaryValueId)));
    ui->combo_secondaryData->setCurrentIndex(ui->combo_secondaryData->findData(QVariant::fromValue(tis.secondaryValueId)));
    ui->frame_primaryColor->setPalette(QPalette(tis.primaryColor));
    ui->frame_secondaryColor->setPalette(QPalette(tis.secondaryColor));
}

void Dialog_deineTopbarItem::createCombo(QComboBox *combo, const TopbarItemType type, bool emptyFirst) {
    combo->clear();

    if (emptyFirst)
        combo->addItem("");

    switch (type) {
        case TopbarItemType::LABEL_PAIR:
        case TopbarItemType::LARGE_LABEL:
            for (int i = 0; i < availableGpuData->count(); ++i) {
                if (availableGpuData->at(i) == ValueID::TEMPERATURE_BEFORE_CURRENT)
                    continue;

                combo->addItem(globalStuff::getNameOfValueID(availableGpuData->at(i)), QVariant::fromValue(availableGpuData->at(i)));
            }
            break;

        case TopbarItemType::PIE:
            for (int i = 0; i < availableGpuData->count(); ++i) {
                if (globalStuff::getUnitFomValueId(availableGpuData->at(i)) == ValueUnit::PERCENT)
                    combo->addItem(globalStuff::getNameOfValueID(availableGpuData->at(i)), QVariant::fromValue(availableGpuData->at(i)));

                if (availableGpuData->at(i) == ValueID::CLK_CORE && gpuParams->maxCoreClock != -1)
                    combo->addItem(globalStuff::getNameOfValueID(ValueID::CLK_CORE), QVariant::fromValue(ValueID::CLK_CORE));

                if (availableGpuData->at(i) == ValueID::CLK_MEM && gpuParams->maxMemClock != -1)
                    combo->addItem(globalStuff::getNameOfValueID(ValueID::CLK_MEM), QVariant::fromValue(ValueID::CLK_MEM));

                if (availableGpuData->at(i) == ValueID::TEMPERATURE_CURRENT && gpuParams->temp1_crit != -1)
                    combo->addItem(globalStuff::getNameOfValueID(ValueID::TEMPERATURE_CURRENT), QVariant::fromValue(ValueID::TEMPERATURE_CURRENT));

            }
            break;
    }
}

TopbarItemType Dialog_deineTopbarItem::getItemType() {
    if (ui->radio_labelPair->isChecked())
        return TopbarItemType::LABEL_PAIR;
    else if (ui->radio_largeLabel->isChecked())
        return TopbarItemType::LARGE_LABEL;
    else if (ui->radio_pie->isChecked())
        return TopbarItemType::PIE;

    return TopbarItemType::LARGE_LABEL;
}

QColor Dialog_deineTopbarItem::getColor(const QColor &c) {
    QColor color = QColorDialog::getColor(c);
    if (color.isValid())
        return color;

    return c;
}

void Dialog_deineTopbarItem::setSecondaryEditEnabled(bool colorEditEnabled, bool comboEnabled)
{
    ui->combo_secondaryData->setEnabled(comboEnabled);

    ui->btn_setSecondaryColor->setEnabled(colorEditEnabled);
    ui->frame_secondaryColor->setVisible(colorEditEnabled);
}

void Dialog_deineTopbarItem::on_radio_labelPair_toggled(bool checked)
{
    if (!checked)
        return;

    createCombo(ui->combo_primaryData, TopbarItemType::LABEL_PAIR);
    createCombo(ui->combo_secondaryData, TopbarItemType::LABEL_PAIR, true);

    setSecondaryEditEnabled(true, true);
}


void Dialog_deineTopbarItem::on_radio_largeLabel_toggled(bool checked)
{
    if (!checked)
        return;

    createCombo(ui->combo_primaryData, TopbarItemType::LARGE_LABEL);
    setSecondaryEditEnabled(false, false);
}

void Dialog_deineTopbarItem::on_radio_pie_toggled(bool checked)
{
    if (!checked)
        return;

    createCombo(ui->combo_primaryData, TopbarItemType::PIE);
    setSecondaryEditEnabled(false, true);
}

TopbarItemDefinitionSchema Dialog_deineTopbarItem::getCreatedSchema() {
    return editedSchema;
}

void Dialog_deineTopbarItem::on_combo_primaryData_currentIndexChanged(int index)
{
    Q_UNUSED(index)

    if (ui->radio_largeLabel->isChecked()) {
        setSecondaryEditEnabled(false, false);
        return;
    } else if (ui->radio_pie->isChecked()) {
        setSecondaryEditEnabled(false, true);
    } else
        setSecondaryEditEnabled(true, true);

    if (ui->radio_labelPair->isChecked())
        return;

    ui->combo_secondaryData->clear();
    ui->combo_secondaryData->addItem("");

    switch (static_cast<ValueID>(ui->combo_primaryData->currentData().toInt())) {
        case ValueID::FAN_SPEED_PERCENT:
            if (availableGpuData->contains(ValueID::FAN_SPEED_RPM))
                ui->combo_secondaryData->addItem(globalStuff::getNameOfValueID(ValueID::FAN_SPEED_RPM), QVariant::fromValue(ValueID::FAN_SPEED_RPM));

            break;
        case ValueID::GPU_VRAM_USAGE_PERCENT:
            if (availableGpuData->contains(ValueID::GPU_VRAM_USAGE_MB))
                ui->combo_secondaryData->addItem(globalStuff::getNameOfValueID(ValueID::GPU_VRAM_USAGE_MB), QVariant::fromValue(ValueID::GPU_VRAM_USAGE_MB));

            break;
        default:
            setSecondaryEditEnabled(false, false);
            break;
    }
}

void Dialog_deineTopbarItem::on_btn_cancel_clicked()
{
    this->setResult(QDialog::Rejected);
    this->reject();
}

void Dialog_deineTopbarItem::on_btn_save_clicked()
{
    editedSchema = TopbarItemDefinitionSchema(static_cast<ValueID>(ui->combo_primaryData->currentData().toInt()),
                                              getItemType(), ui->frame_primaryColor->palette().background().color());

    if (!ui->combo_secondaryData->currentText().isEmpty() && ui->combo_secondaryData->isEnabled()) {
        editedSchema.setSecondaryValueId(static_cast<ValueID>(ui->combo_secondaryData->currentData().toInt()));
        editedSchema.setSecondaryColor(ui->frame_secondaryColor->palette().background().color());
    }

    if (ui->radio_pie->isChecked()) {
        switch (static_cast<ValueID>(ui->combo_primaryData->currentData().toInt())) {
            case ValueID::CLK_CORE:
                editedSchema.pieMaxValue = gpuParams->maxCoreClock;
                break;
            case ValueID::CLK_MEM:
                editedSchema.pieMaxValue = gpuParams->maxMemClock;
                break;
            default:
                editedSchema.pieMaxValue = 100;
                break;
        }
    }

    this->setResult(QDialog::Accepted);
    this->accept();
}

void Dialog_deineTopbarItem::on_btn_setPrimaryColor_clicked()
{
    ui->frame_primaryColor->setPalette(QPalette(getColor(ui->frame_primaryColor->palette().background().color())));
}

void Dialog_deineTopbarItem::on_btn_setSecondaryColor_clicked()
{
    ui->frame_secondaryColor->setPalette(QPalette(getColor(ui->frame_secondaryColor->palette().background().color())));

}
