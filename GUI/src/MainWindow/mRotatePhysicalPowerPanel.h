#pragma once
#pragma execution_character_set("utf-8")
#include "mmainwindow_global.h"

#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
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
#include <map>
#include <algorithm>
#include <QVariant>
#include <QVector3D>

#include "mCalcLineEdit.h"
#include "mPanelBase.h"
#include "mMaterialPropBase.h"

//MxFunction
#include "mPhysicalFieldEnum.h"
#include "mModelStruct.h"

#include "mLoadPanelBase.h"
#include "mRotatePhysicalPower.h"
#include "mCommonTableWidget.h"
#include "mDiscreteDataControl.h"
#include "mPickWidgetController.h"
#include "mDirectionSelectorWidget.h"

using namespace MechCaseData;
using namespace MechSetData;

namespace MechMainWindow
{
    class MMAINWINDOW_EXPORT mRotatePhysicalPowerPanel : public mLoadPanelBase
    {
        Q_OBJECT

    signals:
        void sg_rebuildPanel();

    public:
        /*
        * 构造函数
        */
        mRotatePhysicalPowerPanel(QWidget* parent = nullptr);

        /*
        * 析构函数
        */
        ~mRotatePhysicalPowerPanel();

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
        * 载荷引用状态输入控件
        */
        QVector<QWidget*> getRefStateInputs(FoldSection* fathertar) override;

        /*
        * 获取当前树上的作用对象
        */
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> getPickedData()override;

        /*
        * 切出面板时失活并提交拾取数据
        */
        void deactivatePickAndCommit();

        /*
        * 连接单位制响应信号
        */
        void connectUnitSystemResponse()override;

        /*
        * 断开单位制响应信号
        */
        void disconnectUnitSystemResponse()override;

    private slots:
        void slot_rotateTypeChanged(int idx);
        void slot_defineTypeChanged(int idx);
        void slot_discreteDataChanged();
        void slot_discreteCellEdited(const QPair<int, int>& key, int valueIndex, const QString& text);
        void slot_funcChanged(int idx);
        void slot_unitSystemResponse(UnitType type, QVector<int> idx);

    private:
        void updateDirectionArrowRender();
        void updateOpenedTable();
        void rebuildValueLayout();
        void filterPicksToElementOnly();
        void applyAllowedPickTypes();
        void saveUniformValue();
        void saveFunc();
        void saveScale();
        void notifyLoadChanged();
        void updateLoadSymbolRealtime();
        int getFunc();
        void saveDirectionWidgetState();
        void ensureDirectionWidget();

        comboBoxWithPushBtn combofunc;
        InputWidgetSet _scale;

        singleComboBox _rotateTypeCombo;
        singleComboBox _defineTypeCombo;

        QWidget* _tableRowWidget = nullptr;
        QLabel* _tableLabel = nullptr;
        mDiscreteDataControl* _discreteControl = nullptr;

        QHash<QString, int>_functable;

        std::shared_ptr<mRotatePhysicalPower> _currentLoad;

        mPickWidgetController* _pickWidgetController = nullptr;

        // 方向选择控件（统一赋值下替代原起始点/方向点参考）
        mDirectionSelectorWidget* _directionWidget = nullptr;

        // 实时方向箭头渲染相关（起点 -> 方向点）
        static const QString s_dirArrowRenderKey;
        static const QVector3D s_dirArrowColor;
    };
}
