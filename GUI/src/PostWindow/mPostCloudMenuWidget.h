#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mpostwindow_global.h"

//QT
#include <QWidget>
#include <QToolButton>
#include <QComboBox>
#include <QCheckBox>
#include <QSlider>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QButtonGroup>
#include <QPushButton>

//Function
#include "mCalcLineEdit.h"

//post
#include "mPostDataBase.h"

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;


using namespace PostData;
using namespace MxFunctions;
namespace PostWindow
{
	class MPOSTWINDOW_EXPORT mPostCloudMenuWidget : public QWidget
	{
		Q_OBJECT
	public:
		explicit mPostCloudMenuWidget(QWidget* parent = nullptr);

	public slots:
		/*
		* 选择工况下拉框
		*/
		void slot_chooseCase(const QString&);

		/*
		* 选择帧下拉框
		*/
		void slot_chooseFrame(const QString&);

		/*
		* 选择变量下拉框
		*/
		void slot_chooseVariable(const QString&);

		/*
		* 选择分量
		*/
		void slot_chooseCompo(const QString&);

		/*
		* 选择层下拉框
		*/
		void slot_chooseLayer(const QString&);

		/*
		* 选择复数下拉框
		*/
		void slot_chooseComplex(const QString&);

		/*
		* 选择渲染范围
		*/
		void slot_chooseRangeType(int index);

		/*
		* 重置最大/小值
		*/
		void slot_resetMostValue();

		/*
		* 最值改变
		*/
		void slot_editValueChanged();

		/*
		* 设置位置显隐
		*/
		void slot_setPositionVisible(bool isshow);

		/*
		* 处理显示、隐藏自由节点的结果值
		*/
		void showOrHideFreeNode();

		/*
		* 删除自由节点
		*/
		void deleteFreeNode();

		/*
		* 关联云图
		*/
		void relatedCloudFreeNode(bool flag);

		/*
		* 壳杆梁实体化
		*/
		void slot_ShowOrHidePhysicalization();

		/*
		* 设置点大小
		*/
		void slot_setPointSize(double size);

		/*
		* 更新三维结果菜单
		*/
		void update3DResultMenu(QString viewname);

		/*
		* 更新三维结果菜单界面
		*/
		void updateResultMenuGUI();

		/*
		* 更新三维结果菜单界面
		*/
		void updateFreeNodeBtnStatus(bool isDisabled);

		/*
		* 云图
		*/
		void slot_showCloud();

		/*
		* 改变最大/小值
		*/
		void slot_setMostValue(bool flag);

		/*
		* 设置当前视图名称
		*/
		void setCurViewName(const QString& viewName);

	private:
		// 排序QStringList
		QStringList sortQStringList(const QStringList& frameIds);

		//绘制云图
		void drawCloudMap();

		//查找满足条件的字符串
		bool isContainStr(const QStringList& names, QString str, QString& name);

        //设置显示模式
        void setShowMode();

	private:
		// B1b(M1)：会话渐进装载未就绪帧的一次性 frameReady 重入连接（未就绪保持
		// 上一帧，装载完成后重入 slot_showCloud 完成绑定显示；连接自断开防重复）
		QMetaObject::Connection _cloudFrameReadyConn{};

		// FR-05 跳帧意图标记：播放中用户主动选帧（slot_chooseFrame 入口置位），
		// slot_showCloud 消费后复位——区分"改帧=跳帧续播"与"改变量/分量=当前帧起生效"；
		// 工况级联（slot_chooseCase 重建帧下拉）在源头复位防程序性误标（04 评审 C1）
		bool _frameJumpPending = false;

	signals:

	protected:

	public:
		//切换组随菜单模块移除不再创建，恒为 nullptr（成员保留：槽函数与状态同步
		//路径原样，updateResultMenuGUI 以空指针防护跳过）
		QComboBox* _caseCombo = nullptr;
		QComboBox* _varCombo = nullptr;
		QComboBox* _complexCombo = nullptr;
		QComboBox* _frameCombo = nullptr;
		QComboBox* _compoCombo = nullptr;
		QComboBox* _layerCombo = nullptr;
		QButtonGroup* _rangeBG;
		QLineEdit* _editMax;
        QLineEdit* _editMin;
		QToolButton* _resetBtnMax;
		QToolButton* _resetBtnMin;
		QCheckBox* _checkboxMax;
		QCheckBox* _checkboxMin;
		QToolButton* _toolbtnFreeNode;
		QDoubleSpinBox* _sizeSpinBox;
		QCheckBox* _checkboxColud;
		QPushButton* _deleteBtn;
		QToolButton* _toolbtnFilter;
		QToolButton* _toolbtnExportSTL = nullptr;//云图-过滤 导出STL按钮（过滤开启时可用）
		QComboBox* _comboRule;
		mCalcLineEdit* _lineEditRangeMin;
		mCalcLineEdit* _lineEditRangeMax;
        QSlider* _m_slider = nullptr;//滑块
        bool _isGreater = false;
        bool _isModify = false;//防止循环修改
		bool										_isFreeNodeShow = false;
		bool										_isSBRShow = false;
		QString										_curViewName{};
		bool										_isDrawCloudMap = true; //是否允许绘制云图
		QHash<QString, PostData::oneCaseInfo*>		_caseInfos{};//key-工况  value-工况信息
		QString										_filterMin;
		QString										_filterMax;
		bool										_filterOpen = false;//过滤功能开关
	};
}
