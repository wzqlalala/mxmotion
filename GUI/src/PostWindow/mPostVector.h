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
	class MPOSTWINDOW_EXPORT mPostVectorPanel : public mPanelBase
	{
		Q_OBJECT
	public:
		explicit mPostVectorPanel(QWidget* parent = nullptr);

		void newVectorPanel(int caseid);

		//通过视口名截取云图视口
		QString getCloudViewName(QString view);

		//设置当前视图
		void setCurViewName(const QString& viewName);

        bool haveVector(QString cloudView);

        void updatePanel();

	private slots:
        /** 添加、替换拾取 */
        void slot_addPick();

        /** 清空拾取 */
        void slot_clearPick();

        /** 生成全部节点矢量图 */
        void slot_createAllVector();

        /** 实时生成 */
        void slot_isRealTimeGenerate();

        /** 默认按钮 */
        void slot_default();

        /** 更新按钮 */
        void slot_update();

        /** 选择箭头大小样式下拉框 */
        void slot_chooseArrowType(const QString&);

        /** 选择大小 */
        void slot_changeArrowSize(double value);

    signals:

    protected:
        void hideEvent(QHideEvent* event) override;

	private:
		QPointer<mxr::REBUILD_PostRend>			_curPostRend{};					//当前激活窗口
        std::set<int>						    _pickIds;

		QVBoxLayout*							_mainLayout;
        QLabel*                                 _labelSelectStatu;
        QPushButton*                            _btnAdd;
        QPushButton*                            _btnReplace;
        QPushButton*                            _btnClear;
        QComboBox*                              _comboSize;
        QSlider*                                _sliderSize;
        QCheckBox*                              _checkRTG;
        QPushButton*                            _btnDefault;
        QPushButton*                            _btnUpdate;
	};
}
