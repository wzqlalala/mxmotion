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
#include "mConnector.h"

//MxFunction
#include "mPhysicalFieldEnum.h"
#include "mModelStruct.h"

using namespace MechCaseData;
using namespace MechSetData;
namespace MechMainWindow
{
    class MMAINWINDOW_EXPORT mPredefinedFieldPanelBase : public mPanelBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mPredefinedFieldPanelBase(QWidget* parent = nullptr);

        /*
        * 析构函数
        */
        ~mPredefinedFieldPanelBase();

        /*
        * 初始化控件
        */
        virtual void initInputs() = 0;

        /*
        * 控件设值
        */
        virtual void setInputsValue(std::shared_ptr<MechCaseData::mPredefinedFieldBase> fieldData) = 0;
        /*
        * 清空控件
        */
        virtual void clearInputs() = 0;


        virtual QVector<QWidget*> getRefStateInputs(FoldSection* fathertar) {
            return QVector<QWidget*>();
        };

        /*
        * 作用对象输入控件
        */
        virtual QVector<QWidget*>getTargetInputs(FoldSection* fathertar) = 0;

        /*
        * 值输入控件
        */
        virtual QVector<QWidget*>getValueInputs(FoldSection* fatherval) = 0;

        /*
        * 获取作用对象
        */
        virtual std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> getPickedData()
        {
            return std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>();
        };

        /*
        * 更新作用对象
        */
        void updateTarObject(QTreeWidget* treewidget, unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>_picks);

        /*
        * 清空pick
        */
        void clearPick();

        /*
        * 高亮
        */
        void setHighlight(const MxFunctions::PickDataType& settype, const std::vector<int>& ids, bool on);

        /*
        * 设置预定义场数据
        */
        void setPredefinedFieldData(std::shared_ptr<MechCaseData::mPredefinedFieldBase> fieldData, const int& caseId);

        virtual void deactivatePickAndCommit() {};
    signals:
        /*
        * 编辑完成
        */
        void sg_editFinished();

        /*
        * 拾取完成
        */
        void sg_pickFinished();

    protected:
        std::unordered_map<MxFunctions::SetType, std::vector<RenderPickResultData>>_pickeddata;
        std::shared_ptr<MechCaseData::mPredefinedFieldBase> _fieldData;
        int _caseId = -1;
        FoldSection* fatar_ = nullptr;
        FoldSection* faval_ = nullptr;
        FoldSection* refstate_ = nullptr;
    };
}
