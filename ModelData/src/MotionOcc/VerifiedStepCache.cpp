#include "VerifiedStepCache.h"
#include "CadDocument.h"

#include <BinTools.hxx>
#include <Standard_Version.hxx>

#include <QBuffer>
#include <QCryptographicHash>
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>
#include <QSysInfo>

#include <cmath>
#include <istream>
#include <limits>
#include <ostream>
#include <streambuf>
#include <utility>

namespace MotionOcc {
namespace {

constexpr qint64 cacheLimit = 512ll * 1024 * 1024;
constexpr int digestSize = 32;
constexpr quint32 partLimit = 20000;
constexpr quint32 textLimit = 256 * 1024;

QString cachePath()
{
    const auto directory = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (directory.isEmpty()) return {};
    // 固定使用一个槽位，升级版本或打开另一份 STEP 都不会累积文件。
    return QDir(directory).filePath(QStringLiteral("cad/verified-step.bin"));
}

QByteArray header(const QString& sourceSha256)
{
    // 改变导入几何、验收或离散策略时，也应更新此修订号。
    return QByteArrayLiteral("MxMotion verified STEP 1\n") + OCC_VERSION_COMPLETE + '\n'
        + QByteArray::number(int(QSysInfo::ByteOrder)) + '\n' + sourceSha256.toLatin1() + '\n';
}

bool openEntry(QFile& file, const QByteArray& expectedHeader)
{
    if (file.fileName().isEmpty() || QFileInfo(file).isSymLink() || !file.open(QIODevice::ReadOnly)) return false;
    const auto size = file.size();
    return size > expectedHeader.size() + digestSize && size <= cacheLimit
        && file.read(expectedHeader.size()) == expectedHeader;
}

class PayloadBuffer final : public QBuffer {
protected:
    qint64 writeData(const char* data, qint64 size) override
    {
        if (size < 0 || size > cacheLimit - pos()) return -1;
        return QBuffer::writeData(data, size);
    }
};

class ShapeOutput final : public std::streambuf {
public:
    explicit ShapeOutput(QIODevice& device) : m_device(device), m_begin(device.pos()) {}

protected:
    std::streamsize xsputn(const char* data, std::streamsize size) override
    {
        const auto written = m_device.write(data, size);
        return written < 0 ? 0 : written;
    }

    int_type overflow(int_type value) override
    {
        if (traits_type::eq_int_type(value, traits_type::eof())) return traits_type::not_eof(value);
        const auto byte = traits_type::to_char_type(value);
        return m_device.putChar(byte) ? value : traits_type::eof();
    }

    pos_type seekoff(off_type offset, std::ios_base::seekdir dir, std::ios_base::openmode) override
    {
        const qint64 base = dir == std::ios_base::beg ? m_begin
            : dir == std::ios_base::cur ? m_device.pos() : m_device.size();
        if (offset < m_begin - base || offset > m_device.size() - base || !m_device.seek(base + offset))
            return pos_type(off_type(-1));
        return pos_type(m_device.pos() - m_begin);
    }

    pos_type seekpos(pos_type position, std::ios_base::openmode mode) override
    {
        return seekoff(off_type(position), std::ios_base::beg, mode);
    }

private:
    QIODevice& m_device;
    qint64 m_begin;
};

class ShapeInput final : public std::streambuf {
public:
    ShapeInput(const char* bytes, quint32 size)
    {
        auto* begin = const_cast<char*>(bytes);
        setg(begin, begin, begin + size);
    }

protected:
    pos_type seekoff(off_type offset, std::ios_base::seekdir dir, std::ios_base::openmode) override
    {
        auto* base = dir == std::ios_base::beg ? eback() : dir == std::ios_base::cur ? gptr() : egptr();
        if (offset < eback() - base || offset > egptr() - base) return pos_type(off_type(-1));
        setg(eback(), base + offset, egptr());
        return pos_type(gptr() - eback());
    }

    pos_type seekpos(pos_type position, std::ios_base::openmode mode) override
    {
        return seekoff(off_type(position), std::ios_base::beg, mode);
    }
};

bool writeShape(QDataStream& stream, const TopoDS_Shape& shape)
{
    auto& device = *stream.device();
    const auto offset = device.pos();
    stream << quint32(0);
    if (stream.status() != QDataStream::Ok) return false;
    const auto begin = device.pos();
    ShapeOutput buffer(device);
    std::ostream output(&buffer);
    // 保留原始形状和验证过的米制形状，包括两者各自已有的曲面离散数据。
    BinTools::Write(shape, output, Standard_True, Standard_True, BinTools_FormatVersion_CURRENT);
    const auto end = device.pos();
    if (!output || end <= begin || !device.seek(offset)) return false;
    stream << quint32(end - begin);
    return stream.status() == QDataStream::Ok && device.seek(end);
}

bool readShape(QDataStream& stream, const QByteArray& payload, TopoDS_Shape& shape)
{
    quint32 size = 0;
    stream >> size;
    auto& device = *stream.device();
    if (stream.status() != QDataStream::Ok || size == 0 || size > device.bytesAvailable()) return false;
    ShapeInput buffer(payload.constData() + device.pos(), size);
    std::istream input(&buffer);
    BinTools::Read(shape, input);
    return !input.fail() && !shape.IsNull() && input.rdbuf()->sgetc() == std::char_traits<char>::eof()
        && device.seek(device.pos() + size);
}

bool writeText(QDataStream& stream, const QString& value)
{
    if (value.size() > int(textLimit)) return false;
    const int bytes = value.size() * int(sizeof(QChar));
    stream << quint32(value.size());
    return stream.writeRawData(reinterpret_cast<const char*>(value.constData()), bytes) == bytes;
}

bool readText(QDataStream& stream, QString& value)
{
    quint32 count = 0;
    stream >> count;
    if (stream.status() != QDataStream::Ok || count > textLimit) return false;
    const int bytes = int(count) * int(sizeof(QChar));
    if (bytes > stream.device()->bytesAvailable()) return false;
    value.resize(int(count));
    return stream.readRawData(reinterpret_cast<char*>(value.data()), bytes) == bytes;
}

template<class Value, class Writer>
bool writeList(QDataStream& stream, const std::vector<Value>& values, Writer writer)
{
    stream << quint32(values.size());
    for (const auto& value : values) {
        writer(stream, value);
        if (stream.status() != QDataStream::Ok) return false;
    }
    return stream.status() == QDataStream::Ok;
}

template<int fields, class Value, class Reader>
bool readList(QDataStream& stream, std::vector<Value>& values, quint32& remaining, Reader reader)
{
    quint32 count = 0;
    stream >> count;
    if (stream.status() != QDataStream::Ok || count > remaining
        || qint64(count) * fields * 4 > stream.device()->bytesAvailable()) return false;
    remaining -= count;
    values.resize(count);
    for (auto& value : values)
        if (!reader(stream, value) || stream.status() != QDataStream::Ok) return false;
    return true;
}

void writeVector(QDataStream& stream, const QVector3D& value)
{
    stream << value.x() << value.y() << value.z();
}

bool readVector(QDataStream& stream, QVector3D& value)
{
    float x = 0, y = 0, z = 0;
    stream >> x >> y >> z;
    if (!std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) return false;
    value = QVector3D(x, y, z);
    return true;
}

bool writeDisplay(QDataStream& stream, const DisplayMesh& mesh)
{
    const auto writeIndices = [](QDataStream& out, const auto& indices) {
        for (const auto index : indices) out << quint32(index);
    };
    return writeList(stream, mesh.vertices, writeVector) && writeList(stream, mesh.normals, writeVector)
        && writeList(stream, mesh.triangles, writeIndices) && writeList(stream, mesh.edges, writeIndices);
}

struct DisplayBudget {
    quint32 vertices = 3000000, normals = 3000000, triangles = 1000000, edges = 2000000;
};

bool readDisplay(QDataStream& stream, DisplayMesh& mesh, DisplayBudget& budget)
{
    const auto readIndices = [&mesh](QDataStream& in, auto& indices) {
        for (auto& index : indices) {
            quint32 value = 0;
            in >> value;
            if (value >= mesh.vertices.size()) return false;
            index = value;
        }
        return true;
    };
    return readList<3>(stream, mesh.vertices, budget.vertices, readVector) && !mesh.vertices.empty()
        && readList<3>(stream, mesh.normals, budget.normals, readVector)
        && (mesh.normals.empty() || mesh.normals.size() == mesh.vertices.size())
        && readList<3>(stream, mesh.triangles, budget.triangles, readIndices)
        && readList<2>(stream, mesh.edges, budget.edges, readIndices)
        && (!mesh.triangles.empty() || !mesh.edges.empty());
}

void configure(QDataStream& stream)
{
    stream.setVersion(QDataStream::Qt_5_15);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
}

} // namespace

bool VerifiedStepCache::hasEntry(const QString& sha256)
{
    try {
        QFile file(cachePath());
        return openEntry(file, header(sha256));
    } catch (...) {
        return false;
    }
}

bool VerifiedStepCache::read(const QString& sha256, CadDocument& cad)
{
    try {
        QFile file(cachePath());
        if (!openEntry(file, header(sha256))) return false;
        const auto size = file.size() - file.pos() - digestSize;
        if (size <= 0 || size > cacheLimit - digestSize) return false;
        const auto payload = file.read(size);
        const auto digest = file.read(digestSize);
        if (payload.size() != size || digest.size() != digestSize || !file.atEnd()
            || QCryptographicHash::hash(payload, QCryptographicHash::Sha256) != digest) return false;

        QDataStream stream(payload);
        configure(stream);
        CadDocument staged;
        quint32 partCount = 0, noteCount = 0;
        quint64 nextId = 0;
        stream >> partCount >> nextId >> noteCount;
        if (stream.status() != QDataStream::Ok || partCount == 0 || partCount > partLimit
            || noteCount > 2 * partLimit || nextId > quint64((std::numeric_limits<int>::max)()) + 1) return false;
        for (quint32 i = 0; i < noteCount; ++i) {
            QString note;
            if (!readText(stream, note)) return false;
            staged.m_importNotes.append(std::move(note));
        }
        staged.m_parts.reserve(partCount);
        DisplayBudget budget;
        int lastId = 0;
        for (quint32 i = 0; i < partCount; ++i) {
            CadPart part{};
            quint8 hasMetreShape = 0;
            stream >> part.id >> hasMetreShape;
            if (stream.status() != QDataStream::Ok || part.id <= lastId || quint64(part.id) >= nextId
                || hasMetreShape > 1 || !readText(stream, part.name) || part.name.isEmpty()
                || !readText(stream, part.sourceName) || !readText(stream, part.instancePath) || part.instancePath.isEmpty()
                || !readShape(stream, payload, part.shape)
                || (hasMetreShape && !readShape(stream, payload, part.metreDisplaySource))
                || !readDisplay(stream, part.display, budget)) return false;
            lastId = part.id;
            staged.m_parts.push_back(std::move(part));
        }
        if (!stream.atEnd()) return false;
        staged.m_nextId = nextId;
        cad = std::move(staged);
        return true;
    } catch (...) {
        // 持久缓存可以重建，损坏、旧版本或读入失败时继续走正常 STEP 导入。
        return false;
    }
}

void VerifiedStepCache::write(const QString& sha256, const CadDocument& cad)
{
    try {
        const auto path = cachePath();
        if (path.isEmpty() || QFileInfo(path).isSymLink() || !QDir().mkpath(QFileInfo(path).absolutePath())
            || cad.m_parts.empty() || cad.m_parts.size() > partLimit || cad.m_importNotes.size() > 2 * int(partLimit)) return;
        PayloadBuffer payload;
        if (!payload.open(QIODevice::ReadWrite)) return;
        QDataStream stream(&payload);
        configure(stream);
        stream << quint32(cad.m_parts.size()) << quint64(cad.m_nextId) << quint32(cad.m_importNotes.size());
        for (const auto& note : cad.m_importNotes)
            if (!writeText(stream, note)) return;
        for (const auto& part : cad.m_parts) {
            const bool hasMetre = !part.metreDisplaySource.IsNull();
            stream << part.id << quint8(hasMetre);
            if (!writeText(stream, part.name) || !writeText(stream, part.sourceName) || !writeText(stream, part.instancePath)
                || !writeShape(stream, part.shape) || (hasMetre && !writeShape(stream, part.metreDisplaySource))
                || !writeDisplay(stream, part.display)) return;
        }
        if (stream.status() != QDataStream::Ok) return;
        const auto key = header(sha256);
        const auto& bytes = payload.data();
        if (qint64(key.size()) + bytes.size() + digestSize > cacheLimit) return;
        const auto digest = QCryptographicHash::hash(bytes, QCryptographicHash::Sha256);
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly) || file.write(key) != key.size() || file.write(bytes) != bytes.size()
            || file.write(digest) != digest.size()) return;
        file.commit();
    } catch (...) {
        // 损坏的形状、内存不足或目录不可写都不能阻止已完成的导入。
    }
}

} // namespace MotionOcc
