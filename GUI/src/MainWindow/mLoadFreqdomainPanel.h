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

#include "mCalcLineEdit.h"
#include "mPanelBase.h"
#include "mMaterialPropBase.h"

//MxFunction
#include "mPhysicalFieldEnum.h"
#include "mModelStruct.h"

#include "mLoadPanelBase.h"

#include "mLoadFreqdomain.h"

using namespace MechCaseData;
using namespace MechSetData;
namespace MechMainWindow
{
    class MMAINWINDOW_EXPORT mLoadFreqdomainPanel : public mLoadPanelBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mLoadFreqdomainPanel(QObject* parent = nullptr);

        /*
        * 析构函数
        */
        ~mLoadFreqdomainPanel();

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
        * 值输入控件
        */
        QVector<QWidget*> getValueInputs(FoldSection* fatherval) override;

        /*
        * 作用对象输入控件
        */
        QVector<QWidget*> getTargetInputs(FoldSection* fathertar)override;

        /*
        * 获取当前树上的作用对象
        */
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> getPickedData()override;

        /*
        * 连接单位制响应信号
        */
        void connectUnitSystemResponse()override;

        /*
        * 断开单位制响应信号
        */
        void disconnectUnitSystemResponse()override;


        int getLoadBoundId();
        int getFuncId1();
        int getFuncId2();

        void saveDelayAndPhase();
        void saveLoadBound();
        void saveFunc1();
        void saveFunc2();
        void notifyLoadChanged();

    private slots:

        void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx);

        void slot_loadNameChanged(int index);

        void slot_loadTypeChanged(int index);

        void slot_freqLoadTypeChanged(int index);

        void slot_funcChanged(int index);
    private:
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>_picks;

        QVector<InputWidgetSet> inputWidgets;

        QVector<singleComboBox> comboBoxs;

        QHash<QString, int>_loadboundtable;

        int freqTypeIndex = 0;
        int loadTypeIndex = 0;


        comboBoxWithPushBtn combofunc1;
        comboBoxWithPushBtn combofunc2;

        QHash<QString, int>_functable; //获取的是全局函数，所以有一个就行

        QStringList _units;
    };

}
