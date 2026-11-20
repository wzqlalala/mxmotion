#pragma once
#include "mmainwindow_global.h"
#include "mMeshPanel.h"
//解决中文乱码
#pragma execution_character_set("utf-8")

namespace MechMainWindow
{
	//创建网格面板（视口连续拾取节点，凑满当前形状节点数后自动创建单元）
	class MMAINWINDOW_EXPORT mCreateMeshPanel :public mMeshPanel
	{
		Q_OBJECT
	public:

        mCreateMeshPanel(QWidget* parent = Q_NULLPTR);

		~mCreateMeshPanel();
		void newMeshPanel() override;

		//更新面板
		void editMeshPanel(int meshId);
		//多选网格子节点面板
		void getMeshInfoById(const std::vector<int> ids);
		//根据数据初始化面板
		void updatePanel() override;
		//创建（兼容接口；当前面板无确认按钮，拾取凑满后自动创建）
        void apply();

		//当前形状所需节点数与对应单元类型
		int currentNeedCount() const;
		MxFunctions::ElementType currentElementType() const;

		//拾取信号连接/断开（进入面板连接，退出面板断开）
		void connectPick();
		void disconnectPick();
		//清空当前组拾取（视口选择/高亮 + 内部列表）
		void clearCurrentPick();
		//撤销上一次拾取的节点
		void undoLastPick();
		//撤销上一次创建的单元
		void undoLastElement();

	protected:
		void hideEvent(QHideEvent* event) override;

	private slots:
		//视口拾取完成后处理：累积节点，凑满自动创建单元并继续下一组
		void slot_onNodePicked();

	private:
		//按当前累积节点创建单元，成功返回true
		bool createElementFromNodes();
		//新建部件首个单元创建成功后，切换为"加入该已有部件"模式
		void switchToExistingPart(const QString& partName);
		//面板激活期间锁定拾取类型为网格节点（退出面板时解锁）
		void lockPickToNode(bool lock);

	private:
		FoldSection* m_rangeFold = nullptr;
		QLabel* _meshControlLabel;
		QComboBox* _meshControlBox;
		QLineEdit* _meshControlEdit;

		FoldSection* m_GraphFold = nullptr;
		//清空已拾取节点按钮
		QPushButton* _clearBtn = nullptr;
		//撤销上一次拾取按钮
		QPushButton* _undoBtn = nullptr;
		//撤销上一单元按钮
		QPushButton* _undoElemBtn = nullptr;
		//本次会话创建的单元ID栈（新创建的在末尾，用于撤销）
		QVector<int> _createdElementIds;
		//当前组已拾取节点ID，按点击顺序
		QVector<int> _pickedNodeIds;
		//新建部件创建成功后锁定加入的部件名
		QString _lockedPartName;
        QComboBox* _meshShapeBox;

        QMap<int, QString> _imageMap;
	};
}