#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include "mxr/data/SelectDataStruct.h"
#include "mBasicEnum.h"

#include <memory>
#include <set>

#include "mCalcLineEdit.h"

//QT
#include <QWidget>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QHash>
#include <QGridLayout>
#include <QLabel>
#include <QToolButton>
#include <QTableWidget>
#include <QStringListModel>

#include "mPanelBase.h"

#include "mPhysicalFieldEnum.h"

//通用拾取控件
#include "mPickWidgetData.h"

namespace MechPreData
{
    class mViewportModel;
}
namespace MechSetData
{
    class mSetBase;
}
namespace MechMainWindow
{
	class mPickWidgetController;
	class SetDataPersistenceStrategy;

	//集合面板(包含数据)
	class MMAINWINDOW_EXPORT mSetPanel : public mPanelBase
	{
		Q_OBJECT

    public:
        /*
        * 默认构造函数
        */
        mSetPanel(QWidget* parent = Q_NULLPTR);

        /*
        * 析构函数
        */
        ~mSetPanel();

        /*
        * 新建集合
        */
        void newSetPanel();

        void newSetPanel(QString setName);

        //由中文名变英文名
        static QString getEnglishNameByChineseName(QString chineseName);
        /*
        * 编辑集合(从树上点击过来的)
        */
        void editSetPanel(int setID = 0);

        /*
        * 取消编辑集合
        */
        void uneditSetPanel();

        template<typename T>
		T* toType(MechSetData::mSetBase *base)
        {
            return dynamic_cast<T*>(base);
        };

    private:

		/*
		* 更新面板
		*/
		void updateSetPanel(QString type = "几何点", bool order = false);

        /*
        * 通过拾取模式获取集合类型字符串
        */
        QString getSetTypeStringByPickFilter(mxr::PickFilter pickFilter);

        /*
        * 通过拾取数据类型获取集合类型字符串
        */
        QString getSetTypeStringByPickDataType(MxFunctions::PickDataType pickFilter);

        /*
        * 确定集合
        */
        void okSet();

        /*
        * 清空集合
        */
        void clearSet();

        /*
        * 连接信号
        */
        void connectSig();

        /*
        * 断开连接信号
        */
        void disconnectSig();

        //****************通用拾取控件********************
        /*
        * 按当前面板类型创建集合对象（已存在则直接返回）
        * 拾取型集合延迟至首次拾取提交时创建，避免空集合导致完整性检查失败
        */
        MechSetData::mSetBase* ensureSetData();

        /*
        * 集合中文名 -> 集合类型
        */
        MxFunctions::SetType getSetTypeByChineseName(const QString& chineseName);

        /*
        * 配置拾取控件：按集合类型限定拾取类型 + 绑定集合存储策略
        */
        void setupPickWidget();

        /*
        * 激活拾取控件：反显当前集合数据并激活（激活即存储高亮）
        */
        void activatePickWidget();

        /*
        * 失活拾取控件并提交数据（切出面板/切换编辑对象时调用）
        */
        void deactivatePickWidget();

        /*
        * 校验 _setData 是否仍存活于集合管理器（树上删除集合会先释放对象后通知面板）
        * 已被删除时置空 _setData 并返回 false，防止解引用悬空指针
        */
        bool validateSetData();

        /*
        * 当前集合数据 -> 拾取控件数据（反显用）
        */
        PickWidgetData toPickWidgetData();

        /*
        * 从视口拾取容器直接捕获当前集合类型的数据
        * （与旧版"确定"读取路径一致，作为控件模型为空时的兜底数据源）
        */
        std::unordered_map<MxFunctions::PickDataType, std::vector<RenderPickResultData>> captureViewportBySetType();

        /*
        * 初始化频率点表格
        */
        void initFrequencyPointTabel();

        /*
        * 更新频率点表格
        */
        void updateFrequencyTable(bool deleteMap);

        /*
        * 初始化线段轮廓表格
        */
        void initSegContourTable();

    private slots:
        //切换视口	

		//拾取模式修改变化
		void slot_pickFilterChange();

        //通用拾取控件：数据变化（提交到集合并刷新统计）
        void slot_pickDataChanged();

        //通用拾取控件：激活态变化（失活时数据已提交，刷新树与状态）
        void slot_pickActiveChanged(bool active);

        //****************************频率点集表格**********************
        /*
        * 频率点输入框改变
        */
        void slot_lineEditChanged();

        /*
        * 数据改变
        */
        void slot_lineChanged(QTableWidgetItem* item);

        /*
        * 表格添加一行
        */
        void slot_addLine();

        /*
        * 表格删除多行
        */
        void slot_deleteLine();

        /*
        * 清空表格
        */
        void slot_clearTabel();
        /*
        * 表格数据改变型号连接与断开
        */
        void connectFrequencyPointTableSig();
        void disconnectFrequencyPointTableSig();

        //****************自由度节点集合****************************
        void slot_highLightBtnClicked();

        void slot_comboDofChanged(const QString &text);

        void setHighlight(const MxFunctions::SetType& settype, const std::vector<int>& ids, bool on);

        void clearHighlight();

        /*******************面板集合*************************************/
        void slot_elementItemClicked(QListWidgetItem* item);

        void highlightElement(int elementId);

        void appendElementPanelMap(int elementId, int row);

        //获取所有的单元集
        void slot_allElementSetBtn();
        void slot_addElementSetBtn();
        void slot_reduceElementSetBtn();
        void slot_clearElementSetBtn();

        // 根据当前面板集合类型刷新列表显示
        void refreshPanelListByCollectionType();

        // 解析表面输入框值改变时自动保存
        void slot_analyticalSurfaceDataChanged();
        void slot_contourAddLine();
        void slot_contourDeleteLine();
        void slot_contourClearTable();
        void slot_contourLineChanged(QTableWidgetItem* item);

    private:
        // 解析表面集辅助方法
        void loadAnalyticalSurfaceData();
        void saveAnalyticalSurfaceData();
        void updateAnalyticalSurfaceFolds();
        void createSymbol();

    protected:

        //真正的集合数据
        MechSetData::mSetBase* _setData;

        //总布局
        QVBoxLayout* _mainLayout;

        //类型
        QLabel* _labelType;
        QLabel* _hintLabel;
        QComboBox* _comboLabelType;

        //是否排序数据
        QLabel* _labelOrder;
        QCheckBox* _checkboxOrder;

		//拾取对象：通用拾取控件（不可选择集合，仅存储高亮）
		mPickWidgetController* _pickWidget = nullptr;

		//集合存储策略引用（未激活态数据变化时直接持久化，防失活时空覆盖）
		std::shared_ptr<SetDataPersistenceStrategy> _setStrategy;

        //频率点
        QLabel* _labelFrequencyPoint;
        QLineEdit* _frequencyPoint;
        QMetaObject::Connection m_lineEditChangeConn;

        //频率表格
        QTableWidget* _tableFrequencyPoint;
        QWidget* _widgetFrequencyPoint;
        QPushButton* _addBtn;
        QPushButton* _reduceBtn;
        QPushButton* _clearBtn;
        QMetaObject::Connection m_tableChangeConn;

        //自由度节点集相关统计
        QHBoxLayout* _dofLayout;
        QLabel* _labelDofNode;
        QLineEdit* _lineEditDofNode;
        QPushButton* _btnHighLight;
        QLabel* _labelDof;
        QComboBox* _comboDof;

        //面板集合
        QLabel* _labelInfo;
        QComboBox* _comboPanel;
        QListWidget* _listAssignElement;
        QToolButton* _allEleBtn;
        QToolButton* _addEleBtn;
        QToolButton* _reduceEleBtn;
        QToolButton* _deleteEleBtn;
        QStringListModel* _elementModel; //单元集的数据模型
        bool enableCollectNode = false; //false 面板集仅管理单元集、true时增加管理节点集


        //其他统计数据(几何面为表面积、几何线为长度、其余的隐藏)
        QLabel* _labelStates;
        QLabel* _labelStatesData;

        //当前模型
        //MechPreData::mViewportModel* _currentViewportModel;

        //拾取高亮数据改变的连接
        QMetaObject::Connection conn_geo;
        QMetaObject::Connection conn_pickFiter;

        QString m_symbolName;
        //旋转解析
        QLabel* _labelCoordinateA, * _labelCoordinateB;
        QLabel* _labelCoordinateRZ1, * _labelCoordinateRZ2;
        QLineEdit* _Xa, * _Ya, * _Za;
        QLineEdit* _Xb, * _Yb, * _Zb;
        QLineEdit* _r1, * _z1;
        QLineEdit* _r2, * _z2;

        //拉伸解析
        QLabel* _labelOrigin, * _labelXAxis, * _labelZNegAxis;
        QLineEdit* _Ox, * _Oy, * _Oz;
        QLineEdit* _Xx, * _Xy, * _Xz;
        QLineEdit* _Zx, * _Zy, * _Zz;
        QLabel* _labelStartLocal;
        QLineEdit* _Sx, * _Sy;
        QLabel* _labelEndLocal, * _labelExtent;
        QLineEdit* _Ex, * _Ey, * _El;

        //解析线段
        QLabel* _labelSegOrigin, * _labelSegXAxis, * _labelSegZNegAxis;
        QLineEdit* _segOx, * _segOy, * _segOz;
        QLineEdit* _segXx, * _segXy, * _segXz;
        QLineEdit* _segZx, * _segZy, * _segZz;
        QLabel* _labelSegStartLocal;
        QLineEdit* _segSx, * _segSy;

        QWidget* _widgetContourBtn;
        QPushButton* _contourAddBtn;
        QPushButton* _contourReduceBtn;
        QPushButton* _contourClearBtn;
        QTableWidget* _tableSegContour;
        QMetaObject::Connection m_contourTableChangeConn;

    };
}