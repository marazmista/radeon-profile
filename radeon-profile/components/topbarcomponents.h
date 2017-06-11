#ifndef TOPBARCOMPONENTS_H
#define TOPBARCOMPONENTS_H

//#include "globalStuff.h"
//#include "pieprogressbar.h"
//#include <QWidget>


//enum class TopBarItemType {
//    DISABLED,
//    LABLEL,
//    PIE
//};

//struct TopBarItem {
//    TopBarItem item;
//    QColor fill, backgrund;
//    ValueID dataSource;
//    TopBarItemType type;
//};

//class TopBarItem : public QWidget {
//public:

//    virtual void updateValue(const GPUDataContainer &data) = 0;
    //void setToolTip(const QString &tip);
//    void setForegroundColor(const QColor &c);
//    void setBackgroundColor (const QColor &c);
//    virtual void setSuffix(const QString &s) {
//        suffix = s;
//    }

//    QColor getForegroundColor() = 0;
//    QColor getBackgroundColor() = 0;

//    TopBarItemType itemType;
//    ValueID dataSource;
//    QString suffix;
//};

//class LabelItem : public QLabel, public TopBarItem {
//public:
//    LabelItem(const QString &s, ValueID id, QWidget *parent = 0) : QLabel(parent) {
//        itemType = TopBarItemType::LABLEL;
//        suffix = s;
//        dataSource = id;
//    }

//    void updateValue(const GPUDataContainer &data) {
//        this->setText(data.value(dataSource).strValue);
//    }

//    void setForegroundColor(const QColor &c) {
//        QPalette p = this->palette();
//        palette.setColor(ui->pLabel->foregroundRole(), c);
//         this->setPalette(palette);
//    }
//};

//class PieItem : public PieProgressBar, public TopBarItem {
//public:
//    PieItem(QWidget *parent = 0) : PieProgressBar(parent) {
//        itemType = TopBarItemType::PIE;
//    }

//    PieItem(const int max, const QString &s, ValueID id, QWidget *parent = 0) : PieProgressBar(max, s, parent) {
//        itemType = TopBarItemType::PIE;
//        this->maxValue = max;
//        this->suffix = s;
//        this->dataSource = id;
//    }

//    void updateValue(const GPUDataContainer &data) {
//        this->setValue(data.value(dataSource).value);
//    }

//    void setForegroundColor(const QColor &c) {

//    }

//    void setBackgroundColor(const QColor &c) {

//    }
//};
#endif // TOPBARCOMPONENTS_H
