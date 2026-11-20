#pragma once

//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mDockBase.h"
#include <QFrame>
#include <QSize>

#include "mBasicEnum.h"
#include <set>

#include <QLineEdit>
#include <QVBoxLayout>
#include <QPushButton>
#include <QToolButton>
#include <QTableWidget>
#include <QScrollArea>
#include <QPropertyAnimation>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <memory>
#include <QString>
#include <QVector>
#include <QStyledItemDelegate>
#include <QMdiSubWindow>
#include <QValidator>
#include <QDebug>
#include <QRegularExpression>

class mMeshData;

namespace mxr
{
	class SelectionDataContainer;
}
namespace MGeoData {
	class GeoDataBase;
}

namespace MGeoManager {
	class GeoManager;
}
namespace MechMainWindow
{
	class IDValidator : public QValidator
	{
		Q_OBJECT
	public:
		explicit IDValidator(QObject* parent = nullptr) : QValidator(parent) {}

		State validate(QString& input, int& pos) const override
		{
			Q_UNUSED(pos);

			// 空字符串允许继续输入
			if (input.isEmpty())
				return Intermediate;

			// 完整合法的正则（判断整个输入是否最终合法）
			// 支持：单个数字 / start:end（范围） / start:end:step（带步长）
			static const QRegularExpression fullRe(
				R"(^\s*\d+(\s*:\s*\d+(\s*:\s*\d+)?)?\s*(,\s*\d+(\s*:\s*\d+(\s*:\s*\d+)?)?\s*)*$)"
			);

			if (!fullRe.isValid()) {
				qWarning() << "IDValidator: fullRe invalid:" << fullRe.errorString();
			}
			else {
				if (fullRe.match(input).hasMatch()) {
					// 格式完整合法，业务校验不在此处做，
					// 避免输入过程中（如 2:1 后面还想输 0 变成 2:10）被阻断
					return Acceptable;
				}
			}

			// 部分输入允许：只要每个字符都在允许集合内（数字、空白、逗号、冒号）
			for (QChar c : input) {
				if (!(c.isDigit() || c.isSpace() || c == ',' || c == ':')) {
					return Invalid; // 出现非法字符，立即阻止
				}
			}

			// 禁止以分隔符开头（如 ":", "," 等）
			QString trimmed = input.trimmed();
			if (!trimmed.isEmpty()) {
				QChar first = trimmed.front();
				if (first == ':' || first == ',') {
					return Invalid;
				}
			}

			// 禁止连续分隔符（如 "1::2", "1,,2" 等永远不可能合法的情况）
			for (int i = 0; i < trimmed.size() - 1; ++i) {
				if ((trimmed[i] == ':' || trimmed[i] == ',') &&
				    (trimmed[i + 1] == ':' || trimmed[i + 1] == ',')) {
					return Invalid;
				}
			}

			// 每个逗号分隔的段内，':' 最多只能出现 2 次（start:end:step）
			for (const QString& part : input.split(',')) {
				QString seg = part.trimmed();
				if (seg.isEmpty())
					continue;

				if (seg.count(':') > 2)
					return Invalid;
			}

			// 全部是允许字符但不是完整的合法格式 -> Intermediate（允许继续输入）
			return Intermediate;
		}
	};

	class mInputIDLineEdit : public QLineEdit
	{

		Q_OBJECT
	public:

		//构造函数
		mInputIDLineEdit();

		//构造函数
		~mInputIDLineEdit();

		std::vector<int> getIds();

		// 校验状态只改变背景色，不替换主题 QSS 的边框、字体和内边距。
		enum class InputState { Normal, Invalid, Incomplete };
		void setInputState(InputState state);

	private slots:

		void slot_parseIdString(const QString& text);
		void slot_textChanged(const QString& text);

	private:
		std::vector<int> _ids;

	};
	class mSelectMenuFunction : public QObject
	{
		Q_OBJECT
	public:
		/*
		* 获取单例指针
		*/
		static mSelectMenuFunction* getInstance();

		/*
		* 析构函数
		*/
		~mSelectMenuFunction();

		//获取节点输入行编辑框
		mInputIDLineEdit* getNodeInputLineEdit() { return _nodeLineEdit; };

		//获取单元输入行编辑框
		mInputIDLineEdit* getEleInputLineEdit() { return _eleLineEdit; };

		/*
		* 拾取辅助模式（"通过边角度/通过面角度/路径"按钮选中后联动拾取）
		*/
		enum class PickAssistMode
		{
			None = 0,//未激活
			LineAngle,//通过边角度
			FaceAngle,//通过面角度
			Path//路径
		};

		//设置拾取辅助模式（激活时自动连接当前视口的拾取完成信号）
		void setPickAssistMode(PickAssistMode mode);

		//获取拾取辅助模式
		PickAssistMode getPickAssistMode() const { return _pickAssistMode; };

		//设置通过边角度阈值
		void setPickAssistLineAngle(float angle);

		//设置通过面角度阈值
		void setPickAssistFaceAngle(float angle);

		//对当前拾取立即执行一次已激活模式的逻辑（内部带防重入保护）
		void applyPickAssist();

		//set 减 vector 生成 vector
		static std::vector<int> subtractSetVector(const std::set<int>& s, std::vector<int> v);

	private:
		/*
		* 默认构造函数
		*/
		mSelectMenuFunction(QWidget* parent = 0);

		//清空高亮
		void clearHighlight();



	public slots:

		//搜索满足角度范围的面/边(相邻不递归、限制递归)
		void slot_adjacentFaceLine(bool isRecursive);

		//扩展到连接面
		void slot_extendConnectFace();

		//选择所有圆柱面
		void slot_cylinderFaceSelect();

		//公共边
		void slot_faceCommonEdge();

		//凹陷面/凸起面
		void slot_sunkenRaisedFaceSelect(bool sunken);

		//按照Id选择节点
		void slot_nodeIDSelect();

		//按照Id选择单元
		void slot_eleIDSelect();

		//反向(包括网格和几何)
		void slot_reverse();

		//增长单元
		void slot_growEles();

        //通过边角度
        void slot_lineAngleSelect(float angleThresholdDeg = 60);

        //通过面角度
        void slot_faceAngleSelect(float angleThresholdDeg = 60);

        //通过路径拾取
        void slot_pathSelect();

		//转换为几何点
		void slot_mappingGeoPoint(bool isHighlight = true);

		//转换为几何线
		void slot_mappingGeoEdge(bool isHighlight = true);

		//转换为几何面
		void slot_mappingGeoFace(bool isHighlight = true);

		//转换为几何体
		void slot_mappingGeoSolid(bool isHighlight = true);

		//转换为节点
		void slot_mappingNode(bool isHighlight = true);

		//转换为单元线
		void slot_mappingMeshLine(bool isHighlight = true);

		//转换为单元表面
		void slot_mappingMeshFace(bool isHighlight = true);

		//转换为单元
		void slot_mappingEle(bool isHighlight = true);

		//转换为部件
		void slot_mappingPart(bool isHighlight = true);

		//X/Y/Z相同的实体
		void slot_sameLocationGeoSelect(int xyz);

		//X/Y/Z相同的共面实体(共面才行)
		void slot_sameLocationFaceGeoSelect(int xyz);

		//基于已选选择实体
		void slot_sizeGeoSelect(int compare);

		//基于尺寸选择实体
		void slot_sizeGeoSelect(QString geoType, int compare, double size1, double size2);

	private slots:
		//切换语言
		void slot_changeLanguage(int language);

		/*
		* 切换视口
		*/
		void slot_subWindowActivated(int viewportID);

		//视口拾取完成后联动执行已激活模式的逻辑
		void slot_pickAssistOnPick();

	private:

		//连接当前视口的拾取完成信号（随视口切换重连）
		void ensurePickAssistConnection();

		//获取几何关系（输入id，源类型，转换后的维度）
		std::vector<int> getShapeRelated(std::vector<int> ids, MxFunctions::PickDataType pickDataType, int dim);

		//判断几何特征是否隐藏（【镜像六件套摘除】死参 geoModelData 已删）
		bool getGeoIsMask(int id, int dim, mMeshData*meshData, MGeoData::GeoDataBase *geoDatabase);

	private:
		//单例指针
		static mSelectMenuFunction* _instance;

		//拾取高亮数据改变的连接
		QMetaObject::Connection conn_geo;
		QMetaObject::Connection conn_mesh;

		//通过ID选择节点行编辑框
		mInputIDLineEdit* _nodeLineEdit;

		//通过ID选择单元行编辑框
		mInputIDLineEdit* _eleLineEdit;

		//拾取辅助模式
		PickAssistMode _pickAssistMode = PickAssistMode::None;

		//通过边角度阈值
		float _pickAssistLineAngle = 60.f;

		//通过面角度阈值
		float _pickAssistFaceAngle = 60.f;

		//防重入标记（联动逻辑执行中会再次触发拾取信号，需忽略）
		bool _pickAssistBusy = false;

	};
}
