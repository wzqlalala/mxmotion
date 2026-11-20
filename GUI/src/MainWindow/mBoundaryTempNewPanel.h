#pragma once
#pragma execution_character_set("utf-8")
#include "mmainwindow_global.h"

#include <QLabel>
#include <QObject>
#include <QWidget>
#include <QLineEdit>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QTreeWidget>
#include <QTimer>
#include <memory>

#include "mPanelBase.h"
#include "mBoundaryPanelBase.h"
#include "mBoundaryTempNew.h"
#include "mMixSet.h"
#include "mPickWidgetController.h"

#include "mMapRelationEnum.h"
#include "mSetManager.h"
#include "mMapRelationManager.h"

using namespace MechCaseData;
using namespace MechSetData;
namespace MechMainWindow
{
    class MMAINWINDOW_EXPORT mBoundaryTempNewPanel : public mBoundaryPanelBase
    {
        Q_OBJECT
    public:
        mBoundaryTempNewPanel(QWidget* parent = nullptr);
        ~mBoundaryTempNewPanel();

        /*
        * 初始化输入控件
        */
        void initInputs()override;

        /*
        * 清除输入控件
        */
        void clearInputs()override;

        /*
        * 控件设值
        */
        void setInputsValue(std::shared_ptr<MechCaseData::mLoadBoundaryBase>boundaryData)override;

        /*
        * 作用对象输入控件
        */
        QVector<QWidget*> getTargetInputs(FoldSection* fathertar) override;

        /*
        * 值输入控件
        */
        QVector<QWidget*> getValueInputs(FoldSection* fatherval) override;

        /*
        * 获取拾取数据
        */
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> getPickedData()override;

        /*
         * 创建边界数据
        */
        std::shared_ptr<MechCaseData::mLoadBoundaryBase> createBoundaryData();

        /*
        * 失活并提交拾取数据：供统一退出出口调用
        */
        void deactivatePickAndCommit();

        /*
        * 实时渲染边界符号（启动防抖定时器）
        */
        void updateBoundarySymbolRealtime();

        /*
        * 实时渲染边界符号执行体
        */
        void doUpdateBoundarySymbolRealtime();

    private slots:
        /*
        * 文件浏览
        */
        void slot_browseFileClicked();

    private:
        /*
        * 应用拾取类型白名单
        */
        void applyAllowedPickTypes();

        /*
        * 汇总拾取+集合数据并计算渲染坐标
        */
        void collectRenderPositions(QVector<QVector3D>& outPositions);

        /*
         * 单独保存各数据项，避免每次修改都 emit sg_editFinished 触发全量保存
         */
        void saveMainModelFile();       // 主模型文件路径
        void saveTimeStep();            // 时间步
        void saveMainSet();             // 主模型集合
        void saveUseMainSet();          // 是否指定主模型集合
        void notifyBoundaryChanged();   // 仅通知外部刷新树/渲染

    private:
        // 维护一份界面级 mBoundaryTempNew 数据，避免每次使用基类指针再转换
        std::shared_ptr<MechCaseData::mBoundaryTempNew> _boundaryTempNewData{ nullptr };

        // 通用拾取控件（作用对象）
        mPickWidgetController* _pickWidgetController = nullptr;

        // 实时符号渲染防抖定时器
        QTimer* _realtimeUpdateTimer = nullptr;

        // 上次实时渲染的最终数据签名（对象ID+位置/方向/分量值），未变化时跳过符号赋值与全量重建
        size_t _lastRenderSig = 0;

        // 主模型定义
        QLineEdit* _mainModelFileEdit{ nullptr };
        QLineEdit* _timeStepEdit{ nullptr };
        QLineEdit* _mainSetEdit{ nullptr };
        QCheckBox* _mainSetCheck{ nullptr };
        QPushButton* _openFileBtn{ nullptr };
    };
}
