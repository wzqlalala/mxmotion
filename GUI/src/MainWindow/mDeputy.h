#pragma once
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include <QLabel>
#include <QPushButton>
#include <QObject>
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QComboBox>
#include <QListView>
#include <QCheckBox>
#include <QSpinBox>
#include <QHeaderView>
#include <QFrame>
#include <QTableWidget>
#include <QListWidget>
#include <QToolButton>
#include <QGridLayout>
#include <QCompleter>
#include <QStringListModel>
#include <QPointer>
#include <memory>

#include "mCalcLineEdit.h"
#include "mPanelBase.h"
#include "mMaterialPropBase.h"

//MxFunction
#include "mPhysicalFieldEnum.h"
#include "mElementFunction.h"
#include "mModelStruct.h"

//Data
#include "mDeputyBase.h"
#include "mConnectingDeputy.h"
#include "mConnectorFastener.h"
#include "mDeputyManager.h"

#include "mConnector.h"

namespace MechMainWindow
{
	/*************************************************连接副面板管理器***************************************************************/
	class mDeputyPanelBase;
	class mDeputyConnectorPanel;
	class mDeputyFastenerPanel;
	class MMAINWINDOW_EXPORT mDeputyPanelManager : public QObject
	{
		Q_OBJECT

	public:
		~mDeputyPanelManager();

		/*
		* 获取单例指针
		*/
		static mDeputyPanelManager* getInstance();

		/*
		* 根据类型获取连接副控件
		*/
		mDeputyPanelBase* getPanelByType(MxFunctions::DeputyType type);

	private:
		mDeputyPanelManager(QObject* parent = nullptr);

	private:
		//单例指针
		static mDeputyPanelManager* _instance;

		mDeputyConnectorPanel* m_DeputyConnector = nullptr;
		mDeputyFastenerPanel* m_DeputyFastener = nullptr;
	};

	/*************************************************连接副基类***************************************************************/
	class MMAINWINDOW_EXPORT mDeputyPanelBase : public mPanelBase
	{
		Q_OBJECT

	public:
		/*
		* 构造函数
		*/
		mDeputyPanelBase(QWidget* parent = nullptr);

		/*
		* 析构函数
		*/
		~mDeputyPanelBase();

		/*
		* 初始化控件
		*/
		virtual void initInputs() = 0;

		/*
		* 控件设值
		*/
		virtual void setInputsValue(std::shared_ptr<MechConnectorData::mDeputyBase> deputyData) = 0;

		/*
		* 存值
		*/
		virtual void storeDeputyValue(std::shared_ptr<MechConnectorData::mDeputyBase> deputyData) = 0;

		/*
		* 清空控件
		*/
		virtual void clearInputs() = 0;

		/*
		* 连接拾取信号
		*/
		void connectPickSig();

		/*
		* 断开拾取信号
		*/
		void disconnectPickSig();

		/*
		* 清空pick
		*/
		void clearPick();

		/*
		* 获取拾取矢量数据
		*/
		bool getPickVecData(QVector3D& point);

		/*
		* 获取拾取的点数据
		*/
		bool getPickPointData(QVector3D& point);

		/*
		* 设置连接副数据
		*/
		void setDeputyData(std::shared_ptr<MechConnectorData::mDeputyBase> deputyData);

		/*
		* 目标枚举转字符串
		*/
		QString PickDataEnum2Str(MxFunctions::PickDataType type);

		/*
		* 计算主节点
		*/
		int calculateMasterNode(const std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>>& pickedData);

		/*
		* 临时节点渲染
		*/
		virtual void rendFreeNodes(bool showhide) {};

		/*
		* 将选取对象转为临时节点 形心 or 多点等距
		*/
		QVector<QVector3D> getNodesCenter(std::vector<std::pair<MxFunctions::PickDataType, int>>& nodes_id);
		void transPickedToNode(std::vector<std::pair<MxFunctions::PickDataType, int>>& nodes_id);
		QVector<QVector3D> transPickedToNodes(const std::vector<std::pair<MxFunctions::PickDataType, int>>& nodes_id);

		/*
		* 获取选择对象中的节点
		*/
		void getNodesFromPicked(std::vector<std::pair<MxFunctions::PickDataType, int>>& nodes_id);

		/*
		* 过滤还存在的拾取对象
		*/
		void pickedObjFilter(PosPickWidget& pickwid);

		/*
		* 表格信号
		*/
		virtual void setConnectTbaleSig(bool on) {};

		/*
		* 高亮选择
		*/
		void highlightPicked(const std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>>& nodes_, bool hl);

		/*
		* 获取作用对象
		*/
		virtual std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>> getPickedData()
		{
			return std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>>();
		};

	signals:
		/*
		* 编辑完成
		*/
		void sg_editFinished();

		/*
		* 拾取完成
		*/
		void sg_pickFinished();

		/*
		* 连接器面板专用拾取完成信号
		*/
		void sg_connectorPickFinished();

	private slots:
		/*
		* 智能拾取完毕
		*/
		void slot_finishedPick();

	protected:

		std::shared_ptr<MechConnectorData::mDeputyBase> _deputyData;

		std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>> _pickeddata;

		QString defaultStyle =
			"QPushButton {"
			"background-color: #FFFFFF;"
			"border: 1px solid #DCDFE6;"
			"color: #303133;"
			"padding: 4px;"
			"}"
			"QPushButton:hover {"
			"background-color: #E7F3FE;"
			"border: 1px solid #B8DAFC;"
			"color: #1385F6;"
			"}"
			"QPushButton:pressed {"
			"background-color: #E7F3FE;"
			"border: 1px solid #1385F6;"
			"color: #1385F6;"
			"}"
			"QPushButton:disabled {"
			"background-color: #F5F7FA;"
			"border: 1px solid #DCDFE6;"
			"color: #A8ABB2;"
			"}";
		QString checkedStyle =
			"QPushButton {"
			"background-color: #E7F3FE;"
			"border: 1px solid #1385F6;"
			"color: #1385F6;"
			"padding: 4px;"
			"}"
			"QPushButton:hover {"
			"background-color: #1385F6;"
			"border: 1px solid #1385F6;"
			"color: #FFFFFF;"
			"}"
			"QPushButton:pressed {"
			"background-color: #1178DE;"
			"border: 1px solid #DCDFE6;"
			"color: #FFFFFF;"
			"}"
			"QPushButton:disabled {"
			"background-color: #F5F7FA;"
			"border: 1px solid #DCDFE6;"
			"color: #A8ABB2;"
			"}";
	};

	/*************************************************连接副-连接器***************************************************************/
	class MMAINWINDOW_EXPORT mDeputyConnectorPanel : public mDeputyPanelBase
	{
		Q_OBJECT

	public:
		/*
		* 构造函数
		*/
		mDeputyConnectorPanel(QWidget* parent = nullptr);

		/*
		* 析构函数
		*/
		~mDeputyConnectorPanel();

		/*
		* 初始化输入控件
		*/
		void initInputs()override;

		/*
		* 清除输入控件
		*/
		void clearInputs()override;

		/*
		* 控件设值
		*/
		void setInputsValue(std::shared_ptr<MechConnectorData::mDeputyBase> deputyData)override;

		/*
		* 位置输入控件（定义折叠区内容）
		*/
		QVector<QWidget*> getPosInputs(FoldSection* fapos);

		/*
		* 参数输入控件（参数折叠区内容）
		*/
		QVector<QWidget*> getValInputs(FoldSection* faval);

		/*
		* 存值
		*/
		void storeDeputyValue(std::shared_ptr<MechConnectorData::mDeputyBase> deputyData)override;

	private slots:

		/*
		* 类型下拉框变化
		*/
		void slot_typeChanged(int i);

		/*
		* 子类型下拉框变化
		*/
		void slot_subTypeChanged(int i);

		/*
		* 坐标系
		*/
		void slot_coorChanged(int i);

		/*
		* 新建坐标系
		*/
		void slot_createCoor();

		/*
		* 表格新增行
		*/
		void slot_addRow();

		/*
		* 表格删除行
		*/
		void slot_delRow();

		/*
		* 确定当前行：若移动位置为空，则自动创建临时自由节点
		*/
		void slot_confirmRow();

		/*
		* 表格单元格点击（参考/移动位置列触发拾取）
		*/
		void slot_cellClicked(int row, int col);

		/*
		* 移动位置单元格内容变化
		*/
		void slot_movCellChanged(int row, int col);

		/*
		* 连接器面板专用拾取完成槽
		*/
		void slot_pickFinished();

	private:

		/*
		* 更新子类型控件显隐
		*/
		void updateSubTypeVisible();

		/*
		* 验证节点ID是否存在于模型中
		*/
		bool validateNodeId(int nodeId);

		/*
		* 为指定行创建临时自由节点（复制参考位置节点坐标）
		*/
		int createTempFreeNodeForRow(int row);

		/*
		* 刷新表格序号列
		*/
		void refreshTableIndex();

		/*
		* 显示指定行的预览点（参考点+移动点）
		*/
		void showRowPreview(int row);

		/*
		* 清除预览点
		*/
		void clearPreview();

		/*
		* 获取节点坐标
		*/
		bool getNodePosition(int nodeId, QVector3D& pos);

	public:
		/*
		* 重写：显示/隐藏临时自由节点
		*/
		void rendFreeNodes(bool showhide) override;

	private:
		FoldSection* fapos_;
		FoldSection* faval_;

		// 定义区控件
		QTableWidget* table_;
		QPushButton* btnAddRow_;
		QPushButton* btnDelRow_;
		QPushButton* btnConfirm_;

		// 参数区控件
		ComboWidget typeCombo_;
		ComboWidget subTypeCombo_;
		comboBoxWithPushBtn combocoor_;

		// 类型列表
		QStringList typeList_;
		QStringList subTypeList_;

		// 表格数据缓存（与数据模型对应）
		QVector<int> refNodeIds_;
		QVector<int> movNodeIds_;

		// 当前正在拾取pick的行号，-1表示未拾取
		int pickingRow_;
		// 当前正在拾取pick的列号，-1表示未拾取（1=参考位置, 2=移动位置）
		int pickingCol_;
	};

	/*************************************************焊接点（Fastener）***************************************************************/
	class MMAINWINDOW_EXPORT mDeputyFastenerPanel : public mDeputyPanelBase
	{
		Q_OBJECT

	public:
		/*
		* 构造函数
		*/
		mDeputyFastenerPanel(QWidget* parent = nullptr);

		/*
		* 析构函数
		*/
		~mDeputyFastenerPanel();

		/*
		* 初始化输入控件
		*/
		void initInputs()override;

		/*
		* 清除输入控件
		*/
		void clearInputs()override;

		/*
		* 控件设值-刷新界面数值
		*/
		void setInputsValue(std::shared_ptr<MechConnectorData::mDeputyBase> deputyData)override;

		/*
		* 焊接点定义
		*/
		QVector<QWidget*> getFastenerDefine(FoldSection* value);

		/*
		* 连接部件
		*/
		QVector<QWidget*> getConnectParts(FoldSection* value);

		/*
		* 属性
		*/
		QVector<QWidget*> getProperties(FoldSection* value);

		/*
		* 存值
		*/
		void storeDeputyValue(std::shared_ptr<MechConnectorData::mDeputyBase> deputyData)override;

	private slots:

		/*
		* 类型下拉框变化（节点定义 / 连接器部件定义）
		*/
		void slot_defineTypeChanged(int idx);

		/*
		* 拾取节点
		*/
		void slot_pickNode();

		/*
		* 清空节点拾取
		*/
		void slot_clearNodePick();

		/*
		* 连接器部件下拉框变化
		*/
		void slot_deputyChanged(int idx);

		/*
		* 添加部件到列表
		*/
		void slot_addPart();

		/*
		* 移除选中部件
		*/
		void slot_removePart();

		/*
		* 清空部件列表
		*/
		void slot_clearParts();

		/*
		* 部件列表选中变化
		*/
		void slot_partSelectionChanged();

		/*
		* 影响半径变化
		*/
		void slot_radiusChanged();

	private:

		/*
		* 获取节点坐标
		*/
		bool getNodePosition(int nodeId, QVector3D& pos);

		FoldSection* fastenerDefine_;//焊接点定义
		FoldSection* connectParts_;//连接部件
		FoldSection* properties_;//属性

		// 焊接点定义区
		ComboWidget defineTypeCombo_;       // 类型下拉框
		PosPickWidget nodePick_;            // 节点拾取（节点定义时显示）
		ComboWidget deputyCombo_;           // 连接副下拉框（连接器部件定义时显示）

		// 连接部件区
		QListWidget* partListWidget_;       // 部件列表
		QWidget* partListContainer_;
        QToolButton* addPartBtn_;
        QToolButton* removePartBtn_;
        QToolButton* clearPartBtn_;

		// 属性区
		InputWidget radiusInput_;           // 影响半径

		QStringList partNamesCache_;        // 部件名称缓存
	};
}
