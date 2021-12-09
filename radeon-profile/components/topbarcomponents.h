
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
    virtual void setSecondaryColor(const QColor &c) { Q_UNUSED(c); }
    virtual void setSecondaryValueId(const ValueID vId) {
        secondaryValueId = vId;
        secondaryValueIdEnabled = true;
    }

    virtual ~TopbarItem() { }

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
public:
    LabelPairItem(const ValueID vId, const QColor &c, QWidget *parent = nullptr) : QWidget(parent) {
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
        layout->setContentsMargins(15,0,0,0);
        layout->addWidget(&labelTop);
        layout->addWidget(&labelBottom);

        this->setLayout(layout);
        this->setMinimumHeight(60);

        itemWidget = this;
    }

    void updateItemValue(const GPUDataContainer &data) override {
        labelTop.setText(data.value(primaryValueId).strValue);

        if (secondaryValueIdEnabled)
            labelBottom.setText(data.value(secondaryValueId).strValue);
    }

    void setPrimaryColor(const QColor &c) override {
        QPalette p = this->palette();
        p.setColor(this->foregroundRole(), c);
        labelTop.setPalette(p);
    }

    void setSecondaryColor(const QColor &c) override {
        QPalette p = this->palette();
        p.setColor(this->foregroundRole(), c);
        labelBottom.setPalette(p);
    }

    void setSecondaryValueId(const ValueID vId) override {
        TopbarItem::setSecondaryValueId(vId);
        labelBottom.setToolTip(globalStuff::getNameOfValueID(secondaryValueId));
    }

private:
    QLabel labelTop, labelBottom;
};

class LargeLabelItem : public QLabel, public TopbarItem {
public:
    LargeLabelItem(const ValueID vId, const QColor &c, QWidget *parent = nullptr) : QLabel(parent) {
        itemType = TopbarItemType::LARGE_LABEL;

        QFont f;
        f.setFamily("Monospace");
        f.setPointSize(24);
        this->setFont(f);
        setContentsMargins(15,0,0,0);

        setPrimaryColor(c);

        primaryValueId = vId;
        this->setToolTip(globalStuff::getNameOfValueID(vId));

        itemWidget = this;
    }

    void updateItemValue(const GPUDataContainer &data) override {
        this->setText(data.value(primaryValueId).strValue);
    }

    void setPrimaryColor(const QColor &c) override {
        QPalette p = this->palette();
        p.setColor(this->foregroundRole(), c);
        this->QLabel::setPalette(p);
    }
};

class PieItem : public PieProgressBar, public TopbarItem {
public:
    PieItem(const int max, ValueID vId, QColor fillColor, QWidget *parent = nullptr) : PieProgressBar(max, vId, fillColor, parent) {
        itemType = TopbarItemType::PIE;

        setMinimumHeight(60);
        this->maxValue = max;
        this->primaryValueId = vId;
        this->setToolTip(globalStuff::getNameOfValueID(vId));

        itemWidget = this;
    }

    void updateItemValue(const GPUDataContainer &data) override {
        this->updateValue(data);
    }

    void setPrimaryColor(const QColor &c) override {
        this->PieProgressBar::setFillColor(c);
    }

    void setSecondaryValueId(const ValueID vId) override {
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
    int pieMaxValue = 100;

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
        }

        name.append(globalStuff::getNameOfValueID(vId));
    }

    void setPieMaxValue(int max) {
        pieMaxValue = max;
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

    void setDefaultForeground(const QColor &c) {
        defaultForeground = c;
    }

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
                item = new PieItem(tis.pieMaxValue, tis.primaryValueId, tis.primaryColor, layout->widget());
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

    void createDefaultTopbarSchema(const QList<ValueID> &availableData) {
        if (availableData.contains(ValueID::CLK_CORE)) {
            TopbarItemDefinitionSchema tis(ValueID::CLK_CORE, TopbarItemType::LABEL_PAIR, defaultForeground);
            tis.setSecondaryValueId(ValueID::CLK_MEM);
            tis.setSecondaryColor(defaultForeground);
            addSchema(tis);
        }

        const ValueID t_edge = ValueID(ValueID::TEMPERATURE_CURRENT, ValueID::T_EDGE);
        if (availableData.contains(t_edge)) {
            TopbarItemDefinitionSchema tis(t_edge, TopbarItemType::LARGE_LABEL, defaultForeground);
            addSchema(tis);
        }

        const ValueID t_junction = ValueID(ValueID::TEMPERATURE_CURRENT, ValueID::T_JUNCTION);
        const ValueID t_mem = ValueID(ValueID::TEMPERATURE_CURRENT, ValueID::T_MEM);
        if (availableData.contains(t_junction)) {
            TopbarItemDefinitionSchema tis(t_junction, TopbarItemType::LABEL_PAIR, defaultForeground);
            if (availableData.contains(t_mem)) {
                tis.setSecondaryValueId(t_mem);
                tis.setSecondaryColor(defaultForeground);
            }
            addSchema(tis);
        } else if (availableData.contains(t_mem)) {
            TopbarItemDefinitionSchema tis(t_mem, TopbarItemType::LABEL_PAIR, defaultForeground);
            addSchema(tis);
        }

        if (availableData.contains(ValueID::FAN_SPEED_PERCENT)) {
            TopbarItemDefinitionSchema tis(ValueID::FAN_SPEED_PERCENT, TopbarItemType::PIE, Qt::blue);
            addSchema(tis);
        }

        if (availableData.contains(ValueID::GPU_USAGE_PERCENT)) {
            TopbarItemDefinitionSchema tis(ValueID::GPU_USAGE_PERCENT, TopbarItemType::PIE, Qt::red);
            addSchema(tis);
        }

        if (availableData.contains(ValueID::GPU_VRAM_USAGE_PERCENT)) {
            TopbarItemDefinitionSchema tis(ValueID::GPU_VRAM_USAGE_PERCENT, TopbarItemType::PIE, Qt::yellow);
            addSchema(tis);
        }
    }

private:
    QColor defaultForeground;
};


#endif // TOPBARCOMPONENTS_H
