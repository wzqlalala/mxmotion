#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mpostwindow_global.h"
#include <QWidget>
#include "mPanelBase.h"

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;


class QVBoxLayout;
class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLineEdit;
class QPushButton;
class QSpinBox;

namespace PostWindow
{
    /*
    * 视频对比面板（结构由原 UI/UI_PostCompVideo.h 内联，控件即成员，
    * 与 mPostNotesPanel/mPostDeformationPanel 等面板保持一致风格）：
    * 折叠组"视频设置"（帧速率/循环播放）+ 折叠组"动画同步"（同步开关/时间映射/
    * 时间偏移）+ 折叠组"叠加显示"（视口叠加）+ 折叠组"视频导入"（视频路径行 +
    * 导入/播放按钮）。
    * 动画同步（HyperView video sync 语义）：开启后视频由动画换帧事件驱动
    * （播放/暂停/步进/拖动/循环联动），实现经 mVideoWindow 全局帧通知消费。
    */
    class MPOSTWINDOW_EXPORT mPostCompVideoPanel : public mPanelBase
    {
        Q_OBJECT
    public:
        explicit mPostCompVideoPanel(QWidget* parent = nullptr);

        /** 打开面板（预留） */
        void newCompVideoPanel(int caseId);

        // 视频原始fps就绪（connect mVideoWindow::sigVideoFpsReady——替代原
        // AddFpsReadyFunc 裸 this 回调注册：单例持 bind(this) 在面板销毁后悬空）
        void OnVideoFpsReady(double fps);

    signals:
        void returnSig();

    private slots:
        void slot_changeLanguage(int language);

        void slot_importVideo();
        void slot_playVideo();

        // —— 动画同步 ——
        void slot_syncToggled(bool checked);
        void slot_syncMappingChanged(int index);
        void slot_syncOffsetChanged(double val);
        void slot_playStateChanged(bool playing);

        // —— 视口叠加 ——
        void slot_overlayToggled(bool checked);
        void slot_overlayBgToggled(bool checked);
        void slot_overlayPosChanged(int index);
        void slot_overlayScaleChanged(int val);
        void slot_overlayOpacityChanged(int val);
        void slot_overlayActiveChanged(bool active);   // 视口关闭被动失效时回弹勾选态

        // —— 独立视口（与叠加互斥；格子被外部删除经 sigViewportDisplayChanged 回弹）——
        void slot_viewportDisplayToggled(bool checked);
        void slot_viewportDisplayActiveChanged(bool on);
        void slot_fillViewportToggled(bool checked);

    private:
        void initialUI();
        //叠加组控件级联可用态收口：位置/缩放随叠加开且非背景（背景铺满无效），
        //不透明度与背景钮随叠加开
        void refreshOverlayControls();
        //播放按钮可用态收口：已导入视频 且 未勾同步（动画驱动）且 未勾叠加（控制面在
        //dock 控制条）——三个状态变化点（导入/同步/叠加及被动失效回弹）统一刷新
        void refreshPlayButtonState();

        QVBoxLayout*         _mainLayout{};            // 主布局

        FoldSection*         _circularFold{};          // "视频设置"折叠组
        QSpinBox*            _spinBox_ratenum{};       // 帧速率
        QCheckBox*           _checkBox_loopPlay{};     // 循环播放

        FoldSection*         _syncFold{};              // "动画同步"折叠组
        QCheckBox*           _checkBox_syncAnim{};     // 与动画同步开关
        QComboBox*           _comboBox_syncMapping{};  // 时间映射（0=按时间对齐 1=全程映射）
        QDoubleSpinBox*      _doubleSpinBox_syncOffset{}; // 时间偏移（秒）

        FoldSection*         _overlayFold{};           // "叠加显示"折叠组（HyperView overlay）
        QCheckBox*           _checkBox_overlay{};      // 叠加到视口开关
        QCheckBox*           _checkBox_overlayBg{};    // 作为背景（视频垫底铺满，对比视频本义形态）
        QCheckBox*           _checkBox_viewportDisplay{}; // 独立视口（本体进驻视口矩阵格子，与叠加互斥）
        QCheckBox*           _checkBox_fillViewport{};    // 铺满视口（仅独立视口形态生效，默认铺满）
        QComboBox*           _comboBox_overlayPos{};   // 显示位置（9宫格 0..8）
        QSpinBox*            _spinBox_overlayScale{};  // 画面缩放（占视口百分比）
        QSpinBox*            _spinBox_overlayOpacity{}; // 不透明度（百分比）

        FoldSection*         _importFold{};           // "视频导入"折叠组
        QLineEdit*           _lineEdit_videoPath{};    // 视频路径（只读展示）
        QPushButton*         _pushButton_importVideo{};
        QPushButton*         _pushButton_playVideo{};
        bool                 _videoLoaded = false;     // 导入成功即真（播放按钮可用态判据）
    };
}
