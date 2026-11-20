#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 通道映射对话框（校验通道/模板对话框"勾"按钮共用，任务 4.6/4.7/4.8）
//
// 表格五列（文件名称/工况类型/结果类型/对象/分量，设计图12）逐行列出模板通道
// 与新文件的匹配结果；工况类型/结果类型/对象/分量四列=同款常驻下拉（无边框、
// 无下箭头，选即改配）+状态图（状态图用需求供图 @3x：未匹配=红✗ -> 匹配=浅勾
// （绿勾降透明度）-> 确认=绿✓）。工具行：
//   - 显示通道过滤（全部/已匹配/未匹配，三态过滤排查）；
//   - 选择最佳匹配（只对未匹配行重配，精确命中行不动）/ 确认（已匹配行全部
//     转绿勾，不关闭对话框——对齐设计图确认后绿勾仍可见）/ 重置（恢复初始
//     映射=仅精确匹配）；
//   - 保存配置/导入配置（CSV 图标按钮，按曲线名对齐）；
//   - 自动连接相似通道（右端，默认勾选：失配行按相似度预配（档=文件名），
//     取消勾选失配行退回未配）。
// 四列均常驻下拉选择（候选按该文件该层级现算，mChannelMapCore；当前值
// 不在候选时补入回显），选择即改配并即时试装配验证；底部"关闭"=按当前映射继续生成。
//
// 核心算法（匹配/预配/候选/CSV）在 mChannelMapCore（无 GUI 依赖，可无头单测）；
// 本类只做表格呈现与交互。形态：模态 QDialog，无边框白卡片 + M 徽标标题栏
// （对齐模板曲线/图层对话框；需求图形态）。
// =============================================================================

#include "mChannelMapCore.h"

//QT
#include <QDialog>
#include <QHash>
#include <QList>
#include <QPoint>

class QCheckBox;
class QComboBox;
class QMouseEvent;
class QStringListModel;
class QTableWidget;

namespace PostCurve
{
	class POSTCURVE_EXPORT mChannelMapDlg : public QDialog
	{
		Q_OBJECT
	public:
		//构建映射行集并弹出（模态）：从模板+参数提取行、失配行按相似度预配
		//（"自动连接相似通道"默认勾选，预配档归一见实现），
		//确认/关闭返回 true（rows 带回最终映射，调用方据此装配），行提取失败
		//返回 false（弹告警，err 带因可为空）。用户关窗=接受当前映射继续
		static bool resolveMapping(QWidget* parent, const QString& templatePath,
			const QStringList& effectiveParams, int autoFillMode,
			QList<mChannelMapCore::Row>* rows, QString* err = nullptr);

	private:
		explicit mChannelMapDlg(QList<mChannelMapCore::Row> rows, int autoFillMode,
			QWidget* parent = nullptr);

		//--- UI ---
		void buildUi();
		//全行重建 + 当前过滤原位隐藏（批次5 统一模型，design D6）：过滤只影响
		//可见性；仅构造初建/重置/CSV 导入走全量（低频），改配/确认/过滤/勾选
		//切换/最佳匹配均已差量化
		void rebuildTable();
		//单行可见性（当前过滤模式 -> setRowHidden；onFilterChanged/rebuildTable/
		//差量刷新共用）
		void updateRowVisibility(int vr);
		//四列单元格（同款）：常驻下拉（无边框无下箭头；候选走 _candCache 缓存，
		//当前值不在候选时补入回显）+ 状态标识图
		QWidget* makeChoiceCell(int vr, int col, const mChannelMapCore::Row& row,
			const QString& cur);
		//候选项缓存与共享模型（optimize-channel-map-perf）：同 (列,映射上下文)
		//的候选全对话框只现算一次；同上下文下拉共享一个 QStringListModel
		//（N×P 行不再各挂一份 N×P 项模型），cur 不在候选时该 combo 挂私有模型
		QStringList candidatesFor(int col, const ChannelSelection& sel);
		QStringListModel* sharedModelFor(const QString& key, const QStringList& items);
		QString candKey(int col, const ChannelSelection& sel) const;
		//四列下拉统一回填（QSignalBlocker + prepend 回显/失配选首规则同现行）
		void fillCombo(QComboBox* combo, int col, const ChannelSelection& sel,
			const QString& cur);
		//改配后仅重填本行四格（optimize-channel-map-perf 差量刷新：零全表重建）
		void refillRow(int vr);

		//--- 交互 ---
		//四列单元格现值（存档文本在同行 item 上，与单元格控件解耦）
		QString cellValue(int vr, int col) const;
		//菜单改配提交：行映射按单元格现值重建 + 即时存在性探针（confirmed 退回）
		void commitCell(int vr, int col);
		void onFilterChanged(int index);
		void onAutoConnectToggled(bool checked);
		void onBestMatchClicked();
		void onResetClicked();
		void onSaveCsvClicked();
		void onLoadCsvClicked();
		void onConfirmClicked();

		//行 -> 当前映射 selection（按四列单元格现值构造，complete=试装配通过）
		ChannelSelection rowSelection(int row) const;

		//--- 无边框标题栏拖动（对齐 mLayerManageDlg） ---
	protected:
		void mousePressEvent(QMouseEvent* event) override;
		void mouseMoveEvent(QMouseEvent* event) override;
		void mouseReleaseEvent(QMouseEvent* event) override;

	private:
		QList<mChannelMapCore::Row> _rows;        //行集（映射结果就地更新）
		QList<mChannelMapCore::Row> _initRows;    //初始快照（重置恢复）
		int _autoFillMode = 0;                    //应用选项"自动填充"档（需求原图无此
		                                          //UI 恒"无"=文件名档；复选框默认勾选）

		QComboBox* _filterCombo = nullptr;        //显示通道过滤（0全部/1已匹配/2未匹配）
		QCheckBox* _autoConnectCheck = nullptr;   //自动连接相似通道
		QTableWidget* _table = nullptr;           //映射表（文件名称|工况类型|结果类型|对象|分量）

		QWidget* _titleBar = nullptr;             //无边框标题栏（M 徽标+✗，对齐 mLayerManageDlg）
		QPoint _pressPos;                         //标题栏拖动起点
		QPoint _dragOffset;                       //拖动中窗口左上角偏移
		bool _dragging = false;

		//候选/模型缓存（optimize-channel-map-perf）：键见 candKey；模型 parent=
		//对话框，随对话框销毁，无悬挂
		QHash<QString, QStringList> _candCache;
		QHash<QString, QStringListModel*> _modelCache;
	};
}
