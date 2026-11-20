#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mpostwindow_global.h"

#include <QPushButton>
#include <QLabel>
#include <QList>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QPointer>
#include <set>

namespace PostWindow
{
	/*
	* 部件范围选择字段按钮（自 mPostExplodePanel"选择部件"字段同款复刻，对齐前后处理整合原型）：
	* 白底直角字段按钮（文字居中 + 右缘蓝色拾取图标"⊗"），点击弹出范围选择弹窗，
	* 范围选项随对象类型联动（同原型）：单元=当前显示/所有单元/单元集合（3 项），
	* 部件=当前显示/所有部件/通过集合/通过材料/通过属性（5 项）+ 完成拾取，
	* "通过材料"级联二级勾选弹窗。
	* 渲染端未对接：仅界面状态，选择经 scopeChanged 信号交宿主持久化。
	* 弹窗锚定本按钮右缘、与按钮顶对齐（共享折叠组会被等值面面板接管，宿主面板可能
	* 处于隐藏态、几何不可靠，故不作面板级锚定），右侧越界翻转到按钮左侧。
	*/
	class MPOSTWINDOW_EXPORT mPostPartScopeField : public QPushButton
	{
		Q_OBJECT
	public:
		explicit mPostPartScopeField(QWidget* parent = nullptr);

		//当前范围索引（文本见 scopeText；进程内状态，缺省"所有部件/所有单元"）
		int scope() const { return _scope; }
		void setScope(int index);						//程序化设置（越界按类型回退末项，不触发 scopeChanged）

		//二级弹窗已勾选部件号（真实部件 ID，弹窗勾选实时回写、关后再开保持；
		//"通过材料"范围消费，其余范围忽略）
		const std::set<int>& checkedPartIds() const { return _scopeCheckedIds; }

		//程序化回填已勾选部件（编辑缓冲装载/取消恢复用；不发信号，
		//弹窗下次打开按此集合恢复勾选）
		void setCheckedPartIds(const std::set<int>& ids) { _scopeCheckedIds = ids; }

		//对象类型联动（0=单元 3 项范围；1=部件 5 项，缺省）。类型切换后越界范围
		//回退到该类型末项并刷新字段文字/弹窗（弹窗高度随选项数收缩）
		void setObjectType(int type);
		int objectType() const { return _objectType; }

		//按中文基准文本回定位（匹配当前类型表项，未命中回退"所有部件/所有单元"）；用于 guidata 回填
		void setScopeByText(const QString& text);

		//当前范围的中文基准文本（持久化用，随对象类型取对应表项）
		QString scopeText() const;

		//字段文字/弹窗条目随语言刷新（弹窗每次打开前亦自动刷新）
		void refreshLanguage(int lang);

		//范围文本表（objectType 0=单元/1=部件；index 越界按类型回退末项；lang 0中文/1英文）
		static QString scopeText(int objectType, int index, int lang);

		//V1.3：范围弹窗锚定部件（复合行形态下本按钮隐形、自身几何不可靠，弹窗改锚定
		//外置锚点——对象选择复合行；缺省 nullptr=锚定自身，保持旧用法兼容）
		void setAnchorWidget(QWidget* anchor) { if (anchor) _anchor = anchor; }

		//外部触发开关弹窗（V1.3：入口为对象选择复合行 inspect 槽；已开则关闭；
		//Q_INVOKABLE 供 ui-shot 驱动弹窗态截图——postPartScopeField.openPopup()）
		Q_INVOKABLE void openPopup();

	signals:
		void scopeChanged(int index);					//用户点选新范围
		void finishPickRequested();						//完成拾取
		//范围显示文本变化（点选/回填/类型切换/语言；字段隐形后宿主据此驱动复合行拾取钮文案）
		void scopeTextChanged(const QString& text);
		void scopeMaterialsChanged();					//二级弹窗部件勾选变化（宿主 dirty/aux 状态重算）

	protected:
		bool eventFilter(QObject* watched, QEvent* event) override;

	private:
		int optionCount() const { return _objectType == 0 ? 3 : 5; }

		void ensurePopup();
		void ensureScopeListPopup();
		void fillScopeList();
		void applyScope(int index);
		void applyPopupLanguage(int lang);
		void openScopeListPopup();
		void closeScopeListPopup();
		void showPopup();
		//字段文字更新统一出口：写 _fieldText 并发 scopeTextChanged（宿主同步复合行拾取钮文案）
		void updateCaptionText(const QString& text);

	private:
		QLabel*				_fieldText{};				//字段文字（居中）
		QLabel*				_pickIcon{};				//右缘拾取图标（⊗，同原型）
		QWidget*			_anchor{};					//V1.3 弹窗锚定部件（缺省 nullptr=锚定自身）
		//QPointer：ui-shot 弹窗"用后即焚"（deleteLater）后重建不悬空（ensurePopup 幂等重建）
		QPointer<QWidget>			_popup{};					//范围选择弹窗（Qt::Popup，点击外部自动关闭）
		QList<QPushButton*>		_popupItems;				//弹窗范围选项（按对象类型 3/5 项，多余项隐藏）
		QPushButton*			_btnFinishPick{};			//完成拾取
		QPointer<QWidget>			_scopeListPopup{};			//通过材料 二级弹窗（主弹窗右侧级联）
		QLineEdit*			_scopeSearchEdit{};			//二级弹窗搜索框
		QWidget*			_scopeRowsWidget{};			//二级弹窗勾选列表容器
		QVBoxLayout*		_scopeRowsLayout{};
		QList<QCheckBox*>	_scopeRowChecks;
		int					_objectType{ 1 };			//对象类型（0=单元/1=部件，缺省部件）
		int					_scope{ 1 };				//当前范围（1=所有部件/所有单元）
		int					_scopeListScope{ -1 };		//二级弹窗当前范围（3=通过材料，-1未打开）
		std::set<int>		_scopeCheckedIds;			//二级弹窗已勾选部件号（弹窗关闭后保留，再次打开恢复勾选）
	};
}
