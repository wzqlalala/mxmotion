#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 选择起始页面对话框（叠加/替换模式的作用目标选择，任务 4.2）
//
// 叠加/替换需要既有"模板页面组"作为作用目标：单选列表逐行列出
// mCurveLayerManager::validGroups 的组标签（"模板名 (Page N: 页名)"，
// N=页位置序与视图页序同源），确认=以该组起始页为准对位回放。
// 无组场景不由本对话框呈现——调用方（mTemplateCurveDlg）直接告警终止。
// 形态：模态 QDialog，无边框白卡片 + M 徽标标题栏（对齐其它曲线对话框；需求图形态）。
// =============================================================================

#include "mCurveLayerManager.h"

//QT
#include <QDialog>
#include <QList>
#include <QPoint>

class QListWidget;
class QMouseEvent;

namespace PostCurve
{
	class POSTCURVE_EXPORT mTemplateStartPageDlg : public QDialog
	{
		Q_OBJECT
	public:
		//模态选择起始页面组：单选列表展示组标签（默认选首行），确认返回所选
		//组所属层 id，关闭/取消返回 -1（groups 为空直接返回 -1，不弹窗）
		static int pickGroup(QWidget* parent,
			const QList<mCurveLayerManager::GroupInfo>& groups);

	private:
		explicit mTemplateStartPageDlg(
			const QList<mCurveLayerManager::GroupInfo>& groups,
			QWidget* parent = nullptr);

		QList<mCurveLayerManager::GroupInfo> _groups;
		QListWidget* _list = nullptr;

		//无边框标题栏（M 徽标+✗）拖动，对齐 mLayerManageDlg
		QWidget* _titleBar = nullptr;
		QPoint _pressPos;
		QPoint _dragOffset;
		bool _dragging = false;

	protected:
		void mousePressEvent(QMouseEvent* event) override;
		void mouseMoveEvent(QMouseEvent* event) override;
		void mouseReleaseEvent(QMouseEvent* event) override;
	};
}
