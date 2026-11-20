
//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;

//#pragma once
////解决中文乱码
//#pragma execution_character_set("utf-8")
//
//#include "mpostwindow_global.h"
//
//#include <QWidget>
//#include <QMouseEvent>
//#include <set>
//#include <QVector>
//#include <QHash>
//#include <QMdiSubWindow>
//#include <QTableWidgetItem>
//#include <QPointer>
//
//#include "mPanelBase.h"
//#include "mCalcLineEdit.h"
////#include "mxr/view/REBUILD_PostRend.h"
////#include "mxr/view/REBUILD_PostRender.h"
//#include "mPostDataBase.h"
//#include "mIMxdbFile1.h"
//#include "mTableDataWindow.h"
//
//namespace Ui
//{
//    class mPostProbes;
//}
//
//namespace MPostRend
//{
//    //class mPostRend;
//    class REBUILD_PostRend;
//
//}
//
//namespace Common
//{
//    class mPlotView;
//}
//
//namespace MPostView
//{
//    class mPostOneFrameRendData1;
//}
//
//namespace MDataPost
//{
//    class mOneFrameData1;
//}
//
//using namespace PostData;
//using namespace MIOFile;
//
//namespace PostWindow
//{
//    class MPOSTWINDOW_EXPORT mPostProbePanel : public mPanelBase
//    {
//        Q_OBJECT
//
//    public:
//        /** 构造函数 */
//        mPostProbePanel(QWidget* parent = Q_NULLPTR);
//
//        /** 析构函数 */
//        ~mPostProbePanel();
//
//        /** 当前面板是否显示 */
//        void setPanelPickAndSignal(bool isshow);
//
//        void newProbePanel(int caseid);
//
//        void updateProbePanel(int caseid);
//
//    private:
//        /** 提取数据 */
//        bool generateCurveData();
//
//    private slots:
//        /** 切换中英文 */
//        void slot_changeLanguage(int language);
//
//        /** 完成拾取 */
//        void slot_finishedPick();
//
//        /** 接收导入文件完成的信号 */
//        void slot_finishedImportFiles(const QString& postName);
//
//        /** 变量comboBox选项改变 */
//        void slot_variableChanged(const QString& varName);
//
//        /** 分量listWidget选中 */
//        void slot_selectedChanged();
//
//        /** 拾取类型 */
//        void slot_pickComboBox(const QString& pickText);
//
//        /** 创建新曲线视图 */
//        Common::mPlotView* createNewCurvePlot(QString& viewName);
//
//        /** 创建曲线图 */
//        void slot_createCurve();
//
//        /** 当前激活窗口改变 */
//        void slot_postActiveViewChanged(QMdiSubWindow*);
//
//        /** 点击激活曲线视图 */
//        void slot_onCellClicked(QTableWidgetItem* item);
//
//        /** 删除曲线图 */
//        void slot_deleteCurvePlot();
//
//        void on_pushButton_6_clicked();
//
//    signals:
//        /** 返回到主面板 */
//        void returnSig();
//
//    private:
//        // UI界面
//        Ui::mPostProbes* _ui;		                //UI
//
//        // 内部逻辑变量
//        QPointer<mPostDataBase>					_postDB{};
//        QHash<QString, QString>		     _axisYTitles{};			//Y坐标轴标题
//        QVector<double>					 _XValue{};					//X值
//        QHash<QString, QVector<double>>	 _YValuesD{};				//Y值
//        QHash<QString, QVector<float>>	 _YValuesF{};				//Y值
//        QStringList                      _curveNames{};			    //对应的曲线名
//        QPointer<REBUILD_PostRend>       _postRend{};				//暂存视口
//        std::set<int>                    _usableCurveViewNums{};    //可使用视图名称编号集合
//
//        QString                          _curViewName{};
//    };
//}
