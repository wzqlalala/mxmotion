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
#include <QLabel>
#include <QRadioButton>
#include <QSlider>
#include <QPushButton>
#include <QToolButton>
#include <QLineEdit>

#include "mPanelBase.h"
#include "mMeshPanelBase.h"

#include "mPhysicalFieldEnum.h"

#include "mMeshThread.h"
#include "mMeshPanel.h"

#include "mxr/view/HighlightController.h"
using namespace MxFunctions;
using namespace MGUIMesh;
namespace MechPreData
{
	class mViewportModel;
}
class MXMesh;
namespace MechMainWindow
{

    //网格法向面板：壳（2D）/ 梁（1D）
    class MMAINWINDOW_EXPORT mMeshNormalPanel :public mMeshPanel
    {
    public:

        mMeshNormalPanel(QWidget* parent = Q_NULLPTR);

        ~mMeshNormalPanel();

        void newMeshPanel() override;
        //更新面板
        void editMeshPanel(int meshId);
        //多选网格子节点面板
        void getMeshInfoById(const std::vector<int> ids);
        //根据数据初始化面板
        void updatePanel() override;
        //初始化默认尺寸
        void setMeshSize();
        //初始化默认参数
        void initSize() override;

    protected:
        //面板切出（隐藏）时清除法向渲染（箭头/棱锥符号、正反面颜色）
        void hideEvent(QHideEvent* event) override;

    private:
        // 计算模型包围盒尺寸，用于符号大小缩放
        float computeModelScale() const;
        // 统一删除当前法向符号
        void clearNormalSymbol();
        // 根据当前显示样式刷新参数折叠栏
        void updateParamFoldByStyle();
        // 构建箭头/棱锥参数行
        void buildArrowParams();
        // 构建颜色样式参数行
        void buildColorParams();

        //获取当前目标维度：壳=2，梁=1
        int targetDim() const;
        //验证单元维度是否符合当前类型
        bool checkElementDim(const RenderPickResultData& data) const;
        //从当前拾取容器中获取并过滤目标单元
        std::vector<RenderPickResultData> pickTargetElements();
        //高亮已选单元，不符合要求的取消高亮
        void highlightSelected();
        //清空选择并取消高亮
        void clearSelectionAndHighlight();
        //仅清空高亮，不清除已选数据
        void clearHighlightOnly();
        //获取要渲染的单元列表：若手动选择为空则返回全部可见目标单元
        std::vector<RenderPickResultData> getRenderElements() const;

        //计算壳（2D）法向图标数据
        void computeShellNormalData(const std::vector<RenderPickResultData>& elements,
            QVector<std::pair<QVector3D, QVector3D>>& posDir) const;
        //计算梁（1D）法向图标数据
        void computeBeamNormalData(const std::vector<RenderPickResultData>& elements,
            QVector<std::pair<QVector3D, QVector3D>>& posDir) const;

        //渲染法向
        void renderNormal();
        //反转当前选中的单元法向（真正修改数据）
        void reverseSelectedNormals();

        void onTypeChanged();
        // 统一设置（预留接口）
        void unifySetNormals();
        // 解析参考方向字符串，例如 "1,0,0"
        QVector3D parseVectorImpl(const QString& text);

        //对象折叠栏
        FoldSection* m_ObjectFold = nullptr;
        //参数折叠栏
        FoldSection* m_ParamFold = nullptr;
        //法向调整折叠栏
        FoldSection* m_AdjustFold = nullptr;

        //类型选择
        QLabel* typeLabel = nullptr;
        QRadioButton* shellType = nullptr;   //壳（2D）
        QRadioButton* beamType = nullptr;    //梁（1D）

        //对象拾取
        PosPickWidget1 posPick;

        //显示样式（样式行保留成员，因切换类型时梁隐藏样式行）
        QWidget* styleWid = nullptr;
        QLabel* styleLabel = nullptr;
        QRadioButton* arrowType = nullptr;
        QRadioButton* colorType = nullptr;
        QRadioButton* pyramidType = nullptr;

        //箭头/颜色/棱锥的当前状态
        QVector3D curColor0 = QVector3D(128, 0, 128);   //箭头/棱锥颜色
        QVector3D curColor1 = QVector3D(128, 0, 128); //正面颜色
        QVector3D curColor2 = QVector3D(255, 255, 0); //反面颜色
        float m_arrowScale = 1.0f;      // 模型尺寸比例
        float m_arrowSizeFactor = 1.0f; // 箭头大小滑动条因子（0.5~3.0）

        //操作按钮（壳）
        QPushButton* unifyBtn = nullptr;
        QPushButton* reversedBtn = nullptr;
        QPushButton* showBtn = nullptr;

        //法向调整方式
        QWidget* adjustModeWid = nullptr;
        QLabel* adjustModeLabel = nullptr;
        QRadioButton* autoNormalRadio = nullptr;
        QRadioButton* refDirRadio = nullptr;

        //参考方向向量
        QLabel* refDirLabel = nullptr;
        QLineEdit* refDirEdit = nullptr;
        //参考方向解析值
        QVector3D m_refDirValue = QVector3D(0, 0, 1);

        //已选择的目标单元
        std::vector<RenderPickResultData> MeshIDs;

    private:
        // 标记是否已主动点击显示按钮，控制参数修改是否自动渲染
        bool m_hasRenderShown = false;
    };
}
