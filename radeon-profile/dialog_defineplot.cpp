#include "dialog_defineplot.h"
#include "ui_dialog_defineplot.h"


Dialog_definePlot::Dialog_definePlot(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog_definePlot)
{
    ui->setupUi(this);
    init();

}

Dialog_definePlot::Dialog_definePlot(const PlotDefinitionSchema &s, QWidget *parent) : QDialog(parent) {
    init();
    schema = s;
}


Dialog_definePlot::~Dialog_definePlot()
{
    delete ui;
}


void Dialog_definePlot::init() {
    createStyleCombo();

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
    ui->tree_leftData->clear();
    ui->tree_leftData->addTopLevelItems(createDataListFromUnit(static_cast<ValueUnit>(index)));
}

void Dialog_definePlot::on_combo_rightUnit_currentIndexChanged(int index)
{
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
    if (ui->line_name->text().isEmpty())
        return;


//    for (int i = 0; i < ui->tree_leftData->topLevelItemCount(); ++i) {
//        if (ui->tree_leftData->topLevelItem(i)->checkState(0) == Qt::Checked) {
//            break;
//        }
//    }


//    for (int i = 0; i < ui->tree_rightData->topLevelItemCount(); ++i) {
//        if (ui->tree_rightData->topLevelItem(i)->checkState(0) == Qt::Checked) {
//            PlotDefinitionSchema::PlotDataDefinition pf

//            break;
//        }
//    }

    schema.background = ui->widget_background->palette().background().color();
    schema.enableLeft = ui->cb_enableLeftScale->isChecked();
    schema.enableRight = ui->cb_enableRightScale->isChecked();
    schema.name = ui->line_name->text();
    schema.unitLeft = static_cast<ValueUnit>(ui->combo_leftUnit->currentIndex());
    schema.unitRight = static_cast<ValueUnit>(ui->combo_rightUnit->currentIndex());


    schema.penGridLeft = QPen(ui->btn_leftScaleColor->palette().background().color(),
                              ui->spin_leftLineThickness->value(),
                              comboToStyle(ui->combo_leftScaleStyle->currentIndex()));
    schema.penGridRight = QPen(ui->btn_rightScaleColor->palette().background().color(),
                              ui->spin_rightLineThickness->value(),
                              comboToStyle(ui->combo_rightScaleStyle->currentIndex()));

}

QColor Dialog_definePlot::getColor(const QColor &c) {
    QColor color = QColorDialog::getColor(c);
    if (color.isValid())
        return color;

    return c;
}

void Dialog_definePlot::on_tree_leftData_itemChanged(QTreeWidgetItem *item, int column)
{

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
