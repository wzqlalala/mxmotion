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

#include "mRoatateAnalyseBase.h"
using namespace MechCaseData;
using namespace MechSetData;
namespace MechMainWindow
{
    class MMAINWINDOW_EXPORT mRotateAnalysePanelBase : public mPanelBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mRotateAnalysePanelBase(QWidget* parent = nullptr);

        /*
        * 析构函数
        */
        ~mRotateAnalysePanelBase();

        /*
        * 初始化控件
        */
        virtual void initInputs() = 0;

        /*
        * 控件设值
        */
        virtual void setInputsValue(std::shared_ptr<MechCaseData::mRoatateAnalyseBase> rotateData) {};
        /*
        * 清空控件
        */
        virtual void clearInputs() = 0;

        /*
        * 状态输入控件
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
        * 定义输入控件
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
        * 更新作用对象
        */
        void updateTarObject(QTreeWidget* treewidget, unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> _picks);

        /*
        * 清空pick
        */
        void clearPick();

        /*
        * 高亮
        */
        void setHighlight(const MxFunctions::PickDataType& settype, const std::vector<int>& ids, bool on);

        /*
        * 连接单位制响应信号
        */
        virtual void connectUnitSystemResponse() {};

        /*
        * 断开单位制响应信号
        */
        virtual void disconnectUnitSystemResponse() {};

        /*
        * 获取拾取矢量数据
        */
        bool getPickVecData(QVector3D& point);

        /*
        * 获取拾取的点数据
        */
        bool getPickPointData(QVector3D& point);

        /*
        * 设置旋转分析数据
        */
        void setRotateAnalyseData(std::shared_ptr<MechCaseData::mRoatateAnalyseBase> rotateData, const int& caseId);
    signals:
        /*
        * 编辑完成
        */
        void sg_editFinished();

        /*
        * 拾取完成
        */
        void sg_pickFinished();

    private slots:

        /*
        * 智能拾取完毕
        */
        void slot_finishedPick();

    protected:
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>_pickeddata;
        std::shared_ptr<MechCaseData::mRoatateAnalyseBase> _rotateData;
        int _caseId;
        //归属页面
        FoldSection* fatar_ = nullptr;
        FoldSection* faval_ = nullptr;
        FoldSection* refstate_ = nullptr;
    };
}
