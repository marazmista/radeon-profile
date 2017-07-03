
// copyright marazmista @ 11.06.2017

#ifndef TOPBARCOMPONENTS_H
#define TOPBARCOMPONENTS_H

#include "globalStuff.h"
#include "pieprogressbar.h"
#include <QWidget>

enum class TopBarItemType {
    LABLEL,
    LARGE_LABEL,
    PIE
};

class TopBarItem {
public:
    QWidget *itemWidget;
    TopBarItemType itemType;
    ValueID dataSource;

    virtual void updateItemValue(const GPUDataContainer &data) = 0;
    virtual void setForegroundColor(const QColor &c) = 0;
};

class LabelItem : public QLabel, public TopBarItem {
public:
    LabelItem(ValueID id, TopBarItemType type = TopBarItemType::LABLEL, QWidget *parent = 0) : QLabel(parent) {
        itemType = type;
        itemWidget = this;

        QFont f;
        f.setFamily("Monospace");
        f.setPointSize((type == TopBarItemType::LABLEL ? 10 : 20));
        itemWidget->setFont(f);

        dataSource = id;
        itemWidget->setToolTip(globalStuff::getNameOfValueID(id));
    }

    void updateItemValue(const GPUDataContainer &data) {
        this->setText(data.value(dataSource).strValue);
    }

    void setForegroundColor(const QColor &c) {
        QPalette p = this->palette();
        p.setColor(this->foregroundRole(), c);
        this->QLabel::setPalette(p);
    }
};

class PieItem : public PieProgressBar, public TopBarItem {
public:
    PieItem(const int max, ValueID id, QColor fillColor, QWidget *parent = 0) : PieProgressBar(max, id, fillColor, parent) {
        itemType = TopBarItemType::PIE;
        itemWidget = this;

        setMinimumSize(60,60);
        this->maxValue = max;
        this->dataSource = id;
        itemWidget->setToolTip(globalStuff::getNameOfValueID(id));
    }

    void updateItemValue(const GPUDataContainer &data) {
        this->updateValue(data);
    }

    void setForegroundColor(const QColor &c) {
        this->PieProgressBar::setFillColor(c);
    }
};
#endif // TOPBARCOMPONENTS_H
