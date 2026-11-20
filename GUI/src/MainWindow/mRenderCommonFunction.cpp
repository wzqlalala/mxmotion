#include <qstring.h>
#include <qmainwindow.h>
#include <qtemporarydir.h>
#include <mxr/view/mBaseRend.h>
#include <qprocess.h>
#include "mmainwindow_global.h"
#include "mRenderCommonFunction.h"
#include "mOtherFunction.h"

namespace MechMainWindow
{
	void FastCapture(mxr::mBaseRend* rendWnd, const QString& name)
	{
		// 该方法仅限窗口显示出来
		int x = 0, y = 0;
		int w = rendWnd->width();
		int h = rendWnd->height();

		// 拿到顶层窗口
		QWidget* top = rendWnd->parentWidget();
		while (top && top->parentWidget())
		{
			top = top->parentWidget();
		}
		if (top)
		{
			const auto& rect = rendWnd->mapTo(top->window(), QPoint(0, 0));
			x = rect.x();
			y = top->size().height() - h - rect.y();
		}

		for (int i = 0; i < 10; i++)
			rendWnd->update();


		//// 1. 把当前 COLOR_BUFFER 读到内存
		//std::vector<GLubyte> pixels(w * h * 4);
		//glPixelStorei(GL_PACK_ALIGNMENT, 1);
		//glReadPixels(x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

		//// 2. 转 QImage 并保存
		//QImage img(pixels.data(), w, h, QImage::Format_RGBA8888);
		//img = img.mirrored(false, true);
		//img.save(name);
	}
	
	void FastCaptureVideo(const QString& imgFolderPath, const QString& imgName, const QString& suffix, const QString& outVideoPath, const QString& ffmpegExePath)
	{
		QFileInfo file(outVideoPath);
		QString fmt = file.suffix();
		// 输出视频
		int fps = 24;
		QTemporaryDir tmp;
		QString seq = imgFolderPath + "/" + imgName + "_%8d" + suffix;	// 通配符
		if (seq.isEmpty()) return;

		QStringList args;
		args << "-framerate" << MxFunctions::mNumberToString(fps)
			<< "-i" << seq;
		if (fmt == "avi")
		{
			args << "-pix_fmt" << "yuvj420p";          // avi->mjpeg
			args << "-q:v" << "2";
		}
		else if (fmt == "mpeg")
		{
			args << "-f" << "mpegts";
		}
		else
			args << "-pix_fmt" << "yuv420p";          // 兼容性最好


		if (fmt == "mp4")        args << "-c:v" << "libx264";
		else if (fmt == "gif")   args << "-vf" << "fps=" + MxFunctions::mNumberToString(fps);
		else if (fmt == "mpeg")  args << "-c:v" << "mpeg2video";
		else if (fmt == "avi")        args << "-c:v" << "mjpeg";

		args << "-y" << outVideoPath;                  // 覆盖输出

		QProcess ffmpeg;
		ffmpeg.setProcessChannelMode(QProcess::ForwardedChannels);
		ffmpeg.start(ffmpegExePath, args);
		if (!ffmpeg.waitForFinished(-1)) {
			qDebug() << ffmpeg.readAllStandardError();
			return;
		}
		qDebug() << ffmpegExePath << args;
	}
}
