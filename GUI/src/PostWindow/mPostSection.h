#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mpostwindow_global.h"

//QT
#include <QWidget>
#include <QGridLayout>
#include <QLabel>
#include <QPointer>
#include <QComboBox>
#include <QRadioButton>
#include <QSlider>
#include <QLineEdit>
#include <QPair>

#include "mPanelBase.h"
#include "PosPickWidget1.h"
#include "mCalcLineEdit.h"
#include "mxr/view/REBUILD_PostRend.h"
#include "mxr/view/REBUILD_PostRender.h"
#include "mPostDataBase.h"

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;


using namespace PostData;

namespace PostWindow
{
    class MPOSTWINDOW_EXPORT mPostSectionPanel : public mPanelBase
    {
        Q_OBJECT
    public:
        explicit mPostSectionPanel(QWidget* parent = nullptr);

        void newSectionPanel(int caseid);
        void deleteSectionPanel(int caseid);

        //通过视口名截取云图视口
        QString getCloudViewName(QString view);

        void setCurViewName(const QString& viewName);
	private slots:
	       /** 状态 */
	    void slot_checkBox(bool istrue);

	    /** 反向 */
	    void slot_reverseButton();

	    /** 删除 */
	    void slot_deleteButton();

	    /** 滚动条值改变 */
	    void slot_sliderValueChanged(int value);

        /** 新建 */
        void slot_newSection();

        /** 禁用全部 */
        void slot_banAll();

        /** 删除全部 */
        void slot_delAll();

        /** 位置改变 */
        void slot_positionChanged();

    public:
        // 设置所有切面的显隐状态
        void setAllClipingPlaneVisible(bool visible);
        // 获取当前切面是否可见
        bool isSectionVisible() const { return _isSectionVisible; }
        // 清空面板数据与状态
        void clearPanel();

    protected:
        // 每帧调用，检查状态！！！！！被搞烦了
        void paintEvent(QPaintEvent* e) override;
        // 退出面板时，断开所有连接
        void hideEvent(QHideEvent* event) override;
        // 进入面板时，恢复切面显示
        void showEvent(QShowEvent* event) override;

        /*
        * 设置按钮拾取状态样式,根据selecting
        * 需要执行的自定义函数由secectingFunc传入
        */
        void setupButtonSelectStyle(QWidget* btn, bool selecting, std::function<void()> selectingFunc);
        QVector3D GetPickSinglePoint();             // 获取单次拾取点结果，使其仅高亮选中点，而非累加
        PostClipingPanel CreateClipingPlane();      // 根据当前面板内容新建切面
        void DeleteClipingPlane(int index);         // 删除切面
        void UpdateAllClipingPlane();               // 更新切片数据
        void UpdateClipingPlane(int row, PostClipingPanel panel);  // 更新单个切片
    private:
        QPointer<mPostDataBase> _db = nullptr;      // 后处理渲染数据，窗口创建时newSectionPanel 从全局获取
        QPointer<mxr::REBUILD_PostRend> _rWidget;   // 渲染窗口, 窗口创建时newSectionPanel 从全局获取；渲染器随视口被 delete 时自动置空，防止悬空
        QVBoxLayout*    _mainLayout;
        QTableWidget*   _tableSection;                              // 切面列表
        QPushButton*    _btnNew;                                    // 新建
        QPushButton*    _btnBanAll;                                 // 禁用全部
        QPushButton*    _btnDelAll;                                 // 删除全部

        /**********************************************
        * 平面面板, 数量和位置
        **********************************************/
        QComboBox*      _planeShapeBox;                             // 平面类型
        /*
        * 平面点选择按钮,点击后触发点拾取,处于拾取状态时,颜色加深。
        * 平面点选择中的标志位作为静态变量储存在clicked槽函数中
        */
        QToolButton*            _planePointBtn;                     // 点1拾取（屏蔽不需要了）
        QToolButton*            _basePointBtn;                      // 基准点拾取
        QLineEdit*              _numEdit;                           // 数量
        QLineEdit* _xEdit;                                          //  X方向值
        QLineEdit* _yEdit;                                          //  Y方向值
        QLineEdit* _zEdit;                                          //  Z方向值

        PosPickWidget1          _planePointSelectPanel;             // 平面点选择面板，当面板在基于点状态下使用.
        std::vector<QLineEdit*> _planePointLineEdits;               // 平面-点1 的三个输入框
        std::vector<QLineEdit*> _baseLineEdits;                     // 基准的三个输入框 + 距离
        bool                    _basePointIsSelecting = false;     // 当前是否在拾取基准点状态下
        bool                    _planePointIsSelecting = false;     // 当前是否在拾取单个点状态下
        bool                    _countPointIsSelecting = false;     // 当前是否在拾取多个点状态下
        QMetaObject::Connection _planePointFinishPick;              // 单点拾取结束连接
        QMetaObject::Connection _basePointFinishPick;               // 基准点拾取结束连接
        QMetaObject::Connection _countPointFinishPick;              // 多点拾取结束连接
        int                     _posType = 0;                       // 位置类型: 0 基于点， 1 等间距， 2 偏移
        std::vector<mxr::MeshSelectInfoData> _countPoints;          // 拾取到的点信息
        std::vector<PostClipingPanel>   _clipsData;                 // 切片数据组
        PostClipingPanel*               _currentClip = nullptr;     // 当前选中的切片
        int                             _planeCount = 0;            // 切面数量
        bool                            _isSectionVisible = true;   // 当前切面是否可见
        bool                            _isLoadingView = false;     // 是否正在从持久化数据加载视图，避免重复保存

        void innerSlot_planePointBtnClicked(QWidget* btn);          // 平面点拾取按钮触发
        void innerSlot_countPlaneBtnClicked();                      // 数量点拾取按钮触发
        void innerSlot_btnAddClicked();                             // 添加按钮：确认当前拾取点
        void innerSlot_clearAllPickNode();                          // 清空已拾取点
        void innerSlot_itemPressed(QTableWidgetItem* item);         // 表格行点击
        void innerSlot_planeCountChanged(const QString& text);      // 数量修改。动态计算当前剩余最大切面数量
        void innerSlot_planeCountFinished();                        // 数量修改。强制更改数量到指定范围内

        void syncClipsToGuiData();                                  // 将当前 _clipsData 同步到 per-view 的 mPostGuiData
        void syncPickedNodesToGuiData();                            // 将当前拾取的节点同步到 per-view 的 mPostGuiData
        void restorePickedNodesFromGuiData();                       // 从 per-view 数据恢复拾取节点和高亮
        void updateContextByViewName(const QString& viewName);      // 根据 viewName 重新定位 _rWidget 与 _db
        bool relocateRendWidget();                                  // 按 _curViewName 重定位 _rWidget；若已随视口销毁则置空，返回其是否可用
        void updatePanelControlsFromClip(const PostClipingPanel& clip); // 根据当前选中的切片更新面板控件
        QTableWidgetItem* createNumItem(int row) const;             // 创建序号单元格
        QWidget* createStateWidget(bool checked, int row) const;    // 创建状态列控件
        QWidget* createReverseWidget(int row) const;                // 创建反向列控件
        QWidget* createSliderWidget(int row) const;                 // 创建滚动条列控件
        QWidget* createValueWidget(int row) const;                  // 创建滚动条值列控件
        QWidget* createDeleteWidget(int row) const;                 // 创建删除列控件

        /** 计算切面组初始位置在法向上的投影值 */
        float calcInitialPlanePosition(const PostClipingPanel& clip) const;
        /** 计算切面组在法向上可双向偏移的对称范围 */
        QPair<float, float> calcSliderRange(const PostClipingPanel& clip) const;
        /** 计算滚动条值对应的相对偏移量 */
        float sliderValueToOffset(const PostClipingPanel& clip, int value) const;
        /** 根据相对偏移量反算滚动条值 */
        int offsetToSliderValue(const PostClipingPanel& clip, float offset) const;
    private slots:

        void slot_planeShapeChanged();                              // 平面类型改变

    private:
        QRadioButton*   _radioBasePoint;                            // 基于点
        QRadioButton*   _radioEqualSpacing;                         // 等间距
        QRadioButton*   _radioOffset;                               // 偏移

        FoldSection*    _numandpos;

        QWidget*        _pointContainer;                            // 点容器
        QWidget*        _baseContainer;                             // 基准容器      
        QWidget*        _distanceContainer;                         // 距离容器
        QWidget*        _vectorContainer;                            // 向量容器
	};
}
