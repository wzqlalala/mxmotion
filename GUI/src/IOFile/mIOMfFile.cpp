//解决中文乱码
#pragma execution_character_set("utf-8")

#include "miofile_global.h"
#include "mIOMfFile.h"

#include <QSaveFile>
#include <QTextStream>
#include <QStringList>
#include <QDir>
#include <QFileInfo>
#include "mMechGlobalSignals.h"

#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mModelData.h"
#include "mFrpManager.h"
#include "mFrpBaseData.h"
#include "mFrpDerivedData.h"

namespace
{
    const MechFrpData::mFrpBaseData* frpFindGroup(const MechFrpData::mFrpBaseData* root, MxFunctions::FrpNodeType type)
    {
        if (!root) return nullptr;
        for (const MechFrpData::mFrpBaseData* c : root->children)
            if (c->type == type) return c;
        return nullptr;
    }
    // 材料段数值：界面字符串 -> 求解器纯数值；空串/“请输入”/非法值返回空串。
    QString frpMatNumber(const QString& raw)
    {
        const QString s = raw.trimmed();
        if (s.isEmpty() || s == QStringLiteral("请输入"))
            return QString();
        bool ok = false;
        const double v = s.toDouble(&ok);
        return ok ? QString::number(v, 'g', 15) : QString();
    }
    QString frpAnalysisTypeToKw(const QString& t)
    {
        if (t == QStringLiteral("力学分析")) return QStringLiteral("mechanical");
        if (t == QStringLiteral("热-力学分析")) return QStringLiteral("thermomechanical");
        if (t == QStringLiteral("热学分析")) return QStringLiteral("thermal");
        return QString();
    }

    QString frpHomoModelToKw(const QString& m)
    {
        if (m == QStringLiteral("Mori Tanaka")) return QStringLiteral("moritanaka");
        if (m == QStringLiteral("Double Inclusion"))
            return QStringLiteral("doubleinclusion");
        return QString();
    }

    QString frpMultiPhaseToKw(const QString& m)
    {
        if (m == QStringLiteral("Multi-Step")) return QStringLiteral("multistep");
        if (m == QStringLiteral("Multi-Level")) return QStringLiteral("multilevel");
        return QString();
    }

    static const char* const kFrpFailureKw[9] = {
        "maximum", "tsaihill2d", "tsaihill3dtrans", "tsaihill3d", "azzitsaihill2d",
        "tsaiwu2d", "tsaiwu3dtrans", "tsaiwu3d", "multicomponent2d"
    };
    static const char* const kFrpFailureText[9] = {
        "Maximum", "Tsai Hill 2D", "Tsai Hill 3D Transversely", "Tsai Hill 3D",
        "Azzi Tsai Hill 2D", "Tsai Wu 2D", "Tsai Wu 3D Transversely",
        "Tsai Wu 3D", "Multi Component 2D"
    };

    int frpFailureIndexFromText(const QString& text)
    {
        for (int i = 0; i < 9; ++i)
            if (text == QString::fromLatin1(kFrpFailureText[i])) return i;
        return -1;
    }

    // —— 纤维取向子关键字（defMethod：1 固定角度 / 2 取向张量 / 3 张量文件；0/4/5 无关键字） ——
    void frpWriteOrientation(QTextStream& out, int defMethod,
        const QString& theta, const QString& phi, const QString& tensor,
        const QString& tensorFilePath)
    {
        if (defMethod == 1)
        {
            out << QStringLiteral("*orientation, type = fixed") << '\n';
            out << theta << ", " << phi << '\n';
        }
        else if (defMethod == 2)
        {
            const QStringList t =
                MechFrpData::mFrpManager::reorderTensor6ToAbaqus(tensor.split(QLatin1Char(';')));
            out << QStringLiteral("*orientation, type = tensor") << '\n';
            for (int i = 0; i < 6; ++i)
                out << (i ? ", " : "") << t.value(i);
            out << '\n';
        }
        else if (defMethod == 3)
        {
            out << QStringLiteral("*orientation, type = distribution, file = \"%1\"").arg(tensorFilePath) << '\n';
        }
        else if (defMethod == 4)  // 2D随机：二维平均取向，无参数行
        {
            out << QStringLiteral("*orientation, type = random2d") << '\n';
        }
        else if (defMethod == 5)  // 3D随机：三维平均取向，无参数行
        {
            out << QStringLiteral("*orientation, type = random3d") << '\n';
        }
    }

    // 解析相分数：空串按 0；非法数字 ok=false
    double frpParseFraction(const QString& s, bool& ok)
    {
        const QString t = s.trimmed();
        if (t.isEmpty()) { ok = true; return 0.0; }
        bool good = false;
        const double v = t.toDouble(&good);
        ok = good;
        return v;
    }

    // 校验某微观结构N：规则B（frac 类型一致）、分数合法、规则A（基体值=1-其余和，非负）。
    // 通过返回 true；matrixFracKind/matrixFracValue 回传供导出，失败 outErr 填具体原因。
    bool frpValidateMicro(const MechFrpData::mFrpBaseData* micro,
        int& matrixFracKind, double& matrixFracValue, QString& outErr)
    {
        int fracKind = -1;
        double sum = 0.0;
        bool hasNonMatrix = false;
        for (const MechFrpData::mFrpBaseData* c : micro->children)
        {
            const MechFrpData::mFrpPhaseData* p = c->as<MechFrpData::mFrpPhaseData>();
            if (!p) continue;
            int kind = -1; QString fracText;
            if (c->type == FrpNode_Inclusion) { kind = p->inc.ratioKind; fracText = p->inc.ratioValue; }
            else if (c->type == FrpNode_Fiber) { kind = p->fib.ratioKind; fracText = p->fib.ratioValue; }
            else if (c->type == FrpNode_Cavity) { kind = 1; fracText = p->cav.volumeFraction; }
            else continue;                          // 基体不在此处理
            if (fracKind < 0) fracKind = kind;
            else if (kind != fracKind)
            {
                outErr = QStringLiteral("微观结构“%1”：各相比例类型不一致（质量分数/体积分数混用）").arg(micro->name);
                return false;
            }
            bool ok = false;
            const double v = frpParseFraction(fracText, ok);
            if (!ok)
            {
                outErr = QStringLiteral("微观结构“%1”中相“%2”的分数输入不合法").arg(micro->name, c->name);
                return false;
            }
            sum += v;
            // 高级特征额外占比也从基体分数中扣除：
            QString extraText;
            if (c->type == FrpNode_Inclusion && p->inc.advanced && p->inc.featureType == 1)
                extraText = p->inc.fiberFrac1;                 // 掺杂-纤维族：纤维簇分数
            else if (c->type == FrpNode_Cavity && p->cav.surface)
                extraText = p->cav.surfaceFrac;                // 空腔-表层定义：表层体积分数
            else if (c->type == FrpNode_Fiber && p->fib.coating)
                extraText = p->fib.coatFrac;                   // 连续纤维-涂层定义：涂层质量分数
            if (!extraText.isNull())
            {
                bool eok = false;
                const double ev = frpParseFraction(extraText, eok);
                if (!eok)
                {
                    outErr = QStringLiteral("微观结构“%1”中相“%2”的高级特征分数输入不合法")
                        .arg(micro->name, c->name);
                    return false;
                }
                sum += ev;
            }
            hasNonMatrix = true;
        }
        matrixFracKind = hasNonMatrix ? fracKind : 0;   // 只有基体时默认 mass
        matrixFracValue = 1.0 - sum;
        if (matrixFracValue < 0.0)
        {
            outErr = QStringLiteral("微观结构“%1”：基体分数 = 1 - 其余相分数之和 = %2，为负值，请调整")
                .arg(micro->name).arg(matrixFracValue);
            return false;
        }
        return true;
    }

    // 单个均质化模型导出（排法一：phase 在前，microstructure + 相名列表在后）
    QString frpExportOne(const MechFrpData::mFrpBaseData* r, QString& err)
    {
        const MechFrpData::mFrpAnalysisData* a = r->as<MechFrpData::mFrpAnalysisData>();
        if (!a) { err = QStringLiteral("无效的均质化模型"); return QString(); }
        const QString typeKw = frpAnalysisTypeToKw(a->analysisType);
        const QString modelKw = frpHomoModelToKw(a->homoModel);
        if (typeKw.isEmpty() || modelKw.isEmpty())
        {
            err = QStringLiteral("模型“%1”：未完成“类型/均质化模型”必选项").arg(r->name);
            return QString();
        }

        QString result;
        QTextStream out(&result);

        out << QStringLiteral("*implicit_analysis, type = meanfield") << '\n';

        out << QStringLiteral("*rveanalysis, type = %1, model = %2").arg(typeKw, modelKw);
        const QString homoKw = frpMultiPhaseToKw(a->multiPhase);
        if (!homoKw.isEmpty())
            out << QStringLiteral(", homogenize = %1").arg(homoKw);
        out << '\n';

        // —— 材料属性 ——
        if (const MechFrpData::mFrpBaseData* matGrp = frpFindGroup(r, FrpNode_Material))
        {
            for (const MechFrpData::mFrpBaseData* matNode : matGrp->children)
            {
                const MechFrpData::mFrpMaterialData* mt = matNode->as<MechFrpData::mFrpMaterialData>();
                if (!mt) continue;
                const QString matKindKw = (mt->m_kind == QStringLiteral("线弹性"))
                    ? QStringLiteral("isolinearelastic") : QStringLiteral("heat");
                out << QStringLiteral("*material, type = %1, name = \"%2\"")
                    .arg(matKindKw, matNode->name) << '\n';
                out << frpMatNumber(mt->young) << ", " << frpMatNumber(mt->poisson) << '\n';
                const QString density = frpMatNumber(mt->density);
                if (!density.isEmpty())
                {
                    out << QStringLiteral("*density") << '\n';
                    out << density << '\n';
                }
                if (mt->thermalExp)
                {
                    const QString refTemp = frpMatNumber(mt->refTemp);
                    out << QStringLiteral("*expansion, type = iso, reftemp = %1")
                        .arg(refTemp.isEmpty() ? QStringLiteral("0.0") : refTemp) << '\n';
                    out << frpMatNumber(mt->a1) << '\n';
                }
            }
        }

        // —— 微观结构（排法一）——
        if (const MechFrpData::mFrpBaseData* microGrp = frpFindGroup(r, FrpNode_Micro))
        {
            for (const MechFrpData::mFrpBaseData* micro : microGrp->children)
            {
                int matrixFracKind = 0;
                double matrixFracValue = 0.0;
                QString verr;
                if (!frpValidateMicro(micro, matrixFracKind, matrixFracValue, verr))
                {
                    err = verr;
                    return QString();
                }

                // 1) 该微观结构全部 phase
                for (const MechFrpData::mFrpBaseData* phNode : micro->children)
                {
                    const MechFrpData::mFrpPhaseData* p = phNode->as<MechFrpData::mFrpPhaseData>();
                    if (!p) continue;
                    switch (phNode->type)
                    {
                    case FrpNode_Matrix:
                    {
                        const QString fracKw = matrixFracKind == 1 ? QStringLiteral("volume") : QStringLiteral("mass");
                        out << QStringLiteral("*phase, type = matrix, frac = %1, mat = \"%2\", name = \"%3\"")
                            .arg(fracKw, p->baseMaterial, phNode->name) << '\n';
                        out << QString::number(matrixFracValue, 'g', 15) << '\n';
                        break;
                    }
                    case FrpNode_Inclusion:
                    {
                        const MechFrpData::mFrpPhaseData::Inc& d = p->inc;
                        out << QStringLiteral("*phase, type = inclusion, frac = %1, mat = \"%2\", name = \"%3\"")
                            .arg(QLatin1String(d.ratioKind == 1 ? "volume" : "mass"), d.material, phNode->name);
                        if (d.shape == 1)
                            out << QStringLiteral(", distribution = \"%1\"").arg(d.distFuncName);
                        out << '\n';
                        if (d.shape == 1)
                            out << d.ratioValue << ", " << d.discreteCount << '\n';
                        else
                            out << d.ratioValue << ", " << d.aspect << '\n';
                        frpWriteOrientation(out, d.defMethod, d.theta, d.phi, d.tensor, d.tensorFilePath);
                        if (d.advanced)
                        {
                            if (d.featureType == 0)
                            {
                                out << QStringLiteral("*coated, mat = \"%1\"").arg(d.featureMat) << '\n';
                                out << d.featureFrac << '\n';
                            }
                            else
                            {
                                out << QStringLiteral("*clustering") << '\n';
                                out << d.featureAspect << ", " << d.fiberFrac1 << ", " << d.fiberFrac2 << '\n';
                            }
                        }
                        break;
                    }
                    case FrpNode_Cavity:
                    {
                        const MechFrpData::mFrpPhaseData::Cav& c = p->cav;
                        out << QStringLiteral("*phase, type = void, frac = volume, name = \"%1\"")
                            .arg(phNode->name);
                        if (c.shape == 1)
                            out << QStringLiteral(", distribution = \"%1\"").arg(c.distFuncName);
                        out << '\n';
                        if (c.shape == 1)
                            out << c.volumeFraction << ", " << c.discreteCount << '\n';
                        else
                            out << c.volumeFraction << ", " << c.aspect << '\n';
                        frpWriteOrientation(out, c.defMethod, c.theta, c.phi, c.tensor, c.tensorFilePath);
                        if (c.surface)
                        {
                            out << QStringLiteral("*coated, mat = \"%1\"").arg(c.surfaceMat) << '\n';
                            out << c.surfaceFrac << '\n';
                        }
                        break;
                    }
                    case FrpNode_Fiber:
                    {
                        const MechFrpData::mFrpPhaseData::Fib& f = p->fib;
                        out << QStringLiteral("*phase, type = continuous, frac = %1, mat = \"%2\", name = \"%3\"")
                            .arg(QLatin1String(f.ratioKind == 1 ? "volume" : "mass"), f.material, phNode->name) << '\n';
                        out << f.ratioValue << '\n';
                        // 纤维方向（theta/phi）均未填值：不导出 *orientation 关键字及内容
                        if (!f.theta.trimmed().isEmpty() || !f.phi.trimmed().isEmpty())
                            frpWriteOrientation(out, 1, f.theta, f.phi, QString(), QString());
                        if (f.coating)
                        {
                            out << QStringLiteral("*coated, mat = \"%1\"").arg(f.coatMat) << '\n';
                            out << f.coatFrac << '\n';
                        }
                        break;
                    }
                    default:
                        break;
                    }
                }

                // 2) microstructure + 相名列表
                out << QStringLiteral("*microstructure, name = \"%1\"").arg(micro->name) << '\n';
                bool first = true;
                for (const MechFrpData::mFrpBaseData* phNode : micro->children)
                {
                    if (!first) out << QStringLiteral(", ");
                    out << QStringLiteral("\"%1\"").arg(phNode->name);
                    first = false;
                }
                out << '\n';
            }
        }

        // —— RVE ——
        if (const MechFrpData::mFrpBaseData* rveNode = frpFindGroup(r, FrpNode_Rve))
        {
            if (const MechFrpData::mFrpRveData* rv = rveNode->as<MechFrpData::mFrpRveData>())
            {
                if (rv->mode == QStringLiteral("多层分析"))
                {
                    out << QStringLiteral("*rve, type = multilayer, name = \"%1\"").arg(rveNode->name) << '\n';
                    for (const MechFrpData::RveLayer& l : rv->layers)
                    {
                        if (l.micro.isEmpty()) continue;
                        out << QStringLiteral("\"%1\", %2, %3").arg(l.micro, l.thick, l.angle);
                        if (l.orient == QStringLiteral("固定角度"))
                        {
                            const QStringList v = l.value.split(QLatin1Char(';'));
                            out << ", " << v.value(0) << ", " << v.value(1);
                        }
                        else if (l.orient == QStringLiteral("Random 2D"))
                            out << ", Random2D";
                        else if (l.orient == QStringLiteral("Random 3D"))
                            out << ", Random3D";
                        else if (l.orient == QStringLiteral("固定取向张量"))
                        {
                            const QStringList v =
                                MechFrpData::mFrpManager::reorderTensor6ToAbaqus(l.value.split(QLatin1Char(';')));
                            for (int i = 0; i < 6; ++i)
                                out << ", " << v.value(i);
                        }
                        out << '\n';
                    }
                }
                else
                {
                    out << QStringLiteral("*rve, type = classical, microstructure = \"%1\", name = \"%2\"")
                        .arg(rv->structure, rveNode->name) << '\n';
                }
            }
        }

        // —— 失效准则 ——
        if (const MechFrpData::mFrpBaseData* failGrp = frpFindGroup(r, FrpNode_Failure))
        {
            for (const MechFrpData::mFrpBaseData* fnNode : failGrp->children)
            {
                const MechFrpData::mFrpFailureData* f = fnNode->as<MechFrpData::mFrpFailureData>();
                if (!f) continue;
                const int mi = frpFailureIndexFromText(f->model);
                if (mi < 0) continue;
                const bool hasType = (mi != 4 && mi != 7 && mi != 8);
                out << QStringLiteral("*failure, indicator = %1, level = composite, name = \"%2\"")
                    .arg(QLatin1String(kFrpFailureKw[mi]), fnNode->name);
                if (hasType)
                    out << QStringLiteral(", type = %1")
                    .arg(f->failType == QStringLiteral("Strain") ? QLatin1String("strain") : QLatin1String("stress"));
                out << '\n';
                switch (mi)
                {
                case 0:
                    out << ((f->compCriterion.isEmpty() || f->compCriterion == QStringLiteral("无"))
                        ? QString() : f->compCriterion)
                        << ", " << f->tens << ", " << f->comp2 << '\n';
                    break;
                case 1:
                    out << f->axialTens << ", " << f->axialComp << ", "
                        << f->inPlaneTens << ", " << f->inPlaneComp << ", " << f->shear << '\n';
                    break;
                case 2:
                    out << f->axialTensT << ", " << f->inPlaneTensT << ", " << f->shearT << '\n';
                    break;
                case 3:
                    out << f->xt << ", " << f->yt << ", " << f->zt << ", "
                        << f->xys << ", " << f->yzs << ", " << f->xzs << '\n';
                    break;
                case 4:
                    out << f->azAxTens << ", " << f->azAxComp << ", "
                        << f->azIpTens << ", " << f->azIpComp << ", " << f->azShear << '\n';
                    break;
                case 5:
                    out << f->wuAxTens << ", " << f->wuAxComp << ", "
                        << f->wuIpTens << ", " << f->wuIpComp << ", "
                        << f->wuShear << ", " << f->wuCouple << '\n';
                    break;
                case 6:
                    out << f->wuTAxialTens << ", " << f->wuTAxialComp << ", "
                        << f->wuTInPlaneTens << ", " << f->wuTInPlaneComp << ", " << f->wuTShear << '\n';
                    break;
                case 7:
                {
                    const QStringList v = f->wu3d.split(QLatin1Char(';'));
                    out << v.value(0) << ", " << v.value(1) << ", " << v.value(2)
                        << ", " << v.value(3) << ", " << v.value(4) << ", " << v.value(5) << '\n';
                    for (int i = 6; i <= 26; ++i)
                        out << (i > 6 ? ", " : "") << v.value(i);
                    out << '\n';
                    break;
                }
                case 8:
                    out << f->mcAxialTens << ", " << f->mcAxialComp << ", "
                        << f->mcInPlaneTens << ", " << f->mcInPlaneComp << ", " << f->mcShear << '\n';
                    break;
                default:
                    break;
                }
            }
        }

        // —— 长细比函数 ——
        if (const MechFrpData::mFrpBaseData* funGrp = frpFindGroup(r, FrpNode_AspectRatio))
        {
            for (const MechFrpData::mFrpBaseData* fnNode : funGrp->children)
            {
                const MechFrpData::mFrpAspectRatioData* f = fnNode->as<MechFrpData::mFrpAspectRatioData>();
                if (!f) continue;
                out << QStringLiteral("*ratiofunction, type = linear, name = \"%1\"").arg(fnNode->name) << '\n';
                for (const auto& pt : f->rows)
                    out << pt.first << ", " << pt.second << '\n';
            }
        }

        out << QStringLiteral("*end*") << '\n';
        out.flush();
        err.clear();
        return result;
    }

    // 把已生成的 mf 文本原子写入 path；失败返回 false 且 err 说明
    bool frpWriteFile(const QString& path, const QString& text, QString& err)
    {
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            err = QStringLiteral("文件无法写入"); return false;
        }
        {
            QTextStream o(&file);
            o.setCodec("UTF-8");
            o << text;
            o.flush();
            if (o.status() != QTextStream::Ok)
            {
                err = QStringLiteral("写入出错"); return false;
            }
        }
        if (!file.commit())
        {
            err = QStringLiteral("文件提交失败"); return false;
        }
        return true;
    }


    QStringList frpSplitCsv(const QString& s)
    {
        QStringList out;
        QString cur;
        bool inQ = false;
        for (const QChar c : s)
        {
            if (c == QLatin1Char('"')) { inQ = !inQ; continue; }
            if (c == QLatin1Char(',') && !inQ) { out << cur.trimmed(); cur.clear(); continue; }
            cur += c;
        }
        out << cur.trimmed();
        return out;
    }

    // 解析 * 头行：body 去前导 *，首段为关键字（转小写），其余按 attr = value 入 attrs（键转小写）
    void frpParseHead(const QString& ln, QString& kw, QMap<QString, QString>& attrs)
    {
        attrs.clear();
        QString body = ln;
        if (body.startsWith(QLatin1Char('*'))) body = body.mid(1);
        if (body == QLatin1String("end*")) { kw = QStringLiteral("*end*"); return; }
        const QStringList parts = frpSplitCsv(body);
        if (parts.isEmpty()) { kw.clear(); return; }
        kw = parts.first().trimmed().toLower();
        for (int i = 1; i < parts.size(); ++i)
        {
            const int eq = parts[i].indexOf(QLatin1Char('='));
            if (eq <= 0) continue;
            attrs.insert(parts[i].left(eq).trimmed().toLower(), parts[i].mid(eq + 1).trimmed());
        }
    }

    MechFrpData::mFrpBaseData* frpFindGroup(MechFrpData::mFrpBaseData* root, MxFunctions::FrpNodeType type)
    {
        if (!root) return nullptr;
        for (MechFrpData::mFrpBaseData* c : root->children)
            if (c->type == type) return c;
        return nullptr;
    }

    QString frpAnalysisTypeFromKw(const QString& k)
    {
        if (k == QLatin1String("mechanical")) return QStringLiteral("力学分析");
        if (k == QLatin1String("thermomechanical")) return QStringLiteral("热-力学分析");
        if (k == QLatin1String("thermal")) return QStringLiteral("热学分析");
        return QString();
    }

    // —— 均质化模型（注意 UI 现有拼写 "Dounle Inclusion"，反向映射必须与之完全一致，否则反显失败） ——
    QString frpHomoModelFromKw(const QString& k)
    {
        if (k == QLatin1String("moritanaka")) return QStringLiteral("Mori Tanaka");
        if (k == QLatin1String("doubleinclusion")) return QStringLiteral("Double Inclusion");
        return QString();
    }

    // —— 多相均质化 ——
    QString frpMultiPhaseFromKw(const QString& k)
    {
        if (k == QLatin1String("multistep")) return QStringLiteral("Multi-Step");
        if (k == QLatin1String("multilevel")) return QStringLiteral("Multi-Level");
        return QStringLiteral("无");
    }

    int frpFailureIndexFromKw(const QString& kw)
    {
        for (int i = 0; i < 9; ++i)
            if (kw == QLatin1String(kFrpFailureKw[i])) return i;
        return -1;
    }
    QString frpFailureTextFromIndex(int i)
    {
        return (i >= 0 && i < 9) ? QString::fromLatin1(kFrpFailureText[i]) : QString();
    }

    // ==================================================================================
    // 导入：.mf 关键字文本 -> candidate（临时 manager）；失败返回 false 且 err 说明原因
    // ==================================================================================
        // phase 可能先于 microstructure 出现（排法一/二），先缓存其完整数据，
    // 遇到 microstructure 的相名列表行再按名实例化
    struct PendingPhase
    {
        MxFunctions::FrpNodeType nt = FrpNode_None;
        QString name, mat;
        int fracKind = 0;
        bool isDist = false;
        QString distName;
        QStringList dataTokens;
        int orientDef = -1;          // -1=无 orientation
        QString orientFile;
        QStringList orientTokens;
        bool coated = false;
        QString coatMat, coatFrac;
        bool clustering = false;
        QStringList clusterTokens;
    };

    // 把缓存的 orientation 数据写入相字段（掺杂/空腔共用）
    void frpApplyPendingOrientation(int& defMethod, QString& theta, QString& phi,
        QString& tensor, QString& tensorFilePath, const PendingPhase& pp)
    {
        defMethod = pp.orientDef < 0 ? 0 : pp.orientDef;
        if (pp.orientDef == 3)
            tensorFilePath = pp.orientFile;
        else if (pp.orientDef == 1 && pp.orientTokens.size() >= 2)
        {
            theta = pp.orientTokens[0];
            phi = pp.orientTokens[1];
        }
        else if (pp.orientDef == 2 && pp.orientTokens.size() >= 6)
        {
            tensor = MechFrpData::mFrpManager::reorderTensor6FromAbaqus(pp.orientTokens.mid(0, 6))
                .join(QLatin1Char(';')) + QLatin1Char(';');
        }
    }

    // 在 micro 下按 pp 创建相节点并填充全部字段；失败（如重复基体）返回 nullptr
    MechFrpData::mFrpBaseData* frpInstantiatePhase(MechFrpData::mFrpManager& mgr,
        MechFrpData::mFrpBaseData* micro, const PendingPhase& pp, QStringList& warnings)
    {
        MechFrpData::mFrpBaseData* node = mgr.addChildOfType(micro, pp.nt);
        if (!node)
        {
            warnings << QStringLiteral("微观结构“%1”存在重复基体，已忽略后续基体").arg(micro->name);
            return nullptr;
        }
        if (!pp.name.isEmpty()) { node->name = pp.name;  }
        MechFrpData::mFrpPhaseData* p = node->as<MechFrpData::mFrpPhaseData>();
        if (!p) return node;

        if (pp.nt == FrpNode_Matrix)
        {
            p->baseMaterial = pp.mat;   // 基体分数实时计算、不落库，数据行忽略
        }
        else if (pp.nt == FrpNode_Inclusion)
        {
            p->inc.material = pp.mat;
            p->inc.ratioKind = pp.fracKind;
            p->inc.shape = pp.isDist ? 1 : 0;
            p->inc.distFuncName = pp.isDist ? pp.distName : QString();
            if (pp.dataTokens.size() >= 1) p->inc.ratioValue = pp.dataTokens[0];
            if (pp.dataTokens.size() >= 2)
            {
                if (pp.isDist) p->inc.discreteCount = pp.dataTokens[1];
                else p->inc.aspect = pp.dataTokens[1];
            }
            frpApplyPendingOrientation(p->inc.defMethod, p->inc.theta, p->inc.phi,
                p->inc.tensor, p->inc.tensorFilePath, pp);
            if (pp.coated)
            {
                p->inc.advanced = true;
                p->inc.featureType = 0;
                p->inc.featureMat = pp.coatMat;
                p->inc.featureFrac = pp.coatFrac;
            }
            else if (pp.clustering)
            {
                p->inc.advanced = true;
                p->inc.featureType = 1;
                if (pp.clusterTokens.size() >= 1) p->inc.featureAspect = pp.clusterTokens[0];
                if (pp.clusterTokens.size() >= 2) p->inc.fiberFrac1 = pp.clusterTokens[1];
                if (pp.clusterTokens.size() >= 3) p->inc.fiberFrac2 = pp.clusterTokens[2];
            }
        }
        else if (pp.nt == FrpNode_Cavity)
        {
            p->cav.shape = pp.isDist ? 1 : 0;
            p->cav.distFuncName = pp.isDist ? pp.distName : QString();
            if (pp.dataTokens.size() >= 1) p->cav.volumeFraction = pp.dataTokens[0];
            if (pp.dataTokens.size() >= 2)
            {
                if (pp.isDist) p->cav.discreteCount = pp.dataTokens[1];
                else p->cav.aspect = pp.dataTokens[1];
            }
            frpApplyPendingOrientation(p->cav.defMethod, p->cav.theta, p->cav.phi,
                p->cav.tensor, p->cav.tensorFilePath, pp);
            if (pp.coated)
            {
                p->cav.surface = true;
                p->cav.surfaceMat = pp.coatMat;
                p->cav.surfaceFrac = pp.coatFrac;
            }
        }
        else if (pp.nt == FrpNode_Fiber)
        {
            p->fib.material = pp.mat;
            p->fib.ratioKind = pp.fracKind;
            if (pp.dataTokens.size() >= 1) p->fib.ratioValue = pp.dataTokens[0];
            if (pp.orientDef == 1 && pp.orientTokens.size() >= 2)
            {
                p->fib.theta = pp.orientTokens[0];
                p->fib.phi = pp.orientTokens[1];
            }
            if (pp.coated)
            {
                p->fib.coating = true;
                p->fib.coatMat = pp.coatMat;
                p->fib.coatFrac = pp.coatFrac;
            }
        }
        return node;
    }

    bool frpImportComposite(const QString& content, MechFrpData::mFrpManager& candidate,
        QString& err, QStringList& warnings, const QString& rootNameHint = QString())
    {
        QStringList lines;
        for (const QString& raw : content.split(QLatin1Char('\n')))
        {
            const QString t = raw.trimmed();
            if (!t.isEmpty())
                lines << t;
        }
        if (lines.size() > 200000)
        {
            err = QStringLiteral("文件行数超过上限（200000 行）");
            return false;
        }

        // 文件头必须是固定的 *implicit_analysis, type = meanfield
        if (lines.isEmpty())
        {
            err = QStringLiteral("导入失败：文件为空");
            return false;
        }
        {
            QString hkw; QMap<QString, QString> hattrs;
            frpParseHead(lines.first(), hkw, hattrs);
            if (hkw != QLatin1String("implicit_analysis")
                || hattrs.value(QStringLiteral("type")) != QLatin1String("meanfield"))
            {
                err = QStringLiteral("文件头必须是 *implicit_analysis, type = meanfield，无法导入");
                return false;
            }
        }

        MechFrpData::mFrpBaseData* curRoot = nullptr;
        MechFrpData::mFrpBaseData* legacyMicro = nullptr;      // 旧格式：已开放、phase 立即挂入的 micro
        MechFrpData::mFrpMaterialData* curMaterial = nullptr;
        bool sawAnalysis = false;
        QMap<QString, PendingPhase> pending;                  // 新格式：按相名缓存
        PendingPhase* curPending = nullptr;                   // 最近缓存相（接收 orientation 等）
        MechFrpData::mFrpPhaseData* curPhaseNode = nullptr;   // 旧格式：最近实例化相

        auto groupOf = [&](MxFunctions::FrpNodeType t) -> MechFrpData::mFrpBaseData* {
            return frpFindGroup(curRoot, t);
            };
        // 下一行若为数值行（非 * 开头），推进并返回该行；否则返回空串
        auto nextDataLine = [&](int& idx) -> QString {
            if (idx + 1 < lines.size() && !lines.at(idx + 1).startsWith(QLatin1Char('*')))
            {
                ++idx;
                return lines.at(idx);
            }
            return QString();
            };
        auto setRenamed = [](MechFrpData::mFrpBaseData* n, const QString& name) {
            if (n && !name.isEmpty()) { n->name = name; n->renamed = true; }
            };

        // 容忍 rveanalysis 靠后：首个实质关键字出现时若根还没建，先懒建，
        // 待 rveanalysis 行再补 analysisType/homoModel/multiPhase
        auto ensureRoot = [&]() -> MechFrpData::mFrpBaseData* {
            if (!curRoot)
            {
                curRoot = candidate.addAnalysisRoot();
                if (!rootNameHint.isEmpty())   // 以导入的 mf 文件名为根节点名
                {
                    curRoot->name = rootNameHint;
                    curRoot->renamed = true;
                }
                sawAnalysis = true;
            }
            return curRoot;
            };

        for (int idx = 0; idx < lines.size(); ++idx)
        {
            const QString ln = lines.at(idx);
            if (!ln.startsWith(QLatin1Char('*')))
                continue;
            QString kw;
            QMap<QString, QString> attrs;
            frpParseHead(ln, kw, attrs);
            if (kw == QLatin1String("*end*"))
                break;

            if (kw == QLatin1String("rveanalysis"))
            {
                const bool rootExisted = (curRoot != nullptr);
                ensureRoot();
                MechFrpData::mFrpAnalysisData* a = curRoot->as<MechFrpData::mFrpAnalysisData>();
                a->analysisType = frpAnalysisTypeFromKw(attrs.value(QStringLiteral("type")));
                a->homoModel = frpHomoModelFromKw(attrs.value(QStringLiteral("model")));
                a->multiPhase = frpMultiPhaseFromKw(attrs.value(QStringLiteral("homogenize")));
                if (!rootExisted)   // 仅新建模型根时重置阶段状态；懒建根补属性时不清 pending
                {
                    legacyMicro = nullptr;
                    curPending = nullptr;
                    curPhaseNode = nullptr;
                    curMaterial = nullptr;
                    pending.clear();
                }
                sawAnalysis = true;
                continue;
            }
            if (kw == QLatin1String("implicit_analysis"))
                continue;   // 文件头已在循环前校验，这里跳过，不触发懒建根
            ensureRoot();   // 容忍 rveanalysis 靠后：首个实质关键字懒建模型根

            if (kw == QLatin1String("material"))
            {
                curMaterial = nullptr;
                MechFrpData::mFrpBaseData* matGrp = groupOf(FrpNode_Material);
                if (!matGrp)
                    continue;
                MechFrpData::mFrpBaseData* matNode = candidate.addChildOfType(matGrp, FrpNode_Material);
                if (!matNode)
                    continue;
                curMaterial = matNode->as<MechFrpData::mFrpMaterialData>();
                if (curMaterial)
                {
                    // 材料种类反向映射：heat -> 热学，其余（isolinearelastic/缺省）-> 线弹性
                    const QString typeKw = attrs.value(QStringLiteral("type")).toLower();
                    curMaterial->m_kind = (typeKw == QLatin1String("heat"))
                        ? QStringLiteral("热学") : QStringLiteral("线弹性");
                    curMaterial->matType = QStringLiteral("各向同性");
                    setRenamed(matNode, attrs.value(QStringLiteral("name")));
                    // 首数据行：杨氏模量 E, 泊松比 nu（空字段不覆盖默认空串）
                    const QStringList etok = frpSplitCsv(nextDataLine(idx));
                    if (etok.size() >= 1 && !etok[0].trimmed().isEmpty())
                        curMaterial->young = etok[0].trimmed();
                    if (etok.size() >= 2 && !etok[1].trimmed().isEmpty())
                        curMaterial->poisson = etok[1].trimmed();
                }
                continue;
            }
            if (kw == QLatin1String("density"))
            {
                if (curMaterial)
                {
                    const QStringList tok = frpSplitCsv(nextDataLine(idx));
                    if (tok.size() >= 1 && !tok[0].trimmed().isEmpty())
                        curMaterial->density = tok[0].trimmed();
                }
                continue;
            }
            if (kw == QLatin1String("expansion"))
            {
                if (curMaterial)
                {
                    curMaterial->thermalExp = true;
                    curMaterial->thermalExpType = 0;
                    const QString rt = attrs.value(QStringLiteral("reftemp"));
                    curMaterial->refTemp = rt.isEmpty() ? QStringLiteral("0.0") : rt;
                    // 下一数据行：各向同性热膨胀系数 A1
                    const QStringList tok = frpSplitCsv(nextDataLine(idx));
                    if (tok.size() >= 1 && !tok[0].trimmed().isEmpty())
                        curMaterial->a1 = tok[0].trimmed();
                }
                continue;
            }
            if (kw == QLatin1String("microstructure"))
            {
                MechFrpData::mFrpBaseData* grp = groupOf(FrpNode_Micro);
                MechFrpData::mFrpBaseData* micro =
                    grp ? candidate.addChildOfType(grp, FrpNode_Micro1) : nullptr;
                if (micro) setRenamed(micro, attrs.value(QStringLiteral("name")));

                // 下一非*行存在 = 相名列表（排法一/二）；否则为旧格式
                if (micro && idx + 1 < lines.size()
                    && !lines.at(idx + 1).startsWith(QLatin1Char('*')))
                {
                    ++idx;   // 消费相名列表行
                    const QStringList names = frpSplitCsv(lines.at(idx));
                    for (QString nm : names)
                    {
                        nm = nm.trimmed();
                        auto it = pending.find(nm);
                        if (it == pending.end())
                        {
                            warnings << QStringLiteral("微观结构“%1”引用的相“%2”未定义，已跳过")
                                .arg(micro->name, nm);
                            continue;
                        }
                        frpInstantiatePhase(candidate, micro, it.value(), warnings);
                        pending.erase(it);
                    }
                    legacyMicro = nullptr;
                }
                else
                {
                    legacyMicro = micro;   // 旧格式：后续 phase 立即挂入
                }
                curPending = nullptr;
                curPhaseNode = nullptr;
            }
            else if (kw == QLatin1String("phase"))
            {
                curPending = nullptr;
                curPhaseNode = nullptr;
                MxFunctions::FrpNodeType nt = FrpNode_None;
                const QString t = attrs.value(QStringLiteral("type"));
                if (t == QLatin1String("matrix")) nt = FrpNode_Matrix;
                else if (t == QLatin1String("inclusion")) nt = FrpNode_Inclusion;
                else if (t == QLatin1String("void")) nt = FrpNode_Cavity;
                else if (t == QLatin1String("continuous")) nt = FrpNode_Fiber;
                if (nt == FrpNode_None) continue;

                PendingPhase pp;
                pp.nt = nt;
                pp.name = attrs.value(QStringLiteral("name"));
                pp.mat = attrs.value(QStringLiteral("mat"));
                pp.fracKind = (attrs.value(QStringLiteral("frac")) == QLatin1String("volume")) ? 1 : 0;
                pp.isDist = attrs.contains(QStringLiteral("distribution"));
                pp.distName = attrs.value(QStringLiteral("distribution"));
                pp.dataTokens = frpSplitCsv(nextDataLine(idx));

                if (legacyMicro)   // 旧格式：micro 已开放，立即实例化
                {
                    MechFrpData::mFrpBaseData* node =
                        frpInstantiatePhase(candidate, legacyMicro, pp, warnings);
                    curPhaseNode = node ? node->as<MechFrpData::mFrpPhaseData>() : nullptr;
                }
                else               // 新格式：按名缓存，等 microstructure 列表组装
                {
                    pending[pp.name] = pp;
                    curPending = &pending[pp.name];
                }
            }
            else if (kw == QLatin1String("orientation"))
            {
                const QString t = attrs.value(QStringLiteral("type"));
                int def = 0;
                if (t == QLatin1String("fixed")) def = 1;
                else if (t == QLatin1String("tensor")) def = 2;
                else if (t == QLatin1String("distribution")) def = 3;
                else if (t == QLatin1String("random2d")) def = 4;  
                else if (t == QLatin1String("random3d")) def = 5;  
                const QStringList otok = frpSplitCsv(nextDataLine(idx));

                if (curPending)
                {
                    curPending->orientDef = def;
                    curPending->orientFile = attrs.value(QStringLiteral("file"));
                    curPending->orientTokens = otok;
                }
                else if (curPhaseNode)
                {
                    PendingPhase op;
                    op.orientDef = def;
                    op.orientFile = attrs.value(QStringLiteral("file"));
                    op.orientTokens = otok;
                    if (curPhaseNode->type == FrpNode_Inclusion)
                        frpApplyPendingOrientation(curPhaseNode->inc.defMethod,
                            curPhaseNode->inc.theta, curPhaseNode->inc.phi,
                            curPhaseNode->inc.tensor, curPhaseNode->inc.tensorFilePath, op);
                    else if (curPhaseNode->type == FrpNode_Cavity)
                        frpApplyPendingOrientation(curPhaseNode->cav.defMethod,
                            curPhaseNode->cav.theta, curPhaseNode->cav.phi,
                            curPhaseNode->cav.tensor, curPhaseNode->cav.tensorFilePath, op);
                    else if (curPhaseNode->type == FrpNode_Fiber && def == 1 && otok.size() >= 2)
                    {
                        curPhaseNode->fib.theta = otok[0];
                        curPhaseNode->fib.phi = otok[1];
                    }
                }
            }
            else if (kw == QLatin1String("coated"))
            {
                const QString mat = attrs.value(QStringLiteral("mat"));
                const QString frac = nextDataLine(idx);
                if (curPending)
                {
                    curPending->coated = true;
                    curPending->coatMat = mat;
                    curPending->coatFrac = frac;
                }
                else if (curPhaseNode)
                {
                    if (curPhaseNode->type == FrpNode_Inclusion)
                    {
                        curPhaseNode->inc.advanced = true;
                        curPhaseNode->inc.featureType = 0;
                        curPhaseNode->inc.featureMat = mat;
                        if (!frac.isEmpty()) curPhaseNode->inc.featureFrac = frac;
                    }
                    else if (curPhaseNode->type == FrpNode_Fiber)
                    {
                        curPhaseNode->fib.coating = true;
                        curPhaseNode->fib.coatMat = mat;
                        if (!frac.isEmpty()) curPhaseNode->fib.coatFrac = frac;
                    }
                    else if (curPhaseNode->type == FrpNode_Cavity)
                    {
                        curPhaseNode->cav.surface = true;
                        curPhaseNode->cav.surfaceMat = mat;
                        if (!frac.isEmpty()) curPhaseNode->cav.surfaceFrac = frac;
                    }
                }
            }
            else if (kw == QLatin1String("clustering"))
            {
                const QStringList ctok = frpSplitCsv(nextDataLine(idx));
                if (curPending)
                {
                    curPending->clustering = true;
                    curPending->clusterTokens = ctok;
                }
                else if (curPhaseNode && curPhaseNode->type == FrpNode_Inclusion)
                {
                    curPhaseNode->inc.advanced = true;
                    curPhaseNode->inc.featureType = 1;
                    if (ctok.size() >= 1) curPhaseNode->inc.featureAspect = ctok[0];
                    if (ctok.size() >= 2) curPhaseNode->inc.fiberFrac1 = ctok[1];
                    if (ctok.size() >= 3) curPhaseNode->inc.fiberFrac2 = ctok[2];
                }
            }
            else if (kw == QLatin1String("rve"))
            {
                MechFrpData::mFrpBaseData* rveNode = groupOf(FrpNode_Rve);
                MechFrpData::mFrpRveData* rv = rveNode ? rveNode->as<MechFrpData::mFrpRveData>() : nullptr;
                if (!rv)
                    continue;
                setRenamed(rveNode, attrs.value(QStringLiteral("name")));
                rv->layers.clear();
                if (attrs.value(QStringLiteral("type")) == QLatin1String("multilayer"))
                {
                    rv->mode = QStringLiteral("多层分析");
                    while (idx + 1 < lines.size() && !lines.at(idx + 1).startsWith(QLatin1Char('*')))
                    {
                        ++idx;
                        const QStringList tok = frpSplitCsv(lines.at(idx));
                        if (tok.size() < 3)
                            continue;
                        MechFrpData::RveLayer lyr;
                        lyr.micro = tok[0];
                        lyr.thick = tok[1];
                        lyr.angle = tok[2];
                        if (tok.size() >= 4)
                        {
                            const QString d0 = tok[3];
                            if (d0 == QLatin1String("Random2D")) lyr.orient = QStringLiteral("Random 2D");
                            else if (d0 == QLatin1String("Random3D")) lyr.orient = QStringLiteral("Random 3D");
                            else if (tok.size() == 5)
                            {
                                lyr.orient = QStringLiteral("固定角度");
                                lyr.value = tok[3] + QLatin1Char(';') + tok[4] + QLatin1Char(';');
                            }
                            else if (tok.size() == 9)
                            {
                                lyr.orient = QStringLiteral("固定取向张量");
                                lyr.value =
                                    MechFrpData::mFrpManager::reorderTensor6FromAbaqus(tok.mid(3, 6))
                                    .join(QLatin1Char(';')) + QLatin1Char(';');
                            }
                        }
                        rv->layers << lyr;
                    }
                }
                else
                {
                    rv->mode = QStringLiteral("单层分析");
                    rv->structure = attrs.value(QStringLiteral("microstructure"));
                }
            }
            else if (kw == QLatin1String("failure"))
            {
                MechFrpData::mFrpBaseData* grp = groupOf(FrpNode_Failure);
                if (!grp)
                    continue;
                MechFrpData::mFrpBaseData* fnNode = candidate.addChildOfType(grp, FrpNode_Failure);
                if (!fnNode)
                    continue;
                MechFrpData::mFrpFailureData* f = fnNode->as<MechFrpData::mFrpFailureData>();
                setRenamed(fnNode, attrs.value(QStringLiteral("name")));
                const int mi = frpFailureIndexFromKw(attrs.value(QStringLiteral("indicator")));
                if (mi >= 0)
                    f->model = frpFailureTextFromIndex(mi);
                if (mi != 4 && mi != 7 && mi != 8)
                    f->failType = (attrs.value(QStringLiteral("type")) == QLatin1String("strain"))
                    ? QStringLiteral("Strain") : QStringLiteral("Stress");
                const QStringList tok = frpSplitCsv(nextDataLine(idx));
                switch (mi)
                {
                case 0:
                    if (tok.size() >= 1)
                    {
                        const QString c = tok[0];
                        if (c == QLatin1String("11") || c == QLatin1String("12") || c == QLatin1String("13")
                            || c == QLatin1String("22") || c == QLatin1String("23") || c == QLatin1String("33"))
                            f->compCriterion = c;
                    }
                    if (tok.size() >= 2) f->tens = tok[1];
                    if (tok.size() >= 3) f->comp2 = tok[2];
                    break;
                case 1:
                    if (tok.size() >= 5)
                    {
                        f->axialTens = tok[0]; f->axialComp = tok[1];
                        f->inPlaneTens = tok[2]; f->inPlaneComp = tok[3]; f->shear = tok[4];
                    }
                    break;
                case 2:
                    if (tok.size() >= 3) { f->axialTensT = tok[0]; f->inPlaneTensT = tok[1]; f->shearT = tok[2]; }
                    break;
                case 3:
                    if (tok.size() >= 6)
                    {
                        f->xt = tok[0]; f->yt = tok[1]; f->zt = tok[2];
                        f->xys = tok[3]; f->yzs = tok[4]; f->xzs = tok[5];
                    }
                    break;
                case 4:
                    if (tok.size() >= 5)
                    {
                        f->azAxTens = tok[0]; f->azAxComp = tok[1];
                        f->azIpTens = tok[2]; f->azIpComp = tok[3]; f->azShear = tok[4];
                    }
                    break;
                case 5:
                    if (tok.size() >= 6)
                    {
                        f->wuAxTens = tok[0]; f->wuAxComp = tok[1];
                        f->wuIpTens = tok[2]; f->wuIpComp = tok[3];
                        f->wuShear = tok[4]; f->wuCouple = tok[5];
                    }
                    break;
                case 6:
                    if (tok.size() >= 5)
                    {
                        f->wuTAxialTens = tok[0]; f->wuTAxialComp = tok[1];
                        f->wuTInPlaneTens = tok[2]; f->wuTInPlaneComp = tok[3]; f->wuTShear = tok[4];
                    }
                    break;
                case 7:   // H 行(6) + G 行(21)
                {
                    QStringList store;
                    if (tok.size() >= 6)
                        store << tok[0] << tok[1] << tok[2] << tok[3] << tok[4] << tok[5];
                    const QString gLine = nextDataLine(idx);
                    if (!gLine.isEmpty())
                        for (const QString& v : frpSplitCsv(gLine)) store << v;
                    f->wu3d = store.join(QStringLiteral("; "));
                    break;
                }
                case 8:
                    if (tok.size() >= 5)
                    {
                        f->mcAxialTens = tok[0]; f->mcAxialComp = tok[1];
                        f->mcInPlaneTens = tok[2]; f->mcInPlaneComp = tok[3]; f->mcShear = tok[4];
                    }
                    break;
                default:
                    break;
                }
            }
            else if (kw == QLatin1String("ratiofunction"))
            {
                MechFrpData::mFrpBaseData* grp = groupOf(FrpNode_AspectRatio);
                if (!grp)
                    continue;
                MechFrpData::mFrpBaseData* fnNode = candidate.addChildOfType(grp, FrpNode_AspectRatio);
                if (!fnNode)
                    continue;
                MechFrpData::mFrpAspectRatioData* f = fnNode->as<MechFrpData::mFrpAspectRatioData>();
                setRenamed(fnNode, attrs.value(QStringLiteral("name")));
                while (idx + 1 < lines.size() && !lines.at(idx + 1).startsWith(QLatin1Char('*')))
                {
                    ++idx;
                    const QStringList tok = frpSplitCsv(lines.at(idx));
                    if (tok.size() < 2)
                        continue;
                    f->rows.append(std::make_pair(tok[0], tok[1]));
                }
            }
        }

        if (!sawAnalysis || candidate.roots().isEmpty())
        {
            err = QStringLiteral("文件中未找到有效的均质化模型（*rveanalysis）");
            return false;
        }

        // 材料占位补全：相节点引用的材料名若在所属模型材料分组缺失，补建同名空属性材料（避免引用悬空）
        for (MechFrpData::mFrpBaseData* root : candidate.roots())
        {
            MechFrpData::mFrpBaseData* matGrp = frpFindGroup(root, FrpNode_Material);
            MechFrpData::mFrpBaseData* microGrp = frpFindGroup(root, FrpNode_Micro);
            if (!matGrp || !microGrp)
                continue;
            QSet<QString> existing;
            for (MechFrpData::mFrpBaseData* m : matGrp->children)
                existing.insert(m->name);
            QSet<QString> needed;
            auto collect = [&](const QString& nm) {
                if (!nm.isEmpty() && nm != QStringLiteral("无"))
                    needed.insert(nm);
                };
            for (MechFrpData::mFrpBaseData* micro : microGrp->children)
                for (MechFrpData::mFrpBaseData* phNode : micro->children)
                    if (MechFrpData::mFrpPhaseData* p = phNode->as<MechFrpData::mFrpPhaseData>())
                    {
                        collect(p->baseMaterial);
                        collect(p->inc.material);
                        collect(p->inc.featureMat);
                        collect(p->cav.surfaceMat);
                        collect(p->fib.material);
                        collect(p->fib.coatMat);
                    }
            for (const QString& nm : needed)
            {
                if (existing.contains(nm))
                    continue;
                MechFrpData::mFrpBaseData* m = candidate.addChildOfType(matGrp, FrpNode_Material);
                if (m)
                {
                    m->name = nm;
                    m->renamed = true;
                    warnings << QStringLiteral("材料“%1”无属性定义，已创建占位材料，请补全力学/热学属性").arg(nm);
                }
            }
        }

        err.clear();
        return true;
    }
} // namespace

namespace MechIOFile
{
    mOMfFile::mOMfFile()
        : _isSuccessWrite(false)
    {
        _modelData = nullptr;
    }

    mOMfFile::mOMfFile(QString fname)
        : _outputFileName(fname)
        , _isSuccessWrite(false)
    {
        _modelData = nullptr;
    }

    mOMfFile::~mOMfFile()
    {
    }

    bool mOMfFile::isSuccessWrite() const
    {
        return _isSuccessWrite;
    }

    QString mOMfFile::getOutFilePath()
    {
        return _outputFileName;
    }

    void mOMfFile::setExportModelNames(const QStringList& names)
    {
        _exportModelNames = names;
    }

    void mOMfFile::setExportModelName(const QString& name)
    {
        _exportModelNames = name.isEmpty() ? QStringList() : QStringList(name);
    }

    void mOMfFile::run()
    {
        _isSuccessWrite = false;
        if (!getCVM)
            return;
        _modelData = getCVM->getModelData();
        if (!_modelData)
        {
            emit writeFileErrorSig(MechCommon::DatabaseNull);
            return;
        }
        MechFrpData::mFrpManager& mgr =
            _modelData->getManager<MechFrpData::mFrpManager>(Physical_FRCMaterial);

        // 1) 目标模型：_exportModelNames 为空导出全部，否则按名筛选
        QVector<const MechFrpData::mFrpBaseData*> targets;
        if (_exportModelNames.isEmpty())
        {
            for (const MechFrpData::mFrpBaseData* r : mgr.roots())
                targets.append(r);
        }
        else
        {
            for (const QString& nm : _exportModelNames)
            {
                const MechFrpData::mFrpBaseData* hit = nullptr;
                for (const MechFrpData::mFrpBaseData* r : mgr.roots())
                    if (r->name == nm) { hit = r; break; }
                if (hit) targets.append(hit);
                else mGlobalSignals::getInstance()->outputMessageSig(
                    QStringLiteral("模型“%1”不存在，未导出").arg(nm));
            }
        }
        if (targets.isEmpty())
        {
            mGlobalSignals::getInstance()->outputMessageSig(
                QStringLiteral("没有可导出的均质化模型"));
            emit writeFileErrorSig(MechCommon::ModelMismatching);
            return;
        }

        // 2) 生成文本（生成失败只跳过该模型）
        struct One { const MechFrpData::mFrpBaseData* root; QString text; };
        QVector<One> ready;
        for (const MechFrpData::mFrpBaseData* r : targets)
        {
            QString err;
            const QString text = frpExportOne(r, err);
            if (!err.isEmpty())
            {
                mGlobalSignals::getInstance()->outputMessageSig(
                    QStringLiteral("模型“%1”未导出：%2").arg(r->name, err));
                continue;
            }
            ready.append({ r, text });
        }
        if (ready.isEmpty())
        {
            emit writeFileErrorSig(MechCommon::ModelMismatching);
            return;
        }

        // 3) 落点：单模型且路径以 .mf 结尾 = 直接写该文件；否则按目录、每模型一个文件
        const bool singleFile = (ready.size() == 1
            && _outputFileName.trimmed().endsWith(QLatin1String(".mf"), Qt::CaseInsensitive));

        int okCount = 0;
        if (singleFile)
        {
            QDir().mkpath(QFileInfo(_outputFileName).absolutePath());   // 确保父目录存在
            QString werr;
            if (frpWriteFile(_outputFileName, ready.first().text, werr))
            {
                mGlobalSignals::getInstance()->outputMessageSig(
                    QStringLiteral("模型“%1”已导出：%2").arg(ready.first().root->name, _outputFileName));
                ++okCount;
            }
            else
                mGlobalSignals::getInstance()->outputMessageSig(
                    QStringLiteral("模型“%1”导出失败：%2").arg(ready.first().root->name, werr));
        }
        else
        {
            QDir dir(_outputFileName);
            if (!dir.exists() && !dir.mkpath(QStringLiteral(".")))
            {
                mGlobalSignals::getInstance()->outputMessageSig(
                    QStringLiteral("导出失败：目录无法创建：%1").arg(_outputFileName));
                emit writeFileErrorSig(MechCommon::FileNotOpen);
                return;
            }
            for (const One& o : ready)
            {
                const QString fn = dir.absoluteFilePath(o.root->name + QStringLiteral(".mf"));
                QString werr;
                if (frpWriteFile(fn, o.text, werr))
                {
                    mGlobalSignals::getInstance()->outputMessageSig(
                        QStringLiteral("模型“%1”已导出：%2").arg(o.root->name, fn));
                    ++okCount;
                }
                else
                    mGlobalSignals::getInstance()->outputMessageSig(
                        QStringLiteral("模型“%1”导出失败：%2").arg(o.root->name, werr));
            }
        }

        if (okCount > 0) _isSuccessWrite = true;
        if (okCount == 0) emit writeFileErrorSig(MechCommon::ModelMismatching);
    }

    /************************************* 导入 ***************************************/
    mIMfFile::mIMfFile()
        : _isSuccessRead(false)
    {
    }

    mIMfFile::mIMfFile(QString fname)
        : _inputFileName(fname)
        , _isSuccessRead(false)
    {
    }

    mIMfFile::~mIMfFile()
    {
    }

    bool mIMfFile::isSuccessRead() const
    {
        return _isSuccessRead;
    }

    QString mIMfFile::getInFilePath()
    {
        return _inputFileName;
    }

    QString mIMfFile::errorString() const
    {
        return _errorString;
    }

    QStringList mIMfFile::takeWarnings()
    {
        return _warnings;
    }

    void mIMfFile::run()
    {
        _isSuccessRead = false;
        _errorString.clear();
        _warnings.clear();

        // 1) 读 UTF-8 文本
        if (!QFileInfo::exists(_inputFileName))
        {
            _errorString = QStringLiteral("导入失败：文件不存在！");
            emit readFileErrorSig(MechCommon::FileNotExist);
            return;
        }
        QFile file(_inputFileName);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            _errorString = QStringLiteral("导入失败：文件打不开！");
            emit readFileErrorSig(MechCommon::FileNotOpen);
            return;
        }
        QString content;
        {
            QTextStream in(&file);
            in.setCodec("UTF-8");
            content = in.readAll();
        }
        file.close();

        // 2) 取全局 FRP 模型管理器（与界面层 frpMgr() 同一入口）
        if(!getCVM)
            return;

        auto* pModelData = getCVM->getModelData();
        if (!pModelData)
        {
            _errorString = QStringLiteral("导入失败：模型数据未初始化！");
            emit readFileErrorSig(MechCommon::DatabaseNull);
            return;
        }
        MechFrpData::mFrpManager& mgr =
            pModelData->getManager<MechFrpData::mFrpManager>(Physical_FRCMaterial);

        // 3) 解析到临时 manager：失败不动当前树
        MechFrpData::mFrpManager candidate;
        QString err;
        QStringList warnings;
        const QFileInfo inFi(_inputFileName);
        if (!frpImportComposite(content, candidate, err, warnings, inFi.completeBaseName()))
        {
            _errorString = QStringLiteral("导入失败：%1").arg(err);
            emit readFileErrorSig(MechCommon::ModelMismatching);
            return;
        }

        // 4) 成功：追加到全局管理器（保留已有模型，新模型并排在后）。
        //    树视图重建(rebuildTree)由界面在 finished 槽、主线程执行；
        //    导入期间界面不应操作该模型树（与 mOMfFile 导出同一线程模型假设）。
        mgr.appendFrom(candidate);
        _warnings = warnings;
        _isSuccessRead = true;
        // run() 返回后 QThread 自动发射 finished()，无需自行 emit
    }

} // namespace MechIOFile

