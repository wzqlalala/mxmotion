#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include "mxr/data/SelectDataStruct.h"
#include "mBasicEnum.h"
#include "mPhysicalFieldEnum.h"

#include <memory>
#include <set>
#include <map>

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
#include <QList>

#include "mPanelBase.h"
#include "mMotionFlexBridge.h"

namespace MechPreData
{
	class mViewportModel;
}
namespace MechMotionData
{
    class mMotionResultData;
    class mMotionScenario;
    class mMotionFlexBodyData;
}
namespace MechMainWindow
{

	//方案面板
	class MMAINWINDOW_EXPORT mMotionAnimationPanel : public mPanelBase
	{
		Q_OBJECT

	public:
		/*
		* 默认构造函数
		*/
        mMotionAnimationPanel(QWidget* parent = Q_NULLPTR);

		/*
		* 析构函数
		*/
		~mMotionAnimationPanel();

		/*
		* 编辑方案(从树上点击过来的)
		*/
		void editAnimationPanel(int id = 0);

		/*
		* 取消编辑方案
		*/
		void uneditAnimationPanel();

        /*
        * 方案（及其结果对象）即将被删除：置空结果指针防悬垂；
        * 本面板对空结果有完整守卫（渲染/播放各分支判空）
        */
        void onScenarioDataAboutToBeDestroyed(int id = -1);

	private:
		/*
		* 连接信号
		*/
		void connectSig();

		/*
		* 断开连接信号
		*/
		void disconnectSig();

        // --- 核心动画渲染逻辑 ---
        void renderCurrentFrame();
        void renderFlexBodies();  //【刚柔耦合】柔体变形网格每帧驱动

        // --- 曲线联动（曲线窗显示与标记） ---
        void setCurrentTime(double time);
        void updateCurveMarker(double time);
        void removeCurveMarker();
        void connectCurvePlotSignal();
        void disconnectCurvePlotSignal();

        void refreshMarkerTraceList();
        void rebuildMarkerTraces();
        void updateMarkerTraceCurrentPoints();
        void clearMarkerTraces();

        /**
         * @brief 将所有运动体的渲染矩阵重置为单位矩阵（恢复初始位姿），
         *        并把弹簧符号也恢复到初始位姿显示。
         */
        void resetBodyTransformsToIdentity();

        //【刚柔耦合】动画期间隐藏被柔体替换的部件（几何+网格数据 mask，同树节点隐藏机制），
        // 记录原 mask 供退出动画时恢复；mask 极性：true=隐藏
        void hideFlexReplacedParts(const MechMotionData::mMotionResultData* result);
        void restoreFlexReplacedParts();
        //【刚柔耦合】变形云图图例（复用前处理视口色带窗口：蓝(min)→红(max)，与顶点着色同渐变；
        // 色标逐帧自适应：每帧按当前帧实际最大弹性变形刷新）
        void showFlexDeformLegend(double maxElastic);
        void hideFlexDeformLegend();
        //【刚柔耦合】计算柔体全程最大弹性变形（图例初值与数字格式选择用，一次遍历）
        static double computeMaxElastic(const MechMotionData::mMotionFlexBodyData& fb);

	private slots:
		//切换视口	
		void slot_subWindowActivated(QMdiSubWindow* subWindow);

		//拾取信号
		void slot_finishedPick();

        void onPlayClicked();
        void onPauseClicked();
        void onStopClicked();
        void onTimerTick();
        void onSliderMoved(int frameIndex);
	protected:

        int m_id = -1;

		//真正的结果数据
        MechMotionData::mMotionResultData* m_motionResultData;

		//总布局
		QVBoxLayout* m_mainLayout;

        // ==================== 播放器控件 ====================
        QPushButton* m_btnPlay;
        QPushButton* m_btnPause;
        QPushButton* m_btnStop;

        QSlider* m_sliderTimeline;    ///< 时间轴进度条
        QLabel* m_lblTimeInfo;       ///< 显示 "当前时间 / 总时间"
        QComboBox* m_cmbPlaySpeed;    ///< 播放速度 (1.0x, 2.0x 等)
        QCheckBox* m_chkLoop;         ///< 循环播放复选框
        QCheckBox* m_chkFlexAdaptiveScale = nullptr; ///<【刚柔耦合】云图色标逐帧自适应（默认全程定标）
        QComboBox* m_cmbFlexContourRef = nullptr;    ///< 柔性体云图显示量

        // ==================== Marker 追踪 ====================
        FoldSection* m_foldMarkerTrace;
        QPushButton* m_btnClearMarkerTraces;
        QList<QCheckBox*> m_markerTraceCheckBoxes;
        std::set<int> m_renderedTraceMarkerIds;
        std::set<int> m_renderedCurrentMarkerIds;

        // ==================== 播放器状态 ====================
        QTimer* m_playTimer;          ///< 驱动动画的"引擎"
        int m_currentFrameIndex;      ///< 当前播放到了哪一帧
        bool m_isPlaying;             ///< 播放状态标记
        double m_currentAnimTime = 0.0; // 当前播放的真实物理时间 (秒)
        const void* m_flexBuiltForResult = nullptr;  //【刚柔耦合】柔体网格是否已为当前结果集建立
        std::map<int, std::pair<bool, bool>> m_flexSavedPartMasks; //【刚柔耦合】partId→(几何mask,网格mask) 动画前原值
        std::map<int, double> m_flexMaxElastic;      //【刚柔耦合】bodyId→全程最大弹性变形（图例初值/数字格式选择用）
        std::map<int, QVector<double>> m_flexNodeWeights; //【刚柔耦合】bodyId→节点集总体积权（NX 口径加权拟合用）
        std::map<int, mMotionFlexBridge::FreeModeData> m_flexFreeModes; //【UG 口径】bodyId→自由基模态数据
        double m_flexLegendShownMax = -1.0;          //【刚柔耦合】图例当前显示的本帧色标上限（同帧不重刷）
        qint64 m_lastUpdateTime = 0; // 上一次渲染的时间戳 (毫秒)

        // ==================== 曲线联动 ====================
        QCheckBox* m_chkFollowCurveMark = nullptr;   ///< 曲线上跟随动画帧标记
        QCheckBox* m_chkClickJump = nullptr;         ///< 点击曲线跳转动画帧

		//拾取高亮数据改变的连接
		QMetaObject::Connection conn_geo;
	};
}
