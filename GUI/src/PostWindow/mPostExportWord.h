#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mpostwindow_global.h"
#include <QWidget>
#include <QListWidgetItem>
#include <QPainter>
#include <QDir>
#include "mPanelBase.h"

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;


namespace PostData
{
	class mPostExportWordData;
	struct Type_WordContent;
	class mPostReportContext;
	class mPostReportRenderer;
}

//零件信息
struct PartInfo
{
    QStringList paraNames;
    QStringList paraContents;
};

//结果信息
struct ResultInfo {
    int currentFrame;
    int frameCount;
    int nodeNum;
    int elementNum;
    QString strDiscrete;
    int intDiscrete;
    double defaultMax;
    double defaultMin;
    double modalFrequency;
    double modalEigen;
};

class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;
class QLineEdit;
class QListWidget;
class QPushButton;
class QTextEdit;
class QVBoxLayout;

namespace PostWindow
{
    class mReportPictureDialog;
    class MPOSTWINDOW_EXPORT mPostExportWordPanel : public mPanelBase
    {
        Q_OBJECT
    public:
        explicit mPostExportWordPanel(QWidget* parent = nullptr);

        /** 构造函数 */
        ~mPostExportWordPanel();

        /** 视口绑定打开（用户定案 2026-11-04）：报告不按次生成实例——数据键=当前
         * 激活后处理视口名，同视口重复打开沿用既有配置，切换视口后打开自动跟随 */
        void editExportWordForViewport(int caseid);

        void updateExportWordGUI();

        void updateExportWordPanel(int caseid);

        /** 更新截图列表删除项 */
        void updateUserImages(QString image);

        void setCurViewName(const QString& viewName);

    private:
        // 获取当前视图的导出 Word 数据
        PostData::mPostExportWordData* getExportWordData() const;

        // 获取当前视图专用的截图目录
        QString getExportWordImageDir() const;

        // 根据类型获取用户截图完整路径列表
        QStringList getPictureListByType(const QString& type) const;

        // 根据类型字符串在数据中查找对应内容项
        PostData::Type_WordContent* findWordContent(PostData::mPostExportWordData* data, const QString& type) const;

        // 阻塞/恢复所有相关控件的信号，避免在填充数据时触发槽函数
        void blockAllSignals(bool block);

        // 刷新右侧编辑器状态（根据当前内容类型从数据模型读取）
        void refreshEditorByData(const QString& contentType);

        // 当前选中的内容类型（显示文本）
        QString currentContentType() const;
        void setCurrentContentType(const QString& type);
        /** 初始化界面 */
        void initReportPanel();

        //清空输出信息
        void clearAllContainer();

        //拷贝图片文件夹
        void copyPictureFolder(QString srcPath, QString dstPath);

        //获取时间信息
        void getCurrentTime();

        //获取计算文件
        QString getmxFileName(QString dir);

        //获取模型图片文件
        void getModelImageFiles(QDir reportDir);

        //获取模型前处理信息
        /* void getModelDescription(QString dir); */

        //获取结果信息
        void getResultInfo();

        //设置大标题
        void setHeadline(QString header);

        //设置副标题
        void setSubHeading(QString header);

        //设置一级标题
        void setFirstLevelHeading(QString header);

        //设置表格标题（col:合并的单元格数目）
        void setTableHeading(QString header, int col);

        //设置表头单元格样式（scale:占所在行的比例、field:字段名）
        void setCellHeaderStyle(int col, QString scale, QString field);
        void setCellHeaderStyle(QString scale, QString field);

        //设置表格单元格内容样式
        void setCellContentStyle(int col, QString scale, QString value);
        void setCellContentStyle(QString scale, QString value);

        //表格插入图片
        void insertTableImage(int col, QString path);

        //插入图片
        void insertImage(QString path);

        //插入图题
        void insertImageTitle(QString imageTitle);

        //插入正文
        void insertText(QString text);

        //起始、结束标签
        void tableBegin(QString scale);
        void tableEnd();
        void rowBegin();
        void rowEnd();

        /** 写图片 */
        void writePictures(const QString& type);

        /** 写说明内容 */
        void writeDescriptionContent();

        /** 写模型内容 */
        void writeModelContent();

        /** 写材料内容 */
        void writeMaterialContent();

        /** 写截面属性内容 */
        void writePropertyContent();

        /** 写分析工况内容 */
        void writeCaseContent();

        /** 写载荷内容 */
        void writeLoadContent();

        /** 写边界条件内容 */
        void writeBCContent();

        /** 写连接内容 */
        void writeCNContent();

        /** 写接触内容 */
        void writeCTContent();

        /** 写结果内容 */
        void writeResultContent();

        // 收集各章节数据到 mPostReportContext（用于新架构渲染）
        void collectDescriptionContext(PostData::mPostReportContext& context);
        void collectModelContext(PostData::mPostReportContext& context);
        void collectMaterialContext(PostData::mPostReportContext& context);
        void collectPropertyContext(PostData::mPostReportContext& context);
        void collectCaseContext(PostData::mPostReportContext& context);
        void collectLoadContext(PostData::mPostReportContext& context);
        void collectBCContext(PostData::mPostReportContext& context);
        void collectCNContext(PostData::mPostReportContext& context);
        void collectCTContext(PostData::mPostReportContext& context);
        void collectResultContext(PostData::mPostReportContext& context);

        void slot_formatChanged(int index);

    private slots:

        /** 选择内容列表 */
        void slot_clickContentList(QListWidgetItem* item);

        /** 文本复选框 */
        void slot_textCheckBox(bool ischeck);

        /** 文本框 */
        void slot_textChanged();

        /** 截图复选框 */
        void slot_pictureCheckBox(bool ischeck);

        /** 保存截图按钮 */
        void slot_savePictureButton();

        /** 导出动画按钮：按所选格式(AVI/GIF/MPEG/MP4)录制当前视口动画；H3D 走数据导出 */
        void slot_exportAnimation();

        /** 动画录制结束回调 */
        void slot_animationFinished();

        /** H3D 导出：抽取网格与帧数据 → Generic ASCII → HvTrans 转 H3D */
        void exportAnimationH3D();

        /** 编辑截图按钮 */
        void slot_editPictureButton();

        /** 副标题复选框 */
        void slot_titleCheckBox(bool ischeck);

        /** 设计员复选框 */
        void slot_personCheckBox(bool ischeck);

        /** 公司复选框 */
        void slot_companyCheckBox(bool ischeck);

        /** 地址复选框 */
        void slot_addressCheckBox(bool ischeck);

        /** 电话复选框 */
        void slot_phoneCheckBox(bool ischeck);

        /** 标题信息文本改变 */
        void slot_titleTextChanged();
        void slot_userTextChanged();
        void slot_companyTextChanged();
        void slot_addressTextChanged();
        void slot_phoneTextChanged();

        void slot_changeLanguage(int language);

    public slots:
        void updatePickFilterAndSymbol(bool flag);

        /** 初始化导出格式下拉框 */
        void initFormatComboBox();

        /** 收集当前报告的上下文 */
        bool collectReportContext(PostData::mPostReportContext& context, QString& error);

        /** 查找工作目录中所有已导出的动画文件（按时间倒序），无则返回空列表 */
        QStringList findAllAnimationVideos();

        /** 生成 */
        void slot_generateButton();

        /** 自动打开按钮 */
        void slot_autoOpen();

    private:
        // 构建面板控件（折叠组/表单/按钮，语言文本由 slot_changeLanguage 设置）
        void setupUI();

        QVBoxLayout*                               _mainLayout{};					//主布局

        FoldSection*                               _contentFold{};					//报告内容
        FoldSection*                               _titleFold{};					//标题信息
        FoldSection*                               _executeFold{};					//执行

        //--------- 报告内容 ---------
        QListWidget*                               _listWidget{};					//报告内容列表
        QCheckBox*                                 _checkBox_text{};				//文本单选
        QCheckBox*                                 _checkBox_screenshot{};			//截图单选
        QTextEdit*                                 _textEdit{};					//文本编辑框
        QPushButton*                               _pushButton_saveView{};			//保存当前视图按钮
        QPushButton*                               _pushButton_editScreenshot{};	//编辑截图按钮
        QComboBox*                                 _comboBox_imageFormat{};		//截图保存格式(PNG/JPEG/BMP/TIFF)
        QComboBox*                                 _comboBox_videoFormat{};		//动画输出格式(AVI/GIF/MPEG/MP4)
        QPushButton*                               _pushButton_exportVideo{};		//导出动画按钮

        //--------- 标题信息 ---------
        QCheckBox*                                 _checkBox_subheader{};			//副标题
        QLineEdit*                                 _lineEdit_subHeader{};			//副标题
        QCheckBox*                                 _checkBox_user{};				//分析员
        QLineEdit*                                 _lineEdit_user{};				//分析员
        QCheckBox*                                 _checkBox_company{};			//公司
        QLineEdit*                                 _lineEdit_company{};			//公司
        QCheckBox*                                 _checkBox_addresa{};			//地址
        QLineEdit*                                 _lineEdit_address{};			//地址
        QCheckBox*                                 _checkBox_phone{};				//电话
        QLineEdit*                                 _lineEdit_phone{};				//电话

        //--------- 执行 ---------
        QComboBox*                                 _comboBox_format{};				//导出格式选择
        QCheckBox*                                 _pushButton_open{};				//自动打开按钮
        QPushButton*                               _pushButton_generator{};		//生成按钮

        QString										html_{};
        mReportPictureDialog*                       _pictureDialog{};				//截图窗口
        int											_contentIndex{};				//内容序号	
        QStringList									piclistModel_{};				//模型
        QStringList									piclistMaterial_{};				//材料
        QStringList									piclistProperty_{};				//截面属性
        QStringList									piclistCase_{};					//分析工况
        QStringList									piclistLoad_{};					//载荷
        QStringList									piclistBoundary_{};				//边界条件
        QStringList									piclistResult_{};				//结果
        QString										mxFileName_{};					//计算文件名字
        QString										testDate_{};				    //测试日期
        QList<PartInfo>                             listPartInfo_{};                //零件信息
        QList<PartInfo>                         listMaterialInfo_{};                //材料信息
        QList<PartInfo>                         listPropertyInfo_{};                //截面属性信息
        QList<PartInfo>                             listCaseInfo_{};                //分析工况信息
        QList<PartInfo>                             listLoadInfo_{};                //载荷信息
        QList<PartInfo>                         listBoundaryInfo_{};                //边界条件信息
        QList<PartInfo>                         listConnectInfo_{};                 //连接信息
        QList<PartInfo>                         listContactInfo_{};                 //接触信息
        ResultInfo                                  resultInfo_{};                  //结果信息
        QString                                     _curViewName{};                 // 当前导出报告的名称
        QString                                     _currentContentType{};		// 当前选中的报告内容类型（显示文本）
        QString                                     _selectedFormat{};         // 当前选中的导出格式
        QStringList								_modelPics{};				    //模型
        QStringList									_materialPics{};				//材料
        QStringList									_propertyPics{};				//截面属性
        QStringList									_casePics{};					//分析工况
        QStringList									_loadPics{};					//载荷
        QStringList									_boundaryPics{};				//边界条件
        QStringList									_connectPics{};				    //连接
        QStringList									_contactPics{};				    //接触
        QStringList									_resultPics{};				    //结果

        bool                                        _grabInited;
    };
}
