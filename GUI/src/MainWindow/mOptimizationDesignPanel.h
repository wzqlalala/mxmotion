
#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include "mxr/data/SelectDataStruct.h"
#include "mBasicEnum.h"

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

#include "mPanelBase.h"
#include "mPhysicalFieldEnum.h"
#include "mPickPointWidget.h"
#include "mPointPickerWidget.h"

using namespace MxFunctions;
using namespace MechCommon;
namespace MechPreData
{
	class mViewportModel;
}
namespace MechStructOptionData
{
	class mOptimizationDesignData;
}
namespace MechMainWindow
{
    //结构优化设计域面板
	class MMAINWINDOW_EXPORT mOptimizationDesignPanel : public mPanelBase
	{
		Q_OBJECT

	public:
		/*
		* 默认构造函数
		*/
		mOptimizationDesignPanel(QWidget* parent = Q_NULLPTR);

		/*
		* 析构函数
		*/
		~mOptimizationDesignPanel();

        /*
        * 创建结构优化目标
        */
        void createOptimizationDesign(QString type = "实体");

		/*
		* 编辑结构优化目标(从树上点击过来的)
		*/
		void editOptimizationDesignPanel(int id);

		/*
		* 取消编辑结构优化目标
		*/
		void uneditOptimizationDesignPanel();

	private:

        /*
        * 点数据，设置颜色
        */
        void updatePointRender(QString str, QVector3D point, QVector3D color);

        /*
        * 创建路径通用拾取控件（仅节点、多选）
        * 拾取节点按先后顺序同步写入绑定的路径成员 QVector<int>（顺序不可破坏）
        * 按 _pickerEntries 中的追加顺序参与拾取控件激活/失活调度
        */
        mPickWidgetController* createPathPickWidget(const QString& tip,
            QVector<int> MechStructOptionData::mOptimizationDesignData::* member);

        /*
        * 将路径数据（有序节点ID）反显到通用拾取控件
        */
        void displayPathData(mPickWidgetController* controller, const QVector<int>& path);

        /*
        * 创建计算点控件并绑定数据成员与渲染点（值变化时同步数据并渲染）
        * 按 _pickerEntries 中的追加顺序参与拾取控件激活/失活调度
        */
        mPointPickerWidget* createPointPicker(const QString& label,
            QVector3D MechStructOptionData::mOptimizationDesignData::* member,
            const QString& renderKey,
            const QVector3D& color);

        /*
        * 失活所有不可用（隐藏/禁用）的拾取控件（计算点控件/通用拾取控件），
        * 避免不可用控件占用全局拾取激活状态；勾选折叠框及进入面板/模式切换时的默认激活另见下方接口
        */
        void deactivateUnavailablePickers();

        /*
        * 激活指定折叠框内第一个可见可用的拾取控件（计算点控件或通用拾取控件）
        * 控件间互斥由控件内部全局互斥保证，激活新的会自动失活其他控件
        */
        void activateFirstPickerInFold(FoldWidget* fold);

        /*
        * 进入面板或设计域类型/优化方式切换后的默认激活：
        * 从上到下激活第一个被勾选折叠框中的第一个可见可用拾取控件；
        * 无被勾选折叠框时不激活；控件间互斥由控件内部全局互斥保证
        */
        void activateFirstCheckedFoldPicker();

        //更新冻结区域单元集合下拉框
        void updateFreezeRegionSetCombobox(QComboBox* combobox);

        //更新属性下拉框
        void updatePropertyCombobox(QComboBox* combobox);

        //更新设计域下拉框
        void updateDomainCombobox(QComboBox* combobox);

        //根据设计域类型更新属性下拉框
        void updatePropertyComboboxByType(QComboBox* combobox, const QString& type);

        //更新坐标系下拉框
        void updateShapeCoordCombobox(QComboBox* combobox);

        //更新设计域参数UI
        void updateDesignParamUI();

	private slots:

        /*
        * 截面属性改变
        */
        void slot_propertyChanged(const QString& text);

        /*
        * 设计域类型改变
        */
        void slot_designDomainTypeChanged(const QString& text);

        /*
        * 优化方式改变
        */
        void slot_optimizationMethodChanged(const QString& text);

        /*
        * 对称约束复选框改变
        */
        void slot_checkPatternChanged(bool state);

        /*
        * 对称约束类型改变
        */
        void slot_comboPatternChanged(const QString& text);

        /*
        * 拔模复选框改变
        */
        void slot_checkDraftChanged(bool state);

        /*
        * 挤出复选框改变
        */
        void slot_checkExtrudeChanged(bool state);

        /*
        * 挤出类型改变
        */
        void slot_comboExtrudeChanged(const QString& text);

        /*
        * 挤出复选框改变
        */
        void slot_checkSupportChanged(bool state);

        /*
        * 模式重复复选框改变
        */
        void slot_checkPatternRepChanged(bool state);

        /*
        * 模式重复类型改变
        */
        void slot_comboPatternRepTypeChanged(const QString& text);

        /*
        * 模式重复参考设计域改变
        */
        void slot_comboPatternRepDomainChanged(const QString& text);

        /*
        * 模式重复坐标系改变
        */
        void slot_comboPatternRepCoordChanged(const QString& text);

        /*
         * 起筋模式改变
         */
        void slot_morphModeChanged(const QString& text);

        /*
         * 起筋界限复选框改变
         */
        void slot_checkMorphBoundChanged(int state);

        /*
         * 形状-变形方式改变
         */
        void slot_shapeDeformModeChanged(const QString& text);

        /*
         * 形状-旋转轴改变
         */
        void slot_shapeRotAxisChanged(const QString& text);

	protected:

		//数据
        MechStructOptionData::mOptimizationDesignData* _optimizationDesignData;

		//总布局
		QVBoxLayout* _mainLayout;

        //设计域属性
        QComboBox* _comboDesignDomainType;  // 设计域类型下拉框
        QComboBox* _comboProperty;          // 截面属性下拉框
        PanelLabel* m_lblOptimizationMethod; // 优化方式标签
        QComboBox* m_comboOptimizationMethod;// 优化方式下拉框
        PanelLabel* m_lblBaseThickness;     // 基准厚度标签
        mCalcLineEdit* m_lineBaseThickness; // 基准厚度输入框

        PanelLabel* m_lblFreezeRegion;       // 冻结区域复选框
        QComboBox* m_comboFreezeRegion;       // 冻结区域单元集合下拉框

        // --- 设计域组 UI 控件 ---
        QCheckBox* m_checkMinMember{ nullptr }; ///< 最小成员尺寸开关
        mCalcLineEdit* m_lineMinMember{ nullptr }; ///< 最小成员尺寸输入框

        QCheckBox* m_checkMaxMember{ nullptr }; ///< 最大成员尺寸开关
        mCalcLineEdit* m_lineMaxMember{ nullptr }; ///< 最大成员尺寸输入框

        QCheckBox* m_checkMinGap{ nullptr }; ///< 最小间隙开关
        mCalcLineEdit* m_lineMinGap{ nullptr }; ///< 最小间隙输入框

        QCheckBox* m_checkStressConstraint{ nullptr }; ///< 应力约束开关
        mCalcLineEdit* m_lineStressConstraint{ nullptr }; ///< 应力约束输入框

        // --- 加强筋参数 UI 控件 ---
        QCheckBox* m_checkRibHeight{ nullptr }; ///< 加强筋高度开关
        mCalcLineEdit* m_lineRibHeight{ nullptr }; ///< 加强筋高度输入框
        QCheckBox* m_checkRibWidth{ nullptr }; ///< 加强筋宽度开关
        mCalcLineEdit* m_lineRibWidth{ nullptr }; ///< 加强筋宽度输入框

        // --- 对称约束组 UI 控件 ---
        PanelLabel* m_lblPattern{ nullptr }; // 对称状态复选框
        QComboBox* m_comboPattern{ nullptr }; // 对称方式下拉框
        PanelLabel* m_lblPatternNum{ nullptr }; // 周向对称数量 Label
        mCalcLineEdit* m_linePatternNum{ nullptr }; // 周向对称数量 输入框

        mPointPickerWidget* m_originWidget;
        mPointPickerWidget* m_dirPoint1Widget;
        mPointPickerWidget* m_dirPoint2Widget;

        // --- 拔模约束组 UI 控件 ---
        PanelLabel* m_lblDraftType{ nullptr };        ///< 拔模方式复选框
        QComboBox* m_comboDraftType{ nullptr };        ///< 拔模方式下拉框

        PanelLabel* m_lblNoHole{ nullptr };            ///< 无孔约束标签
        QComboBox* m_comboNoHole{ nullptr };           ///< 无孔约束下拉框

        mPointPickerWidget* m_startPointWidget{ nullptr };     ///< 起始点计算点控件
        mPointPickerWidget* m_draftDirWidget{ nullptr };       ///< 方向点计算点控件

        // ==========================================
        // 挤出约束组 UI 控件
        // ==========================================
        PanelLabel* m_lblExtrude{ nullptr };        ///< 挤出方式复选框
        QComboBox* m_comboExtrudeType{ nullptr }; ///< 挤出方式下拉框

        mPickWidgetController* m_path1Controller{ nullptr }; ///< 路径1通用拾取控件
        mPickWidgetController* m_path2Controller{ nullptr }; ///< 路径2通用拾取控件

        // ==========================================
        // 自支撑约束组 UI 控件
        // ==========================================
        PanelLabel* m_lblSupportStatus{ nullptr };      ///< 自支撑约束开启状态开关
        mCalcLineEdit* m_lineOverhangAngle{ nullptr };   ///< 悬垂角输入框

        mPointPickerWidget* m_supportStartWidget{ nullptr }; ///< 支撑起始点计算点控件
        mPointPickerWidget* m_supportDirWidget{ nullptr };   ///< 支撑方向点计算点控件

        // ==========================================
        // 模式重复组 UI 控件
        // ==========================================
        PanelLabel* m_lblPatternRepStatus{ nullptr };
        QComboBox* m_comboDomainType{ nullptr };
        PanelLabel* m_lblDomain{ nullptr };
        QComboBox* m_comboDomain{ nullptr };
        QComboBox* m_comboCoordType{ nullptr };

        // 计算点控件
        mPointPickerWidget* m_repBasePointWidget{ nullptr };
        mPointPickerWidget* m_repDirPoint1Widget{ nullptr };
        mPointPickerWidget* m_repDirPoint2Widget{ nullptr };
        mPointPickerWidget* m_repDirPoint3Widget{ nullptr };

        mCalcLineEdit* m_lineScaleX{ nullptr };
        mCalcLineEdit* m_lineScaleY{ nullptr };
        mCalcLineEdit* m_lineScaleZ{ nullptr };

        // ==========================================
        // 形貌-起筋参数 UI 控件
        // ==========================================
        FoldSection* m_morphParamFold{ nullptr };
        PanelLabel* m_lblMorphUpLimit{ nullptr };
        mCalcLineEdit* m_lineMorphUpLimit{ nullptr };    ///< 上限
        PanelLabel* m_lblMorphDownLimit{ nullptr };
        mCalcLineEdit* m_lineMorphDownLimit{ nullptr };  ///< 下限
        PanelLabel* m_lblMorphInitRatio{ nullptr };
        QComboBox* m_comboMorphInitRatio{ nullptr };     ///< 初始起筋比

        // ==========================================
        // 形貌-起筋模式 UI 控件
        // ==========================================
        FoldSection* m_morphModeFold{ nullptr };
        QComboBox* m_comboMorphMode{ nullptr };          ///< 起筋模式
        mCalcLineEdit* m_lineMorphMinWidth{ nullptr };   ///< 最小宽度
        mCalcLineEdit* m_lineMorphAngle{ nullptr };      ///< 起筋角度
        mCalcLineEdit* m_lineMorphHeight{ nullptr };     ///< 起筋高度
        QComboBox* m_comboMorphDirection{ nullptr };     ///< 起筋方向

        // ==========================================
        // 形貌-起筋界限 UI 控件
        // ==========================================
        QCheckBox* m_morphBoundCheck{ nullptr };         ///< 起筋界限启用复选框
        FoldSection* m_morphBoundFold{ nullptr };
        mPointPickerWidget* m_morphOriginWidget{ nullptr };      ///< 原点
        mPointPickerWidget* m_morphDirPoint1Widget{ nullptr };   ///< 方向点1
        mPointPickerWidget* m_morphDirPoint2Widget{ nullptr };   ///< 方向点2

        // ==========================================
        // 形状-设计域属性附加 UI 控件（四行复选框）
        // ==========================================
        QCheckBox* m_checkShapeSplitDomain{ nullptr };   ///< 按域分割
        QCheckBox* m_checkShapeSplitPart{ nullptr };     ///< 按部件分割
        QCheckBox* m_checkShapeKeepHandle{ nullptr };    ///< 保留控制柄
        QCheckBox* m_checkShape2DSplit{ nullptr };       ///< 2D域分割

        // ==========================================
        // 形状-变形定义折叠框 UI 控件
        // ==========================================
        FoldSection* m_shapeDeformFold{ nullptr };       ///< 变形定义折叠框
        QComboBox* m_comboShapeDeformMode{ nullptr };    ///< 变形方式：交互/平移/旋转/缩放/移动至节点

        // 平移/缩放/旋转公共行（动态显隐）
        PanelLabel* m_lblShapeTx{ nullptr };
        mCalcLineEdit* m_lineShapeTx{ nullptr };
        PanelLabel* m_lblShapeTy{ nullptr };
        mCalcLineEdit* m_lineShapeTy{ nullptr };
        PanelLabel* m_lblShapeTz{ nullptr };
        mCalcLineEdit* m_lineShapeTz{ nullptr };

        // 坐标系（平移/缩放时显示）
        PanelLabel* m_lblShapeCoordSystem{ nullptr };
        QComboBox* m_comboShapeCoordSystem{ nullptr };

        // 旋转专用
        PanelLabel* m_lblShapeRotAngle{ nullptr };
        mCalcLineEdit* m_lineShapeRotAngle{ nullptr };   ///< 旋转角度
        mPointPickerWidget* m_shapeRotPickWidget{ nullptr };     ///< 旋转中心点计算点控件
        PanelLabel* m_lblShapeRotAxis{ nullptr };
        QComboBox* m_comboShapeRotAxis{ nullptr };       ///< X轴/Y轴/Z轴/自定义
        PanelLabel* m_lblShapeCustomAxis{ nullptr };
        mCalcLineEdit* m_lineShapeCustomAxis{ nullptr }; ///< 自定义中心轴

        // 移动至节点专用
        PanelLabel* m_lblShapeTargetNode{ nullptr };
        mCalcLineEdit* m_lineShapeTargetNode{ nullptr }; ///< 节点ID

        // ==========================================
        // 形状-变形参数折叠框 UI 控件
        // ==========================================
        FoldSection* m_shapeParamFold{ nullptr };        ///< 变形参数折叠框
        PanelLabel* m_lblShapeInitValue{ nullptr };
        mCalcLineEdit* m_lineShapeInitValue{ nullptr };  ///< 初始值
        PanelLabel* m_lblShapeMinValue{ nullptr };
        mCalcLineEdit* m_lineShapeMinValue{ nullptr };   ///< 下限值
        PanelLabel* m_lblShapeMaxValue{ nullptr };
        mCalcLineEdit* m_lineShapeMaxValue{ nullptr };   ///< 上限值
        PanelLabel* m_lblShapeMoveLimit{ nullptr };
        mCalcLineEdit* m_lineShapeMoveLimit{ nullptr };  ///< 移动限制

        // 拾取控件调度条目：计算点控件与通用拾取控件统一参与激活/失活调度
        struct PickerEntry
        {
            QWidget* view{ nullptr };                      ///< 参与可见/可用及所属折叠框判断的控件
            mPickWidgetController* controller{ nullptr };  ///< 激活/失活控制
            mPointPickerWidget* pointPicker{ nullptr };    ///< 计算点控件（通用拾取控件为空）
        };

        // 按面板自上而下顺序记录全部拾取控件（计算点控件与通用拾取控件），用于拾取控件激活/失活调度
        QList<PickerEntry> _pickerEntries;
 };
}
