#include "mGlobalOutputControlPanel.h"

//Function
#include "mGlobalSignals.h"

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mMechGlobalSignals.h"
#include "mDataStatusManager.h"

//MechModelData
#include "mGlobalParam.h"
#include "mGlobalOutputControlData.h"
#include "mViewportModelManager.h"

using namespace MechGlobalParamData;
namespace MechMainWindow
{
    mGlobalOutputControlPanel::mGlobalOutputControlPanel(QWidget* parent)
        : mPanelBase(parent)
    {
        initUI();
    }

    mGlobalOutputControlPanel::~mGlobalOutputControlPanel()
    {
    }

    void mGlobalOutputControlPanel::initUI()
    {
        _mainLayout = new QVBoxLayout(this);
        _mainLayout->setContentsMargins(2, 0, 2, 0);

        // ========== 输出控制 ==========
        _outputFoldWidget = new FoldSection("输出控制", this, QColor("#909399"), true);

        QHash<QString, QStringList> allOutputs = buildAllOutputVariables();
        // 默认全部不选中，可在这里根据需求修改 defaults
        QHash<QString, QStringList> currentSelections;

        _outputControlWidget = new OutputControlWidget(allOutputs, currentSelections,nullptr, this);
        // 默认值也置空，全部不勾选；如需默认全选可改为 allOutputs
        _outputControlWidget->setDefaultOutputs(currentSelections);
        _outputControlWidget->setTreeMinimumHeight(350);

        // 勾选结果实时保存到全局输出控制数据
        connect(_outputControlWidget, &OutputControlWidget::outputsChanged, this, [=](const QHash<QString, QHash<QString, QStringList>>& newData) {
            auto data = getCVM->getModelData()->getGlobalParam()->getGlobalOutputControlData();
            if (data)
            {
                QHash<QString, QStringList> outputData;
                for (auto key : newData.keys()) {
                    auto data = newData.value(key);
                    outputData[key] = data.keys();
                }
                data->setOutputSelections(outputData);
                getCVM->getDataStatusManager()->updateDynamicInfoData(Physical_GlobalOutputControl, 0);
            }
        });

        _outputFoldWidget->addRow(_outputControlWidget);
        m_foldWidgets.append(_outputFoldWidget);
        _mainLayout->addWidget(_outputFoldWidget);

        _mainLayout->addStretch();
    }

    void mGlobalOutputControlPanel::createGlobalOutputControlPanel()
    {
        if (!getCVM) return;
        auto globalParam = getCVM->getModelData()->getGlobalParam();
        if (globalParam->getGlobalOutputControlData())
        {
            mGlobalSignals::getInstance()->outputMessageSig(1, QString("只能创建一个全局输出控制"), true);
            return;
        }

        globalParam->createGlobalOutputControlData();
        mGlobalSignals::getInstance()->outputMessageSig(QString("成功创建全局输出控制"), true);
        emit MechCommon::mMechGlobalSignals::getInstance()->createUpdateGlobalOutputControlSig(true);
    }

    void mGlobalOutputControlPanel::editGlobalOutputControlPanel()
    {
        if (!getCVM) return;
        auto data = getCVM->getModelData()->getGlobalParam()->getGlobalOutputControlData();
        if (data == nullptr) return;

        _outputControlWidget->setDefaultOutputs(data->getOutputSelections());
    }

    QHash<QString, QStringList> mGlobalOutputControlPanel::buildAllOutputVariables() const
    {
        QHash<QString, QStringList> outputs;

        outputs["基本结果"] = QStringList{
            "位移", "速度", "加速度"
        };
        outputs["应力"] = QStringList{
            "节点应力", "单元应力", "简单磨平应力", "单元高斯积分点应力"
        };
        outputs["应变"] = QStringList{
            "单元应变", "节点应变", "节点等效塑性应变",
            "节点等效蠕变应变", "单元等效塑性应变",
            "简单磨平应变", "简单磨平等效塑性应变"
        };
        outputs["能量"] = QStringList{
            "节点应变能", "单元应变能", "简单磨平应变能", "应变能密度"
        };
        outputs["热学"] = QStringList{
            "温度", "节点温度梯度", "单元温度梯度", "节点热流密度", "单元热流密度"
        };
        

        outputs["PSD谱分析"] = QStringList{
            "节点位移功率谱", "节点速度功率谱", "节点加速度功率谱", "节点应变功率谱", "节点应力功率谱", "单元应力功率谱",
            "单元应变功率谱", "单元磨平应力功率谱", "单元磨平应变功率谱"
        };
        outputs["RMS谱分析"] = QStringList{
            "节点位移1σ均方根", "节点位移2σ均方根", "节点位移3σ均方根",
            "单元磨平应力1σ均方根", "单元磨平应力2σ均方根", "单元磨平应力3σ均方根", "单元应力均方根"
        };
        outputs["复合材料"] = QStringList{
            "单元复合应变", "单元复合应力", "单元失效指数", "hashin失效系数",
            "单元损伤状态变量", "铺层温度", "单元失效强度比", "单元铺层应力", "实体铺层节点应力", "铺层划分"
        };
        outputs["结构力与内力"] = QStringList{
            "支反力", "多点约束力", "杆梁节点内力", "梁截面内力", "接触力",
            "602载荷", "轴力", "单元节点力", "刚性墙"
        };
        outputs["声学"] = QStringList{
            "声压级(分贝)", "声压(帕斯卡)"
        };
        outputs["损伤"] = QStringList{
            "损伤", "拉伸损伤", "压缩损伤", "损伤起始准则", "剪应力比"
        };
        

        return outputs;
    }
}
