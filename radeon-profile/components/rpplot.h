
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

struct SeriesSchema {
    ValueID id;
    QColor lineColor;

    SeriesSchema() { }
    SeriesSchema(const QColor &c) {
        lineColor = c;
    }

    SeriesSchema(const ValueID &i, const QColor &c) {
        id = i;
        lineColor = c;
    }
};

struct PlotDefinitionSchema {
    QString name;
    bool enableLeft, enableRight;
    QColor background;
    ValueUnit unitLeft, unitRight;
    QPen penGridLeft, penGridRight;

    QList<SeriesSchema> dataListLeft, dataListRight;
};

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

    ~RPPlot() {
        qDeleteAll(series);
        series.clear();

        delete axisLeft;
        delete axisRight;
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
    QMap<QString, PlotDefinitionSchema> definedPlotsSchemas;

    PlotManager() {
    }

    void save();

    void addSchema(const PlotDefinitionSchema &pds) {
        definedPlotsSchemas.insert(pds.name, pds);
    }

    void removeSchema(const QString &name) {
        definedPlotsSchemas.remove(name);
    }

    void removePlot(const QString &name) {
        RPPlot *rpp = definedPlots.take(name);
        qDeleteAll(rpp->series);
        rpp->series.clear();

        delete rpp;
    }

    void createPlotsFromSchemas() {
        qDeleteAll(definedPlots);
        definedPlots.clear();

        for (const QString &k : definedPlotsSchemas.keys()) {
            PlotDefinitionSchema pds = definedPlotsSchemas.value(k);

            RPPlot *rpp = new  RPPlot();
            rpp->name = pds.name;
            definedPlots.insert(rpp->name, rpp);

            setPlotBackground(rpp->name, pds.background);

            if (pds.enableLeft) {
                rpp->addAxis(Qt::AlignLeft, pds.unitLeft, pds.penGridLeft);

                for (int i = 0; i < pds.dataListLeft.count(); ++i) {
                    addSeries(rpp->name, pds.dataListLeft[i].id);
                    setLineColor(rpp->name, pds.dataListLeft[i].id, pds.dataListLeft[i].lineColor);
                }
            }

            if (pds.enableRight) {
                rpp->addAxis(Qt::AlignRight, pds.unitRight, pds.penGridRight);

                for (int i = 0; i < pds.dataListRight.count(); ++i) {
                    addSeries(rpp->name, pds.dataListRight[i].id);
                    setLineColor(rpp->name, pds.dataListRight[i].id, pds.dataListRight[i].lineColor);
                }
            }
        }
    }

    void removeSeries(const QString &pn, const ValueID id) {
        RPPlot *p =  definedPlots.take(pn);
        p->plotArea.removeSeries(p->series[id]);
        delete p;
    }

    void addPlot(const QString &name) {
        RPPlot *rpp = new  RPPlot();
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
            definedPlots[rppk]->timeAxis.setRange(timestamp -100, timestamp + 20);

            for (const ValueID &dsk : definedPlots[rppk]->series.keys()) {
                definedPlots[rppk]->series[dsk]->append(timestamp, data.value(dsk).value);


                if (definedPlots[rppk]->series[dsk]->count() > 100)
                    definedPlots[rppk]->series[dsk]->remove(0);
            }
        }
    }
};


#endif // RPPLOT_H
