#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mpostwindow_global.h"

//QT
#include <QWidget>
#include <QGridLayout>
#include <QLabel>
#include <QPointer>

#include "mPanelBase.h"
#include "mCalcLineEdit.h"
#include "mxr/view/REBUILD_PostRend.h"
#include "mxr/view/REBUILD_PostRender.h"
#include "mPostAnimationRendData.h"

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;


namespace PostData
{
	class mPostDataBase;
}

namespace MechPreData
{
	class mPostGUIData;
}

namespace mxr
{
	class PostFrameDataProxy;
}

namespace PostWindow
{
	class MPOSTWINDOW_EXPORT mPostAnimationPanel : public mPanelBase
	{
		Q_OBJECT
	public:
		explicit mPostAnimationPanel(QWidget* parent = nullptr);

		void newAniPanel(int caseid);

		//进入动画模式（FR-01/FR-05 单视口形态：viewport=云图视口名，不再创建第二视口）
		void slot_createAnimationViewport(QString viewport, int type);

		//一键生成动画
		bool autoGenerateAnimation();

		//自定义生成动画
		bool customGenerateAnimation(QString ani, QString var, QString start, QString end);

		//更新动画帧数信息显示
		void updateAimationFrameInfo();

		//获取最接近输入时间的帧号
		int getClosestFrame(double time);

		//获取最接近输入的时间间隔的帧间隔数及时间
		QPair<int,double> getClosestIntervalFrameNum(double time);

		//设置当前视图
		void setCurViewName(const QString& viewName);

		/** 进度条显示*/
		void setProgressBarValue(int i, int size);

	private slots:
		//设置起始帧
		void slot_setStartFrame(int f);

		//设置结束帧
		void slot_setEndFrame(int f);

		//设置当前帧
		void slot_setCurrentFrame(int f);

		//设置帧间隔
		void slot_setFrameInterval(int f);

		//计时器槽函数
		void slot_aniTimer(int frame);

		//【FR-07 死代码清理·unify-anim-playback】旧 GUI 定时器直驱 tick 链（定时器成员
		//+超时槽）已删除：定时器全文件无 start 调用=永不执行路径，播放推进由会话播放器
		//playTick 单通道承担（B1b 门面直接替换收官）

		//设置起始帧时间
		void slot_setStartFrameTime();

		//设置结束帧时间
		void slot_setEndFrameTime();

		//设置当前帧时间
		void slot_setCurrentFrameTime();

		//设置帧间隔时间
		void slot_setFrameIntervalTime();

		//删除动画（当前面板视口）
		void slot_deleteAnimate();

	public slots:
		//设置失效单元隐藏
		void slot_setHideFailedElement(bool checked);

		//删除指定视口上的动画（FR-01/FR-05 单视口形态：树侧删除动画入口；
		//stop 回退+状态复位，视口不关闭——视口归云图条目管辖）
		void slot_deleteAnimateFor(const QString& viewname);

		//第一帧
		void slot_firstFrame();

		//上一帧
		void slot_lastFrame();

		//开始/暂停
		void slot_PlayAnimation(bool isrun);

		//下一帧
		void slot_nextFrame();

		//最后一帧
		void slot_endFrame();

		//设置渲染范围
		void slot_setRenderRange(const QString& type);

		//设置循环播放
		void slot_setLoopPlay(bool istrue);

		//设置帧速率
		void slot_setFrameRate(int fr);

		//设置控件状态
		void slot_setStatus(bool isEnable);

		//更新动画参数页界面
		void updateAnimationGUI();

		//获取当前帧
		int getCurrentFrameID();

		//获取起始帧
		int getStartFrameID();

		//获取结束帧
		int getEndFrameID();

	signals:

	private:
		//根据动画类型字符串获取渲染动画模式（切换动画时恢复面板状态用）
		mxr::PostMode postModeFromAniType(const QString& aniType);

		QMetaObject::Connection					m_viewportChangeConn;
		QPointer<mxr::REBUILD_PostRend>			_curPostRend{};				// 当前激活窗口
		QString									_lastViewname{};				//上一次激活视口
		QList<int>								_frameIndexes{};				// 帧索引
		mxr::PostMode							_animationType{};				// 生成的动画类型
		PostData::mPostAnimationRendData*			_animationRendData{};			// 后处理动画数据
		int									_currentFrameId{};				// 当前帧
		int									_startFrameId{};				// 起始帧
		int										_endFrameId{};					// 结束帧
		int										_intervalFrame{ 1 };			// 帧间隔
		int										_rateFrame{ 10 };				// 帧速率
		bool									_animationLoopPlay{ true };		// 是否循环播放
		bool									_animationIsStart{ false };		// 是否启动
		bool									_isSelRange = false;
		bool									_isHideFailedElement{ true };	// 是否隐藏失效单元

		//生成阶段按帧处理失效单元隐藏（供 autoGenerateAnimation/customGenerateAnimation 复用）
		void applyHideFailedElementForFrame(PostData::mPostDataBase* postDB,
			mxr::REBUILD_PostRend* postRend,
			mxr::PostFrameDataProxy* frameDataProxy, int frame,
			const std::set<int>& lastLoseEles);

		//切换动画节点时从guidata恢复面板运行时状态（帧设置、动画类型、帧索引）
		void restoreAnimationState(MechPreData::mPostGUIData* guidata, PostData::mPostDataBase* postDB);
		//【FR-07】旧 GUI 直驱定时器成员已随旧 tick 链删除（unify-anim-playback：创建/
		//连接/stop 兜底全链清理，见上方删除注释）
		int										_frameCountLinear{};			// 线性动画帧的总数
		QMap<int, double>							_frameTimes{};					// 时间、帧号
		bool									_isGenerate{};					// 是否生成
		QHash<QString, bool>						_viewPlayStatus;				// 存储不同视口播放状态
		QHash<QString, QMetaObject::Connection>		_frameChangedConns;			// 各动画视口frameChanged信号连接（防重复连接）
		// S5 门面直接替换：B1b(R17/H1) 过渡别名连接 _aliasTickConns（外壳 updateAnimationPanelSig）
		// 随旧信号删除自然退役，成员移除——帧事件单通道接门面 frameChanged(n, source)


		QVBoxLayout*							_mainLayout;
		QSpinBox*								_spinCurFrame;
		QLineEdit*								_lineEditCurTime;
		QSpinBox*								_spinStartFrame;
		QLineEdit*								_lineEditStartTime;
		QSpinBox*								_spinEndFrame;
		QLineEdit*								_lineEditEndTime;
		QSpinBox*								_spinIntervalFrame;
		QLineEdit*								_lineEditIntervalTime;
		QSpinBox*								_spinFrameNum;

	};
}
