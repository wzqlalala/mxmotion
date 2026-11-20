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

#include "mLoadTransient.h"

using namespace MechCaseData;
using namespace MechSetData;
namespace MechMainWindow
{
    class MMAINWINDOW_EXPORT mLoadTransientPanel : public mLoadPanelBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mLoadTransientPanel(QObject* parent = nullptr);

        /*
        * 析构函数
        */
        ~mLoadTransientPanel();

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

        int getLoadBoundId();

        void saveDelayTime();

        void saveLoadBound();

        void notifyLoadChanged();

    private slots:
        void slot_loadNameChanged(int index);

        void slot_loadTypeChanged(int index);

    private:
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>_picks;

        QVector<InputWidgetSet> inputWidgets;

        QVector<singleComboBox> comboBoxs;
        QHash<QString, int>_loadboundtable; //载荷与边界统一，名称与id都唯一，所以放到一个哈希表即可

        int index = 0;
    };

}