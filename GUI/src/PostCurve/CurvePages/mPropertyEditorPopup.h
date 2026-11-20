#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 曲线模块 - 浮动属性编辑器（UI设计图03.1）
//
// 图上双击曲线/图例项/轴标题、右键"编辑"时弹出：宽固定 280（高默认 400）的
// 无边框窗口，可拖动标题栏移动、右下角 QSizeGrip 缩放高度，标题栏含关闭按钮。
// 标题栏固定"属性编辑器"（设计图03.1）。
//
// 内容为弹窗专属页面族（mCurveEditorPages，经 mCurvePropPageManager::
// registerEditorPage 注册；未注册类型回退参数栏页面族）的【独立实例】
// （由 mCurvePropPageManager::showEditorPopupAt 创建并 host 进来），
// 两个实例都订阅模型信号，编辑任一处另一处自动同步。
// 弹窗由管理器懒创建并复用；host() 换页时释放旧页面。
// =============================================================================

#include "../PostCurveGlobal.h"

//QT
#include <QWidget>

class QResizeEvent;
class QSizeGrip;
class QVBoxLayout;

namespace PostCurve
{
	class mCurvePropPageBase;

	class POSTCURVE_EXPORT mPropertyEditorPopup : public QWidget
	{
		Q_OBJECT
	public:
		//无边框顶层窗口（Qt::Window | Qt::FramelessWindowHint），默认 resize(280,400)；
		//父窗口由创建方传入（宿主主窗口，见 mCurvePropPageManager::buildEditorPopup）
		explicit mPropertyEditorPopup(QWidget* parent = nullptr);

		//托管属性页（接管所有权；替换并释放旧页面）。page 为空时仅清空内容
		void host(mCurvePropPageBase* page);

	protected:
		//标题栏按下记录按下点 / 移动超阈值进入拖动 / 松手复位拖动态
		//（双击不挪窗，仅用户拖动改位置）
		void mousePressEvent(QMouseEvent* event) override;
		void mouseMoveEvent(QMouseEvent* event) override;
		void mouseReleaseEvent(QMouseEvent* event) override;
		void mouseDoubleClickEvent(QMouseEvent* event) override;
		//右下角缩放手柄贴角跟随（手柄不占布局行，对齐 mApplyStyleDlg）
		void resizeEvent(QResizeEvent* event) override;

	private:
		void buildUi();   //标题栏（固定"属性编辑器"+关闭按钮）+ 内容区 + 右下角缩放手柄

		QWidget* _titleBar = nullptr;        //标题栏（拖动热区，白底+M徽标，设计图03.1）
		QVBoxLayout* _bodyLayout = nullptr;  //内容区布局（host 的页面放这里）
		QSizeGrip* _grip = nullptr;          //右下角缩放手柄（覆盖贴角，不占布局行）
		QPoint _dragOffset;                  //拖动起始偏移
		QPoint _pressPos;                    //标题栏按下点（拖动阈值起算，防双击微动挪窗）
		bool _dragging = false;              //拖动中标志
	};
}
