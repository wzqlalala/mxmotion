#pragma once
#pragma execution_character_set("utf-8")
#include "mmainwindow_global.h"

//QT
#include <QLabel>
#include <QPushButton>
#include <QObject>
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QComboBox>
#include <QListView>
#include <QCheckBox>
#include <QSpinBox>
#include <QHeaderView>
#include <QTableWidget>
#include <QTreeWidget>
#include <QRadioButton>
#include <QButtonGroup>

#include "mTableDataWindow.h"

//BasicData
#include "mModelData.h"
#include "mMaterialBase.h"
#include "mUnitSystemManager.h"
//#include "mEmptyMaterial.h"
//#include "mIsotropyMaterial.h"

#include "mMaterialModel.h"
#include "mCalcLineEdit.h"
#include "mPanelBase.h"

#include <functional>

// 校验模式别名，用于材质属性输入控件
using CalcMode = MxFunctions::mCalcLineEdit::CalcMode;

//MxFunction
#include "mPhysicalFieldEnum.h"
#include "mOtherFunction.h"

using namespace MxFunctions;
using namespace MechMaterialData;
namespace MechMainWindow
{
    class mParamCurveControl;
    struct singleCheckBox;

    /**
     * @brief 「温度相关」行封装：标签 + 勾选框 + 吸附式曲线弹窗入口按钮（三位一体）
     *
     * 布局：左侧标签 + 勾选框（两者合计固定宽 90，与上方输入行 PanelLabel 对齐，
     * 勾选框视觉紧贴标签文字），右侧为 mParamCurveControl 剖分出来的入口按钮
     * （拉伸填充至行尾，与输入框右缘对齐）。
     * 按钮可用状态与勾选框联动（勾选可用、未勾选置灰并关闭弹窗），
     * 控件本体隐藏，仅承担弹窗宿主职责。
     *
     * 数据协议与 singleCheckBox 一致：曲线数据（列主序，首列自变量）存于
     * checkBox()->property("curve")，任何数据变化经 dataChanged 信号透出，
     * 宿主据此 emit sg_editFinished()。
     */
    class MMAINWINDOW_EXPORT mTempCurveRowWidget : public QWidget
    {
        Q_OBJECT
    public:
        /**
         * @param labelText         左侧标签文本，如 "温度相关"
         * @param curveTitle        入口按钮/弹窗标题，如 "温度-各向同性参数曲线"
         * @param independentHeader 自变量列标题（含单位），如 "温度 / °C"
         * @param paramHeaders      参数列标题（含单位），如 {"杨氏模量 / MPa", "泊松比"}
         */
        mTempCurveRowWidget(const QString& labelText,
            const QString& curveTitle,
            const QString& independentHeader,
            const QStringList& paramHeaders,
            QWidget* parent = nullptr);

        /** 列标题是否含单位后缀（如 "杨氏模量 / MPa"）；默认 true。
            停靠表兼容通道或表头约定不含单位时置 false，直接用传入文本 */
        void setHeaderHasUnit(bool hasUnit);
        /** 曲线表格标题/自变量/参数列标题（单位制切换时刷新） */
        void setCurveHeaders(const QString& curveTitle,
            const QString& independentHeader,
            const QStringList& paramHeaders);

        QCheckBox* checkBox() const { return _cb; }
        mParamCurveControl* curveControl() const { return _curveCtrl; }
        QPushButton* entryButton() const { return _entryBtn; }

        /** 勾选状态同步：界面封装勾选框（触发联动）+ 数据载体 singleCheckBox 的 cb */
        void syncCheckedTo(singleCheckBox& carrier, bool checked);

        /** 同 syncCheckedTo，但双向 blockSignals：仅同步状态、不触发 stateChanged，
         *  供界面勾选框的 stateChanged 槽内调用，避免程序化 setChecked 递归触发保存 */
        void syncCheckedToCarrier(singleCheckBox& carrier, bool checked);

        /** 覆盖式设置曲线数据：curves[0] 为自变量列，其后每列一个参数；
         *  同步写入 checkBox 的 "curve" 属性（与 collectProps(singleCheckBox) 协议一致） */
        void setCurveData(const QVector<QVector<double>>& curves);
        /** 清空全部行与 "curve" 属性 */
        void clearCurve();

    signals:
        /** 曲线数据发生任何变化（编辑/增删/清空/导入/外部 setCurveData） */
        void dataChanged();

    private:
        QLabel* _label = nullptr;
        QCheckBox* _cb = nullptr;
        QPushButton* _entryBtn = nullptr;
        mParamCurveControl* _curveCtrl = nullptr;
        QString _curveTitle;
        QString _independentHeader;
        QStringList _paramHeaders;
        bool _headerHasUnit = true;
    };


    struct InputWidgetSet
    {
        QLabel* labelname = nullptr;
        mCalcLineEdit* lineeditvalue = nullptr;
        QLabel* labelunit = nullptr;
        QHBoxLayout* layout = nullptr;
        QWidget* propwid = nullptr;
        MxFunctions::MaterialPropType proptype_;
        Unit unit_;
        QString name_;
        void setDisableAll(bool on)
        {
            labelname->setDisabled(on);
            lineeditvalue->setDisabled(on);
            labelunit->setDisabled(on);
            propwid->setDisabled(on);
        }
        void initWidget(const QString& name, bool hasunit = false, const MxFunctions::MaterialPropType& proptype = MaterialPropType::Unknown, CalcMode mode = CalcMode::Standard)
        {
            name_ = name;
            proptype_ = proptype;
            labelname = new PanelLabel;
            lineeditvalue = new mCalcLineEdit(mode);
            labelunit = new QLabel();
            layout = new QHBoxLayout();
            propwid = new QWidget();
            labelname->setText(name);
            //labelname->setMinimumWidth(115);
			layout->setMargin(0);
			layout->addWidget(labelname);
			layout->addWidget(lineeditvalue);
			if (hasunit)
				layout->addWidget(labelunit);

            propwid->setWindowFlags(Qt::Widget);
            propwid->setContentsMargins(0, 0, 0, 0);
            propwid->setLayout(layout);
        }
        void clear()
        {
            if (labelname)
            {
                delete labelname;
                labelname = nullptr;
            }
            if (lineeditvalue)
            {
                delete lineeditvalue;
                lineeditvalue = nullptr;
            }
            if (labelunit)
            {
                delete labelunit;
                labelunit = nullptr;
            }
            if (layout)
            {
                delete layout;
                layout = nullptr;
            }
            if (propwid)
            {
                delete propwid;
                propwid = nullptr;
            }
        }

    };
    struct singleComboBox
    {
        QLabel* labelname = nullptr;
        QComboBox* combobox = nullptr;
        QHBoxLayout* layout = nullptr;
        QWidget* propwid = nullptr;
        MxFunctions::MaterialPropType proptype_;
        void setDisableAll(bool on)
        {
            labelname->setDisabled(on);
            combobox->setDisabled(on);
            propwid->setDisabled(on);
        }
        void initWidget(const QString& name, const MxFunctions::MaterialPropType& proptype
            = MaterialPropType::Unknown)
        {
            proptype_ = proptype;
            combobox = new QComboBox();
            combobox->setView(new QListView);
            combobox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            // 或设置最小宽度，防止被过度压缩：
            combobox->setMinimumWidth(100);
            labelname = new PanelLabel();
            layout = new QHBoxLayout();
            propwid = new QWidget();
            labelname->setText(name);
            //labelname->setMinimumWidth(115);
            layout->setMargin(0);
            //layout->setSpacing(0); // 无间距
            layout->addWidget(labelname);
            layout->addWidget(combobox);
            propwid->setWindowFlags(Qt::Widget);
            propwid->setContentsMargins(0, 0, 0, 0);
            propwid->setLayout(layout);
        }
        void clear()
        {
            if (labelname)
            {
                delete labelname;
                labelname = nullptr;
            }
            if (combobox)
            {
                delete combobox;
                combobox = nullptr;
            }
            if (layout)
            {
                delete layout;
                layout = nullptr;
            }
            if (propwid)
            {
                delete propwid;
                propwid = nullptr;
            }
        }
    };

    struct radioButBox
    {
        QHBoxLayout* layout = nullptr;
        QVector<QRadioButton*> radioVec;
        QButtonGroup* group;
        int currentId = 0;

        QWidget* propwid = nullptr;
        MxFunctions::MaterialPropType proptype_;

        void initWidget(const QStringList& radioButs,
            const MxFunctions::MaterialPropType& proptype = MaterialPropType::Unknown)
        {
            proptype_ = proptype;
            layout = new QHBoxLayout();
            propwid = new QWidget();
            group = new QButtonGroup();
            group->setExclusive(true);
            int i = 0;
            for (auto str : radioButs)
            {
                QRadioButton* but = new QRadioButton(str);
                layout->addWidget(but);
                radioVec.append(but);
                group->addButton(but, i);
                i++;
            }


            propwid->setLayout(layout);
            propwid->setWindowFlags(Qt::Widget);
            propwid->setContentsMargins(0, 0, 0, 0);
        }

        void clear()
        {
            if (layout)
            {
                delete layout;
                layout = nullptr;
            }
            if (propwid)
            {
                delete propwid;
                propwid = nullptr;
            }
        }
    };

    struct comboBoxWithPushBtn
    {
        QLabel* labelname = nullptr;
        QComboBox* combobox = nullptr;
        QPushButton* pushbtn = nullptr;
        QHBoxLayout* layout = nullptr;
        QWidget* propwid = nullptr;
        MxFunctions::MaterialPropType proptype_;
        void initWidget(const QString& name, const MxFunctions::MaterialPropType& proptype = MaterialPropType::Unknown)
        {
            proptype_ = proptype;
            combobox = new QComboBox();
            combobox->setView(new QListView);
            labelname = new PanelLabel();
            layout = new QHBoxLayout();
            propwid = new QWidget();
            pushbtn = new QPushButton();
            labelname->setText(name);
            pushbtn->setFixedWidth(30);
            //combobox->setMinimumWidth(160);
            layout->setMargin(0);
            layout->setSpacing(5);
            layout->addWidget(labelname);
            layout->addWidget(combobox);
            layout->addWidget(pushbtn);
            propwid->setWindowFlags(Qt::Widget);
            propwid->setContentsMargins(0, 0, 0, 0);
            propwid->setLayout(layout);
        }
        void clear()
        {
            if (pushbtn)
            {
                delete pushbtn;
                pushbtn = nullptr;
            }
            if (labelname)
            {
                delete labelname;
                labelname = nullptr;
            }
            if (combobox)
            {
                delete combobox;
                combobox = nullptr;
            }
            if (layout)
            {
                delete layout;
                layout = nullptr;
            }
            if (propwid)
            {
                delete propwid;
                propwid = nullptr;
            }
        }
    };
    struct InputWithCheckBox
    {
        QCheckBox* checkbox = nullptr;
        mCalcLineEdit* lineeditvalue = nullptr;
        QLabel* labelunit = nullptr;
        QHBoxLayout* layout = nullptr;
        QWidget* propwid = nullptr;
        MxFunctions::MaterialPropType proptype_;
        void setVisibleAll(bool vis)
        {
            checkbox->setVisible(vis);
            lineeditvalue->setVisible(vis);
            labelunit->setVisible(vis);
            propwid->setVisible(vis);
        }
        void setDisableAll(bool on)
        {
            checkbox->setDisabled(on);
            lineeditvalue->setDisabled(on);
            labelunit->setDisabled(on);
            propwid->setDisabled(on);
        }
        void initWidget(bool hasunit = false, const MxFunctions::MaterialPropType& proptype = MaterialPropType::Unknown)
        {
            proptype_ = proptype;
            checkbox = new QCheckBox();
            lineeditvalue = new mCalcLineEdit();
            labelunit = new QLabel();
            layout = new QHBoxLayout();
            propwid = new QWidget();
            checkbox->setFixedWidth(90);
            layout->setMargin(0);
            layout->addWidget(checkbox);
            layout->addWidget(lineeditvalue);
            if (hasunit)layout->addWidget(labelunit);
            //propwid->setMinimumWidth(250);
            propwid->setWindowFlags(Qt::Widget);
            propwid->setContentsMargins(0, 0, 0, 0);
            propwid->setLayout(layout);
        }
        void clear()
        {
            if (checkbox)
            {
                delete checkbox;
                checkbox = nullptr;
            }
            if (lineeditvalue)
            {
                delete lineeditvalue;
                lineeditvalue = nullptr;
            }
            if (labelunit)
            {
                delete labelunit;
                labelunit = nullptr;
            }
            if (layout)
            {
                delete layout;
                layout = nullptr;
            }
            if (propwid)
            {
                delete propwid;
                propwid = nullptr;
            }
        }
    };
    struct singleSpinBox
    {
        QLabel* labelname = nullptr;
        QSpinBox* spinbox = nullptr;
        QHBoxLayout* layout = nullptr;
        QWidget* propwid = nullptr;
        MxFunctions::MaterialPropType proptype_;
        void initWidget(const QString& name, const MxFunctions::MaterialPropType& proptype = MaterialPropType::Unknown)
        {
            proptype_ = proptype;
            spinbox = new QSpinBox();
            labelname = new PanelLabel();
            layout = new QHBoxLayout();
            propwid = new QWidget();
            labelname->setText(name);
            layout->setMargin(0);
            layout->addWidget(labelname);
            layout->addWidget(spinbox);
            propwid->setWindowFlags(Qt::Widget);
            propwid->setContentsMargins(0, 0, 0, 0);
            propwid->setLayout(layout);
        }
        void setDisableAll(bool on)
        {
            labelname->setDisabled(on);
            spinbox->setDisabled(on);
            propwid->setDisabled(on);
        }
        void clear()
        {
            if (labelname)
            {
                delete labelname;
                labelname = nullptr;
            }
            if (spinbox)
            {
                delete spinbox;
                spinbox = nullptr;
            }
            if (layout)
            {
                delete layout;
                layout = nullptr;
            }
            if (propwid)
            {
                delete propwid;
                propwid = nullptr;
            }
        }
    };
    struct singleTableWidget
    {
        QLabel* labelname = nullptr;
        mTableWidget* tablewidget = nullptr;
        QVBoxLayout* layout = nullptr;
        QWidget* propwid = nullptr;
        MxFunctions::MaterialPropType proptype_;
        void initWidget(const QString& name, const MxFunctions::MaterialPropType& proptype = MaterialPropType::Unknown)
        {
            proptype_ = proptype;
            tablewidget = new mTableWidget();
            labelname = new QLabel();
            layout = new QVBoxLayout();
            propwid = new QWidget();
            labelname->setText(name);
            tablewidget->verticalHeader()->setHidden(true);
            layout->setMargin(0);
            layout->addWidget(labelname);
            layout->addWidget(tablewidget);
            layout->addStretch();
            propwid->setMinimumWidth(250);
            propwid->setWindowFlags(Qt::Widget);
            propwid->setContentsMargins(0, 0, 0, 0);
            propwid->setLayout(layout);
        }
        QStringList getHeaders()
        {
            QStringList headers;
            for (int i = 0; i < tablewidget->columnCount(); ++i)
            {
                QTableWidgetItem* item = tablewidget->horizontalHeaderItem(i);
                if (item)
                {
                    headers << item->text();
                }
                else
                {
                    headers << QString("Column%1").arg(i);
                }
            }

            return headers;
        }
        QVector<QVector<double>> collectValue()
        {
            int row = tablewidget->rowCount();
            int col = tablewidget->columnCount();
            QVector<QVector<double>>res;
            // 初始化结果数组
            res.resize(row);
            for (int i = 0; i < row; ++i)
            {
                res[i].resize(col);
            }
            // 收集数据
            for (int i = 0; i < row; ++i)
            {
                for (int j = 0; j < col; ++j)
                {
                    QTableWidgetItem* item = tablewidget->item(i, j);
                    if (item && !item->text().isEmpty())
                    {
                        // 尝试转换为double，如果转换失败则设为0
                        bool ok;
                        double value = item->text().toDouble(&ok);
                        if (ok)
                        {
                            res[i][j] = value;
                        }
                        else
                        {
                            res[i][j] = 0.0;
                        }
                    }
                    else
                    {
                        res[i][j] = 0.0;
                    }
                }
            }

            return res;
        }
        void setValue(const QVector<QVector<double>>& values)
        {
            tablewidget->setFixedRow(values.size());
            for (int row = 0; row < values.size(); ++row)
            {
                for (int col = 0; col < values[row].size(); ++col)
                {
                    QTableWidgetItem* item = tablewidget->item(row, col);
                    if (item)
                    {
                        item->setText(MxFunctions::mNumberToString(values[row][col]));
                    }
                }
            }
        }
        void clearData(int rowcnt, int colcnt)
        {
            for (int row = 0; row < rowcnt; ++row)
            {
                for (int col = 0; col < colcnt; ++col)
                {
                    QTableWidgetItem* item = tablewidget->item(row, col);
                    if (item)
                    {
                        item->setText("");
                    }
                }
            }
        }
        void clear()
        {
            if (labelname)
            {
                delete labelname;
                labelname = nullptr;
            }
            if (tablewidget)
            {
                delete tablewidget;
                tablewidget = nullptr;
            }
            if (layout)
            {
                delete layout;
                layout = nullptr;
            }
            if (propwid)
            {
                delete propwid;
                propwid = nullptr;
            }
        }
    };
    struct singleCheckBox
    {
        QCheckBox* cb = nullptr;
        QHBoxLayout* layout = nullptr;
        QPushButton* tablebtn = nullptr;
        QWidget* propwid = nullptr;
        QLabel* labelname = nullptr;
        MxFunctions::MaterialPropType proptype_;

		void setPropType(const MxFunctions::MaterialPropType& proptype)
		{
			proptype_ = proptype;
			if (tablebtn)
				tablebtn->setProperty("curvePropType", static_cast<int>(proptype));
		}

		void initWidget(const QString&name,bool usingBtn = true,
		          const MxFunctions::MaterialPropType& proptype = MaterialPropType::Unknown)
		{
		          setPropType(proptype);

		          cb = new QCheckBox();
		          cb->setFixedWidth(92);  // 和上面 labelname 宽度一致
		          cb->setText(name);

		          tablebtn = new QPushButton();
		          tablebtn->setFixedSize(30, 30);
		          tablebtn->setProperty("curvePropType", static_cast<int>(proptype));
		          QIcon icon(":/mMainWidgets/Icon/table.png");
		          tablebtn->setIcon(icon);
		          tablebtn->setIconSize(tablebtn->size());
		          tablebtn->setStyleSheet(
		              "QPushButton:pressed {"
		              "    background-color: #666666;"
		              "    border: 2px solid #333333;"
		              "}"
		          );

            layout = new QHBoxLayout();
            propwid = new QWidget();
            layout->setMargin(0);
            layout->setSpacing(4);
            layout->addWidget(cb);
            if (usingBtn)
                layout->addWidget(tablebtn);
            layout->addStretch();

            propwid->setWindowFlags(Qt::Widget);
            propwid->setContentsMargins(0, 0, 0, 0);
            propwid->setLayout(layout);
		}
		void clear()
		{
			if (cb)
			{
				delete cb;
				cb = nullptr;
			}
			if (tablebtn)
			{
				delete tablebtn;
				tablebtn = nullptr;
			}
			if (layout)
			{
				delete layout;
				layout = nullptr;
			}
			if (propwid)
			{
				delete propwid;
				propwid = nullptr;
			}
		}
	}; 
	struct singleLabelBtn
	{
		QLabel* lablename = nullptr;
		QHBoxLayout* layout = nullptr;
		QPushButton* tablebtn = nullptr;
		QWidget* propwid = nullptr;
		MxFunctions::MaterialPropType proptype_;
		QVector<Unit>units_;
        void setPropType(const MxFunctions::MaterialPropType& proptype)
        {
            proptype_ = proptype;
            if (tablebtn)
                tablebtn->setProperty("curvePropType", static_cast<int>(proptype));
        }
		void initWidget(const QString& name, const MxFunctions::MaterialPropType& proptype = MaterialPropType::Unknown)
		{
            setPropType(proptype);
            if(proptype)
            tablebtn->setProperty("curvePropType", static_cast<int>(proptype));
			proptype_ = proptype;
			lablename = new QLabel();
			tablebtn = new QPushButton();
			layout = new QHBoxLayout();
			propwid = new QWidget();
			tablebtn->setFixedSize(30, 30);
			lablename->setText(name);
			lablename->setFixedWidth(90);
			QIcon icon(":/mMainWidgets/Icon/table.png");
			tablebtn->setIcon(icon);
			tablebtn->setIconSize(tablebtn->size());
			tablebtn->setStyleSheet(
				"QPushButton:pressed {"
				"    background-color: #666666;"
				"    border: 2px solid #333333;"
				"}"
			);
			layout->setMargin(0);
			layout->addWidget(lablename);
			layout->addWidget(tablebtn);
			layout->addStretch();
			propwid->setMinimumWidth(250);
			propwid->setWindowFlags(Qt::Widget);
			propwid->setContentsMargins(0, 0, 0, 0);
			propwid->setLayout(layout);
		}
		void clear()
		{
			if (lablename)
			{
				delete lablename;
				lablename = nullptr;
			}
			if (tablebtn)
			{
				delete tablebtn;
				tablebtn = nullptr;
			}
			if (layout)
			{
				delete layout;
				layout = nullptr;
			}
			if (propwid)
			{
				delete propwid;
				propwid = nullptr;
			}
		}
	};
    struct treeWidgetWithPushBtns
    {
        QTreeWidget* treewidget = nullptr;
        QPushButton* pushbtnremove = nullptr;
        QPushButton* pushbtnappend = nullptr;
        QPushButton* pushbtnclear = nullptr;
        QGridLayout* gridLayout = nullptr;   // 改为 QGridLayout
        QWidget* propwid = nullptr;
        MxFunctions::MaterialPropType proptype_;

        void setDisabled(bool on)
        {
            treewidget->setDisabled(on);
            pushbtnremove->setDisabled(on);
            pushbtnclear->setDisabled(on);
            pushbtnappend->setDisabled(on);
        }

        void initWidget(const MxFunctions::MaterialPropType& proptype = MaterialPropType::Unknown)
        {
            proptype_ = proptype;

            treewidget = new QTreeWidget();
            pushbtnremove = new QPushButton();
            pushbtnappend = new QPushButton();
            pushbtnclear = new QPushButton();
            propwid = new QWidget();
            gridLayout = new QGridLayout(propwid);

            pushbtnremove->setIcon(QIcon(":/mMainWidgets/Icon/minimize3x.png"));
            pushbtnremove->setToolTip("移除拾取");

            pushbtnappend->setIcon(QIcon(":/mMainWidgets/Icon/daoru3x.png"));
            pushbtnappend->setToolTip("添加拾取");

            pushbtnclear->setIcon(QIcon(":/mMainWidgets/Icon/shanchu3x.png"));
            pushbtnclear->setToolTip("清空拾取");

            treewidget->setHeaderHidden(true);
            treewidget->setFrameShape(QFrame::NoFrame);
            treewidget->setStyleSheet(
                "QTreeWidget::item {"
                "    padding: 0px;" // 最小内边距
                "    margin: 0px;"  // 无外边距
                "    border: none;"
                "    min-height: 5px;" // 设置最小高度
                "}"
            );

            pushbtnremove->setFixedSize(26, 26);
            pushbtnclear->setFixedSize(26, 26);
            pushbtnappend->setFixedSize(26, 26);

            // 布局
            gridLayout->setSpacing(8);
            gridLayout->setContentsMargins(0, 0, 0, 0);
            gridLayout->setColumnStretch(0, 1);

            gridLayout->addWidget(treewidget, 0, 0, 8, 7);
            gridLayout->addWidget(pushbtnappend, 0, 7);
            gridLayout->addWidget(pushbtnremove, 1, 7);
            gridLayout->addWidget(pushbtnclear, 2, 7);

            propwid->setWindowFlags(Qt::Widget);
            propwid->setContentsMargins(0, 0, 0, 0);
        }

        void clear()
        {
            if (pushbtnappend)
            {
                delete pushbtnappend;
                pushbtnappend = nullptr;
            }
            if (pushbtnremove)
            {
                delete pushbtnremove;
                pushbtnremove = nullptr;
            }
            if (pushbtnclear)
            {
                delete pushbtnclear;
                pushbtnclear = nullptr;
            }
            if (treewidget)
            {
                delete treewidget;
                treewidget = nullptr;
            }
            if (propwid)
            {
                delete propwid;
                propwid = nullptr;
            }
        }
    };

    struct pushButtonsSet
    {
        QLabel* labelname = nullptr;
        QPushButton* btnapply = nullptr;
        QPushButton* btnreverse = nullptr;
        QHBoxLayout* layout = nullptr;
        QWidget* propwid = nullptr;
        MxFunctions::MaterialPropType proptype_;
        void setDisableAll(bool on)
        {
            labelname->setDisabled(on);
            btnapply->setDisabled(on);
            btnreverse->setDisabled(on);
            propwid->setDisabled(on);
        }
        void initWidget(const QString& name, const MxFunctions::MaterialPropType& proptype = MaterialPropType::Unknown)
        {
            proptype_ = proptype;
            btnapply = new QPushButton();
            btnreverse = new QPushButton();
            btnapply->setText("应用");
            btnreverse->setText("反向");
            labelname = new PanelLabel();
            layout = new QHBoxLayout();
            propwid = new QWidget();
            labelname->setText(name);
            btnapply->setMinimumWidth(25);
            btnreverse->setMinimumWidth(25);
            layout->setMargin(0);
            layout->addWidget(labelname);
            layout->addWidget(btnapply);
            layout->addWidget(btnreverse);
            propwid->setWindowFlags(Qt::Widget);
            propwid->setContentsMargins(0, 0, 0, 0);
            propwid->setLayout(layout);
        }
        void clear()
        {
            if (labelname)
            {
                delete labelname;
                labelname = nullptr;
            }
            if (btnapply)
            {
                delete btnapply;
                btnapply = nullptr;
            }
            if (btnreverse)
            {
                delete btnreverse;
                btnreverse = nullptr;
            }
            if (layout)
            {
                delete layout;
                layout = nullptr;
            }
            if (propwid)
            {
                delete propwid;
                propwid = nullptr;
            }
        }
    };
    struct singleLabel
    {
        QLabel* labelname = nullptr;
        QLabel* labelvalue = nullptr;
        QLabel* labelunit = nullptr;
        QHBoxLayout* layout = nullptr;
        QWidget* propwid = nullptr;
        MxFunctions::MaterialPropType proptype_;
        void initWidget(const QString& name, const MxFunctions::MaterialPropType& proptype = MaterialPropType::Unknown)
        {
            proptype_ = proptype;
            labelname = new PanelLabel();
            labelvalue = new QLabel();
            labelunit = new QLabel();
            layout = new QHBoxLayout();
            propwid = new QWidget();
            labelname->setText(name);
            labelvalue->setMinimumWidth(160);
            labelvalue->setAlignment(Qt::AlignLeft);
            layout->setMargin(0);
            layout->addWidget(labelname);
            layout->addWidget(labelvalue);
            layout->addWidget(labelunit);
            layout->addStretch();
            propwid->setMinimumWidth(250);
            propwid->setWindowFlags(Qt::Widget);
            propwid->setContentsMargins(0, 0, 0, 0);
            propwid->setLayout(layout);
        }
        void clear()
        {
            if (labelname)
            {
                delete labelname;
                labelname = nullptr;
            }
            if (labelvalue)
            {
                delete labelvalue;
                labelvalue = nullptr;
            }
            if (labelunit)
            {
                delete labelunit;
                labelunit = nullptr;
            }
            if (layout)
            {
                delete layout;
                layout = nullptr;
            }
            if (propwid)
            {
                delete propwid;
                propwid = nullptr;
            }
        }
    };
    struct singleButton
    {
        QLabel* text = nullptr;
        QHBoxLayout* layout = nullptr;
        QPushButton* btn = nullptr;
        QWidget* propwid = nullptr;
        void initWidget(const QString& name)
        {
            text = new QLabel();
            text->setFixedWidth(90);
            btn = new QPushButton();
            layout = new QHBoxLayout();
            propwid = new QWidget();
            btn->setMinimumSize(140, 30);
            text->setText(name);
            layout->setMargin(0);
            layout->addWidget(text);
            layout->addWidget(btn);
            layout->addStretch();
            propwid->setWindowFlags(Qt::Widget);
            propwid->setContentsMargins(0, 0, 0, 0);
            propwid->setLayout(layout);
        }
        void clear()
        {
            if (text)
            {
                delete text;
                text = nullptr;
            }
            if (btn)
            {
                delete btn;
                btn = nullptr;
            }
            if (layout)
            {
                delete layout;
                layout = nullptr;
            }
            if (propwid)
            {
                delete propwid;
                propwid = nullptr;
            }
        }
    };

    struct foldDetail
    {
        bool checkable = false; //默认必选false 可选true
        QString foldText;
        QVector<QWidget*> vecWidget;
    };
    class MMAINWINDOW_EXPORT mMaterialPropBase : public FoldSection
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mMaterialPropBase(const MxFunctions::MaterialClass matType, const QString propname, QWidget* parent = nullptr);

        /*
        * 析构函数
        */
        virtual ~mMaterialPropBase();

        /*
        * 获取材料类型
        */
        virtual MxFunctions::MaterialClass getPropertyClass();

        /*
        * 收集当前属性
        */
        virtual QHash<MxFunctions::MaterialPropType, QVariant>collectProperties() = 0;

        /*
        * 收集材料库属性
        */
        virtual QHash<QString, QPair<QVector<Unit>, QVariant>> collectLibProperties()
        {
            return QHash<QString, QPair<QVector<Unit>, QVariant>>();
        };

        /*
        * 所有输入控件
        */
        virtual void InitFoldSection() = 0;

        /*
        * 增加获得属性所有窗体默认 所属的FoldSection
        */
        virtual QVector<foldDetail*> getAllInputsFold(QVBoxLayout* mainLayout,
            QList<FoldSection*>& folds)
        {
            return QVector<foldDetail*>();
        }


        /*
        * 获取属性名
        */
        virtual QString getPropName();

        /*
        * 初始化控件
        */
        virtual void initInputs() = 0;

        /*
        * 设置控件值
        */
        virtual void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop);
        virtual void setInputsValue(const QHash<QString, QPair<QVector<Unit>, QVariant>>& prop);

        /*
        * 清空控件
        */
        virtual void clearInputs() = 0;

        /*
        * 清空数据
        */
        virtual void clearData() {};

        /*
        * 更新当前界面单位制
        */
        virtual void updatePropUnit() = 0;

        /*
        * 连接单位系统变化响应（进入编辑态时调用；默认连接到虚槽 slot_unitSystemResponse）
        */
        virtual void connectUnitSystemResponse();

        /*
        * 断开单位系统变化响应（退出编辑态时调用）
        */
        virtual void disconnectUnitSystemResponse();

        /*
        * 单位系统变化响应：模型值固定基准单位制{3,0,0,0}，不随系统变化；
        * 涉及单位的派生类重写本函数，把显示值按"旧系统→新系统"换算并同步单位标签（参考 mDensityProp）
        */
        virtual void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx);

        /*
        * 控制表格信号连接
        */
        virtual void connTable(bool conn) {};

        /*
        * 编辑表格
        */
        void tableEdit(bool usingCurve, QString title, QVariant value, QStringList header);


        /*
        * 获取有效的表格数据
        */
        static QVector<QVector<double>> getValidTableData(const QVector<QVector<double>>& x, const QVector<QVector<double>>& y);

        /*
        * 材料库曲线行：标签 + 吸附式曲线弹窗入口（无「温度相关」勾选框，曲线常驻可编辑）。
        * 列标题为「温度 + 属性名」，与原停靠表约定一致；返回行控件（父对象为本折叠块）。
        * 编辑产生的数据变化经 onDataChanged 回调透出（NaN 逐列过滤后的列主序曲线，首列温度）
        */
        QWidget* createLibCurveRow(const QString& name,
            const QVector<QVector<double>>& curves,
            const std::function<void(const QVector<QVector<double>>&)>& onDataChanged);

        void bindMaterial(const std::shared_ptr<MechMaterialData::mMaterialBase>& mat) { m_matData = mat; }

        static bool s_unitSyncing;
    protected:
        /*
        * 收集属性
        */
        virtual void collectProps(QHash<MxFunctions::MaterialPropType, QVariant>& props, const QVector<InputWidgetSet>& inputWidgets);
        virtual void collectProps(QHash<MxFunctions::MaterialPropType, QVariant>& props, const QVector<InputWidgetSet>& inputWidgets, const QHash<MxFunctions::MaterialPropType, double>& ratios);
        virtual void collectProps(QHash<QString, QPair<QVector<Unit>, QVariant>>& props, const QVector<InputWidgetSet*>& inputWidgets);
        virtual void collectProps(QHash<MxFunctions::MaterialPropType, QVariant>& props, const QVector<InputWithCheckBox>& inputWidgets);
        virtual void collectProps(QHash<MxFunctions::MaterialPropType, QVariant>& props, singleTableWidget& inputWidget);
        virtual void collectProps(QHash<MxFunctions::MaterialPropType, QVariant>& props, const singleCheckBox& inputWidgets, const CurveType& correlation = CurveType::TemperatureCorrelation);
        virtual void collectProps(QHash<MxFunctions::MaterialPropType, QVariant>& props, const singleLabelBtn& inputWidgets, const CurveType& correlation = CurveType::NoCorrelation);
        virtual void collectProps(QHash<MxFunctions::MaterialPropType, QVariant>& props, const MxFunctions::MaterialPropType& proptype, const QString& singleporp);
        virtual void collectProps(QHash<MxFunctions::MaterialPropType, QVariant>& props, const MxFunctions::MaterialPropType& proptype, const int& singleporp);


    signals:
        void sg_editFinished();

        void sg_hardModelChanged(int index);

        void hideFold(int index);

        void sig_qComBoxTypeChanged(MxFunctions::MaterialType type);

    protected:
        //属性名
        QString m_strPropName;

        //材料类别
        MxFunctions::MaterialClass m_foldClass;

        std::shared_ptr<MechMaterialData::mMaterialBase> m_matData;
    };

}