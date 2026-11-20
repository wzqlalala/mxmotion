#pragma once

namespace MechMainWindow
{
	/// <summary>
	/// 快速截屏
	/// </summary>
	/// <param name="rendWnd">渲染窗口</param>
	/// <param name="name">图片名称+路径</param>
	static void FastCapture(mxr::mBaseRend* rendWnd, const QString& name);

	/// <summary>
	/// 输出视频文件,配合FastCapture使用
	/// </summary>
	/// <param name="imgPath">图片所在文件夹</param>
	/// <param name="imgFolderPath">图片名称，例如HelloWorld_00000001, name就是HelloWorld</param>
	/// <param name="suffix">图片格式后缀 ".jpg",".jegp" ...等</param>
	/// <param name="outVideoPath">视频文件输出路径,支持格式:gif ,mp4,avi,mpeg</param>
	/// <param name="ffmpegExePath">ffmpeg程序的路径</param>
	static void FastCaptureVideo(
		const QString& imgFolderPath, const QString& imgName, const QString& suffix,
		const QString& outVideoPath,
		const QString& ffmpegExePath);
}