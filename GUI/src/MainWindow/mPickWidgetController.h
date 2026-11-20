#pragma once
#pragma execution_character_set("utf-8")

#include <QObject>
#include <QEvent>
#include <QPointer>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>

#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>

#include "mmainwindow_global.h"
#include "mPickWidgetData.h"
#include "mPickWidgetViewBase.h"
#include "mPickWidgetDataModel.h"
#include "mPickWidgetHighlightService.h"
#include "IPickPersistenceStrategy.h"
// 以下包含与 dev_Mech 侧保持一致：mPointPickerWidget/mDirectionSelectorWidget
// 等控件实现依赖 getCVM/渲染窗口类型，经本头文件传递可见
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "PickCommonFunction.h"
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"

// PreData
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "PickCommonFunction.h"

// Render
#include "mxr/view/CCSMRendWidget.h"
#include "mxr/data/SelectDataStruct.h"

namespace MechMainWindow
{
    class MMAINWINDOW_EXPORT mPickWidgetController : public QObject
    {
        Q_OBJECT

    public:
        friend class mSetPickWidgetConntroller;

        explicit mPickWidgetController(
            std::shared_ptr<mPickWidgetHighlightServiceBase> highlightService, mPickWidgetViewBase* view,
            QWidget* parent = nullptr);
        ~mPickWidgetController() override;

        // 获取内部视图（基类指针）
        mPickWidgetViewBase* widget() const { return _view; }

        // 运行时显隐整个拾取控件
        void setPickWidgetVisible(bool visible);
        bool isPickWidgetVisible() const { return _isVisible; }

        // 设置默认提示
        void setDefultTip(const QString& str, const QString& topStr = QString());

        // 设置可选集合列表
        void setSelectSetList(const QHash<QString, QString>& setToType = {});

        // 运行时显隐集合按钮
        void setSelectSetBtnVisible(bool visible);

        // 反显：写入数据 + 刷新UI + 按激活态渲染高亮（反显不置脏）
        void displayData(const PickWidgetData& data, bool highlight = false);

        // 用户级修改：清空集合与非匹配拾取，仅保留传入拾取数据
        void replacePickedDataAndClearSets(const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& data);

        // 获取所有数据（集合+拾取）
        const PickWidgetData& getAllData() const { return _model ? _model->getAllData() : s_emptyPickWidgetData; }

        // 激活/失活
        void setPickActive(bool active);
        bool isPickActive() const { return _isActive; }

        // 登记绑定数据存活性判定（面板通常判自己的数据裸指针非空，
        // 或"指针非空且管理器里仍可查到"）：失活与静态提交前统一校验，
        // 数据已死则跳过 commit()/pickActiveChanged，只做机械摘除——防止
        // 模型对象删除后残留激活态经互斥失活回写悬垂指针。
        // 守卫 lambda 随控制器析构失效（控制器挂在面板生命周期内，面板不先亡）
        void setBindingGuard(std::function<bool()> alive) { _bindingAlive = std::move(alive); }

        // 数据对象已删/将删时的安全摘除（幂等）：解除全局激活注册、断视口连接、
        // 按失活惯例清高亮；不提交、不发信号——供各面板的数据销毁通知槽调用
        void detachActive();

        // 控件由隐藏转为可见（如面板切出后再切回）时恢复本控件应有的高亮形态：
        // 激活态恢复正式高亮；失活但有拾取数据的恢复临时高亮
        virtual bool eventFilter(QObject* watched, QEvent* event) override;

        // 运行时切换拾取模式
        void setPickMode(mPickMode pickMode, bool clearWhenSwitch = false);
        mPickMode pickMode() const { return _view ? _view->pickMode() : PickMore; }

        // 失活时是否保留临时高亮
        void setKeepHighlightOnDeactivate(bool keep) { _keepHighlightOnDeactivate = keep; }
        bool keepHighlightOnDeactivate() const { return _keepHighlightOnDeactivate; }

        // 云图模式：供带云图的面板使用。开启后清空拾取高亮显示并持续抑制高亮渲染，
        // 拾取数据（共享选中容器+本控件模型）完整保留、拾取交互照常；
        // 关闭后按当前激活态恢复高亮显示
        void setCloudChartMode(bool enable);
        bool isCloudChartMode() const { return _cloudChartMode; }

        // 设置允许的拾取类型
        void setAllowedTypes(std::function<QSet<MxFunctions::PickDataType>()> getSetype);

        // 设置拾取数据过滤器（按维度二次过滤）
        void setPickFilter(QSet<int> filter);

        // 设置边载荷拾取过滤：几何线仅保留位于几何面上的线，单元线仅保留属于二维单元的边
        void setEdgeLoadPickFilter(bool enable);

        // 全部置灰/恢复拾取工具栏按钮
        void setAllPickTypesDisabled(bool disabled);

        // 统一刷新UI + 高亮
        void refreshAll();

        // 清空本控件所有高亮
        void clearAllHighlight();

        // 清空本控件临时高亮
        void clearAllTempHighlight();

        // 刷新高亮
        void refreshHighlight();

        // 设置集合类型化解析器
        void setSetResolver(std::function<std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>(const QString&)> resolver);

        // 注入持久化策略
        void setPersistenceStrategy(std::shared_ptr<IPickPersistenceStrategy> strategy);

        //提交当前激活拾取控件的数据（导出/计算前调用，指针为空则跳过）
        static void commitActivePickWidget();

        //全局是否存在激活中的拾取控件（跨面板查询，激活态全局互斥至多一个）
        static bool hasActivePickController() { return s_activeController != nullptr; }

        //过滤激活的拾取方式
        void setPickType();

    signals:
        //void requestActivate();              // 请求外部激活本控件
        void pickActiveChanged(bool isActive);
        void dataChanged(); // 数据变化通知
        void calcMainNodeSig();

    protected slots:
        void onUiActivated();
        void onUiClear();
        // 集合选择：基类空实现，子类重写
        void onUiSelectSetChanged(const QSet<QString>& selectedNames);
        // 删除逻辑：基类提供实现，由带详情UI的子类连接信号
        void onUiRemovePickRequested(const QList<PickDisplayItem>& items);
        void onUiRemovePickRequestedByType(MxFunctions::PickDataType type);
        // 计算主节点：基类空实现
        void onUiCalcMainNode();

        void onViewportPickFinished();

        // 视口/模型切换联动（多模型多视口下保证拾取数据不错乱）：
        // 切换前：同模型切换保持拾取会话；跨模型切换按"退出面板"处理——
        //         按旧当前模型提交、失活并清空高亮（不保留临时高亮残留）
        // 切换后：仍激活的控件把拾取连接与高亮重绑到新激活视口
        void onViewportModelAboutToChange(int newViewportID);
        void onViewportModelSwitched(int viewportID);

    protected:
        // 通用初始化
        void initControllerCommon(const QString& colorKeyPrefix = "PickBase");
        // 连接UI信号
        void connectUiSignals();

        void connectViewportSignal();
        void disconnectViewportSignal();

        // 刷新UI（基类仅刷新拾取数据，子类扩展集合）
        virtual void refreshUI();

        // 数据修改后的统一收尾：置脏 + 刷新 + 通知
        void markDirtyAndNotify();
        PickDisplayItem toPickDisplayItem(MxFunctions::PickDataType type, const RenderPickResultData& data) const;

        // 生成拾取类型提示文本
        static QString buildAllowedTypesTip(const QSet<MxFunctions::PickDataType>& types);

        // 捕获视口当前拾取结果
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> captureViewportPickData() const;

        // 渲染数据为临时高亮
        void renderPickedDataAsTempHighlight();
        // 高亮渲染统一出口
        void applyHighlight(bool active);
        // 获取高亮颜色
        QColor highlightColor() const;

        // 本控件独立colorKey
        const QString& uniqueColorKey() const { return _uniqueColorKey; }

        // 单选模式数据裁剪
        void enforceSinglePickLimit();

        // 去重合并工具
        static void mergeInto(
            std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& target,
            const std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>& source);

        // 最终提交：去重合并 + 持久化 + 高亮刷新
        virtual void commit();

        // 解析单个集合
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>
            resolveSet(const QString& setName) const;

        // 解析所有选中集合
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>
            resolveSelectedSets(const QSet<QString>& selectedSetNames) const;

    private:
        QPointer<mPickWidgetViewBase> _view = nullptr;
        std::unique_ptr<mPickWidgetDataModel> _model;
        std::shared_ptr<mPickWidgetHighlightServiceBase> _highlightService;

        bool _isActive = false;
        QMetaObject::Connection _viewportConnection;

        // 可见性标志位：由外部显隐接口控制，隐藏时拒绝激活并清理高亮
        bool _isVisible = true;

        //脏数据标志位
        bool _dirty = false;
        bool _keepHighlightOnDeactivate = true;

        enum class HighlightForm { None, ActiveNormal, InactiveTemp };
        HighlightForm _highlightForm = HighlightForm::None;
        QString _uniqueColorKey;

        std::shared_ptr<IPickPersistenceStrategy> _persistenceStrategy;
        std::function<std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>(const QString&)> _setResolver;
        std::function<QSet<MxFunctions::PickDataType>()> _getSetype;

        //可使用单元维度
        QSet<int> _elementFilter;

        //边载荷拾取过滤：几何线须位于几何面上，单元线须属于二维单元
        bool _edgeLoadFilter = false;

        //云图模式：清空并抑制拾取高亮显示，拾取数据保留
        bool _cloudChartMode = false;

    private:
        // 全局互斥：同一时刻仅一个激活（QPointer：控制器销毁自动置空，
        // 互斥失活与静态提交不依赖析构函数手动清理）
        static QPointer<mPickWidgetController> s_activeController;
        static int s_nextInstanceId;
        static const PickWidgetData s_emptyPickWidgetData;

        // 绑定存活性判定：未登记视为存活（存量面板行为不变）
        std::function<bool()> _bindingAlive;
        bool bindingAlive() const { return !_bindingAlive || _bindingAlive(); }

        // 失活/摘除共用的机械收尾：断视口连接+按惯例清高亮（不提交、不发信号）
        void deactivateMechanical();
    };

    class MMAINWINDOW_EXPORT mSetPickWidgetConntroller : public mPickWidgetController {
    public:
        explicit mSetPickWidgetConntroller(
            std::shared_ptr<mPickWidgetHighlightServiceBase> highlightService, mPickWidgetViewBase* view,
            QWidget* parent = nullptr);
        ~mSetPickWidgetConntroller() override;
    protected:

        virtual void commit() override;
    };
}