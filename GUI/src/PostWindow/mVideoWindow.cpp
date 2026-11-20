
#include "mVideoWindow.h"
#include "mPostDockProvider.h"
#include "mAppConfiguration.h"
#include "mGlobalSignals.h"
#include "mTitleBar.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
#include <libavutil/time.h>
#ifdef __cplusplus
}
#endif

//Qt
#include <QComboBox>
#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QTextCodec>
#include <QMessageBox>
#include <QLabel>
#include <QCheckBox>
#include <QFont>
#include <QSplitter>
#include <QLibraryInfo>
#include <QFile>
#include <QtGlobal>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <algorithm>
#include <cstring>
#include <vector>

#include "mMechGlobalSignals.h"
#include "mDataStatusManager.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mPhysicalFieldEnum.h"
#include "mPostDataBase.h"
#include "mPostDataManager.h"
#include "mxr/view/REBUILD_PostRend.h"

using namespace MxFunctions;
using namespace MechCommon;
using namespace PostData;
using namespace MechPreData;


// ===================== DecodeThread =====================
namespace PostWindow
{
DecodeThread::DecodeThread(QObject* parent)
    : QThread(parent)
{
}

DecodeThread::~DecodeThread()
{
    stopDecode();
    wait();
    cleanFFmpeg();
}

// MSVC兼容：替代av_err2str，获取ffmpeg错误文本
static QString ffmpegErrorStr(int ret)
{
    char buf[AV_ERROR_MAX_STRING_SIZE] = { 0 };
    av_strerror(ret, buf, AV_ERROR_MAX_STRING_SIZE);
    return QString::fromUtf8(buf);
}

bool DecodeThread::setFile(const QString& path)
{
    //调用约定：仅 GUI 线程且线程未运行时调用（FFmpeg 上下文所有权归解码线程）
    cleanFFmpeg();
    m_filePath = path;

    {
        QMutexLocker lock(&m_cmdMutex);
        //会话态复位（新文件=新播放会话）；m_loopPlay/m_forceFps 是用户偏好
        //（面板循环勾选/帧速率），跨文件保留——原随本块清零，面板构造期推送的
        //"循环开"在导入时被冲掉，表现为勾了循环仍播完即停
        m_transport = Transport::Idle;
        m_exitThread = false;
        m_seekRequest = false;
        m_seekTargetSec = 0.0;
    }
    m_atEof = false;
    m_lastPtsSec = -1.0;

    int ret = avformat_open_input(&m_fmtCtx, m_filePath.toUtf8().constData(), nullptr, nullptr);
    if (ret < 0)
    {
        qWarning() << "avformat_open_input failed:" << ffmpegErrorStr(ret);
        return false;
    }
    ret = avformat_find_stream_info(m_fmtCtx, nullptr);
    if (ret < 0)
    {
        qWarning() << "avformat_find_stream_info fail";
        return false;
    }

    m_videoStreamIndex = av_find_best_stream(m_fmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &m_codec, 0);
    if (m_videoStreamIndex < 0)
    {
        qWarning() << "no video stream";
        return false;
    }

    AVRational avgRate = m_fmtCtx->streams[m_videoStreamIndex]->avg_frame_rate;
    m_originalFps = av_q2d(avgRate);
    if (m_originalFps <= 1e-6)
    {
        //个别封装 avg_frame_rate 缺失，退回 r_frame_rate
        m_originalFps = av_q2d(m_fmtCtx->streams[m_videoStreamIndex]->r_frame_rate);
    }
    m_durationSec = m_fmtCtx->duration > 0 ? m_fmtCtx->duration / (double)AV_TIME_BASE : 0.0;
    if (m_durationSec <= 0 && m_originalFps > 1e-6)
    {
        //个别封装容器 duration 无效：按流帧数/帧率估算，避免上层误判"无视频"
        int64_t nb = m_fmtCtx->streams[m_videoStreamIndex]->nb_frames;
        if (nb > 0)
            m_durationSec = nb / m_originalFps;
    }

    m_codecCtx = avcodec_alloc_context3(m_codec);
    avcodec_parameters_to_context(m_codecCtx, m_fmtCtx->streams[m_videoStreamIndex]->codecpar);
    ret = avcodec_open2(m_codecCtx, m_codec, nullptr);
    if (ret < 0)
    {
        qWarning() << "avcodec_open2 fail";
        return false;
    }

    m_frameRaw = av_frame_alloc();
    m_frameRgb = av_frame_alloc();
    m_pkt = av_packet_alloc();

    emit sigVideoFpsReady(m_originalFps);
    emit sigStreamInfo(m_durationSec);
    return true;
}

void DecodeThread::stopDecode()
{
    {
        QMutexLocker lock(&m_cmdMutex);
        m_exitThread = true;
    }
    m_cmdWait.wakeAll();
}

void DecodeThread::play()
{
    {
        QMutexLocker lock(&m_cmdMutex);
        //非循环播放到 EOF 后再 play：先回绕（seek 请求通道由解码线程执行）
        if (m_atEof)
        {
            m_seekRequest = true;
            m_seekTargetSec = 0.0;
        }
        m_transport = Transport::Playing;
    }
    m_cmdWait.wakeAll();
    //播放态需主动上报：run() 仅在 EOF 时发 false，不发 true 则窗口 m_isPlaying
    //全程为 false，控制条/对比面板按钮镜像失真
    emit sigPlayStateChanged(true);
    if (!isRunning())
        start();
}

void DecodeThread::pause()
{
    {
        QMutexLocker lock(&m_cmdMutex);
        m_transport = Transport::Paused;
    }
    m_cmdWait.wakeAll();
    emit sigPlayStateChanged(false);
}

void DecodeThread::togglePlay()
{
    if (isPlaying())
        pause();
    else
        play();
}

void DecodeThread::rewind()
{
    {
        QMutexLocker lock(&m_cmdMutex);
        m_transport = Transport::Paused;
        m_seekRequest = true;
        m_seekTargetSec = 0.0;
    }
    m_atEof = false;
    m_cmdWait.wakeAll();
    emit sigPlayStateChanged(false);
    if (!isRunning())
        start();
}

void DecodeThread::requestFrameAtTime(double sec)
{
    {
        QMutexLocker lock(&m_cmdMutex);
        //未处理的新请求覆盖旧请求——动画快速换帧时跳到最新目标（丢中间帧）
        m_seekRequest = true;
        m_seekTargetSec = sec;
    }
    m_cmdWait.wakeAll();
    if (!isRunning())
        start();
}

bool DecodeThread::isPlaying() const
{
    QMutexLocker lock(&m_cmdMutex);
    return m_transport == Transport::Playing;
}

void DecodeThread::setLoopPlay(bool enable)
{
    QMutexLocker lock(&m_cmdMutex);
    m_loopPlay = enable;
}

bool DecodeThread::isLoopPlay() const
{
    QMutexLocker lock(&m_cmdMutex);
    return m_loopPlay;
}

double DecodeThread::getOriginalFps() const
{
    return m_originalFps;
}

double DecodeThread::getDurationSec() const
{
    return m_durationSec;
}

void DecodeThread::setForceFps(double fps)
{
    QMutexLocker lock(&m_cmdMutex);
    m_forceFps = fps;
}

double DecodeThread::getForceFps() const
{
    QMutexLocker lock(&m_cmdMutex);
    return m_forceFps;
}

bool DecodeThread::seekToStart()
{
    if (!m_fmtCtx) return false;
    int ret = av_seek_frame(m_fmtCtx, m_videoStreamIndex, 0, AVSEEK_FLAG_BACKWARD);
    if (ret < 0) {
        qWarning() << "seek to start failed:" << ffmpegErrorStr(ret);
        return false;
    }
    //冲刷解码器缓存，防止残留旧帧花屏
    avcodec_flush_buffers(m_codecCtx);
    return true;
}

void DecodeThread::cleanFFmpeg()
{
    if (m_rgbBuffer)
    {
        av_free(m_rgbBuffer);
        m_rgbBuffer = nullptr;
    }
    if (m_swsCtx)
    {
        sws_freeContext(m_swsCtx);
        m_swsCtx = nullptr;
    }
    if (m_pkt)
    {
        av_packet_free(&m_pkt);
        m_pkt = nullptr;
    }
    if (m_frameRaw)
    {
        av_frame_free(&m_frameRaw);
        m_frameRaw = nullptr;
    }
    if (m_frameRgb)
    {
        av_frame_free(&m_frameRgb);
        m_frameRgb = nullptr;
    }
    if (m_codecCtx)
    {
        avcodec_close(m_codecCtx);
        avcodec_free_context(&m_codecCtx);
    }
    if (m_fmtCtx)
    {
        avformat_close_input(&m_fmtCtx);
    }
    m_videoStreamIndex = -1;
    m_originalFps = 0.0;
    m_durationSec = 0.0;
}

bool DecodeThread::decodeNextFrame(double& ptsSec, QImage* outImg)
{
    if (!m_fmtCtx || m_videoStreamIndex < 0 || !m_codecCtx)
        return false;
    AVRational tb = m_fmtCtx->streams[m_videoStreamIndex]->time_base;
    while (true)
    {
        {
            QMutexLocker lock(&m_cmdMutex);
            if (m_exitThread) return false;
        }
        int ret = av_read_frame(m_fmtCtx, m_pkt);
        if (ret < 0)
            return false;   //EOF 或读错误
        bool isVideo = (m_pkt->stream_index == m_videoStreamIndex);
        if (isVideo)
            avcodec_send_packet(m_codecCtx, m_pkt);
        av_packet_unref(m_pkt);
        if (!isVideo)
            continue;
        while (true)
        {
            ret = avcodec_receive_frame(m_codecCtx, m_frameRaw);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                break;      //EAGAIN：继续读下一包
            if (ret < 0)
            {
                qWarning() << "decode frame error, ret=" << ret;
                return false;
            }
            ptsSec = (m_frameRaw->pts != AV_NOPTS_VALUE)
                ? av_q2d(tb) * m_frameRaw->pts : 0.0;
            //sws/rgb 资源仅在 run() 开头创建：setFile 换文件后若线程未重启，
            //此处指针悬空——与 emitFrame 的防护对齐，宁丢帧不崩溃
            if (outImg != nullptr && m_swsCtx && m_rgbBuffer)
            {
                sws_scale(m_swsCtx,
                    m_frameRaw->data, m_frameRaw->linesize,
                    0, m_codecCtx->height,
                    m_frameRgb->data, m_frameRgb->linesize);
                QImage img(m_frameRgb->data[0], m_codecCtx->width, m_codecCtx->height,
                    m_frameRgb->linesize[0], QImage::Format_RGB888);
                *outImg = img.copy();
            }
            return true;
        }
    }
}

void DecodeThread::emitFrame(double ptsSec)
{
    if (!m_swsCtx || !m_rgbBuffer)
        return;
    sws_scale(m_swsCtx,
        m_frameRaw->data, m_frameRaw->linesize,
        0, m_codecCtx->height,
        m_frameRgb->data, m_frameRgb->linesize);
    QImage img(m_frameRgb->data[0], m_codecCtx->width, m_codecCtx->height,
        m_frameRgb->linesize[0], QImage::Format_RGB888);
    m_lastPtsSec = ptsSec;
    emit sigNewFrame(img.copy(), ptsSec);
}

bool DecodeThread::serveFrameAt(double sec)
{
    if (!m_fmtCtx || m_videoStreamIndex < 0 || !m_codecCtx)
        return false;
    //同步/拖动取帧必须完整解码：快进加速的丢非参考帧策略（run 节奏块按帧设置）
    //会把目标时刻的非参考帧跳掉导致取帧偏移——本路径恒恢复完整解码
    m_codecCtx->skip_frame = AVDISCARD_DEFAULT;
    m_codecCtx->skip_loop_filter = AVDISCARD_DEFAULT;
    if (sec < 0) sec = 0;
    if (m_durationSec > 0 && sec > m_durationSec) sec = m_durationSec;
    AVRational tb = m_fmtCtx->streams[m_videoStreamIndex]->time_base;

    //前向小步优化：目标在当前帧之后且间隔 <1s，免 seek 顺序解码
    //（动画同步逐帧推进时高频命中，避免每次 av_seek 的关键帧回退+前向解码开销）
    bool forward = m_lastPtsSec >= 0 && sec > m_lastPtsSec && (sec - m_lastPtsSec) < 1.0;
    if (!forward)
    {
        int64_t ts = av_rescale_q((int64_t)(sec * AV_TIME_BASE), AVRational{ 1, AV_TIME_BASE }, tb);
        int ret = av_seek_frame(m_fmtCtx, m_videoStreamIndex, ts, AVSEEK_FLAG_BACKWARD);
        if (ret < 0)
        {
            qWarning() << "seek to" << sec << "failed:" << ffmpegErrorStr(ret);
            return false;
        }
        avcodec_flush_buffers(m_codecCtx);
    }

    //前向解码到"覆盖目标时刻"的帧：取最后一帧 pts<=sec+eps；越过目标即停
    double eps = m_originalFps > 1e-6 ? 0.5 / m_originalFps : 0.02;
    QImage bestImg;
    double bestPts = -1.0;
    bool have = false;
    while (true)
    {
        {
            QMutexLocker lock(&m_cmdMutex);
            if (m_exitThread) return false;
        }
        double pts = 0.0;
        QImage img;
        if (!decodeNextFrame(pts, &img))
            break;      //EOF：目标在片尾之后，用已解到的最后帧
        if (pts <= sec + eps)
        {
            bestImg = img;
            bestPts = pts;
            have = true;
        }
        else
        {
            if (!have)
            {
                //目标在首帧之前（seek BACKWARD 落点首帧已越过目标）：用首帧
                bestImg = img;
                bestPts = pts;
                have = true;
            }
            break;
        }
    }
    if (!have)
        return false;
    m_atEof = false;
    m_lastPtsSec = bestPts;
    emit sigNewFrame(bestImg, bestPts);
    return true;
}

bool DecodeThread::sleepUntil(int64_t targetUs)
{
    while (true)
    {
        int64_t now = av_gettime();
        if (now >= targetUs)
            return true;
        bool playing = false;
        {
            QMutexLocker lock(&m_cmdMutex);
            playing = (m_transport == Transport::Playing) && !m_exitThread && !m_seekRequest;
        }
        if (!playing)
            return false;  //暂停/退出/新 seek 请求：中断等待
        int64_t chunkUs = targetUs - now;
        if (chunkUs > 1000) chunkUs = 1000;
        av_usleep((unsigned)chunkUs);
    }
}

void DecodeThread::run()
{
    if (!m_fmtCtx || m_videoStreamIndex < 0 || !m_codecCtx)
        return;

    int width = m_codecCtx->width;
    int height = m_codecCtx->height;

    //线程停后重启（未换文件）会再次走到这里：先释放旧资源防泄漏
    //（cleanFFmpeg 仅在 GUI 线程且线程已 wait() 停稳后调用，此处无并发）
    if (m_swsCtx)
    {
        sws_freeContext(m_swsCtx);
        m_swsCtx = nullptr;
    }
    if (m_rgbBuffer)
    {
        av_free(m_rgbBuffer);
        m_rgbBuffer = nullptr;
    }
    m_swsCtx = sws_getContext(width, height, m_codecCtx->pix_fmt,
        width, height, AV_PIX_FMT_RGB24,
        SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!m_swsCtx)
    {
        qWarning() << "sws_getContext failed";
        return;
    }

    int rgbSize = av_image_get_buffer_size(AV_PIX_FMT_RGB24, width, height, 1);
    m_rgbBuffer = (uint8_t*)av_malloc(rgbSize);
    av_image_fill_arrays(m_frameRgb->data, m_frameRgb->linesize, m_rgbBuffer,
        AV_PIX_FMT_RGB24, width, height, 1);

    seekToStart();
    serveFrameAt(0.0);   //打开即显示首帧（暂停态停驻）

    Transport prevTr = Transport::Paused;
    int64_t playStartUs = av_gettime();  //播放段节奏基准（恢复播放/seek 后重置）
    int64_t frameCounter = 0;            //强制帧率模式帧计数

    forever
    {
        // ---- 命令等待 ----
        bool seekReq = false;
        double seekTarget = 0.0;
        {
            QMutexLocker lock(&m_cmdMutex);
            while (!m_exitThread && m_transport != Transport::Playing && !m_seekRequest)
                m_cmdWait.wait(&m_cmdMutex);
            if (m_exitThread)
                break;
            seekReq = m_seekRequest;
            seekTarget = m_seekTargetSec;
            if (seekReq)
                m_seekRequest = false;
        }

        if (seekReq)
        {
            serveFrameAt(seekTarget);
            //seek 后播放节奏以新位置为基准
            playStartUs = av_gettime() - (int64_t)(m_lastPtsSec * 1e6);
            frameCounter = 0;
            continue;
        }

        // ---- 暂停后恢复/首次播放：以当前帧时间重建节奏基准（免追帧） ----
        if (prevTr != Transport::Playing)
        {
            playStartUs = av_gettime() - (int64_t)(m_lastPtsSec * 1e6);
            frameCounter = 0;
        }
        prevTr = Transport::Playing;

        double pts = 0.0;
        if (!decodeNextFrame(pts))
        {
            //EOF
            m_atEof = true;
            bool loop = false;
            {
                QMutexLocker lock(&m_cmdMutex);
                loop = m_loopPlay;
                m_transport = loop ? Transport::Playing : Transport::Paused;
            }
            if (loop)
            {
                seekToStart();
                m_atEof = false;
                m_lastPtsSec = -1.0;
                playStartUs = av_gettime();
                frameCounter = 0;
                prevTr = Transport::Paused;   //强制下轮重置节奏基准
            }
            else
            {
                emit sigPlayFinished();
                emit sigPlayStateChanged(false);
                prevTr = Transport::Paused;
            }
            continue;
        }

        // ---- 帧率节奏 ----
        double forceFps = -1.0;
        {
            QMutexLocker lock(&m_cmdMutex);
            forceFps = m_forceFps;
        }
        //快进解码加速：强制帧率显著高于原始（>1.5×）时丢非参考帧+跳环路面滤波
        //——实测反馈"30 设 300 提速不明显"根因=解码吞吐瓶颈（每帧解码+RGB 转换
        //+整帧拷贝串行 ~8-15ms，全速仅 70-120fps，目标速率填不满）；丢非参考帧
        //（B 帧）不破坏参考链不花屏，画质略降属快进语义可接受，解码提速 2-5×。
        //codecCtx 仅解码线程访问，此处直接写安全；恢复原始节奏时还原
        const bool turbo = forceFps > 1e-6 && m_originalFps > 1e-6
            && forceFps > m_originalFps * 1.5;
        m_codecCtx->skip_frame = turbo ? AVDISCARD_NONREF : AVDISCARD_DEFAULT;
        m_codecCtx->skip_loop_filter = turbo ? AVDISCARD_ALL : AVDISCARD_DEFAULT;
        bool paced = true;
        if (forceFps > 1e-6)
        {
            int64_t intervalUs = (int64_t)(1000000.0 / forceFps);
            paced = sleepUntil(playStartUs + frameCounter * intervalUs);
            frameCounter++;
        }
        else if (m_originalFps > 1e-6)
        {
            paced = sleepUntil(playStartUs + (int64_t)(pts * 1e6));
        }

        if (!paced)
        {
            //等待被打断（暂停/新 seek 请求）：当前帧丢弃，回到命令等待
            prevTr = Transport::Paused;
            continue;
        }

        emitFrame(pts);
    }

    //FFmpeg 资源不在run内清理：线程停止后由 setFile/cleanFFmpeg 统一处理
}
} // namespace PostWindow


namespace PostWindow
{
    // ===================== mVideoCanvas =====================
    mVideoCanvas::mVideoCanvas(QWidget* parent)
        : QWidget(parent)
    {
        setAttribute(Qt::WA_OpaquePaintEvent);
        setMinimumSize(QSize(160, 90));
    }

    void mVideoCanvas::setFrame(const QImage& img)
    {
        m_frame = img;
        m_dirty = true;
        update();
    }

    void mVideoCanvas::clearFrame()
    {
        m_frame = QImage();
        m_scaled = QImage();
        m_dirty = true;
        update();
    }

    void mVideoCanvas::setFillViewport(bool fill)
    {
        if (m_fillViewport == fill)
            return;
        m_fillViewport = fill;
        m_dirty = true;   //缩放模式变化，缓存失效
        update();
    }

    void mVideoCanvas::paintEvent(QPaintEvent* event)
    {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.fillRect(this->rect(), Qt::black);
        if (m_frame.isNull())
            return;

        //缩放缓存：仅帧更新/画布尺寸变化/模式切换时重算（原实现每帧实时 SmoothTransformation）
        if (m_dirty || m_scaled.isNull() || m_scaledForSize != this->size())
        {
            //铺满模式（独立视口形态）：拉伸填满格子无视宽高比（用户要求"视频铺满
            //整个视口不留黑边"）；其余形态保持宽高比（对比场景比例忠实优先）
            m_scaled = m_frame.scaled(this->size(),
                m_fillViewport ? Qt::IgnoreAspectRatio : Qt::KeepAspectRatio,
                Qt::SmoothTransformation);
            m_scaledForSize = this->size();
            m_dirty = false;
        }
        //居中绘制（原缺陷：KeepAspectRatio 缩出留边小图后又 drawImage(this->rect())
        //拉伸回全窗，比例保持失效、画面变形）
        int x = (this->width() - m_scaled.width()) / 2;
        int y = (this->height() - m_scaled.height()) / 2;
        painter.drawImage(QPoint(x, y), m_scaled);
    }

    // ===================== mVideoOverlayCanvas =====================
    mVideoOverlayCanvas::mVideoOverlayCanvas(QWidget* parent)
        : QWidget(parent)
    {
        //点击穿透：叠加层不挡视口相机操作（旋转/缩放/拾取直达渲染表面）
        setAttribute(Qt::WA_TransparentForMouseEvents);
        //未绘制区域透出视口内容（半透明子件合成，mAnimationRecordWidget 覆盖件先例）
        setAttribute(Qt::WA_TranslucentBackground);
        setMinimumSize(QSize(32, 18));
    }

    void mVideoOverlayCanvas::setFrame(const QImage& img)
    {
        m_frame = img;
        m_dirty = true;
        update();
    }

    void mVideoOverlayCanvas::clearFrame()
    {
        m_frame = QImage();
        m_scaled = QImage();
        m_dirty = true;
        update();
    }

    void mVideoOverlayCanvas::setOpacityPct(int pct)
    {
        if (pct < 10) pct = 10;
        if (pct > 100) pct = 100;
        m_opacity = pct / 100.0;
        update();
    }

    void mVideoOverlayCanvas::paintEvent(QPaintEvent* event)
    {
        Q_UNUSED(event);
        if (m_frame.isNull())
            return;
        QPainter painter(this);
        //几何管理器已按帧宽高比定尺寸，此处 KeepAspectRatio+铺满等价且防边界舍入
        if (m_dirty || m_scaled.isNull() || m_scaledForSize != this->size())
        {
            m_scaled = m_frame.scaled(this->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
            m_scaledForSize = this->size();
            m_dirty = false;
        }
        painter.setOpacity(m_opacity);
        int x = (this->width() - m_scaled.width()) / 2;
        int y = (this->height() - m_scaled.height()) / 2;
        painter.drawImage(QPoint(x, y), m_scaled);
    }

    // ===================== mVideoWindow =====================
    static QString formatVideoTime(double sec)
    {
        if (sec < 0) sec = 0;
        int minutes = int(sec) / 60;
        double s = sec - minutes * 60;
        return QString("%1:%2").arg(minutes).arg(s, 4, 'f', 1, '0');
    }

    mVideoWindow* mVideoWindow::_instance = nullptr;
    mVideoWindow* mVideoWindow::getInstance()
    {
        if (_instance == nullptr)
        {
            _instance = new mVideoWindow;
            _instance->setObjectName("VideoWindow");
        }
        return _instance;
    }

    mVideoWindow::mVideoWindow(QWidget* parent)
        : QWidget(parent)
    {
        m_decodeThread = new DecodeThread(this);

        //解码线程信号（跨线程一律队列连接）
        connect(m_decodeThread, &DecodeThread::sigNewFrame,
            this, &mVideoWindow::onNewFrame, Qt::QueuedConnection);
        connect(m_decodeThread, &DecodeThread::sigVideoFpsReady,
            this, &mVideoWindow::onThreadFpsReady, Qt::QueuedConnection);
        connect(m_decodeThread, &DecodeThread::sigStreamInfo,
            this, &mVideoWindow::onThreadStreamInfo, Qt::QueuedConnection);
        connect(m_decodeThread, &DecodeThread::sigPlayFinished,
            this, &mVideoWindow::onThreadPlayFinished, Qt::QueuedConnection);
        connect(m_decodeThread, &DecodeThread::sigPlayStateChanged,
            this, &mVideoWindow::onThreadPlayStateChanged, Qt::QueuedConnection);

        // ---- 界面：画布 + 播放控制条 ----
        QVBoxLayout* rootLayout = new QVBoxLayout(this);
        rootLayout->setContentsMargins(0, 0, 0, 0);
        rootLayout->setSpacing(2);

        m_canvas = new mVideoCanvas(this);
        m_canvas->setObjectName(QString::fromUtf8("videoCanvas"));
        rootLayout->addWidget(m_canvas, 1);

        QHBoxLayout* barLayout = new QHBoxLayout();
        barLayout->setContentsMargins(4, 2, 4, 2);
        barLayout->setSpacing(4);

        m_btnPlay = new QPushButton(this);
        m_btnPlay->setObjectName(QString::fromUtf8("videoBtnPlayPause"));
        m_btnPlay->setMinimumWidth(56);
        m_btnPlay->setEnabled(false);
        barLayout->addWidget(m_btnPlay);

        m_btnStop = new QPushButton(this);
        m_btnStop->setObjectName(QString::fromUtf8("videoBtnStop"));
        m_btnStop->setMinimumWidth(48);
        m_btnStop->setEnabled(false);
        barLayout->addWidget(m_btnStop);

        m_slider = new QSlider(Qt::Horizontal, this);
        m_slider->setObjectName(QString::fromUtf8("videoSliderProgress"));
        m_slider->setRange(0, 0);
        barLayout->addWidget(m_slider, 1);

        m_labelTime = new QLabel(this);
        m_labelTime->setObjectName(QString::fromUtf8("videoLabelTime"));
        m_labelTime->setMinimumWidth(110);
        m_labelTime->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        barLayout->addWidget(m_labelTime);

        rootLayout->addLayout(barLayout);

        connect(m_btnPlay, &QPushButton::clicked, this, &mVideoWindow::onPlayClicked);
        connect(m_btnStop, &QPushButton::clicked, this, &mVideoWindow::onStopClicked);
        connect(m_slider, &QSlider::sliderMoved, this, &mVideoWindow::onSliderMoved);
        connect(m_slider, &QSlider::sliderPressed, this, [this] { m_sliderDragging = true; });
        connect(m_slider, &QSlider::sliderReleased, this, [this] { m_sliderDragging = false; });

        //动画/云图换帧全局通知 → 同步模式驱动视频（HyperView video sync 语义：
        //视频不自持播放时钟，播放/暂停/步进/拖动/循环全部由动画侧事件驱动）
        connect(MechCommon::mMechGlobalSignals::getInstance(),
            &MechCommon::mMechGlobalSignals::postCloudFrameChanged,
            this, &mVideoWindow::onCloudFrameChanged);

        //视口格子被外部删除（主窗口代办摘离隐藏后通知）→ 复位+面板回弹
        connect(MechCommon::mMechGlobalSignals::getInstance(),
            &MechCommon::mMechGlobalSignals::sig_videoViewportClosed,
            this, &mVideoWindow::onVideoViewportClosed);

        //切换界面语言
        connect(MxFunctions::mGlobalSignals::getInstance(), SIGNAL(updateTranslator(int)),
            this, SLOT(slot_changeLanguage(int)));
        slot_changeLanguage(MAppConfiguration::mAppConfiguration::getInstance()->_language);
        refreshTimeLabel();
        updateControlEnabled();
    }

    mVideoWindow::~mVideoWindow()
    {
        closeVideo();
    }

    bool mVideoWindow::openVideo(const QString& filePath)
    {
        //必须无条件先停稳线程：暂停/停驻态线程不退出 run 循环（m_isPlaying 因
        //无 true 信号本就不可靠），否则 setFile→cleanFFmpeg 释放的 sws/rgb
        //资源仍被旧线程使用，动画同步 seek 路径 sws_scale 空指针崩溃
        closeVideo();
        bool ok = m_decodeThread->setFile(filePath);
        if (ok)
        {
            m_hasVideo = true;
            m_durationSec = m_decodeThread->getDurationSec();
            m_lastPtsSec = 0.0;
            m_slider->setRange(0, int(m_durationSec * 1000));
            m_slider->setValue(0);
            refreshTimeLabel();
            //导入即显示首帧（拉模式取第 0 秒：线程启动→serve 首帧→停驻，不影响
            //后续播放节奏）——修复"导入后画面黑直到点播放"（独立视口格暴露最明显，
            //dock 时代同样存在）
            m_decodeThread->requestFrameAtTime(0.0);
            updateControlEnabled();
        }
        return ok;
    }

    void mVideoWindow::closeVideo()
    {
        if (m_decodeThread)
        {
            m_decodeThread->stopDecode();
            m_decodeThread->wait();
        }
        m_isPlaying = false;
        m_hasVideo = false;
        m_durationSec = 0.0;
        m_lastPtsSec = 0.0;
        if (m_canvas) m_canvas->clearFrame();
        if (m_overlay) m_overlay->clearFrame();   //叠加件保留（模式持续），仅清画面
        if (m_slider)
        {
            m_slider->setRange(0, 0);
            m_slider->setValue(0);
        }
        refreshTimeLabel();
        updateControlEnabled();
    }

    void mVideoWindow::play()
    {
        if (m_syncWithAnim)
            return;   //同步模式下视频由动画驱动，禁自主播放
        if (m_decodeThread)
            m_decodeThread->play();
    }

    void mVideoWindow::pause()
    {
        if (m_decodeThread)
            m_decodeThread->pause();
    }

    void mVideoWindow::togglePlay()
    {
        if (m_syncWithAnim)
            return;
        if (m_decodeThread)
            m_decodeThread->togglePlay();
    }

    void mVideoWindow::stop()
    {
        if (m_decodeThread)
            m_decodeThread->rewind();
    }

    bool mVideoWindow::isPlaying() const
    {
        return m_isPlaying;
    }

    void mVideoWindow::seekToTime(double sec)
    {
        if (m_syncWithAnim || !m_hasVideo)
            return;
        if (sec < 0) sec = 0;
        if (sec > m_durationSec) sec = m_durationSec;
        if (m_decodeThread)
            m_decodeThread->requestFrameAtTime(sec);
    }

    double mVideoWindow::getVideoOriginalFps() const
    {
        return m_cacheOriginalFps;
    }

    double mVideoWindow::getVideoDuration() const
    {
        return m_durationSec;
    }

    void mVideoWindow::setPlayFps(double fps)
    {
        if (m_decodeThread)
        {
            m_decodeThread->setForceFps(fps);
        }
    }

    void mVideoWindow::setLoopPlay(bool enable)
    {
        if (m_decodeThread)
            m_decodeThread->setLoopPlay(enable);
    }

    bool mVideoWindow::isLoopPlay() const
    {
        if (!m_decodeThread) return false;
        return m_decodeThread->isLoopPlay();
    }

    // ---- 动画同步 ----

    void mVideoWindow::setSyncWithAnimation(bool on)
    {
        if (m_syncWithAnim == on)
            return;
        m_syncWithAnim = on;
        if (on)
        {
            //开启即停自主播放；按激活视口当前云图/动画帧立即对齐一次
            pause();
            reAlignToCurrentFrame();
        }
        updateControlEnabled();
    }

    bool mVideoWindow::isSyncWithAnimation() const
    {
        return m_syncWithAnim;
    }

    void mVideoWindow::setSyncMapping(int mode)
    {
        m_syncMapping = mode;
        reAlignToCurrentFrame();
    }

    void mVideoWindow::setSyncOffset(double sec)
    {
        m_syncOffset = sec;
        reAlignToCurrentFrame();
    }

    //同步参数（开关/映射/偏移）变化后立即按云图视口当前帧重对齐一次——
    //调偏移时动画常处暂停态，无换帧事件驱动则画面不动、用户无反馈
    void mVideoWindow::reAlignToCurrentFrame()
    {
        if (!m_syncWithAnim || !m_hasVideo || m_decodeThread == nullptr)
            return;
        auto cvm = MechPreData::mViewportModelManager::getInstance()->getCurrentViewportModel();
        mxr::REBUILD_PostRend* rend = cloudRendOfCurrentModel();
        MechPreData::mPostGUIData* guidata =
            (rend != nullptr && cvm != nullptr) ? cvm->getGUIDataByPostRender(rend) : nullptr;
        if (guidata == nullptr || guidata->_cloudFrame <= 0)
            return;
        double simTime = 0.0;
        if (resolveSimTime(guidata->_cloudFrame, simTime))
            m_decodeThread->requestFrameAtTime(mapToVideoTime(simTime));
    }

    mxr::REBUILD_PostRend* mVideoWindow::cloudRendOfCurrentModel()
    {
        auto cvm = MechPreData::mViewportModelManager::getInstance()->getCurrentViewportModel();
        if (cvm == nullptr)
            return nullptr;
        //后处理模型的云图视口（用户口径：一个模型只有一个视口）：当前派生优先、
        //激活悬空/前处理视口激活时按类型遍历兜底——不依赖激活态
        QWidget* postSub = cvm->activatePostViewport();
        if (postSub == nullptr)
            return nullptr;
        MechPreData::mViewportPrivate* priv = cvm->getViewportPrivate(postSub);
        return priv ? priv->toType<mxr::REBUILD_PostRend>() : nullptr;
    }

    bool mVideoWindow::resolveSimTime(int frame, double& simTime)
    {
        auto cvm = MechPreData::mViewportModelManager::getInstance()->getCurrentViewportModel();
        if (cvm == nullptr)
            return false;
        mxr::REBUILD_PostRend* rend = cloudRendOfCurrentModel();
        if (rend == nullptr)
            return false;
        QString postname = rend->getPostDataName();
        if (postname.isEmpty())
            return false;
        QPointer<mPostDataBase> postDB =
            cvm->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postname);
        if (postDB == nullptr || frame <= 0)
            return false;

        //动画时间范围缓存（换结果文件即失效重查）——全程映射模式用
        if (m_aniPostName != postname || !m_aniRangeValid)
        {
            m_aniPostName = postname;
            m_aniRangeValid = false;
            QList<int> ids = postDB->getFrameIds();
            if (!ids.isEmpty())
            {
                auto minmax = std::minmax_element(ids.begin(), ids.end());
                double t0 = postDB->getFrameTime(*minmax.first);
                double t1 = postDB->getFrameTime(*minmax.second);
                if (t0 >= 0 && t1 > t0)
                {
                    m_aniStartTime = t0;
                    m_aniEndTime = t1;
                    m_aniRangeValid = true;
                }
            }
        }

        double t = postDB->getFrameTime(frame);
        if (t < 0)
            return false;
        simTime = t;
        return true;
    }

    double mVideoWindow::mapToVideoTime(double simTime) const
    {
        double vt = simTime;
        if (m_syncMapping == 1 && m_aniRangeValid && m_durationSec > 0)
        {
            //全程映射：动画 [起始时间,结束时间] 线性映射到视频 [0,总时长]
            vt = (simTime - m_aniStartTime) / (m_aniEndTime - m_aniStartTime) * m_durationSec;
        }
        vt += m_syncOffset;
        if (vt < 0) vt = 0;
        if (m_durationSec > 0 && vt > m_durationSec) vt = m_durationSec;
        return vt;
    }

    // ---- 视口叠加 ----

    void mVideoWindow::setOverlayToViewport(bool on)
    {
        if (m_overlayOn == on)
            return;
        m_overlayOn = on;
        if (on)
        {
            //与独立视口互斥：开叠加先关视口显示（格子摘除，本体隐藏待选形态）
            if (m_viewportDisplay)
                setViewportDisplay(false);
            attachOverlay();
            if (!m_overlay)
            {
                //无可用后处理视口：开关回弹（面板经 sigOverlayActiveChanged 同步勾选态）
                m_overlayOn = false;
            }
        }
        else
        {
            detachOverlay();
        }
    }

    bool mVideoWindow::isOverlayToViewport() const
    {
        return m_overlayOn;
    }

    bool mVideoWindow::isOverlayActive() const
    {
        return m_overlay != nullptr;
    }

    void mVideoWindow::setOverlayPosition(int pos9)
    {
        if (pos9 < 0) pos9 = 0;
        if (pos9 > 8) pos9 = 8;
        m_overlayPos9 = pos9;
        updateOverlayGeometry();
    }

    void mVideoWindow::setOverlayScalePct(int pct)
    {
        if (pct < 10) pct = 10;
        if (pct > 100) pct = 100;
        m_overlayScalePct = pct;
        updateOverlayGeometry();
    }

    void mVideoWindow::setOverlayOpacityPct(int pct)
    {
        if (pct < 10) pct = 10;
        if (pct > 100) pct = 100;
        m_overlayOpacityPct = pct;
        if (m_overlay)
            m_overlay->setOpacityPct(m_overlayOpacityPct);
    }

    // ---- 背景模式（对比视频本义：视频作动画背景）----
    //形态：视频帧画进场景背景 quad（渲染侧纹理路径，GL 内部完成——QOpenGLWidget
    //子件透明合成实测不可达，两次真机均"背景黑"）；叠加画布在背景模式下隐藏

    void mVideoWindow::setOverlayAsBackground(bool on)
    {
        if (m_overlayAsBackground == on)
            return;
        m_overlayAsBackground = on;
        if (on)
        {
            //画布让位（视频走场景纹理），缓存帧立即上屏（暂停态开背景也有画面）
            if (m_overlay)
                m_overlay->hide();
            if (m_overlayRend && !m_lastFrameImg.isNull())
                pushBackgroundFrame(m_lastFrameImg);
        }
        else
        {
            clearBackgroundFrame();
            if (m_overlay)
            {
                //画布让位期间帧只走背景纹理路径，画布内无帧——切回前景补推缓存帧
                //（暂停/动画停走态无新帧驱动，不补推则画布空画面，视频"凭空消失"）
                if (!m_lastFrameImg.isNull())
                    m_overlay->setFrame(m_lastFrameImg);
                m_overlay->show();
            }
        }
        updateOverlayGeometry();
    }

    bool mVideoWindow::isOverlayAsBackground() const
    {
        return m_overlayAsBackground;
    }

    void mVideoWindow::pushBackgroundFrame(const QImage& img)
    {
        if (img.isNull() || img.format() != QImage::Format_RGB888 || !m_overlayRend)
            return;
        //QImage bytesPerLine 按 4 字节对齐（width*3 不对齐时行尾有 padding），
        //纹理契约要求每行 w*3 紧凑——逐行拷贝
        const int w = img.width();
        const int h = img.height();
        const int bpl = img.bytesPerLine();
        const uchar* bits = img.constBits();
        std::vector<uchar> compact(size_t(w) * h * 3);
        for (int y = 0; y < h; ++y)
            std::memcpy(compact.data() + size_t(y) * w * 3,
                bits + size_t(y) * bpl, size_t(w) * 3);
        m_overlayRend->updateVideoBackgroundFrame(w, h, compact.data());
    }

    void mVideoWindow::clearBackgroundFrame()
    {
        if (m_overlayRend)
            m_overlayRend->updateVideoBackgroundFrame(0, 0, nullptr);
    }

    void mVideoWindow::attachOverlay()
    {
        if (m_overlay)
            return;
        //渲染表面是 QOpenGLWidget——其子件被 GL 纹理盖住；叠加件挂其父件作同级覆盖件并
        //raise（mPostQueryResultsDock 同款）；几何按渲染表面矩形换算（父件坐标系）
        //视口解析走 cloudRendOfCurrentModel（云图视口，不依赖激活态——激活悬空/
        //前处理视口激活时 getPostRenderWidget 派生空，叠加挂不上即切换出错根因）
        mxr::REBUILD_PostRend* rend = cloudRendOfCurrentModel();
        QWidget* osg = rend ? rend->getOsgWidget() : nullptr;
        QWidget* host = osg ? osg->parentWidget() : nullptr;
        if (!host && rend)
            host = static_cast<QWidget*>(static_cast<mxr::mBaseRend*>(rend));
        if (!host)
            return;

        m_overlay = new mVideoOverlayCanvas(host);
        m_overlay->setOpacityPct(m_overlayOpacityPct);
        m_overlayRend = rend;
        m_overlaySurface = osg ? osg : host;
        m_overlaySurface->installEventFilter(this);
        if (m_overlaySurface->parentWidget())
            m_overlaySurface->parentWidget()->installEventFilter(this);
        //宿主视口销毁→叠加件连带析构：QPointer 置空 + 意图开关回弹 + 面板勾选同步
        connect(m_overlay, &QObject::destroyed, this, [this]()
        {
            m_overlaySurface = nullptr;
            m_overlayRend = nullptr;
            m_overlayOn = false;
            emit sigOverlayActiveChanged(false);
        });
        updateOverlayGeometry();
        m_overlay->show();
        //形态按当前模式：背景模式视频走场景纹理（画布隐藏+缓存帧上屏），前景 raise
        if (m_overlayAsBackground)
        {
            m_overlay->hide();
            if (!m_lastFrameImg.isNull())
                pushBackgroundFrame(m_lastFrameImg);
        }
        else
        {
            //前景挂载同样补推缓存帧：暂停态无新帧驱动，空画布叠加等于视频消失
            if (!m_lastFrameImg.isNull())
                m_overlay->setFrame(m_lastFrameImg);
            m_overlay->raise();
        }
        emit sigOverlayActiveChanged(true);
    }

    void mVideoWindow::detachOverlay()
    {
        if (!m_overlay)
        {
            m_overlaySurface = nullptr;
            m_overlayRend = nullptr;
            return;
        }
        //还原渲染背景（背景模式可能开着）：宿主视口仍存活时还原，销毁路径
        //（destroyed 回调）m_overlayRend 已空自然跳过
        if (m_overlayAsBackground)
            clearBackgroundFrame();
        if (m_overlaySurface)
        {
            m_overlaySurface->removeEventFilter(this);
            if (m_overlaySurface->parentWidget())
                m_overlaySurface->parentWidget()->removeEventFilter(this);
        }
        m_overlaySurface = nullptr;
        m_overlayRend = nullptr;
        m_overlay->hide();
        m_overlay->deleteLater();   //destroyed 回调收尾（置空/回弹/发信号）
    }

    void mVideoWindow::updateOverlayGeometry()
    {
        if (!m_overlay || !m_overlaySurface)
            return;
        QWidget* osg = m_overlaySurface;
        //父件坐标系下的渲染表面矩形（叠加件与其同父）
        QRect base(osg->x(), osg->y(), osg->width(), osg->height());

        //背景模式：视频走场景纹理，画布保持隐藏（几何无意义）；前景模式正常定位
        if (m_overlayAsBackground)
        {
            m_overlay->hide();
            return;
        }

        const int margin = 12;
        QRect inner = base.adjusted(margin, margin, -margin, -margin);
        if (inner.width() <= 10 || inner.height() <= 10)
        {
            m_overlay->hide();
            return;
        }
        //目标框：视口百分比 × 帧宽高比（KeepAspectRatio）
        double ar = m_frameAspect > 0.01 ? m_frameAspect : 16.0 / 9.0;
        double w = inner.width() * m_overlayScalePct / 100.0;
        double h = w / ar;
        if (h > inner.height())
        {
            h = inner.height();
            w = h * ar;
        }
        //9宫格定位：列 0/1/2 → 左/中/右，行 0/1/2 → 上/中/下
        int col = m_overlayPos9 % 3;
        int row = m_overlayPos9 / 3;
        int x = inner.x() + (inner.width() - int(w)) * col / 2;
        int y = inner.y() + (inner.height() - int(h)) * row / 2;
        m_overlay->setGeometry(x, y, int(w), int(h));
        m_overlay->show();
        m_overlay->raise();
    }

    bool mVideoWindow::eventFilter(QObject* watched, QEvent* event)
    {
        //渲染表面/其父件尺寸变化 → 叠加画布重排（两处都盯：视口 splitter 拖动先动父件）
        if (event->type() == QEvent::Resize && m_overlay
            && (watched == m_overlaySurface.data()
                || (m_overlaySurface && watched == m_overlaySurface->parentWidget())))
        {
            updateOverlayGeometry();
        }
        return QWidget::eventFilter(watched, event);
    }

    // ---- 独立视口形态（图表卡片同款：本体整体进驻视口格子，主窗口代办）----

    void mVideoWindow::setViewportDisplay(bool on)
    {
        if (m_viewportDisplay == on)
            return;
        m_viewportDisplay = on;
        //铺满仅独立视口形态生效（用户可选）；其余形态恒为宽高比保持
        if (m_canvas)
            m_canvas->setFillViewport(on && m_viewportFill);
        if (on)
        {
            //与叠加互斥：开视口显示先关叠加（含背景还原/叠加件 detach）
            if (m_overlayOn)
                setOverlayToViewport(false);
            emit MechCommon::mMechGlobalSignals::getInstance()->sig_requestVideoViewportOpen();
        }
        else
        {
            emit MechCommon::mMechGlobalSignals::getInstance()->sig_requestVideoViewportClose();
        }
        updateControlEnabled();
        emit sigViewportDisplayChanged(on);
    }

    bool mVideoWindow::isViewportDisplay() const
    {
        return m_viewportDisplay;
    }

    void mVideoWindow::setViewportFill(bool fill)
    {
        if (m_viewportFill == fill)
            return;
        m_viewportFill = fill;
        if (m_canvas)
            m_canvas->setFillViewport(m_viewportDisplay && fill);
    }

    bool mVideoWindow::isViewportFill() const
    {
        return m_viewportFill;
    }

    void mVideoWindow::onVideoViewportClosed()
    {
        //格子被外部删除（主窗口已摘离本体隐藏）：意图复位并通知面板回弹
        if (!m_viewportDisplay)
            return;
        m_viewportDisplay = false;
        updateControlEnabled();
        emit sigViewportDisplayChanged(false);
    }

    // ---- 槽 ----

    void mVideoWindow::onCloudFrameChanged(int frame)
    {
        if (!m_syncWithAnim || !m_hasVideo)
            return;
        double simTime = 0.0;
        if (!resolveSimTime(frame, simTime))
            return;
        if (m_decodeThread)
            m_decodeThread->requestFrameAtTime(mapToVideoTime(simTime));
    }

    void mVideoWindow::onNewFrame(const QImage& frameImg, double ptsSec)
    {
        m_lastFrameImg = frameImg;   //缓存（背景模式开启/参数变化时重推）
        if (m_overlayAsBackground && m_overlayRend)
        {
            //背景模式：帧推渲染场景背景纹理（画布路径不用）
            pushBackgroundFrame(frameImg);
        }
        else if (m_canvas)
        {
            m_canvas->setFrame(frameImg);
        }
        if (m_overlay && !m_overlayAsBackground)
        {
            //首帧采集宽高比（几何管理用），随后按需重排
            double ar = (frameImg.height() > 0)
                ? double(frameImg.width()) / double(frameImg.height()) : 0.0;
            if (ar > 0.01 && qAbs(ar - m_frameAspect) > 0.01)
            {
                m_frameAspect = ar;
                updateOverlayGeometry();
            }
            m_overlay->setFrame(frameImg);
        }
        m_lastPtsSec = ptsSec;
        if (!m_sliderDragging)
        {
            m_slider->blockSignals(true);
            m_slider->setValue(int(ptsSec * 1000));
            m_slider->blockSignals(false);
        }
        refreshTimeLabel();
    }

    void mVideoWindow::onThreadFpsReady(double fps)
    {
        m_cacheOriginalFps = fps;
        emit sigVideoFpsReady(fps);
    }

    void mVideoWindow::onThreadStreamInfo(double durationSec)
    {
        if (durationSec > 0)
        {
            m_durationSec = durationSec;
            m_slider->setRange(0, int(m_durationSec * 1000));
            refreshTimeLabel();
            updateControlEnabled();
        }
    }

    void mVideoWindow::onThreadPlayFinished()
    {
        emit sigPlayFinished();
    }

    void mVideoWindow::onThreadPlayStateChanged(bool playing)
    {
        m_isPlaying = playing;
        refreshButtonTexts();
    }

    void mVideoWindow::onSliderMoved(int value)
    {
        if (m_syncWithAnim)
            return;
        seekToTime(value / 1000.0);
    }

    void mVideoWindow::onPlayClicked()
    {
        togglePlay();
    }

    void mVideoWindow::onStopClicked()
    {
        stop();
    }

    void mVideoWindow::slot_changeLanguage(int language)
    {
        Q_UNUSED(language);
        refreshButtonTexts();
    }

    // ---- 界面辅助 ----

    void mVideoWindow::refreshButtonTexts()
    {
        int lang = MAppConfiguration::mAppConfiguration::getInstance()->_language;
        if (lang == 1)
        {
            m_btnPlay->setText(m_isPlaying ? QString::fromUtf8("Pause") : QString::fromUtf8("Play"));
            m_btnStop->setText(QString::fromUtf8("Stop"));
        }
        else
        {
            m_btnPlay->setText(m_isPlaying ? QString::fromUtf8("暂停") : QString::fromUtf8("播放"));
            m_btnStop->setText(QString::fromUtf8("停止"));
        }
    }

    void mVideoWindow::refreshTimeLabel()
    {
        if (m_labelTime == nullptr)
            return;
        if (m_durationSec <= 0)
        {
            m_labelTime->setText(QString("0:00.0 / 0:00.0"));
            return;
        }
        m_labelTime->setText(QString("%1 / %2")
            .arg(formatVideoTime(m_lastPtsSec), formatVideoTime(m_durationSec)));
    }

    void mVideoWindow::updateControlEnabled()
    {
        //同步模式：视频由动画驱动，自主播放控制整体禁用（面板开关可恢复）
        bool manualEnabled = m_hasVideo && !m_syncWithAnim;
        if (m_btnPlay) m_btnPlay->setEnabled(manualEnabled);
        if (m_btnStop) m_btnStop->setEnabled(manualEnabled);
        if (m_slider) m_slider->setEnabled(manualEnabled);
    }
}

//视频窗口提供者自注册（"Video"dock 已取消：主窗口独立视口开格经此获取单例本体，同 PostTree 模式）
static const bool s_registerVideoDockProvider = []()
{
    MechCommon::videoDockProvider() = []() -> QWidget*
    {
        return PostWindow::mVideoWindow::getInstance();
    };
    return true;
}();
