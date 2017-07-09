
// copyright marazmista @ 24.05.2017

#ifndef RPPLOT_H
#define RPPLOT_H

#include <QObject>
#include <QtCharts>
#include "globalStuff.h"
#include <QDebug>

using namespace QtCharts;

class PlotManager;
class RPPlot;

struct PlotInitialValues {
    int left = 0, right = 0;
};

struct PlotAxisSchema {
    bool enabled = false;
    ValueUnit unit;
    QPen penGrid;
    int ticks;

    QMap<ValueID, QColor> dataList;
};

struct PlotDefinitionSchema {
    QString name;
    bool enabled;
    QColor background;

    PlotAxisSchema left, right;
};

class YAxis : public QValueAxis {

    Q_OBJECT

public:
    ValueUnit unit;

    YAxis() { }

    YAxis(ValueUnit u, QObject *parent = 0) : QValueAxis(parent) {
        unit = u;
    }
};


class DataSeries : public QLineSeries {
    Q_OBJECT

public:
    ValueID id;

    DataSeries(ValueID t, QWidget *parent = 0) : QLineSeries(parent) {
        id = t;
        setName(globalStuff::getNameOfValueIDWithUnit(id));
    }
};

class RPPlot : public QChartView
{
    Q_OBJECT

public:
    QString name;
    QChart plotArea;
    YAxis *axisLeft = nullptr,  *axisRight = nullptr;
    QValueAxis timeAxis;

    QMap<ValueID, DataSeries*> series;


    explicit RPPlot() : QChartView() {
        plotArea.setMargins(QMargins(-5,-8,-5,-10));
        plotArea.setMinimumSize(0,0);
        plotArea.setBackgroundRoundness(0);
        plotArea.legend()->setVisible(false);
        setRenderHint(QPainter::Antialiasing);
        plotArea.addAxis(&timeAxis, Qt::AlignBottom);
//        plotArea.setAnimationOptions(QChart::SeriesAnimations);
        setChart(&plotArea);
        timeAxis.setLabelsVisible(false);
        timeAxis.setLineVisible(false);
//        timeAxis.setGridLineVisible(false);
//        timeAxis.setGridLinePen(QPen((QBrush(Qt::yellow)),1,Qt::DashLine));
    }

    ~RPPlot() {
        qDeleteAll(series);
        series.clear();

        delete axisLeft;
        delete axisRight;
    }

    void addAxis(const Qt::Alignment &a, const ValueUnit &u, const QPen &p, const int ticks) {
        YAxis *tmpax = new YAxis(u, this);
        tmpax->setGridLinePen(p);
        tmpax->setLabelsColor(p.color());
        tmpax->setTitleBrush(p.brush());
        tmpax->setTickCount(ticks);
        tmpax->setTitleText(globalStuff::getNameOfUnit(u));

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
                case ValueUnit::MEGAHERTZ:
                case ValueUnit::MEGABYTE:
                case ValueUnit::RPM:
                case ValueUnit::MILIVOLT:
                    axis->setMin(value - 100);
                    return;
                default:
                    return;
            }
        }
    }

    void showLegend(bool show) {
        plotArea.legend()->setVisible(show);
    }
};

class PlotManager {
private:
    int timeRange = 150,
        rightGap = 10;

    constexpr static int maxRange = 1800;

public:
    QMap<QString, RPPlot*> plots;
    QMap<QString, PlotDefinitionSchema> schemas;

    PlotManager() { }

    void setRightGap(bool enabled) {
        rightGap = (enabled) ? 10 : 0;
    }

    void setTimeRange(int range) {
        timeRange = range;
    }

    void setInitialYRange(YAxis *axis, const int &intialValue) {
        switch (axis->unit) {
            case ValueUnit::PERCENT:
                axis->setRange(0, 100);
                return;

            case ValueUnit::CELSIUS:
                axis->setRange(intialValue - 5, intialValue + 5);
                return;

            case ValueUnit::MEGAHERTZ:
            case ValueUnit::MILIVOLT:
            case ValueUnit::RPM:
                axis->setRange(intialValue - 100, intialValue + 200);
                return;

            case ValueUnit::MEGABYTE:
                axis->setRange(intialValue - 100, intialValue + 100);
                return;
            default:
                return;
        }
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

    void createAxis(RPPlot *plot, const PlotAxisSchema &pas, Qt::Alignment align) {
        plot->addAxis(align, pas.unit, pas.penGrid, pas.ticks);

        for (const ValueID &id : pas.dataList.keys()) {
            if (addSeries(plot->name, id))
                setLineColor(plot->name, id, pas.dataList.value(id));
        }
    }

    void createPlotFromSchema(const QString &name, const PlotInitialValues &intialValues) {
        PlotDefinitionSchema pds = schemas.value(name);

        RPPlot *rpp = new  RPPlot();
        rpp->name = pds.name;
        plots.insert(rpp->name, rpp);

        setPlotBackground(rpp->name, pds.background);

        if (pds.left.enabled) {
            createAxis(rpp, pds.left, Qt::AlignLeft);
            setInitialYRange(rpp->axisLeft, intialValues.left);
        }

        if (pds.right.enabled) {
            createAxis(rpp, pds.right, Qt::AlignRight);
            setInitialYRange(rpp->axisRight, intialValues.right);
        }
    }

    void createPlotsFromSchemas(const GPUDataContainer &referenceData) {
        if (schemas.count() == 0)
            return;

        qDeleteAll(plots);
        plots.clear();

        for (const QString &k : schemas.keys()) {
            if (!schemas.value(k).enabled)
                continue;

            PlotInitialValues piv;
            if (schemas.value(k).left.enabled)
                piv.left = referenceData.value(schemas.value(k).left.dataList.keys().at(0)).value;

            if (schemas.value(k).right.enabled)
                piv.right = referenceData.value(schemas.value(k).right.dataList.keys().at(0)).value;

            createPlotFromSchema(k, piv);
        }
    }

    void addPlot(const QString &name) {
        RPPlot *rpp = new  RPPlot();
        plots.insert(name,rpp);
    }

    QColor invertColor(const QColor &c) {
        return QColor::fromRgb(255 - c.red(), 255 - c.green(),255 - c.blue());
    }

    void setPlotBackground(const QString &name, const QColor &color) {
        plots[name]->timeAxis.setGridLineColor(invertColor(color));
        plots[name]->plotArea.legend()->setLabelColor(invertColor(color));
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

        if (p->axisLeft != nullptr && p->axisLeft->unit == tmpUnit)
            ds->attachAxis(p->axisLeft);
        else if (p->axisRight != nullptr && p->axisRight->unit == tmpUnit)
            ds->attachAxis(p->axisRight);
        else {
            delete ds;
            return false;
        }

        ds->setUseOpenGL(true);
        p->series.insert(id, ds);
        return true;
    }

    void cleanupSeries() {
        for (const QString &rppk : plots.keys()) {
            for (ValueID &sk : plots.value(rppk)->series.keys()) {
                if (plots[rppk]->series[sk]->count() > maxRange)
                    plots[rppk]->series[sk]->remove(0);
            }
        }
    }

    void updateSeries(int timestamp, const GPUDataContainer &data) {
        for (const QString &rppk : plots.keys()) {
            plots[rppk]->timeAxis.setRange(timestamp - timeRange, timestamp + rightGap);
            plots[rppk]->updatePlot(timestamp, data);
        }

        // cleaunp every 60 refreshes
        if (timestamp % 60 == 0)
            cleanupSeries();
    }
};


#endif // RPPLOT_H
