#pragma once
#pragma execution_character_set("utf-8")


#include "mPanelBase.h"
#include "mGeoPanelBase.h"
#include "mPhysicalFieldEnum.h"
#include <QWidget>
#include <QList>
#include <QString>

QT_BEGIN_NAMESPACE
class QTableWidget;
class QSplitter;
class QGridLayout;
class QCheckBox;
class QLineEdit;
class QToolButton;
class FoldSection;   // 若你没有，用 QGroupBox 代替
QT_END_NAMESPACE

namespace MxFunctions { class mCalcLineEdit; }

// occsp::SectionCoordinateOptions 前向声明所在的头文件
//#include "SectionAnalysis.h" //【OCC摘除·S10/P0-1】退役头死 include 清剿（occsp 冻结岛保档，拉 SectionGeometry/OCC 链）

namespace MechSectionShapeData {
    class mSectionShapeBase;
    class mSectionShapeManager;
}

namespace MechMainWindow
{

    /* 继承 mPanelBase，不再继承 QDockWidget */
    class mSectionShapePanel : public mGeoPanelBase
    {
        Q_OBJECT
    public:
        explicit mSectionShapePanel(QWidget* parent = nullptr);

        ~mSectionShapePanel();

        void newSectionShapePanel(int type);

        void createCopyPanel(std::vector<int> copyId);

        void clearGeoWidget();
        /* 唯一对外接口：刷新整个面板 */
       // void rebuildPanel(const QList<double>& para, const QString& name);

        void clearTableWidget();


        void slotShowDataOnPanel(const QList<double>& secData); // 原 DataWidget
        void editCustomPara();
        void editHSecPara();
        void editLSecPara();
        void editISecPara();
        void editI1SecPara();
        void editTSecPara();
        void editBoxSecPara();

        void editRecSecPara();

        void editCirSecPara();

        void editAnnSecPara();

        void editPolySecPara();

        void editChanSecPara();

        void editChan1SecPara();

        void editChan2SecPara();


        void editT1SecPara();

        void editT2SecPara();

        void editBox1SecPara();

        void editCrossSecPara();

        void editHatSecPara();

        void editZSecPara();

        void updatePanel();
        void editSectionShapePanel(int sectionShapeId);
        //显示截面形状渲染窗口
        void showGeoWidget(int type);
        //更新截面属性和显示
        void slot_RebuildSec(QList<double> para, int Id, MxFunctions::SecType type);
        /* 两张表 */
        QTableWidget* _paramTable = nullptr;
        QTableWidget* _dataTable = nullptr;
        QComboBox* sectionShape;

        /* 构建梁截面(几何拾取)区控件 */
        MxFunctions::mCalcLineEdit* sectionTolerance = nullptr;  // 重合容差
        MxFunctions::mCalcLineEdit* sectionShellThickness = nullptr;  // 壳厚度
        QLabel* sectionToleranceUnit = nullptr;
        QLabel* sectionShellThicknessUnit = nullptr;
        QCheckBox* sectionMeshAuto = nullptr;  // 实体截面网格尺寸自动
        MxFunctions::mCalcLineEdit* sectionMeshSize = nullptr;  // 实体截面网格尺寸
        QLabel* sectionMeshSizeUnit = nullptr;
        QMetaObject::Connection conn_sectionPick;  // 拾取填充表格的连接

        PanelLabel* sectionToleranceLabel = nullptr;
        PanelLabel* sectionShellThicknessLabel = nullptr;
        PanelLabel* sectionMeshSizeLabel = nullptr;

        /* 构建梁截面(自定义截面)附加设置 */
        // 梁截面平面法向：X/Y/Z轴、垂直/平行屏幕、自定义、与拾取几何垂直
        QComboBox* sectionNormalCombo = nullptr;
        QLineEdit* sectionNormalCustomVec = nullptr;
        QToolButton* sectionNormalPickBtn = nullptr;
        QToolButton* sectionNormalReverseBtn = nullptr;
        QLabel* sectionNormalCustomLabel = nullptr;
        int sectionNormalLastLineID = -1;

        // 截面基准点：剪切中心/形心/自定义(拾取几何点自动填充坐标)
        QComboBox* sectionBasePointCombo = nullptr;
        QLineEdit* sectionBasePointCustomPnt = nullptr;
        QToolButton* sectionBasePointPickBtn = nullptr;
        QLabel* sectionBasePointCustomLabel = nullptr;
        int sectionBasePointLastPntID = -1;

        // Y轴方向向量：X/Y/Z轴、垂直/平行屏幕、自定义、自动
        QComboBox* sectionYAxisCombo = nullptr;
        QLineEdit* sectionYAxisCustomVec = nullptr;
        QToolButton* sectionYAxisPickBtn = nullptr;
        QToolButton* sectionYAxisReverseBtn = nullptr;
        QLabel* sectionYAxisCustomLabel = nullptr;
        int sectionYAxisLastLineID = -1;

        //按 sectionShape 当前选项切换拾取模式/表格列/壳厚度行显隐
        void updateSectionPickMode();
        //薄壁壳截面属性计算并显示
        void computeShellSectionAction();
        //实体截面属性计算并显示(isFace=true 直接拾取面，否则由线构面)
        void computeSolidSectionAction(bool isFace);
        //把界面上 梁截面平面法向/截面基准点/Y轴方向向量 三个下拉框设置
        //转换为 occsp::SectionCoordinateOptions(参照分割面 getDir 的方向提取逻辑)
        //occsp::SectionCoordinateOptions makeSectionCoordinateOptions(); //【OCC摘除·S10 保档】occsp 随 SectionAnalysis 冻结
        //自定义截面：属性计算完成后把全部属性数值填入列表，
        //目前只将列表中的面积/扭转矩/惯性积填入 _dataTable 自定义表，其余后续再补
        void showSectionPropertyRows(const QList<double>& sectionValues);


        QList<double> _cachedData;   // 上次收到的数据
    private:
        void buildUI();
        void initParamTable();
        void initDataTable();

        void addParam(int r, const QString& title, double val);
        void addData(int r, const QString& txt, double v = qQNaN());

        //36套老函数整体搬进来
        void setCustomPara(const QList<QString>& para);
        void setHSecPara(const QList<double>& para, QString nsm);
        void setI1SecPara(const QList<double>& para, QString nsm);
        void setISecPara(const QList<double>& para, QString nsm);
        void setLSecPara(const QList<double>& para, QString nsm);
        void setTSecPara(const QList<double>& para, QString nsm);
        void setRecSecPara(const QList<double>& para, QString nsm);
        void setCirSecPara(const QList<double>& para, QString nsm);
        void setAnnSecPara(const QList<double>& para, QString nsm);
        void setBoxSecPara(const QList<double>& para, QString nsm);
        void setPolySecPara(const QList<double>& para, QString nsm);
        void setChanSecPara(const QList<double>& para, QString nsm);
        void setChan1SecPara(const QList<double>& para, QString nsm);
        void setChan2SecPara(const QList<double>& para, QString nsm);
        void setT1SecPara(const QList<double>& para, QString nsm);
        void setT2SecPara(const QList<double>& para, QString nsm);
        void setBox1SecPara(const QList<double>& para, QString nsm);
        void setCrossSecPara(const QList<double>& para, QString nsm);
        void setHatSecPara(const QList<double>& para, QString nsm);
        void setZSecPara(const QList<double>& para, QString nsm);


        /*void setI1SecData(const QList<double>& secData);
        void setISecData(const QList<double>& secData);
        void setLSecData(const QList<double>& secData);
        void setTSecData(const QList<double>& secData);
        void setRecSecData(const QList<double>& secData);
        void setCirSecData(const QList<double>& secData);
        void setAnnSecData(const QList<double>& secData);
        void setBoxSecData(const QList<double>& secData);
        void setPolySecData(const QList<double>& secData);
        void setChanSecData(const QList<double>& secData);
        void setChan1SecData(const QList<double>& secData);
        void setT1SecData(const QList<double>& secData);
        void setT2SecData(const QList<double>& secData);
        void setBox1SecData(const QList<double>& secData);
        void setCrossSecData(const QList<double>& secData);
        void setHatSecData(const QList<double>& secData);
        void setZSecData(const QList<double>& secData);*/


        /* 上下文（暂时留空，后续业务再填） */
        QList<double> _para;
        QList<QString> _customPara;
        QString       _nsm;
        QString       _name;

        bool          _edit = false;

        MechSectionShapeData::mSectionShapeBase* _sectionShapeData = nullptr;

        /* 折叠根节点 */
        FoldSection* _foldParam = nullptr;  // 参数区
        FoldSection* _foldData = nullptr;  // 数据区
        FoldSection* _foldSectionBuild = nullptr;  // 构建梁截面区(仅自定义截面显示)

        //按截面类型显隐构建梁截面区(仅自定义截面可见)
        void updateBuildSectionFoldVisible();
        //自定义截面隐藏 PropShape 预览窗口并恢复主渲染视口显示(便于拾取几何)，
        //其他截面保持原逻辑(PropShape 显示截面图)
        void updateGeoPartsVisibleForSection(bool showParts);


    public slots:
        void slotGetParaFormTabel(int r, int c);
        void slotShowParaOnPanel(QList<double>, QString nsm, QString);
        //构建梁截面:确认按钮，按截面类型/拾取模式调用实体或薄壁壳属性计算
        void slot_sectionAction();
        //拾取变化时把几何ID填充进 pickObjTable(薄壁壳同时按行写入默认壳厚度)
        void slot_sectionPickChanged();
        //视口切换时重连拾取信号(参照 mGeoTransformPanel::slot_subWindowActivated)
        void slot_subWindowActivated();



    };

} // namespace MxBeam
