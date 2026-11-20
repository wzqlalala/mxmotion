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
#include <memory>

#include "mCalcLineEdit.h"
#include "mPanelBase.h"
#include "mMaterialPropBase.h"

//MxFunction
#include "mPhysicalFieldEnum.h"
#include "mModelStruct.h"

#include "mBoundaryPanelBase.h"
#include "mPickWidgetController.h"
#include "mBoundaryFixed.h"

using namespace MechCaseData;
using namespace MechSetData;

namespace MechMainWindow
{
    class MMAINWINDOW_EXPORT mBoundaryFixedPanel : public mBoundaryPanelBase
    {
        Q_OBJECT
    public:
        mBoundaryFixedPanel(QWidget* parent = nullptr);
        ~mBoundaryFixedPanel();

        // 初始化输入控件
        void initInputs() override;

        // 清除输入控件
        void clearInputs() override;

        // 控件设值
        void setInputsValue(std::shared_ptr<MechCaseData::mLoadBoundaryBase> boundaryData) override;

        // 作用对象输入控件
        QVector<QWidget*> getTargetInputs(FoldSection* fathertar) override;

        // 值输入控件
        QVector<QWidget*> getValueInputs(FoldSection* fatherval) override;

        // 获取当前控件上的作用对象
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> getPickedData() override;

        // 失活并提交拾取数据：供统一退出出口调用
        void deactivatePickAndCommit();

        // 实时渲染边界固定约束符号
        void updateBoundarySymbolRealtime();

    private slots:
        // 固定约束类型切换
        void slot_valueFixTypeChanged(int idx);

    private:
        // 应用拾取类型白名单
        void applyAllowedPickTypes();

        // 单独保存固定约束类型，不触发 sg_editFinished
        void saveFixedType();

        // 仅通知外部刷新树/渲染
        void notifyBoundaryChanged();

        // 汇总拾取+集合数据并计算渲染坐标
        void collectRenderPositions(QVector<QVector3D>& outPositions);

        QVector<singleComboBox> comboBoxs;
        singleLabel _label;

        // 当前边界数据
        std::shared_ptr<MechCaseData::mBoundaryFixed> _currentBoundary;

        // 通用拾取控件（作用对象）
        mPickWidgetController* _pickWidgetController = nullptr;
    };
}
