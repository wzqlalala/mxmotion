#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include "mPanelBase.h"

#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QPushButton>
#include <QTextStream>
#include <QVector>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QMap>
#include <QSet>

#include "mMotionCurveRequest.h"
#include "mMotionResultQueryService.h"

namespace MechMainWindow
{
    class mPickWidgetController;

    /**
     * @brief 载荷导出面板（三模式）
     * @details
     * 模式一「手动提取」：选单个载荷来源对象，按最大帧/指定帧/全部帧导出
     * CSV 或 Nastran BDF（原功能保留）。
     *
     * 模式二「特征工况导出」：选一个运动体，自动列出其上的全部运动副，按
     * "每副每条曲线 m 个极值 → 跨副去重 → 得 f 个时刻 → 按时刻分组工况"
     * 的规则生成载荷工况 JSON（docs/loadcase_format_spec.json），供隐式
     * 结构软件读取并批量生成静力工况。导出成功后上报平台 results。
     *
     * 模式三「连杆同步工况导出」：特征时刻筛选与模式二一致，但每个工况
     * 导出分析连杆全部关联运动副在该时刻的同步载荷（机械臂单连杆静力分析），
     * 按分析体在副 action/base 端处理符号（base 端整体取反），并统一用
     * 分析体 R(t)^T 转换到其初始姿态系。JSON 与模式二同构。
     */
    class MMAINWINDOW_EXPORT mMotionLoadExportPanel : public mPanelBase
    {
        Q_OBJECT

    public:
        mMotionLoadExportPanel(QWidget* parent = Q_NULLPTR);
        ~mMotionLoadExportPanel();

        void editLoadExportPanel(int id = 0);
        void uneditLoadExportPanel();

    private:
        /// 参与工况导出的运动副选择项
        struct JointSel {
            int jointId = -1;
            QString jointName;
            QString jointType;
            int actionBodyId = -1;
            int baseBodyId = -1;
        };

        // ---- 特征工况/连杆同步导出（模式二/三共用页上下文） ----
        /// 一页的控件集合：两模式输入完全同构，建页与逻辑共用，仅导出语义不同
        struct CasePageCtx {
            FoldSection* sourceSection = nullptr;  ///< “载荷来源”折叠区（复选框增删后刷新高度）
            QComboBox* comboBody = nullptr;        ///< 运动体
            QWidget* jointCheckArea = nullptr;     ///< 运动副复选框容器
            QVBoxLayout* jointCheckLayout = nullptr;
            QMap<int, QCheckBox*> jointChecks;     ///< jointId -> 复选框
            FoldSection* faceSection = nullptr;    ///< “运动副几何面”折叠区（免导入生成用）
            QMap<int, mPickWidgetController*> jointFacePicks;  ///< jointId -> 几何面拾取器
            QSpinBox* spinPeaks = nullptr;         ///< 每条曲线极值数 m
            QDoubleSpinBox* spinMergeTol = nullptr;///< 时刻合并容差(秒)
            QCheckBox* chkAutoGenerate = nullptr;  ///< 直接生成隐式静力工况
            QCheckBox* chkExportJson = nullptr;    ///< 生成时是否同时导出工况 JSON（默认勾选）
            QPushButton* btnExport = nullptr;
        };

        // ---- 手动提取 ----
        void refreshSourceObjects();
        void refreshResultState();
        MechMotionData::CurveSourceType getCurrentSourceType() const;
        int getMaxLoadFrameIndex(const mMotionResultQueryService::LoadSeries& series) const;
        void writeCsvContent(QTextStream& out, const mMotionResultQueryService::LoadSeries& series,
            const QVector<int>& frameIndices, double effectiveScale) const;
        void writeBdfContent(QTextStream& out, const mMotionResultQueryService::LoadSeries& series,
            const QVector<int>& frameIndices, double effectiveScale) const;

        // ---- 特征工况/连杆同步导出（模式二/三共用页上下文） ----
        void refreshBodies();                       ///< 刷新两页运动体下拉
        void refreshJointChecks(CasePageCtx& ctx);  ///< 按选中运动体刷新运动副复选框
        bool collectCaseJoints(const CasePageCtx& ctx, QVector<JointSel>& out, QString& error);
        bool exportLoadCaseJson(const QString& fileName, bool synchronized,
            bool autoGenerate, bool writeJson, QString& error,
            int* genCases, int* genLoads, int* genRbe3);
        void reportToPlatform(const QString& filePath);  ///< 上报平台 results 通道

        int m_scenarioId = -1;

        // ============ 顶部：导出模式切换 ============
        QComboBox* m_comboExportMode = nullptr;   ///< 0=手动提取 1=特征工况导出 2=连杆同步工况导出
        QStackedWidget* m_modeStack = nullptr;

        // ============ 模式一：手动提取（原控件） ============
        QLineEdit* m_editName = nullptr;
        QComboBox* m_comboSourceType = nullptr;
        QComboBox* m_comboSourceId = nullptr;
        QButtonGroup* m_btnGroupMarker = nullptr;
        QRadioButton* m_radioIMarker = nullptr;
        QRadioButton* m_radioJMarker = nullptr;
        QLineEdit* m_editNodeId = nullptr;
        QPushButton* m_btnPickNode = nullptr;
        QButtonGroup* m_btnGroupMode = nullptr;
        QRadioButton* m_radioMaxFrame = nullptr;
        QRadioButton* m_radioSingleFrame = nullptr;
        QRadioButton* m_radioAllFrames = nullptr;
        QSpinBox* m_spinFrameIndex = nullptr;
        QComboBox* m_comboFormat = nullptr;
        QCheckBox* m_chkExportForce = nullptr;
        QCheckBox* m_chkExportTorque = nullptr;
        QCheckBox* m_chkUseGlobal = nullptr;
        QCheckBox* m_chkFx = nullptr;
        QCheckBox* m_chkFy = nullptr;
        QCheckBox* m_chkFz = nullptr;
        QCheckBox* m_chkMx = nullptr;
        QCheckBox* m_chkMy = nullptr;
        QCheckBox* m_chkMz = nullptr;
        QDoubleSpinBox* m_spinScale = nullptr;
        QPushButton* m_btnExport = nullptr;

        // ============ 模式二/三：特征工况 / 连杆同步 ============
        CasePageCtx m_casePage;   ///< 模式二：特征工况导出
        CasePageCtx m_syncPage;   ///< 模式三：连杆同步工况导出

        /// 共用建页：按 ctx 建一页并连接信号（体切换→刷新副复选框，导出按钮→onExportCaseClicked）
        QWidget* buildCasePage(CasePageCtx& ctx, bool synchronized);
        /// 当前导出模式对应的页上下文（模式三返回同步页，否则返回特征工况页）
        CasePageCtx* currentCasePage();

        // ---- 视口联动高亮 ----
        /// 按当前页选择联动视口：体部件拾取集高亮 + 勾选副符号显示（未勾选隐藏）
        void highlightSelection();
        /// 清体部件高亮并隐藏本面板显示过的副符号（退出面板时调用）
        void clearViewportHighlight();
        QSet<QString> m_shownJointSymbols;  ///< 本面板显示中的副符号名（motionJoint_<id>）

    private slots:
        void onExportModeChanged(int index);
        void onSourceTypeChanged(int index);
        void onModeChanged(int id);
        void onExportClicked();
        void onExportCaseClicked();
    };
}
