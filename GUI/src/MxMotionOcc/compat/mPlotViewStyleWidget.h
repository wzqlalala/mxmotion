#pragma once
#include <QPointer>
#include <QWidget>

class QComboBox;
class QTableWidget;
namespace PlotView {
class mPlotView;
// The surrounding splitter/visibility behavior remains the uploaded
// mCurveWindow. This adapter supplies the absent SDK's editable style service.
class mPlotViewStyleWidget : public QWidget {
    Q_OBJECT
public:
    explicit mPlotViewStyleWidget(mPlotView*,QWidget* parent=nullptr);
    void setTableGridVisible(bool);
    void setTableAlternatingRowColors(bool);
    void refreshData();
    void refreshAxisRangeData();
private:
    void populate();
    QPointer<mPlotView> plot_;
    QComboBox* section_=nullptr;
    QComboBox* item_=nullptr;
    QTableWidget* table_=nullptr;
    bool refreshing_=false;
};
}
