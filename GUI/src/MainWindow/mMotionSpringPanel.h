#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include "mBasicEnum.h"
#include "mPhysicalFieldEnum.h"
#include "mMotionSpringBase.h"
#include "mPointPickerWidget.h"

#include <memory>
#include <set>

//QT
#include <QWidget>
#include <QMouseEvent>
#include <QHash>
#include <QGridLayout>
#include <QLabel>
#include <QToolButton>
#include <QMdiSubWindow>
#include <QComboBox>
#include <QStackedWidget>
#include <QVector3D>
#include <QQuaternion>

#include "mPanelBase.h"
#include "mCalcLineEdit.h"

namespace MechPreData
{
	class mViewportModel;
}
namespace MechMotionData
{
    class mMotionSpringBase;
    class mMotionBodyPointSpring;
}
namespace mxr
{
    class mSymbolController;
}
using namespace MechMotionData;
namespace MechMainWindow
{
	class mMotionExprBuilderWidget;


	//多体弹簧面板
	class MMAINWINDOW_EXPORT mMotionSpringPanel : public mPanelBase
	{
		Q_OBJECT

	public:
		/*
		* 默认构造函数
		*/
		mMotionSpringPanel(QWidget* parent = Q_NULLPTR);

		/*
		* 析构函数
		*/
		~mMotionSpringPanel();

		/*
		* 新建弹簧
		*/
		void newMotionSpringPanel(MechMotionData::SpringType type);

		/*
		* 编辑弹簧(从树上点击过来的)
		*/
		void editMotionSpringPanel(int id = 0);

		/*
		* 取消编辑弹簧
		*/
		void uneditMotionSpringPanel();
        /*
        * 数据对象即将被删除（树删除该类型前调用，此时对象仍有效）：置空编辑指针
        * 防悬垂——面板刷新链读已释放对象是未定义行为（衬套删除崩溃同源）
        */
        void onMotionSpringDataAboutToBeDestroyed(int id = -1);


		/**
		 * @brief 静态接口：创建/更新弹簧符号（动画面板可直接调用，不依赖面板实例）
		 * @param symbolController 符号控制器
		 * @param springId 弹簧ID
		 * @param type 弹簧类型
		 * @param dampingEnabled 是否启用阻尼
		 * @param name 弹簧名称，用于文字标注
		 * @param origin 符号中心（世界坐标）
		 * @param axis 符号轴向（世界坐标，已归一化）
		 */
		static void createOrUpdateSpringSymbol(mxr::mSymbolController* symbolController,
			int springId,
			MechMotionData::SpringType type,
			bool dampingEnabled,
			const QString& name,
			const QVector3D& origin,
			const QVector3D& axis);

		/**
		 * @brief 静态接口：根据运动副两个 marker 的世界位姿创建/更新弹簧符号（用于动画播放）
		 * @param posI marker I 的世界位置
		 * @param posJ marker J 的世界位置
		 * @param rotI marker I 的世界旋转（用于确定扭转弹簧轴向等方向）
		 * @param rotJ marker J 的世界旋转（预留）
		 */
		static void createOrUpdateSpringSymbolByMarkers(mxr::mSymbolController* symbolController,
			int springId,
			MechMotionData::SpringType type,
			bool dampingEnabled,
			const QString& name,
			const QVector3D& posI,
			const QVector3D& posJ,
			const QQuaternion& rotI = QQuaternion(),
			const QQuaternion& rotJ = QQuaternion());

		/**
		 * @brief 静态接口：根据运动体世界矩阵创建/更新体-点弹簧符号
		 * @param bodyTransformI 运动体 I 的世界矩阵
		 * @param bodyTransformJ 运动体 J 的世界矩阵
		 */
		static void createOrUpdateSpringSymbolByBodyTransforms(mxr::mSymbolController* symbolController,
			int springId,
			MechMotionData::SpringType type,
			bool dampingEnabled,
			const QString& name,
			const QMatrix4x4& bodyTransformI,
			const QMatrix4x4& bodyTransformJ,
			const QVector3D& localI,
			const QVector3D& localJ);

		/**
		 * @brief 静态接口：删除弹簧符号
		 */
		static void clearSpringSymbol(mxr::mSymbolController* symbolController, int springId);

	private:

		void refreshJointList(MechMotionData::SpringType type);
		void refreshBodyComboBoxes();

		void updatePointRender(const QString& key, const QVector3D& point, const QVector3D& color);

		/**
		 * @brief 高亮单个运动体（用于体-点弹簧选择动作/基本体时的临时反馈）
		 */
		void highlightBody(int bodyId, const std::string& key, const QColor& color);

		/**
		 * @brief 清除动作/基本体的临时高亮
		 */
		void clearBodyHighlights();

		void updateUIFromData();
		void updateUIFromBodyPointData();
		void updateInstallValueFromBodyPoints();

		void setBodyPointUIVisible(bool visible);

		/**
		 * @brief 触发三维视口高亮
		 */
		void triggerHighlight();

		// 构建弹簧符号（内部实例方法，调用静态接口）
		void buildTranslationalSpringSymbol(const QVector3D& origin, const QVector3D& axis);
		void buildTorsionalSpringSymbol(const QVector3D& origin, const QVector3D& axis);
		void clearSpringSymbol();

		// 内部静态实现
		static void buildTranslationalSpringSymbolImpl(mxr::mSymbolController* symbolController,
			int springId, bool dampingEnabled, const QString& name,
			const QVector3D& origin, const QVector3D& axis);
		static void buildTranslationalSpringSymbolByMarkersImpl(mxr::mSymbolController* symbolController,
			int springId, bool dampingEnabled, const QString& name,
			const QVector3D& posI, const QVector3D& posJ);
		static void buildTorsionalSpringSymbolImpl(mxr::mSymbolController* symbolController,
			int springId, bool dampingEnabled, const QString& name,
			const QVector3D& origin, const QVector3D& axis);

		//专门处理类型切换与数据迁移的核心逻辑
		void handleSpringTypeChange(MechMotionData::SpringType newType);

	public:
		/**
		 * @brief 按数据对象补建弹簧符号（仅在符号不存在时创建，供模型树父节点点击等
		 *        面板未编辑过的场景批量渲染；显隐由树的 setSymbolShow 统一控制）
		 */
		static void ensureSpringSymbolByData(mxr::mSymbolController* symbolController,
			MechMotionData::mMotionSpringBase* springData, mxr::CCSMRendWidget* rendWidget = nullptr);

	public:
		/**
		 * @brief 根据外部传入的关节世界原点和轴重新绘制弹簧符号（用于动画播放时跟随运动）
		 */
		void updateSpringSymbolForAnimation(const QVector3D& origin, const QVector3D& axis);

	private:
		/*
		* 连接信号
		*/
		void connectSig();

		/*
		* 断开连接信号
		*/
		void disconnectSig();

	private slots:
		//切换视口	
		void slot_subWindowActivated(QMdiSubWindow* subWindow);

		//拾取信号
		void slot_finishedPick();
	protected:

		//真正的弹簧数据
        MechMotionData::mMotionSpringBase* m_motionSpringData;

		//总布局
		QVBoxLayout* m_mainLayout;

        // ==================== 界面控件 ====================
        // 1. 基本属性
        QLabel* m_lblJoint;             ///< 关联运动副标签
        QLabel* m_lblStiffness = nullptr;   ///< 刚度标签（单位随弹簧类型切换）
        QLabel* m_lblFreeValue = nullptr;   ///< 自由长度/角度标签
        QLabel* m_lblInstallValue = nullptr;///< 安装长度/角度标签
        QLabel* m_lblPreload = nullptr;     ///< 预紧力/力矩标签
        QLabel* m_lblDamping = nullptr;     ///< 阻尼标签
        QComboBox* m_cmbSpringType;     ///< [拉压弹簧 / 扭转弹簧 / 体点弹簧]
        QComboBox* m_cmbJoint;          ///< 关联运动副

        FoldSection* m_foldActionBodyPoint; ///< 动作折叠框（体点弹簧）
        FoldSection* m_foldBaseBodyPoint;  ///< 基本折叠框（体点弹簧）

        // 1.5 体-点弹簧端点绑定
        QComboBox* m_cmbActionBody;     ///< 动作运动体
        QComboBox* m_cmbBaseBody;       ///< 基本运动体

        mPointPickerWidget* m_actionPointPicker;   ///< 动作点（端点I）拾取控件
        mPointPickerWidget* m_basePointPicker;     ///< 基本点（端点J）拾取控件

        // 2. 弹簧参数
        MxFunctions::mCalcLineEdit* m_spbStiffness; ///< 刚度
        MxFunctions::mCalcLineEdit* m_spbFreeValue; ///< 自由长度/自由角度
        MxFunctions::mCalcLineEdit* m_spbInstallValue; ///< 安装长度/安装角度
        MxFunctions::mCalcLineEdit* m_spbPreload;   ///< 预紧力/预紧力矩
        QComboBox* m_cmbSpringDirection; ///< 方向：双向/仅拉伸/仅压缩（或扭转方向）

        // 3. 阻尼参数
        QCheckBox* m_chkEnableDamping;  ///< 启用阻尼
        MxFunctions::mCalcLineEdit* m_spbDamping; ///< 阻尼
        QComboBox* m_cmbDampingDirection; ///< 阻尼方向

        // 4. 高级（函数/曲线）
        QCheckBox* m_chkUseForceFunction;   ///< 使用力函数
        mMotionExprBuilderWidget* m_forceFuncBuilder;  ///< 力函数构造器
        QCheckBox* m_chkUseDampingFunction;   ///< 使用阻尼函数
        mMotionExprBuilderWidget* m_dampFuncBuilder;    ///< 阻尼函数构造器

		//拾取高亮数据改变的连接
		QMetaObject::Connection conn_geo;
	};
}
