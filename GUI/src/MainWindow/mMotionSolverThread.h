#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include "mxr/data/SelectDataStruct.h"
#include "mBasicEnum.h"
#include "mPhysicalFieldEnum.h"
#include "mMapRelationEnum.h"

#include <memory>
#include <set>
#include <atomic>

//QT
#include <QWidget>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QHash>
#include <QGridLayout>
#include <QLabel>
#include <QToolButton>
#include <QMdiSubWindow>
#include <QComboBox>
#include <QStackedWidget>
#include <QThread>
#include "mPanelBase.h"

#include "motion/physics/SystemSMC.h"
#include "mMotionPidSystem.h"
#include "mMotionFlexBridge.h"

namespace MechPreData
{
    class mViewportModel;
}
namespace MechMotionData
{
    class mMotionDriveBase;
    class mMotionScenario;
}

namespace MechMainWindow
{
    /**
     * @brief 动力学求解器后台线程
     * @details 将耗时的数值积分计算放在后台，防止UI假死
     */
    class MMAINWINDOW_EXPORT mMotionSolverThread : public QThread
    {
        Q_OBJECT

    public:
        /*
        * 默认构造函数
        */
        mMotionSolverThread(int scenarioId, MechPreData::mViewportModel* viewportModel, QObject* parent = nullptr);

        /*
        * 析构函数
        */
        ~mMotionSolverThread();

        void doTransInput();

        void doSolve();

        /*
        * 求解前全量数据完整性校验：一次性收集所有力元/弹簧/接触/驱动的引用问题，
        * 返回问题清单（空列表表示通过）。装配期的逐项抛错保留作为纵深防御。
        */
        QStringList checkMotionDataIntegrity();

        void requestStopSolve() noexcept
        {
            m_cancelRequested.store(true, std::memory_order_release);

            // 同时保留 Qt 的中断状态
            requestInterruption();

            // sys 的停止变量是 atomic，因此可以跨线程调用
            m_sys.RequestStopSolve();
        }

        void doTransOutput();

    protected:
        /**
         * @brief 线程的核心执行函数 (在这里进行耗时计算)
         */
        void run() override;

    public:
//【刚柔耦合】柔性体求解期采集（进度回调线程写，纯 STL 容器）
        struct FlexCapture {
            mMotionFlexBridge::FlexInput input;
            mMotionFlexBridge::FlexOutput output;
            mMotionFlexBridge::FlexCaptureFrameData data;
        };
        std::vector<FlexCapture> m_flexCaptures;
        std::atomic<int> m_currentProgress{0};
        double m_flexDivergedTime = -1.0;   //【发散守卫】柔体模态坐标发散时刻（<0=未发散）

    private:

        void test();

    signals:
        // --- 向外发送状态的信号 (UI线程会捕获这些信号来更新进度条) ---
        void progressUpdated(int percent);
        void messageLogged(const QString& msg);
        void solveFinished(int scenarioId, bool success);
    protected:

        int m_scenarioId;

        // 求解启动时由 UI 线程捕获的视口模型指针。
        // 求解期间窗口可能失焦导致全局 getCVM 变为 nullptr，
        // 线程内部必须统一使用此成员，禁止再访问 getCVM。
        MechPreData::mViewportModel* m_viewportModel = nullptr;

        std::atomic_bool m_cancelRequested{ false };

        //多体实体引用查询（映射存储；求解期间仅用启动时捕获的视口模型，不回全局单例）
        int refId(MxFunctions::PhysicalFieldType src, int srcId, MxFunctions::PhysicalFieldType dst,
            MxFunctions::MapRelationType role = MxFunctions::MapRelation_None) const;
        std::set<int> refSet(MxFunctions::PhysicalFieldType src, int srcId, MxFunctions::PhysicalFieldType dst,
            MxFunctions::MapRelationType role = MxFunctions::MapRelation_None) const;

        mMotionPidSystem m_sys;
    };
}