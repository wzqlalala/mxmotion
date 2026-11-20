#pragma once
#pragma execution_character_set("utf-8")
#include "mmainwindow_global.h"

#include <QLabel>
#include <QPushButton>
#include <QObject>
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QComboBox>
#include <QListView>
#include <QCheckBox>
#include <QSpinBox>
#include <QHeaderView>
#include <QTableWidget>
#include <QMap>
#include <memory>
#include <set>
#include <unordered_map>
#include <vector>

#include "mCalcLineEdit.h"
#include "mPanelBase.h"
#include "mMaterialPropBase.h"
#include "mConnector.h"

//MxFunction
#include "mPhysicalFieldEnum.h"
#include "mModelStruct.h"

#include "mBoundaryPanelBase.h"
#include "mPickWidgetController.h"
#include "mBoundaryVirtual.h"

using namespace MechCaseData;
using namespace MechSetData;

namespace MechMainWindow
{
    class MMAINWINDOW_EXPORT mBoundaryVirtualPanel : public mBoundaryPanelBase
    {
        Q_OBJECT
    public:
        mBoundaryVirtualPanel(QWidget* parent = nullptr);
        ~mBoundaryVirtualPanel();

        // 初始化输入控件
        void initInputs() override;

        // 清除输入控件
        void clearInputs() override;

        // 控件设值
        void setInputsValue(std::shared_ptr<MechCaseData::mLoadBoundaryBase>boundaryData) override;

        // 作用对象输入控件
        QVector<QWidget*> getTargetInputs(FoldSection* fathertar) override;

        // 值输入控件
        QVector<QWidget*> getValueInputs(FoldSection* fatherval) override;

        // 获取当前控件上的作用对象
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> getPickedData() override;

        // 失活并提交拾取数据：供统一退出出口调用
        void deactivatePickAndCommit();

        // 实时渲染边界符号（启动防抖定时器）
        void updateBoundarySymbolRealtime();
        // 实时渲染边界符号执行体
        void doUpdateBoundarySymbolRealtime();

    private slots:
        // 勾选自由度
        void slot_checkDegreeOfFreedom(int state);

    private:
        // 应用拾取类型白名单
        void applyAllowedPickTypes();

        // 单独保存各数据项，避免每次修改都 emit sg_editFinished 触发全量保存
        void saveDOF();                 // U1..UR3 自由度勾选
        void notifyBoundaryChanged();   // 仅通知外部刷新树/渲染

        // 汇总拾取+集合数据并计算渲染坐标
        void collectRenderPositions(QVector<QVector3D>& outPositions);

        // 当前边界数据
        std::shared_ptr<MechCaseData::mBoundaryVirtual> _currentBoundary;

        QVector<CheckBoxsWidget> checkswid;
        QVector<QString> names1_;
        QVector<QString> names2_;

        // 通用拾取控件（作用对象）
        mPickWidgetController* _pickWidgetController = nullptr;

        // 实时符号渲染防抖定时器
        QTimer* _realtimeUpdateTimer = nullptr;

        // 上次实时渲染的最终数据签名（对象ID+位置/方向/分量值），未变化时跳过符号赋值与全量重建
        size_t _lastRenderSig = 0;
    };
}
