
// copyright marazmista @ 24.05.2017

#ifndef RPPLOT_H
#define RPPLOT_H

#include <QObject>
#include <QtCharts>
#include "globalStuff.h"
#include <QDebug>

// ZAMIAST NA ENUMACH DZIEDZCZ qvalueaxis i ustawiaj kolory itp, alignment
// series collection w rp.cpp i tam update serii. chart tylko wyświetla

// unit znamy na podstawie typu

class PlotManager;
class RPPlot;



class YAxis : public QValueAxis {

    Q_OBJECT

public:
    ValueUnit unit;

    YAxis() { }

    YAxis(ValueUnit u, QObject *parnt = 0) : QValueAxis(parnt) {
        unit = u;
    }
};


class DataSeries : public QLineSeries {
    Q_OBJECT

public:
    ValueID id;

    DataSeries(ValueID t, QWidget *parent = 0) : QLineSeries(parent) {
        id = t;

        switch (t) {
            case ValueID::CLK_CORE:
                setName(tr("GPU core clock"));
                break;
            default:
                setName(tr("test"));
                break;
        }
    }
};


class RPPlot : public QChartView
{
    Q_OBJECT

public:
    QString name;


    explicit RPPlot() : QChartView() {
        plotArea.setMargins(QMargins(0,0,0,0));
        plotArea.setBackgroundRoundness(0);
        plotArea.legend()->setVisible(false);
        setRenderHint(QPainter::Antialiasing);
        timeAxis.setRange(-50, 50);
        plotArea.addAxis(&timeAxis, Qt::AlignBottom);
//        plotArea.setAnimationOptions(QChart::SeriesAnimations);
        setChart(&plotArea);

        timeAxis.setLabelsVisible(false);
//        timeAxis.setGridLinePen(QPen((QBrush(Qt::yellow)),1,Qt::DashLine));
    }

    void addAxis(const Qt::Alignment &a, const ValueUnit &u, const QPen &p) {
        YAxis *tmpax = new YAxis(u, this);
        tmpax->setGridLinePen(p);
        tmpax->setLabelsColor(p.color());

        if (a == Qt::AlignRight)
            axisRight = tmpax;
        else if (a == Qt::AlignLeft)
            axisLeft = tmpax;

        plotArea.addAxis(tmpax, a);
    }


    QChart plotArea;
    YAxis *axisLeft = nullptr,  *axisRight = nullptr;
    QValueAxis timeAxis;

    QMap<ValueID, DataSeries*> series;
private:

signals:

public slots:
};

class PlotManager {
public:
    QMap<QString, RPPlot*> definedPlots;

    PlotManager() {
    }

    void save();

    void removePlot(const QString &name) {
        for (const ValueID &k : definedPlots[name]->series.keys())
            delete definedPlots[name]->series[k];

        delete definedPlots[name];
    }

    void removeSeries(const QString &pn, const ValueID id) {
        RPPlot *p =  definedPlots.take(pn);
        p->plotArea.removeSeries(p->series[id]);
        delete p;
    }

    void addPlot(const QString &name) {
        RPPlot *rpp = new RPPlot();
        definedPlots.insert(name,rpp);
    }

    void setPlotBackground(const QString &name, const QColor &color) {
        definedPlots[name]->plotArea.setBackgroundBrush(QBrush(color));
    }

    void setLineColor(const QString &pn, ValueID id, const QColor &color) {
        definedPlots[pn]->series[id]->setColor(color);
    }

    bool addSeries(const QString &name, ValueID id) {
        ValueUnit tmpUnit = globalStuff::getUnitFomValueId(id);
        RPPlot *p = definedPlots[name];

        DataSeries *ds = new DataSeries(id, p);


        p->plotArea.addSeries(ds);

        ds->attachAxis(&p->timeAxis);
        ds->id = id;

        if (p->axisLeft != nullptr && p->axisLeft->unit == tmpUnit) {
            p->axisLeft->setRange(0,100);
            ds->attachAxis(definedPlots[name]->axisLeft);
        } else if (p->axisRight != nullptr && p->axisRight->unit == tmpUnit) {
            p->axisRight->setRange(0,100);
            ds->attachAxis(p->axisRight);
        } else {
            delete ds;
            return false;
        }

        ds->setUseOpenGL(true);
        p->series.insert(id, ds);
        return true;
    }

    void updateSeries(int timestamp, const GpuDataContainer &data) {
        for (const QString &rppk : definedPlots.keys()) {
            definedPlots[rppk]->timeAxis.setRange(timestamp -50, timestamp + 50);

            for (const ValueID &dsk : definedPlots[rppk]->series.keys()) {
                definedPlots[rppk]->series[dsk]->append(timestamp, data.value(dsk).value);


                if (definedPlots[rppk]->series[dsk]->count() > 100)
                    definedPlots[rppk]->series[dsk]->remove(0);
            }
        }

    }
};

struct PlotDefinitionSchema {
    struct PlotDataDefinition {
        bool enabled;
        ValueID id;
        QColor lineColor;
    };

    QString name;
    bool enableLeft, enableRight;
    QColor background;
    ValueUnit unitLeft, unitRight;
    QPen penGridLeft, penGridRight;

    QList<PlotDataDefinition> dataListLeft, dataListRight;
};


#endif // RPPLOT_H
