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

#include "mLoadBoundaryBase.h"
#include "mUnitSystemManager.h"
using namespace MechCaseData;
using namespace MechSetData;
namespace MechMainWindow
{
    class MMAINWINDOW_EXPORT mLoadPanelBase : public mPanelBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mLoadPanelBase(QWidget* parent = nullptr);

        /*
        * 析构函数
        */
        ~mLoadPanelBase();

        /*
        * 初始化控件
        */
        virtual void initInputs() = 0;

        /*
        * 控件设值
        */
        virtual void setInputsValue(std::shared_ptr<MechCaseData::mLoadBoundaryBase>loadData) {};
        /*
        * 清空控件
        */
        virtual void clearInputs() = 0;

        /*
        * 退出编辑界面时的清理（载荷面板为复用单例、不销毁，需要收尾的资源在此释放）
        */
        virtual void unEditInputs() {};

        /*
        * 载荷引用状态输入控件
        */
        virtual QVector<QWidget*>getRefStateInputs(FoldSection* fathertar) {
            return QVector<QWidget*>();
        };

        /*
        * 作用对象输入控件
        */
        virtual QVector<QWidget*>getTargetInputs(FoldSection* fathertar) {
            return QVector<QWidget*>();
        };

        /*
        * xxx定义输入控件
        */
        virtual QVector<QWidget*>getValueInputs(FoldSection* fatherval) {
            return QVector<QWidget*>();
        };

        /*
        * 获取当前树上的作用对象
        */
        virtual std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> getPickedData() {
            return std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>();
        };

        /*
        * 连接单位制响应信号
        */
        virtual void connectUnitSystemResponse() {};

        /*
        * 断开单位制响应信号
        */
        virtual void disconnectUnitSystemResponse() {};

        /*
        * 设置载荷数据
        */
        void setLoadData(std::shared_ptr<MechCaseData::mLoadBoundaryBase> loadData, const int& caseId);

        /*
        * 提交通用拾取控件信息
        */
        virtual void deactivatePickAndCommit() {};
    protected:
        std::shared_ptr<MechCaseData::mLoadBoundaryBase> _loadData;
        int _caseId;
        //归属页面
        FoldSection* fatar_ = nullptr;
        FoldSection* faval_ = nullptr;
        FoldSection* refstate_ = nullptr;
    };
}
