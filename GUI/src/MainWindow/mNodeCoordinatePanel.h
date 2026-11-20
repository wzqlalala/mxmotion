#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include "mPanelBase.h"

#include <QWidget>
#include <QComboBox>
#include <QTableWidget>
#include <QStyledItemDelegate>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <memory>
#include <map>
#include <utility>

// 拾取数据类型与结果数据结构
#include "mPhysicalFieldEnum.h"
#include "mModelStruct.h"
#include "mBasicEnum.h"
#include "mOtherFunction.h"

namespace MechMainWindow
{
	class mNodeCoordinateTableWidget;

	struct MMAINWINDOW_EXPORT NCNodePickWidget
	{
		QLabel* labelname{ nullptr };
		QCheckBox* labelpicked{ nullptr };
		QPushButton* btnok{ nullptr };
		QPushButton* btnclear{ nullptr };
		QPushButton* btnadd{ nullptr };
		QHBoxLayout* layout{ nullptr };
		QWidget* propwid{ nullptr };
		std::vector<int> nodeIds;
		std::vector<std::pair<MxFunctions::PickDataType, RenderPickResultData>> pickedData;

		void initWidget(QWidget* parent, const QString& name, bool hasadd = false)
		{
			labelname = new PanelLabel(parent);
			labelpicked = new QCheckBox(parent);
			btnok = new QPushButton(parent);
			btnclear = new QPushButton(parent);
			btnadd = new QPushButton(parent);
			layout = new QHBoxLayout();
			propwid = new QWidget(parent);

			labelname->setText(name);
			labelpicked->setText("当前未选择");
			labelpicked->setMinimumWidth(100);
			btnok->setFixedWidth(30);
			btnclear->setFixedWidth(30);
			btnadd->setFixedWidth(30);
			btnok->setToolTip("选择节点");
			btnok->setIcon(QIcon(":/mMainWidgets/Icon/tihuan3x.png"));
			btnclear->setToolTip("清除选择");
			btnclear->setIcon(QIcon(":/mMainWidgets/Icon/shanchu3x.png"));
			btnadd->setToolTip("添加节点");
			btnadd->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));

			layout->setMargin(0);
			layout->addWidget(labelname);
			layout->addWidget(labelpicked);
			layout->addSpacing(3);
			layout->addWidget(btnadd);
			layout->addWidget(btnok);
			layout->addWidget(btnclear);
			btnadd->setVisible(hasadd);
			//不设整行最小宽度：行内控件随参数栏自然收缩，
			//最小宽度由参数栏 dock 统一兜底，防止窄栏下右侧被裁切
			propwid->setContentsMargins(0, 0, 0, 0);
			propwid->setLayout(layout);

			setCheckedState(false);
			labelpicked->setChecked(false);
			labelpicked->setDisabled(true);
		}

		void setCheckedState(bool checked)
		{
			if (!labelpicked)
				return;
			if (checked)
			{
				labelpicked->setStyleSheet(R"(
					QCheckBox {
						border: 1px solid #1385F6;
						border-radius: 4px;
						color: #1385F6;
						padding-left: 7px;
					}
				)");
			}
			else
			{
				labelpicked->setStyleSheet(R"(
					QCheckBox {
						border: 1px solid #DCDFE6;
						border-radius: 4px;
						color: #303133;
						padding-left: 7px;
					}
				)");
			}
		}

		void setState(bool picked, bool checkBoxChanged = true)
		{
			if (!labelpicked)
				return;
			if (nodeIds.empty())
			{
				picked = false;
			}

			if (picked)
			{
				labelpicked->setText("已选择" + MxFunctions::mNumberToString(nodeIds.size()) + "个节点");
				labelpicked->setDisabled(false);
				if (checkBoxChanged)
				{
					labelpicked->setChecked(true);
				}
			}
			else
			{
				labelpicked->setText("当前未选择");
				labelpicked->setDisabled(true);
				labelpicked->setChecked(false);
				nodeIds.clear();
				pickedData.clear();
			}
			setCheckedState(labelpicked->isChecked());
		}

		void clear()
		{
			nodeIds.clear();
			pickedData.clear();
			if (labelname) { delete labelname; labelname = nullptr; }
			if (labelpicked) { delete labelpicked; labelpicked = nullptr; }
			if (btnok) { delete btnok; btnok = nullptr; }
			if (btnclear) { delete btnclear; btnclear = nullptr; }
			if (btnadd) { delete btnadd; btnadd = nullptr; }
			if (layout) { delete layout; layout = nullptr; }
			if (propwid) { delete propwid; propwid = nullptr; }
		}
	};

	struct MMAINWINDOW_EXPORT NCComboWidget
	{
		QLabel* labelname{ nullptr };
		QComboBox* combobox{ nullptr };
		QHBoxLayout* layout{ nullptr };
		QWidget* propwid{ nullptr };

		void initWidget(const QString& name)
		{
			combobox = new QComboBox();
			combobox->setView(new QListView);
			labelname = new PanelLabel();
			layout = new QHBoxLayout();
			propwid = new QWidget();
			combobox->setFixedHeight(30);
			labelname->setText(name);
			layout->setMargin(0);
			layout->addWidget(labelname);
			layout->addWidget(combobox);
			//不设整行最小宽度：行内控件随参数栏自然收缩，
			//最小宽度由参数栏 dock 统一兜底，防止窄栏下右侧被裁切
			propwid->setWindowFlags(Qt::Widget);
			propwid->setContentsMargins(0, 0, 0, 0);
			propwid->setLayout(layout);
		}

		void clear()
		{
			if (labelname) { delete labelname; labelname = nullptr; }
			if (combobox) { delete combobox; combobox = nullptr; }
			if (layout) { delete layout; layout = nullptr; }
			if (propwid) { delete propwid; propwid = nullptr; }
		}
	};

	class MMAINWINDOW_EXPORT mNodeCoordinatePanel : public mPanelBase
	{
		Q_OBJECT
	public:
		explicit mNodeCoordinatePanel(QWidget* parent = nullptr);
		~mNodeCoordinatePanel();

		// 新建节点坐标系设置
		void newNodeCoordinatePanel();

		// 取消编辑
		void uneditNodeCoordinatePanel();

	private:
		void initUi();
		void refreshComboBoxes();
		void restoreNodePickFromManager();

		void switchMode(int modeIndex);
		void updateModeWidgets(int modeIndex);

		void onOkClicked();
		void onResetClicked();
		void onEditTableClicked();
		void onClearTableClicked();
		void openNodeCoordinateTable();
		void onTableItemChanged();

		void onPickNode();
		void onAddNodePick();
		void onClearNodePick();
		void onHighlightPicked(int state);

		void highlightPickedNodes(bool highlight);
		void updateNodePickFromSelected(bool append);
		void pickNodesToTable(bool append);

	private:
		// 折叠区
		FoldSection* _foldSection{ nullptr };

		// 节点拾取
		NCNodePickWidget* _nodePick{ nullptr };

		// 设置方式
		NCComboWidget _modeCombo;

		// 统一设置：参考/分析坐标系
		NCComboWidget _refCombo;
		NCComboWidget _anaCombo;

		// 按钮
		QPushButton* _btnOk{ nullptr };
		QPushButton* _btnReset{ nullptr };      // 统一设置模式：重置
		QPushButton* _btnResetSep{ nullptr };   // 分别设置模式：重置
		QPushButton* _btnEditTable{ nullptr };

		// 按钮行容器（切换模式时复用，避免重复 addRow 同一按钮）
		QWidget* _btnUnifiedWidget{ nullptr };
		QWidget* _btnSeparateWidget{ nullptr };

		// 当前模式：0=统一设置，1=分别设置
		int _currentMode{ 0 };

		// 自定义表格
		std::unique_ptr<mNodeCoordinateTableWidget> _currentTable;
	};

	// 自定义节点坐标系表格：第一列节点ID，第二列节点坐标，第三列参考坐标系，第四列分析坐标系
	class MMAINWINDOW_EXPORT mNodeCoordinateTableWidget : public QTableWidget
	{
		Q_OBJECT
	public:
		explicit mNodeCoordinateTableWidget(QWidget* parent = nullptr);

		void setCoordinateNames(const QStringList& names);
		void setNodeData(const std::map<int, std::pair<QString, QString>>& data);
		std::map<int, std::pair<QString, QString>> getNodeData() const;

		// 批量设置当前列所有选中单元格
		void batchSetColumn(int col, const QString& value);

	protected:
		void mousePressEvent(QMouseEvent* event) override;
		void mouseReleaseEvent(QMouseEvent* event) override;
		void mouseDoubleClickEvent(QMouseEvent* event) override;

	private:
		void initUi();

	private:
		QStringList _coordinateNames;
		QModelIndex m_lastPressedIndex;
	};

	// 节点坐标系下拉框委托：每次只创建一个 QComboBox
	class MMAINWINDOW_EXPORT mNodeCoordinateComboDelegate : public QStyledItemDelegate
	{
		Q_OBJECT
	public:
		using BatchCallback = std::function<void(int, const QString&)>;

		explicit mNodeCoordinateComboDelegate(const QStringList& items, BatchCallback callback = nullptr, QObject* parent = nullptr);

		QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option,
			const QModelIndex& index) const override;
		void setEditorData(QWidget* editor, const QModelIndex& index) const override;
		void setModelData(QWidget* editor, QAbstractItemModel* model,
			const QModelIndex& index) const override;

	private:
		QStringList _items;
		BatchCallback _callback;
	};
}
