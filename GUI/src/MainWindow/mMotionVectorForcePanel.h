#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include "mPhysicalFieldEnum.h"
#include "mMotionVectorForce.h"
#include "mPointPickerWidget.h"
#include "mDirectionSelectorWidget.h"

#include <memory>
#include <set>

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
#include <QCheckBox>

#include "mPanelBase.h"
#include "mCalcLineEdit.h"

class QStackedWidget;

namespace MechPreData
{
	class mViewportModel;
}
namespace MechMotionData
{
    class mMotionVectorForce;
    class mMotionBushing;
    class mMotionScalarForce;
    class mMotionScalarTorque;
    class mMotionThreePointForce;
    class mMotionThreePointTorque;
}
namespace MechMainWindow
{
	class mMotionExprBuilderWidget;

	//矢量载荷面板（矢量力 / 矢量力矩）
	class MMAINWINDOW_EXPORT mMotionVectorForcePanel : public mPanelBase
	{
		Q_OBJECT

	public:
		/*
		* 默认构造函数
		*/
		mMotionVectorForcePanel(QWidget* parent = Q_NULLPTR);

		/*
		* 析构函数
		*/
		~mMotionVectorForcePanel();

		/*
		* 新建矢量载荷
		*/
		void newMotionVectorForcePanel(MechMotionData::VectorLoadType type);

		/*
		* 编辑矢量载荷(从树上点击过来的)
		*/
		void editMotionVectorForcePanel(int id = 0);

		/*
		* 取消编辑矢量载荷
		*/
		void uneditMotionVectorForcePanel();

        /*
        * 数据对象即将被删除（树删除单个/全部力元时，必须在 delete 数据前调用）：
        * 此时 m_forceElementData 仍有效，安全置空防悬垂——此后面板刷新链里的
        * dynamic_cast（如衬套分支）读到已释放对象会抛 __non_rtti_object 崩溃。
        * feID=-1 表示全部（删除所有力元/清空运动数据）
        */
        void onForceElementDataAboutToBeDestroyed(int feID = -1);

	private:

	       // ==================== 界面构建（构造函数拆分） ====================
	       void buildUI();                 ///< 构建全部界面区域并加入总布局
	       void buildBasicSection();       ///< 基础信息（载荷类型）
	       void buildTargetSection();      ///< 作用对象（作用体+作用点=marker_I 动作侧）
	       void buildDirectionSection();   ///< 方向定义（方向向量=marker_I 定向 + 方向模式）
	       void buildBaseBodySection();    ///< 基本体（marker_J：体+原点+坐标系+偏置）
	       void buildMagnitudeSection();   ///< 载荷大小（常量/表达式，衬套时隐藏）
	       void buildStiffnessSection();   ///< 衬套刚度（6K）
	       void buildDampingSection();     ///< 衬套阻尼（6C）
	       void buildPreloadSection();     ///< 衬套预载（F0/T0）
	       void buildScalarForceSection();
        void buildScalarTorqueSection(); ///< 标量力端点与参数
        void buildThreePointForceSection(); ///< 三点力：应用点/出发点/目标点 + 大小

	       // ==================== 信号绑定（构造函数拆分） ====================
	       void connectSignals();          ///< 绑定全部界面信号

	       // ==================== 辅助 ====================
	       MxFunctions::mCalcLineEdit* makeCalcLineEdit(MxFunctions::mCalcLineEdit::CalcMode mode = MxFunctions::mCalcLineEdit::CalcMode::Standard); ///< 统一表达式输入框工厂
	       MechMotionData::mMotionBushing* bushingData(); ///< 取 Bushing 子类指针（非衬套返回 nullptr）
	       MechMotionData::mMotionVectorForce* vectorData();
	       MechMotionData::mMotionScalarForce* scalarData();
        MechMotionData::mMotionScalarTorque* scalarTorqueData();
        MechMotionData::mMotionThreePointTorque* threePointTorqueData(); ///< 取三点力矩子类指针（非三点力矩返回 nullptr）
        MechMotionData::mMotionThreePointForce* threePointData(); ///< 取三点力子类指针（非三点力返回 nullptr）
	       void notifyDataChanged();       ///< 发射数据更新信号（供外部刷新树/动画）

	       void refreshBodyList();

	       void updateUIFromData();

        /**
         * @brief 按载荷类型刷新界面布局（衬套时隐藏非衬套控件）
         * @param isBushing 是否为衬套类型
         */
        void refreshLayoutByType(bool isBushing);

        void handleLoadTypeChange(MechMotionData::VectorLoadType newType);

        /**
         * @brief 触发三维视口高亮（方向箭头）
         */
        void triggerHighlight();

        /**
         * @brief 根据当前数据更新渲染（作用点+高亮+符号）
         */
        void updateRenderFromData();

        /**
         * @brief 高亮单个运动体（指定高亮渲染 key 与颜色）
         */
        void highlightBody(int bodyId, const std::string& key, const QColor& color);

        /**
         * @brief 清除动作体/基本体的高亮
         */
        void clearBodyHighlights();

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

		//当前力元数据
        MechMotionData::mMotionForceElement* m_forceElementData;

		//总布局
		QVBoxLayout* m_mainLayout;

        // ==================== 界面控件 ====================
        // 1. 基础信息层
        QComboBox* m_cmbLoadType;       ///< [矢量力 / 矢量力矩]

        // 2. 作用对象层
        FoldSection* m_foldTarget;       ///< 作用对象折叠面板
        QComboBox* m_cmbBody;           ///< 作用体选择
        QToolButton* m_btnPickBody;     ///< 拾取作用体
        mPointPickerWidget* m_actionPointPicker; ///< 作用点通用拾取控件

        // 3. 方向层
        FoldSection* m_foldDirection;   ///< 方向定义折叠面板
        mDirectionSelectorWidget* m_directionSelector; ///< 方向选择控件（marker_I 定向）
        QToolButton* m_btnPickDirection;///< 拾取方向矢量
        QLabel* m_lblDirectionMode;     ///< 方向模式标签（衬套时随控件一并隐藏）
        QComboBox* m_cmbDirectionMode;  ///< [全局固定 / 体固定]

        // 4. 大小层
        FoldSection* m_foldMagnitude;   ///< 载荷大小折叠面板（衬套时整体隐藏）
        QComboBox* m_cmbMagnitudeMode;  ///< [常量 / 表达式]
        QLabel* m_lblMagnitude;         ///< 常量大小标签（与输入框一起动态插拔）
        MxFunctions::mCalcLineEdit* m_spbMagnitude; ///< 常量大小
        mMotionExprBuilderWidget* m_exprBuilder;      ///< 幅值表达式构造器

        // 4.5 Bushing 基本体层（marker_J，仅 Bushing 类型显示；原点/方向复用 marker_I）
        FoldSection* m_foldBaseBody;    ///< 基本体折叠面板
        QComboBox* m_cmbBaseBody;       ///< 基本体选择
        QComboBox* m_cmbBaseOffset;     ///< 基本体应用偏置 [否/是]
        MxFunctions::mCalcLineEdit* m_spbOffsetX;   ///< 偏置 X（仅应用偏置=是 时可用）
        MxFunctions::mCalcLineEdit* m_spbOffsetY;   ///< 偏置 Y
        MxFunctions::mCalcLineEdit* m_spbOffsetZ;   ///< 偏置 Z

        // 5. Bushing 刚度层
        FoldSection* m_foldStiffness;   ///< 刚度折叠面板
        MxFunctions::mCalcLineEdit* m_spbKxx;       ///< 刚度 Kxx
        MxFunctions::mCalcLineEdit* m_spbKyy;       ///< 刚度 Kyy
        MxFunctions::mCalcLineEdit* m_spbKzz;       ///< 刚度 Kzz
        MxFunctions::mCalcLineEdit* m_spbKrx;       ///< 刚度 Krx
        MxFunctions::mCalcLineEdit* m_spbKry;       ///< 刚度 Kry
        MxFunctions::mCalcLineEdit* m_spbKrz;       ///< 刚度 Krz

        // 6. Bushing 阻尼层
        FoldSection* m_foldDamping;     ///< 阻尼折叠面板
        MxFunctions::mCalcLineEdit* m_spbCxx;       ///< 阻尼 Cxx
        MxFunctions::mCalcLineEdit* m_spbCyy;       ///< 阻尼 Cyy
        MxFunctions::mCalcLineEdit* m_spbCzz;       ///< 阻尼 Czz
        MxFunctions::mCalcLineEdit* m_spbCrx;       ///< 阻尼 Crx
        MxFunctions::mCalcLineEdit* m_spbCry;       ///< 阻尼 Cry
        MxFunctions::mCalcLineEdit* m_spbCrz;       ///< 阻尼 Crz

        // 7. Bushing 预载层
        FoldSection* m_foldPreload;     ///< 预载折叠面板
        MxFunctions::mCalcLineEdit* m_spbF0x;       ///< 预载力 F0x
        MxFunctions::mCalcLineEdit* m_spbF0y;       ///< 预载力 F0y
        MxFunctions::mCalcLineEdit* m_spbF0z;       ///< 预载力 F0z
        MxFunctions::mCalcLineEdit* m_spbT0x;       ///< 预载扭矩 T0x
        MxFunctions::mCalcLineEdit* m_spbT0y;       ///< 预载扭矩 T0y
        MxFunctions::mCalcLineEdit* m_spbT0z;       ///< 预载扭矩 T0z

        // 8. 标量力专属区
        FoldSection* m_foldScalarForce;
        QComboBox* m_cmbBodyJ;
        mPointPickerWidget* m_basePointPicker; ///< 基本点通用拾取控件
        QComboBox* m_cmbScalarDirectionMode;
        MxFunctions::mCalcLineEdit* m_spbScalarMagnitude;
        mMotionExprBuilderWidget* m_forceFuncBuilder;

        // 标量力矩专属控件
        QComboBox* m_cmbTorqueAxisMode;      ///< 轴模式
        QLabel* m_lblBaseBodyJ;               ///< 基本体标签（标量力/标量力矩共用，动态搬运）
        QLabel* m_lblTorqueMagnitude;          ///< 力矩大小标签
        QComboBox* m_cmbTorqueInputMode;       ///< 标量力矩输入模式（常量/函数）
        QString m_lastSymbolName;               ///< 上一次渲染的符号名（类型切换时先清除旧符号）

        // 标量力输入模式
        QComboBox* m_cmbScalarInputMode;       ///< 标量力输入模式（常量/函数）
        QLabel* m_lblScalarMagnitude;          ///< 标量力载荷大小标签

        // 固定轴方向输入（轴模式≠两点连线时显示）
        mDirectionSelectorWidget* m_axisDirSelector = nullptr; ///< 固定轴方向选择控件
        MxFunctions::mCalcLineEdit* m_spbTorqueMagnitude;  ///< 力矩常量值
        mMotionExprBuilderWidget* m_torqueFuncBuilder;     ///< 力矩函数构造器
        QCheckBox* m_chkTorqueActionOnly;   ///< 仅作用在基本体

        // 三点力专属区（UG Three Point Force：应用点 + 出发点 + 目标点 + 大小）
        FoldSection* m_foldThreePointForce = nullptr;
        QComboBox* m_cmbOriginBody = nullptr;              ///< 出发体
        mPointPickerWidget* m_originPointPicker = nullptr; ///< 出发点通用拾取控件（局部坐标）
        QComboBox* m_cmbTargetBody = nullptr;              ///< 目标体
        mPointPickerWidget* m_targetPointPicker = nullptr; ///< 目标点通用拾取控件（局部坐标）
        QComboBox* m_cmbThreePointInputMode = nullptr;     ///< 输入模式（常量/函数）
        QLabel* m_lblThreePointMagnitude = nullptr;
        MxFunctions::mCalcLineEdit* m_spbThreePointMagnitude = nullptr; ///< 力常量值
        mMotionExprBuilderWidget* m_threePointFuncBuilder = nullptr;   ///< 力函数构造器
        FoldSection* m_foldScalarTorque;    ///< 标量力矩折叠区  ///< 标量力函数构造器
        QCheckBox* m_chkActionOnly;

  //拾取高亮数据改变的连接
  QMetaObject::Connection conn_geo;
	};

}
