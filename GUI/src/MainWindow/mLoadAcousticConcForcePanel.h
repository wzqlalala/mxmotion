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
#include <memory>
#include <unordered_map>

#include "mCalcLineEdit.h"
#include "mPanelBase.h"
#include "mMaterialPropBase.h"

//MxFunction
#include "mPhysicalFieldEnum.h"
#include "mModelStruct.h"

#include "mLoadPanelBase.h"
#include "mLoadAcousticConcForce.h"
#include "mPickWidgetController.h"

using namespace MechCaseData;
using namespace MechSetData;

namespace MechMainWindow
{
    class MMAINWINDOW_EXPORT mLoadAcousticConcForcePanel : public mLoadPanelBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mLoadAcousticConcForcePanel(QWidget* parent = nullptr);

        /*
        * 析构函数
        */
        ~mLoadAcousticConcForcePanel();

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
        void setInputsValue(std::shared_ptr<MechCaseData::mLoadBoundaryBase>loadData)override;

        /*
        * 作用对象输入控件
        */
        QVector<QWidget*> getTargetInputs(FoldSection* fathertar) override;

        /*
        * 值输入控件
        */
        QVector<QWidget*> getValueInputs(FoldSection* fatherval) override;

        /*
        * 获取当前树上的作用对象
        */
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> getPickedData()override;

        /*
        * 切出面板时失活并提交拾取
        */
        void deactivatePickAndCommit();

        /*
        * 连接/断开单位制响应（声学载荷当前无单位切换，留空实现）
        */
        void connectUnitSystemResponse()override {}
        void disconnectUnitSystemResponse()override {}

    private slots:
        // 大小输入
        void slot_valueChanged();

    private:
        // 局部保存辅助
        void saveValue();
        void notifyLoadChanged();

        // 根据数据实时刷新声学载荷符号
        void updateLoadSymbolRealtime();

        // 汇总拾取+集合，计算渲染位置
        QVector<QVector3D> collectRenderPositions();

        QVector<InputWidgetSet> inputWidgets;

        // 通用拾取控件
        mPickWidgetController* _pickWidgetController = nullptr;

        // 当前声学载荷数据
        std::shared_ptr<MechCaseData::mLoadAcousticConcForce> _currentLoad;
    };
}