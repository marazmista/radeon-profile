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

    ui->cb_enableLeftScale->setChecked(schema.enableLeft);
    ui->cb_enableRightScale->setChecked(schema.enableRight);
    on_cb_enableLeftScale_clicked(schema.enableLeft);
    on_cb_enableRightScale_clicked(schema.enableRight);

    ui->widget_background->setPalette(QPalette(schema.background));
    ui->widget_leftGridColor->setPalette(QPalette(schema.penGridLeft.color()));
    ui->spin_leftLineThickness->setValue(schema.penGridLeft.width());
    ui->widget_rightGridColor->setPalette(QPalette(schema.penGridRight.color()));
    ui->spin_rightLineThickness->setValue(schema.penGridRight.width());

    ui->combo_leftScaleStyle->setCurrentIndex(ui->combo_leftScaleStyle->findData(QVariant::fromValue(schema.penGridLeft.style())));
    ui->combo_rightScaleStyle->setCurrentIndex(ui->combo_rightScaleStyle->findData(QVariant::fromValue(schema.penGridRight.style())));

    loadListFromSchema(ui->list_leftData, schema.dataListLeft);
    loadListFromSchema(ui->list_rightData, schema.dataListRight);
}

Dialog_definePlot::~Dialog_definePlot()
{
    delete ui;
}

void Dialog_definePlot::loadListFromSchema(QTreeWidget *list, QMap<ValueID, QColor> selected) {
    for (int i = 0; i < list->topLevelItemCount(); ++i) {
        if (selected.contains(listRelationToValueID.value(i))) {
           list->topLevelItem(i)->setCheckState(0, Qt::Checked);
           list->topLevelItem(i)->setBackgroundColor(1, selected.value(listRelationToValueID.value(i)));
        }
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
    ui->widget_rightGridColor->setAutoFillBackground(true);
    ui->widget_leftGridColor->setAutoFillBackground(true);
    ui->widget_background->setAutoFillBackground(true);
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
        item->setText(0, globalStuff::getNameOfValueID(id));
        items << item;

        listRelationToValueID.insert(items.count() - 1, id);
    }
    return items;
}

void Dialog_definePlot::on_buttonBox_accepted()
{
    if (ui->line_name->text().isEmpty()) {
        QMessageBox::information(this, tr("Info"), tr("Name cannot be empty."));
        return;
    }

    if ((schema.dataListLeft.count() == 0 && schema.dataListRight.count() == 0) ||
            (!ui->cb_enableLeftScale->isChecked() && !ui->cb_enableRightScale->isChecked())) {
        QMessageBox::information(this, tr("Info"), tr("Cannot create empty plot."));
        return;
    }

    schema.background = ui->widget_background->palette().background().color();
    schema.enableLeft = ui->cb_enableLeftScale->isChecked();
    schema.enableRight = ui->cb_enableRightScale->isChecked();
    schema.name = ui->line_name->text();

    if (schema.dataListLeft.count() > 0)
        schema.unitLeft = globalStuff::getUnitFomValueId(schema.dataListLeft.keys()[0]);
    else
        schema.enableLeft = false;

    if (schema.dataListRight.count() > 0)
        schema.unitRight = globalStuff::getUnitFomValueId(schema.dataListRight.keys()[0]);
    else
        schema.enableRight = false;

    schema.penGridLeft = QPen(ui->widget_leftGridColor->palette().background().color(),
                              ui->spin_leftLineThickness->value(),
                              static_cast<Qt::PenStyle>(ui->combo_leftScaleStyle->currentData().toInt()));

    schema.penGridRight = QPen(ui->widget_rightGridColor->palette().background().color(),
                              ui->spin_rightLineThickness->value(),
                              static_cast<Qt::PenStyle>(ui->combo_rightScaleStyle->currentData().toInt()));

    schema.enabled = true;

    this->setResult(QDialog::Accepted);
    this->accept();
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
    ui->widget_background->setPalette(QPalette(getColor(ui->widget_background->palette().background().color())));

}

void Dialog_definePlot::on_btn_leftScaleColor_clicked()
{
    ui->widget_leftGridColor->setPalette(QPalette(getColor(ui->widget_leftGridColor->palette().background().color())));
}

void Dialog_definePlot::on_btn_rightScaleColor_clicked()
{
    ui->widget_rightGridColor->setPalette(QPalette(getColor(ui->widget_rightGridColor->palette().background().color())));
}

void Dialog_definePlot::on_list_rightData_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column)
    item->setBackgroundColor(1,getColor(item->backgroundColor(1)));
}

PlotDefinitionSchema Dialog_definePlot::getCreatedSchema() {
    return schema;
}

void Dialog_definePlot::on_buttonBox_rejected()
{
    this->setResult(QDialog::Rejected);
    this->reject();
}

void Dialog_definePlot::on_cb_enableLeftScale_clicked(bool checked)
{
    ui->widget_left->setVisible(checked);
    if (!checked)
        schema.dataListLeft.clear();
}

void Dialog_definePlot::on_cb_enableRightScale_clicked(bool checked)
{
    ui->widget_right->setVisible(checked);
    if (!checked)
        schema.dataListRight.clear();
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
    addSelectedItemToSchema(ui->list_leftData->indexOfTopLevelItem(item), item, schema.dataListLeft);
}

void Dialog_definePlot::on_list_rightData_itemChanged(QTreeWidgetItem *item, int column)
{
    addSelectedItemToSchema(ui->list_rightData->indexOfTopLevelItem(item), item, schema.dataListRight);
}
