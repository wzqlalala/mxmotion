#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include "mPanelBase.h"
#include "mMotionCurveRequest.h"
#include "mCalcLineEdit.h"

#include <QComboBox>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QListWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMetaType>

Q_DECLARE_METATYPE(MechMotionData::CurveVariable)
Q_DECLARE_METATYPE(MechMotionData::CurveComponent)

namespace MechMotionData
{
    class mMotionScenario;
}

namespace MechMainWindow
{
    /**
     * @brief 运动曲线面板
     * @details 求解后配置曲线请求并加入列表，多曲线叠加显示；
     *          支持曲线颜色与显隐管理、时间范围过滤。
     *          动画↔曲线联动（帧标记、点击跳帧）由动画面板负责。
     */
    class MMAINWINDOW_EXPORT mMotionCurvePanel : public mPanelBase
    {
        Q_OBJECT

    public:
        explicit mMotionCurvePanel(QWidget* parent = nullptr);
        ~mMotionCurvePanel();

        void newMotionCurvePanel();
        void editMotionCurvePanel(int scenarioId);
        void uneditMotionCurvePanel();

        /**
         * @brief 把方案配置的曲线显示到曲线窗（全时间范围，无时间窗过滤）
         * @return 是否成功显示了至少一条曲线
         * @details 供动画面板等外部调用：曲线面板负责配置画哪些曲线，
         *          调用方负责在合适时机让曲线显示出来
         */
        static bool displayScenarioCurves(int scenarioId);

    private slots:
        void onSourceTypeChanged(int index);
        void onAddCurve();
        void onClearCurves();
        void onExportCurve();
        void onUIChanged();
        void onTimeRangeChanged();

    private:
        struct VarCompItem {
            MechMotionData::CurveVariable variable;
            MechMotionData::CurveComponent component;
            QString label;
        };

        void setupUI();
        void updateUIFromData();

        void populateSourceTypeCombo();
        void populateObjectCombo(MechMotionData::CurveSourceType type);
        void populateVarCompCombo(MechMotionData::CurveSourceType type);

        int getSelectedObjectId() const;
        VarCompItem getSelectedVarComp() const;
        QString getSelectedObjectName() const;

        MechMotionData::mMotionCurveRequest collectRequestFromUI() const;
        void applyRequestToUI(const MechMotionData::mMotionCurveRequest& req);

        MechMotionData::mMotionScenario* getScenario() const;
        void rebuildCurveList();
        QWidget* createCurveRowWidget(const MechMotionData::mMotionCurveRequest& req, int rowIndex);

        // 可见曲线的成批提取结果（显示与导出共用）
        struct VisibleSeries {
            QVector<QVector<double>> xs;
            QVector<QVector<double>> ys;
            QStringList titles;
            QVector<QColor> colors;
            bool isValid() const { return !xs.isEmpty() && xs.size() == ys.size() && xs.size() == titles.size(); }
        };

        static VisibleSeries collectSeries(const MechMotionData::mMotionScenario* scenario);
        static bool applySeriesToPlot(const VisibleSeries& series);
        VisibleSeries collectVisibleSeries() const;
        void refreshPlotCurves();
        static QColor autoColorFor(int index);
        static QString requestTitle(const MechMotionData::mMotionCurveRequest& req);
        QString uniqueCurveTitle(const QString& base) const;
        bool timeWindow(double& start, double& end) const;

        int m_scenarioId = -1;

        // UI
        QVBoxLayout* m_mainLayout = nullptr;
        QComboBox* m_comboSourceType = nullptr;
        QComboBox* m_comboObject = nullptr;
        QComboBox* m_comboVarComp = nullptr;
        QLineEdit* m_editCurveName = nullptr;
        QPushButton* m_btnAddCurve = nullptr;
        QPushButton* m_btnClearCurves = nullptr;
        QListWidget* m_listCurves = nullptr;
        MxFunctions::mCalcLineEdit* m_editTimeStart = nullptr;
        MxFunctions::mCalcLineEdit* m_editTimeEnd = nullptr;
        QPushButton* m_btnExport = nullptr;
        QSpinBox* m_spinExportInterval = nullptr;
    };

}
