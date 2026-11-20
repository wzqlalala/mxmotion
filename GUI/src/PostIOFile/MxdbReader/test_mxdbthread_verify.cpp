// mMxdbThread 按需读取接口验证程序
// 用法: MxdbThreadVerify.exe <sample.mxdb>
// 验证 mMxdbThread 迁移到 MxdbReaderCore 后的全部按需读取接口：
//   readModelAndFrameInfo / readVarDatas x4 / readElemData / readFlattenStress /
//   readVectorData / readCurveData / readTimeVarCurveData / readVarVarCurveData /
//   readTimeVarCurve / readIDVarCurve

#include <QApplication>
#include <QFileInfo>
#include <QDebug>
#include <QTextStream>
#include <cmath>
#include <set>
#include <vector>

#include "mMxdbThread.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mPostPartDataStruct.h"
#include "MxdbReaderCore/NewSingleMxdbReader.h"
#include <fstream>

using namespace MIOFile;
using namespace PostData;

static QTextStream& out()
{
    static QTextStream ts(stdout);
    return ts;
}

static int g_passCount = 0;
static int g_failCount = 0;
static int g_skipCount = 0;

static void reportPass(const QString& name, const QString& detail = QString())
{
    ++g_passCount;
    out() << "[PASS] " << name;
    if (!detail.isEmpty())
        out() << " | " << detail;
    out() << "\n";
    out().flush();
}

static void reportFail(const QString& name, const QString& detail = QString())
{
    ++g_failCount;
    out() << "[FAIL] " << name;
    if (!detail.isEmpty())
        out() << " | " << detail;
    out() << "\n";
    out().flush();
}

static void reportSkip(const QString& name, const QString& reason)
{
    ++g_skipCount;
    out() << "[SKIP] " << name << " | " << reason << "\n";
    out().flush();
}

static bool checkFinite(const QHash<int, double>& h, QString& badInfo)
{
    for (auto it = h.constBegin(); it != h.constEnd(); ++it) {
        if (!std::isfinite(it.value())) {
            badInfo = QString("id=%1 value=%2").arg(it.key()).arg(it.value());
            return false;
        }
    }
    return true;
}

static bool checkFiniteVec(const QHash<QString, QHash<int, double>>& h, QString& badInfo)
{
    for (auto it = h.constBegin(); it != h.constEnd(); ++it) {
        if (!checkFinite(it.value(), badInfo)) {
            badInfo = it.key() + ": " + badInfo;
            return false;
        }
    }
    return true;
}

static bool checkFiniteVec(const QHash<QString, QVector<double>>& h, QString& badInfo)
{
    for (auto it = h.constBegin(); it != h.constEnd(); ++it) {
        for (double v : it.value()) {
            if (!std::isfinite(v)) {
                badInfo = QString("%1 value=%2").arg(it.key()).arg(v);
                return false;
            }
        }
    }
    return true;
}

// 收集部件中所有单元真实 id（最多 maxCount 个）
static std::set<int> collectElementRIds(QPointer<mPostModelData> modelData, int maxCount)
{
    std::set<int> result;
    if (modelData == nullptr)
        return result;
    for (auto part : modelData->_partdata) {
        if (part == nullptr)
            continue;
        for (const auto& kv : part->_parteledata.cell_rid_to_cid) {
            result.insert(kv.first);
            if (static_cast<int>(result.size()) >= maxCount)
                return result;
        }
    }
    return result;
}

// 取节点真实 id（最多 maxCount 个）
static QVector<int> collectNodeRIds(QPointer<mPostModelData> modelData, int maxCount)
{
    QVector<int> result;
    if (modelData == nullptr)
        return result;
    const auto keys = modelData->_nodedata.nodeidmap.keys();
    for (int id : keys) {
        result.append(id);
        if (result.size() >= maxCount)
            break;
    }
    return result;
}

int main(int argc, char* argv[])
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);

    QString sample;
    if (argc > 1)
        sample = QString::fromLocal8Bit(argv[1]);
    else
        sample = QString::fromWCharArray(L"F:\\Test_Model_Data\\数据重构测试文件\\PostV3.0测试文件\\现在能导入\\4x4四边形.mxdb");

    if (!QFileInfo::exists(sample)) {
        out() << "sample not found: " << sample << "\n";
        return 2;
    }
    out() << "sample: " << sample << "\n";

    // 1. 无头视口模型（不创建渲染视口）
    auto vmm = MechPreData::mViewportModelManager::getInstance();
    vmm->newPostViewportModel();
    auto cvm = vmm->getCurrentViewportModel();
    if (cvm == nullptr) {
        reportFail("setup/viewport", "getCurrentViewportModel returned null");
        return 1;
    }
    reportPass("setup/viewport");

    // 2. 注册后处理数据库（同 MainWindow 导入路径）
    QPointer<mPostDataBase> db = new mPostDataBase(sample);
    cvm->getModelData()->addPhysicalFieldP(db);
    auto& mgr = cvm->getModelData()->getManager<mPostDataManager>(MxFunctions::Physical_Post);
    if (mgr.getPostDatabase(sample) == nullptr) {
        reportFail("setup/database", "getPostDatabase returned null after addPhysicalFieldP");
        return 1;
    }
    reportPass("setup/database");

    mMxdbThread reader(sample, db);

    // 3. 整体导入
    if (!reader.readModelAndFrameInfo()) {
        reportFail("import/readModelAndFrameInfo");
        return 1;
    }
    auto frames = db->getAllFrameData();
    QPointer<mPostModelData> initFrame = db->getModelData(1); // 初始帧注册在 [1]
    const int nframe = frames.size();
    out() << QString("result frames=%1, initFrame=%2").arg(nframe).arg(initFrame != nullptr ? "yes" : "no") << "\n";
    if (frames.isEmpty() && initFrame == nullptr) {
        reportFail("import/readModelAndFrameInfo", "no frames and no initFrame");
        return 1;
    }
    reportPass("import/readModelAndFrameInfo", QString("resultFrames=%1 initFrame=%2").arg(nframe).arg(initFrame != nullptr));

    // 4. 枚举变量（优先最后一帧结果；无结果帧时用初始帧）
    QPointer<mPostModelData> md = frames.value(nframe); // 初始帧 [1]，结果帧 [2..]
    if (md == nullptr)
        md = initFrame;
    if (md == nullptr) {
        reportFail("import/frameData", "no modelData available");
        return 1;
    }
    const PostFrameInfo frameInfo = md->_frameInfo;
    // 帧号约定：初始帧 [1]，结果帧 [2..nframe]；单帧模型只有 [1]
    const int testFrame = nframe;
    out() << "variables in frame " << testFrame << ":\n";
    QString vectorVar, elemVar, stressVar, anyVar;
    QStringList vectorVarCompos, elemVarCompos, anyVarCompos, stressVarCompos;
    static const QSet<QString> stressNames = {
        "Stress(E)", "Strain(E)", "PSD Stress(E)", "PSD Strain(E)", "RMS Stress(E)"
    };
    for (const PostVariableInfo& v : frameInfo._frameVars) {
        out() << QString("  %1 type=%2 nCompo=%3 compos=[%4] varByte=%5")
                     .arg(v._varName).arg(v._varType).arg(v._nCompo)
                     .arg(v._compoNames.join(",")).arg(v._varByte) << "\n";
        if (anyVar.isEmpty() && v._nCompo >= 1) {
            anyVar = v._varName;
            anyVarCompos = v._compoNames;
        }
        if (vectorVar.isEmpty() && v._compoNames.size() == 3) {
            vectorVar = v._varName;
            vectorVarCompos = v._compoNames;
        }
        if (elemVar.isEmpty() && v._varType == 1) {
            elemVar = v._varName;
            elemVarCompos = v._compoNames;
        }
        if (stressVar.isEmpty() && stressNames.contains(v._varName)) {
            stressVar = v._varName;
            stressVarCompos = v._compoNames;
        }
    }
    out().flush();
    if (anyVar.isEmpty()) {
        reportFail("import/variables", "no variables in frame");
        return 1;
    }

    const QString testVar = anyVar;
    const QString testCompo = anyVarCompos.isEmpty() ? QString() : anyVarCompos.first();
    const QVector<int> nodeIds = collectNodeRIds(md, 5);
    const std::set<int> eleRIds = collectElementRIds(md, 5);
    out() << QString("test frame=%1 anyVar=%2 vectorVar=%3 elemVar=%4 stressVar=%5 nodes=%6 eles=%7")
                 .arg(testFrame).arg(testVar).arg(vectorVar).arg(elemVar).arg(stressVar)
                 .arg(nodeIds.size()).arg(eleRIds.size()) << "\n";
    out().flush();

    const QString emptyStr;
    const QVector<double> emptyLocal;
    const QStringList testCompos = anyVarCompos.isEmpty() ? QStringList{ testCompo } : anyVarCompos;

    // ---- 接口 1: readVarDatas(filePath, f, var, compo, layer, complex, local) ----
    {
        bool ok = reader.readVarDatas(sample, testFrame, testVar, testCompo, emptyStr, emptyStr, emptyLocal);
        // 读回 setComponeValues 存的值（颜色表数据源）
        const Compones& stored = db->getComponeValues();
        out() << "  stored compone: isD=" << stored.isD << " frameID=" << stored.frameID
              << " flag=" << stored.flag << " v1=" << stored.value1.size()
              << " v2=" << stored.value2.size() << " v3=" << stored.value3.size() << "\n";
        // addResultValue 算出的结果（颜色表数据源）
        auto rd = db->getResultDataByFrameID(stored.frameID);
        double rmax = -DBL_MAX, rmin = DBL_MAX;
        for (auto it = rd.constBegin(); it != rd.constEnd(); ++it) {
            if (it.value() > rmax) rmax = it.value();
            if (it.value() < rmin) rmin = it.value();
        }
        out() << "  _resultDatas[" << stored.frameID << "] = " << rd.size()
              << " max=" << rmax << " min=" << rmin << "\n";
        out().flush();
        if (ok && !stored.value1.isEmpty())
            reportPass("readVarDatas/local", QString("%1/%2 stored=%3").arg(testVar, testCompo).arg(stored.value1.size()));
        else if (ok)
            reportFail("readVarDatas/local", QString("%1/%2 stored empty (v1=%3 v2=%4 v3=%5)")
                .arg(testVar, testCompo).arg(stored.value1.size()).arg(stored.value2.size()).arg(stored.value3.size()));
        else
            reportFail("readVarDatas/local", QString("%1/%2").arg(testVar, testCompo));
    }

    // ---- 接口 1b: readVarDatas(local) 对 Stress(E)（云图显示路径，壳变量需 layer） ----
    if (!stressVar.isEmpty()) {
        QString compo = stressVarCompos.isEmpty() ? QString() : stressVarCompos.first();
        for (const QString& lyr : {QString("Top"), QString("Bottom"), QString("Max")}) {
            bool ok = reader.readVarDatas(sample, testFrame, stressVar, compo, lyr, emptyStr, emptyLocal);
            auto rd = db->getResultDataByFrameID(testFrame);
            double rmax = -DBL_MAX, rmin = DBL_MAX;
            for (auto it = rd.constBegin(); it != rd.constEnd(); ++it) {
                if (it.value() > rmax) rmax = it.value();
                if (it.value() < rmin) rmin = it.value();
            }
            out() << "  Stress layer=" << lyr << ": rd=" << rd.size() << " max=" << rmax << " min=" << rmin << "\n";
            out().flush();
            if (!ok || rd.isEmpty())
                reportFail("readVarDatas/local-Stress", QString("layer=%1 ok=%2 rd=%3").arg(lyr).arg(ok).arg(rd.size()));
        }
        reportPass("readVarDatas/local-Stress", "Top/Bottom/Max checked");
    }

    // ---- 接口 2: readVarDatas(..., Compones&, local) ----
    {
        Compones componeData;
        bool ok = reader.readVarDatas(sample, testFrame, testVar, testCompo, emptyStr, emptyStr, componeData, emptyLocal);
        QString bad;
        bool finite = checkFinite(componeData.value1, bad);
        if (ok && !componeData.value1.isEmpty() && finite) {
            reportPass("readVarDatas/Compones", QString("values=%1").arg(componeData.value1.size()));
        } else if (ok && componeData.value1.isEmpty() && !componeData.dRValue.isEmpty()) {
            reportPass("readVarDatas/Compones", QString("dRValue parts=%1").arg(componeData.dRValue.size()));
        } else {
            reportFail("readVarDatas/Compones",
                QString("ok=%1 value1=%2 finite=%3 %4").arg(ok).arg(componeData.value1.size()).arg(finite).arg(bad));
        }
    }

    // ---- 接口 3: readVarDatas(..., selids, allVals, flag) ----
    QHash<QString, QHash<int, double>> allValsRef;
    {
        int flag0 = 0;
        QHash<QString, QHash<int, double>> tmpVals;
        bool ok0 = reader.readVarDatas(sample, testFrame, testVar, testCompos, emptyStr, emptyStr, emptyLocal, tmpVals, flag0);
        if (ok0 && !tmpVals.isEmpty()) {
            allValsRef = tmpVals;
            QList<int> entityIds = tmpVals.constBegin().value().keys();
            std::set<int> selids;
            for (int i = 0; i < entityIds.size() && i < 3; ++i)
                selids.insert(entityIds.at(i));

            int flag = 0;
            QHash<QString, QHash<int, double>> allVals;
            bool ok = reader.readVarDatas(sample, testFrame, testVar, testCompos, emptyStr, emptyStr, emptyLocal, selids, allVals, flag);
            QString bad;
            bool finite = checkFiniteVec(allVals, bad);
            bool subsetOk = true;
            for (auto it = allVals.constBegin(); it != allVals.constEnd() && subsetOk; ++it) {
                for (int id : selids) {
                    if (!it.value().contains(id)) {
                        subsetOk = false;
                        bad = QString("compo=%1 missing id=%2").arg(it.key()).arg(id);
                        break;
                    }
                    double vSel = it.value().value(id);
                    double vAll = tmpVals.value(it.key()).value(id);
                    if (std::abs(vSel - vAll) > 1e-6 * (1.0 + std::abs(vAll))) {
                        subsetOk = false;
                        bad = QString("compo=%1 id=%2 sel=%3 all=%4").arg(it.key()).arg(id).arg(vSel).arg(vAll);
                        break;
                    }
                }
            }
            if (ok && !allVals.isEmpty() && finite && subsetOk)
                reportPass("readVarDatas/selids", QString("selids=%1 values match unfiltered").arg(selids.size()));
            else
                reportFail("readVarDatas/selids", QString("ok=%1 empty=%2 finite=%3 subset=%4 %5")
                    .arg(ok).arg(allVals.isEmpty()).arg(finite).arg(subsetOk).arg(bad));
        } else {
            reportFail("readVarDatas/selids", "unfiltered pre-read failed");
        }
    }

    // ---- 接口 4: readVarDatas(..., allVals, flag)（接口 3 预读已覆盖，正式断言） ----
    {
        QString bad;
        if (!allValsRef.isEmpty() && checkFiniteVec(allValsRef, bad))
            reportPass("readVarDatas/allVals", QString("compos=%1 entities=%2")
                .arg(allValsRef.size()).arg(allValsRef.constBegin().value().size()));
        else
            reportFail("readVarDatas/allVals", bad);
    }

    // ---- 基线抽样：各分量前 10 个 id+值（用于跨版本 diff） ----
    if (qEnvironmentVariableIsSet("MXDB_VERIFY_DUMP")) {
        out() << "==== baseline dump ====\n";
        for (auto cit = allValsRef.constBegin(); cit != allValsRef.constEnd(); ++cit) {
            out() << "compo " << cit.key() << ":";
            QList<int> keys = cit.value().keys();
            std::sort(keys.begin(), keys.end());
            for (int i = 0; i < keys.size() && i < 10; ++i)
                out() << " " << keys[i] << "=" << QString::number(cit.value().value(keys[i]), 'g', 17);
            out() << "\n";
        }
        if (!stressVar.isEmpty()) {
            Compones sc;
            QString compo = stressVarCompos.isEmpty() ? QString() : stressVarCompos.first();
            // 壳单元变量需要 layer（Top/Bottom/Max），实体变量 layer 为空
            QString layer = "Max";
            if (reader.readVarDatas(sample, testFrame, stressVar, compo, layer, emptyStr, sc, emptyLocal)) {
                out() << "stress " << stressVar << "/" << compo << "(v1=" << sc.value1.size() << " v2=" << sc.value2.size() << " v3=" << sc.value3.size() << " v4=" << sc.value4.size() << " isD=" << sc.isD << "):";
                const QHash<int, double>* src = &sc.value1;
                if (src->isEmpty()) src = &sc.value2;
                QList<int> keys = src->isEmpty() ? sc.value3.keys() : src->keys();
                std::sort(keys.begin(), keys.end());
                for (int i = 0; i < keys.size() && i < 10; ++i)
                    out() << " " << keys[i] << "=" << QString::number(src->isEmpty() ? (double)sc.value3.value(keys[i]) : src->value(keys[i]), 'g', 17);
                out() << "\n";
            }
        }
        out().flush();
    }

    // ---- 接口 5: readElemData ----
    if (!eleRIds.empty()) {
        bool ok = reader.readElemData(sample, testFrame, testVar, testCompo, emptyStr, emptyStr, emptyLocal, eleRIds);
        if (ok)
            reportPass("readElemData", QString("eleids=%1").arg(eleRIds.size()));
        else
            reportFail("readElemData", QString("%1/%2").arg(testVar, testCompo));
    } else {
        reportSkip("readElemData", "no elements in model");
    }

    // ---- 接口 6: readFlattenStress ----
    if (!stressVar.isEmpty()) {
        QString compo = stressVarCompos.isEmpty() ? QString() : stressVarCompos.first();
        bool ok = reader.readFlattenStress(sample, testFrame, stressVar, compo, "Max", emptyStr, emptyLocal);
        // Stress 是单元变量，检查 _resultDatas（颜色表数据源）
        auto rd = db->getResultDataByFrameID(testFrame);
        double rmax = -DBL_MAX, rmin = DBL_MAX;
        for (auto it = rd.constBegin(); it != rd.constEnd(); ++it) {
            if (it.value() > rmax) rmax = it.value();
            if (it.value() < rmin) rmin = it.value();
        }
        out() << "  Stress _resultDatas[" << testFrame << "] = " << rd.size()
              << " max=" << rmax << " min=" << rmin << "\n";
        out().flush();
        if (ok)
            reportPass("readFlattenStress", QString("%1/%2 _resultDatas=%3").arg(stressVar, compo).arg(rd.size()));
        else
            reportFail("readFlattenStress", QString("%1/%2").arg(stressVar, compo));
    } else {
        reportSkip("readFlattenStress", "no Stress(E)/Strain(E) variable");
    }

    // ---- 接口 7: readVectorData ----
    if (!vectorVar.isEmpty()) {
        bool ok = reader.readVectorData(sample, testFrame, vectorVar);
        QHash<int, QVector3D> vecData = db->getVectorDataByFrameID(testFrame);
        bool finite = true;
        for (auto it = vecData.constBegin(); it != vecData.constEnd(); ++it) {
            const QVector3D v = it.value();
            if (!std::isfinite(v.x()) || !std::isfinite(v.y()) || !std::isfinite(v.z())) {
                finite = false;
                break;
            }
        }
        if (ok && !vecData.isEmpty() && finite)
            reportPass("readVectorData", QString("%1 vectors=%2").arg(vectorVar).arg(vecData.size()));
        else
            reportFail("readVectorData", QString("ok=%1 vectors=%2 finite=%3").arg(ok).arg(vecData.size()).arg(finite));
    } else {
        reportSkip("readVectorData", "no 3-component variable");
    }

    // 曲线接口公共参数
    QVector<int> curveIds;
    if (!allValsRef.isEmpty()) {
        QList<int> keys = allValsRef.constBegin().value().keys();
        for (int i = 0; i < keys.size() && i < 3; ++i)
            curveIds.append(keys.at(i));
    }
    if (curveIds.isEmpty())
        curveIds = nodeIds;
    std::set<int> frameIdSet;
    for (int i = 1; i <= nframe; ++i)
        frameIdSet.insert(i);

    // ---- 接口 8: readCurveData ----
    {
        QStringList cnames;
        QHash<QString, QHash<int, double>> yD;
        QHash<QString, QHash<int, float>> yF;
        reader.readCurveData(sample, testVar, testCompos, emptyStr, emptyStr, curveIds, frameIdSet, cnames, yD, yF);
        QString bad;
        bool finite = checkFiniteVec(yD, bad);
        if (!yD.isEmpty() && finite)
            reportPass("readCurveData", QString("curves=%1 ids=%2").arg(yD.size()).arg(yD.constBegin().value().size()));
        else if (yD.isEmpty() && nframe <= 1)
            reportSkip("readCurveData", "empty result, single frame");
        else
            reportFail("readCurveData", QString("empty=%1 finite=%2 %3").arg(yD.isEmpty()).arg(finite).arg(bad));
    }

    // ---- 接口 9: readTimeVarCurveData ----
    {
        QStringList cnames;
        QVector<double> xD;
        QHash<QString, QVector<double>> yD;
        QHash<QString, QVector<float>> yF;
        reader.readTimeVarCurveData(sample, testVar, testCompos, emptyStr, emptyStr, curveIds, cnames, xD, yD, yF);
        QString bad;
        bool finite = checkFiniteVec(yD, bad);
        if (!yD.isEmpty() && finite)
            reportPass("readTimeVarCurveData", QString("curves=%1 xvalues=%2").arg(yD.size()).arg(xD.size()));
        else if (yD.isEmpty() && nframe <= 1)
            reportSkip("readTimeVarCurveData", "empty result, single frame");
        else
            reportFail("readTimeVarCurveData", QString("empty=%1 finite=%2 %3").arg(yD.isEmpty()).arg(finite).arg(bad));
    }

    // ---- 接口 10: readVarVarCurveData ----
    {
        QStringList cnames;
        QVector<double> xD;
        QVector<float> xF;
        QHash<QString, QVector<double>> yD;
        QHash<QString, QVector<float>> yF;
        reader.readVarVarCurveData(sample, testVar, testCompo, emptyStr, emptyStr,
            testVar, testCompos, emptyStr, emptyStr, curveIds, cnames, xD, xF, yD, yF);
        QString bad;
        bool finite = checkFiniteVec(yD, bad);
        if (!yD.isEmpty() && finite)
            reportPass("readVarVarCurveData", QString("curves=%1 xvalues=%2").arg(yD.size()).arg(xD.size()));
        else if (yD.isEmpty() && nframe <= 1)
            reportSkip("readVarVarCurveData", "empty result, single frame");
        else
            reportFail("readVarVarCurveData", QString("empty=%1 finite=%2 %3").arg(yD.isEmpty()).arg(finite).arg(bad));
    }

    // ---- 接口 11: readTimeVarCurve（CurveKey） ----
    {
        CurveKey keyGen;
        QList<CurveKey> keys = keyGen.generateCurveKeys(testCompos, {}, {});
        QStringList cnames;
        QVector<double> xD;
        QHash<QString, QVector<double>> yD;
        QHash<QString, QVector<float>> yF;
        reader.readTimeVarCurve(sample, testVar, keys, curveIds, cnames, xD, yD, yF);
        QString bad;
        bool finite = checkFiniteVec(yD, bad);
        if (!yD.isEmpty() && finite)
            reportPass("readTimeVarCurve", QString("curves=%1 xvalues=%2").arg(yD.size()).arg(xD.size()));
        else if (yD.isEmpty() && nframe <= 1)
            reportSkip("readTimeVarCurve", "empty result, single frame");
        else
            reportFail("readTimeVarCurve", QString("empty=%1 finite=%2 %3").arg(yD.isEmpty()).arg(finite).arg(bad));
    }

    // ---- 接口 12: readIDVarCurve（CurveKey） ----
    {
        CurveKey keyGen;
        QList<CurveKey> keys = keyGen.generateCurveKeys(testCompos, {}, {});
        QStringList cnames;
        QHash<QString, QHash<int, double>> yD;
        QHash<QString, QHash<int, float>> yF;
        reader.readIDVarCurve(sample, testVar, keys, curveIds, frameIdSet, cnames, yD, yF, "All");
        QString bad;
        bool finite = checkFiniteVec(yD, bad);
        if (!yD.isEmpty() && finite)
            reportPass("readIDVarCurve", QString("curves=%1 ids=%2").arg(yD.size()).arg(yD.constBegin().value().size()));
        else if (yD.isEmpty() && nframe <= 1)
            reportSkip("readIDVarCurve", "empty result, single frame");
        else
            reportFail("readIDVarCurve", QString("empty=%1 finite=%2 %3").arg(yD.isEmpty()).arg(finite).arg(bad));
    }

    out() << QString("\n==== summary: pass=%1 fail=%2 skip=%3 ====\n")
                 .arg(g_passCount).arg(g_failCount).arg(g_skipCount);
    out().flush();

    // 模拟 GUI 关闭流程：clearDataBasesData 双重释放检测
    if (qEnvironmentVariableIsSet("MXDB_VERIFY_CLEAR")) {
        out() << "calling clearDataBasesData...\n";
        out().flush();
        db->clearDataBasesData();
        out() << "clearDataBasesData OK\n";
        out().flush();
    }
    return g_failCount == 0 ? 0 : 1;
}
