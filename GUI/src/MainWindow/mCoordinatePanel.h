#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include "mxr/data/SelectDataStruct.h"
#include "mPhysicalFieldEnum.h"

#include <memory>
#include <set>

//QT
#include <QWidget>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QHash>
#include <QGridLayout>
#include <QLabel>
#include <QToolButton>
#include <QMdiSubWindow>
#include <QComboBox>

#include "mPanelBase.h"

#include "mCalcLineEdit.h"
#include "mPointPickerWidget.h"
#include "mDirectionSelectorWidget.h"

namespace MechPreData
{
    class mViewportModel;
}
namespace MechCoordinateData
{
    class mCoordinateBase;
}
namespace MechMainWindow
{
    struct CoordinateSnapshot
    {
        QVector3D x;
        QVector3D y;
        QVector3D z;
        QVector3D origin;
        int type;
        QString method;
        int coordinateId;
        int modelId;
        bool first;

        bool operator==(const CoordinateSnapshot& rhs)
        {
            if (!first)
            {
                return false;
                first = true;
            }
            return origin == rhs.origin &&
                x == rhs.x &&
                y == rhs.y &&
                z == rhs.z &&
                type == rhs.type &&
                method == rhs.method &&
                coordinateId == rhs.coordinateId &&
                modelId == rhs.modelId;
        }
    };
    class MMAINWINDOW_EXPORT mCoordinatePanel : public mPanelBase
    {
        Q_OBJECT
    public:
        explicit mCoordinatePanel(QWidget* parent = nullptr);

        //新建坐标系
        void newCoordinatePanel(MxFunctions::CoordinateType type = Cartesian, QVector3D origin = QVector3D(0, 0, 0));

        //编辑坐标系(从树上点击过来的)
        void editCoordinatePanel(int coordinateID = 0);

        //取消编辑坐标系	
        void uneditCoordinatePanel();

        void setDiscreteMode(bool discrete);
        void refreshDiscreteFieldCombo();

    private:
        //连接信号	
        void connectSig();

        //断开连接信号
        void disconnectSig();

        //更新数据
        void requestUpdate();

        //更新显示数据和渲染
        void updateCoordinateData();

        //清空拾取
        void clearPick();

        //通过三个行编辑框获取QVector3D
        QVector3D getVec3DByLineEdits(QVector<MxFunctions::mCalcLineEdit*> edits);

        //激活当前可见的第一个拾取控件
        void activateFirstVisiblePicker();

        //以"移除再插入"方式控制折叠块表单行的显隐（隐藏的行从布局中移除、不占空间，避免残留空白）
        void setFormRowPresent(FoldSection* fold, QWidget* label, QWidget* field, bool present);

        //跨列整行控件（无标签行，如方向控件）的显隐处理，行为同上
        void setFormRowPresent(FoldSection* fold, QWidget* widget, bool present);

        //失活方向控件内部的拾取控件（行隐藏/退出编辑时调用；激活由控件内部按模式自管理）
        void deactivateDirectionSelector(mDirectionSelectorWidget* selector);

    private slots:
        //类型切换
        void slot_onTypeChanged(int index);

        //定义方式切换
        void slot_onDefineChanged(int index);

        //主轴切换
        void slot_onMainAxisChanged(const QString& text);

        //主轴朝向切换
        //void slot_onMainDirChanged(const QString& text);

        //轴点切换
        void slot_onAxisPointChanged(const QString& text);

        //平面上的点切换
        void slot_onPlanePointChanged(const QString& text);

        //切换视口	
        void slot_subWindowActivated(int viewportID);

        //拾取信号
        void slot_finishedPick();

    private:
        QWidget* wrapWidget(QLayout* l) {
            QWidget* w = new QWidget();
            w->setLayout(l);
            return w;
        }

        QWidget* createXYZRow() {
            QWidget* w = new QWidget();
            QHBoxLayout* l = new QHBoxLayout(w);
            l->setContentsMargins(0, 0, 0, 0);
            l->addWidget(new QLineEdit()); // X
            l->addWidget(new QLineEdit()); // Y
            l->addWidget(new QLineEdit()); // Z
            return w;
        }

    private:
        QString _theta;

        //更新定时器
        QTimer* _updateTimer = nullptr;

        //坐标系数据
        MechCoordinateData::mCoordinateBase* _coorData;

        //当前坐标系快照
        CoordinateSnapshot _lastSnapshot;

        //拾取高亮数据改变的连接
        QMetaObject::Connection conn_geo;

        QVBoxLayout* _mainLayout;
        QComboBox* _comboType;
        QComboBox* _comboDefine;

        //所有单点拾取控件统一替换为 mPointPickerWidget
        mPointPickerWidget* _originPicker = nullptr; // 原点拾取
        mPointPickerWidget* _pt1Picker = nullptr;    // 轴点拾取
        mPointPickerWidget* _pt2Picker = nullptr;    // 平面点拾取

        //主轴/主轴朝向折叠块内的拾取控件替换为 mDirectionSelectorWidget
        mDirectionSelectorWidget* _mainDirSelector = nullptr; // 主轴方向
        mDirectionSelectorWidget* _dirDirSelector = nullptr;  // 主轴朝向方向

        QWidget* _rowMainAxis;
        QWidget* _rowDirAxis;
        QComboBox* _mainAxis, * _mainRef, * _dirAxis, * _dirRef;
        QPushButton* _mainReverse, * _dirReverse, * _pt1Reverse;

        QWidget* _rowPoint1;
        QWidget* _rowPoint2;
        QComboBox* _pt1, * _pt2;

        QLabel* _labelDirY;
        QLineEdit* _dirX, * _dirY, * _dirZ;

        //离散坐标系
        QLabel* _labelDefine;
        QLabel* _labelDiscreteField;
        QComboBox* _comboDiscreteField;
    };
}