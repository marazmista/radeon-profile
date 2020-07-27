
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
    bool enabled, modified = false;
    QColor background;
    RPPlot *plot = nullptr;

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

    void setMinAboveZero(const int &value, const int &offset) {
        this->setMin((value - offset < 0) ? 0 : value - offset);
    }

    void setRangeAboveZero(const int &value, const int &offset) {
        this->setRange((value - offset < 0) ? 0 : value - offset,
                       value + offset);
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
    QChart plotArea;
    YAxis *axisLeft = nullptr,  *axisRight = nullptr;
    QValueAxis timeAxis;

    QList<DataSeries*> series;


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
        for (int i = 0; i < series.count(); ++i) {
            series.at(i)->append(timestamp, data.value(series.at(i)->id).value);
            rescale(axisLeft, data.value(series.at(i)->id).value, globalStuff::getUnitFomValueId(series.at(i)->id));
            rescale(axisRight, data.value(series.at(i)->id).value, globalStuff::getUnitFomValueId(series.at(i)->id));
        }
    }

    void rescale(YAxis *axis,  float value,  ValueUnit unit) {
        if (axis == nullptr)
            return;

        if (unit == axis->unit && axis->max() < value) {
            switch (axis->unit) {
                case ValueUnit::CELSIUS:
                    axis->setMax(value + 5);
                    return;
                case ValueUnit::PERCENT:
                    axis->setMax(value + 10);
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
                    axis->setMinAboveZero(value, 5);
                    return;
                case ValueUnit::PERCENT:
                    axis->setMinAboveZero(value, 10);
                    return;
                case ValueUnit::MEGAHERTZ:
                case ValueUnit::MEGABYTE:
                case ValueUnit::RPM:
                case ValueUnit::MILIVOLT:
                    axis->setMinAboveZero(value, 100);
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

    struct PlotsGeneralConfiguration {
        bool graphOffset, showLegend, commonPlotsBackground;
        QColor plotsBackground;
    };

public:
    QList<PlotDefinitionSchema> schemas;
    PlotsGeneralConfiguration generalConfig;

    PlotManager() { }

    void setRightGap() {
        rightGap = (generalConfig.graphOffset) ? 10 : 0;
    }

    void setTimeRange(int range) {
        timeRange = range;
    }

    void setInitialYRange(YAxis *axis, const int &intialValue) {
        switch (axis->unit) {
            case ValueUnit::PERCENT:
                axis->setRangeAboveZero(intialValue, 10);
                return;

            case ValueUnit::CELSIUS:
                axis->setRangeAboveZero(intialValue, 5);
                return;

            case ValueUnit::MEGAHERTZ:
            case ValueUnit::MILIVOLT:
            case ValueUnit::RPM:
                axis->setRangeAboveZero(intialValue, 100);
                return;

            case ValueUnit::MEGABYTE:
                axis->setRangeAboveZero(intialValue, 100);
                return;
            default:
                return;
        }
    }

    static PlotInitialValues figureOutInitialScale(const PlotDefinitionSchema &pds, GPUDataContainer *gpuData) {
        PlotInitialValues piv;
        if (pds.left.enabled)
            piv.left = gpuData->value(pds.left.dataList.keys().at(0)).value;

        if (pds.right.enabled)
            piv.right = gpuData->value(pds.right.dataList.keys().at(0)).value;

        return piv;
    }

    int findSchemaIdByName(const QString &name) {
        for (int i = 0; i < schemas.count(); ++i) {
            if (schemas.at(i).name == name)
                return i;
        }

        return -1;
    }

    void addSchema(const PlotDefinitionSchema &pds) {
        schemas.append(pds);
    }

    void removeSchema(const int index) {
        removePlot(schemas[index]);
        schemas.removeAt(index);
    }

    void removePlot(PlotDefinitionSchema &pds) {
        delete pds.plot;
        pds.plot = nullptr;
    }

    void createAxis(RPPlot *plot, const PlotAxisSchema &pas, Qt::Alignment align) {
        plot->addAxis(align, pas.unit, pas.penGrid, pas.ticks);

        for (const ValueID &id : pas.dataList.keys()) {
            int seriesId = addSeries(plot, id);
            if (seriesId != -1)
                setLineColor(plot, seriesId, pas.dataList.value(id));
        }
    }

    void createPlotFromSchema(PlotDefinitionSchema &pds, const PlotInitialValues &intialValues) {
        if (pds.plot != nullptr)
            delete pds.plot;

        pds.plot = new RPPlot();

        setPlotBackground(pds.plot, pds.background);

        if (pds.left.enabled) {
            createAxis(pds.plot , pds.left, Qt::AlignLeft);
            setInitialYRange(pds.plot ->axisLeft, intialValues.left);
        }

        if (pds.right.enabled) {
            createAxis(pds.plot , pds.right, Qt::AlignRight);
            setInitialYRange(pds.plot ->axisRight, intialValues.right);
        }
    }

    void createPlotsFromSchemas(const GPUDataContainer &referenceData) {
        if (schemas.count() == 0)
            return;

        for (int i = 0; i < schemas.count(); ++i) {
            if (!schemas.at(i).enabled)
                continue;

            PlotInitialValues piv;
            if (schemas.at(i).left.enabled)
                piv.left = referenceData.value(schemas.at(i).left.dataList.keys().at(0)).value;

            if (schemas.at(i).right.enabled)
                piv.right = referenceData.value(schemas.at(i).right.dataList.keys().at(0)).value;

            createPlotFromSchema(schemas[i], piv);
        }
    }

    QColor invertColor(const QColor &c) {
        return QColor::fromRgb(255 - c.red(), 255 - c.green(),255 - c.blue());
    }

    void setPlotBackground(RPPlot *plot, const QColor &color) {
        plot->timeAxis.setGridLineColor(invertColor(color));
        plot->plotArea.legend()->setLabelColor(invertColor(color));
        plot->plotArea.setBackgroundBrush(QBrush(color));
        plot->setBackgroundBrush(QBrush(color));
    }

    void setLineColor(RPPlot *plot, const int index, const QColor &color) {
        plot->series.at(index)->setColor(color);
    }

    int addSeries(RPPlot *plot, ValueID id) {
        ValueUnit tmpUnit = globalStuff::getUnitFomValueId(id);
        DataSeries *ds = new DataSeries(id, plot);

        plot->plotArea.addSeries(ds);

        ds->attachAxis(&plot->timeAxis);
        ds->id = id;

        if (plot->axisLeft != nullptr && plot->axisLeft->unit == tmpUnit)
            ds->attachAxis(plot->axisLeft);
        else if (plot->axisRight != nullptr && plot->axisRight->unit == tmpUnit)
            ds->attachAxis(plot->axisRight);
        else {
            delete ds;
            return -1;
        }

        ds->setUseOpenGL(true);
        plot->series.append(ds);

        return plot->series.count() - 1;
    }

    void cleanupSeries() {
        for (int i = 0; i < schemas.count(); ++i) {
            if (schemas.at(i).plot == nullptr)
                continue;

            for (int j = 0; j < schemas.at(i).plot->series.count(); ++j) {
                if (schemas.at(i).plot->series.at(j)->count() > maxRange)
                    schemas.at(i).plot->series.at(j)->remove(0);
            }
        }
    }

    void updateSeries(int timestamp, const GPUDataContainer &data) {
        for (int i = 0; i < schemas.count(); ++i) {
            if (schemas.at(i).plot == nullptr)
                continue;

            schemas.at(i).plot->timeAxis.setRange(timestamp - timeRange, timestamp + rightGap);
            schemas.at(i).plot->updatePlot(timestamp, data);
        }

        // cleaunp every 60 refreshes
        if (timestamp % 60 == 0)
            cleanupSeries();
    }
};


#endif // RPPLOT_H
