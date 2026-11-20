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

#include "mRotateAnalysePanelBase.h"

#include "mLoadRotationalACC.h"
#include "mDirectionSelectorWidget.h"
#include "mRotateACCTableWidget.h"
#include "mTableDataWindow.h"
#include "mDockLayoutManager.h"

using namespace MechCaseData;
using namespace MechSetData;

namespace MechMainWindow
{
    class MMAINWINDOW_EXPORT mLoadRotationalACCPanel : public mRotateAnalysePanelBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mLoadRotationalACCPanel(QObject* parent = nullptr);

        /*
        * 析构函数
        */
        ~mLoadRotationalACCPanel();

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
        void setInputsValue(std::shared_ptr<MechCaseData::mRoatateAnalyseBase>rotateData)override;

        /*
        * 值输入控件
        */
        QVector<QWidget*> getValueInputs(FoldSection* fatherval) override;

        /*
        * 作用对象输入控件
        */
        QVector<QWidget*> getTargetInputs(FoldSection* fathertar)override;

        /*
        * 存值
        */
        void storeLoadValue(std::shared_ptr<MechCaseData::mLoadRotationalACC> load);

        /*
        * 获取当前树上的作用对象
        */
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> getPickedData()override;

        int getLoadBoundId();

        void updateDirectionArrowRender();

        // 通知外部刷新树/渲染（数据已实时保存到内部对象）
        void notifyRotateAnalyseChanged();

    private slots:
        void slot_roPartChanged(QString partName);
        void slot_curveTypeChanged(QString text);
        void slot_roACCTable(bool connFlag = true);

    private:
        void saveDirectionWidgetState();
        void initTable();
        void updateTableHeaders();

        // 按曲线类型保存/恢复表格数据，确保切换页签后回显不丢失
        void saveCurrentTableData();
        void restoreTableDataByType(int curveType);
        void syncCurveTypeFromData();
        void applyTableWindowStyleByType(int curveType);

        // 常数模式下使用默认 mTableWidget，需要单独保存/恢复
        void saveDefaultTableData();
        void restoreDefaultTableData();

        // 按曲线类型统一保存/恢复表格数据（区分默认/自定义表格）
        void saveTableDataByCurveType(int curveType);
        void restoreTableDataByCurveType(int curveType);

        int curveTypeFromText(const QString& text) const;
        QString textFromCurveType(int curveType) const;

        // 兼容旧数据：裁剪缓存中旧格式的“起始点/方向点”列，仅保留数据列
        static void trimLegacyTableCache(QStringList& headers, QList<QStringList>& data, int keepCols);

        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>_picks;

        mLoadRotationalACC* mLoadRotationalACC_ = nullptr;

        mDirectionSelectorWidget* _directionWidget = nullptr;

        singleComboBox _accCurveTypeCombo; // 旋转加速度类型下拉框
        singleCheckBox _tableBtn;          // 表格打开按钮
        RotateACCTableWidget* _tableWidget = nullptr; // 自定义表格

        // 各曲线类型对应的表格数据缓存，用于页签切换后回显
        QStringList _constantTableHeaders;
        QList<QStringList> _constantTableData;
        QStringList _timeCurveTableHeaders;
        QList<QStringList> _timeCurveTableData;

        // 常数模式下默认 mTableWidget 数据变化信号连接
        QMetaObject::Connection _defaultTableConn;

        QVector<singleComboBox> comboBoxs;

        static const QString s_dirArrowRenderKey;
        static const QVector3D s_dirArrowColor;
        QHash<QString, int>_loadboundtable; //载荷与边界统一，名称与id都唯一，所以放到一个哈希表即可

        int index = 0;
    };

}
