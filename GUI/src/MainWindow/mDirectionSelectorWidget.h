#pragma once
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QToolButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QVariant>
#include <QVector3D>
#include <QStackedWidget>
#include <functional>
#include <vector>

#include "mPanelBase.h"
#include "mPickWidgetData.h"
#include "mModelStruct.h"
#include "mPickWidgetController.h"
#include "mCalcLineEdit.h"

struct LableAndLineEdit
{
    QLabel* labelname = nullptr;
    mCalcLineEdit* lineeditvalue = nullptr;
    QLabel* labelunit = nullptr;
    QHBoxLayout* layout = nullptr;
    QWidget* propwid = nullptr;
    Unit unit_;
    QString name_;
    void setDisableAll(bool on)
    {
        labelname->setDisabled(on);
        lineeditvalue->setDisabled(on);
        labelunit->setDisabled(on);
        propwid->setDisabled(on);
    }
    void initWidget(const QString& name, bool hasunit = false, mCalcLineEdit::CalcMode mode = mCalcLineEdit::CalcMode::Standard)
    {
        name_ = name;
        labelname = new PanelLabel;
        lineeditvalue = new mCalcLineEdit(mode);
        labelunit = new QLabel();
        layout = new QHBoxLayout();
        propwid = new QWidget();
        labelname->setText(name);
        layout->setMargin(0);
        layout->addWidget(labelname);
        layout->addWidget(lineeditvalue);
        if (hasunit)
            layout->addWidget(labelunit);
        propwid->setWindowFlags(Qt::Widget);
        propwid->setContentsMargins(0, 0, 0, 0);
        propwid->setLayout(layout);
    }
    void clear()
    {
        if (labelname)
        {
            delete labelname;
            labelname = nullptr;
        }
        if (lineeditvalue)
        {
            delete lineeditvalue;
            lineeditvalue = nullptr;
        }
        if (labelunit)
        {
            delete labelunit;
            labelunit = nullptr;
        }
        if (layout)
        {
            delete layout;
            layout = nullptr;
        }
        if (propwid)
        {
            delete propwid;
            propwid = nullptr;
        }
    }
};

namespace MxFunctions { class mGroupBox; }

namespace MechMainWindow
{
    //通用方向细线框（实现见 mDirectionSelectorWidget.cpp）
    class mDirectionFrame;

    /**
     * @brief 方向选择复合控件
     *
     * 结构：
     *   [Label] [方向选择下拉框] [反向按钮]
     *   [动态内容面板：默认/X/Y/Z 无 | 自定义坐标 | 选择对象 | 直接输入]
     *   [Label] [大小输入框(LableAndLineEdit)]  （可构造时隐藏）
     *
     * 行为：
     *   - 默认方向：控件未启用，getDirection() 返回无效 QVariant。
     *   - X/Y/Z轴：返回 (1,0,0) / (0,1,0) / (0,0,1)，可点击反向按钮取反。
     *   - 自定义坐标：起点/终点各一行（Label + 拾取控件 + 应用按钮），
     *     点击“应用”后计算起点/终点坐标并回填到各自输入框；方向 = 终点 - 起点。
     *   - 选择对象：一行（Label + 拾取控件），无输入框/应用按钮，
     *     拾取变化后自动由外部回调计算方向。
     *   - 直接输入：一行（Label + 输入框）。
     *
     * 外部依赖：
     *   - 通过 setDirectionCalculator 注入“根据拾取数据计算方向矢量”的回调。
     *   - 自定义坐标/选择对象模式均使用此回调。
     */
    class MMAINWINDOW_EXPORT mDirectionSelectorWidget : public QWidget
    {
        Q_OBJECT

    public:
        /**
         * @brief 方向选择模式
         */
        enum DirectionMode
        {
            DefaultDirection = 0, ///< 默认方向（未启用）
            XAxis,                ///< X轴
            YAxis,                ///< Y轴
            ZAxis,                ///< Z轴
            CustomCoordinate,     ///< 自定义坐标
            SelectObject,         ///< 选择对象
            DirectInput           ///< 直接输入
        };

        /**
         * @brief 构造函数
         * @param labelText 第一行左侧标签文本（如“方向”）
         * @param showMagnitude 是否显示底部“大小”输入行
         * @param magnitudeLabel 底部大小输入框的标签文本（如“大小”）
         * @param hasUnit 是否显示单位标签
         * @param unitText 单位文本（如 "N"）
         * @param parent 父控件
         * @param objectPickColorBlock “选择对象”模式拾取控件的颜色块，
         *        同时决定该拾取的正式/临时高亮颜色。同一面板存在多个方向控件时
         *        应传入不同颜色，便于区分各自的拾取对象
         */
        explicit mDirectionSelectorWidget(
            const QString& labelText = QStringLiteral("方向"),
            bool showMagnitude = true,
            const QString& magnitudeLabel = QStringLiteral("大小"),
            bool hasUnit = false,
            const QString& unitText = QString(),
            bool hasDefaultDirect = false,
            QWidget* parent = nullptr,
            mColorBlock objectPickColorBlock = CBRed);

        ~mDirectionSelectorWidget();

        /**
         * @brief 设置/获取当前方向模式
         */
        DirectionMode directionMode() const;
        void setDirectionMode(DirectionMode mode);

        /// 设置外层分组框标题（透传 QGroupBox::setTitle，运行时可改）
        void setGroupTitle(const QString& title);

        // 运行时显隐整个拾取控件
        void setPickWidgetVisible(bool visible);

        /**
         * @brief 设置方向计算回调
         * @param calculator 输入为按 PickDataType 分组的拾取结果，输出为方向矢量
         *
         * 在 选择对象 模式下使用：将单个拾取对象转换为方向。
         */
        using DirectionCalculator = std::function<QVector3D(const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData> >&)>;
        void setDirectionCalculator(DirectionCalculator calculator);

        /**
         * @brief 设置坐标计算回调
         * @param calculator 输入为按 PickDataType 分组的拾取结果，输出为平均坐标
         *
         * 在 自定义坐标 模式下使用：将一组点/线/面/节点/实体计算为一个坐标点。
         * 规则：
         *   - 多个点/节点：直接取平均坐标。
         *   - 多根线：先取每条线中点，再平均。
         *   - 多个面/体：取面上所有离散点或体上所有离散点再平均。
         */
        using CoordinateCalculator = std::function<QVector3D(const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData> >&)>;
        void setCoordinateCalculator(CoordinateCalculator calculator);

        /**
         * @brief 获取当前方向矢量
         * @return 默认方向返回无效 QVariant；其余模式返回 QVector3D（已考虑反向）
         */
        QVariant getDirection() const;

        /**
         * @brief 获取当前是否已反向
         */
        bool isReversed() const;

        /**
         * @brief 设置反向状态
         */
        void setReversed(bool reversed);

        /**
         * @brief 设置方向矢量（仅对直接输入/自定义坐标有效，X/Y/Z轴会忽略）
         */
        void setDirection(const QVector3D& dir);

        /**
         * @brief 获取/设置自定义坐标模式下的起点/终点
         */
        QVector3D startPoint() const;
        QVector3D endPoint() const;
        void setCustomCoordinatePoints(const QVector3D& start, const QVector3D& end);

        /**
         * @brief 获取大小值（字符串）
         */
        QString magnitudeValue() const;

        /**
        * @brief 设置大小值
        */
        void setMagnitudeValue(const QString& value);

        /**
         * @brief 设置大小输入框的单位文本
         */
        void setMagnitudeUnit(const QString& unitText);

        /**
         * @brief 设置是否显示大小输入框的单位
         */
        void setMagnitudeUnitVisible(bool visible);

        /**
         * @brief 设置是否显示整个大小输入行
         */
        void setMagnitudeVisible(bool visible);

        /**
         * @brief 设置大小输入行的标签文本
         */
        void setMagnitudeLabel(const QString& label);

        /**
         * @brief 获取内部通用拾取控件（用于外部设置拾取类型白名单等）
         */
        mPickWidgetController* startPickController() const { return _startPickController; }
        mPickWidgetController* endPickController() const { return _endPickController; }
        mPickWidgetController* objectPickController() const { return _objectPickController; }

        /**
         * @brief 获取当前自定义坐标/选择对象模式下的原始拾取数据（用于外部持久化）
         */
        PickWidgetData startPickData() const;
        PickWidgetData endPickData() const;
        PickWidgetData objectPickData() const;

        /**
         * @brief 反显拾取数据（用于外部恢复状态）
         */
        void setStartPickData(const PickWidgetData& data);
        void setEndPickData(const PickWidgetData& data);
        void setObjectPickData(const PickWidgetData& data);

        /**
         * @brief 设置是否禁用（整体灰化）
         */
        void setWidgetEnabled(bool enabled);

        /**
        * @brief 恢复控件到初始默认状态
        * @details 重置方向模式、反向状态、所有输入框内容、拾取数据与预览渲染，不重建控件
        */
        void resetToDefault();

        /*
        * 获取当前方向模式
        */
        DirectionMode getCurrentMode() { return _currentMode; }

        /*
        * 清空临时高亮
        */
        void clearAllHighlight();

        /// 起点-终点页是否仍处于出厂默认值（创建后未被编辑/拾取/反显过）。
        /// 供外部在模式切换瞬间避免默认方向(+X)顶掉已有数据。
        bool customCoordinatesUntouched() const { return _customPage && _customUntouched; }

    signals:
        /**
         * @brief 方向发生变化时发射
         */
        void directionChanged();

        /**
         * @brief 大小输入发生变化时发射
         */
        void magnitudeChanged();

        /**
         * @brief 内部布局发生变化（如模式切换导致 QStackedWidget 页面显隐/高度变化），
         *        通知外层容器刷新折叠块布局，避免内容被挤压或留白。
         */
        void layoutRefreshRequested();

        /*
        * @brief 方式切换时，通知外部当前方式无拾取控件，方便其激活其他拾取控件
        */
        void currentWayNoPickWidget();

    private slots:
        void onModeChanged(int index);
        void onReverseClicked();
        void onApplyStartDirection();
        void onApplyEndDirection();
        void onObjectPickedChanged();
        void onDirectInputChanged();
        void onCustomInputChanged();
        void onMagnitudeChanged();

        // 拾取控件互斥激活（统一处理，基类 mPickWidgetBase 已负责全局互斥）
        void onPickControllerActivated(mPickWidgetController* controller);

    private:
        void ensureCustomPage();
        void ensureObjectPage();
        void ensureDirectPage();

        void initUI(
            const QString& labelText,
            bool showMagnitude,
            const QString& magnitudeLabel,
            bool hasUnit,
            const QString& unitText,
            bool hasDefaultDirect,
            mColorBlock objectPickColorBlock);
        void updateDirectionFromAxis();
        void updateReverseIcon();
        void setInputLineEditText(QLineEdit* edit, const QVector3D& vec);
        QVector3D parseLineEditToVector(const QLineEdit* edit) const;
        QVector3D computeCoordinateFromController(const mPickWidgetController* controller) const;
        QVector3D computeDirectionFromController(const mPickWidgetController* controller) const;

        // 提取公共逻辑：从拾取控件计算坐标并回填到编辑框，然后触发方向变化。
        void applyControllerCoordinate(mPickWidgetController* controller, QLineEdit* edit);

        // 档位/主题跟随：按 UiMetrics 重发行高/列宽/页高，自绘细线框随 UiPalette 重绘
        void applyThemeStyles();

        void setContentPage(QWidget* page);
        void refreshContainerLayout();

        /// @brief 更新指定拾取控件对应的预览渲染点
        void updatePreviewRender(mPickWidgetController* controller);
        /// @brief 清除所有预览渲染节点
        void clearPreviewRender();
        /// @brief 清除指定拾取控件对应的预览渲染节点
        void clearPickPreview(mPickWidgetController* controller);

        DirectionMode _currentMode;
        DirectionMode _initialMode; // 新增：保存初始化时的默认模式
        bool _reversed = false;
        DirectionCalculator _calculator;
        CoordinateCalculator _coordinateCalculator;

        // 子拾取控件允许拾取类型（延迟创建后统一应用）
        QSet<MxFunctions::PickDataType> _pickAllowedTypes;

        // “选择对象”模式拾取控件的颜色块（决定UI色块与拾取高亮颜色，延迟创建后应用）
        mColorBlock _objectPickColorBlock = CBRed;

        // 抑制 refreshContainerLayout 在同一事件循环内重复发射 layoutRefreshRequested
        bool _layoutRefreshPending = false;

        // 第一行（业务标题改由细线框题注承载，行内标签统一「定义方式」）
        QLabel* _label = nullptr;
        QComboBox* _modeCombo = nullptr;
        QToolButton* _reverseBtn = nullptr;
        QWidget* _topWidget = nullptr;

        // 外边框分组框（MxFunctions::mGroupBox 骑线标题=构造参数 labelText）
        MxFunctions::mGroupBox* _groupBox = nullptr;

        // 内容区域（动态切换）
        QStackedWidget* _stackedWidget = nullptr;
        QWidget* _currentContentPage = nullptr;
        QWidget* _customPage = nullptr;
        QWidget* _objectPage = nullptr;
        QWidget* _directPage = nullptr;

        // 自定义坐标页（延迟创建）
        QLabel* _startLabel = nullptr;
        mPickWidgetController* _startPickController = nullptr;
        QToolButton* _startApplyBtn = nullptr;
        QLineEdit* _startDirEdit = nullptr;
        QLabel* _endLabel = nullptr;
        mPickWidgetController* _endPickController = nullptr;
        QToolButton* _endApplyBtn = nullptr;
        QLineEdit* _endDirEdit = nullptr;

        // 选择对象页（延迟创建）
        QLabel* _objectLabel = nullptr;
        mPickWidgetController* _objectPickController = nullptr;

        // 直接输入页（延迟创建）
        QLabel* _directLabel = nullptr;
        QLineEdit* _directEdit = nullptr;

        // 主布局
        QVBoxLayout* _rootLayout = nullptr;

        // 大小行
        QWidget* _magnitudeWidget = nullptr;
        LableAndLineEdit _magnitudeInput;

        // 缓存当前方向（非轴模式时）
        QVector3D _currentDir = QVector3D(1, 0, 0);

        /// 当前激活的拾取控件（用于渲染状态管理）
        mPickWidgetController* _activePickController = nullptr;

        /// 拾取坐标缓存（解决点击应用时拾取失活导致数据丢失）
        QVector3D _cachedStartCoord = QVector3D(0, 0, 0);
        QVector3D _cachedEndCoord = QVector3D(1, 0, 0);

        /// 缓存坐标是否来源于有效拾取（未拾取过/已应用清空后为 false）
        bool _cachedStartCoordValid = false;
        bool _cachedEndCoordValid = false;

        /// 起点-终点页是否仍为出厂默认值（未被用户编辑/拾取/外部设置过）
        bool _customUntouched = false;
    };
}