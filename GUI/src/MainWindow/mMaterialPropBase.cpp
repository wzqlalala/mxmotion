#include "mMaterialPropBase.h"
#include "mUnitSystemManager.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mModelData.h"
#include "mMaterialBase.h"
#include "mParamCurveControl.h"

#include <QFontMetrics>
#include <QSignalBlocker>
#include <QStyle>
#include <cmath>
#include <limits>

using namespace MechMaterialData;
namespace MechMainWindow
{
    mTempCurveRowWidget::mTempCurveRowWidget(const QString& labelText,
        const QString& curveTitle,
        const QString& independentHeader,
        const QStringList& paramHeaders,
        QWidget* parent)
        : QWidget(parent)
    {
        QHBoxLayout* layout = new QHBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(4);

        _curveTitle = curveTitle;
        _independentHeader = independentHeader;
        _paramHeaders = paramHeaders;

        // 标签宽度 = 文本 + 勾选框，与 PanelLabel 的 90px 对齐：勾选框视觉紧贴文字
        _label = new QLabel(this);
        {
            QFontMetrics fm(_label->font());
            const int cbExtent = _label->style()->pixelMetric(QStyle::PM_IndicatorWidth);
            const int spacing = 4; // 与行布局 spacing 一致
            _label->setFixedWidth(qMax(0, 90 - cbExtent - spacing));
            _label->setText(labelText);
        }
        layout->addWidget(_label);

        _cb = new QCheckBox(this);
        _cb->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        layout->addWidget(_cb);

        // 曲线控件本体不入布局：仅承担吸附式弹窗宿主职责，创建后即隐藏
        _curveCtrl = new mParamCurveControl(curveTitle, independentHeader, paramHeaders, this);
        _curveCtrl->setAnchorWidget(this);

        // 入口按钮从控件中剖分出来直接放入行布局：按钮可用状态与控件本体的
        // 显隐/禁用解耦，点击经控件转发弹窗开关（checked 态由 popupOpened/Closed 同步）
        _entryBtn = _curveCtrl->entryButton();
        if (_entryBtn)
        {
            _curveCtrl->detachEntryButton(this);
            _entryBtn->setMinimumHeight(28);
            _entryBtn->setMaximumHeight(QWIDGETSIZE_MAX);
            _entryBtn->setMinimumWidth(0);
            _entryBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            _entryBtn->setEnabled(false); // 初始未勾选：置灰
            layout->addWidget(_entryBtn, 1); // 拉伸填充至行尾，与上方输入框右缘对齐
        }
        _curveCtrl->hide();

        // 按钮可用状态与勾选框联动：勾选可用、未勾选置灰并关闭弹窗
        connect(_cb, &QCheckBox::stateChanged, this, [this](int state) {
            if (_entryBtn)
                _entryBtn->setEnabled(state != 0);
            if (state == 0 && _curveCtrl)
                _curveCtrl->closePopup();
            });

        // 控件曲线数据（列主序，首列自变量）同步到勾选框 "curve" 属性，
        // 与 collectProps(singleCheckBox) 的既有协议一致；参数列空/非法单元格
        // 以 NaN 占位，此处按「温度有效 + 该参数该点有效」逐列过滤
        connect(_curveCtrl, &mParamCurveControl::dataChanged, this, [this]() {
            const QVector<QVector<double>> all = _curveCtrl->curve();
            QVector<QVector<double>> curves;
            if (all.size() >= 2 && !all[0].isEmpty())
            {
                curves.append(all[0]); // 自变量列
                for (int c = 1; c < all.size(); ++c)
                {
                    QVector<double> col;
                    col.reserve(all[0].size());
                    for (int r = 0; r < all[0].size(); ++r)
                    {
                        const double v = (r < all[c].size()) ? all[c][r]
                            : std::numeric_limits<double>::quiet_NaN();
                        if (!std::isnan(v))
                            col.append(v);
                    }
                    curves.append(col);
                }
            }
            _cb->setProperty("curve", QVariant::fromValue(curves));
            emit dataChanged();
            });
    }

    void mTempCurveRowWidget::syncCheckedTo(singleCheckBox& carrier, bool checked)
    {
        // 勾选状态以界面上的封装勾选框为准（触发按钮置灰/关弹窗联动），
        // 隐藏的数据载体同步供 collectProps(singleCheckBox) 判定
        checkBox()->setChecked(checked);
        if (carrier.cb)
            carrier.cb->setChecked(checked);
    }

    void mTempCurveRowWidget::syncCheckedToCarrier(singleCheckBox& carrier, bool checked)
    {
        // 与 syncCheckedTo 等价，但 setChecked 期间屏蔽两个勾选框的信号：
        // 本接口专供界面勾选框的 stateChanged 槽调用，防止程序化同步再次触发
        // stateChanged 形成递归保存（属性块为跨材料复用的单例，会把上一材料的
        // 勾选态带入下一材料的收集结果）
        const QSignalBlocker b1(_cb);
        const QSignalBlocker b2(carrier.cb);
        _cb->setChecked(checked);
        if (carrier.cb)
            carrier.cb->setChecked(checked);
    }

    void mTempCurveRowWidget::setCurveData(const QVector<QVector<double>>& curves)
    {
        _cb->setProperty("curve", QVariant::fromValue(curves));
        if (_curveCtrl)
            _curveCtrl->setCurve(curves);
    }

    void mTempCurveRowWidget::clearCurve()
    {
        _cb->setProperty("curve", QVariant());
        if (_curveCtrl)
            _curveCtrl->clearCurve();
    }

    void mTempCurveRowWidget::setHeaderHasUnit(bool hasUnit)
    {
        _headerHasUnit = hasUnit;
        // 立即应用：按新约定剥离/保留列标题单位后缀
        setCurveHeaders(_curveTitle, _independentHeader, _paramHeaders);
    }

    void mTempCurveRowWidget::setCurveHeaders(const QString& curveTitle,
        const QString& independentHeader,
        const QStringList& paramHeaders)
    {
        _curveTitle = curveTitle;
        _independentHeader = independentHeader;
        _paramHeaders = paramHeaders;
        if (!_curveCtrl) return;
        auto stripUnit = [](const QString& header) -> QString {
            const int idx = header.indexOf(QLatin1Char('/'));
            return idx > 0 ? header.left(idx).trimmed() : header.trimmed();
        };
        if (_headerHasUnit)
        {
            _curveCtrl->setColumnHeaders(_independentHeader, _paramHeaders);
        }
        else
        {
            QStringList params;
            for (const QString& h : _paramHeaders)
                params << stripUnit(h);
            _curveCtrl->setColumnHeaders(stripUnit(_independentHeader), params);
        }
        // 弹窗标题栏跟随曲线标题（2.2.6 层级规则）
        _curveCtrl->setPopupTitle(_curveTitle);
        // 入口按钮摘要跟随曲线标题刷新
        if (QLabel* summary = _curveCtrl->entrySummaryLabel())
            summary->setText(_curveTitle);
    }

    QWidget* mMaterialPropBase::createLibCurveRow(const QString& name,
        const QVector<QVector<double>>& curves,
        const std::function<void(const QVector<QVector<double>>&)>& onDataChanged)
    {
        QWidget* row = new QWidget(this);
        QHBoxLayout* lay = new QHBoxLayout(row);
        lay->setContentsMargins(0, 0, 0, 0);
        lay->setSpacing(4);
        QLabel* label = new QLabel(name, row);
        label->setFixedWidth(90);
        lay->addWidget(label);

        // 列标题与原停靠表约定一致（无单位后缀）；参数列数按实际曲线列数取，
        // 多参数列时以「属性名+序号」区分，避免 Y 轴下拉出现重名
        const int paramCount = qMax(1, curves.size() - 1);
        QStringList paramHeaders;
        for (int i = 0; i < paramCount; ++i)
            paramHeaders << (paramCount > 1
                ? QStringLiteral("%1%2").arg(name).arg(i + 1) : name);

        // 控件本体不入布局：仅承担吸附式弹窗宿主职责，入口按钮剖分进行布局
        mParamCurveControl* ctrl = new mParamCurveControl(name,
            QStringLiteral("温度"), paramHeaders, row);
        ctrl->setAnchorWidget(row);
        if (QPushButton* btn = ctrl->entryButton())
        {
            ctrl->detachEntryButton(row);
            btn->setMinimumHeight(28);
            btn->setMaximumHeight(QWIDGETSIZE_MAX);
            btn->setMinimumWidth(0);
            btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            lay->addWidget(btn, 1); // 拉伸填充至行尾，与上方输入框右缘对齐
        }
        ctrl->hide();
        if (curves.size() >= 2)
            ctrl->setCurve(curves);

        // 编辑产生的数据（列主序，空/非法格以 NaN 占位）按「温度有效 + 该参数
        // 该点有效」逐列过滤后回调宿主写回
        connect(ctrl, &mParamCurveControl::dataChanged, this, [ctrl, onDataChanged]() {
            const QVector<QVector<double>> all = ctrl->curve();
            QVector<QVector<double>> filtered;
            if (all.size() >= 2 && !all[0].isEmpty())
            {
                filtered.append(all[0]); // 自变量列（温度）
                for (int c = 1; c < all.size(); ++c)
                {
                    QVector<double> col;
                    col.reserve(all[0].size());
                    for (int r = 0; r < all[0].size(); ++r)
                    {
                        const double v = (r < all[c].size()) ? all[c][r]
                            : std::numeric_limits<double>::quiet_NaN();
                        if (!std::isnan(v))
                            col.append(v);
                    }
                    filtered.append(col);
                }
            }
            onDataChanged(filtered);
        });
        return row;
    }
    bool mMaterialPropBase::s_unitSyncing = false;
    mMaterialPropBase::mMaterialPropBase(const MxFunctions::MaterialClass matClass, const QString propname, QWidget* parent)
        : FoldSection(propname, parent), m_foldClass(matClass), m_strPropName(propname)
    {
    }

    mMaterialPropBase::~mMaterialPropBase()
    {
    }

    MxFunctions::MaterialClass mMaterialPropBase::getPropertyClass()
    {
        return m_foldClass;
    }

    void mMaterialPropBase::connectUnitSystemResponse()
    {
        auto* modelData = getCVM ? getCVM->getModelData() : nullptr;
        if (modelData == nullptr || modelData->getUnitSystemManager() == nullptr)
        {
            return;
        }
        connect(modelData->getUnitSystemManager(), &CommonData::mUnitSystemManager::sg_unitChanged,
            this, &mMaterialPropBase::slot_unitSystemResponse, Qt::UniqueConnection);
    }

    void mMaterialPropBase::disconnectUnitSystemResponse()
    {
        auto* modelData = getCVM ? getCVM->getModelData() : nullptr;
        if (modelData == nullptr || modelData->getUnitSystemManager() == nullptr)
        {
            return;
        }
        disconnect(modelData->getUnitSystemManager(), &CommonData::mUnitSystemManager::sg_unitChanged,
            this, &mMaterialPropBase::slot_unitSystemResponse);
    }

    void mMaterialPropBase::slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx)
    {
        Q_UNUSED(type);
        Q_UNUSED(idx);
        //默认只刷新单位标签（复用各派生类已实现的 updatePropUnit）；
        //数值需要换算的派生类重写本函数（参考 mDensityProp/mBulkModulusProp：getSwitchUnitSystemRatio 旧→新乘数）
        updatePropUnit();
    }

    QString mMaterialPropBase::getPropName()
    {
        return m_strPropName;
    }

    void mMaterialPropBase::setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop)
    {
        if (prop.isEmpty())
        {
            return;
        }
    }

    void mMaterialPropBase::setInputsValue(const QHash<QString, QPair<QVector<Unit>, QVariant>>& prop)
    {
        if (prop.isEmpty())
        {
            return;
        }
    }

    //编辑表格
    void mMaterialPropBase::tableEdit(bool usingCurve, QString title, QVariant value, QStringList header)
    {
        mTableDataWindow::getInstance()->resetInitialTable();
        mTableDataWindow::getInstance()->setUseCurve(usingCurve);
        mTableDataWindow::getInstance()->setTableTitle(title);

        QMap<int, mTableDataWindow::ColumnValidationType> validations;

        for (int i = 0; i < header.size(); i++)
        {
            validations.insert(i, mTableDataWindow::ColumnValidationType::NonNegativeReal);
        }

        mTableDataWindow::getInstance()->setColumnValidations(validations);

        if (value.isValid())
        {
            QVector<QVector<double>>curves = value.value<QVector<QVector<double>>>();
            QVector<QVector<double>>X;
            QVector<QVector<double>>Y;
            QVector<double>deftmp = { 0 };
            for (int j = 1; j < curves.size(); ++j)
            {
                X.append(curves[0].size() == 0 ? deftmp : curves[0]);
                Y.append(curves[j].size() == 0 ? deftmp : curves[j]);
            }
            mTableDataWindow::getInstance()->setTableData(X, Y);
        }
        else
        {
            QVector<QVector<double>>X;
            QVector<QVector<double>>Y;
            for (int i = 1; i < header.size(); ++i)
            {
                X.append(QVector<double>{ 0 });
                Y.append(QVector<double>{ 0 });
            }
            mTableDataWindow::getInstance()->setTableData(X, Y);
        }
        mTableDataWindow::getInstance()->setHorizontalHeaders(header);

    }

    void mMaterialPropBase::collectProps(QHash<MxFunctions::MaterialPropType, QVariant>& props, const QVector<InputWidgetSet>& inputWidgets)
    {
        for (auto inp : inputWidgets)
        {
            auto proptype = inp.proptype_;
            QVariant value;
            QString text = inp.lineeditvalue->text().trimmed();
            if (!text.isEmpty())
            {
                bool ok;
                double val = text.toDouble(&ok);
                if (ok)
                {
                    value = QVariant::fromValue<double>(val);
                }
                else
                {
                    value = QVariant();
                }
            }
            props[proptype] = value;
        }
    }

    void mMaterialPropBase::collectProps(QHash<MxFunctions::MaterialPropType, QVariant>& props, const QVector<InputWidgetSet>& inputWidgets, const QHash<MxFunctions::MaterialPropType, double>& ratios)
    {
        for (auto inp : inputWidgets)
        {
            auto proptype = inp.proptype_;
            QVariant value;
            double ratio;
            QString text = inp.lineeditvalue->text().trimmed();
            if (!text.isEmpty())
            {
                bool ok;
                double val = text.toDouble(&ok);
                if (ok)
                {
                    ratio = ratios.value(proptype, 1.0);
                    value = QVariant::fromValue<double>(val * ratio);
                    //value = QVariant::fromValue<double>(val / ratio);

                }
                else
                {
                    value = QVariant();
                }
            }
            props[proptype] = value;
        }
    }

    void mMaterialPropBase::collectProps(QHash<QString, QPair<QVector<Unit>, QVariant>>& props, const QVector<InputWidgetSet*>& inputWidgets)
    {
        for (auto inp : inputWidgets)
        {
            auto proptype = inp->proptype_;
            Unit unit = inp->unit_;
            QString text = inp->lineeditvalue->text().trimmed();
            QString name = inp->name_;
            QVariant value;
            if (!text.isEmpty())
            {
                bool ok;
                double val = text.toDouble(&ok);
                if (ok)
                {
                    value = QVariant::fromValue<double>(val);
                }
                else
                {
                    value = QVariant();
                }
                props[name] = { {unit},value };
            }
        }
    }

    void mMaterialPropBase::collectProps(QHash<MxFunctions::MaterialPropType, QVariant>& props, const QVector<InputWithCheckBox>& inputWidgets)
    {
        for (auto inp : inputWidgets)
        {
            if (!inp.checkbox->isChecked())
            {
                continue;
            }
            auto proptype = inp.proptype_;
            QVariant value;
            QString text = inp.lineeditvalue->text().trimmed();
            if (!text.isEmpty())
            {
                bool ok;
                double val = text.toDouble(&ok);
                if (ok)
                {
                    value = QVariant::fromValue<double>(val);
                }
                else
                {
                    value = QVariant();
                }
            }
            props[proptype] = value;
        }
    }

    void mMaterialPropBase::collectProps(QHash<MxFunctions::MaterialPropType, QVariant>& props, singleTableWidget& inputWidget)
    {
        auto proptype = inputWidget.proptype_;
        QVector<QVector<double>> value = inputWidget.collectValue();
        props[proptype] = QVariant::fromValue<QVector<QVector<double>>>(value);
    }

    void mMaterialPropBase::collectProps(QHash<MxFunctions::MaterialPropType, QVariant>& props, const singleCheckBox& inputWidget
        , const CurveType& correlation)
    {
        QVector<QVector<double>>curve;
        if (inputWidget.cb->property("curve").isValid())
        {
            curve = inputWidget.cb->property("curve").value<QVector<QVector<double>>>();
        }
        auto proptype = inputWidget.proptype_;
        props[proptype] = MechMaterialData::mMaterialBase::transCurve2Varint(curve, correlation);
    }

    void mMaterialPropBase::collectProps(QHash<MxFunctions::MaterialPropType, QVariant>& props, const singleLabelBtn& inputWidgets, const CurveType& correlation)
    {
        QVector<QVector<double>>curve;
        if (inputWidgets.tablebtn->property("curve").isValid())
        {
            curve = inputWidgets.tablebtn->property("curve").value<QVector<QVector<double>>>();
        }
        auto proptype = inputWidgets.proptype_;
        props[proptype] = MechMaterialData::mMaterialBase::transCurve2Varint(curve, correlation);
    }

    void mMaterialPropBase::collectProps(QHash<MxFunctions::MaterialPropType, QVariant>& props, const MxFunctions::MaterialPropType& proptype, const QString& singleporp)
    {
        props[proptype] = QVariant::fromValue<QString>(singleporp);
    }

    void mMaterialPropBase::collectProps(QHash<MxFunctions::MaterialPropType, QVariant>& props, const MxFunctions::MaterialPropType& proptype, const int& singleporp)
    {
        props[proptype] = QVariant::fromValue<int>(singleporp);
    }

    QVector<QVector<double>> mMaterialPropBase::getValidTableData(const QVector<QVector<double>>& x, const QVector<QVector<double>>& y)
    {
        //x与y一一对应
        QVector<QVector<double>> curves;
        int idx = 0;
        QSet<double>validX;//有效行数
        if (!x.empty())
        {
            //取最少行数的x与下标
            int minlen = 1e9;
            for (int i = 0; i < x.size(); ++i)
            {
                if (x[i].size() < minlen)
                {
                    minlen = x[i].size();
                    idx = i;
                }
            }
            if (minlen == 1e9)
            {
                return curves;
            }
            for (int i = 0; i < x[idx].size(); ++i)
            {
                validX.insert(x[idx][i]);
            }
            curves.append(x[idx]);
        }
        else
        {
            return curves;
        }
        for (int i = 0; i < y.size(); ++i)
        {
            QVector<double>validY;
            for (int j = 0; j < y[i].size(); ++j)
            {
                if (validX.contains(x[i][j]))
                    validY.append(y[i][j]);
            }
            curves.append(validY);
        }

        return curves;
    }
}
