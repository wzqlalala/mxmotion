#pragma once

//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

//QT
#include <QVBoxLayout>
#include <QWidget>
#include <QListWidgetItem>
#include <QTableWidget>
#include <QMouseEvent>
#include <QHash>
#include <QGridLayout>
#include <QLabel>
#include <QToolButton>
#include <QComboBox>
#include <QRadioButton>
#include <qvector3d.h>

#include "mGeoPanelBase.h"

#include "mGeoPreviewTypes.h"


namespace MechMainWindow
{
    class MMAINWINDOW_EXPORT mGeoBoolOperationPanel : public mGeoPanelBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mGeoBoolOperationPanel(QWidget* parent = nullptr);

        /*
        * 析构函数
        */
        ~mGeoBoolOperationPanel();

        /*
        * 显示特征界面
        */
        void showFold(GeoOperationType type);

		/*
		* 进入面板时连接拾取等全局信号（mInfoWindow::createGeoBool 调用）
		*/
		void connectGeoSignals() override;

		/*
		* 退出面板时断开全部全局信号（mInfoWindow::uneditGeometry 调用）
		*/
		void disconnectGeoSignals() override;


        void slot_contextMenuTableData(const QPoint& pos);

    protected:
        /*
        * 面板隐藏时取消预览
        */
        void hideEvent(QHideEvent* event) override;

    private slots:

        void slot_onCancel();

        /*
        * 布尔求和操作
        */
        void slot_summationAction();

        /*
        * 布尔求交操作
        */
        void slot_interseAction();

        /*
        * 布尔求差操作
        */
        void slot_susubtracAction();

        /*
        * 布尔预览
        */
        void slot_booleanPreview();

    private:
        //视口切换信号的连接（connectGeoSignals 建立）
        QMetaObject::Connection conn_viewportChanged;

        /*
        * 初始化布尔求交操作界面
        */
        void initInterseFold();

        /*
        * 初始化布尔求和操作界面
        */
        void initSummationFold();

        /*
        * 初始化布尔求差操作界面
        */
        void initSusubtracFold();

        /*
        * 连接/断开布尔预览信号；conn 为 true 时拾取表格变化都会触发预览
        */
        void connectPreView(bool conn);

        /*
        * 激活/取消激活直接建模渲染工具
        */
        void activateDirectGeoModelingTool(bool active);

        /*
        * 清除预览渲染
        */
        void clearPreviewRender();

        /*
        * 把预览离散化结果渲染到视口（颜色取被操作部件内色，面/线/点一致）
        */
        void renderPreviewResult(const EventDispatcher::mGeoPreviewResult& res, int faceAlpha = 0);

        /*
        * 布尔预览时把原部件透明，预览结束时恢复原透明度
        */
        void setSourcePartTransparent(const std::vector<int>& partIDs, ushort alpha);
        void restoreSourcePartAlpha();

        /*
        * 捕获窗口激活
        */
        void slot_subWindowActivated();

        /*
        * 高亮几何数据变化
        */
        void slot_highlightGeoData();

        //拾取高亮数据改变的连接
        QMetaObject::Connection conn_geo;

        //布尔求和控件
        QCheckBox* summationRetainChkBox;
        QCheckBox* summationCopyChkBox;
        QCheckBox* summationPreviewChkBox = nullptr;

        //布尔求差控件
        QTableWidget* targetBTable;
        QCheckBox* subtracCopyChkBox;
        QCheckBox* subtracPreviewChkBox = nullptr;

        //布尔求交控件
        QCheckBox* interseCopyChkBox;
        QCheckBox* intersePreviewChkBox = nullptr;

        //表格右键菜单
        QAction* subDelAction = nullptr;

        //布尔预览时被透明化的部件及其原透明度（ushort 刻度）
        std::vector<int> m_transparentPartIDs;
        std::vector<int> m_transparentPartOriginAlphas;
    };
}