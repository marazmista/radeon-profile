
// copyright marazmista @ 24.05.2017

#ifndef RPPLOT_H
#define RPPLOT_H

#include <QObject>
#include <QtCharts>
#include "globalStuff.h"
#include <QDebug>

class PlotManager;
class RPPlot;

struct PlotDefinitionSchema {
    QString name;
    bool enabled, enableLeft = false, enableRight = false;
    QColor background;
    ValueUnit unitLeft, unitRight;
    QPen penGridLeft, penGridRight;

    QMap<ValueID, QColor> dataListLeft, dataListRight;
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
        setName(globalStuff::getNameOfValueID(id));
    }
};

class RPPlot : public QChartView
{
    Q_OBJECT

public:
    QString name;

    explicit RPPlot() : QChartView() {
        plotArea.setMargins(QMargins(-8,-8,-8,-8));
        plotArea.setMinimumSize(0,0);
        plotArea.setBackgroundRoundness(0);
        plotArea.legend()->setVisible(false);
        setRenderHint(QPainter::Antialiasing);
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

    void updatePlot(int timestamp, const GPUDataContainer &data) {
        for (const ValueID &dsk : series.keys()) {
            series[dsk]->append(timestamp, data.value(dsk).value);
            rescale(axisLeft, data.value(dsk).value, globalStuff::getUnitFomValueId(series[dsk]->id));
            rescale(axisRight, data.value(dsk).value, globalStuff::getUnitFomValueId(series[dsk]->id));
        }
    }

    void setInitialScale(YAxis *axis,  ValueUnit unit) {
        switch (unit) {
            case ValueUnit::PERCENT:
                axis->setRange(0, 100);
                return;

            case ValueUnit::CELSIUS:
                axis->setRange(30, 50);
                return;

            case ValueUnit::MEGAHERTZ:
            case ValueUnit::MILIVOLT:
                axis->setRange(200, 500);
                return;
            case ValueUnit::RPM:
                axis->setRange(200, 500);
                return;
        }
    }

    void rescale(YAxis *axis,  float value,  ValueUnit unit) {
        // percent has const scale 0-100
        if (unit == ValueUnit::PERCENT)
            return;

        if (axis == nullptr)
            return;

        if (unit == axis->unit && axis->max() < value) {
            switch (axis->unit) {
                case ValueUnit::CELSIUS:
                    axis->setMax(value + 5);
                    return;
                case ValueUnit::MEGABYTE:
                    axis->setMax(value + 50);
                    return;
                default:
                    axis->setMax(value + 150);
                    return;
            }
        }

        if (axis->unit == unit && axis->min() > value) {
            switch (unit) {
                case ValueUnit::CELSIUS:
                    axis->setMin(value - 5);
                    return;
                default:
                    return;
            }
        }
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
    QMap<QString, RPPlot*> plots;
    QMap<QString, PlotDefinitionSchema> schemas;

    PlotManager() {
    }

    void addSchema(const PlotDefinitionSchema &pds) {
        schemas.insert(pds.name, pds);
    }

    void removeSchema(const QString &name) {
        if (plots.contains(name))
            removePlot(name);

        schemas.remove(name);
    }

    void removePlot(const QString &name) {
        RPPlot *rpp = plots.take(name);
        delete rpp;
    }

    void modifySchemaState(const QString &name, bool enabled) {
        schemas[name].enabled = enabled;

        if (enabled)
            createPlotFromSchema(name);
        else
            removePlot(name);
    }

    void createPlotFromSchema(const QString &name) {
        PlotDefinitionSchema pds = schemas.value(name);

        RPPlot *rpp = new  RPPlot();
        rpp->name = pds.name;
        plots.insert(rpp->name, rpp);

        setPlotBackground(rpp->name, pds.background);

        if (pds.enableLeft) {
            rpp->addAxis(Qt::AlignLeft, pds.unitLeft, pds.penGridLeft);

            for (const ValueID &id : pds.dataListLeft.keys()) {
                if (addSeries(rpp->name, id))
                    setLineColor(rpp->name, id, pds.dataListLeft.value(id));
            }
        }

        if (pds.enableRight) {
            rpp->addAxis(Qt::AlignRight, pds.unitRight, pds.penGridRight);

            for (const ValueID &id : pds.dataListRight.keys()) {
                if (addSeries(rpp->name, id))
                    setLineColor(rpp->name, id, pds.dataListRight.value(id));
            }
        }
    }

    void recreatePlotsFromSchemas() {
        qDeleteAll(plots);
        plots.clear();

        for (const QString &k : schemas.keys()) {
            if (!schemas.value(k).enabled)
                continue;

            createPlotFromSchema(k);
        }
    }

    void removeSeries(const QString &pn, const ValueID id) {
        RPPlot *p =  plots.take(pn);
        p->plotArea.removeSeries(p->series[id]);
        delete p;
    }

    void addPlot(const QString &name) {
        RPPlot *rpp = new  RPPlot();
        plots.insert(name,rpp);
    }

    void setPlotBackground(const QString &name, const QColor &color) {
        plots[name]->plotArea.setBackgroundBrush(QBrush(color));
        plots[name]->setBackgroundBrush(QBrush(color));
    }

    void setLineColor(const QString &pn, ValueID id, const QColor &color) {
        plots[pn]->series[id]->setColor(color);
    }

    bool addSeries(const QString &name, ValueID id) {
        ValueUnit tmpUnit = globalStuff::getUnitFomValueId(id);
        RPPlot *p = plots[name];

        DataSeries *ds = new DataSeries(id, p);

        p->plotArea.addSeries(ds);

        ds->attachAxis(&p->timeAxis);
        ds->id = id;

        if (p->axisLeft != nullptr && p->axisLeft->unit == tmpUnit) {
            p->setInitialScale(p->axisLeft, tmpUnit);
            ds->attachAxis(plots[name]->axisLeft);
        } else if (p->axisRight != nullptr && p->axisRight->unit == tmpUnit) {
            p->setInitialScale(p->axisRight, tmpUnit);
            ds->attachAxis(p->axisRight);
        } else {
            delete ds;
            return false;
        }

        ds->setUseOpenGL(true);
        p->series.insert(id, ds);
        return true;
    }

    void updateSeries(int timestamp, const GPUDataContainer &data) {
        for (const QString &rppk : plots.keys()) {
            plots[rppk]->timeAxis.setRange(timestamp - 150, timestamp + 10);
            plots[rppk]->updatePlot(timestamp, data);

//                if (definedPlots[rppk]->series[dsk]->count() > 100)
//                    definedPlots[rppk]->series[dsk]->remove(0);
        }
    }
};


#endif // RPPLOT_H
