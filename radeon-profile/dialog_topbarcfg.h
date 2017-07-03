#ifndef DIALOG_TOPBARCFG_H
#define DIALOG_TOPBARCFG_H

#include <QDialog>
#include <QGridLayout>

namespace Ui {
class Dialog_topbarCfg;
}

class Dialog_topbarCfg : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog_topbarCfg(QWidget *parent = 0);
    ~Dialog_topbarCfg();

private:
    Ui::Dialog_topbarCfg *ui;
    QGridLayout *grid;

    void addColumn(const int column);

};

#endif // DIALOG_TOPBARCFG_H
