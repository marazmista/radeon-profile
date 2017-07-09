#include "dialog_defineplot.h"
#include "ui_dialog_defineplot.h"


Dialog_definePlot::Dialog_definePlot(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog_definePlot)
{
    ui->setupUi(this);
    init();

}

void Dialog_definePlot::setAvailableGPUData(const QList<ValueID> &gpu) {
    availableGPUData = gpu;

    ui->list_leftData->addTopLevelItems(createList());
    ui->list_rightData->addTopLevelItems(createList());
}

void Dialog_definePlot::setEditedPlotSchema(const PlotDefinitionSchema &pds) {
    schema = pds;

    ui->line_name->setText(schema.name);

    ui->cb_enableLeftScale->setChecked(schema.left.enabled);
    ui->cb_enableRightScale->setChecked(schema.right.enabled);
    on_cb_enableLeftScale_clicked(schema.left.enabled);
    on_cb_enableRightScale_clicked(schema.right.enabled);

    ui->frame_background->setPalette(QPalette(schema.background));
    ui->frame_leftGridColor->setPalette(QPalette(schema.left.penGrid.color()));
    ui->slider_thickLeft->setValue(schema.left.penGrid.width());
    ui->frame_rightGridColor->setPalette(QPalette(schema.right.penGrid.color()));
    ui->slider_thickRight->setValue(schema.right.penGrid.width());

    ui->slider_ticksLeft->setValue(schema.left.ticks);
    ui->slider_ticksRight->setValue(schema.right.ticks);

    ui->combo_leftScaleStyle->setCurrentIndex(ui->combo_leftScaleStyle->findData(QVariant::fromValue(schema.left.penGrid.style())));
    ui->combo_rightScaleStyle->setCurrentIndex(ui->combo_rightScaleStyle->findData(QVariant::fromValue(schema.right.penGrid.style())));

    loadListFromSchema(ui->list_leftData, schema.left.dataList);
    loadListFromSchema(ui->list_rightData, schema.right.dataList);
}

Dialog_definePlot::~Dialog_definePlot()
{
    delete ui;
}

void Dialog_definePlot::loadListFromSchema(QTreeWidget *list, QMap<ValueID, QColor> selected) {
    for (int i = 0; i < list->topLevelItemCount(); ++i) {
        if (!selected.contains(listRelationToValueID.value(i)))
            continue;

        list->topLevelItem(i)->setCheckState(0, Qt::Checked);
        list->topLevelItem(i)->setBackgroundColor(1, selected.value(listRelationToValueID.value(i)));
    }
}

void Dialog_definePlot::init() {
    createStyleCombo(ui->combo_leftScaleStyle);
    createStyleCombo(ui->combo_rightScaleStyle);

    ui->widget_left->setVisible(false);
    ui->widget_right->setVisible(false);

    ui->list_leftData->header()->resizeSection(0, 180);
    ui->list_rightData->header()->resizeSection(0, 180);
    ui->list_leftData->header()->resizeSection(1, 20);
    ui->list_rightData->header()->resizeSection(1, 20);
    ui->frame_rightGridColor->setAutoFillBackground(true);
    ui->frame_leftGridColor->setAutoFillBackground(true);
    ui->frame_background->setAutoFillBackground(true);
}

void Dialog_definePlot::createStyleCombo(QComboBox *combo) {
    combo->addItem(tr("Solid line"), QVariant::fromValue(Qt::SolidLine));
    combo->addItem(tr("Dash line"), QVariant::fromValue(Qt::DashLine));
    combo->addItem(tr("Dot Line"), QVariant::fromValue(Qt::DotLine));
}

QList<QTreeWidgetItem* > Dialog_definePlot::createList() {
    QList<QTreeWidgetItem *> items;

    for (const ValueID id : availableGPUData) {
        if (!globalStuff::isValueIdPlottable(id))
            continue;

        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        item->setCheckState(0, Qt::Unchecked);
        item->setText(0, globalStuff::getNameOfValueIDWithUnit(id));
        items << item;

        listRelationToValueID.insert(items.count() - 1, id);
    }
    return items;
}

QColor Dialog_definePlot::getColor(const QColor &c) {
    QColor color = QColorDialog::getColor(c);
    if (color.isValid())
        return color;

    return c;
}

void Dialog_definePlot::on_list_leftData_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column)
    item->setBackgroundColor(1,getColor(item->backgroundColor(1)));
}

void Dialog_definePlot::on_btn_setBackground_clicked()
{
    ui->frame_background->setPalette(QPalette(getColor(ui->frame_background->palette().background().color())));

}

void Dialog_definePlot::on_btn_leftScaleColor_clicked()
{
    ui->frame_leftGridColor->setPalette(QPalette(getColor(ui->frame_leftGridColor->palette().background().color())));
}

void Dialog_definePlot::on_btn_rightScaleColor_clicked()
{
    ui->frame_rightGridColor->setPalette(QPalette(getColor(ui->frame_rightGridColor->palette().background().color())));
}

void Dialog_definePlot::on_list_rightData_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column)
    item->setBackgroundColor(1,getColor(item->backgroundColor(1)));
}

PlotDefinitionSchema Dialog_definePlot::getCreatedSchema() {
    return schema;
}

void Dialog_definePlot::on_cb_enableLeftScale_clicked(bool checked)
{
    ui->widget_left->setVisible(checked);
    if (!checked)
        schema.left.dataList.clear();
}

void Dialog_definePlot::on_cb_enableRightScale_clicked(bool checked)
{
    ui->widget_right->setVisible(checked);
    if (!checked)
        schema.right.dataList.clear();
}

void Dialog_definePlot::addSelectedItemToSchema(int itemIndex, QTreeWidgetItem *item , QMap<ValueID, QColor> &schemaDataList) {
    if (item->checkState(0)  == Qt::Unchecked)
        schemaDataList.remove(listRelationToValueID.value(itemIndex));

    if (item->checkState(0) == Qt::Checked) {
        if (!schemaDataList.isEmpty() && globalStuff::getUnitFomValueId(schemaDataList.keys().at(0)) != globalStuff::getUnitFomValueId(listRelationToValueID.value(itemIndex))) {
            QMessageBox::information(this, "Info", tr("Values with different units cannot be selected for the same scale"));
            item->setCheckState(0, Qt::Unchecked);
        } else
            schemaDataList.insert(listRelationToValueID.value(itemIndex), item->backgroundColor(1));
    }
}

void Dialog_definePlot::on_list_leftData_itemChanged(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    addSelectedItemToSchema(ui->list_leftData->indexOfTopLevelItem(item), item, schema.left.dataList);
}

void Dialog_definePlot::on_list_rightData_itemChanged(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    addSelectedItemToSchema(ui->list_rightData->indexOfTopLevelItem(item), item, schema.right.dataList);
}

void Dialog_definePlot::on_btn_save_clicked()
{
    if (ui->line_name->text().isEmpty()) {
        QMessageBox::information(this, tr("Info"), tr("Name cannot be empty."));
        return;
    }

    if ((schema.left.dataList.count() == 0 && schema.right.dataList.count() == 0) ||
            (!ui->cb_enableLeftScale->isChecked() && !ui->cb_enableRightScale->isChecked())) {
        QMessageBox::information(this, tr("Info"), tr("Cannot create empty plot."));
        return;
    }

    schema.background = ui->frame_background->palette().background().color();
    schema.name = ui->line_name->text();
    schema.left.enabled = ui->cb_enableLeftScale->isChecked();
    schema.right.enabled = ui->cb_enableRightScale->isChecked();
    schema.left.ticks = ui->slider_ticksLeft->value();
    schema.right.ticks = ui->slider_ticksRight->value();


    if (schema.left.dataList.count() > 0)
        schema.left.unit = globalStuff::getUnitFomValueId(schema.left.dataList.keys()[0]);
    else
        schema.left.enabled = false;

    if (schema.right.dataList.count() > 0)
        schema.right.unit = globalStuff::getUnitFomValueId(schema.right.dataList.keys()[0]);
    else
        schema.right.enabled = false;

    schema.left.penGrid = QPen(ui->frame_leftGridColor->palette().background().color(),
                              ui->slider_thickLeft->value(),
                              static_cast<Qt::PenStyle>(ui->combo_leftScaleStyle->currentData().toInt()));

    schema.right.penGrid = QPen(ui->frame_rightGridColor->palette().background().color(),
                              ui->slider_thickRight->value(),
                              static_cast<Qt::PenStyle>(ui->combo_rightScaleStyle->currentData().toInt()));

    schema.enabled = true;

    this->setResult(QDialog::Accepted);
    this->accept();
}

void Dialog_definePlot::on_btn_cancel_clicked()
{
    this->setResult(QDialog::Rejected);
    this->reject();
}
