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

    ui->combo_leftUnit->setCurrentIndex(static_cast<int>(schema.unitLeft));
    ui->combo_rightUnit->setCurrentIndex(static_cast<int>(schema.unitRight));
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


    ui->combo_leftUnit->addItems(createUnitCombo());
    ui->combo_rightUnit->addItems(createUnitCombo());

    ui->tree_leftData->header()->resizeSection(0, 150);
    ui->tree_rightData->header()->resizeSection(0, 150);
    ui->widget_rightGridColor->setAutoFillBackground(true);
    ui->widget_leftGridColor->setAutoFillBackground(true);
    ui->widget_background->setAutoFillBackground(true);
}

void Dialog_definePlot::createStyleCombo() {
    ui->combo_leftScaleStyle->addItems(penStyles);
    ui->combo_rightScaleStyle->addItems(penStyles);
}

QStringList Dialog_definePlot::createUnitCombo() {
    QStringList unitComboList;

    unitComboList.append(tr("Megahertz"));
    unitComboList.append(tr("Percent"));
    unitComboList.append(tr("Celsius"));
    unitComboList.append(tr("Megabyte"));
    unitComboList.append(tr("Milivolt"));
    unitComboList.append("RPM");

    return unitComboList;
}


QList<QTreeWidgetItem* > Dialog_definePlot::createList(QStringList titles) {
    QList<QTreeWidgetItem *> items;

    for (int i = 0; i < titles.count(); ++i) {
        QTreeWidgetItem *item = new QTreeWidgetItem();
        item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        item->setCheckState(0, Qt::Unchecked);
        item->setText(0, titles[i]);
        items << item;
    }
    return items;
}

QList<QTreeWidgetItem* > Dialog_definePlot::createDataListFromUnit(ValueUnit u) {
    switch (u) {
        case ValueUnit::MEGAHERTZ:
            return createList(QStringList() << tr("Core clock") << tr("Meomory clock"));
        case ValueUnit::CELSIUS:
            return createList(QStringList() << tr("Temperature"));
        case ValueUnit::PERCENT:
            return createList(QStringList() << tr("GPU load") << tr("GPU Vram load [%]") << tr("Fan speed [%]"));
        case ValueUnit::MEGABYTE:
            return createList(QStringList() << tr("GPU Vram load [MB]"));
        case ValueUnit::MILIVOLT:
            return createList(QStringList() << tr("Core volt") << tr("Mem volt"));
        case ValueUnit::RPM:
            return createList(QStringList() << tr("Fan speed [rpm]"));
    }

}

void Dialog_definePlot::on_combo_leftUnit_currentIndexChanged(int index)
{

    schema.dataListLeft.clear();
    ui->tree_leftData->clear();
    ui->tree_leftData->addTopLevelItems(createDataListFromUnit(static_cast<ValueUnit>(index)));
}

void Dialog_definePlot::on_combo_rightUnit_currentIndexChanged(int index)
{
    schema.dataListRight.clear();
    ui->tree_rightData->clear();
    ui->tree_rightData->addTopLevelItems(createDataListFromUnit(static_cast<ValueUnit>(index)));
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

    for (int i = 0; i < ui->tree_leftData->topLevelItemCount(); ++i) {
        if (ui->tree_leftData->topLevelItem(i)->checkState(0) == Qt::Checked)
            schema.dataListLeft.append(addToSchemaData(static_cast<ValueUnit>(ui->combo_leftUnit->currentIndex()),
                                                                              i,
                                                                              ui->tree_leftData->topLevelItem(i)->backgroundColor(1)));
    }

    for (int i = 0; i < ui->tree_rightData->topLevelItemCount(); ++i) {
        if (ui->tree_rightData->topLevelItem(i)->checkState(0) == Qt::Checked)
            schema.dataListRight.append(addToSchemaData(static_cast<ValueUnit>(ui->combo_rightUnit->currentIndex()),
                                                                              i,
                                                                              ui->tree_rightData->topLevelItem(i)->backgroundColor(1)));
    }

    if (schema.dataListLeft.count() == 0 && schema.dataListRight.count() == 0) {
        QMessageBox::information(this, tr("Info"), tr("Cannot create empty plot."));
        return;
    }

    this->setResult(QDialog::Accepted);

    schema.background = ui->widget_background->palette().background().color();
    schema.enableLeft = ui->cb_enableLeftScale->isChecked();
    schema.enableRight = ui->cb_enableRightScale->isChecked();
    schema.name = ui->line_name->text();
    schema.unitLeft = static_cast<ValueUnit>(ui->combo_leftUnit->currentIndex());
    schema.unitRight = static_cast<ValueUnit>(ui->combo_rightUnit->currentIndex());

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

SeriesSchema Dialog_definePlot::addToSchemaData(const ValueUnit &u, const int itemIdx, const QColor &itemColor) {
    SeriesSchema pdd(itemColor);

    switch (u) {
        case 0:
            switch (itemIdx) {
                case 0:
                    pdd.id = ValueID::CLK_CORE;
                    break;
                case 1:
                    pdd.id = ValueID::CLK_MEM;
            }
            break;
        case 1:
            switch (itemIdx) {
                case 0:
                    pdd.id = ValueID::GPU_LOAD_PERCENT;
                    break;
                case 1:
                    pdd.id = ValueID::GPU_VRAM_LOAD_PERCENT;
                    break;
                case 2:
                    pdd.id = ValueID::FAN_SPEED_PERCENT;
                    break;
            }
            break;
        case 2:
            pdd.id = ValueID::TEMPERATURE_CURRENT;
            break;
        case 3:
            pdd.id = ValueID::GPU_VRAM_LOAD_MB;
            break;
        case 4:
            switch (itemIdx) {
                case 0:
                    pdd.id = ValueID::VOLT_CORE;
                    break;
                case 1:
                    pdd.id = ValueID::VOLT_MEM;
                    break;
            }
        case 5:
            pdd.id = ValueID::FAN_SPEED_RPM;
            break;
    }

    return pdd;

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
