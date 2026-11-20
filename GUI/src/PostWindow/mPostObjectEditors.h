#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mpostwindow_global.h"

#include <QObject>
#include <QPointer>
#include <QString>
#include <functional>

class QCheckBox;
class QPushButton;
class QLabel;
class QWidget;
class QLayout;

namespace PostWindow
{
	/*
	* 后处理对象编辑公共契约（V1.3 原型 window.CAEPostObjectEditors 的 Qt 落地；
	* 爆炸视图/截面剖切/截面力/测量四面板共用；标签面板另批接入——主干同期合入了
	* 注释标签渲染链（pending 占位流），契约需在其之上重做）。
	*
	* - 每面板一个 controller：自建 [即时保存复选 / 取消编辑+保存修改整行等宽双按钮 /
	*   状态行] footer（原型 .post-object-edit-footer），attachFooter 插入面板布局；
	* - dirty 检测：面板提供 snapshot()（编辑缓冲规范化签名）；无修改双钮真实置灰，
	*   改回原值重新置灰（原型 refresh：签名 vs 基线）；
	* - 未保存切换保护：checkSwitch() 在 dirty 时弹三选一确认窗
	*   （继续编辑/放弃并切换/保存并切换；Esc=继续编辑；保存失败保持原对象）；
	* - 即时保存：勾选后隐藏双按钮行，编辑变更自动触发 saveRequested(true)，
	*   失败置状态行"修改尚未保存，请完善有效参数。"；默认关闭；
	* - 删空禁用：endEdit() 后双钮禁用、编辑区（setEditorHost）整体禁用。
	*
	* 面板接线（对照原型 register(kind,{save,cancel,draft,currentId})）：
	*   setSnapshotFn/setSaveFn/setCancelFn/setCurrentNameFn → 面板回调；
	*   beginEdit()=装载当前记录字段完成后取基线；markSaved()=保存/取消恢复成功；
	*   notifyEditChanged()=任意字段变更后调用（内部去抖至事件循环，近似原型 queueMicrotask）。
	*/
	class MPOSTWINDOW_EXPORT mPostObjectEditController : public QObject
	{
		Q_OBJECT

	public:
		explicit mPostObjectEditController(QObject* parent = nullptr);

		/** 创建 footer 并插入 targetLayout（insertRow<0 追加末尾，否则插至该行） */
		void attachFooter(QLayout* targetLayout, int insertRow, QWidget* parentWidget);

		/** 无记录时整体禁用的编辑字段容器（仅禁容器，折叠组标题仍可点） */
		void setEditorHost(QWidget* host);

		void setSnapshotFn(const std::function<QString()>& fn) { _snapshot = fn; }
		void setSaveFn(const std::function<bool(bool immediate)>& fn) { _save = fn; }
		void setCancelFn(const std::function<void()>& fn) { _cancelFn = fn; }
		void setCurrentNameFn(const std::function<QString()>& fn) { _currentName = fn; }

		/** 进入/切换编辑对象：面板装载完当前记录字段后调用（基线=当前快照） */
		void beginEdit();
		/** 记录删空：退出编辑态（双钮禁用、即时保存禁用、编辑区禁用） */
		void endEdit();
		/** 任意字段变更后调用：重算 dirty；即时保存开启且 dirty 时自动保存 */
		void notifyEditChanged();
		/** 保存/取消恢复成功后：基线重取（清 dirty） */
		void markSaved();
		/** 行内快捷改动（如管理表颜色列）不应计入 dirty：基线重取 */
		void rebase();
		/** 切换脏闸：dirty 时弹确认窗；放行/拦截由返回值示知（action 仅在放行时执行） */
		bool checkSwitch(const std::function<void()>& action);

		bool isDirty() const { return _dirty; }
		bool isEditing() const { return _editing; }
		bool immediateSave() const;

		/** 语言切换（false=中文） */
		void retranslate(bool english);

		/** footer 部件（面板需要非 QBoxLayout 布局插入时自取） */
		QWidget* footerWidget() const { return _footer; }

	signals:
		/** 保存修改按钮（immediate=false）/ 即时保存自动保存（true） */
		void saveRequested(bool immediate);
		/** 取消编辑按钮 */
		void cancelRequested();

	private slots:
		void slotApplyPendingRefresh();

	private:
		void refresh();
		void updateStatusText();
		QString snapshot() const;

		QPointer<QWidget> _footer = nullptr;
		QCheckBox* _immediateCheck = nullptr;
		QWidget* _buttonRow = nullptr;
		QPushButton* _cancelBtn = nullptr;
		QPushButton* _submitBtn = nullptr;
		QLabel* _statusLabel = nullptr;
		QPointer<QWidget> _editorHost;

		std::function<QString()> _snapshot;
		std::function<bool(bool)> _save;
		std::function<void()> _cancelFn;
		std::function<QString()> _currentName;

		QString _baseline;
		QString _attempt;          //上次自动保存尝试的签名（防同签名反复尝试）
		bool _editing = false;
		bool _dirty = false;
		bool _bypass = false;      //确认窗放行中（checkSwitch 内 action 不再过闸）
		bool _refreshQueued = false;
		bool _saving = false;
		bool _dialogOpen = false;
		bool _langEn = false;
	};

	/** 原型 uniqueName：基名被占用时追加 _2/_3… */
	MPOSTWINDOW_EXPORT QString postObjectUniqueName(const QString& base, const QStringList& usedNames);
}
