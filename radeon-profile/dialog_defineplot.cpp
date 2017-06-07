#include "dialog_defineplot.h"
#include "ui_dialog_defineplot.h"


Dialog_definePlot::Dialog_definePlot(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog_definePlot)
{
    ui->setupUi(this);
    init();

}

int Dialog_definePlot::penStyleToInt(const Qt::PenStyle &ps) {
    switch (ps) {
        case Qt::SolidLine:
            return 0;
        case Qt::DashLine:
            return 1;
        case Qt::DotLine:
            return 2;
    }
    return 0;
}

void Dialog_definePlot::setAvailableGPUData(const QList<ValueID> &gpu) {
    availableGPUData = gpu;

    ui->tree_leftData->addTopLevelItems(createList());
    ui->tree_rightData->addTopLevelItems(createList());
}

void Dialog_definePlot::setEditedPlotSchema(const PlotDefinitionSchema &pds) {
    init();
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
    ui->combo_leftScaleStyle->setCurrentIndex(penStyleToInt(schema.penGridLeft.style()));
    ui->combo_rightScaleStyle->setCurrentIndex(penStyleToInt(schema.penGridRight.style()));
}

Dialog_definePlot::~Dialog_definePlot()
{
    delete ui;
}

void Dialog_definePlot::init() {
    penStyles << tr("Solid line") << tr("Dash line") << tr("Dot Line");
    createStyleCombo();
    ui->widget_left->setVisible(false);
    ui->widget_right->setVisible(false);

    ui->tree_leftData->header()->resizeSection(0, 180);
    ui->tree_rightData->header()->resizeSection(0, 180);
    ui->tree_leftData->header()->resizeSection(1, 20);
    ui->tree_rightData->header()->resizeSection(1, 20);
    ui->widget_rightGridColor->setAutoFillBackground(true);
    ui->widget_leftGridColor->setAutoFillBackground(true);
    ui->widget_background->setAutoFillBackground(true);
}

void Dialog_definePlot::createStyleCombo() {
    ui->combo_leftScaleStyle->addItems(penStyles);
    ui->combo_rightScaleStyle->addItems(penStyles);
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

Qt::PenStyle comboToStyle(int i) {
    switch (i) {
        case 0:
            return Qt::SolidLine;
        case 1:
            return Qt::DashLine;
        case 2:
            return Qt::DotLine;
    }
}

void Dialog_definePlot::on_buttonBox_accepted()
{
    if (ui->line_name->text().isEmpty()) {
        QMessageBox::information(this, tr("Info"), tr("Name cannot be empty."));
        return;
    }

    if (schema.dataListLeft.count() == 0 && schema.dataListRight.count() == 0) {
        QMessageBox::information(this, tr("Info"), tr("Cannot create empty plot."));
        return;
    }

    if (schema.dataListLeft.count() > 0)
        schema.unitLeft = globalStuff::getUnitFomValueId(schema.dataListLeft.keys()[0]);

    if (schema.dataListRight.count() > 0)
        schema.unitRight = globalStuff::getUnitFomValueId(schema.dataListRight.keys()[0]);

    this->setResult(QDialog::Accepted);

    schema.background = ui->widget_background->palette().background().color();
    schema.enableLeft = ui->cb_enableLeftScale->isChecked();
    schema.enableRight = ui->cb_enableRightScale->isChecked();
    schema.name = ui->line_name->text();

    schema.penGridLeft = QPen(ui->widget_leftGridColor->palette().background().color(),
                              ui->spin_leftLineThickness->value(),
                              comboToStyle(ui->combo_leftScaleStyle->currentIndex()));

    schema.penGridRight = QPen(ui->widget_rightGridColor->palette().background().color(),
                              ui->spin_rightLineThickness->value(),
                              comboToStyle(ui->combo_rightScaleStyle->currentIndex()));

    this->accept();
}

QColor Dialog_definePlot::getColor(const QColor &c) {
    QColor color = QColorDialog::getColor(c);
    if (color.isValid())
        return color;

    return c;
}

void Dialog_definePlot::on_tree_leftData_itemDoubleClicked(QTreeWidgetItem *item, int column)
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

void Dialog_definePlot::on_tree_rightData_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column)
    item->setBackgroundColor(1,getColor(item->backgroundColor(1)));
}

PlotDefinitionSchema Dialog_definePlot::getCreatedSchema() {
    return schema;
}

void Dialog_definePlot::on_buttonBox_rejected()
{
    this->reject();
}

void Dialog_definePlot::on_cb_enableLeftScale_clicked(bool checked)
{
    ui->widget_left->setVisible(checked);
}

void Dialog_definePlot::on_cb_enableRightScale_clicked(bool checked)
{
    ui->widget_right->setVisible(checked);
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

void Dialog_definePlot::on_tree_leftData_itemChanged(QTreeWidgetItem *item, int column)
{
    addSelectedItemToSchema(ui->tree_leftData->indexOfTopLevelItem(item), item, schema.dataListLeft);
}

void Dialog_definePlot::on_tree_rightData_itemChanged(QTreeWidgetItem *item, int column)
{
    addSelectedItemToSchema(ui->tree_rightData->indexOfTopLevelItem(item), item, schema.dataListRight);
}
