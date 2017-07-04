
// copyright marazmista @ 11.06.2017

#ifndef TOPBARCOMPONENTS_H
#define TOPBARCOMPONENTS_H

#include "globalStuff.h"
#include "pieprogressbar.h"
#include <QWidget>

enum class TopbarItemType {
    LABEL,
    LARGE_LABEL,
    PIE
};

class TopbarItem {
public:
    QWidget *itemWidget;
    TopbarItemType itemType;
    ValueID itemDataId;

    virtual void updateItemValue(const GPUDataContainer &data) = 0;
    virtual void setForegroundColor(const QColor &c) = 0;
    virtual QColor getForegroundColor() = 0;
};

class LabelItem : public QLabel, public TopbarItem {
public:
    LabelItem(ValueID id, TopbarItemType type, QWidget *parent = 0) : QLabel(parent) {
        itemType = type;
        itemWidget = this;

        QFont f;
        f.setFamily("Monospace");
        f.setPointSize((type == TopbarItemType::LABEL ? 10 : 20));
        itemWidget->setFont(f);

        itemDataId = id;
        itemWidget->setToolTip(globalStuff::getNameOfValueID(id));
    }

    void updateItemValue(const GPUDataContainer &data) {
        this->setText(data.value(itemDataId).strValue);
    }

    void setForegroundColor(const QColor &c) {
        QPalette p = this->palette();
        p.setColor(this->foregroundRole(), c);
        this->QLabel::setPalette(p);
    }

    QColor getForegroundColor() {
        return this->palette().color(QPalette::Foreground);
    }
};

class PieItem : public PieProgressBar, public TopbarItem {
public:
    PieItem(const int max, ValueID id, QColor fillColor, QWidget *parent = 0) : PieProgressBar(max, id, fillColor, parent) {
        itemType = TopbarItemType::PIE;
        itemWidget = this;

        setMinimumSize(60,60);
        this->maxValue = max;
        this->itemDataId = id;
        itemWidget->setToolTip(globalStuff::getNameOfValueID(id));
    }

    void updateItemValue(const GPUDataContainer &data) {
        this->updateValue(data);
    }

    void setForegroundColor(const QColor &c) {
        this->PieProgressBar::setFillColor(c);
    }

    QColor getForegroundColor() {
        return this->PieProgressBar::getFillColor();
    }
};

struct TopbarItemDefinitionSchema {
    TopbarItemType type;
    QColor foregroundColor;
    ValueID dataId;
    int row = -1, column = -1;

    TopbarItemDefinitionSchema(ValueID id, TopbarItemType t, QColor color) {
        dataId = id;
        type = t;
        foregroundColor = color;
    }

    void setPosition(int c, int r = 0) {
        row = r;
        column = c;
    }
};

class TopbarManager {
    QList<TopbarItemDefinitionSchema> schemas;
    QList<TopbarItem*> items;
public:

    void createTopbar(QGridLayout *grid) {
        for (const TopbarItemDefinitionSchema &tis : schemas) {
            if (tis.row == -1 || tis.column == -1)
                continue;

            addToGrid(grid, tis);
        }
    }

    void removeSchema(int i) {
        schemas.removeAt(i);
        items.removeAt(i);
    }

    void addSchema(const TopbarItemDefinitionSchema &tis) {
        schemas.append(tis);
    }

    void setItemPosition(int itemIndex, int c, int r) {
        schemas[itemIndex].setPosition(c, r);
    }

    void addToGrid(QGridLayout *grid, const TopbarItemDefinitionSchema &tis) {
        TopbarItem *item;
        int rowStretch = 1;

        switch (tis.type) {
            case TopbarItemType::LARGE_LABEL:
                rowStretch = 2;
            case TopbarItemType::LABEL:
                item = new LabelItem(tis.dataId, tis.type, grid->widget());
                break;

            case TopbarItemType::PIE:
                rowStretch = 2;
                item = new PieItem(100, tis.dataId, tis.foregroundColor, grid->widget());
                break;
        }
        grid->addWidget(item->itemWidget, tis.row, tis.column, rowStretch, 1, Qt::AlignLeft);
        items.append(item);
    }

    void updateItems(const GPUDataContainer &data) {
        for (TopbarItem *ti : items)
            ti->updateItemValue(data);
    }

};


#endif // TOPBARCOMPONENTS_H
