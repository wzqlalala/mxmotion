#include "NativeMotionExchange.h"
#include "CadDocument.h"
#include "NativeMotionDocument.h"
#include "NativeTopologyBinding.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTemporaryDir>

#include <exception>
#include <utility>

namespace MotionOcc {
namespace {

constexpr int exchangeVersion = 1;
constexpr int maxParts = 20000;
constexpr qint64 maxStepBytes = 128ll * 1024 * 1024;
const QLatin1String exchangeKey("geometryExchange");

struct ExchangeError { QString message; };

void require(bool valid, const QString& message)
{
    if (!valid) throw ExchangeError{message};
}

template<class Operation>
bool runExchange(const Operation& operation, QString& error)
{
    try {
        operation();
        error.clear();
        return true;
    } catch (const ExchangeError& failure) {
        error = failure.message;
    } catch (const std::exception& failure) {
        error = QString::fromUtf8(failure.what());
    }
    return false;
}

QString relativeStepPath(const QJsonValue& value)
{
    auto path = value.toString();
    path.replace('\\', '/');
    const auto suffix = QFileInfo(path).suffix().toLower();
    require(!path.isEmpty() && !path.contains(QChar::Null) && !path.contains(':')
        && !QDir::isAbsolutePath(path) && (suffix == "step" || suffix == "stp"),
        QStringLiteral("STEP 路径必须是相对于运动 JSON 的 .step 或 .stp 文件。"));
    return path;
}

QString stepHash(const QString& path, QFile* snapshot = nullptr)
{
    QFile file(path);
    require(QFileInfo(path).isFile() && file.open(QIODevice::ReadOnly),
        QStringLiteral("无法读取关联 STEP：%1").arg(path));
    const auto size = file.size();
    require(size > 0 && size <= maxStepBytes, QStringLiteral("关联 STEP 为空或超过 128 MiB。"));
    QCryptographicHash hash(QCryptographicHash::Sha256);
    qint64 read = 0;
    while (read < size) {
        const auto bytes = file.read(qMin<qint64>(1024 * 1024, size - read));
        require(!bytes.isEmpty(), QStringLiteral("关联 STEP 读取不完整：%1").arg(path));
        read += bytes.size();
        hash.addData(bytes);
        if (snapshot) require(snapshot->write(bytes) == bytes.size(), snapshot->errorString());
    }
    require(file.atEnd() && file.error() == QFile::NoError, QStringLiteral("关联 STEP 在读取时发生变化：%1").arg(path));
    return QString::fromLatin1(hash.result().toHex());
}

void importVerifiedStep(const QString& path, const QJsonValue& expected, CadDocument& cad)
{
    const auto sha256 = expected.toString().toLower();
    static const QRegularExpression hashPattern(QStringLiteral("\\A[0-9a-f]{64}\\z"));
    require(hashPattern.match(sha256).hasMatch(), QStringLiteral("关联 STEP 缺少有效的 SHA-256。"));
    QTemporaryDir temporary;
    require(temporary.isValid(), temporary.errorString());
    // Import exactly the bytes whose hash was checked, even if the companion
    // file is concurrently updated by another process.
    QFile snapshot(temporary.filePath("verified.step"));
    require(snapshot.open(QIODevice::WriteOnly), snapshot.errorString());
    require(stepHash(path, &snapshot) == sha256, QStringLiteral("关联 STEP 的 SHA-256 不匹配：%1").arg(path));
    require(snapshot.flush(), snapshot.errorString());
    snapshot.close();
    std::vector<int> imported;
    QString detail;
    require(cad.importFile(snapshot.fileName(), imported, detail), detail);
}

void restoreExchangeBindings(const QJsonObject& exchange, CadDocument& cad, NativeMotionDocument& document)
{
    const auto parts = exchange.value("parts").toArray();
    const auto bindings = exchange.value("bindings").toArray();
    require(exchange.value("parts").isArray() && !parts.isEmpty() && parts.size() <= maxParts &&
        exchange.value("bindings").isArray() && bindings.size() <= maxParts,
        QStringLiteral("运动 JSON 的 STEP 零件或运动体绑定记录无效。"));
    QMap<QString, std::pair<int, QString>> namedParts;
    for (const auto& value : parts) {
        const auto part = value.toObject();
        const int partId = part.value("partId").toInt(-1);
        const auto occurrence = part.value("occurrence").toString();
        const auto name = part.value("name").toString();
        require(partId > 0 && !occurrence.isEmpty() && occurrence.size() <= 1024 && !occurrence.contains(QChar::Null)
            && !name.isEmpty() && name.size() <= 1024 && !name.contains(QChar::Null) && !namedParts.contains(occurrence),
            QStringLiteral("运动 JSON 包含缺失或重复的 STEP 零件标识。"));
        namedParts.insert(occurrence, {partId, name});
    }
    QString detail;
    require(cad.restoreNamedStepParts(namedParts, detail), detail);
    document.bodyPartBindings.clear();
    for (const auto& value : bindings) {
        const auto row = value.toObject();
        const int bodyId = row.value("bodyId").toInt(-1);
        require(bodyId >= 0 && !document.bodyPartBindings.contains(bodyId) && row.value("partIds").isArray(),
            QStringLiteral("运动 JSON 包含无效或重复的运动体绑定。"));
        const auto partIds = row.value("partIds").toArray();
        require(partIds.size() <= parts.size(), QStringLiteral("运动体引用的零件数量无效。"));
        QVector<int> ids;
        ids.reserve(partIds.size());
        for (const auto& part : partIds) ids.append(part.toInt(-1));
        require(setNativeMotionBodyBinding(cad, document, bodyId, ids, detail), detail);
    }
    require(decodeNativeProjectTopology(cad, document.root, detail), detail);
    for (const auto& value : document.collection("joints")) {
        const auto joint = value.toObject();
        const auto type = joint.value("jointType").toString();
        const auto hasCurve = [&](const char* field) { return !joint.value(QLatin1String(field)).toArray().isEmpty(); };
        const bool complete = (type != "PointOnCurve" || hasCurve("occCurveEdges")) &&
            (type != "CurveOnCurve" || (hasCurve("occCurveIEdges") && hasCurve("occCurveJEdges"))) &&
            (type != "PointOnSurface" || joint.value("surfaceFaceIds").toArray().isEmpty() || hasCurve("occSurfaceFaces"));
        require(complete, QStringLiteral("运动副 %1 缺少经过确认的 STEP 拾取记录。").arg(joint.value("id").toInt(-1)));
    }
    document.ownsTopologyCad = true;
    require(validateNativeMotionBindings(cad, document, detail), detail);
}

void resolveReferencedGeometry(const CadDocument& cad, NativeMotionDocument& document)
{
    QVector<NativeTopologyBindingIssue> issues;
    if (!autoBindNativeMotionTopology(cad, document, issues)) {
        QStringList messages;
        for (const auto& issue : issues) messages.append(issue.message);
        require(false, messages.join('\n'));
    }
}

void restoreLegacyBindings(const CadDocument& cad, NativeMotionDocument& document)
{
    document.bodyPartBindings.clear();
    document.ownsTopologyCad = false;
    QVector<NativeMotionBindingIssue> issues;
    if (!autoBindNativeMotionBodies(cad, document, issues)) {
        QStringList messages;
        for (const auto& issue : issues) messages.append(issue.message);
        require(false, messages.join('\n'));
    }
    resolveReferencedGeometry(cad, document);
    QString detail;
    require(validateNativeMotionBindings(cad, document, detail), detail);
    document.ownsTopologyCad = true;
}

QString stepStem(const QFileInfo& output)
{
    auto stem = output.fileName();
    if (stem.endsWith(".motion.json", Qt::CaseInsensitive)) stem.chop(12);
    else if (stem.endsWith(".json", Qt::CaseInsensitive)) stem.chop(5);
    return stem.isEmpty() ? QStringLiteral("geometry") : stem;
}

QString publishCompanion(const QString& stagedPath, const QFileInfo& motionOutput)
{
    const auto directory = motionOutput.absoluteDir();
    const auto stem = stepStem(motionOutput);
    for (int i = 1; i <= 10000; ++i) {
        const auto name = i == 1 ? stem + ".step" : QStringLiteral("%1.geometry-%2.step").arg(stem).arg(i);
        const auto target = directory.filePath(name);
        if (QFile::rename(stagedPath, target)) return name;
        const QFileInfo existing(target);
        require(existing.exists() || existing.isSymLink(), QStringLiteral("无法发布关联 STEP：%1").arg(target));
    }
    require(false, QStringLiteral("关联 STEP 文件名已用尽。"));
    return {};
}

} // namespace

bool readNativeMotionExchange(const QString& motionPath, NativeMotionDocument& document,
                              CadDocument& cad, bool& hasGeometry, QString& error)
{
    return runExchange([&] {
        const auto exchangeValue = document.root.value(exchangeKey);
        const bool mapped = !exchangeValue.isUndefined();
        const auto legacy = document.root.value("generatorNotes").toObject().value("geometryBinding").toObject();
        if (!mapped && !legacy.contains("stepFile") && !legacy.contains("stepSHA256")) {
            hasGeometry = false;
            return;
        }
        const auto exchange = mapped ? exchangeValue.toObject() : legacy;
        if (mapped) require(exchange.value("version").toInt(-1) == exchangeVersion,
            QStringLiteral("不支持此运动 JSON 的几何交换版本。"));
        const auto relative = relativeStepPath(exchange.value("stepFile"));
        const auto companion = QFileInfo(motionPath).absoluteDir().absoluteFilePath(relative);
        CadDocument imported;
        importVerifiedStep(companion, exchange.value(mapped ? "sha256" : "stepSHA256"), imported);
        auto restored = document;
        if (mapped) {
            restoreExchangeBindings(exchange, imported, restored);
            restored.root.remove(exchangeKey);
        } else {
            restoreLegacyBindings(imported, restored);
        }
        cad = std::move(imported);
        document = std::move(restored);
        hasGeometry = true;
    }, error);
}

bool writeNativeMotionExchange(const QString& motionPath, const CadDocument& cad,
                               const NativeMotionDocument& document, QString& error)
{
    QString publishedPath;
    const bool written = runExchange([&] {
        QString detail;
        auto exported = document;
        exported.root.remove(exchangeKey);
        if (cad.parts().empty()) {
            require(writeNativeMotionDocument(motionPath, exported, detail, true), detail);
            return;
        }
        require(validateNativeMotionBindings(cad, exported, detail), detail);
        resolveReferencedGeometry(cad, exported);
        require(encodeNativeProjectTopology(cad, exported.root, detail), detail);

        QMap<int, QString> names;
        QJsonArray parts, bindings;
        for (const auto& part : cad.parts()) {
            const auto token = QStringLiteral("MxMotion-Part-%1").arg(part.id);
            names.insert(part.id, token);
            parts.append(QJsonObject{{"partId", part.id}, {"name", part.name}, {"occurrence", token}});
        }
        for (auto it = exported.bodyPartBindings.cbegin(); it != exported.bodyPartBindings.cend(); ++it) {
            QJsonArray ids;
            for (const int partId : it.value()) ids.append(partId);
            bindings.append(QJsonObject{{"bodyId", it.key()}, {"partIds", ids}});
        }
        QJsonObject exchange{{"version", exchangeVersion}, {"parts", parts}, {"bindings", bindings}};
        const QFileInfo output(motionPath);
        QTemporaryDir stage(output.absoluteDir().filePath(".mxmotion-exchange-XXXXXX"));
        require(stage.isValid(), stage.errorString());
        const auto stepPath = stage.filePath("geometry.step");
        require(cad.exportNamedStep(stepPath, names, detail), detail);
        CadDocument imported;
        std::vector<int> importedIds;
        require(imported.importFile(stepPath, importedIds, detail), detail);
        auto verified = exported;
        restoreExchangeBindings(exchange, imported, verified);

        exchange["sha256"] = stepHash(stepPath);
        const auto companion = publishCompanion(stepPath, output);
        publishedPath = output.absoluteDir().filePath(companion);
        exchange["stepFile"] = companion;
        exported.root[exchangeKey] = exchange;
        require(writeNativeMotionDocument(motionPath, exported, detail, true), detail);
    }, error);
    if (!written && !publishedPath.isEmpty() && !QFile::remove(publishedPath))
        error += QStringLiteral("\n无法清理未使用的 STEP：%1").arg(publishedPath);
    return written;
}

} // namespace MotionOcc
