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
#include <QWidget>
#include <QTableWidget>

#include "mPanelBase.h"
#include "mPhysicalFieldEnum.h"
#include "mModelStruct.h"

using namespace MxFunctions;
namespace MechPreData
{
	class mViewportModel;
}
namespace MechStructOptionData
{
	class mOptimizationGoalData;
}
namespace MechMainWindow
{
    struct OptGoalPosPickWidget
    {
        QCheckBox* labelpicked = nullptr;
        QPushButton* btnok = nullptr;
        QPushButton* btnclear = nullptr;
        QHBoxLayout* layout = nullptr;
        QWidget* propwid = nullptr;
        QString name_;
        QString PickDataEnum2Str(MxFunctions::PickDataType type)
        {
            switch (type)
            {
            case MxFunctions::PickDataType::None:
                return "";
            case MxFunctions::PickDataType::GeoPoint:
                return "几何点";
            case MxFunctions::PickDataType::GeoEdge:
                return "几何边";
            case MxFunctions::PickDataType::GeoFace:
                return "几何面";
            case MxFunctions::PickDataType::GeoSolid:
                return "几何实体";
            case MxFunctions::PickDataType::Node:
                return "节点";
            case MxFunctions::PickDataType::ElementEdge:
                return "单元边";
            case MxFunctions::PickDataType::ElementFace:
                return "单元面";
            case MxFunctions::PickDataType::Element:
                return "单元";
            case MxFunctions::PickDataType::Part:
                return "部件";
            default:
                break;
            }
            return "";
        }
        void setState(std::pair<MxFunctions::PickDataType, RenderPickResultData> node);

        void setCheckedState(bool checked)
        {
            if (checked == true) {
                // 勾选状态：边框和字体都是 #1385F6
                labelpicked->setStyleSheet(R"(
            QWidget {
                border: 1px solid #1385F6;
                border-radius: 4px;
                color: #1385F6;
                padding-left: 7px;  /* 左内边距，内部所有内容右移 */
            }
        )");
            }
            else {
                // 非勾选状态：普通颜色
                labelpicked->setStyleSheet(R"(
            QWidget {
                border: 1px solid #DCDFE6;
                border-radius: 4px;
                color: #303133;
                padding-left: 7px;  /* 左内边距，内部所有内容右移 */
            }
        )");
            }
        }

        void initWidget(const QString& name, bool hasadd = false)
        {
            name_ = name;
            labelpicked = new QCheckBox();
            btnok = new QPushButton();
            btnclear = new QPushButton();
            layout = new QHBoxLayout();
            propwid = new QWidget();

            btnok->setFixedWidth(30);
            btnclear->setFixedWidth(30);
            btnok->setIcon(QIcon("")); btnok->setToolTip("选择节点"); btnok->setIcon(QIcon(":/mMainWidgets/Icon/tihuan3x.png"));
            btnclear->setIcon(QIcon(":/mMainWidgets/Icon/shanchu3x.png")); btnclear->setToolTip("清空选择");
            layout->setMargin(0);
            layout->addWidget(labelpicked);
            layout->addWidget(btnok);
            layout->addWidget(btnclear);

            propwid->setWindowFlags(Qt::Widget);
            propwid->setContentsMargins(0, 0, 0, 0);
            propwid->setLayout(layout);

            setCheckedState(false);

        }
        void clear()
        {
            if (labelpicked)
            {
                delete labelpicked;
                labelpicked = nullptr;
            }
            if (btnok)
            {
                delete btnok;
                btnok = nullptr;
            }
            if (btnclear)
            {
                delete btnclear;
                btnclear = nullptr;
            }
            if (layout)
            {
                delete layout;
                layout = nullptr;
            }
            if (propwid)
            {
                delete propwid;
                propwid = nullptr;
            }
        }
    };


	//结构优化目标面板
	class MMAINWINDOW_EXPORT mOptimizationGoalPanel : public mPanelBase
	{
		Q_OBJECT

	public:
		/*
		* 默认构造函数
		*/
		mOptimizationGoalPanel(QWidget* parent = Q_NULLPTR);

		/*
		* 析构函数
		*/
		~mOptimizationGoalPanel();

		/*
		* 编辑结构优化目标(从树上点击过来的)
		*/
		void editOptimizationGoalPanel(const QString &type= "");

		/*
		* 取消编辑结构优化目标
		*/
		void uneditOptimizationGoalPanel();

	private:

        //更新预应力下拉框
        void updateCaseCombobox(QComboBox* combobox, QList<CaseType> caseType = { ImplicitLinearStatic });

        //更新柔度表格
        void updateComplianceTable(bool deleteMap, QList<CaseType> caseType = { ImplicitLinearStatic });

        //更新频率表格布局
        void updateFrequencyLaout(const QString& text, FoldSection* fold);

        //更新频率表格
        void updateFrequencyTable(bool deleteMap);

        /*
        * 初始化表格
        */
        void initFrequencyTable();

        /*
        * 表格数据改变信号连接与断开
        */
        void connectFrequencyTableSig();
        void disconnectFrequencyTableSig();

	private slots:

        /*
        * 结构优化方式改变
        */
        void slot_objectiveTypeChanged(const QString& text);

        /*
        * 结构优化响应类型改变
        */
        void slot_responseTypeChanged(const QString &text);

        /*
        * 工况改变(一对一适用)
        */
        void slot_caseChanged(const QString& text);

        /*
        * 目标频率类型改变
        */
        void slot_frequencyTargetTypeChanged(const QString& text);

        /*
        * 指定频率点改变
        */
        void slot_specifiedFrequencyChanged();

        /*
        * 频响优化类型改变
        */
        void slot_frfValueTypeChanged(const QString& text);

        /*
        * 自由度改变
        */
        void  slot_dofChanged(const QString& text);

        /*
        * 获取节点
        */
        void slot_getNode();

        /*
        * 清空节点
        */
        void slot_clearNode();

        /*
        * 替换节点
        */
        void slot_checkNode(int state);

        /*
        * 数据改变
        */
        void slot_lineChanged(QTableWidgetItem* item);

        /*
        * 添加一行
        */
        void slot_addLine();

        /*
        * 删除多行
        */
        void slot_deleteLine();

        /*
        * 清空表格
        */
        void slot_clearTable();

        /*
        * 柔度复选框改变
        */
        void slot_complianceCheckBox(bool isCheck);

        /*
        * 频率复选框改变
        */
        void slot_frequencyCheckBox(bool isCheck);
	protected:

		//数据
        MechStructOptionData::mOptimizationGoalData* _optimizationGoalData;

		//总布局
		QVBoxLayout* _mainLayout;

		//优化方式
        QComboBox* _comboObjectiveType;

        //优化响应
        QComboBox* _comboResponseType;

        //工况
        QComboBox* _comboCase;

        //节点
        OptGoalPosPickWidget* _pickNode;

        //自由度
        QComboBox* _comboDof;

        //目标阶数
        mCalcLineEdit* _orderEdit;

        //目标频率类型(指定频率点/最大响应点/最小响应点/平均/求和/RMS)
        QComboBox* _comboFrequencyTargetType;

        //指定频率点
        mCalcLineEdit* _editSpecifiedFrequency;

        //频响位移优化类型(幅值/实部/虚部)
        QComboBox* _comboFrfValueType;

        //PSD/频响自由度专用下拉框(1-6)
        QComboBox* _comboDofDynamic;

        // 复用标签
        PanelLabel* _labelCase;
        PanelLabel* _labelFrequency;
        PanelLabel* _labelNode;
        PanelLabel* _labelDof;
        // 指定频率点标签（复用，避免重复 new 造成泄漏与切换闪烁）
        PanelLabel* _labelSpecifiedFrequency;
        // 优化类型标签（复用，避免重复 new 造成泄漏与切换闪烁）
        PanelLabel* _labelFrfValueType;

        //综合柔度
        QTableWidget* _tableCompliance;

        //综合频率
        QTableWidget* _tableFrequency;
        QWidget* _tablePara;
        QPushButton* _addBtn;
        QPushButton* _reduceBtn;
        QPushButton* _clearBtn;
        QMetaObject::Connection m_tableChangeConn;

        //柔度指数
        QCheckBox* _complianceCheck;
        QCheckBox* _frequencyCheck;

	};
}