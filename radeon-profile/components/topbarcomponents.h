#ifndef TOPBARCOMPONENTS_H
#define TOPBARCOMPONENTS_H

#include "globalStuff.h"
#include "pieprogressbar.h"
#include <QWidget>


//enum class TopBarItemType {
//    DISABLED,
//    LABLEL,
//    PIE
//};

class TopBarItem {
public:
    QWidget *itemWidget;
//    TopBarItemType itemType;
    ValueID dataSource;

    virtual void updateItemValue(const GPUDataContainer &data) = 0;
    virtual void setForegroundColor(const QColor &c) = 0;
};

class LabelItem : public QLabel, public TopBarItem {
public:
    LabelItem(ValueID id, QWidget *parent = 0) : QLabel(parent) {
//        itemType = TopBarItemType::LABLEL;
        itemWidget = this;

        QFont f;
        f.setFamily("Monospace");
        f.setPointSize(10);
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
//        itemType = TopBarItemType::PIE;
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
