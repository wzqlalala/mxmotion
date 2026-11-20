#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include <memory>

//QT
#include <QWidget>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QHash>
#include <QGridLayout>
#include <QMdiSubWindow>
#include <QStringListModel>
#include <QStackedWidget>
#include <QListWidget>
#include <QLabel>
#include <QTableWidget>
#include <QRadioButton>
#include <QPushButton>
#include <QLineEdit>
#include <QIntValidator>
#include <QDoubleValidator>
#include <QComboBox>
#include <QHeaderView>
#include <QInputDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QCoreApplication>
#include <algorithm>
#include <QStyledItemDelegate>
#include <QAction>
#include <QMenu>
#include <QApplication>
#include <QChar>

#include "mPanelBase.h"
#include "mPlyLaminate.h"

#include "mPhysicalFieldEnum.h"

using namespace MxFunctions;
using namespace MechPropertyData;
namespace MechPropertyData
{
    class mPropertyBase;
    class mTorsionBar;
    class mCrossBeam;
    
}
namespace MechCaseData
{
    class mNonStructuralQuality;
}
namespace MechPreData
{
    class mViewportPrivate;
}
namespace mxr
{
    struct BeamEditStruct;
}

namespace MechMainWindow
{
    class mPlyTableWidget1;

    //非结构质量面板
    class MMAINWINDOW_EXPORT mNonStructuralQualityPanel : public mPanelBase
    {
        Q_OBJECT

    public:
        /*
        * 默认构造函数
        */
        mNonStructuralQualityPanel(QWidget* parent = Q_NULLPTR);

        /*
        * 析构函数
        */
        ~mNonStructuralQualityPanel();


        /*
        * 新建非结构质量
        */
        void newNonStructuralQualityPanel();

        /*
        * 编辑属性(从树上点击过来的)
        */
        void editPropertyPanel(int propertyID = 0);

        /*
        * 取消编辑非结构重量面板
        */
        void uneditNonStructuralQualityPanel();

        /*
        类型和属性类型的组合变化
        */

    protected slots:
        void onComboChanged();
        void updateTableAndLayout();

        void slot_nonStructuralQualityChanged(QString value);

        void onAllEleBtnClicked();
        void onAddEleBtnClicked();
        void onReduceEleBtnClicked();
        void onDeleteEleBtnClicked();
        void slot_tableItemChanged(QTableWidgetItem* item);

        void clearHighlight();
        void slot_eleIDSelect(const QString& id);

        //视口拾取单元完成，将拾取到的单元ID填入当前行ID列
        void slot_pickedElementFinished();

    protected:
        void highlightRowFirstColumn(int row);

        void startElementPick();
        void stopElementPick();

        //校验ID列输入：需为模型中存在的单元且不与其他行重复，非法则还原并提示
        bool validateElementIdItem(QTableWidgetItem* item);

        //面板被隐藏（切到其它面板/关闭信息区）时结束拾取并恢复工具栏
        void hideEvent(QHideEvent* event) override;

        MxFunctions::Non _typeIndex = MxFunctions::Non::Union;
        MxFunctions::PhysicalFieldType _propIndex = MxFunctions::PhysicalFieldType::Physical_Property;


        //两个下拉框的组合方式
        int _iNonType = 1;
        QString _unionValue;

        std::map<QString, QString> _crossPropertyData;
        std::map<QString, QString> _elementData;
        std::map<QString, QString> _elementSetData;

        DataBinder* binder;

        int _nonStructuralQualityDataID = -1;
        //非结构质量数据
        MechCaseData::mNonStructuralQuality* _nonStructuralQualityData = nullptr;


        //总布局
        QVBoxLayout* _mainLayout;

        //类型下拉框
        QComboBox* _comboType;

        //属性类型下拉框
        QComboBox* _comboProperty;

        // 1）第一种组合的表格
        QTableWidget* _tableUnionToCrossPropety;

        // 单位非结构质量值标签和输入框（用于1对多+截面属性）
        QLabel* _valueLabel = nullptr;
        QLineEdit* _valueLineEdit = nullptr;

        int _clickRow = -1;

        //视口单元拾取回填
        QMetaObject::Connection _pickConnection;
        bool _applyingPick = false;
        int _pickTargetRow = -1;

        QToolButton* _allEleBtn;
        QToolButton* _addEleBtn;
        QToolButton* _reduceEleBtn;
        QToolButton* _deleteEleBtn;
        QStringListModel* _elementModel; //单元集的数据模型

    };

}
