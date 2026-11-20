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

#include "mLoadBolt.h"

using namespace MechCaseData;
using namespace MechSetData;
namespace MechMainWindow
{
    class MMAINWINDOW_EXPORT mLoadBoltPanel : public mLoadPanelBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mLoadBoltPanel(QObject* parent = nullptr);

        /*
        * 析构函数
        */
        ~mLoadBoltPanel();

        /*
        * 初始化输入控件
        */
        void initInputs()override;

        /*
        * 清除输入控件
        */
        void clearInputs()override;

        QVector<QWidget*> getRefStateInputs(FoldSection* fathertar) override;

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
        * 获取函数
        */
        int getFuncId();

        /*
        * 获取螺栓约束
        */
        int getConnectorBoltId();

        // 局部保存辅助函数（不再 emit sg_editFinished）
        void saveValue();
        void saveBolt();
        void saveWay();
        void saveFunc();
        void saveScale();
        void notifyLoadChanged();

    private slots:
        /*
        * 选择螺栓响应函数
        */
        void slot_selectBoltChanged(int idx);

        /*
        * 方式
        */
        void slot_wayChanged(int idx);

        /*
        * 函数
        */
        void slot_funcChanged(int idx);


    private:
        QVector<singleComboBox> comboBoxs;
        comboBoxWithPushBtn combofunc;
        QHash<QString, int>_functable;
        QVector<InputWidgetSet> inputWidgets;

        QHash<QString, int>_bolttable;

        InputWidgetSet _scale;

        int type = 0;
    };
}