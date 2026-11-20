#pragma once
#pragma execution_character_set("utf-8")

// =============================================================================
// 复制自 src/MainWindow/mPickWidgetViewBase.h（前处理拾取控件栈，dev_base@8d278d784）
// 移植改动：类名 mPickWidgetViewBase→mPostPickWidgetViewBase；导出宏换
// MPOSTWINDOW_EXPORT；命名空间 MechMainWindow→PostWindow；数据头换
// mPostPickWidgetData.h；其余（枚举、接口、交互逻辑）保持一致。
// post 侧增补（同步守则豁免项，勿因前处理侧无对应而误删）：
//   - deactivatePick 信号：点击已激活控件本体=请求停止拾取（开关语义；
//     前处理侧靠 checkable 拾取钮/外部互斥承担停止入口，后处理面板无此承载）
// 同步守则：前处理侧对应文件的结构/逻辑调整须对照同步到本文件，不得单侧漂移。
// =============================================================================

#include <QObject>
#include <QWidget>
#include <QLineEdit>
#include <QToolButton>
#include <QListWidget>
#include <QHBoxLayout>
#include <QFrame>
#include <QMouseEvent>
#include <QString>
#include <QPoint>
#include <QLabel>
#include <QList>
#include <QSet>
#include <vector>

#include "mpostwindow_global.h"
#include "mPostPickWidgetData.h"
#include "mRoundTooltip.h"

namespace PostWindow
{
    // CommonWidgets 圆角控件沿用 MechMainWindow 命名空间（PostWindow 库内既有兼容惯例）
    using namespace MechMainWindow;

    // 拾取模式枚举
    enum mPickMode
    {
        PickOne,  // 单选模式
        PickMore, // 多选模式
    };

    // 颜色块枚举
    enum mColorBlock
    {
        CBNone,
        CBPurple, // 紫色
        CBBlue,   // 蓝色
        CBRed,    // 红色
        CBGreen,  // 绿色
        CBOrange, // 橘色
        CBPink,   // 粉色
        CBSilver, // 银色
        CBBrown,  // 棕色
    };

    /**
     * @brief 拾取控件基类
     * 包含：拾取数据管理、主行展示、清空功能、自定义Tooltip、激活/失活交互、单选/多选切换、颜色块、通用弹窗框架
     * 不包含：拾取详情按钮、详情弹窗、集合选择、计算按钮
     */
    class MPOSTWINDOW_EXPORT mPostPickWidgetViewBase : public QWidget
    {
        Q_OBJECT

    public:
        explicit mPostPickWidgetViewBase(
            mPickMode pickMode = PickMore,
            QWidget* parent = nullptr);
        ~mPostPickWidgetViewBase() override;

        QWidget* widget() { return this; }

        // 拾取数据设置
        void setCurrentPickData(const std::vector<PickDisplayItem>& items);
        void setDefultTip(const QString& str, const QString& topStr = QString());

        // 激活状态控制
        void setPickActive(bool active);
        bool isPickActive() const { return _isActive; }

        // 拾取模式动态切换
        virtual void setPickMode(mPickMode pickMode, bool clearWhenSwitch = false);
        mPickMode pickMode() const { return _pickMode; }

        // 自定义圆角Tooltip
        void setRoundTooltipText(const QString& text);
        QString roundTooltipText() const;

        // 获取当前拾取数据
        std::vector<PickDisplayItem> currentPickItems() const;

        virtual void setSelectSetBtnVisible(bool visible) {}

        // 颜色块查询接口
        virtual QColor colorBlockColor() const;
        virtual QString colorBlockKey() const;

        // 数据设置（基类默认实现，解决数据不刷新问题）
        virtual void setCurrentData(const std::vector<PickDisplayItem>& items, const QSet<QString>& names);
        virtual void setSelectSetList(const QHash<QString, QString>& setToType = {}) {}

        // 设置整个控件是否可用（置灰/恢复）
        void setPickWidgetEnabled(bool enabled);

    signals:
        void activatePick();                     // 请求激活拾取
        void deactivatePick();                   // post 增补：请求停止拾取（点击已激活控件本体）
        void removePickRequested(const QList<PickDisplayItem>& items); // 删除指定项
        void removePickRequestedByType(MxFunctions::PickDataType type); // 按类型整删
        void clearAllRequested();                // 清空全部
        void selectSetChanged(const QSet<QString>& selectedNames);
        void calcMainNodeRequested();

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;

        // ---------- 虚函数扩展接口 ----------
        virtual void initUI();                   // UI初始化
        virtual void updateLineEditText();       // 更新主行显示文本
        virtual void refreshDataWidgets();       // 统一刷新数据控件
        virtual void enableAllWidget(bool flag); // 批量设置控件启用状态

        // 弹窗扩展虚函数（基类已提供注册表默认实现，子类可重写）
        virtual bool isPopupCoveringGlobalPos(const QPoint& globalPos) const;
        virtual bool isClickOnOwnPopup(QObject* watched) const;
        virtual void hidePopupsOnExternalClick(const QPoint& globalPos);

        // 数据判空
        virtual bool hasData() const;

        // 显示逻辑扩展：子类重写即可扩展计数和特殊文案，无需重写整段显示逻辑
        virtual size_t getTotalCount() const;
        virtual QString getCustomDisplayText() const;

        // ---------- 公共工具函数 ----------
        void setQListWidgetStyle(QListWidget* listWidget);
        QToolButton* createIconButton(const QString& iconPath, const QString& tipStr);
        bool isMeshCategory(MxFunctions::PickDataType type) const;
        bool isGeoCategory(MxFunctions::PickDataType type) const;
        QString buildDisplayText(const PickDisplayItem& item) const;
        QString buildGroupText(MxFunctions::PickDataType type, size_t count) const;

        // 颜色块设置（子类可调用）
        void setColorBlock(mColorBlock colorBlock);

        // 通用弹窗工具
        void showPopupBelowWidget(QWidget* anchorWidget, QWidget* popup, const QSize& fixedSize);
        void registerPopup(QWidget* popup); // 注册弹窗，基类自动处理外部点击关闭、Tooltip避让

        // 内部状态刷新
        void updateClearBtnVisible();

        QFrame* _frame = nullptr;
        QLineEdit* _lineEdit = nullptr;
        QToolButton* _clearBtn = nullptr;

        // 颜色块成员
        QLabel* _colorBlock = nullptr;
        mColorBlock _currentColorBlock = CBNone;
        QString _colorStr;
        QColor _currentColor;

        mPickMode _pickMode = PickMore;
        bool _isActive = false;

        QString _defaultTip;
        QString _topTip;
        std::vector<PickDisplayItem> _pickItems;

        // 自定义圆角Tooltip
        mRoundTooltip* _roundTooltip = nullptr;
        QString _roundTooltipText;
        bool _roundTooltipVisible = false;

        // 公共样式常量（静态共享，减少内存）
        static const QString lineEditBlackStyle;
        static const QString lineEditBlueStyle;
        static const QString lineEditRedStyle;

    private:
        QList<QWidget*> _registeredPopups; // 弹窗注册表
    };
}
