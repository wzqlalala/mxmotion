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

using namespace MechCaseData;
using namespace MechSetData;
namespace MechMainWindow
{
    class MMAINWINDOW_EXPORT mBoundaryPanelBase : public mPanelBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mBoundaryPanelBase(QWidget* parent = nullptr);

        /*
        * 析构函数
        */
        ~mBoundaryPanelBase();

        /*
        * 初始化控件
        */
        virtual void initInputs() = 0;

        /*
        * 控件设值
        */
        virtual void setInputsValue(std::shared_ptr<MechCaseData::mLoadBoundaryBase>boundaryData) = 0;
        /*
        * 清空控件
        */
        virtual void clearInputs() = 0;

        virtual QVector<QWidget*> getRefStateInputs (FoldSection* fathertar){
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
        * 设置边界数据
        */
        void setBoundaryData(std::shared_ptr<MechCaseData::mLoadBoundaryBase> boundaryData, const int& caseId);

        virtual void deactivatePickAndCommit() {};
    protected:
        std::shared_ptr<MechCaseData::mLoadBoundaryBase> _boundaryData;
        int _caseId;
        //归属页面
        FoldSection* fatar_ = nullptr;
        FoldSection* faval_ = nullptr;
        FoldSection* refstate_ = nullptr;
    };
}
