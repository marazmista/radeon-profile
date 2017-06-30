
// copyright marazmista @ 23.05.2017

#ifndef PIEPROGRESSBAR_H
#define PIEPROGRESSBAR_H

#include <QWidget>
#include <QtCharts>
#include "globalStuff.h"
#include "ui_pieprogressbar.h"

using namespace QtCharts;

namespace Ui {
class PieProgressBar;
}


class PieProgressBar : public QWidget
{
    Q_OBJECT

public:
    explicit PieProgressBar(QWidget *parent = 0) : QWidget(parent), ui(new Ui::PieProgressBar){
        ui->setupUi(this);
        init();
    }

    explicit PieProgressBar(const int max, ValueID id, QColor fillColor = Qt::yellow, QWidget *parent = 0) : QWidget(parent), ui(new Ui::PieProgressBar) {
        ui->setupUi(this);

        maxValue = max;
        dataId = id;
        fill = fillColor;
        bg = QWidget::palette().color(QWidget::backgroundRole());
        init();
    }

    virtual ~PieProgressBar() { }

    void updateValue(const GPUDataContainer &gpuData) {
        data.slices().at(1)->setValue(gpuData.value(dataId).value);
        data.slices().at(2)->setValue(maxValue - gpuData.value(dataId).value);
        label.setText(gpuData.value(dataId).strValue);
    }

protected:
    Ui::PieProgressBar *ui;
    int maxValue = 100;
    ValueID dataId;

    QPieSeries data;
    QChart chart;
    QChartView *chartView;
    QLabel label;
    QColor fill, bg;

    void init() {
        QFont f;
        f.setFamily("Monospace");
        f.setPointSize(8);
        label.setFont(f);

        QPalette p = this->palette();
        setAutoFillBackground(true);
        p.setColor(QPalette::Background, Qt::black);

        chartView = new QChartView(this);

        data.setPieStartAngle(-250);
        data.setPieEndAngle(90);
//        data.setPieStartAngle(-210);
//        data.setPieEndAngle(130);
        data.setHoleSize(0.32);

        data.append("",maxValue / 3);
        data.append("Usage", 0);
        data.append("", maxValue);

        data.slices().at(0)->setBrush(bg);
        data.slices().at(0)->setPen(QPen(bg, 0));
        data.slices().at(1)->setPen(QPen(bg, 0));
        data.slices().at(2)->setPen(QPen(bg, 0));
        data.slices().at(0)->setLabelVisible(false);
        data.slices().at(1)->setLabelVisible(false);
        data.slices().at(2)->setLabelVisible(false);
        data.slices().at(1)->setBrush(fill);
        data.slices().at(2)->setBrush(Qt::darkGray);

        chart.addSeries(&data);
        chart.legend()->setVisible(false);
        chart.setBackgroundVisible(false);

        chart.setMargins(QMargins(-18,-18,-18,-18));
        chart.setContentsMargins(0,0,0,0);
        chart.setMinimumSize(0,0);

        chartView->setChart(&chart);
        chartView->setMinimumSize(0,0);
        chartView->setRenderHint(QPainter::Antialiasing);

        label.setContentsMargins(QMargins(0,18,7,0));
        ui->grid->addWidget(chartView,0,0,Qt::AlignCenter);
        ui->grid->addWidget(&label,0,0,Qt::AlignRight);
    }

};

#endif // PIEPROGRESSBAR_H
