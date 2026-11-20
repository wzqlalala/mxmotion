#pragma once
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include <QWidget>
#include <QToolButton>
#include <QLineEdit>
#include <QLabel>
#include <QGridLayout>
#include <QVector3D>
#include <functional>
#include <unordered_map>

#include "mPickWidgetController.h"
#include "mPickWidgetData.h"
#include "mModelStruct.h"

namespace MxFunctions { class mGroupBox; }

namespace MechMainWindow
{
    class MMAINWINDOW_EXPORT mPointPickerWidget : public QWidget
    {
        Q_OBJECT

    public:
        /// @brief 坐标计算回调类型：输入拾取结果，输出平均坐标点
        using CoordinateCalculator = std::function<QVector3D(const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData> >&)>;

        explicit mPointPickerWidget(const QString& groupTitle = QString(),
            const QString& labelText = QStringLiteral("模型拾取"),
            QWidget* parent = nullptr);

        //兼容重载：旧签名 (labelText, parent)。dev_base 新签名把第二参改为 labelText 后
        //wzq 侧存量调用（坐标/离散场/多体各面板）全部失配，逐点迁移面太大，
        //此处按旧语义映射：groupTitle=旧 labelText，行内标签用默认"模型拾取"
        mPointPickerWidget(const QString& labelText, QWidget* parent)
            : mPointPickerWidget(labelText, QStringLiteral("模型拾取"), parent) {}
        ~mPointPickerWidget() override;

        /// @brief 获取当前坐标（读取输入框值）
        QVector3D coordinate() const;

        /// @brief 设置坐标（回填到输入框）
        void setCoordinate(const QVector3D& coord);

        /// @brief 注入坐标计算回调，传入空函数则保留默认计算器
        void setCoordinateCalculator(CoordinateCalculator calculator);

        /// @brief 获取内部拾取控制器（用于设置允许拾取类型、白名单等）
        mPickWidgetController* pickController() const { return _pickController; }

        /// @brief 获取原始拾取数据（用于持久化保存）
        PickWidgetData pickData() const;

        /// @brief 反显拾取数据（用于恢复状态）
        void setPickData(const PickWidgetData& data);

        /// @brief 设置整体控件启用/禁用
        void setWidgetEnabled(bool enabled);

        /// @brief 显示/隐藏“应用”按钮（快速创建模式下拾取即自动应用，按钮无需手动点）
        void setApplyButtonVisible(bool visible);

        /// @brief 显示/隐藏内置标签（外部已有标签列时隐藏，使拾取控件与外部标签列对齐）
        void setLabelVisible(bool visible);

        /// @brief 设置内置标签文字（运行时可改，如三点/基准轴切换"模型拾取"/"点位置"）
        void setLabelText(const QString& text);

        /// @brief 设置外层分组框标题（透传 QGroupBox::setTitle，运行时可改）
        void setGroupTitle(const QString& title);

        /// @brief 自动应用：拾取坐标落输入框并清理拾取状态（等价点击“应用”按钮）
        void autoApply();

        /// @brief 重置到初始默认状态
        void resetToDefault();

        /// @brief 清除拾取高亮
        void clearAllHighlight();

    signals:
        /// @brief 坐标发生变化时发射（拾取变化、点击应用、输入框编辑完成均触发）
        void coordinateChanged();

        /// @brief 拾取控件激活时发射，用于外部全局互斥管理
        void pickControllerActivated(mPickWidgetController* controller);

        /// @brief 点击“应用”按钮后发射（供外部区分自动应用/手动应用，如坐标系快速创建）
        void manuallyApplied();

        /// @brief 输入框编辑完成(回车)后发射
        void editConfirmed();

    private slots:
        void onApplyClicked();
        void onPickDataChanged();
        void onPickActiveChanged(bool isActive);
        void onEditFinished();

    private:
        void initUI(const QString& groupTitle, const QString& labelText);
        QVector3D computeCoordinateFromController() const;
        void updatePreviewRender();
        void clearPreviewRender();
        void applyPickCoordinate();

        // 档位/主题跟随：行高/标签列宽按 UiMetrics 重发，随 UiPalette/档位自适配
        void applyThemeStyles();

        /// 坐标计算器
        CoordinateCalculator _coordinateCalculator;
        /// 拾取坐标缓存（解决点击应用时拾取失活导致数据丢失）
        QVector3D _cachedCoord = QVector3D(0, 0, 0);

        // UI 控件
        MxFunctions::mGroupBox* _groupBox = nullptr;   // 外层分组框(标题=groupTitle)
        QLabel* _label = nullptr;
        mPickWidgetController* _pickController = nullptr;
        QToolButton* _applyBtn = nullptr;
        QLineEdit* _edit = nullptr;
        QGridLayout* _mainLayout = nullptr;

        /// 拾取是否处于激活状态
        bool _isPickActive = false;
    };
}