#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mpostwindow_global.h"

//QT
#include <QWidget>
#include <QGridLayout>
#include <QLabel>
#include <QPointer>
#include <QRadioButton>
#include <QButtonGroup>

#include "mPanelBase.h"
#include "mCalcLineEdit.h"
#include "mxr/view/REBUILD_PostRend.h"
#include "mxr/view/REBUILD_PostRender.h"
#include "mPostDataBase.h"
#include "mIMxdbFile1.h"
#include "PostCurveStruct.h"
#include "mTableDataWindow.h"

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;


using namespace PostData;
using namespace MIOFile;

namespace PostWindow
{
    class mPostDataTable;
	class MPOSTWINDOW_EXPORT mPostCurvePanel : public mPanelBase
	{
		Q_OBJECT
	public:
		explicit mPostCurvePanel(QWidget* parent = nullptr);

		/** 视口绑定打开（用户定案 2026-11-04）：曲线不按次生成实例——数据键=当前
		 * 激活后处理视口名，同视口重复打开沿用既有配置，切换视口后打开自动跟随 */
		void editCurveForViewport(int caseid);

		void updateCurvePanel(int caseid);

		QStringList getCheckedComplex();

		void getCurveDatas(QVector<double> &xValues, QVector<QVector<double>>& yValues, QStringList &headers);

		//设置当前视图
		void setCurViewName(const QString& viewName);

		//更新界面数据
		void updateCurveGUI();

		//初始化界面
		void initUI();

		//断开复选框信号
		void blockCheckSig(bool open);

		//曲线-更新显示曲线
		void updateDisplayCurve();

		//显示表格
		void showTableData();

		//更新节点状态
		void updateItemStatus();

		//设置时间变量控件显示
		void showTimeVarWidget();

		//设置导入控件显示
		void showImportWidget();

		//设置路径变量控件显示
		void showPathWidget();

		//计算路径曲线X值
		QVector<double> calPathCurveXvalues(QVector<QVector3D> vertexs, QString& type);

		//获取路径曲线Y值
		void getPathCurveYvalues(QVector<int> ids);

        //高亮路径
        void highlightPath(QVector<int> ids, bool isIntersect = false);

        //清空高亮
        void clearhighlight();

	private slots:
		/** 切换曲线类型下拉框 */
		void slot_chooseCurveType(const QString& type);
		
		/** 切换工况下拉框 */
		void slot_changeCaseName(const QString& caseName);

		/** 切换路径帧下拉框 */
		void slot_chooseFrame(const QString& frame);

		/** 切换变量下拉框 */
		void slot_chooseVariable(const QString& var);

		/** 选中分量列表 */
		void slot_selectedCompoChanged();

		/** 选中层列表 */
		void slot_selectedLayerChanged();

		/** 全选 */
		void slot_selectAll();

		/** 全不选 */
		void slot_selectNone();

		/** 反选 */
		void slot_selectReverse();

		/** 添加、替换拾取 */
		void slot_addPick();

		/** 清空拾取 */
		void slot_clearPick();

		/** 复数复选框 */
		void slot_checkedChange();

		/** 选择曲线文件 */
		void slot_chooseCurveFile();

        /** 解析曲线文件（新增），失败时通过 errMsg 返回具体原因 */
        bool loadCurveFile(const QString& filePath, QString& errMsg);

		/** 读取曲线文件 */
		void slot_readCurveFile(QString filename, QStringList& curveNames, QVector<double>& xValue, QVector<QVector<double>>& yValue);

		/** 生成曲线 */
		void slot_generateCurve();

        /** 导出曲线 */
        void slot_exportCurve();

        PostData::mPostCurveData* getCurveData();

    signals:

    protected:
        void hideEvent(QHideEvent* event) override;

	private:
		QVBoxLayout*							_mainLayout;
		//_curViewName 用基类 mPanelBase 的同名成员（提升基类后随视口改名注册表自动跟随，
		//勿在派生类重新声明遮蔽，否则 onViewportRenamed 更新的基类副本面板读不到）
		QPointer<mPostDataBase>					_postDB{};
		QVector<int>							_pickIds; 
		int										_varType = 0;				//0:节点 1：单元
		bool									_isComplex{};				//是否为复数
		QList<QCheckBox*>						_boxList;
		QStringList								_curveNames{};				//对应的曲线名
		QVector<double>							_XValueD{};					//X值
		QVector<float>							_XValueF{};					//X值
		QHash<QString, QVector<double>>			_YValuesD{};				//Y值
		QHash<QString, QVector<float>>			_YValuesF{};				//Y值
		mTableWidget*							_tableWidget{};				//表格

        FoldSection*                            _typeFold;
        FoldSection*                            _xFold;
        FoldSection*                            _yFold;

		QComboBox*								_comboCurveType;
		QWidget*								_widgetPick;
		QLabel*									_labelObject;
		QLabel*									_labelSelectStatu;
		QPushButton*							_btnAdd;
		QPushButton*							_btnReplace;
		QPushButton*							_btnClear;
		QLabel*									_labelFile;
		QLineEdit*								_lineEditFile;
		QPushButton*							_btnFile;
		QLabel*									_labelPath;
		QComboBox*								_comboPath;
		QPushButton*							_btnPath;
		QCheckBox*								_checkPath;
		QLabel*									_labelData;
		QLabel*									_labelDataType;
		QWidget*								_widgetPath;
		QButtonGroup*							_btnGroupPath;

		QLabel*									_labelCase;
		QLabel*									_labelFrame;
		QComboBox*								_comboCase;
		QComboBox*								_comboFrame;
		QComboBox*								_comboVariable;                // 变量
		QPushButton*							_btnAllCompo;                  // 全选
		QPushButton*							_btnNoneCompo;                 // 全不选
		QPushButton*							_btnReverseCompo;              // 反选
		QListWidget*							_listComponent;                // 分量
		QPushButton*							_btnAllLayer;
		QPushButton*							_btnNoneLayer;
		QPushButton*							_btnReverseLayer;
		QListWidget*							_listLayer;
		QCheckBox*								_checkReal;
		QCheckBox*								_checkImag;
		QCheckBox*								_checkMag;
		QCheckBox*								_checkPhase;
		QPushButton*							_btnGenerate;
		QPushButton*							_btnExport;                    //导出
        mPostDataTable* _dataTable{};				//数据表窗口
	};
}
