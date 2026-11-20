#pragma once
#include "mpostwindow_global.h"
#include <QSize>
#include <QPoint>
#include <QWidget>
#include <QMouseEvent>
#include <QObject>
#include <QPushButton>
#include <QHBoxLayout>
#include <QKeyEvent>

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;


enum ScreenType1
{
    QuadScreen = 0x0001,
    FullScreen = 0x0002,
    OneScreen = 0x0004,
    SoftScreen = 0x0008,
};
enum HitRegion
{
	None,
	Inside,        // 矩形内部 → 移动
	Left,
	Right,
	Top,
	Bottom,
	TopLeft,
	TopRight,
	BottomLeft,
	BottomRight
};

enum RecordType
{
	ShotRecord,
	VideoRecord
};

namespace PostWindow
{

	constexpr int RESIZE_MARGIN = 6; // px，靠近边界多远算 resize
	constexpr int MIN_RECT_SIZE = 10; // 最小宽高，防止翻转/消失
	class mScreen
	{
	public:
		enum STATUS
		{
			SELECT, MOV, SET_W_H
		};
		mScreen() {}
		mScreen(QSize size);
		void setStart(QPoint pos);
		void setEnd(QPoint pos);
		QPoint getStart();
		QPoint getEnd();
		QPoint getLeftUp();
		QPoint getRightDown();
		STATUS getStatus();
		void setStatus(STATUS status);
		int width();
		int height();
		bool isInArea(QPoint pos);							// 检测pos是否在截图区域内
		void move(QPoint p);								// 按 p 移动截图区域
		void resizeLeft(int dx);
		void resizeRight(int dx);
		void resizeTop(int dx);
		void resizeBottom(int dx);
		HitRegion hitTest(const QPoint& pos);
		void resetPos();
	private:
		QPoint			leftUpPos_{}, rightDownPos_{};		//记录 截图区域 左上角、右下角
		QPoint			startPos_{}, endPos_{};				//记录 鼠标开始位置、结束位置
		int				maxWidth_{}, maxHeight_{};            //记录屏幕大小
		STATUS			status_{};							//三个状态: 选择区域、移动区域、设置width height
		void cmpPoint(QPoint& s, QPoint& e);				//比较两位置，判断左上角、右下角
	};

	class ConfirmCancelBar : public QWidget {
		Q_OBJECT
	public:
		QPushButton* btnConfirm;
		QPushButton* btnCancel;
		explicit ConfirmCancelBar(QWidget* parent = nullptr);
	};

	class MPOSTWINDOW_EXPORT mScreenWidget :public QWidget
	{
		Q_OBJECT
	public:
		explicit mScreenWidget(QSize size, QPoint point, RecordType type, QWidget* parent = 0);
		virtual void startScreen(ScreenType1 screenType, QRect rect = QRect());

		void updateCursor(HitRegion region);

		void setStart(QPoint pos);
		void setEnd(QPoint pos);

	protected:
		mScreen* screen_{};				//截屏对象
		QPixmap* fullScreen_{};			//保存全屏图像
		QPixmap* bgScreen_{};				//模糊背景图
		QPoint			movPos_{};				//坐标
		ScreenType1		screenType_{};			//截屏类型
		ConfirmCancelBar* _btnBar = nullptr;		//按钮
		RecordType      _type = ShotRecord;

		HitRegion _activeRegion = HitRegion::None;//区域
		QPoint _pressPos;//鼠标按下的位置
		QPoint _bgPos{};//背景图创建位置
		QPoint _defaultStart, _defaultEnd;//初始值
	signals:
		void sig_sendPos(QPoint startPos, QPoint endPos);
		void sig_saveShot();
		void sig_saveVideo();

	protected:
		void mousePressEvent(QMouseEvent*);
		void mouseMoveEvent(QMouseEvent*);
		void mouseReleaseEvent(QMouseEvent*);
		void paintEvent(QPaintEvent*);
		void showEvent(QShowEvent*);
		void keyPressEvent(QKeyEvent* event) override;

	public slots:
		void slot_saveButton();
		void slot_cancelButton();
	};

	class mAnimationRecordWidget :public QWidget
	{
		Q_OBJECT
	public:
		mAnimationRecordWidget(QSize size, QPoint point, RecordType type, QWidget* parent = 0);
		~mAnimationRecordWidget();
		void getStartAndEndPos(ScreenType1 type, QWidget* parent);
		void closeScreenWidget();

		void setStart(QPoint pos);
		void setEnd(QPoint pos);
		void cancelBtn();

	signals:
		void sig_sendPos(QPoint startPos, QPoint endPos);
		void sig_saveShot();
		void sig_saveVideo();

	private:
		mScreenWidget* screenWidget_{};
	};
}
