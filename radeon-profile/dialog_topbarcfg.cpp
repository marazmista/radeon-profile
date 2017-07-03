#include "dialog_topbarcfg.h"
#include "ui_dialog_topbarcfg.h"

void Dialog_topbarCfg::addColumn(const int column) {
    QPushButton *b = new QPushButton(this);
//    b->setMaximumSize(40,40);
    b->setText("+");
    QPushButton *b2 = new QPushButton(this);
//    b2->setMaximumSize(40,40);
    b2->setText("+");

    grid->addWidget(b, 0,column, Qt::AlignLeft);
    grid->addWidget(b2, 1,column, Qt::AlignLeft);
}

Dialog_topbarCfg::Dialog_topbarCfg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog_topbarCfg)
{
    ui->setupUi(this);
    grid = new QGridLayout(this);
    ui->frame->setLayout(grid);

    addColumn(0);
    addColumn(1);
    grid->setColumnStretch(grid->columnCount()-1,1);
}

Dialog_topbarCfg::~Dialog_topbarCfg()
{
    delete ui;
}
