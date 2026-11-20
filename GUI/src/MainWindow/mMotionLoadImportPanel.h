#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include "mPanelBase.h"

#include <QWidget>
#include <QVector>
#include <QMap>
#include <QSet>
#include <QVector3D>

class QComboBox;
class QLineEdit;
class QPushButton;

namespace MechMainWindow { class FoldSection; }

namespace MechMainWindow
{
    class mPickWidgetController;

    /**
     * @brief 多体载荷导入面板
     * @details
     * 读取多体软件（MxMotion）经平台 couplingFiles 下发的「特征工况 JSON」
     * （MxMotionLoadCases），在隐式结构侧完成：
     *   解析 → 选目标体 → 列出该体运动副 → 每副拾取几何面 → 建 RBE3（分布式
     *   耦合）→ 批量生成静力工况并施加力/力矩/压力。
     *
     * 坐标系约定：JSON 中力/力矩已是分析体初始姿态系（bodyInitial），本软件
     * 在固定几何模型上按全局系直接施加，不再旋转。
     */
    class MMAINWINDOW_EXPORT mMotionLoadImportPanel : public mPanelBase
    {
        Q_OBJECT

    public:
        explicit mMotionLoadImportPanel(QWidget* parent = nullptr);
        ~mMotionLoadImportPanel();

        /// 由平台 pending coupling 路径或菜单按钮打开：path 为空则让用户手动选
        void openWithFile(const QString& path = QString());

        /// 是否处于拾取编辑态（有激活的面拾取控件或已拾取数据）
        bool isEditing() const;

        /// 统一退出出口（由 mInfoWindow::unEditInfoWindow 链路调用）：
        /// 失活全部几何面拾取控件并清掉高亮，防止面板退出后拾取仍占用视口
        void deactivatePickAndCommit();

    private:
        // ---- JSON 数据结构（与 MxMotionLoadCases 对齐） ----
        struct BodyInfo {
            int id = -1;
            QString name;
            QVector<int> partIds;
            bool isFixed = false;
        };
        struct JointInfo {
            int id = -1;
            QString name;
            QString type;
            int actionBodyId = -1;
            int baseBodyId = -1;
            QVector<int> actionPartIds;
            QVector<int> basePartIds;
            QVector3D actionPoint;
        };
        struct LoadComp {
            double fx=0, fy=0, fz=0, fm=0;
            double tx=0, ty=0, tz=0, tm=0;
        };
        struct CaseLoad {
            int jointId = -1;
            QString jointName;
            LoadComp comp;
        };
        struct LoadCase {
            QString name;
            double timeSeconds = 0.0;
            QVector<CaseLoad> loads;
        };

        // 每个选中运动副的绑定行：复选 + 几何面拾取器（直接作为 FoldSection 行）
        struct JointRow {
            int jointId = -1;
            class QCheckBox* chk = nullptr;
            mPickWidgetController* facePick = nullptr;  // 几何面拾取器
        };

        // ---- 解析与刷新 ----
        bool parseJson(const QString& path, QString& error);
        void refreshBodyCombo();
        void refreshJointRows();

        // ---- 生成（收集绑定后调 mMotionLoadCaseGenerator 共用内核） ----
        bool generateCases(QString& error);

        /// 取某副在 cases 里出现的所有载荷（按 jointId）
        QVector<QPair<QString, LoadComp>> loadsForJoint(int jointId) const;

        const JointInfo* findJoint(int jointId) const;
        const BodyInfo* findBody(int bodyId) const;

    protected:

        // ---- 数据 ----
        QString m_filePath;
        QVector<BodyInfo> m_bodies;
        QVector<JointInfo> m_joints;
        QVector<LoadCase> m_cases;
        int m_analysisBodyId = -1;
        QString m_analysisBodyName;

        // ---- UI ----
        QLineEdit* m_editPath = nullptr;
        QPushButton* m_btnBrowse = nullptr;
        QComboBox* m_comboBody = nullptr;
        MechMainWindow::FoldSection* m_jointSection = nullptr;
        QPushButton* m_btnGenerate = nullptr;

        QVector<JointRow> m_jointRows;

    private slots:
        void onBrowse();
        void onBodyChanged(int index);
        void onGenerate();
    };
}
