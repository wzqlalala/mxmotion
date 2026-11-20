#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include <memory>

//QT
#include <QWidget>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QHash>
#include <QGridLayout>
#include <QStringListModel>
#include <QTreeWidget>
#include <QLineEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QFrame>
#include <QToolButton>
#include <QComboBox>
#include <QPointer>

#include "mPanelBase.h"

#include <functional>

#include "mPhysicalFieldEnum.h"
#include "mMapRelationEnum.h"

#include "mPickWidgetController.h"

using namespace MxFunctions;
namespace MechCaseData
{
	class mCaseBase;
	class mCaseImplicitLinearStatic;
}
namespace MechPreData
{
	class mViewportModel;
}
namespace MechMainWindow
{
	class OutputControlWidget : public QWidget
	{
		Q_OBJECT
	public:
        enum VariableType {
            None = 1,   //不启用集合
            NodeSet,    //节点类型集合
            ElementSet  //单元类型集合
        };

		explicit OutputControlWidget(QHash<QString, QStringList> structure,
			QHash<QString, QStringList> currentSelections,
            shared_ptr<QHash<QString, mPickWidgetController*>>,
			QWidget* parent = nullptr);

		// 获取当前选中的数据，用于保存
        //更新输出列表，新增变量选择集合
        QHash<QString, QHash<QString, QStringList>> getFinalOutputs();

		void setDefaultOutputs(const QHash<QString, QStringList>& defaults);


        // 设置数结构定义
        void resetTreeData(const QHash<QString, QStringList>& structure, const QHash<QString, QStringList>& defaults);

		// 设置树形控件最小高度，用于特定面板需要更高显示区域
		void setTreeMinimumHeight(int height);

        static VariableType getVariableType(const QString& key, const QString& value);

        void updateTopLabel(); // 更新顶部的显示文本

        //注册挂载/卸载回调（面板侧实现）：面板是拾取控件的唯一所有者，
        //本控件只决定“某行要不要显示拾取控件”，视图父子关系的建立与解除全部由面板完成
        void setPickerMounter(const std::function<void(const QString& var, QWidget* host, bool checked)>& mount);
        //nextVar：取消勾选时同树内下一个已勾选的拾取变量（接力激活用），无则为空
        void setPickerUnmounter(const std::function<void(const QString& var, const QString& nextVar)>& unmount);

        void refreshTreeState(); 		// 将更新树状态的逻辑提取出来，方便复用
signals:
		void outputsChanged(const QHash<QString, QHash<QString, QStringList>>& newOutputs);

        void firstNodeChecked(const QString& name);

	private slots:
		// 树节点改变信号（勾选）
		void onItemChanged(QTreeWidgetItem* item, int column);
		// 搜索框文本改变
		void onFilterTextChanged(const QString& text);
		void onResetClicked();

	private:
        QString buildVariableStrFromTreeItem(QTreeWidgetItem* item);
        QStringList getSelectSetListFromTreeItem(QTreeWidgetItem* item);

        //输出项勾选后才显示对应的拾取控件
        void updatePickWidgetVisibility(QTreeWidgetItem* item);

        //按树顺序查找 varName 之外下一个已勾选的拾取变量（接力激活目标），无则返回空串
        QString nextCheckedPickVar(const QString& varName) const;

        bool hasOtherCheckedPickVar(const QString& varName) const;

        QWidget* createPickerShell(const QString& var);

		void initUI();
		void initTreeData();
		void updateParentState(QTreeWidgetItem* parentItem);
		void updateChildrenState(QTreeWidgetItem* parentItem);    // 根据父节点的状态，强行设置所有子节点的状态
		
		

        //改为预定义树结构，控制树节点顺序
        static QHash<QString, QHash<QString, VariableType>> m_sequenceValues;
        static QStringList m_sequence;
		QHash<QString, QStringList> m_structure; // 树的结构定义
		QHash<QString, QStringList> m_selections; // 当前选中的值
		QHash<QString, QStringList> m_defaults; // 存储默认勾选配置

		QToolButton* m_resetBtn; // 复位按钮
		QLabel* m_displayLabel;      // 顶部显示已选项
		QLineEdit* m_filterEdit;     // 过滤输入框
		QTreeWidget* m_treeWidget;   // 树形控件
		bool m_isProgrammaticChange; // 防止信号递归调用的标志位

        //OutputControlWidget不持有控件：面板是唯一所有者，防止每次构建树都重新创建
        weak_ptr<QHash<QString, mPickWidgetController*>> m_pickers;

        //挂载/卸载回调，由面板注册：树上的挂载壳可被 Qt 随意销毁，视图始终归面板
        std::function<void(const QString& var, QWidget* host, bool checked)> m_pickerMounter;
        std::function<void(const QString& var, const QString& nextVar)> m_pickerUnmounter;
	};

	// 历史数据输出控制控件（含下拉框 + 树形选择 + 动刚度值输入）
    class HistoryOutputControlWidget : public QWidget
    {
        Q_OBJECT

    public:
        explicit HistoryOutputControlWidget(
            QHash<QString, QStringList> structure,
            QHash<QString, QStringList> currentSelections,
            QHash<QString, QStringList> kdSelections,
            QWidget* parent = nullptr);

        // 输出结果获取
        QHash<QString, QStringList> getFinalOutputs() const;
        QStringList getKdOutputs() const;

        // 默认值设置
        void setDefaultOutputs(const QHash<QString, QStringList>& defaults);
        void setKdDefaultOutputs(const QStringList& defaults);

        // 动刚度值操作
        QString getDynamicStiff() const;
        void setDynamicStiff(const QString& val);

        // 外部可访问控件
        QComboBox* m_comboFileType = nullptr;
        QComboBox* m_comboDataType = nullptr;
        QComboBox* m_comboSet = nullptr;
        QLineEdit* m_dynamicStiffLineEdit = nullptr;

    signals:
        void outputsChanged(const QHash<QString, QStringList>& newOutputs);
        void kdOutputsChanged(const QStringList& newKdOutputs);
        void dynamicStiffChanged(const QString& val);

    private slots:
        void onItemChanged(QTreeWidgetItem* item, int column);
        void onFilterTextChanged(const QString& text);
        void onResetClicked();

    private:
        // UI初始化
        void initUI();
        void initTreeStructure(); // 树结构一次性初始化

        // 状态更新
        void refreshTreeState(); // 只更新勾选状态
        void updateParentState(QTreeWidgetItem* parentItem);
        void updateChildrenState(QTreeWidgetItem* parentItem);
        void updateTopLabel();
        void updateKdAreaVisibility();

        // 数据结构
        QStringList m_sequence;
        QHash<QString, QStringList> m_structure;
        QHash<QString, QStringList> m_selections;
        QHash<QString, QStringList> m_defaults;

        // 动刚度相关
        QStringList m_kdItems;
        QHash<QString, QStringList> m_kdSelections;
        QHash<QString, QStringList> m_kdDefaults;

        // UI控件
        QToolButton* m_resetBtn = nullptr;
        QLabel* m_displayLabel = nullptr;
        QLineEdit* m_filterEdit = nullptr;
        QTreeWidget* m_treeWidget = nullptr;
        QLabel* m_lblDynamicStiff = nullptr;
        QWidget* m_kdAreaContainer = nullptr;

        // 节点缓存（优化性能）
        QHash<QString, QTreeWidgetItem*> m_categoryNodes;
        QHash<QString, QHash<QString, QTreeWidgetItem*>> m_itemNodes;
        QTreeWidgetItem* m_kdRootNode = nullptr;
        QHash<QString, QTreeWidgetItem*> m_kdItemNodes;
        QTreeWidgetItem* m_areaValueItem = nullptr;

        // 状态标志
        bool m_isProgrammaticChange = false;
    };

	//分析工况设置面板
	class MMAINWINDOW_EXPORT mCaseSettingPanel : public mPanelBase
	{
		Q_OBJECT

	public:
		/*
		* 默认构造函数
		*/
		mCaseSettingPanel(QWidget* parent = Q_NULLPTR, QString type = "");

		/*
		* 析构函数
		*/
		~mCaseSettingPanel();

		/*
		* 获取caseID
		*/
		int getCaseID();

        /*
        * 获取casebase
        */
        const MechCaseData::mCaseBase* getCaseData();

		template<typename T>
		T* toType(MechCaseData::mCaseBase* base)
		{
			return dynamic_cast<T*>(base);
		};

		//设置工况数据
		void newCasePanel(int caseID);

		//更新关联数据到界面
		void updateMapRelationData();

		//退出工况编辑态（供删除工况等场景调用）
		void uneditCasePanel();

		//新建工况	
		virtual void newCasePanel();

		//更新面板
		virtual void updatePanel();

		//切换视口	

    public slots:
        //子类可自定义切换输出文件类型的逻辑
        virtual void slot_outputTypeChanged(const QString& type);

        void onOutputChanged(const QHash<QString, QHash<QString, QStringList>>& newData);

        void onPchOutputChanged(const QHash<QString, QHash<QString, QStringList>>& newData);

	protected:
        //面板退出钩子：setMainWidget 切换面板时旧面板被 hide()，借此恢复拾取工具栏按钮（激活态保留，见 showEvent）
        void hideEvent(QHideEvent* event) override;
        //面板进入钩子：全局无激活中的拾取控件时，恢复上一次激活的输出拾取控件
        void showEvent(QShowEvent* event) override;

        void buildPickWidgets(const QHash<QString, QStringList>& defaultData);
        void buildPchPickWidgets(const QHash<QString, QStringList>& defaultData);

        //拾取控件挂载/卸载/释放：视图父子关系的改动全部集中在此，面板是唯一所有者
        void mountOutputPicker(const QString& var, bool isPch, QWidget* host, bool checked);
        //nextVar 非空且卸载的是激活态控件时，接力激活下一个拾取变量（拾取会话转移）；
        //无接力目标时正常失活卸载，但延迟恢复拾取工具栏按钮（仅退出/切换面板时恢复）
        void unmountOutputPicker(const QString& var, bool isPch, const QString& nextVar = QString());
        //immediate=false：deleteLater 延迟销毁（类型切换用）；true：同步 delete（panel 析构用，不依赖级联顺序）
        void releaseOutputPickers(bool isPch, bool immediate = false);

        //失活全部处于激活态的输出/PCH拾取控件（退出/切换面板时调用），失活路径会恢复被置灰的拾取工具栏按钮
        void deactivateActiveOutputPickers();
        //接力激活下一个已勾选变量的拾取控件
        void activateOutputPicker(const QString& var, bool isPch);
        //恢复拾取工具栏按钮（全部可选）
        void restoreOutputPickToolbar();

        //最近激活的输出/PCH拾取变量（跨面板切换保留，进入面板时按需恢复激活态）
        QString m_lastActiveVar;
        QString m_lastActivePchVar;

        mPickWidgetController* ensureOutputPicker(const QString& var, bool isPch);
        QWidget* ensurePickerPark();

        mPickWidgetController* createOutputPickerOnDemand(const QString& var, bool isPch);

        inline QHash<QString, QString> getSetNamesList(QVector<MxFunctions::SetType>types);

        void createNodePickController(mPickWidgetController*& picker, mColorBlock colorBlock, const QString& tip, const QStringList& selected);

        void createCellPickController(mPickWidgetController*& picker, mColorBlock colorBlock, const QString& tip, const QStringList& selected);

		//更新预应力下拉框
		void updatePreloadCombobox(QComboBox* combobox, QList<CaseType> caseType = {ImplicitLinearStatic});

		//更新曲线下拉框
		void updateFunctionCombobox(QComboBox* combobox);

        //更新超单元参数下拉框
        void updateGenericParamCombobox(QComboBox* combobox);

		//更新集合下拉框(默认是所有类型集合)
		void updateSetCombobox(QComboBox* combobox, QList<SetType> setTypes = {} , MapRelationType type = MapRelation_None);

		//集合数据改变映射关系
		void updateSetMapRelation(QComboBox* combobox, MapRelationType type = MapRelation_None);

		//初始化输出控制块
		void initOutputSection(QHash<QString, QStringList> defaultData);
        //初始化PCH输出控制块
        void initPchOutputSection(QHash<QString, QStringList> defaultData);

		//初始化历史输出设置块
		void initHistoryOutputSection(QHash<QString, QStringList> defaultData, QHash<QString, QStringList> defaultKdData, bool defaultOpen = false);

	protected:

		//分析工况数据
		MechCaseData::mCaseBase* _caseData = nullptr;

		//总布局
		QVBoxLayout* _mainLayout;

        //工况定义框
        FoldSection* _defineFold;

		//当前模型
		//MechPreData::mViewportModel* _currentViewportModel;
		// 支持选择的工况类型
		QList<CaseType> _caseTypes;
		//工况
		QComboBox* _preloadCombo;

		//函数
		QComboBox* _functionCombo;

        //超单元参数
        QComboBox* _genericParamCombo;

		//集合
		QHash<QComboBox*, QPair<QList<SetType>, MapRelationType>> _setCombos;

		//所有工况名称
		QStringListModel* _caseModel;

		//工况数据变化信号
		QMetaObject::Connection conn_case;

		//输出控制折叠块
		FoldSection* _outputFoldWidget;

        //PCH输出控制折叠块
        FoldSection* _outputPchFoldWidget;

		//历史输出设置折叠块
		FoldSection* _historyOutputFoldWidget;

		//输出控制树
		QTreeWidget* _outputTree;

		//输出控制的对（键为根节点，值为子节点），只有两层，第一层复选框有三个状态（根据子节点的状态变化），第二层复选框有两个状态
		QHash<QString, QStringList> _outputCheckboxs;

        //不同输出文件类型对应的输出控制对的映射
        QHash<QString, QHash<QString, QStringList>> _typeOfoutputCheckBoxs;

        //不同输出文件类型对应的输出控制对的默认值映射
        QHash<QString, QHash<QString, QStringList>> _typeOfDefault;

		//历史输出控制的对
		QHash<QString, QStringList> _historyOutputCheckboxs;

		//历史输出动刚度结果控制
		QHash<QString, QStringList> _historyKdCheckboxs;

		//历史输出控制控件（供子类访问下拉框）
		HistoryOutputControlWidget* _historyOutputControlWidget = nullptr;

        //输出控制变量树
        OutputControlWidget* controlWidget;
        //PCH输出控制变量树
        OutputControlWidget* pchControlWidget;

		//信号连接
		DataBinder* binder;
		UiController* uiCtrl;

        //输出控制通用拾取控件映射
        shared_ptr<QHash<QString, mPickWidgetController*>> m_pickers;

        //PCH输出控制通用拾取控件映射
        shared_ptr<QHash<QString, mPickWidgetController*>> _pch_pickers;

        //拾取控件寄存容器（面板所有，永久隐藏，防止父级show级联把寄存控件显示成悬浮控件；
        //挂在面板上比输出控制树长寿，工况重建时寄存的视图不会陪葬）
        QPointer<QWidget> m_pickerParkWidget;

        //变量名 -> 集合类型登记表，按需创建拾取控件用（键值对齐上面两个哈希）
        QHash<QString, OutputControlWidget::VariableType> m_pickerVarTypes;
        QHash<QString, OutputControlWidget::VariableType> _pch_pickerVarTypes;
	};
}