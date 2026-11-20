#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include <memory>

//QT
#include <QWidget>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QHash>
#include <QGridLayout>
#include <QMdiSubWindow>
#include <QStringListModel>
#include <QLabel>

#include "mPanelBase.h"
#include "mMeshPanelBase.h"

#include "mPhysicalFieldEnum.h"

#include "mMeshThread.h"
#include "mMeshPanel.h"

#include "mxr/view/HighlightController.h"
using namespace MxFunctions;
using namespace MGUIMesh;
namespace MechPreData
{
	class mViewportModel;
}
class MXMesh;
class mMeshData;
namespace MechMainWindow
{


	class Max20Validator : public QValidator {
		Q_OBJECT
	public:
		Max20Validator(QObject* parent = nullptr) : QValidator(parent) {}
		State validate(QString& input, int&) const override {
			if (input.isEmpty()) return Acceptable;
			if (input == "-") return Acceptable;
			bool ok;
			double v = input.toDouble(&ok);
			if (!ok) return Invalid;          // 非数字直接拒绝
			if (v > 20.0) return Invalid;     // >20 拒绝输入
			if (v < -20.0) return Invalid;     // >20 拒绝输入
			return Acceptable;
		}
	};
	//局部尺寸面板
	class MMAINWINDOW_EXPORT mLocalSizePanel :public mMeshPanel
	{
		Q_OBJECT
	public:

		mLocalSizePanel(QWidget* parent = Q_NULLPTR);

		~mLocalSizePanel();
		void newMeshPanel() override;

		//更新面板
		void editMeshPanel(int meshId);
		//多选网格子节点面板
		void getMeshInfoById(const std::vector<int> ids);
		//根据数据初始化面板
		void updatePanel() override;
		//计算IDToCenterOfGEdge
		void getIDToCenterOfGEdge(std::shared_ptr<MXMesh> mesh, set<int> meshGeoId);
        //
		void slot_localSizePreview();
		void setLocalNum();
		void setLocalSize();
	private:
		//对几何边按偏置值重分布节点（偏置值>0向起点加密，<0向终点加密，0跳过）
		void applyEdgeBias(MeshEntity* entity, mMeshData* meshdata);
		//几何线-偏置缓存；_meshPanelData 由旧版 MeshPanelData.dll 构造，不含 _ids2BiasMap，
		//直接往该对象尾部偏移写会崩，待 MeshPanelData 重编后可换回 _meshPanelData->_ids2BiasMap
		std::map<int, double> _ids2BiasCache;
	public slots:
		void meshBiasChanged();

		void meshNumChanged();

		void meshSizeChanged();

	private:
		//多线程
		mMeshThread* _meshthread = nullptr;
		//全局尺寸
		FoldSection* m_rangeFold = nullptr;
		QLabel* _meshControlLabel;
		QComboBox* _meshControlBox;

		QLabel* _meshSizeLabel;
		QLineEdit* _meshSizeEdit;
		QToolButton* _meshSizeButton;

		QLabel* _meshNumLabel;
		QLineEdit* _meshNumEdit;
		QToolButton* _meshNumButton;
		QPushButton* _comfirmButton;
		//MeshData::mFunctionBase* _funcData = nullptr;

		FoldSection* m_OrderFold = nullptr;
		QComboBox* _orderComboBox;
		QLabel* _offsetDistanceLabel;
		QLineEdit* _offsetDistanceEdit;

		QLabel* _offsetDistanceLabel1;
		QLineEdit* _offsetDistanceEdit1;

		FoldSection* m_GeoFold = nullptr;
		QLabel* _geoLabel;


		double _factor = 0;
		QToolButton* allBtn;
		QToolButton* addBtn;
		QToolButton* reduceBtn;
		QToolButton* deleteBtn;

        QStackedWidget* stackWidget;
        QTableWidget* sizeTable;
        QTableWidget* numTable;
	};
}