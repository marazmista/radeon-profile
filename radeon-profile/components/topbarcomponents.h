
// copyright marazmista @ 11.06.2017

#ifndef TOPBARCOMPONENTS_H
#define TOPBARCOMPONENTS_H

#include "globalStuff.h"
#include "pieprogressbar.h"
#include <QWidget>

enum TopbarItemType {
    LABEL_PAIR,
    LARGE_LABEL,
    PIE
};

class TopbarItem {
public:
    virtual void updateItemValue(const GPUDataContainer &data) = 0;
    virtual void setPrimaryColor(const QColor &c) = 0;
    virtual void setSecondaryColor(const QColor &c) { }
    virtual void setSecondaryValueId(const ValueID vId) {
        secondaryValueId = vId;
        secondaryValueIdEnabled = true;
    }

    QWidget* getItemWidget() {
        return itemWidget;
    }

protected:
    QWidget *itemWidget;
    TopbarItemType itemType;
    ValueID primaryValueId, secondaryValueId;
    bool secondaryValueIdEnabled = false;
};

class LabelPairItem : public QWidget, public TopbarItem {
    QLabel labelTop, labelBottom;

public:
    LabelPairItem(const ValueID vId, const QColor &c, QWidget *parent = 0) : QWidget(parent) {
        itemType = TopbarItemType::LABEL_PAIR;
        primaryValueId = vId;

        QFont f;
        f.setFamily("Monospace");
        f.setPointSize(11);
        labelTop.setFont(f);
        labelBottom.setFont(f);

        setPrimaryColor(c);

        labelTop.setToolTip(globalStuff::getNameOfValueID(primaryValueId));

        QVBoxLayout *layout = new QVBoxLayout();
        layout->setSpacing(10);
        layout->addWidget(&labelTop);
        layout->addWidget(&labelBottom);

        this->setLayout(layout);
        this->setMinimumHeight(60);
        itemWidget = this;
    }

    void init() {

    }

    void updateItemValue(const GPUDataContainer &data) {
        labelTop.setText(data.value(primaryValueId).strValue);

        if (secondaryValueIdEnabled)
            labelBottom.setText(data.value(secondaryValueId).strValue);
    }

    void setPrimaryColor(const QColor &c) {
        QPalette p = this->palette();
        p.setColor(this->foregroundRole(), c);
        labelTop.setPalette(p);
    }

    void setSecondaryColor(const QColor &c) {
        QPalette p = this->palette();
        p.setColor(this->foregroundRole(), c);
        labelBottom.setPalette(p);
    }

    void setSecondaryValueId(const ValueID vId) {
        TopbarItem::setSecondaryValueId(vId);
        labelBottom.setToolTip(globalStuff::getNameOfValueID(secondaryValueId));
    }
};

class LargeLabelItem : public QLabel, public TopbarItem {
public:
    LargeLabelItem(const ValueID vId, const QColor &c, QWidget *parent = 0) : QLabel(parent) {
        itemType = TopbarItemType::LARGE_LABEL;
        itemWidget = this;

        QFont f;
        f.setFamily("Monospace");
        f.setPointSize(24);
        itemWidget->setFont(f);

        setPrimaryColor(c);

        primaryValueId = vId;
        itemWidget->setToolTip(globalStuff::getNameOfValueID(vId));
    }

    void updateItemValue(const GPUDataContainer &data) {
        this->setText(data.value(primaryValueId).strValue);
    }

    void setPrimaryColor(const QColor &c) {
        QPalette p = this->palette();
        p.setColor(this->foregroundRole(), c);
        this->QLabel::setPalette(p);
    }
};

class PieItem : public PieProgressBar, public TopbarItem {
public:
    PieItem(const int max, ValueID vId, QColor fillColor, QWidget *parent = 0) : PieProgressBar(max, vId, fillColor, parent) {
        itemType = TopbarItemType::PIE;
        itemWidget = this;

        setMinimumHeight(60);
        this->maxValue = max;
        this->primaryValueId = vId;
        itemWidget->setToolTip(globalStuff::getNameOfValueID(vId));
    }

    void updateItemValue(const GPUDataContainer &data) {
        this->updateValue(data);
    }

    void setPrimaryColor(const QColor &c) {
        this->PieProgressBar::setFillColor(c);
    }

    void setSecondaryValueId(const ValueID vId) {
        TopbarItem::setSecondaryValueId(vId);
        PieProgressBar::setSecondaryDataId(vId);
    }
};

struct TopbarItemDefinitionSchema {
    TopbarItemType type;
    QColor primaryColor, secondaryColor;
    ValueID primaryValueId, secondaryValueId;
    QString name;
    bool secondaryValueIdEnabled = false;

    TopbarItemDefinitionSchema() { }

    TopbarItemDefinitionSchema(ValueID vId, TopbarItemType t, QColor color) {
        primaryValueId = vId;
        type = t;
        primaryColor = color;

        switch (type) {
            case TopbarItemType::LABEL_PAIR:
                name.append("[Pair]\n");
                break;
            case TopbarItemType::LARGE_LABEL:
                name.append("[Large label]\n");
                break;
            case TopbarItemType::PIE:
                name.append("[Pie]\n");
                break;
            default:
                break;
        }
        name.append(globalStuff::getNameOfValueID(vId));
    }

    void setSecondaryValueId(ValueID vId) {
        secondaryValueId = vId;
        secondaryValueIdEnabled = true;
        name.append("\n" + globalStuff::getNameOfValueID(vId));
    }

    void setSecondaryColor(const QColor &c) {
        secondaryColor = c;
    }
};

class TopbarManager {
public:
    QList<TopbarItemDefinitionSchema> schemas;
    QList<TopbarItem*> items;

    void createTopbar(QHBoxLayout *layout) {
        items.clear();

        QLayoutItem *item;
        while ((item = layout->takeAt(0)) != 0) {
            delete item->widget();
            delete item;
        }

        for (const TopbarItemDefinitionSchema &tis : schemas)
            addToLayout(layout, tis);

        layout->setStretch(layout->count()-1,1);
    }

    void removeSchema(const int id) {
        schemas.removeAt(id);
    }

    void addSchema(const TopbarItemDefinitionSchema &tis) {
        schemas.append(tis);
    }

    void addToLayout(QHBoxLayout *layout, const TopbarItemDefinitionSchema &tis) {
        TopbarItem *item;

        switch (tis.type) {
            case TopbarItemType::LARGE_LABEL:
                item = new LargeLabelItem(tis.primaryValueId, tis.primaryColor, layout->widget());
                break;
            case TopbarItemType::LABEL_PAIR:
                item = new LabelPairItem(tis.primaryValueId, tis.primaryColor, layout->widget());
                break;
            case TopbarItemType::PIE:
                item = new PieItem(100, tis.primaryValueId, tis.primaryColor, layout->widget());
                break;
        }

        if (tis.secondaryValueIdEnabled) {
            item->setSecondaryValueId(tis.secondaryValueId);
            item->setSecondaryColor(tis.secondaryColor);
        }

        items.append(item);
        layout->addWidget(item->getItemWidget(), 0, Qt::AlignLeft);
    }

    void updateItems(const GPUDataContainer &data) {
        for (TopbarItem *ti : items)
            ti->updateItemValue(data);
    }

};


#endif // TOPBARCOMPONENTS_H
