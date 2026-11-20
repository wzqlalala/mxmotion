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
#include <QLineEdit>
#include <QStringList>
#include <QList>
#include <memory>

#include "mCalcLineEdit.h"
#include "mPanelBase.h"
#include "mMaterialPropBase.h"

//MxFunction
#include "mPhysicalFieldEnum.h"
#include "mModelStruct.h"

#include "mLoadPanelBase.h"

#include "mLoadCombined.h"

using namespace MechCaseData;
using namespace MechSetData;
namespace MechMainWindow
{
    class LoadTableWidget;

    class MMAINWINDOW_EXPORT mLoadCombinedPanel : public mLoadPanelBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mLoadCombinedPanel(QObject* parent = nullptr);

        /*
        * 析构函数
        */
        ~mLoadCombinedPanel();

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

        void saveTotalRatio();
        void saveTableData();
        void notifyLoadChanged();

        /*
         * 获取当前树上的作用对象
        */
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> getPickedData()override;

        /*
        * 表格初始化
        */
        void initTable();

        /*
        * 检查时域载荷与频域/声学频域载荷混合使用冲突
        */
        void checkLoadTypeConflict();

    public slots:
        void slot_combinedLoadTable();
    private:
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>>_picks;

        QVector<InputWidgetSet> inputWidgets;

        singleCheckBox _combinedBtn;

        LoadTableWidget* table_;

        QList<int> delRows;
    };

    class LoadTableWidget : public QWidget
    {
        Q_OBJECT
    public:
        explicit LoadTableWidget(QWidget* parent = nullptr);

        // ===================== 核心对外接口 =====================
        // 获取所有表格数据（系数 + 载荷名称）
        QList<QPair<double, QString>> getAllData() const;

        // 设置整个下拉框的选项（全覆盖）
        void setLoadOptions(const QStringList& options);

        // 追加一个载荷选项
        void addLoadOption(const QString& option);

        // 删除指定载荷选项
        void removeLoadOption(const QString& option);

        // 清空所有载荷选项
        void clearLoadOptions();

        // 获取当前所有载荷选项
        QStringList getLoadOptions() const;

        //获取当前所有下拉框当前所选的载荷
        QStringList getAllCurrentLoadNames() const;

        //设置单行下拉框
        void setCurrentComboText(int row, const QString& text);

        //获取所有系数
        QList<double> getAllRatios() const;

        // 设置第 row 行的比例系数
        void setRatio(int row, double value);

        //设置行数
        void setRowCount(int count);
    signals:
        //第一列系数系数编辑完成
        void ratioEditFinished(int row, double value);
        //第二列下拉框切换
        void loadNameChanged(int row, const QString& text);

        void rowAdded(int row); //新增的行号
        void rowRemoved(int row); //删除的行号

    private slots:
        void onRatioEditFinished();
        void onLoadNameChanged(const QString& text);

    private:
        void initUI();
        void addRow();
        void removeRow();
        void clearTable();
        void setRowByInput();

        // 内部：同步所有行的下拉框数据
        void syncAllComboBox();

        //单行透明
        void setWidgetTransparent(int row);
        //所有行透明
        void setAllWidgetTransparent();

    private:
        QTableWidget* m_table;
        QPushButton* m_btnAdd;
        QPushButton* m_btnRemove;
        QPushButton* m_btnClear;
        QLineEdit* m_editRowNum;

        QStringList m_loadOptions; // 载荷选项列表
    };
}