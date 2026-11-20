#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")
#include "mmainwindow_global.h"
#include <QDialog>
#include <QListWidgetItem>
#include <QPushButton>
#include <QLineEdit>
#include <set>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QMdiSubWindow>
#include <QPointer>

#include "mPanelBase.h"
#include "mCalcLineEdit.h"
#include "mPostDataBase.h"
#include "mMeshPanel.h"
#include <qtextedit.h>
#include "PickCommonFunction.h"

using namespace PostData;

namespace MechMainWindow
{
	class MMAINWINDOW_EXPORT mPostSetPanel : public mPanelBase
	{
		Q_OBJECT

	public:
		/** 默认构造函数 */
		mPostSetPanel(QWidget *parent = Q_NULLPTR);

		/** 析构函数 */
		~mPostSetPanel();

        /** 新建集合面板 */
        void newSetPanel(int caseid);

        //设置当前视图
        void setCurViewName(const QString& viewName);

        //更新
        void updateSetInfo(QString name,QString type);
    private:
        void updateContextByViewName(const QString& viewName);      // 根据 viewName 重新定位 _rWidget 与 _db
 
        void slot_clear();
        void slot_confirm();
        void slot_type(QString type);
        void slot_range(QString type);
        void slot_new();
        void slot_update();
        void slot_highlight();
        void table_updata(int row, int column);
        void slot_partname(const QModelIndex& index);
        void slot_linepart(const QString& text);
    protected:
        // B1b(M12/N-B-3)：视口改名跟随（mPanelBase dispatchViewportRenamed 单点分发）
        void onViewportRenamed(const QString& oldName, const QString& newName) override;

    private:
        void initConnncet();
        QPointer<mPostDataBase> _db = nullptr;      // 后处理渲染数据，窗口创建时newSectionPanel 从全局获取
        mxr::REBUILD_PostRend* _rWidget = nullptr;  // 渲染窗口, 窗口创建时newSectionPanel 从全局获取
        // B1b(M12/N-B-3)：原自有 _curViewName 遮蔽基类 mPanelBase 成员且未接改名
        // 分发，视口改名跟随对本面板失效——消遮蔽，成员所有权归基类，改名跟随
        // 经下方 onViewportRenamed override 接入
        QVBoxLayout* _mainLayout;
        QTableWidget* _tableSet;                              // 集合列表
        QPushButton* _btnNew;                                    // 新建
        QPushButton* _btnUpdate;                                 // 更新

        QLabel* _nameLabel;                       //名称
        QLabel* _typeLabel;                       //类型
        QLabel* _rangeLabel;                      //范围
        QLabel* _angleLabel;                      //角度

        QLineEdit* _nameLine;                     //名称
        QDoubleSpinBox* _angleDoubleSB;           //角度
        QComboBox* _typeCom;                      //类型
        QComboBox* _rangeCom;                     //范围

        QStackedWidget* _functionStackedWidget;
        QLabel* _partNameLabel;                       //零件名过滤
        QLineEdit* _partNameLine;                     //零件名过滤
        QListWidget* _partNameListWidget;

        QCheckBox* _idCheck;
        QPushButton* _highlightBtn;                     // 高亮
        QStackedWidget* _idStackedWidget;
        QTextEdit* _idTextEt;                          //ID编写栏

        QLabel* _idStartLabel;                        //起始ID
        QLabel* _idSpaceLabel;                        //ID间隔
        QLabel* _idEndLabel;                          //终止ID

        QLineEdit* _idStartLine;                      //起始ID
        QLineEdit* _idSpaceLine;                      //ID间隔
        QLineEdit* _idEndLine;                        //终止ID

        QPushButton* _clearBtn;                       // 清除
        QPushButton* _confirmBtn;                     // 确定


        QHash<QString, std::vector<mxr::MeshSelectInfoData>> _countDataInformation;//保存数据信息
        bool isSelect;                                 //是否选中
	};
}