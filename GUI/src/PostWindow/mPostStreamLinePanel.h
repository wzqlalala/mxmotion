#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mpostwindow_global.h"

//QT
#include <QWidget>
#include <QGridLayout>
#include <QLabel>
#include <QPointer>

#include "mPanelBase.h"
#include "mPostDataBase.h"
#include "mxr/view/REBUILD_PostRend.h"
#include "mxr/view/REBUILD_PostRender.h"

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;


using namespace PostData;

namespace PostWindow
{
	class MPOSTWINDOW_EXPORT mPostStreamLinePanel : public mPanelBase
	{
		Q_OBJECT
	public:
		explicit mPostStreamLinePanel(QWidget* parent = nullptr);

		void newStreamLinePanel(int caseid);

		//通过视口名截取云图视口
		QString getCloudViewName(QString view);

		//设置当前视图
		void setCurViewName(const QString& viewName);

		bool haveStreamLine(QString cloudView);

		void updatePanel();

	private slots:
        /** 添加/替换拾取 */
        void slot_addPick();

        /** 清空拾取 */
        void slot_clearPick();

        /** 生成流线 */
        void slot_createStreamLine();

    protected:
        void hideEvent(QHideEvent* event) override;

	private:
		QPointer<mxr::REBUILD_PostRend>			              _curPostRend{};					//当前激活窗口
        std::set<int>						                  _pickIds;
        std::map<int,MeshSelectInfoData>                      _allOldPickEleData;//所有被拾取过的单元数据

		QVBoxLayout*							              _mainLayout;
        QLabel*                                               _labelSelectStatu;
        QPushButton*                                          _btnAdd;
        QPushButton*                                          _btnReplace;
        QPushButton*                                          _btnClear;
        QPushButton*                                          _btnGenerate;
	};
}
