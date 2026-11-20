#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mpostwindow_global.h"
#include "mPanelBase.h"
#include "mMechGlobalSignals.h"
#include <QWidget>
#include <QColor>
#include <QListWidgetItem>

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;


class QVBoxLayout;
class QHBoxLayout;
class QListWidget;
class QPushButton;
class QSpinBox;
class QLabel;

namespace mxr
{
    class REBUILD_PostRend;
}

namespace MechMainWindow
{
    class FoldSection;
}

namespace PostWindow
{
    class FbdColorPicker;

    /**
     * @brief 后处理标注面板
     *
     * 用于控制后处理三维视口右上角的信息标注显示。
     * 支持选择需要显示的字段（模型、工况、帧号、时间、文件、模态频率、模态特征值等），
     * 设置字体颜色、字体大小，并将结果实时绘制到当前后处理视口中。
     *
     * 面板结构与 UI_PostNotesPanel.h 解耦，所有控件均在构造函数中直接创建，
     * 与 mPostIsosurfacePanel 等面板保持一致风格。
     */
    class MPOSTWINDOW_EXPORT mPostNotesPanel : public mPanelBase
    {
        Q_OBJECT

    public:
        /**
         * @brief 构造函数
         * @param parent 父窗口
         */
        explicit mPostNotesPanel(QWidget* parent = nullptr);

        /**
         * @brief 析构函数
         *
         * 断开与后处理渲染对象的帧变化信号连接。
         */
        ~mPostNotesPanel();

        /**
         * @brief 初始化/刷新标注面板
         * @param caseId 当前后处理工况 ID
         *
         * 用于面板重新显示时（如切换工况、导入完成后自动显示），
         * 重新建立帧变化监听并应用当前标注设置。
         */
        void newNotesPanel(int caseId);

    public slots:
        /**
         * @brief 语言切换槽
         * @param language 0: 中文, 1: 英文
         */
        void slot_changeLanguage(int language);

        /**
         * @brief 应用标注设置到当前视口
         *
         * 根据当前选中的字段、颜色、字号生成文本并显示在右上角。
         */
        void slot_applyNotes();

        /**
         * @brief 后处理帧变化槽
         * @param frame 当前帧号（未使用，仅用于触发刷新）
         *
         * 当后处理动画播放或云图面板切帧时自动刷新标注内容（如时间、帧号）。
         */
        void slot_frameChanged(int frame);

    private slots:
        /**
         * @brief 全选所有标注字段
         */
        void selectAll();

        /**
         * @brief 全不选所有标注字段
         */
        void deselectAll();

        /**
         * @brief 打开共享取色弹窗（FbdColorPicker）设置字体颜色
         */
        void setFontColor();

        /**
         * @brief 设置字体大小并立即应用
         * @param size 字体大小（像素）
         */
        void setFontSize(int size);

    private:
        /**
         * @brief 色块按钮换色（共享口径，同 mPostTagPanel::applySwatchColor）
         */
        void applySwatchColor(QPushButton* swatch, const QColor& color);
        /**
         * @brief 初始化界面显示状态
         *
         * 设置颜色按钮背景色、字体大小默认值等。
         */
        void initialUI();

        /**
         * @brief 根据语言构建标注字段列表
         * @param language 0: 中文, 1: 英文
         *
         * 在 QListWidget 中生成可勾选的标注项。
         */
        void buildItems(int language);

        /**
         * @brief 获取当前选中的字段项
         * @return 选中项列表
         */
        QList<QListWidgetItem*> getSelectedItems() const;

        /**
         * @brief 组装需要显示的标注文本内容
         * @return 每条标注一行字符串列表
         *
         * 根据当前后处理数据库和帧信息，生成 Model/Case/Frame/Time/File 等字段内容。
         */
        QStringList getNoteContents() const;

        /**
         * @brief 将组装好的标注文本应用到当前视口
         *
         * 调用 mPostFrameText 的接口设置文字、颜色、大小，并触发渲染更新。
         */
        void applyToCurrentViewport();

        /**
         * @brief 获取当前后处理渲染对象
         * @return 当前活动视口的后处理渲染对象指针
         */
        mxr::REBUILD_PostRend* getCurrentPostRend() const;

        /**
         * @brief 连接后处理渲染的帧变化信号
         *
         * 当后处理动画播放切帧时，自动刷新标注显示。
         */
        void connectFrameChanged();

        /**
         * @brief 断开帧变化信号连接
         */
        void disconnectFrameChanged();

        /**
         * @brief 连接云图切帧信号
         *
         * 监听 mMechGlobalSignals::postCloudFrameChanged，使云图面板切帧也能刷新标注。
         */
        void connectPostCloudFrameChanged();

        /**
         * @brief 断开云图切帧信号连接
         */
        void disconnectPostCloudFrameChanged();


        /**
         * @brief 刷新界面文本翻译
         *
         * 根据当前 _language 更新所有控件的中英文显示。
         */
        void retranslateUi();

    private:
        // 主垂直布局
        QVBoxLayout* _mainLayout = nullptr;

        // 标注折叠块
        FoldSection* _noteFold = nullptr;              ///< 标注字段折叠块
        QListWidget* _listWidget = nullptr;            ///< 标注字段列表
        QPushButton* _pushButton_select = nullptr;     ///< 全选按钮
        QPushButton* _pushButton_deselect = nullptr;   ///< 全不选按钮

        // 样式折叠块
        FoldSection* _styleFold = nullptr;               ///< 样式设置折叠块
        QLabel* _label_color = nullptr;                  ///< 字体颜色标签
        QPushButton* _pushButton_color = nullptr;        ///< 字体颜色选择按钮
        QLabel* _label_fontSize = nullptr;               ///< 字体大小标签
        QSpinBox* _spinBox_fontSize = nullptr;           ///< 字体大小输入框
        //【R13】上次已应用的标注文本（内容比对门：播放换帧每帧触发 apply，标注内容
        // 未变时跳过右上角文本实体删建——实测 apply 链 ~4.5ms/帧）
        QStringList _lastAppliedLines;

        // 应用折叠块
        FoldSection* _applyFold = nullptr;               ///< 应用操作折叠块
        QPushButton* _pushButton_ok = nullptr;           ///< 确定/应用按钮

        QColor _color{ 0, 0, 0 };                       ///< 当前字体颜色，默认黑色
        FbdColorPicker* _colorPicker = nullptr;      ///< 共享受色弹窗（懒创建，FbdColorPicker 复用控件）
        int _fontSize = 15;                              ///< 当前字体大小（像素），默认 15
        int _language = 0;                               ///< 当前语言，0 中文 / 1 英文
        int _caseId = -1;                                ///< 当前后处理工况 ID
        QMetaObject::Connection _frameChangedConn;       ///< REBUILD_PostRender 帧变化信号连接
        QMetaObject::Connection _postCloudFrameChangedConn; ///< 云图切帧信号连接
    };
}
