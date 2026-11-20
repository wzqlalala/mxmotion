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
#include <QToolButton>
#include <QIcon>
#include <QCoreApplication>
#include <algorithm>
#include <functional>
#include <QStyledItemDelegate>
#include <QAction>
#include <QMenu>
#include <QApplication>
#include <QChar>

#include "mPanelBase.h"
#include "mPlyLaminate.h"

#include "mPhysicalFieldEnum.h"
#include "mOtherFunction.h"

using namespace MxFunctions;
using namespace MechPropertyData;
namespace MechPropertyData
{
    class mPropertyBase;
    class mTorsionBar;
    class mCrossBeam;
}
namespace MechSectionShapeData
{
    class mSectionShapeBase;
}
namespace MechPreData
{
	class mViewportPrivate;
}
namespace mxr
{
	struct BeamEditStruct;
}
namespace MechPreData
{
	class PlyViewportWidget;
}
namespace MechMainWindow
{
    class mPlyTableWidget;
    class BeamSectionStage;      //梁截面参数编辑区(参数+图纸+快速新建)
    class BeamElementPanel;      //梁单元参数面板(单元表格+拾取+批量编辑)
    class AnchoredPopup;         //吸附式弹窗

    //截面属性面板
    class MMAINWINDOW_EXPORT mPropertyPanel : public mPanelBase
    {
        Q_OBJECT

    public:
        enum FoldIndex {
            Fold_Material = 0,      // 材料指派
            Fold_Scope,             // 作用域
            Fold_BeamSection,       // 梁截面参数
            Fold_Param,             // 参数
            Fold_BeamElement,       // 梁单元参数
            Fold_Coordinate,        // 参考坐标
            Fold_Spring,            // 弹簧单元参数
            Fold_PlyType,           // 铺层类型定义
            Fold_MaterialDir,       // 材料方向
            Fold_PlyAssignment,     // 铺层定义
            Fold_SectionDefine,     // 截面定义
            Fold_SpringCategory,     // 弹簧类型
            Fold_MacroEquivalentSection,     // 宏观等效截面定义
        };
        /*
        * 默认构造函数
        */
        mPropertyPanel(QWidget* parent = Q_NULLPTR);

        /*
        * 析构函数
        */
        ~mPropertyPanel();

        template<typename T>
        T* toType(MechPropertyData::mPropertyBase* base)
        {
            return dynamic_cast<T*>(base);
        };

        /*
        * 新建属性
        */
        void newPropertyPanel(PropertyType type);

        /*
        * 编辑属性(从树上点击过来的)
        */
        void editPropertyPanel(int propertyID = 0);

        /*
        * 取消编辑属性
        */
        void uneditPropertyPanel();

        /*
        * 获取坐标系
        */
        int getCoor();


    private slots:

        //材料下拉框变化
        void slot_matComboBoxChanged(const QString& text);

        //部件集合下拉框变化
        void slot_partSetComboBoxChanged(const QString& text);

        //梁截面形状轮廓类型下拉框变化
        void slot_beamTypeComboBoxChanged(const QString& text);

        //截面形状下拉框变化
        void slot_beamComboBoxChanged(const QString& text);

        //添加槽函数
        void slot_allPartBtn();

        //添加部件槽函数
        void slot_addPartBtn();

        //减少部件槽函数
        void slot_reducePartBtn();

        //清空部件槽函数
        void slot_clearPartBtn();

        //跳转到新建截面形状槽函数(并且获取最新创建的材料然后存到属性数据中)
        void slot_newMaterial();

        //跳转到新建截面形状槽函数(并且获取最新创建的材料然后存到属性数据中)
        void slot_newPropertyShape();

        //切换视口
        void slot_subWindowActivated(int viewportID);

        //拾取信号
        void slot_finishedPick();

        //坐标系存值
        void slot_coorStore();

        /*
        * 表格连接
        */
        void slot_tableData(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title);

        /*
        * 铺层类型下拉框切换
        */
        void slot_plyTypeChanged(int idx);

        /*
        * 直接定义or层合板
        */
        void onRadioToggled();

        /*
        *材料方向坐标系下拉框切换
        */
        void slot_materialDirCoor();

        /*
        * 相应表格槽函数
        */
        void onPlyRowAdded(int row);
        void onPlyRowRemoved(int row);
        void onPlyTableCleared();
        void onPlyMaterialChanged(int row, const QString& material);
        void onPlyThicknessChanged(int row, const QString& value);
        void onPlyAngleChanged(int row, const QString& value);
        void onPlyIntePointChanged(int row, const QString& value);

        /*
        * 铺层显示
        */
        void slot_showPlyWindow();
    protected:

        /*
        * 事件过滤：变截面位置行点击/聚焦时选中该行
        */
        bool eventFilter(QObject* watched, QEvent* event) override;

        /*
        * 重新建立层合结构参数折叠块
        */
        void updatePlyLaminatePanel();

        void plyTableShow();

        /*
        * 更新材料方向坐标系下拉框
        */
        void updatematerialDirCoor();

        /*
        * 更新材料方向折叠块
        */
        void updatematerialDirFold(int coorid);

        /*
        * 更新铺层定义折叠块
        */
        void updatePlyLamDefineFold();

        /*
        * 更新界面
        */
        void updatePanel();

        /*
        * 更新部件集合
        */
        void updatePartSetCombobox();

        /*
        * 更新材料
        */
        void updateMatCombobox(QComboBox* combobox);

        /*
        * 更新坐标系
        */
        void updateCoordiCombobox(QComboBox* combobox);

        /*
        * 更新截面形状
        */
        void updatePropShapeCombobox(QComboBox* combobox);

        /*
        * 连接信号
        */
        void connectSig();

        /*
        * 断开连接信号
        */
        void disconnectSig();

        /*
        * 更新部件的属性(返回部件id)
        */
        int updatePartProperty(QString partName);

        /*
        * 更新截面面板
        */
        void updateSectionPanel();

        /*
        * 更新材料页面
        */
        void updateMatPanel();

        /*
        * 更新坐标系页面
        */
        void updateCoorPanel();

        /*
        * 更新赋予页面
        */
        std::vector<int> updatePartAssignPanel();

        /*
        * 更新参数页面
        */
        void updateParamPanel();

        /*
        * 更新弹簧参数页面
        */
        void updateSpringParamPanel();

        //========== 梁截面参数面板(新版，参考侧栏原型) ==========
        /*
        * 构建梁截面参数折叠块内容
        */
        void buildBeamSectionFold(FoldSection* fold);

        /*
        * 更新梁截面参数面板(一致/变化两种模式整体刷新)
        */
        void updateBeamSectionPanel();

        /*
        * 同步一致截面界面(下拉/参数区)
        */
        void syncUniformSectionUI();

        /*
        * 同步变化截面界面(统一形状/位置行/参数区)
        */
        void syncVariableSectionUI();

        /*
        * 重建变截面位置行
        */
        void renderStationRows();

        /*
        * 校验变截面位置(0~1、升序、不重复)
        */
        bool validateStations(bool focusFirst);

        /*
        * 选中某个位置行并展示其截面参数
        */
        void selectStation(int row);

        /*
        * 在当前位置后插入位置
        */
        void insertStation();

        /*
        * 删除当前选中的中间位置
        */
        void removeStation();

        /*
        * 清空中间位置，保留A/B端
        */
        void clearStations();

        /*
        * 切换变截面统一形状
        */
        void changeVariableShape(MxFunctions::SecType type);

        /*
        * 更新变截面统一形状按钮显示
        */
        void syncVariableShapeButton();

        /*
        * 更新插入/删除/清空按钮可用状态
        */
        void updateStationButtons();

        /*
        * 取某位置行的位置输入框(无则返回nullptr)
        */
        QLineEdit* stationPosEdit(int row);

        /*
        * 取某位置行关联的截面形状id(无则-1)
        */
        int stationShapeId(int row);

        /*
        * 当前参数编辑区对应的截面形状(一致:当前截面；变化:当前位置截面)
        */
        MechSectionShapeData::mSectionShapeBase* currentEditSection();

        /*
        * 快速新建截面形状(直接写入管理器，不跳转截面形状窗口)
        */
        MechSectionShapeData::mSectionShapeBase* quickCreateSectionShape(MxFunctions::SecType type);

        /*
        * 离开参数编辑前的校验(参数非法则阻止切换)
        */
        bool canLeaveSectionEditor();

        /*
        * 构建梁单元参数折叠块内容
        */
        void buildBeamElementFold(FoldSection* fold);

        /*
        * 同步梁单元参数面板(作用域、表格)
        */
        void syncBeamElementPanel();

        //========== 吸附式弹窗 ==========
        /*
        * 打开梁截面定义吸附弹窗(已打开则关闭)
        */
        void toggleBeamSectionPopup();

        /*
        * 打开梁单元参数吸附弹窗(已打开则关闭)
        */
        void toggleBeamElementPopup();

        /*
        * 关闭全部吸附弹窗(切换属性/取消编辑时)
        */
        void closeBeamPopups();

        /*
        * 刷新条目入口摘要(截面名称/单元数)
        */
        void updateBeamEntrySummaries();

        /*
        * 吸附弹窗关联的拾取是否进行中(外部点击不关闭弹窗)
        */
        bool beamPopupPickActive();

        //给部件赋予单元
        void assignElement(std::vector<int> ids);

        //高亮
        void highlightPart(std::vector<int> ids);

        //不高亮
        void unHighlightPart(std::vector<int> ids);

        //更新角度渲染(角度变化、部件增减、截面形状修改、截面形状参数修改都会触发)
        void updateRotateAngleRend();

        //获取部件默认初始平面向量
        std::vector<QVector3D> getDefaultUpVector(std::vector<int> parIds);

        //更新角度渲染
        void updateRotateRender();

        //更新壳厚度渲染
        void updateThickRender();

        //角度改变信号连接
        void connectAngleRotateUpdate();

        //加载弹簧单元参数折叠块
        void appendSpringFoldSection();

        //更新截面属性状态
        void updateDynamicInfoData();

        //更新截面定义中的标签
        void updatePropertyDefine();

    protected:

        //分析工况数据
        MechPropertyData::mPropertyBase* _propertyData = nullptr;

        //总布局
        QVBoxLayout* _mainLayout;

        //截面类别（一维截面、二维截面、三维截面、铺层截面）
        QLabel* _lblVariety;

        //截面类型（杆、梁、管等等）
        QLabel* _lblType;

        //材料下拉框
        QComboBox* _comboMat;

        //坐标系下拉框
        QComboBox* _comboCoordinate;

        //部件集合下拉框
        QComboBox* _comboPartSet;

        //截面形状的数据模型
        QStringListModel* _propShapeModel;

        //部件的数据模型
        QStringListModel* _partModel;

        //赋予属性的部件列表
        QListWidget* _listAssignPart;

        //梁截面形状轮廓类型
        QComboBox* _comboBeamType;

        //一致截面：梁截面下拉框
        QComboBox* _comboPropShape;

        //一致截面：快速新建按钮
        QPushButton* _btnQuickCreate;

        //一致截面选择行(轮廓类型为"一致"时显示)
        QWidget* _uniformSelectionRow;

        //变化截面选择行(轮廓类型为"变化"时显示)
        QWidget* _variableSelectionRow;

        //变化截面：统一截面形状按钮
        QToolButton* _btnVariableShape;

        //变化截面：插入/删除/清空位置按钮
        QPushButton* _btnInsertStation = nullptr;
        QPushButton* _btnDeleteStation = nullptr;
        QPushButton* _btnClearStation = nullptr;

        //变化截面：位置行区域
        QWidget* _stationWidget;

        //变化截面：位置行网格布局
        QGridLayout* _stationRowsLayout;

        //变化截面：位置错误提示
        QLabel* _stationErrorLabel;

        //截面参数错误提示
        QLabel* _beamErrorLabel;

        //梁截面参数编辑区(参数+图纸+快速新建)
        BeamSectionStage* _beamStage;

        //当前选中的位置行(变化截面)
        int _activeStation = 0;

        //变化截面统一形状类型
        MxFunctions::SecType _variableShape = MxFunctions::Sec_H;

        //变化截面切换截面的位置列表暂存(一致↔变化来回切换不丢数据)
        QStringList _stashCrossPositions;

        //变化截面切换的各行截面id暂存
        QList<int> _stashCrossShapeRows;

        //一致截面切换时的截面id暂存(一致/变化各自一套数据，互不干涉)
        int _stashUniformShapeId = -1;

        //变化截面切换形状时缓存各行历史截面(行号->形状类型->截面id)
        QHash<int, QHash<int, int>> _stationShapeCache;

        //梁单元参数面板
        BeamElementPanel* _beamElementPanel = nullptr;

        //梁截面定义：属性面板条目按钮与摘要
        QPushButton* _btnSectionEntry = nullptr;
        QLabel* _entrySectionSummary = nullptr;

        //梁单元参数：属性面板条目按钮与摘要
        QPushButton* _btnElementEntry = nullptr;
        QLabel* _entryElementSummary = nullptr;

        //吸附式弹窗(梁截面定义/梁单元参数)
        AnchoredPopup* _beamSectionPopup = nullptr;
        AnchoredPopup* _beamElementPopup = nullptr;

        //拾取信号连接
        QMetaObject::Connection conn;

        //方向旋转信号链接
        QMetaObject::Connection angleConn;

        //方向
        std::shared_ptr<mCalcLineEdit> _angleLineEdit;

        //上次拾取数据
        std::vector<int> _lastPickData;

        // 控件所需要的数据
        std::shared_ptr<mxr::BeamEditStruct> _beam_struct;

        //弹簧类型下拉框
        QComboBox* _springCategoryCombo;

        //单自由度弹簧参数控件
        QLabel* _springLableK;
        mCalcLineEdit* _springLineK;
        QLabel* _springLableGE;
        mCalcLineEdit* _springLineGE;
        QLabel* _springLableS;
        mCalcLineEdit* _springLineS;

        //cbush表格行头与列头
        std::vector<QStringList> _hor;
        std::vector<QStringList> _ver;

        //铺层类型下拉框
        QComboBox* _comboPlyType;

        //铺层定义两个圆点选择
        QRadioButton* _radDefine;
        QRadioButton* _radLaminate;

        //铺层材料方向坐标系
        QComboBox* _comboPlyCoord;

        //铺层表格
        mPlyTableWidget* m_plyTableWidget = nullptr;

        //铺层显示窗口(带交互的视口包装)
        MechPreData::PlyViewportWidget* _plyViewport = nullptr;

        QComboBox* _comboMacro;

    };

    // 铺层表格类
    // 材料下拉委托
    class MaterialDelegate : public QStyledItemDelegate
    {
        Q_OBJECT
    public:
        using BatchCallback = std::function<void(int, const QString&)>;

        MaterialDelegate(QStringList materials, BatchCallback callback = nullptr, QObject* parent = nullptr)
            : QStyledItemDelegate(parent), m_materials(materials), m_callback(callback) {}

        QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override
        {
            QComboBox* combo = new QComboBox(parent);
            combo->addItems(m_materials);
            combo->setCurrentText(index.data().toString());
            QTimer::singleShot(0, combo, &QComboBox::showPopup);

            // 当下拉框选项变化时，立即同步到当前列所有选中的单元格
            if (m_callback)
            {
                connect(combo, QOverload<const QString&>::of(&QComboBox::currentTextChanged),
                    this, [this, index](const QString& text) {
                        if (m_callback) m_callback(index.column(), text);
                    });
            }
            return combo;
        }

        void setEditorData(QWidget* editor, const QModelIndex& index) const override
        {
            QComboBox* combo = qobject_cast<QComboBox*>(editor);
            combo->setCurrentText(index.data().toString());
        }

        void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override
        {
            QComboBox* combo = qobject_cast<QComboBox*>(editor);
            model->setData(index, combo->currentText());
        }

        void updateEditorGeometry(QWidget* editor, const QStyleOptionViewItem& option, const QModelIndex& index) const override
        {
            editor->setGeometry(option.rect);
        }

    private:
        QStringList m_materials;
        BatchCallback m_callback;
    };

    // 允许清空的数字校验器：空串视为合法(QDoubleValidator/QIntValidator默认拒绝空串，
    // 编辑器会以不可接受输入关闭而丢弃提交，单元格回退为旧值，表现为"删不掉数据")
    class AllowEmptyDoubleValidator : public QDoubleValidator
    {
    public:
        using QDoubleValidator::QDoubleValidator;
        State validate(QString& input, int& pos) const override
        {
            if (input.simplified().isEmpty()) return Acceptable;
            return QDoubleValidator::validate(input, pos);
        }
    };

    class AllowEmptyIntValidator : public QIntValidator
    {
    public:
        using QIntValidator::QIntValidator;
        State validate(QString& input, int& pos) const override
        {
            if (input.simplified().isEmpty()) return Acceptable;
            return QIntValidator::validate(input, pos);
        }
    };

    // 数字编辑委托
    class NumberDelegate : public QStyledItemDelegate
    {
        Q_OBJECT
    public:
        enum NumberType { Double, IntOdd };
        NumberDelegate(NumberType type, QObject* parent = nullptr)
            : QStyledItemDelegate(parent), m_type(type) {}

        QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override
        {
            QLineEdit* edit = new QLineEdit(parent);
            edit->setText(index.data().toString());

            if (m_type == Double) {
                edit->setValidator(new AllowEmptyDoubleValidator(-9999, 9999, 6, edit));
            }
            else {
                edit->setValidator(new AllowEmptyIntValidator(1, 99, edit));
            }
            return edit;
        }

        void setEditorData(QWidget* editor, const QModelIndex& index) const override
        {
            QLineEdit* edit = qobject_cast<QLineEdit*>(editor);
            if (!edit) return;
            edit->setText(index.data().toString());
            edit->selectAll();
        }

        void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override
        {
            QLineEdit* edit = qobject_cast<QLineEdit*>(editor);
            QString text = edit->text();

            //空值允许保存(删除数据)；仅非空时归一化积分点为1-99的奇数
            if (m_type == IntOdd && !text.simplified().isEmpty()) {
                int val = text.toInt();
                if (val < 1) val = 1;
                if (val % 2 == 0) val++;
                text = MxFunctions::mNumberToString(val);
            }
            model->setData(index, text);
        }

    private:
        NumberType m_type;
    };

    // 自定义表格，保留多选状态并直接编辑
    class PlyTableWidget : public QTableWidget
    {
        Q_OBJECT
    public:
        explicit PlyTableWidget(QWidget* parent = nullptr) : QTableWidget(parent) {}

    protected:
        void mousePressEvent(QMouseEvent* event) override
        {
            if (event->button() == Qt::LeftButton)
            {
                QModelIndex idx = indexAt(event->pos());
                if (idx.isValid() && selectionModel() && selectionModel()->isSelected(idx)
                    && !(event->modifiers() & Qt::ControlModifier)
                    && !(event->modifiers() & Qt::ShiftModifier))
                {
                    // 在已选中的单元格上按下鼠标时不改变选区，只记录当前索引
                    m_lastPressedIndex = idx;
                    return;
                }
            }
            m_lastPressedIndex = QModelIndex();
            QTableWidget::mousePressEvent(event);
        }

        void mouseReleaseEvent(QMouseEvent* event) override
        {
            if (m_lastPressedIndex.isValid())
            {
                // 如果鼠标还在同一个单元格上释放，直接触发编辑
                QModelIndex idx = indexAt(event->pos());
                if (idx == m_lastPressedIndex)
                {
                    edit(idx);
                }
                m_lastPressedIndex = QModelIndex();
                return;
            }
            QTableWidget::mouseReleaseEvent(event);
        }

        void mouseDoubleClickEvent(QMouseEvent* event) override
        {
            if (event->button() == Qt::LeftButton)
            {
                QModelIndex idx = indexAt(event->pos());
                if (idx.isValid() && selectionModel() && selectionModel()->isSelected(idx)
                    && !(event->modifiers() & Qt::ControlModifier)
                    && !(event->modifiers() & Qt::ShiftModifier))
                {
                    // 双击已选中的单元格，直接进入编辑，不改变选区
                    edit(idx);
                    return;
                }
            }
            QTableWidget::mouseDoubleClickEvent(event);
        }

    private:
        QModelIndex m_lastPressedIndex;
    };

    // ============== 主表格类=====================
    class mPlyTableWidget : public QWidget
    {
        Q_OBJECT
    public:
        explicit mPlyTableWidget(QWidget* parent = nullptr) : QWidget(parent)
        {
            initUI();
            initConnections();
        }

        void setMaterialItems(const QStringList& items)
        {
            m_materialItems = items;
            QStringList delegateItems = items;
            delegateItems.prepend("");
            auto callback = [this](int col, const QString& value) {
                this->batchSetSelectedColumn(col, value);
            };
            m_table->setItemDelegateForColumn(0, new MaterialDelegate(delegateItems, callback, this));
        }

        // 批量设置当前列所有选中单元格，并同步数据
        void batchSetSelectedColumn(int col, const QString& value)
        {
            if (!m_table || !m_table->selectionModel()) return;

            QModelIndexList selectedIndexes = m_table->selectionModel()->selectedIndexes();
            bool blocked = m_table->signalsBlocked();
            m_table->blockSignals(true);
            for (const QModelIndex& idx : selectedIndexes)
            {
                if (idx.column() != col) continue;
                QTableWidgetItem* item = m_table->item(idx.row(), idx.column());
                if (item) item->setText(value);
            }
            m_table->blockSignals(blocked);

            // 触发数据同步信号
            for (const QModelIndex& idx : selectedIndexes)
            {
                if (idx.column() != col) continue;
                if (col == 0) emit materialChanged(idx.row(), value);
                else if (col == 1) emit angleChanged(idx.row(), value);
                else if (col == 2) emit thicknessChanged(idx.row(), value);
                else if (col == 3) emit intePointChanged(idx.row(), value);
            }
        }

        void clearTableData(bool silent = false)
        {
            disconnectTableSig();
            m_table->setRowCount(0);
            connectTableSig();
            if (!silent)
                emit tableCleared();
        }

        void addRows(int count, bool silent = false)
        {
            disconnectTableSig();
            m_table->setRowCount(count);
            for (int row = 0; row < count; ++row)
            {
                m_table->setItem(row, 0, new QTableWidgetItem(""));     // 材料默认空
                m_table->setItem(row, 1, new QTableWidgetItem(""));     // 角度默认空
                m_table->setItem(row, 2, new QTableWidgetItem(""));     // 厚度默认空
                m_table->setItem(row, 3, new QTableWidgetItem("1"));     // 积分点默认1
            }
            connectTableSig();
        }

        bool setRowData(int row, const QString& material, const QString& angle, const QString& thickness, const QString& intePoint)
        {
            if (row < 0 || row >= m_table->rowCount()) return false;
            m_table->setItem(row, 0, new QTableWidgetItem(material));
            m_table->setItem(row, 1, new QTableWidgetItem(angle));
            m_table->setItem(row, 2, new QTableWidgetItem(thickness));
            m_table->setItem(row, 3, new QTableWidgetItem(intePoint));
            return true;
        }

        QList<QPair<QString, QList<QString>>> getAllPlyData() const
        {
            QList<QPair<QString, QList<QString>>> res;
            for (int i = 0; i < m_table->rowCount(); ++i)
            {
                if (!m_table->item(i, 0)) continue;
                QString material = m_table->item(i, 0)->text();
                QString angle = m_table->item(i, 1)->text();
                QString thickness = m_table->item(i, 2)->text();
                QString intePoint = m_table->item(i, 3)->text();
                res << qMakePair(material, QList<QString>() << angle << thickness << intePoint);
            }
            return res;
        }

    signals:
        void rowAdded(int row);
        void rowRemoved(int row);
        void tableCleared();
        void materialChanged(int row, const QString& material);
        void angleChanged(int row, const QString& value);
        void thicknessChanged(int row, const QString& value);
        void intePointChanged(int row, const QString& value);
        void plyStackPlotClicked();

    private slots:
        void addRow()
        {
            int row = m_table->rowCount();
            m_table->insertRow(row);
            m_table->setItem(row, 0, new QTableWidgetItem(""));     // 材料默认空
            m_table->setItem(row, 1, new QTableWidgetItem(""));
            m_table->setItem(row, 2, new QTableWidgetItem(""));
            m_table->setItem(row, 3, new QTableWidgetItem("1"));
            emit rowAdded(row);
        }

        void removeRow()
        {
            QSet<int> rows;
            for (auto& rng : m_table->selectedRanges())
                for (int r = rng.topRow(); r <= rng.bottomRow(); r++)
                    rows.insert(r);

            QList<int> rs = rows.values();
            std::sort(rs.begin(), rs.end(), std::greater<int>());

            for (int r : rs)
            {
                m_table->removeRow(r);
                emit rowRemoved(r);
            }
        }

        void clearTable() { clearTableData(); }

        void setRowByInput()
        {
            bool ok;
            int num = m_editRowNum->text().toInt(&ok);
            if (!ok || num <= 0) return;

            //必须发信号同步清空面板数据(厚度/角度/积分点列表与材料映射)，否则数据只增不减、行删不掉
            clearTableData();
            for (int i = 0; i < num; ++i) addRow();
            m_editRowNum->clear();
        }

        void onCellChanged(int row, int col)
        {
            if (!m_table->item(row, col)) return;
            QString txt = m_table->item(row, col)->text();
            if (col == 0)
            {
                emit materialChanged(row, txt);
                applyEditToSelectedItems(row, col);
            }
            if (col == 1)
            {
                emit angleChanged(row, txt);
                applyEditToSelectedItems(row, col);
            }
            if (col == 2)
            {
                emit thicknessChanged(row, txt);
                applyEditToSelectedItems(row, col);
            }
            if (col == 3)
            {
                emit intePointChanged(row, txt);
                applyEditToSelectedItems(row, col);
            }
        }

        // 当用户在目标列直接编辑某单元格时，把该值同步到当前列所有选中的单元格
        void applyEditToSelectedItems(int row, int col)
        {
            QTableWidgetItem* currentItem = m_table->item(row, col);
            if (!currentItem) return;
            QString newValue = currentItem->text();

            QModelIndexList selectedIndexes = m_table->selectionModel()->selectedIndexes();
            bool blocked = m_table->signalsBlocked();
            m_table->blockSignals(true);
            for (const QModelIndex& idx : selectedIndexes)
            {
                if (idx.column() == col && idx.row() != row)
                {
                    QTableWidgetItem* item = m_table->item(idx.row(), idx.column());
                    if (item) item->setText(newValue);
                }
            }
            m_table->blockSignals(blocked);

            // 手动触发数据同步信号，确保数据类同步更新
            for (const QModelIndex& idx : selectedIndexes)
            {
                if (idx.column() == col && idx.row() != row)
                {
                    if (col == 0) emit materialChanged(idx.row(), newValue);
                    else if (col == 1) emit angleChanged(idx.row(), newValue);
                    else if (col == 2) emit thicknessChanged(idx.row(), newValue);
                    else if (col == 3) emit intePointChanged(idx.row(), newValue);
                }
            }
        }

    private:
        void initUI()
        {
            QHBoxLayout* controlLayout = new QHBoxLayout;
            m_btnAdd = new QPushButton;
            m_btnRemove = new QPushButton;
            m_btnClear = new QPushButton;
            m_editRowNum = new QLineEdit;

            m_btnAdd->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
            m_btnRemove->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png"));
            m_btnClear->setIcon(QIcon(":/mMainWidgets/Icon/shanchu3x.png"));
            m_editRowNum->setPlaceholderText("行数");
            m_editRowNum->setFixedWidth(80);

            m_btnPlyPlot = new QPushButton;
            m_btnPlyPlot->setIcon(QIcon(":/mMainWidgets/Icon/plyThick.png"));
            m_btnPlyPlot->setToolTip("Ply Stack Plot");

            controlLayout->addWidget(m_btnAdd);
            controlLayout->addWidget(m_btnRemove);
            controlLayout->addWidget(m_btnClear);
            controlLayout->addWidget(m_editRowNum);
            controlLayout->addWidget(m_btnPlyPlot);
            controlLayout->addStretch();

            m_table = new PlyTableWidget;
            m_table->setColumnCount(4);
            m_table->setHorizontalHeaderLabels({ "材料", "角度", "厚度", "积分点数" });

            m_table->setSelectionBehavior(QAbstractItemView::SelectItems);
            m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
            m_table->setAlternatingRowColors(true);
            m_table->verticalHeader()->setDefaultSectionSize(35);
            m_table->verticalHeader()->setVisible(true);
            m_table->verticalHeader()->setDefaultAlignment(Qt::AlignCenter);
            m_table->verticalHeader()->setFixedWidth(40);
            m_table->horizontalHeader()->setStretchLastSection(true);
            m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
            m_table->setContextMenuPolicy(Qt::CustomContextMenu);

            m_table->setStyleSheet(R"(
QHeaderView::section {
    background-color: #87cefa;
    color: #2c3e50;
    font-weight: bold;
    padding: 6px;
    border: 1px solid #b0d4f1;
}
QTableWidget {
    gridline-color: #d0e6f7;
    alternate-background-color:#ECF5FF;
    selection-background-color: #add8e6;
    selection-color: black;
}
QTableWidget::item {
    background: transparent;
}
QHeaderView::section:horizontal
{
    background: #F7F8FA;
    border: 1px solid #dcdcdc;
    border-left: 0;
    padding:4px;
    font-weight:normal;
}
QHeaderView::section:vertical
{
    background: #F7F8FA;
    border: 1px solid #dcdcdc;
    border-top: 0;
    padding:4px;
    font-weight:normal;
}
        )");

            m_table->setItemDelegateForColumn(1, new NumberDelegate(NumberDelegate::Double, this));
            m_table->setItemDelegateForColumn(2, new NumberDelegate(NumberDelegate::Double, this));
            m_table->setItemDelegateForColumn(3, new NumberDelegate(NumberDelegate::IntOdd, this));

            QVBoxLayout* mainLayout = new QVBoxLayout(this);
            mainLayout->addLayout(controlLayout);
            mainLayout->addWidget(m_table);
            mainLayout->setContentsMargins(0, 0, 0, 0);
        }

        void initConnections()
        {
            connect(m_btnAdd, &QPushButton::clicked, this, &mPlyTableWidget::addRow);
            connect(m_btnRemove, &QPushButton::clicked, this, &mPlyTableWidget::removeRow);
            connect(m_btnClear, &QPushButton::clicked, this, &mPlyTableWidget::clearTable);
            connect(m_editRowNum, &QLineEdit::returnPressed, this, &mPlyTableWidget::setRowByInput);
            connect(m_btnPlyPlot, &QPushButton::clicked, this, &mPlyTableWidget::plyStackPlotClicked);
            connectTableSig();
        }

        void connectTableSig()
        {
            disconnectTableSig();
            m_tableChangeConn = connect(m_table, &QTableWidget::cellChanged, this, &mPlyTableWidget::onCellChanged);
        }

        void disconnectTableSig()
        {
            if (m_tableChangeConn)
                disconnect(m_tableChangeConn);
        }

    private:
        QPushButton* m_btnAdd;
        QPushButton* m_btnRemove;
        QPushButton* m_btnClear;
        QPushButton* m_btnPlyPlot;
        QLineEdit* m_editRowNum;
        PlyTableWidget* m_table;
        QStringList m_materialItems;
        QMetaObject::Connection m_tableChangeConn;
        bool m_silentMode = false;
    };
}