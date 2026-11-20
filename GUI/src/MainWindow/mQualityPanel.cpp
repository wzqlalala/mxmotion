//Mesh
#include "mMeshQualityCheck.h"

//Mech
#include "mQualityPanel.h"
#include "mDockLayoutManager.h"
#include "mTableDataWindow.h"
#include "mOtherFunction.h"

using namespace MechPreData;
using namespace MeshPanelData;

Q_DECLARE_METATYPE(std::set<int>)
Q_DECLARE_METATYPE(MeshParam)          // 头文件

namespace MechMainWindow
{
    mQualityPanel::mQualityPanel(QWidget* parent)
        : mMeshPanel(parent)
    {
        auto* mainLay = new QVBoxLayout(this);

      

        FoldSection* m_defineFold = new FoldSection("网格定义", this);
        line = new QCheckBox("线网格");
        tri = new QCheckBox("三角形"); tri->setFixedWidth(160);
        quad = new QCheckBox("四边形");
        tet = new QCheckBox("四面体"); tet->setFixedWidth(160);
        hex = new QCheckBox("六面体");
        wedge = new QCheckBox("三棱柱"); wedge->setFixedWidth(160);
        pyramid = new QCheckBox("金字塔");

        m_defineFold->addRow(line);
        m_defineFold->addRow(tri,quad);
        m_defineFold->addRow(tet, hex);
        m_defineFold->addRow(wedge, pyramid);

        FoldSection* m_sizeFold = new FoldSection("几何遍历与尺寸", this);

        initMetrics();
        sizeTable = new QTableWidget;
        sizeTable->setColumnCount(3);
        sizeTable->setRowCount(sizeMap.size());
        for (int i = 0; i < sizeMap.size(); i++)
        {
           QCheckBox* box = new QCheckBox(sizeMap[i]);
           box->setChecked(true);
           sizeTable->setCellWidget(i,0,box);
           sizeTable->setItem(i,1, new QTableWidgetItem(MetricMap[sizeMap[i]].min)); sizeTable->item(i, 1)->setTextAlignment(Qt::AlignCenter);
           sizeTable->setItem(i,2, new QTableWidgetItem(MetricMap[sizeMap[i]].max)); sizeTable->item(i, 2)->setTextAlignment(Qt::AlignCenter);

        }

        sizeTable->setHorizontalHeaderLabels({ "检查指标",">","<" });
        sizeTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);  // 自动拉伸
        m_sizeFold->addRow(sizeTable);

        FoldSection* m_shapeFold = new FoldSection("角度与形状偏差", this);
        shapeTable = new QTableWidget;
        shapeTable->setColumnCount(3);
        shapeTable->setRowCount(shapeMap.size());
        for (int i = 0; i < shapeMap.size(); i++)
        {
            QCheckBox* box = new QCheckBox(shapeMap[i]);
            box->setChecked(true);
            shapeTable->setCellWidget(i, 0, box);
            shapeTable->setItem(i, 1, new QTableWidgetItem(MetricMap[shapeMap[i]].min)); shapeTable->item(i, 1)->setTextAlignment(Qt::AlignCenter);
            shapeTable->setItem(i, 2, new QTableWidgetItem(MetricMap[shapeMap[i]].max)); shapeTable->item(i, 2)->setTextAlignment(Qt::AlignCenter);
        }

        shapeTable->setHorizontalHeaderLabels({ "检查指标",">","<" });
        shapeTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);  // 自动拉伸
        m_shapeFold->addRow(shapeTable);
       
        FoldSection* m_paramFold = new FoldSection("代数映射与稳定性", this);
        paramTable = new QTableWidget;
        paramTable->setColumnCount(3);
        paramTable->setRowCount(paramMap.size());
        for (int i = 0; i < paramMap.size(); i++)
        {
            QCheckBox* box = new QCheckBox(paramMap[i]);
            box->setChecked(true);
            paramTable->setCellWidget(i, 0, box);
            paramTable->setItem(i, 1, new QTableWidgetItem(MetricMap[paramMap[i]].min)); paramTable->item(i, 1)->setTextAlignment(Qt::AlignCenter);
            paramTable->setItem(i, 2, new QTableWidgetItem(MetricMap[paramMap[i]].max)); paramTable->item(i, 2)->setTextAlignment(Qt::AlignCenter);
        }

        paramTable->setHorizontalHeaderLabels({ "检查指标",">","<" });
        paramTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);  // 自动拉伸
        m_paramFold->addRow(paramTable);
        QFrame* horizontalLine = createHorizontalLine();
        QHBoxLayout* hlayout = new QHBoxLayout;
        QPushButton* chooseBtn = new QPushButton("全选");      
        QPushButton* clearBtn = new QPushButton("清空选择");      
        QPushButton* applyBtn = new QPushButton("检查");
        hlayout->addWidget(chooseBtn);
        hlayout->addWidget(clearBtn);
        hlayout->addWidget(applyBtn);
        mainLay->addWidget(m_defineFold);
        mainLay->addWidget(m_sizeFold);
        mainLay->addWidget(m_shapeFold);
        mainLay->addWidget(m_paramFold);
        mainLay->addWidget(horizontalLine);
        mainLay->addLayout(hlayout);
        mainLay->addStretch();

        setDisableAll();

        //连接信号和槽
        connect(line, &QCheckBox::stateChanged, this, &mQualityPanel::setTableEnable);
        connect(tri, &QCheckBox::stateChanged, this, &mQualityPanel::setTableEnable);
        connect(quad, &QCheckBox::stateChanged, this, &mQualityPanel::setTableEnable);
        connect(tet, &QCheckBox::stateChanged, this, &mQualityPanel::setTableEnable);
        connect(hex, &QCheckBox::stateChanged, this, &mQualityPanel::setTableEnable);
        connect(wedge, &QCheckBox::stateChanged, this, &mQualityPanel::setTableEnable);
        connect(pyramid, &QCheckBox::stateChanged, this, &mQualityPanel::setTableEnable);
        connect(applyBtn, &QPushButton::clicked, this, &mQualityPanel::analysis);
        connect(chooseBtn, &QPushButton::clicked, this, &mQualityPanel::chooseAllCheckBox);
        connect(clearBtn, &QPushButton::clicked, this, &mQualityPanel::clearAllCheckBox);
    }

    mQualityPanel::~mQualityPanel()
    {

    }

    void mQualityPanel::setDisableAll()
    {
        for (int i=0; i < sizeMap.size(); i++)
        {
            setRowEnabled(sizeTable, i, false);
        }
        for (int i = 0; i < shapeMap.size(); i++)
        {
            setRowEnabled(shapeTable, i, false);
        }
        for (int i = 0; i < paramMap.size(); i++)
        {
            setRowEnabled(paramTable, i, false);
        }
        renumberVisibleRows(sizeTable);
        renumberVisibleRows(shapeTable);
        renumberVisibleRows(paramTable);
    }
    void mQualityPanel::initMetrics()
    {
        for (const QString& name : Metric) {
            // 方式A：使用 operator[]（简洁，自动创建默认值）
            MetricMap[name] = MetricMinMax{ "", "" };
        }
        MetricMap["长度"] = MetricMinMax{"0","20"};
        MetricMap["翘曲度"] = MetricMinMax{"5",""};
        MetricMap["纵横比"] = MetricMinMax{"5",""};
        MetricMap["偏斜度"] = MetricMinMax{"60",""};
        MetricMap["挤压指数"] = MetricMinMax{"0.5",""};
        MetricMap["雅可比"] = MetricMinMax{"","0.7"};
        MetricMap["等角偏斜"] = MetricMinMax{"0.6",""};
        MetricMap["锥度"] = MetricMinMax{"0.5",""};
        MetricMap["最小角"] = MetricMinMax{"","20"};
        MetricMap["最大角"] = MetricMinMax{"120",""};
        MetricMap["坍塌比"] = MetricMinMax{"0.1",""};
        MetricMap["等体积偏斜"] = MetricMinMax{"0.95",""};

    }
    void mQualityPanel::analysis()
    {
        ctx = new QualityCheckContext;
        if (line->isChecked())
        {
            ctx->settings.line.enable_all();
        }
        if (tri->isChecked())
        {          
            ctx->settings.tri.enable_all();
        }
        if (quad->isChecked())
        {
            ctx->settings.quad.enable_all();
        }
        if (tet->isChecked())
        {
            ctx->settings.tet.enable_all();
        }
        if (hex->isChecked())
        {
            ctx->settings.hex.enable_all();
        }
        if (wedge->isChecked())
        {
            ctx->settings.wedge.enable_all();
        }
        if (pyramid->isChecked())
        {
            ctx->settings.pyramid.enable_all();
        }

        if (getCVM == nullptr) return;
        auto* modelData = getCVM->getModelData();
        if (!modelData) return;

        ctx->meshData = modelData->getMeshData();
        if (!ctx->meshData) return;

        meshQualityCheck(*ctx);

        mTableDataWindow::getInstance()->resetInitialTable();
        mTableDataWindow::getInstance()->setTableTitle("质量统计");
        QStringList header;
        header << "检查指标"<<"最大值"<<"最小值"<<"平均值";
        mTableDataWindow::getInstance()->getTableWidget()->clear();
        mTableDataWindow::getInstance()->getTableWidget()->setRowCount(Metric.size());
        mTableDataWindow::getInstance()->getTableWidget()->setColumnCount(4);

        //第一列 各项指标
        for (int i = 0; i < Metric.size(); i++)
        {
            if(i< sizeMap.size())
                mTableDataWindow::getInstance()->getTableWidget()->setItem(i, 0, new QTableWidgetItem(Metric[i]));
            else if(i< (sizeMap.size()+shapeMap.size()))
                mTableDataWindow::getInstance()->getTableWidget()->setItem(i, 0,new QTableWidgetItem(Metric[i]));
            else
                mTableDataWindow::getInstance()->getTableWidget()->setItem(i, 0, new QTableWidgetItem(Metric[i]));
        }

        //设置最大值、最小值、平均值
        setTableMaxMin();
        mTableDataWindow::getInstance()->setHorizontalHeaders(header);
        mDockLayoutManager::getInstance()->activateDock("Table");
       
    }
    void mQualityPanel::chooseAllCheckBox()
    {
       
        sizeTable->setUpdatesEnabled(false);  // 性能优化

        for (int row = 0; row < sizeTable->rowCount(); ++row) {
            // 获取 cellWidget 并转换为 QCheckBox
            QCheckBox* checkBox = qobject_cast<QCheckBox*>(sizeTable->cellWidget(row, 0));

            if (checkBox) {  // 必须检查是否为 nullptr
                checkBox->setChecked(true);  // 勾选
                // checkBox->setCheckState(Qt::Checked); // 也可用 setCheckState
            }
        }

        sizeTable->setUpdatesEnabled(true);
        
        shapeTable->setUpdatesEnabled(false);  // 性能优化

        for (int row = 0; row < shapeTable->rowCount(); ++row) {
            // 获取 cellWidget 并转换为 QCheckBox
            QCheckBox* checkBox = qobject_cast<QCheckBox*>(shapeTable->cellWidget(row, 0));

            if (checkBox) {  // 必须检查是否为 nullptr
                checkBox->setChecked(true);  // 勾选
                // checkBox->setCheckState(Qt::Checked); // 也可用 setCheckState
            }
        }

        shapeTable->setUpdatesEnabled(true);
        paramTable->setUpdatesEnabled(false);  // 性能优化

        for (int row = 0; row < paramTable->rowCount(); ++row) {
            // 获取 cellWidget 并转换为 QCheckBox
            QCheckBox* checkBox = qobject_cast<QCheckBox*>(paramTable->cellWidget(row, 0));

            if (checkBox) {  // 必须检查是否为 nullptr
                checkBox->setChecked(true);  // 勾选
                // checkBox->setCheckState(Qt::Checked); // 也可用 setCheckState
            }
        }

        paramTable->setUpdatesEnabled(true);

    }
    void mQualityPanel::clearAllCheckBox()
    {
        sizeTable->setUpdatesEnabled(false);  // 性能优化

        for (int row = 0; row < sizeTable->rowCount(); ++row) {
            // 获取 cellWidget 并转换为 QCheckBox
            QCheckBox* checkBox = qobject_cast<QCheckBox*>(sizeTable->cellWidget(row, 0));

            if (checkBox) {  // 必须检查是否为 nullptr
                checkBox->setChecked(false);  // 勾选
                // checkBox->setCheckState(Qt::Checked); // 也可用 setCheckState
            }
        }

        sizeTable->setUpdatesEnabled(true);

        shapeTable->setUpdatesEnabled(false);  // 性能优化

        for (int row = 0; row < shapeTable->rowCount(); ++row) {
            // 获取 cellWidget 并转换为 QCheckBox
            QCheckBox* checkBox = qobject_cast<QCheckBox*>(shapeTable->cellWidget(row, 0));

            if (checkBox) {  // 必须检查是否为 nullptr
                checkBox->setChecked(false);  // 勾选
                // checkBox->setCheckState(Qt::Checked); // 也可用 setCheckState
            }
        }

        shapeTable->setUpdatesEnabled(true);
        paramTable->setUpdatesEnabled(false);  // 性能优化

        for (int row = 0; row < paramTable->rowCount(); ++row) {
            // 获取 cellWidget 并转换为 QCheckBox
            QCheckBox* checkBox = qobject_cast<QCheckBox*>(paramTable->cellWidget(row, 0));

            if (checkBox) {  // 必须检查是否为 nullptr
                checkBox->setChecked(false);  // 勾选
                // checkBox->setCheckState(Qt::Checked); // 也可用 setCheckState
            }
        }

        paramTable->setUpdatesEnabled(true);
    }
    void mQualityPanel::editMeshPanel(int meshId)
    {
        _meshPanelData = getCVM->getModelData()->getManager<mMeshPanelManager>(Physical_MeshPanel).getMeshPanelData(meshId);
        auto a = _meshPanelData->getMeshPanelType();
        updatePanel();

    }


    // 禁用/启用指定行
    void mQualityPanel::setRowEnabled(QTableWidget* table, int row, bool enabled) {
        if (!table || row < 0 || row >= table->rowCount()) return;

        for (int col = 0; col < table->columnCount(); ++col) {
            QTableWidgetItem* item = table->item(row, col);
            if (!item) {
                item = new QTableWidgetItem();
                table->setItem(row, col, item);
            }

            if (enabled) {
                // 启用：恢复所有标志
                //item->setFlags(item->flags() | Qt::ItemIsEnabled | Qt::ItemIsSelectable);
                // 恢复外观
                item->setBackground(QBrush(Qt::white));
                item->setForeground(QBrush(Qt::black));
                table->showRow(row);
            }
            else {
                // 禁用：移除可交互标志
                //item->setFlags(item->flags() & ~(Qt::ItemIsEnabled | Qt::ItemIsEditable));
                // 设置灰色外观（视觉反馈）
                item->setBackground(QBrush(QColor(240, 240, 240)));
                item->setForeground(QBrush(Qt::gray));
                table->hideRow(row);
            }
        }

        // **重要**：处理通过 setCellWidget 添加的控件
        for (int col = 0; col < table->columnCount(); ++col) {
            QWidget* widget = table->cellWidget(row, col);
            if (widget) {
                widget->setEnabled(enabled);
            }
        }
    }
    //按可见顺序重排表格垂直表头序号：可见行连续编号，隐藏行留空，避免显示的序号跳号
    void mQualityPanel::renumberVisibleRows(QTableWidget* table)
    {
        if (!table) return;
        QStringList rowLabels;
        int visibleSeq = 0;
        for (int row = 0; row < table->rowCount(); ++row)
        {
            rowLabels << (table->isRowHidden(row) ? QString() : QString::number(++visibleSeq));
        }
        table->setVerticalHeaderLabels(rowLabels);
    }
    void mQualityPanel::setTableEnable()
    {
        setDisableAll();         
        if (line->isChecked())
        {
            setRowEnabled(sizeTable, std::find(sizeMap.begin(), sizeMap.end(), "长度") - sizeMap.begin(), true);
        }
        if (tri->isChecked())
        {
            setRowEnabled(sizeTable, std::find(sizeMap.begin(), sizeMap.end(), "边长比") - sizeMap.begin(), true);
            setRowEnabled(sizeTable, std::find(sizeMap.begin(), sizeMap.end(), "纵横比") - sizeMap.begin(), true);
            setRowEnabled(sizeTable, std::find(sizeMap.begin(), sizeMap.end(), "半径比") - sizeMap.begin(), true);
            setRowEnabled(sizeTable, std::find(sizeMap.begin(), sizeMap.end(), "Frobenius纵横比") - sizeMap.begin(), true);
            setRowEnabled(sizeTable, std::find(sizeMap.begin(), sizeMap.end(), "面积") - sizeMap.begin(), true);
            setRowEnabled(sizeTable, std::find(sizeMap.begin(), sizeMap.end(), "归一化内切圆半径") - sizeMap.begin(), true);
            setRowEnabled(sizeTable, std::find(sizeMap.begin(), sizeMap.end(), "相对尺寸平方") - sizeMap.begin(), true);
            //setRowEnabled(sizeTable, std::find(sizeMap.begin(), sizeMap.end(), "最大边长比") - sizeMap.begin(), true);

            setRowEnabled(shapeTable, std::find(shapeMap.begin(), shapeMap.end(), "最小角") - shapeMap.begin(), true);
            setRowEnabled(shapeTable, std::find(shapeMap.begin(), shapeMap.end(), "最大角") - shapeMap.begin(), true);
            setRowEnabled(shapeTable, std::find(shapeMap.begin(), shapeMap.end(), "形状") - shapeMap.begin(), true);
            setRowEnabled(shapeTable, std::find(shapeMap.begin(), shapeMap.end(), "等角偏斜") - shapeMap.begin(), true);
            setRowEnabled(shapeTable, std::find(shapeMap.begin(), shapeMap.end(), "形状和尺寸") - shapeMap.begin(), true);

            setRowEnabled(paramTable, std::find(paramMap.begin(), paramMap.end(), "条件数") - paramMap.begin(), true);
            setRowEnabled(paramTable, std::find(paramMap.begin(), paramMap.end(), "缩放雅可比") - paramMap.begin(), true);
            setRowEnabled(paramTable, std::find(paramMap.begin(), paramMap.end(), "畸变") - paramMap.begin(), true);
            setRowEnabled(paramTable, std::find(paramMap.begin(), paramMap.end(), "增强型正交质量") - paramMap.begin(), true);
        }
        if (quad->isChecked())
        {
            setRowEnabled(sizeTable, std::find(sizeMap.begin(), sizeMap.end(), "边长比") - sizeMap.begin(), true);
            setRowEnabled(sizeTable, std::find(sizeMap.begin(), sizeMap.end(), "纵横比") - sizeMap.begin(), true);
            setRowEnabled(sizeTable, std::find(sizeMap.begin(), sizeMap.end(), "半径比") - sizeMap.begin(), true);
            //四边形后端不计算Frobenius纵横比（仅有中值/最大两种），启用会显示N/A，故不启用
            //setRowEnabled(sizeTable, std::find(sizeMap.begin(), sizeMap.end(), "Frobenius纵横比") - sizeMap.begin(), true);
            setRowEnabled(sizeTable, std::find(sizeMap.begin(), sizeMap.end(), "面积") - sizeMap.begin(), true);
            //setRowEnabled(sizeTable, std::find(sizeMap.begin(), sizeMap.end(), "归一化内切圆半径") - sizeMap.begin(), true);
            setRowEnabled(sizeTable, std::find(sizeMap.begin(), sizeMap.end(), "相对尺寸平方") - sizeMap.begin(), true);
            setRowEnabled(sizeTable, std::find(sizeMap.begin(), sizeMap.end(), "最大边长比") - sizeMap.begin(), true);
            setRowEnabled(sizeTable, std::find(sizeMap.begin(), sizeMap.end(), "中值Frobenius纵横比") - sizeMap.begin(), true);
            setRowEnabled(sizeTable, std::find(sizeMap.begin(), sizeMap.end(), "最大Frobenius纵横比") - sizeMap.begin(), true);

            setRowEnabled(shapeTable, std::find(shapeMap.begin(), shapeMap.end(), "最小角") - shapeMap.begin(), true);
            setRowEnabled(shapeTable, std::find(shapeMap.begin(), shapeMap.end(), "最大角") - shapeMap.begin(), true);
            setRowEnabled(shapeTable, std::find(shapeMap.begin(), shapeMap.end(), "形状") - shapeMap.begin(), true);
            setRowEnabled(shapeTable, std::find(shapeMap.begin(), shapeMap.end(), "等角偏斜") - shapeMap.begin(), true);
            setRowEnabled(shapeTable, std::find(shapeMap.begin(), shapeMap.end(), "形状和尺寸") - shapeMap.begin(), true);
            setRowEnabled(shapeTable, std::find(shapeMap.begin(), shapeMap.end(), "偏斜度") - shapeMap.begin(), true);
            setRowEnabled(shapeTable, std::find(shapeMap.begin(), shapeMap.end(), "锥度") - shapeMap.begin(), true);
            setRowEnabled(shapeTable, std::find(shapeMap.begin(), shapeMap.end(), "翘曲度") - shapeMap.begin(), true);
            setRowEnabled(shapeTable, std::find(shapeMap.begin(), shapeMap.end(), "拉伸度") - shapeMap.begin(), true);
            setRowEnabled(shapeTable, std::find(shapeMap.begin(), shapeMap.end(), "剪切度") - shapeMap.begin(), true);
            setRowEnabled(shapeTable, std::find(shapeMap.begin(), shapeMap.end(), "剪切和尺寸") - shapeMap.begin(), true);

            setRowEnabled(paramTable, std::find(paramMap.begin(), paramMap.end(), "条件数") - paramMap.begin(), true);
            setRowEnabled(paramTable, std::find(paramMap.begin(), paramMap.end(), "缩放雅可比") - paramMap.begin(), true);
            setRowEnabled(paramTable, std::find(paramMap.begin(), paramMap.end(), "畸变") - paramMap.begin(), true);
            setRowEnabled(paramTable, std::find(paramMap.begin(), paramMap.end(), "Oddy指标") - paramMap.begin(), true);
            setRowEnabled(paramTable, std::find(paramMap.begin(), paramMap.end(), "雅可比") - paramMap.begin(), true);
            setRowEnabled(paramTable, std::find(paramMap.begin(), paramMap.end(), "增强型正交质量") - paramMap.begin(), true);
        }
        if (tet->isChecked())
        {
            setRowEnabled(sizeTable, std::find(sizeMap.begin(), sizeMap.end(), "边长比") - sizeMap.begin(), true);
            setRowEnabled(sizeTable, std::find(sizeMap.begin(), sizeMap.end(), "纵横比") - sizeMap.begin(), true);
            setRowEnabled(sizeTable, std::find(sizeMap.begin(), sizeMap.end(), "半径比") - sizeMap.begin(), true);
            setRowEnabled(sizeTable, std::find(sizeMap.begin(), sizeMap.end(), "Frobenius纵横比") - sizeMap.begin(), true);
            setRowEnabled(sizeTable, std::find(sizeMap.begin(), sizeMap.end(), "相对尺寸平方") - sizeMap.begin(), true);
            //四面体后端不计算最大Frobenius纵横比（该指标仅四边形类单元有），启用会显示N/A，故不启用
            //setRowEnabled(sizeTable, std::find(sizeMap.begin(), sizeMap.end(), "最大Frobenius纵横比") - sizeMap.begin(), true);
            setRowEnabled(sizeTable, std::find(sizeMap.begin(), sizeMap.end(), "纵横比Gamma") - sizeMap.begin(), true);
            setRowEnabled(sizeTable, std::find(sizeMap.begin(), sizeMap.end(), "体积") - sizeMap.begin(), true);

            setRowEnabled(shapeTable, std::find(shapeMap.begin(), shapeMap.end(), "最小角") - shapeMap.begin(), true);
            setRowEnabled(shapeTable, std::find(shapeMap.begin(), shapeMap.end(), "形状") - shapeMap.begin(), true);
            setRowEnabled(shapeTable, std::find(shapeMap.begin(), shapeMap.end(), "等角偏斜") - shapeMap.begin(), true);
            setRowEnabled(shapeTable, std::find(shapeMap.begin(), shapeMap.end(), "形状和尺寸") - shapeMap.begin(), true);
            setRowEnabled(shapeTable, std::find(shapeMap.begin(), shapeMap.end(), "坍塌比") - shapeMap.begin(), true);
            setRowEnabled(shapeTable, std::find(shapeMap.begin(), shapeMap.end(), "平均比") - shapeMap.begin(), true);
            setRowEnabled(shapeTable, std::find(shapeMap.begin(), shapeMap.end(), "等体积偏斜") - shapeMap.begin(), true);
            setRowEnabled(shapeTable, std::find(shapeMap.begin(), shapeMap.end(), "挤压指数") - shapeMap.begin(), true);

            setRowEnabled(paramTable, std::find(paramMap.begin(), paramMap.end(), "条件数") - paramMap.begin(), true);
            setRowEnabled(paramTable, std::find(paramMap.begin(), paramMap.end(), "缩放雅可比") - paramMap.begin(), true);
            setRowEnabled(paramTable, std::find(paramMap.begin(), paramMap.end(), "畸变") - paramMap.begin(), true);
            setRowEnabled(paramTable, std::find(paramMap.begin(), paramMap.end(), "雅可比") - paramMap.begin(), true);
            setRowEnabled(paramTable, std::find(paramMap.begin(), paramMap.end(), "增强型正交质量") - paramMap.begin(), true);
        }
        if (hex->isChecked())
        {
            setRowEnabled(sizeTable, std::find(sizeMap.begin(), sizeMap.end(), "边长比") - sizeMap.begin(), true);
            setRowEnabled(sizeTable, std::find(sizeMap.begin(), sizeMap.end(), "相对尺寸平方") - sizeMap.begin(), true);
            setRowEnabled(sizeTable, std::find(sizeMap.begin(), sizeMap.end(), "最大边长比") - sizeMap.begin(), true);
            setRowEnabled(sizeTable, std::find(sizeMap.begin(), sizeMap.end(), "中值Frobenius纵横比") - sizeMap.begin(), true);
            setRowEnabled(sizeTable, std::find(sizeMap.begin(), sizeMap.end(), "最大Frobenius纵横比") - sizeMap.begin(), true);
            setRowEnabled(sizeTable, std::find(sizeMap.begin(), sizeMap.end(), "体积") - sizeMap.begin(), true);
            setRowEnabled(sizeTable, std::find(sizeMap.begin(), sizeMap.end(), "对角线") - sizeMap.begin(), true);
            setRowEnabled(sizeTable, std::find(sizeMap.begin(), sizeMap.end(), "维度") - sizeMap.begin(), true);

            setRowEnabled(shapeTable, std::find(shapeMap.begin(), shapeMap.end(), "形状") - shapeMap.begin(), true);
            setRowEnabled(shapeTable, std::find(shapeMap.begin(), shapeMap.end(), "等角偏斜") - shapeMap.begin(), true);
            setRowEnabled(shapeTable, std::find(shapeMap.begin(), shapeMap.end(), "形状和尺寸") - shapeMap.begin(), true);
            setRowEnabled(shapeTable, std::find(shapeMap.begin(), shapeMap.end(), "偏斜度") - shapeMap.begin(), true);
            setRowEnabled(shapeTable, std::find(shapeMap.begin(), shapeMap.end(), "锥度") - shapeMap.begin(), true);
            setRowEnabled(shapeTable, std::find(shapeMap.begin(), shapeMap.end(), "拉伸度") - shapeMap.begin(), true);
            setRowEnabled(shapeTable, std::find(shapeMap.begin(), shapeMap.end(), "剪切度") - shapeMap.begin(), true);
            setRowEnabled(shapeTable, std::find(shapeMap.begin(), shapeMap.end(), "剪切和尺寸") - shapeMap.begin(), true);

            setRowEnabled(paramTable, std::find(paramMap.begin(), paramMap.end(), "条件数") - paramMap.begin(), true);
            setRowEnabled(paramTable, std::find(paramMap.begin(), paramMap.end(), "缩放雅可比") - paramMap.begin(), true);
            setRowEnabled(paramTable, std::find(paramMap.begin(), paramMap.end(), "畸变") - paramMap.begin(), true);
            setRowEnabled(paramTable, std::find(paramMap.begin(), paramMap.end(), "Oddy指标") - paramMap.begin(), true);
            setRowEnabled(paramTable, std::find(paramMap.begin(), paramMap.end(), "节点雅可比比") - paramMap.begin(), true);
            setRowEnabled(paramTable, std::find(paramMap.begin(), paramMap.end(), "增强型正交质量") - paramMap.begin(), true);
        }
        if (wedge->isChecked())
        {
            setRowEnabled(sizeTable, std::find(sizeMap.begin(), sizeMap.end(), "边长比") - sizeMap.begin(), true);
            setRowEnabled(sizeTable, std::find(sizeMap.begin(), sizeMap.end(), "最大Frobenius纵横比") - sizeMap.begin(), true);
            setRowEnabled(sizeTable, std::find(sizeMap.begin(), sizeMap.end(), "体积") - sizeMap.begin(), true);
            setRowEnabled(sizeTable, std::find(sizeMap.begin(), sizeMap.end(), "平均Frobenius纵横比") - sizeMap.begin(), true);

            setRowEnabled(shapeTable, std::find(shapeMap.begin(), shapeMap.end(), "形状") - shapeMap.begin(), true);
            setRowEnabled(shapeTable, std::find(shapeMap.begin(), shapeMap.end(), "等角偏斜") - shapeMap.begin(), true);
            setRowEnabled(shapeTable, std::find(shapeMap.begin(), shapeMap.end(), "最大拉伸") - shapeMap.begin(), true);

            setRowEnabled(paramTable, std::find(paramMap.begin(), paramMap.end(), "条件数") - paramMap.begin(), true);
            setRowEnabled(paramTable, std::find(paramMap.begin(), paramMap.end(), "缩放雅可比") - paramMap.begin(), true);
            setRowEnabled(paramTable, std::find(paramMap.begin(), paramMap.end(), "雅可比") - paramMap.begin(), true);
            setRowEnabled(paramTable, std::find(paramMap.begin(), paramMap.end(), "畸变") - paramMap.begin(), true);
        }
        if (pyramid->isChecked())
        {
            setRowEnabled(sizeTable, std::find(sizeMap.begin(), sizeMap.end(), "体积") - sizeMap.begin(), true);


            setRowEnabled(shapeTable, std::find(shapeMap.begin(), shapeMap.end(), "形状") - shapeMap.begin(), true);
            setRowEnabled(shapeTable, std::find(shapeMap.begin(), shapeMap.end(), "等角偏斜") - shapeMap.begin(), true);



            setRowEnabled(paramTable, std::find(paramMap.begin(), paramMap.end(), "缩放雅可比") - paramMap.begin(), true);
            setRowEnabled(paramTable, std::find(paramMap.begin(), paramMap.end(), "雅可比") - paramMap.begin(), true);
    

        }
        renumberVisibleRows(sizeTable);
        renumberVisibleRows(shapeTable);
        renumberVisibleRows(paramTable);
    }
    void mQualityPanel::setTableMaxMin()
    {
        QualityMaxMin qualityMaxMin;
        setLine(qualityMaxMin);
        setTri(qualityMaxMin);
        setQuad(qualityMaxMin);
        setTet(qualityMaxMin);
        setHex(qualityMaxMin);
        setWedge(qualityMaxMin);
        setPyramid(qualityMaxMin);
        //计算最大最小值

        QualityAggregator aggregator(qualityMaxMin);
        aggregator.aggregate(qualityMaxMin.lineMaxMin);
        aggregator.aggregate(qualityMaxMin.triMaxMin);
        aggregator.aggregate(qualityMaxMin.quadMaxMin);
        aggregator.aggregate(qualityMaxMin.tetMaxMin);
        aggregator.aggregate(qualityMaxMin.hexMaxMin);
        aggregator.aggregate(qualityMaxMin.wedgeMaxMin);
        aggregator.aggregate(qualityMaxMin.pyramidMaxMin);
        aggregator.computeQualityMaxMinAverages();
            

        
        mTableDataWindow::getInstance()->getTableWidget()->setItem(0, 1, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.edge_ratio.second)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(0, 2, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.edge_ratio.first)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(1, 1, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.length.second)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(1, 2, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.length.first)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(2, 1, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.aspect_ratio.second)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(2, 2, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.aspect_ratio.first)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(3, 1, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.aspect_frobenius.second)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(3, 2, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.aspect_frobenius.first)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(4, 1, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.radius_ratio.second)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(4, 2, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.radius_ratio.first)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(5, 1, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.area.second)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(5, 2, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.area.first)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(6, 1, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.normalized_inradius.second)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(6, 2, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.normalized_inradius.first)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(7, 1, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.relative_size_squared.second)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(7, 2, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.relative_size_squared.first)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(8, 1, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.max_edge_ratio.second)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(8, 2, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.max_edge_ratio.first)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(9, 1, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.med_aspect_frobenius.second)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(9, 2, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.med_aspect_frobenius.first)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(10, 1, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.max_aspect_frobenius.second)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(10, 2, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.max_aspect_frobenius.first)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(11, 1, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.aspect_gamma.second)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(11, 2, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.aspect_gamma.first)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(12, 1, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.volume.second)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(12, 2, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.volume.first)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(13, 1, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.diagonal.second)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(13, 2, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.diagonal.first)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(14, 1, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.dimension.second)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(14, 2, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.dimension.first)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(15, 1, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.mean_aspect_frobenius.second)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(15, 2, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.mean_aspect_frobenius.first)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(16, 1, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.minimum_angle.second)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(16, 2, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.minimum_angle.first)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(17, 1, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.maximum_angle.second)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(17, 2, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.maximum_angle.first)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(18, 1, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.shape.second)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(18, 2, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.shape.first)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(19, 1, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.equiangle_skew.second)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(19, 2, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.equiangle_skew.first)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(20, 1, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.shape_and_size.second)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(20, 2, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.shape_and_size.first)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(21, 1, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.skew.second)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(21, 2, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.skew.first)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(22, 1, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.taper.second)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(22, 2, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.taper.first)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(23, 1, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.warpage.second)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(23, 2, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.warpage.first)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(24, 1, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.stretch.second)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(24, 2, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.stretch.first)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(25, 1, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.shear.second)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(25, 2, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.shear.first)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(26, 1, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.shear_and_size.second)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(26, 2, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.shear_and_size.first)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(27, 1, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.collapse_ratio.second)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(27, 2, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.collapse_ratio.first)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(28, 1, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.equivolume_skew.second)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(28, 2, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.equivolume_skew.first)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(29, 1, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.squish_index.second)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(29, 2, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.squish_index.first)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(30, 1, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.max_stretch.second)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(30, 2, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.max_stretch.first)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(31, 1, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.mean_ratio.second)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(31, 2, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.mean_ratio.first)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(32, 1, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.condition.second)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(32, 2, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.condition.first)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(33, 1, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.scaled_jacobian.second)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(33, 2, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.scaled_jacobian.first)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(34, 1, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.distortion.second)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(34, 2, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.distortion.first)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(35, 1, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.jacobian.second)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(35, 2, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.jacobian.first)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(36, 1, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.oddy.second)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(36, 2, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.oddy.first)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(37, 1, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.nodal_jacobian_ratio.second)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(37, 2, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.nodal_jacobian_ratio.first)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(38, 1, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.enhanced_orth.second)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(38, 2, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.enhanced_orth.first)));

        mTableDataWindow::getInstance()->getTableWidget()->setItem(0, 3, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.edge_ratio_mean)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(1, 3, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.length_mean)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(2, 3, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.aspect_ratio_mean)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(3, 3, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.aspect_frobenius_mean)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(4, 3, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.radius_ratio_mean)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(5, 3, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.area_mean)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(6, 3, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.normalized_inradius_mean)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(7, 3, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.relative_size_squared_mean)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(8, 3, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.max_edge_ratio_mean)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(9, 3, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.med_aspect_frobenius_mean)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(10, 3, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.max_aspect_frobenius_mean)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(11, 3, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.aspect_gamma_mean)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(12, 3, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.volume_mean)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(13, 3, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.diagonal_mean)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(14, 3, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.dimension_mean)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(15, 3, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.mean_aspect_frobenius_mean)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(16, 3, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.minimum_angle_mean)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(17, 3, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.maximum_angle_mean)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(18, 3, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.shape_mean)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(19, 3, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.equiangle_skew_mean)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(20, 3, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.shape_and_size_mean)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(21, 3, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.skew_mean)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(22, 3, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.taper_mean)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(23, 3, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.warpage_mean)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(24, 3, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.stretch_mean)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(25, 3, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.shear_mean)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(26, 3, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.shear_and_size_mean)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(27, 3, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.collapse_ratio_mean)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(28, 3, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.equivolume_skew_mean)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(29, 3, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.squish_index_mean)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(30, 3, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.max_stretch_mean)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(31, 3, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.mean_ratio_mean)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(32, 3, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.condition_mean)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(33, 3, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.scaled_jacobian_mean)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(34, 3, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.distortion_mean)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(35, 3, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.jacobian_mean)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(36, 3, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.oddy_mean)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(37, 3, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.nodal_jacobian_ratio_mean)));
        mTableDataWindow::getInstance()->getTableWidget()->setItem(38, 3, new QTableWidgetItem(MxFunctions::mNumberToString(qualityMaxMin.enhanced_orth_mean)));

        //若最大最小值不存在，置为N/A
        if (qualityMaxMin.edge_ratio.second == std::numeric_limits<double>::lowest())
            mTableDataWindow::getInstance()->getTableWidget()->item(0, 1)->setText("N/A");
        if (qualityMaxMin.edge_ratio.first == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(0, 2)->setText("N/A");
        if (qualityMaxMin.length.second == std::numeric_limits<double>::lowest())
            mTableDataWindow::getInstance()->getTableWidget()->item(1, 1)->setText("N/A");
        if (qualityMaxMin.length.first == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(1, 2)->setText("N/A");
        if (qualityMaxMin.aspect_ratio.second == std::numeric_limits<double>::lowest())
            mTableDataWindow::getInstance()->getTableWidget()->item(2, 1)->setText("N/A");
        if (qualityMaxMin.aspect_ratio.first == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(2, 2)->setText("N/A");
        if (qualityMaxMin.aspect_frobenius.second == std::numeric_limits<double>::lowest())
            mTableDataWindow::getInstance()->getTableWidget()->item(3, 1)->setText("N/A");
        if (qualityMaxMin.aspect_frobenius.first == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(3, 2)->setText("N/A");
        if (qualityMaxMin.radius_ratio.second == std::numeric_limits<double>::lowest())
            mTableDataWindow::getInstance()->getTableWidget()->item(4, 1)->setText("N/A");
        if (qualityMaxMin.radius_ratio.first == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(4, 2)->setText("N/A");
        if (qualityMaxMin.area.second == std::numeric_limits<double>::lowest())
            mTableDataWindow::getInstance()->getTableWidget()->item(5, 1)->setText("N/A");
        if (qualityMaxMin.area.first == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(5, 2)->setText("N/A");
        if (qualityMaxMin.normalized_inradius.second == std::numeric_limits<double>::lowest())
            mTableDataWindow::getInstance()->getTableWidget()->item(6, 1)->setText("N/A");
        if (qualityMaxMin.normalized_inradius.first == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(6, 2)->setText("N/A");
        if (qualityMaxMin.relative_size_squared.second == std::numeric_limits<double>::lowest())
            mTableDataWindow::getInstance()->getTableWidget()->item(7, 1)->setText("N/A");
        if (qualityMaxMin.relative_size_squared.first == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(7, 2)->setText("N/A");
        if (qualityMaxMin.max_edge_ratio.second == std::numeric_limits<double>::lowest())
            mTableDataWindow::getInstance()->getTableWidget()->item(8, 1)->setText("N/A");
        if (qualityMaxMin.max_edge_ratio.first == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(8, 2)->setText("N/A");
        if (qualityMaxMin.med_aspect_frobenius.second == std::numeric_limits<double>::lowest())
            mTableDataWindow::getInstance()->getTableWidget()->item(9, 1)->setText("N/A");
        if (qualityMaxMin.med_aspect_frobenius.first == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(9, 2)->setText("N/A");
        if (qualityMaxMin.max_aspect_frobenius.second == std::numeric_limits<double>::lowest())
            mTableDataWindow::getInstance()->getTableWidget()->item(10, 1)->setText("N/A");
        if (qualityMaxMin.max_aspect_frobenius.first == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(10, 2)->setText("N/A");
        if (qualityMaxMin.aspect_gamma.second == std::numeric_limits<double>::lowest())
            mTableDataWindow::getInstance()->getTableWidget()->item(11, 1)->setText("N/A");
        if (qualityMaxMin.aspect_gamma.first == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(11, 2)->setText("N/A");
        if (qualityMaxMin.volume.second == std::numeric_limits<double>::lowest())
            mTableDataWindow::getInstance()->getTableWidget()->item(12, 1)->setText("N/A");
        if (qualityMaxMin.volume.first == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(12, 2)->setText("N/A");
        if (qualityMaxMin.diagonal.second == std::numeric_limits<double>::lowest())
            mTableDataWindow::getInstance()->getTableWidget()->item(13, 1)->setText("N/A");
        if (qualityMaxMin.diagonal.first == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(13, 2)->setText("N/A");
        if (qualityMaxMin.dimension.second == std::numeric_limits<double>::lowest())
            mTableDataWindow::getInstance()->getTableWidget()->item(14, 1)->setText("N/A");
        if (qualityMaxMin.dimension.first == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(14, 2)->setText("N/A");
        if (qualityMaxMin.mean_aspect_frobenius.second == std::numeric_limits<double>::lowest())
            mTableDataWindow::getInstance()->getTableWidget()->item(15, 1)->setText("N/A");
        if (qualityMaxMin.mean_aspect_frobenius.first == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(15, 2)->setText("N/A");
        if (qualityMaxMin.minimum_angle.second == std::numeric_limits<double>::lowest())
            mTableDataWindow::getInstance()->getTableWidget()->item(16, 1)->setText("N/A");
        if (qualityMaxMin.minimum_angle.first == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(16, 2)->setText("N/A");
        if (qualityMaxMin.maximum_angle.second == std::numeric_limits<double>::lowest())
            mTableDataWindow::getInstance()->getTableWidget()->item(17, 1)->setText("N/A");
        if (qualityMaxMin.maximum_angle.first == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(17, 2)->setText("N/A");
        if (qualityMaxMin.shape.second == std::numeric_limits<double>::lowest())
            mTableDataWindow::getInstance()->getTableWidget()->item(18, 1)->setText("N/A");
        if (qualityMaxMin.shape.first == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(18, 2)->setText("N/A");
        if (qualityMaxMin.equiangle_skew.second == std::numeric_limits<double>::lowest())
            mTableDataWindow::getInstance()->getTableWidget()->item(19, 1)->setText("N/A");
        if (qualityMaxMin.equiangle_skew.first == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(19, 2)->setText("N/A");
        if (qualityMaxMin.shape_and_size.second == std::numeric_limits<double>::lowest())
            mTableDataWindow::getInstance()->getTableWidget()->item(20, 1)->setText("N/A");
        if (qualityMaxMin.shape_and_size.first == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(20, 2)->setText("N/A");
        if (qualityMaxMin.skew.second == std::numeric_limits<double>::lowest())
            mTableDataWindow::getInstance()->getTableWidget()->item(21, 1)->setText("N/A");
        if (qualityMaxMin.skew.first == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(21, 2)->setText("N/A");
        if (qualityMaxMin.taper.second == std::numeric_limits<double>::lowest())
            mTableDataWindow::getInstance()->getTableWidget()->item(22, 1)->setText("N/A");
        if (qualityMaxMin.taper.first == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(22, 2)->setText("N/A");
        if (qualityMaxMin.warpage.second == std::numeric_limits<double>::lowest())
            mTableDataWindow::getInstance()->getTableWidget()->item(23, 1)->setText("N/A");
        if (qualityMaxMin.warpage.first == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(23, 2)->setText("N/A");
        if (qualityMaxMin.stretch.second == std::numeric_limits<double>::lowest())
            mTableDataWindow::getInstance()->getTableWidget()->item(24, 1)->setText("N/A");
        if (qualityMaxMin.stretch.first == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(24, 2)->setText("N/A");
        if (qualityMaxMin.shear.second == std::numeric_limits<double>::lowest())
            mTableDataWindow::getInstance()->getTableWidget()->item(25, 1)->setText("N/A");
        if (qualityMaxMin.shear.first == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(25, 2)->setText("N/A");
        if (qualityMaxMin.shear_and_size.second == std::numeric_limits<double>::lowest())
            mTableDataWindow::getInstance()->getTableWidget()->item(26, 1)->setText("N/A");
        if (qualityMaxMin.shear_and_size.first == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(26, 2)->setText("N/A");
        if (qualityMaxMin.collapse_ratio.second == std::numeric_limits<double>::lowest())
            mTableDataWindow::getInstance()->getTableWidget()->item(27, 1)->setText("N/A");
        if (qualityMaxMin.collapse_ratio.first == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(27, 2)->setText("N/A");
        if (qualityMaxMin.equivolume_skew.second == std::numeric_limits<double>::lowest())
            mTableDataWindow::getInstance()->getTableWidget()->item(28, 1)->setText("N/A");
        if (qualityMaxMin.equivolume_skew.first == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(28, 2)->setText("N/A");
        if (qualityMaxMin.squish_index.second == std::numeric_limits<double>::lowest())
            mTableDataWindow::getInstance()->getTableWidget()->item(29, 1)->setText("N/A");
        if (qualityMaxMin.squish_index.first == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(29, 2)->setText("N/A");
        if (qualityMaxMin.max_stretch.second == std::numeric_limits<double>::lowest())
            mTableDataWindow::getInstance()->getTableWidget()->item(30, 1)->setText("N/A");
        if (qualityMaxMin.max_stretch.first == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(30, 2)->setText("N/A");
        if (qualityMaxMin.mean_ratio.second == std::numeric_limits<double>::lowest())
            mTableDataWindow::getInstance()->getTableWidget()->item(31, 1)->setText("N/A");
        if (qualityMaxMin.mean_ratio.first == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(31, 2)->setText("N/A");
        if (qualityMaxMin.condition.second == std::numeric_limits<double>::lowest())
            mTableDataWindow::getInstance()->getTableWidget()->item(32, 1)->setText("N/A");
        if (qualityMaxMin.condition.first == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(32, 2)->setText("N/A");
        if (qualityMaxMin.scaled_jacobian.second == std::numeric_limits<double>::lowest())
            mTableDataWindow::getInstance()->getTableWidget()->item(33, 1)->setText("N/A");
        if (qualityMaxMin.scaled_jacobian.first == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(33, 2)->setText("N/A");
        if (qualityMaxMin.distortion.second == std::numeric_limits<double>::lowest())
            mTableDataWindow::getInstance()->getTableWidget()->item(34, 1)->setText("N/A");
        if (qualityMaxMin.distortion.first == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(34, 2)->setText("N/A");
        if (qualityMaxMin.jacobian.second == std::numeric_limits<double>::lowest())
            mTableDataWindow::getInstance()->getTableWidget()->item(35, 1)->setText("N/A");
        if (qualityMaxMin.jacobian.first == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(35, 2)->setText("N/A");
        if (qualityMaxMin.oddy.second == std::numeric_limits<double>::lowest())
            mTableDataWindow::getInstance()->getTableWidget()->item(36, 1)->setText("N/A");
        if (qualityMaxMin.oddy.first == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(36, 2)->setText("N/A");
        if (qualityMaxMin.nodal_jacobian_ratio.second == std::numeric_limits<double>::lowest())
            mTableDataWindow::getInstance()->getTableWidget()->item(37, 1)->setText("N/A");
        if (qualityMaxMin.nodal_jacobian_ratio.first == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(37, 2)->setText("N/A");
        if (qualityMaxMin.enhanced_orth.second == std::numeric_limits<double>::lowest())
            mTableDataWindow::getInstance()->getTableWidget()->item(38, 1)->setText("N/A");
        if (qualityMaxMin.enhanced_orth.first == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(38, 2)->setText("N/A");

        if (qualityMaxMin.edge_ratio_mean == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(0, 3)->setText("N/A");
        if (qualityMaxMin.length_mean == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(1, 3)->setText("N/A");
        if (qualityMaxMin.aspect_ratio_mean == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(2, 3)->setText("N/A");    
        if (qualityMaxMin.aspect_frobenius_mean == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(3, 3)->setText("N/A");    
        if (qualityMaxMin.radius_ratio_mean == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(4, 3)->setText("N/A");     
        if (qualityMaxMin.area_mean == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(5, 3)->setText("N/A");   
        if (qualityMaxMin.normalized_inradius_mean == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(6, 3)->setText("N/A");
        if (qualityMaxMin.relative_size_squared_mean == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(7, 3)->setText("N/A");
        if (qualityMaxMin.max_edge_ratio_mean == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(8, 3)->setText("N/A");
        if (qualityMaxMin.med_aspect_frobenius_mean == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(9, 3)->setText("N/A");
        if (qualityMaxMin.max_aspect_frobenius_mean == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(10, 3)->setText("N/A");
        if (qualityMaxMin.aspect_gamma_mean == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(11, 3)->setText("N/A");
        if (qualityMaxMin.volume_mean == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(12, 3)->setText("N/A");
        if (qualityMaxMin.diagonal_mean == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(13, 3)->setText("N/A");
        if (qualityMaxMin.dimension_mean == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(14, 3)->setText("N/A");
        if (qualityMaxMin.mean_aspect_frobenius_mean == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(15, 3)->setText("N/A");
        if (qualityMaxMin.minimum_angle_mean == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(16, 3)->setText("N/A");
        if (qualityMaxMin.maximum_angle_mean == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(17, 3)->setText("N/A");
        if (qualityMaxMin.shape_mean == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(18, 3)->setText("N/A");
        if (qualityMaxMin.equiangle_skew_mean == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(19, 3)->setText("N/A");
        if (qualityMaxMin.shape_and_size_mean == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(20, 3)->setText("N/A");
        if (qualityMaxMin.skew_mean == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(21, 3)->setText("N/A");
        if (qualityMaxMin.taper_mean == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(22, 3)->setText("N/A");
        if (qualityMaxMin.warpage_mean == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(23, 3)->setText("N/A");
        if (qualityMaxMin.stretch_mean == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(24, 3)->setText("N/A");
        if (qualityMaxMin.shear_mean == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(25, 3)->setText("N/A");
        if (qualityMaxMin.shear_and_size_mean == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(26, 3)->setText("N/A");
        if (qualityMaxMin.collapse_ratio_mean == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(27, 3)->setText("N/A");
        if (qualityMaxMin.equivolume_skew_mean == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(28, 3)->setText("N/A");
        if (qualityMaxMin.squish_index_mean == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(29, 3)->setText("N/A");
        if (qualityMaxMin.max_stretch_mean == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(30, 3)->setText("N/A");
        if (qualityMaxMin.mean_ratio_mean == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(31, 3)->setText("N/A");
        if (qualityMaxMin.condition_mean == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(32, 3)->setText("N/A");
        if (qualityMaxMin.scaled_jacobian_mean == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(33, 3)->setText("N/A");
        if (qualityMaxMin.distortion_mean == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(34, 3)->setText("N/A");
        if (qualityMaxMin.jacobian_mean == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(35, 3)->setText("N/A");
        if (qualityMaxMin.oddy_mean == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(36, 3)->setText("N/A");
        if (qualityMaxMin.nodal_jacobian_ratio_mean == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(37, 3)->setText("N/A");
        if (qualityMaxMin.enhanced_orth_mean == std::numeric_limits<double>::max())
            mTableDataWindow::getInstance()->getTableWidget()->item(38, 3)->setText("N/A");
        //未勾选的指标应隐藏
        for (int i = 0; i < sizeMap.size(); i++)
        {
            QWidget* widget = sizeTable->cellWidget(i, 0);
            QCheckBox* checkBox = qobject_cast<QCheckBox*>(widget);

            if (checkBox) {
                // 成功获取到 checkbox
                bool isChecked = checkBox->isChecked();
                bool isEnabled = checkBox->isEnabled();
                QString text = checkBox->text();
                auto it = std::find(Metric.begin(), Metric.end(), text);
                int index = it - Metric.begin();
                if (!isChecked || !isEnabled)
                {        
                    mTableDataWindow::getInstance()->getTableWidget()->hideRow(index);
                }
                else
                {
                    mTableDataWindow::getInstance()->getTableWidget()->showRow(index);
                }
            }
        }
        for (int i = 0; i < shapeMap.size(); i++)
        {
            QWidget* widget = shapeTable->cellWidget(i, 0);
            QCheckBox* checkBox = qobject_cast<QCheckBox*>(widget);

            if (checkBox) {
                // 成功获取到 checkbox
                bool isChecked = checkBox->isChecked();
                bool isEnabled = checkBox->isEnabled();
                QString text = checkBox->text();
                auto it = std::find(Metric.begin(), Metric.end(), text);
                int index = it - Metric.begin();
                if (!isChecked || !isEnabled)
                {
                    mTableDataWindow::getInstance()->getTableWidget()->hideRow(index);
                }
                else
                {
                    mTableDataWindow::getInstance()->getTableWidget()->showRow(index);
                }
            }
        }
        for (int i = 0; i < paramMap.size(); i++)
        {
            QWidget* widget = paramTable->cellWidget(i, 0);
            QCheckBox* checkBox = qobject_cast<QCheckBox*>(widget);

            if (checkBox) {
                // 成功获取到 checkbox
                bool isChecked = checkBox->isChecked();
                bool isEnabled = checkBox->isEnabled();
                QString text = checkBox->text();
                auto it = std::find(Metric.begin(), Metric.end(), text);
                int index = it - Metric.begin();
                if (!isChecked || !isEnabled)
                {
                    mTableDataWindow::getInstance()->getTableWidget()->hideRow(index);
                }
                else
                {
                    mTableDataWindow::getInstance()->getTableWidget()->showRow(index);
                }
            }
        }
        //按可见顺序重排垂直表头序号：可见行连续编号，隐藏行留空，避免显示的序号跳号
        QTableWidget* resultTable = mTableDataWindow::getInstance()->getTableWidget();
        QStringList rowLabels;
        int visibleSeq = 0;
        for (int row = 0; row < resultTable->rowCount(); ++row)
        {
            rowLabels << (resultTable->isRowHidden(row) ? QString() : QString::number(++visibleSeq));
        }
        resultTable->setVerticalHeaderLabels(rowLabels);
    }
    void mQualityPanel::setWedge(MechMainWindow::QualityMaxMin& qualityMaxMin)
    {
        // 三棱柱
        if (!ctx->results.wedge.volume.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.wedge.volume.begin(), ctx->results.wedge.volume.end());
            qualityMaxMin.wedgeMaxMin.volume = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.wedge.volume.begin(), ctx->results.wedge.volume.end(), 0.0) / ctx->results.wedge.volume.size();
            qualityMaxMin.wedgeMaxMin.volume_mean = mean;
            qualityMaxMin.wedgeMaxMin.num = ctx->results.wedge.volume.size();
        }

        if (!ctx->results.wedge.edge_ratio.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.wedge.edge_ratio.begin(), ctx->results.wedge.edge_ratio.end());
            qualityMaxMin.wedgeMaxMin.edge_ratio = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.wedge.edge_ratio.begin(), ctx->results.wedge.edge_ratio.end(), 0.0) / ctx->results.wedge.edge_ratio.size();
            qualityMaxMin.wedgeMaxMin.edge_ratio_mean = mean;
            qualityMaxMin.wedgeMaxMin.num = ctx->results.wedge.edge_ratio.size();
        }

        if (!ctx->results.wedge.max_aspect_frobenius.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.wedge.max_aspect_frobenius.begin(), ctx->results.wedge.max_aspect_frobenius.end());
            qualityMaxMin.wedgeMaxMin.max_aspect_frobenius = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.wedge.max_aspect_frobenius.begin(), ctx->results.wedge.max_aspect_frobenius.end(), 0.0) / ctx->results.wedge.max_aspect_frobenius.size();
            qualityMaxMin.wedgeMaxMin.max_aspect_frobenius_mean = mean;
            qualityMaxMin.wedgeMaxMin.num = ctx->results.wedge.max_aspect_frobenius.size();
        }

        if (!ctx->results.wedge.mean_aspect_frobenius.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.wedge.mean_aspect_frobenius.begin(), ctx->results.wedge.mean_aspect_frobenius.end());
            qualityMaxMin.wedgeMaxMin.mean_aspect_frobenius = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.wedge.mean_aspect_frobenius.begin(), ctx->results.wedge.mean_aspect_frobenius.end(), 0.0) / ctx->results.wedge.mean_aspect_frobenius.size();
            qualityMaxMin.wedgeMaxMin.mean_aspect_frobenius_mean = mean;
            qualityMaxMin.wedgeMaxMin.num = ctx->results.wedge.mean_aspect_frobenius.size();
        }

        if (!ctx->results.wedge.jacobian.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.wedge.jacobian.begin(), ctx->results.wedge.jacobian.end());
            qualityMaxMin.wedgeMaxMin.jacobian = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.wedge.jacobian.begin(), ctx->results.wedge.jacobian.end(), 0.0) / ctx->results.wedge.jacobian.size();
            qualityMaxMin.wedgeMaxMin.jacobian_mean = mean;
            qualityMaxMin.wedgeMaxMin.num = ctx->results.wedge.jacobian.size();
        }

        if (!ctx->results.wedge.distortion.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.wedge.distortion.begin(), ctx->results.wedge.distortion.end());
            qualityMaxMin.wedgeMaxMin.distortion = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.wedge.distortion.begin(), ctx->results.wedge.distortion.end(), 0.0) / ctx->results.wedge.distortion.size();
            qualityMaxMin.wedgeMaxMin.distortion_mean = mean;
            qualityMaxMin.wedgeMaxMin.num = ctx->results.wedge.distortion.size();
        }

        if (!ctx->results.wedge.max_stretch.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.wedge.max_stretch.begin(), ctx->results.wedge.max_stretch.end());
            qualityMaxMin.wedgeMaxMin.max_stretch = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.wedge.max_stretch.begin(), ctx->results.wedge.max_stretch.end(), 0.0) / ctx->results.wedge.max_stretch.size();
            qualityMaxMin.wedgeMaxMin.max_stretch_mean = mean;
            qualityMaxMin.wedgeMaxMin.num = ctx->results.wedge.max_stretch.size();
        }

        if (!ctx->results.wedge.scaled_jacobian.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.wedge.scaled_jacobian.begin(), ctx->results.wedge.scaled_jacobian.end());
            qualityMaxMin.wedgeMaxMin.scaled_jacobian = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.wedge.scaled_jacobian.begin(), ctx->results.wedge.scaled_jacobian.end(), 0.0) / ctx->results.wedge.scaled_jacobian.size();
            qualityMaxMin.wedgeMaxMin.scaled_jacobian_mean = mean;
            qualityMaxMin.wedgeMaxMin.num = ctx->results.wedge.scaled_jacobian.size();
        }

        if (!ctx->results.wedge.shape.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.wedge.shape.begin(), ctx->results.wedge.shape.end());
            qualityMaxMin.wedgeMaxMin.shape = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.wedge.shape.begin(), ctx->results.wedge.shape.end(), 0.0) / ctx->results.wedge.shape.size();
            qualityMaxMin.wedgeMaxMin.shape_mean = mean;
            qualityMaxMin.wedgeMaxMin.num = ctx->results.wedge.shape.size();
        }

        if (!ctx->results.wedge.condition.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.wedge.condition.begin(), ctx->results.wedge.condition.end());
            qualityMaxMin.wedgeMaxMin.condition = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.wedge.condition.begin(), ctx->results.wedge.condition.end(), 0.0) / ctx->results.wedge.condition.size();
            qualityMaxMin.wedgeMaxMin.condition_mean = mean;
            qualityMaxMin.wedgeMaxMin.num = ctx->results.wedge.condition.size();
        }

        if (!ctx->results.wedge.equiangle_skew.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.wedge.equiangle_skew.begin(), ctx->results.wedge.equiangle_skew.end());
            qualityMaxMin.wedgeMaxMin.equiangle_skew = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.wedge.equiangle_skew.begin(), ctx->results.wedge.equiangle_skew.end(), 0.0) / ctx->results.wedge.equiangle_skew.size();
            qualityMaxMin.wedgeMaxMin.equiangle_skew_mean = mean;
            qualityMaxMin.wedgeMaxMin.num = ctx->results.wedge.equiangle_skew.size();
        }

        if (!ctx->results.wedge.orthogonality.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.wedge.orthogonality.begin(), ctx->results.wedge.orthogonality.end());
            qualityMaxMin.wedgeMaxMin.enhanced_orth = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.wedge.orthogonality.begin(), ctx->results.wedge.orthogonality.end(), 0.0) / ctx->results.wedge.orthogonality.size();
            qualityMaxMin.wedgeMaxMin.enhanced_orth_mean = mean;
            qualityMaxMin.wedgeMaxMin.num = ctx->results.wedge.orthogonality.size();
        }

    }
    void mQualityPanel::setHex(MechMainWindow::QualityMaxMin& qualityMaxMin)
    {
        // 六面体
        if (!ctx->results.hex.edge_ratio.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.hex.edge_ratio.begin(), ctx->results.hex.edge_ratio.end());
            qualityMaxMin.hexMaxMin.edge_ratio = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.hex.edge_ratio.begin(), ctx->results.hex.edge_ratio.end(), 0.0) / ctx->results.hex.edge_ratio.size();
            qualityMaxMin.hexMaxMin.edge_ratio_mean = mean;
            qualityMaxMin.hexMaxMin.num = ctx->results.hex.edge_ratio.size();
        }

        if (!ctx->results.hex.max_edge_ratio.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.hex.max_edge_ratio.begin(), ctx->results.hex.max_edge_ratio.end());
            qualityMaxMin.hexMaxMin.max_edge_ratio = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.hex.max_edge_ratio.begin(), ctx->results.hex.max_edge_ratio.end(), 0.0) / ctx->results.hex.max_edge_ratio.size();
            qualityMaxMin.hexMaxMin.max_edge_ratio_mean = mean;
            qualityMaxMin.hexMaxMin.num = ctx->results.hex.max_edge_ratio.size();
        }

        if (!ctx->results.hex.skew.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.hex.skew.begin(), ctx->results.hex.skew.end());
            qualityMaxMin.hexMaxMin.skew = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.hex.skew.begin(), ctx->results.hex.skew.end(), 0.0) / ctx->results.hex.skew.size();
            qualityMaxMin.hexMaxMin.skew_mean = mean;
            qualityMaxMin.hexMaxMin.num = ctx->results.hex.skew.size();
        }

        if (!ctx->results.hex.taper.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.hex.taper.begin(), ctx->results.hex.taper.end());
            qualityMaxMin.hexMaxMin.taper = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.hex.taper.begin(), ctx->results.hex.taper.end(), 0.0) / ctx->results.hex.taper.size();
            qualityMaxMin.hexMaxMin.taper_mean = mean;
            qualityMaxMin.hexMaxMin.num = ctx->results.hex.taper.size();
        }

        if (!ctx->results.hex.volume.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.hex.volume.begin(), ctx->results.hex.volume.end());
            qualityMaxMin.hexMaxMin.volume = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.hex.volume.begin(), ctx->results.hex.volume.end(), 0.0) / ctx->results.hex.volume.size();
            qualityMaxMin.hexMaxMin.volume_mean = mean;
            qualityMaxMin.hexMaxMin.num = ctx->results.hex.volume.size();
        }

        if (!ctx->results.hex.stretch.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.hex.stretch.begin(), ctx->results.hex.stretch.end());
            qualityMaxMin.hexMaxMin.stretch = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.hex.stretch.begin(), ctx->results.hex.stretch.end(), 0.0) / ctx->results.hex.stretch.size();
            qualityMaxMin.hexMaxMin.stretch_mean = mean;
            qualityMaxMin.hexMaxMin.num = ctx->results.hex.stretch.size();
        }

        if (!ctx->results.hex.diagonal.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.hex.diagonal.begin(), ctx->results.hex.diagonal.end());
            qualityMaxMin.hexMaxMin.diagonal = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.hex.diagonal.begin(), ctx->results.hex.diagonal.end(), 0.0) / ctx->results.hex.diagonal.size();
            qualityMaxMin.hexMaxMin.diagonal_mean = mean;
            qualityMaxMin.hexMaxMin.num = ctx->results.hex.diagonal.size();
        }

        if (!ctx->results.hex.dimension.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.hex.dimension.begin(), ctx->results.hex.dimension.end());
            qualityMaxMin.hexMaxMin.dimension = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.hex.dimension.begin(), ctx->results.hex.dimension.end(), 0.0) / ctx->results.hex.dimension.size();
            qualityMaxMin.hexMaxMin.dimension_mean = mean;
            qualityMaxMin.hexMaxMin.num = ctx->results.hex.dimension.size();
        }

        if (!ctx->results.hex.oddy.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.hex.oddy.begin(), ctx->results.hex.oddy.end());
            qualityMaxMin.hexMaxMin.oddy = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.hex.oddy.begin(), ctx->results.hex.oddy.end(), 0.0) / ctx->results.hex.oddy.size();
            qualityMaxMin.hexMaxMin.oddy_mean = mean;
            qualityMaxMin.hexMaxMin.num = ctx->results.hex.oddy.size();
        }

        if (!ctx->results.hex.med_aspect_frobenius.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.hex.med_aspect_frobenius.begin(), ctx->results.hex.med_aspect_frobenius.end());
            qualityMaxMin.hexMaxMin.med_aspect_frobenius = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.hex.med_aspect_frobenius.begin(), ctx->results.hex.med_aspect_frobenius.end(), 0.0) / ctx->results.hex.med_aspect_frobenius.size();
            qualityMaxMin.hexMaxMin.med_aspect_frobenius_mean = mean;
            qualityMaxMin.hexMaxMin.num = ctx->results.hex.med_aspect_frobenius.size();
        }

        if (!ctx->results.hex.max_aspect_frobenius.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.hex.max_aspect_frobenius.begin(), ctx->results.hex.max_aspect_frobenius.end());
            qualityMaxMin.hexMaxMin.max_aspect_frobenius = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.hex.max_aspect_frobenius.begin(), ctx->results.hex.max_aspect_frobenius.end(), 0.0) / ctx->results.hex.max_aspect_frobenius.size();
            qualityMaxMin.hexMaxMin.max_aspect_frobenius_mean = mean;
            qualityMaxMin.hexMaxMin.num = ctx->results.hex.max_aspect_frobenius.size();
        }

        if (!ctx->results.hex.condition.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.hex.condition.begin(), ctx->results.hex.condition.end());
            qualityMaxMin.hexMaxMin.condition = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.hex.condition.begin(), ctx->results.hex.condition.end(), 0.0) / ctx->results.hex.condition.size();
            qualityMaxMin.hexMaxMin.condition_mean = mean;
            qualityMaxMin.hexMaxMin.num = ctx->results.hex.condition.size();
        }

        if (!ctx->results.hex.jacobian.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.hex.jacobian.begin(), ctx->results.hex.jacobian.end());
            qualityMaxMin.hexMaxMin.jacobian = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.hex.jacobian.begin(), ctx->results.hex.jacobian.end(), 0.0) / ctx->results.hex.jacobian.size();
            qualityMaxMin.hexMaxMin.jacobian_mean = mean;
            qualityMaxMin.hexMaxMin.num = ctx->results.hex.jacobian.size();
        }

        if (!ctx->results.hex.scaled_jacobian.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.hex.scaled_jacobian.begin(), ctx->results.hex.scaled_jacobian.end());
            qualityMaxMin.hexMaxMin.scaled_jacobian = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.hex.scaled_jacobian.begin(), ctx->results.hex.scaled_jacobian.end(), 0.0) / ctx->results.hex.scaled_jacobian.size();
            qualityMaxMin.hexMaxMin.scaled_jacobian_mean = mean;
            qualityMaxMin.hexMaxMin.num = ctx->results.hex.scaled_jacobian.size();
        }

        if (!ctx->results.hex.shear.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.hex.shear.begin(), ctx->results.hex.shear.end());
            qualityMaxMin.hexMaxMin.shear = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.hex.shear.begin(), ctx->results.hex.shear.end(), 0.0) / ctx->results.hex.shear.size();
            qualityMaxMin.hexMaxMin.shear_mean = mean;
            qualityMaxMin.hexMaxMin.num = ctx->results.hex.shear.size();
        }

        if (!ctx->results.hex.shape.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.hex.shape.begin(), ctx->results.hex.shape.end());
            qualityMaxMin.hexMaxMin.shape = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.hex.shape.begin(), ctx->results.hex.shape.end(), 0.0) / ctx->results.hex.shape.size();
            qualityMaxMin.hexMaxMin.shape_mean = mean;
            qualityMaxMin.hexMaxMin.num = ctx->results.hex.shape.size();
        }

        if (!ctx->results.hex.distortion.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.hex.distortion.begin(), ctx->results.hex.distortion.end());
            qualityMaxMin.hexMaxMin.distortion = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.hex.distortion.begin(), ctx->results.hex.distortion.end(), 0.0) / ctx->results.hex.distortion.size();
            qualityMaxMin.hexMaxMin.distortion_mean = mean;
            qualityMaxMin.hexMaxMin.num = ctx->results.hex.distortion.size();
        }

        if (!ctx->results.hex.equiangle_skew.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.hex.equiangle_skew.begin(), ctx->results.hex.equiangle_skew.end());
            qualityMaxMin.hexMaxMin.equiangle_skew = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.hex.equiangle_skew.begin(), ctx->results.hex.equiangle_skew.end(), 0.0) / ctx->results.hex.equiangle_skew.size();
            qualityMaxMin.hexMaxMin.equiangle_skew_mean = mean;
            qualityMaxMin.hexMaxMin.num = ctx->results.hex.equiangle_skew.size();
        }

        if (!ctx->results.hex.nodal_jacobian_ratio.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.hex.nodal_jacobian_ratio.begin(), ctx->results.hex.nodal_jacobian_ratio.end());
            qualityMaxMin.hexMaxMin.nodal_jacobian_ratio = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.hex.nodal_jacobian_ratio.begin(), ctx->results.hex.nodal_jacobian_ratio.end(), 0.0) / ctx->results.hex.nodal_jacobian_ratio.size();
            qualityMaxMin.hexMaxMin.nodal_jacobian_ratio_mean = mean;
            qualityMaxMin.hexMaxMin.num = ctx->results.hex.nodal_jacobian_ratio.size();
        }

        if (!ctx->results.hex.relative_size_squared.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.hex.relative_size_squared.begin(), ctx->results.hex.relative_size_squared.end());
            qualityMaxMin.hexMaxMin.relative_size_squared = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.hex.relative_size_squared.begin(), ctx->results.hex.relative_size_squared.end(), 0.0) / ctx->results.hex.relative_size_squared.size();
            qualityMaxMin.hexMaxMin.relative_size_squared_mean = mean;
            qualityMaxMin.hexMaxMin.num = ctx->results.hex.relative_size_squared.size();
        }

        if (!ctx->results.hex.shape_and_size.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.hex.shape_and_size.begin(), ctx->results.hex.shape_and_size.end());
            qualityMaxMin.hexMaxMin.shape_and_size = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.hex.shape_and_size.begin(), ctx->results.hex.shape_and_size.end(), 0.0) / ctx->results.hex.shape_and_size.size();
            qualityMaxMin.hexMaxMin.shape_and_size_mean = mean;
            qualityMaxMin.hexMaxMin.num = ctx->results.hex.shape_and_size.size();
        }

        if (!ctx->results.hex.shear_and_size.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.hex.shear_and_size.begin(), ctx->results.hex.shear_and_size.end());
            qualityMaxMin.hexMaxMin.shear_and_size = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.hex.shear_and_size.begin(), ctx->results.hex.shear_and_size.end(), 0.0) / ctx->results.hex.shear_and_size.size();
            qualityMaxMin.hexMaxMin.shear_and_size_mean = mean;
            qualityMaxMin.hexMaxMin.num = ctx->results.hex.shear_and_size.size();
        }

        if (!ctx->results.hex.orthogonality.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.hex.orthogonality.begin(), ctx->results.hex.orthogonality.end());
            qualityMaxMin.hexMaxMin.enhanced_orth = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.hex.orthogonality.begin(), ctx->results.hex.orthogonality.end(), 0.0) / ctx->results.hex.orthogonality.size();
            qualityMaxMin.hexMaxMin.enhanced_orth_mean = mean;
            qualityMaxMin.hexMaxMin.num = ctx->results.hex.orthogonality.size();
        }
    }
    void mQualityPanel::setTet(MechMainWindow::QualityMaxMin& qualityMaxMin)
    {
        // 四面体
        if (!ctx->results.tet.edge_ratio.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.tet.edge_ratio.begin(), ctx->results.tet.edge_ratio.end());
            qualityMaxMin.tetMaxMin.edge_ratio = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.tet.edge_ratio.begin(), ctx->results.tet.edge_ratio.end(), 0.0) / ctx->results.tet.edge_ratio.size();
            qualityMaxMin.tetMaxMin.edge_ratio_mean = mean;
            qualityMaxMin.tetMaxMin.num = ctx->results.tet.edge_ratio.size();
        }

        if (!ctx->results.tet.radius_ratio.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.tet.radius_ratio.begin(), ctx->results.tet.radius_ratio.end());
            qualityMaxMin.tetMaxMin.radius_ratio = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.tet.radius_ratio.begin(), ctx->results.tet.radius_ratio.end(), 0.0) / ctx->results.tet.radius_ratio.size();
            qualityMaxMin.tetMaxMin.radius_ratio_mean = mean;
            qualityMaxMin.tetMaxMin.num = ctx->results.tet.radius_ratio.size();
        }

        if (!ctx->results.tet.aspect_ratio.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.tet.aspect_ratio.begin(), ctx->results.tet.aspect_ratio.end());
            qualityMaxMin.tetMaxMin.aspect_ratio = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.tet.aspect_ratio.begin(), ctx->results.tet.aspect_ratio.end(), 0.0) / ctx->results.tet.aspect_ratio.size();
            qualityMaxMin.tetMaxMin.aspect_ratio_mean = mean;
            qualityMaxMin.tetMaxMin.num = ctx->results.tet.aspect_ratio.size();
        }

        if (!ctx->results.tet.aspect_gamma.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.tet.aspect_gamma.begin(), ctx->results.tet.aspect_gamma.end());
            qualityMaxMin.tetMaxMin.aspect_gamma = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.tet.aspect_gamma.begin(), ctx->results.tet.aspect_gamma.end(), 0.0) / ctx->results.tet.aspect_gamma.size();
            qualityMaxMin.tetMaxMin.aspect_gamma_mean = mean;
            qualityMaxMin.tetMaxMin.num = ctx->results.tet.aspect_gamma.size();
        }

        if (!ctx->results.tet.aspect_frobenius.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.tet.aspect_frobenius.begin(), ctx->results.tet.aspect_frobenius.end());
            qualityMaxMin.tetMaxMin.aspect_frobenius = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.tet.aspect_frobenius.begin(), ctx->results.tet.aspect_frobenius.end(), 0.0) / ctx->results.tet.aspect_frobenius.size();
            qualityMaxMin.tetMaxMin.aspect_frobenius_mean = mean;
            qualityMaxMin.tetMaxMin.num = ctx->results.tet.aspect_frobenius.size();
        }

        if (!ctx->results.tet.minimum_angle.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.tet.minimum_angle.begin(), ctx->results.tet.minimum_angle.end());
            qualityMaxMin.tetMaxMin.minimum_angle = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.tet.minimum_angle.begin(), ctx->results.tet.minimum_angle.end(), 0.0) / ctx->results.tet.minimum_angle.size();
            qualityMaxMin.tetMaxMin.minimum_angle_mean = mean;
            qualityMaxMin.tetMaxMin.num = ctx->results.tet.minimum_angle.size();
        }

        if (!ctx->results.tet.collapse_ratio.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.tet.collapse_ratio.begin(), ctx->results.tet.collapse_ratio.end());
            qualityMaxMin.tetMaxMin.collapse_ratio = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.tet.collapse_ratio.begin(), ctx->results.tet.collapse_ratio.end(), 0.0) / ctx->results.tet.collapse_ratio.size();
            qualityMaxMin.tetMaxMin.collapse_ratio_mean = mean;
            qualityMaxMin.tetMaxMin.num = ctx->results.tet.collapse_ratio.size();
        }

        if (!ctx->results.tet.volume.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.tet.volume.begin(), ctx->results.tet.volume.end());
            qualityMaxMin.tetMaxMin.volume = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.tet.volume.begin(), ctx->results.tet.volume.end(), 0.0) / ctx->results.tet.volume.size();
            qualityMaxMin.tetMaxMin.volume_mean = mean;
            qualityMaxMin.tetMaxMin.num = ctx->results.tet.volume.size();
        }

        if (!ctx->results.tet.condition.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.tet.condition.begin(), ctx->results.tet.condition.end());
            qualityMaxMin.tetMaxMin.condition = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.tet.condition.begin(), ctx->results.tet.condition.end(), 0.0) / ctx->results.tet.condition.size();
            qualityMaxMin.tetMaxMin.condition_mean = mean;
            qualityMaxMin.tetMaxMin.num = ctx->results.tet.condition.size();
        }

        if (!ctx->results.tet.jacobian.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.tet.jacobian.begin(), ctx->results.tet.jacobian.end());
            qualityMaxMin.tetMaxMin.jacobian = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.tet.jacobian.begin(), ctx->results.tet.jacobian.end(), 0.0) / ctx->results.tet.jacobian.size();
            qualityMaxMin.tetMaxMin.jacobian_mean = mean;
            qualityMaxMin.tetMaxMin.num = ctx->results.tet.jacobian.size();
        }

        if (!ctx->results.tet.scaled_jacobian.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.tet.scaled_jacobian.begin(), ctx->results.tet.scaled_jacobian.end());
            qualityMaxMin.tetMaxMin.scaled_jacobian = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.tet.scaled_jacobian.begin(), ctx->results.tet.scaled_jacobian.end(), 0.0) / ctx->results.tet.scaled_jacobian.size();
            qualityMaxMin.tetMaxMin.scaled_jacobian_mean = mean;
            qualityMaxMin.tetMaxMin.num = ctx->results.tet.scaled_jacobian.size();
        }

        if (!ctx->results.tet.mean_ratio.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.tet.mean_ratio.begin(), ctx->results.tet.mean_ratio.end());
            qualityMaxMin.tetMaxMin.mean_ratio = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.tet.mean_ratio.begin(), ctx->results.tet.mean_ratio.end(), 0.0) / ctx->results.tet.mean_ratio.size();
            qualityMaxMin.tetMaxMin.mean_ratio_mean = mean;
            qualityMaxMin.tetMaxMin.num = ctx->results.tet.mean_ratio.size();
        }

        if (!ctx->results.tet.normalized_inradius.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.tet.normalized_inradius.begin(), ctx->results.tet.normalized_inradius.end());
            qualityMaxMin.tetMaxMin.normalized_inradius = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.tet.normalized_inradius.begin(), ctx->results.tet.normalized_inradius.end(), 0.0) / ctx->results.tet.normalized_inradius.size();
            qualityMaxMin.tetMaxMin.normalized_inradius_mean = mean;
            qualityMaxMin.tetMaxMin.num = ctx->results.tet.normalized_inradius.size();
        }

        if (!ctx->results.tet.shape.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.tet.shape.begin(), ctx->results.tet.shape.end());
            qualityMaxMin.tetMaxMin.shape = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.tet.shape.begin(), ctx->results.tet.shape.end(), 0.0) / ctx->results.tet.shape.size();
            qualityMaxMin.tetMaxMin.shape_mean = mean;
            qualityMaxMin.tetMaxMin.num = ctx->results.tet.shape.size();
        }

        if (!ctx->results.tet.distortion.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.tet.distortion.begin(), ctx->results.tet.distortion.end());
            qualityMaxMin.tetMaxMin.distortion = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.tet.distortion.begin(), ctx->results.tet.distortion.end(), 0.0) / ctx->results.tet.distortion.size();
            qualityMaxMin.tetMaxMin.distortion_mean = mean;
            qualityMaxMin.tetMaxMin.num = ctx->results.tet.distortion.size();
        }

        if (!ctx->results.tet.equivolume_skew.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.tet.equivolume_skew.begin(), ctx->results.tet.equivolume_skew.end());
            qualityMaxMin.tetMaxMin.equivolume_skew = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.tet.equivolume_skew.begin(), ctx->results.tet.equivolume_skew.end(), 0.0) / ctx->results.tet.equivolume_skew.size();
            qualityMaxMin.tetMaxMin.equivolume_skew_mean = mean;
            qualityMaxMin.tetMaxMin.num = ctx->results.tet.equivolume_skew.size();
        }

        if (!ctx->results.tet.squish_index.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.tet.squish_index.begin(), ctx->results.tet.squish_index.end());
            qualityMaxMin.tetMaxMin.squish_index = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.tet.squish_index.begin(), ctx->results.tet.squish_index.end(), 0.0) / ctx->results.tet.squish_index.size();
            qualityMaxMin.tetMaxMin.squish_index_mean = mean;
            qualityMaxMin.tetMaxMin.num = ctx->results.tet.squish_index.size();
        }

        if (!ctx->results.tet.equiangle_skew.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.tet.equiangle_skew.begin(), ctx->results.tet.equiangle_skew.end());
            qualityMaxMin.tetMaxMin.equiangle_skew = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.tet.equiangle_skew.begin(), ctx->results.tet.equiangle_skew.end(), 0.0) / ctx->results.tet.equiangle_skew.size();
            qualityMaxMin.tetMaxMin.equiangle_skew_mean = mean;
            qualityMaxMin.tetMaxMin.num = ctx->results.tet.equiangle_skew.size();
        }

        if (!ctx->results.tet.relative_size_squared.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.tet.relative_size_squared.begin(), ctx->results.tet.relative_size_squared.end());
            qualityMaxMin.tetMaxMin.relative_size_squared = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.tet.relative_size_squared.begin(), ctx->results.tet.relative_size_squared.end(), 0.0) / ctx->results.tet.relative_size_squared.size();
            qualityMaxMin.tetMaxMin.relative_size_squared_mean = mean;
            qualityMaxMin.tetMaxMin.num = ctx->results.tet.relative_size_squared.size();
        }

        if (!ctx->results.tet.shape_and_size.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.tet.shape_and_size.begin(), ctx->results.tet.shape_and_size.end());
            qualityMaxMin.tetMaxMin.shape_and_size = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.tet.shape_and_size.begin(), ctx->results.tet.shape_and_size.end(), 0.0) / ctx->results.tet.shape_and_size.size();
            qualityMaxMin.tetMaxMin.shape_and_size_mean = mean;
            qualityMaxMin.tetMaxMin.num = ctx->results.tet.shape_and_size.size();
        }

        if (!ctx->results.tet.orthogonality.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.tet.orthogonality.begin(), ctx->results.tet.orthogonality.end());
            qualityMaxMin.tetMaxMin.enhanced_orth = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.tet.orthogonality.begin(), ctx->results.tet.orthogonality.end(), 0.0) / ctx->results.tet.orthogonality.size();
            qualityMaxMin.tetMaxMin.enhanced_orth_mean = mean;
            qualityMaxMin.tetMaxMin.num = ctx->results.tet.orthogonality.size();
        }
    }
    void mQualityPanel::setQuad(MechMainWindow::QualityMaxMin& qualityMaxMin)
    {
        // 四边形
        if (!ctx->results.quad.edge_ratio.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.quad.edge_ratio.begin(), ctx->results.quad.edge_ratio.end());
            qualityMaxMin.quadMaxMin.edge_ratio = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.quad.edge_ratio.begin(), ctx->results.quad.edge_ratio.end(), 0.0) / ctx->results.quad.edge_ratio.size();
            qualityMaxMin.quadMaxMin.edge_ratio_mean = mean;
            qualityMaxMin.quadMaxMin.num = ctx->results.quad.edge_ratio.size();
        }

        if (!ctx->results.quad.max_edge_ratio.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.quad.max_edge_ratio.begin(), ctx->results.quad.max_edge_ratio.end());
            qualityMaxMin.quadMaxMin.max_edge_ratio = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.quad.max_edge_ratio.begin(), ctx->results.quad.max_edge_ratio.end(), 0.0) / ctx->results.quad.max_edge_ratio.size();
            qualityMaxMin.quadMaxMin.max_edge_ratio_mean = mean;
            qualityMaxMin.quadMaxMin.num = ctx->results.quad.max_edge_ratio.size();
        }

        if (!ctx->results.quad.aspect_ratio.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.quad.aspect_ratio.begin(), ctx->results.quad.aspect_ratio.end());
            qualityMaxMin.quadMaxMin.aspect_ratio = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.quad.aspect_ratio.begin(), ctx->results.quad.aspect_ratio.end(), 0.0) / ctx->results.quad.aspect_ratio.size();
            qualityMaxMin.quadMaxMin.aspect_ratio_mean = mean;
            qualityMaxMin.quadMaxMin.num = ctx->results.quad.aspect_ratio.size();
        }

        if (!ctx->results.quad.radius_ratio.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.quad.radius_ratio.begin(), ctx->results.quad.radius_ratio.end());
            qualityMaxMin.quadMaxMin.radius_ratio = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.quad.radius_ratio.begin(), ctx->results.quad.radius_ratio.end(), 0.0) / ctx->results.quad.radius_ratio.size();
            qualityMaxMin.quadMaxMin.radius_ratio_mean = mean;
            qualityMaxMin.quadMaxMin.num = ctx->results.quad.radius_ratio.size();
        }

        if (!ctx->results.quad.med_aspect_frobenius.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.quad.med_aspect_frobenius.begin(), ctx->results.quad.med_aspect_frobenius.end());
            qualityMaxMin.quadMaxMin.med_aspect_frobenius = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.quad.med_aspect_frobenius.begin(), ctx->results.quad.med_aspect_frobenius.end(), 0.0) / ctx->results.quad.med_aspect_frobenius.size();
            qualityMaxMin.quadMaxMin.med_aspect_frobenius_mean = mean;
            qualityMaxMin.quadMaxMin.num = ctx->results.quad.med_aspect_frobenius.size();
        }

        if (!ctx->results.quad.max_aspect_frobenius.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.quad.max_aspect_frobenius.begin(), ctx->results.quad.max_aspect_frobenius.end());
            qualityMaxMin.quadMaxMin.max_aspect_frobenius = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.quad.max_aspect_frobenius.begin(), ctx->results.quad.max_aspect_frobenius.end(), 0.0) / ctx->results.quad.max_aspect_frobenius.size();
            qualityMaxMin.quadMaxMin.max_aspect_frobenius_mean = mean;
            qualityMaxMin.quadMaxMin.num = ctx->results.quad.max_aspect_frobenius.size();
        }

        if (!ctx->results.quad.skew.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.quad.skew.begin(), ctx->results.quad.skew.end());
            qualityMaxMin.quadMaxMin.skew = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.quad.skew.begin(), ctx->results.quad.skew.end(), 0.0) / ctx->results.quad.skew.size();
            qualityMaxMin.quadMaxMin.skew_mean = mean;
            qualityMaxMin.quadMaxMin.num = ctx->results.quad.skew.size();
        }

        if (!ctx->results.quad.taper.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.quad.taper.begin(), ctx->results.quad.taper.end());
            qualityMaxMin.quadMaxMin.taper = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.quad.taper.begin(), ctx->results.quad.taper.end(), 0.0) / ctx->results.quad.taper.size();
            qualityMaxMin.quadMaxMin.taper_mean = mean;
            qualityMaxMin.quadMaxMin.num = ctx->results.quad.taper.size();
        }

        if (!ctx->results.quad.warpage.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.quad.warpage.begin(), ctx->results.quad.warpage.end());
            qualityMaxMin.quadMaxMin.warpage = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.quad.warpage.begin(), ctx->results.quad.warpage.end(), 0.0) / ctx->results.quad.warpage.size();
            qualityMaxMin.quadMaxMin.warpage_mean = mean;
            qualityMaxMin.quadMaxMin.num = ctx->results.quad.warpage.size();
        }

        if (!ctx->results.quad.area.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.quad.area.begin(), ctx->results.quad.area.end());
            qualityMaxMin.quadMaxMin.area = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.quad.area.begin(), ctx->results.quad.area.end(), 0.0) / ctx->results.quad.area.size();
            qualityMaxMin.quadMaxMin.area_mean = mean;
            qualityMaxMin.quadMaxMin.num = ctx->results.quad.area.size();
        }

        if (!ctx->results.quad.stretch.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.quad.stretch.begin(), ctx->results.quad.stretch.end());
            qualityMaxMin.quadMaxMin.stretch = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.quad.stretch.begin(), ctx->results.quad.stretch.end(), 0.0) / ctx->results.quad.stretch.size();
            qualityMaxMin.quadMaxMin.stretch_mean = mean;
            qualityMaxMin.quadMaxMin.num = ctx->results.quad.stretch.size();
        }

        if (!ctx->results.quad.minimum_angle.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.quad.minimum_angle.begin(), ctx->results.quad.minimum_angle.end());
            qualityMaxMin.quadMaxMin.minimum_angle = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.quad.minimum_angle.begin(), ctx->results.quad.minimum_angle.end(), 0.0) / ctx->results.quad.minimum_angle.size();
            qualityMaxMin.quadMaxMin.minimum_angle_mean = mean;
            qualityMaxMin.quadMaxMin.num = ctx->results.quad.minimum_angle.size();
        }

        if (!ctx->results.quad.maximum_angle.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.quad.maximum_angle.begin(), ctx->results.quad.maximum_angle.end());
            qualityMaxMin.quadMaxMin.maximum_angle = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.quad.maximum_angle.begin(), ctx->results.quad.maximum_angle.end(), 0.0) / ctx->results.quad.maximum_angle.size();
            qualityMaxMin.quadMaxMin.maximum_angle_mean = mean;
            qualityMaxMin.quadMaxMin.num = ctx->results.quad.maximum_angle.size();
        }

        if (!ctx->results.quad.oddy.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.quad.oddy.begin(), ctx->results.quad.oddy.end());
            qualityMaxMin.quadMaxMin.oddy = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.quad.oddy.begin(), ctx->results.quad.oddy.end(), 0.0) / ctx->results.quad.oddy.size();
            qualityMaxMin.quadMaxMin.oddy_mean = mean;
            qualityMaxMin.quadMaxMin.num = ctx->results.quad.oddy.size();
        }

        if (!ctx->results.quad.condition.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.quad.condition.begin(), ctx->results.quad.condition.end());
            qualityMaxMin.quadMaxMin.condition = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.quad.condition.begin(), ctx->results.quad.condition.end(), 0.0) / ctx->results.quad.condition.size();
            qualityMaxMin.quadMaxMin.condition_mean = mean;
            qualityMaxMin.quadMaxMin.num = ctx->results.quad.condition.size();
        }

        if (!ctx->results.quad.jacobian.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.quad.jacobian.begin(), ctx->results.quad.jacobian.end());
            qualityMaxMin.quadMaxMin.jacobian = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.quad.jacobian.begin(), ctx->results.quad.jacobian.end(), 0.0) / ctx->results.quad.jacobian.size();
            qualityMaxMin.quadMaxMin.jacobian_mean = mean;
            qualityMaxMin.quadMaxMin.num = ctx->results.quad.jacobian.size();
        }

        if (!ctx->results.quad.scaled_jacobian.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.quad.scaled_jacobian.begin(), ctx->results.quad.scaled_jacobian.end());
            qualityMaxMin.quadMaxMin.scaled_jacobian = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.quad.scaled_jacobian.begin(), ctx->results.quad.scaled_jacobian.end(), 0.0) / ctx->results.quad.scaled_jacobian.size();
            qualityMaxMin.quadMaxMin.scaled_jacobian_mean = mean;
            qualityMaxMin.quadMaxMin.num = ctx->results.quad.scaled_jacobian.size();
        }

        if (!ctx->results.quad.shear.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.quad.shear.begin(), ctx->results.quad.shear.end());
            qualityMaxMin.quadMaxMin.shear = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.quad.shear.begin(), ctx->results.quad.shear.end(), 0.0) / ctx->results.quad.shear.size();
            qualityMaxMin.quadMaxMin.shear_mean = mean;
            qualityMaxMin.quadMaxMin.num = ctx->results.quad.shear.size();
        }

        if (!ctx->results.quad.shape.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.quad.shape.begin(), ctx->results.quad.shape.end());
            qualityMaxMin.quadMaxMin.shape = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.quad.shape.begin(), ctx->results.quad.shape.end(), 0.0) / ctx->results.quad.shape.size();
            qualityMaxMin.quadMaxMin.shape_mean = mean;
            qualityMaxMin.quadMaxMin.num = ctx->results.quad.shape.size();
        }

        if (!ctx->results.quad.distortion.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.quad.distortion.begin(), ctx->results.quad.distortion.end());
            qualityMaxMin.quadMaxMin.distortion = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.quad.distortion.begin(), ctx->results.quad.distortion.end(), 0.0) / ctx->results.quad.distortion.size();
            qualityMaxMin.quadMaxMin.distortion_mean = mean;
            qualityMaxMin.quadMaxMin.num = ctx->results.quad.distortion.size();
        }

        if (!ctx->results.quad.equiangle_skew.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.quad.equiangle_skew.begin(), ctx->results.quad.equiangle_skew.end());
            qualityMaxMin.quadMaxMin.equiangle_skew = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.quad.equiangle_skew.begin(), ctx->results.quad.equiangle_skew.end(), 0.0) / ctx->results.quad.equiangle_skew.size();
            qualityMaxMin.quadMaxMin.equiangle_skew_mean = mean;
            qualityMaxMin.quadMaxMin.num = ctx->results.quad.equiangle_skew.size();
        }

        if (!ctx->results.quad.relative_size_squared.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.quad.relative_size_squared.begin(), ctx->results.quad.relative_size_squared.end());
            qualityMaxMin.quadMaxMin.relative_size_squared = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.quad.relative_size_squared.begin(), ctx->results.quad.relative_size_squared.end(), 0.0) / ctx->results.quad.relative_size_squared.size();
            qualityMaxMin.quadMaxMin.relative_size_squared_mean = mean;
            qualityMaxMin.quadMaxMin.num = ctx->results.quad.relative_size_squared.size();
        }

        if (!ctx->results.quad.shape_and_size.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.quad.shape_and_size.begin(), ctx->results.quad.shape_and_size.end());
            qualityMaxMin.quadMaxMin.shape_and_size = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.quad.shape_and_size.begin(), ctx->results.quad.shape_and_size.end(), 0.0) / ctx->results.quad.shape_and_size.size();
            qualityMaxMin.quadMaxMin.shape_and_size_mean = mean;
            qualityMaxMin.quadMaxMin.num = ctx->results.quad.shape_and_size.size();
        }

        if (!ctx->results.quad.shear_and_size.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.quad.shear_and_size.begin(), ctx->results.quad.shear_and_size.end());
            qualityMaxMin.quadMaxMin.shear_and_size = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.quad.shear_and_size.begin(), ctx->results.quad.shear_and_size.end(), 0.0) / ctx->results.quad.shear_and_size.size();
            qualityMaxMin.quadMaxMin.shear_and_size_mean = mean;
            qualityMaxMin.quadMaxMin.num = ctx->results.quad.shear_and_size.size();
        }

        if (!ctx->results.quad.orthogonality.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.quad.orthogonality.begin(), ctx->results.quad.orthogonality.end());
            qualityMaxMin.quadMaxMin.enhanced_orth = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.quad.orthogonality.begin(), ctx->results.quad.orthogonality.end(), 0.0) / ctx->results.quad.orthogonality.size();
            qualityMaxMin.quadMaxMin.enhanced_orth_mean = mean;
            qualityMaxMin.quadMaxMin.num = ctx->results.quad.orthogonality.size();
        }
    }
    void mQualityPanel::setTri(MechMainWindow::QualityMaxMin& qualityMaxMin)
    {
        // 三角形
        if (!ctx->results.tri.edge_ratio.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.tri.edge_ratio.begin(), ctx->results.tri.edge_ratio.end());
            qualityMaxMin.triMaxMin.edge_ratio = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.tri.edge_ratio.begin(), ctx->results.tri.edge_ratio.end(), 0.0) / ctx->results.tri.edge_ratio.size();
            qualityMaxMin.triMaxMin.edge_ratio_mean = mean;
            qualityMaxMin.triMaxMin.num = ctx->results.tri.edge_ratio.size();
        }

        if (!ctx->results.tri.aspect_ratio.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.tri.aspect_ratio.begin(), ctx->results.tri.aspect_ratio.end());
            qualityMaxMin.triMaxMin.aspect_ratio = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.tri.aspect_ratio.begin(), ctx->results.tri.aspect_ratio.end(), 0.0) / ctx->results.tri.aspect_ratio.size();
            qualityMaxMin.triMaxMin.aspect_ratio_mean = mean;
            qualityMaxMin.triMaxMin.num = ctx->results.tri.aspect_ratio.size();
        }

        if (!ctx->results.tri.radius_ratio.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.tri.radius_ratio.begin(), ctx->results.tri.radius_ratio.end());
            qualityMaxMin.triMaxMin.radius_ratio = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.tri.radius_ratio.begin(), ctx->results.tri.radius_ratio.end(), 0.0) / ctx->results.tri.radius_ratio.size();
            qualityMaxMin.triMaxMin.radius_ratio_mean = mean;
            qualityMaxMin.triMaxMin.num = ctx->results.tri.radius_ratio.size();
        }

        if (!ctx->results.tri.aspect_frobenius.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.tri.aspect_frobenius.begin(), ctx->results.tri.aspect_frobenius.end());
            qualityMaxMin.triMaxMin.aspect_frobenius = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.tri.aspect_frobenius.begin(), ctx->results.tri.aspect_frobenius.end(), 0.0) / ctx->results.tri.aspect_frobenius.size();
            qualityMaxMin.triMaxMin.aspect_frobenius_mean = mean;
            qualityMaxMin.triMaxMin.num = ctx->results.tri.aspect_frobenius.size();
        }

        if (!ctx->results.tri.area.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.tri.area.begin(), ctx->results.tri.area.end());
            qualityMaxMin.triMaxMin.area = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.tri.area.begin(), ctx->results.tri.area.end(), 0.0) / ctx->results.tri.area.size();
            qualityMaxMin.triMaxMin.area_mean = mean;
            qualityMaxMin.triMaxMin.num = ctx->results.tri.area.size();
        }

        if (!ctx->results.tri.minimum_angle.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.tri.minimum_angle.begin(), ctx->results.tri.minimum_angle.end());
            qualityMaxMin.triMaxMin.minimum_angle = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.tri.minimum_angle.begin(), ctx->results.tri.minimum_angle.end(), 0.0) / ctx->results.tri.minimum_angle.size();
            qualityMaxMin.triMaxMin.minimum_angle_mean = mean;
            qualityMaxMin.triMaxMin.num = ctx->results.tri.minimum_angle.size();
        }

        if (!ctx->results.tri.maximum_angle.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.tri.maximum_angle.begin(), ctx->results.tri.maximum_angle.end());
            qualityMaxMin.triMaxMin.maximum_angle = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.tri.maximum_angle.begin(), ctx->results.tri.maximum_angle.end(), 0.0) / ctx->results.tri.maximum_angle.size();
            qualityMaxMin.triMaxMin.maximum_angle_mean = mean;
            qualityMaxMin.triMaxMin.num = ctx->results.tri.maximum_angle.size();
        }

        if (!ctx->results.tri.condition.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.tri.condition.begin(), ctx->results.tri.condition.end());
            qualityMaxMin.triMaxMin.condition = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.tri.condition.begin(), ctx->results.tri.condition.end(), 0.0) / ctx->results.tri.condition.size();
            qualityMaxMin.triMaxMin.condition_mean = mean;
            qualityMaxMin.triMaxMin.num = ctx->results.tri.condition.size();
        }

        if (!ctx->results.tri.scaled_jacobian.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.tri.scaled_jacobian.begin(), ctx->results.tri.scaled_jacobian.end());
            qualityMaxMin.triMaxMin.scaled_jacobian = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.tri.scaled_jacobian.begin(), ctx->results.tri.scaled_jacobian.end(), 0.0) / ctx->results.tri.scaled_jacobian.size();
            qualityMaxMin.triMaxMin.scaled_jacobian_mean = mean;
            qualityMaxMin.triMaxMin.num = ctx->results.tri.scaled_jacobian.size();
        }

        if (!ctx->results.tri.shape.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.tri.shape.begin(), ctx->results.tri.shape.end());
            qualityMaxMin.triMaxMin.shape = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.tri.shape.begin(), ctx->results.tri.shape.end(), 0.0) / ctx->results.tri.shape.size();
            qualityMaxMin.triMaxMin.shape_mean = mean;
            qualityMaxMin.triMaxMin.num = ctx->results.tri.shape.size();
        }

        if (!ctx->results.tri.distortion.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.tri.distortion.begin(), ctx->results.tri.distortion.end());
            qualityMaxMin.triMaxMin.distortion = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.tri.distortion.begin(), ctx->results.tri.distortion.end(), 0.0) / ctx->results.tri.distortion.size();
            qualityMaxMin.triMaxMin.distortion_mean = mean;
            qualityMaxMin.triMaxMin.num = ctx->results.tri.distortion.size();
        }

        if (!ctx->results.tri.equiangle_skew.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.tri.equiangle_skew.begin(), ctx->results.tri.equiangle_skew.end());
            qualityMaxMin.triMaxMin.equiangle_skew = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.tri.equiangle_skew.begin(), ctx->results.tri.equiangle_skew.end(), 0.0) / ctx->results.tri.equiangle_skew.size();
            qualityMaxMin.triMaxMin.equiangle_skew_mean = mean;
            qualityMaxMin.triMaxMin.num = ctx->results.tri.equiangle_skew.size();
        }

        if (!ctx->results.tri.normalized_inradius.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.tri.normalized_inradius.begin(), ctx->results.tri.normalized_inradius.end());
            qualityMaxMin.triMaxMin.normalized_inradius = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.tri.normalized_inradius.begin(), ctx->results.tri.normalized_inradius.end(), 0.0) / ctx->results.tri.normalized_inradius.size();
            qualityMaxMin.triMaxMin.normalized_inradius_mean = mean;
            qualityMaxMin.triMaxMin.num = ctx->results.tri.normalized_inradius.size();
        }

        if (!ctx->results.tri.relative_size_squared.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.tri.relative_size_squared.begin(), ctx->results.tri.relative_size_squared.end());
            qualityMaxMin.triMaxMin.relative_size_squared = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.tri.relative_size_squared.begin(), ctx->results.tri.relative_size_squared.end(), 0.0) / ctx->results.tri.relative_size_squared.size();
            qualityMaxMin.triMaxMin.relative_size_squared_mean = mean;
            qualityMaxMin.triMaxMin.num = ctx->results.tri.relative_size_squared.size();
        }

        if (!ctx->results.tri.shape_and_size.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.tri.shape_and_size.begin(), ctx->results.tri.shape_and_size.end());
            qualityMaxMin.triMaxMin.shape_and_size = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.tri.shape_and_size.begin(), ctx->results.tri.shape_and_size.end(), 0.0) / ctx->results.tri.shape_and_size.size();
            qualityMaxMin.triMaxMin.shape_and_size_mean = mean;
            qualityMaxMin.triMaxMin.num = ctx->results.tri.shape_and_size.size();
        }
        if (!ctx->results.tri.orthogonality.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.tri.orthogonality.begin(), ctx->results.tri.orthogonality.end());
            qualityMaxMin.triMaxMin.enhanced_orth = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.tri.orthogonality.begin(), ctx->results.tri.orthogonality.end(), 0.0) / ctx->results.tri.orthogonality.size();
            qualityMaxMin.triMaxMin.enhanced_orth_mean = mean;
            qualityMaxMin.triMaxMin.num = ctx->results.tri.orthogonality.size();
        }
    }
    void mQualityPanel::setLine(MechMainWindow::QualityMaxMin& qualityMaxMin)
    {
        //线网格
        if (!ctx->results.line.length.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.line.length.begin(), ctx->results.line.length.end());
            qualityMaxMin.lineMaxMin.length = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.line.length.begin(), ctx->results.line.length.end(), 0.0) / ctx->results.line.length.size();
            qualityMaxMin.lineMaxMin.length_mean = mean;
            qualityMaxMin.lineMaxMin.num = ctx->results.line.length.size();
        }
    }
    void mQualityPanel::setPyramid(MechMainWindow::QualityMaxMin& qualityMaxMin)
    {
        // 金字塔
        if (!ctx->results.pyramid.volume.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.pyramid.volume.begin(), ctx->results.pyramid.volume.end());
            qualityMaxMin.pyramidMaxMin.volume = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.pyramid.volume.begin(), ctx->results.pyramid.volume.end(), 0.0) / ctx->results.pyramid.volume.size();
            qualityMaxMin.pyramidMaxMin.volume_mean = mean;
            qualityMaxMin.pyramidMaxMin.num = ctx->results.pyramid.volume.size();
        }

        if (!ctx->results.pyramid.jacobian.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.pyramid.jacobian.begin(), ctx->results.pyramid.jacobian.end());
            qualityMaxMin.pyramidMaxMin.jacobian = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.pyramid.jacobian.begin(), ctx->results.pyramid.jacobian.end(), 0.0) / ctx->results.pyramid.jacobian.size();
            qualityMaxMin.pyramidMaxMin.jacobian_mean = mean;
            qualityMaxMin.pyramidMaxMin.num = ctx->results.pyramid.jacobian.size();
        }

        if (!ctx->results.pyramid.scaled_jacobian.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.pyramid.scaled_jacobian.begin(), ctx->results.pyramid.scaled_jacobian.end());
            qualityMaxMin.pyramidMaxMin.scaled_jacobian = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.pyramid.scaled_jacobian.begin(), ctx->results.pyramid.scaled_jacobian.end(), 0.0) / ctx->results.pyramid.scaled_jacobian.size();
            qualityMaxMin.pyramidMaxMin.scaled_jacobian_mean = mean;
            qualityMaxMin.pyramidMaxMin.num = ctx->results.pyramid.scaled_jacobian.size();
        }

        if (!ctx->results.pyramid.shape.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.pyramid.shape.begin(), ctx->results.pyramid.shape.end());
            qualityMaxMin.pyramidMaxMin.shape = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.pyramid.shape.begin(), ctx->results.pyramid.shape.end(), 0.0) / ctx->results.pyramid.shape.size();
            qualityMaxMin.pyramidMaxMin.shape_mean = mean;
            qualityMaxMin.pyramidMaxMin.num = ctx->results.pyramid.shape.size();
        }
        if (!ctx->results.pyramid.equiangle_skew.empty())
        {
            auto maxmin = std::minmax_element(ctx->results.pyramid.equiangle_skew.begin(), ctx->results.pyramid.equiangle_skew.end());
            qualityMaxMin.pyramidMaxMin.equiangle_skew = { *maxmin.first, *maxmin.second };
            auto mean = std::accumulate(ctx->results.pyramid.equiangle_skew.begin(), ctx->results.pyramid.equiangle_skew.end(), 0.0) / ctx->results.pyramid.equiangle_skew.size();
            qualityMaxMin.pyramidMaxMin.equiangle_skew_mean = mean;
            qualityMaxMin.pyramidMaxMin.num = ctx->results.pyramid.equiangle_skew.size();
        }
    }
}
