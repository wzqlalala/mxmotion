#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

#include <memory>

// 
#include "mxr/data/SelectDataStruct.h"

//QT
#include <QWidget>
#include <QMouseEvent>
#include <QHash>
#include <QGridLayout>
#include <QMdiSubWindow>
#include <QLabel>
#include <QTableWidget>
#include <QButtonGroup>

#include "mPanelBase.h"

#include "mPhysicalFieldEnum.h"

#include "mDataElement.h"
#include "mElementBase.h"

using namespace MxFunctions;
namespace MechElementData
{
	class mDataElement;
}
namespace MechPreData
{
	class mViewportPrivate;
}
namespace mxr
{
	struct MeshSelectInfoData;
}
namespace MechMainWindow
{
    class mElemeneParmTableWidget;
	class mPickWidgetController;
	//单元面板
	class MMAINWINDOW_EXPORT mElementPanel : public mPanelBase
	{
		Q_OBJECT

	public:
		/*
		* 默认构造函数
		*/
		mElementPanel(QWidget* parent = Q_NULLPTR);

		/*
		* 析构函数
		*/
		~mElementPanel();

		template<typename T>
		T* toType(MechElementData::mDataElement* base)
		{
			return dynamic_cast<T*>(base);
		};

		/*
		* 编辑单元
		*/
		void editElementPanel(QString type);

		/*
		* 取消编辑单元
		*/
		void uneditElementPanel();


	private slots:

        //单元种类下拉框变化
        void slot_varietyComboBoxChanged(const QString& text);

		//单元类型下拉框变化
		void slot_typeComboBoxChanged(const QString &text);

        //设置为此类型
        void slot_setTypeBtn();

		//切换视口	
		void slot_subWindowActivated(int viewportID);

		//切换形状
		void slot_shapeChanged(QAbstractButton* button, bool isChecked);

        //点击修改单元数据槽函数
        void slot_paramBtn();

	protected:

		/*
		* 连接信号
		*/
		void connectSig();

		/*
		* 断开连接信号
		*/
		void disconnectSig();

		//检查单元合理的列表
		std::vector<mxr::MeshSelectInfoData> checkElement(const std::vector<mxr::MeshSelectInfoData> &ids);

		//赋予杆/三维实体等无参数的单元
		void assignElement(std::vector<mxr::MeshSelectInfoData> ids);

		//获取通用拾取控件当前拾取的单元
		std::vector<mxr::MeshSelectInfoData> collectPickedElements();

		//临时高亮当前单元类型所有的id（蓝色渲染）并更新单元参数表格
		void tempHighlightEle();

		//失活通用拾取控件并清空拾取数据与高亮
		void deactivatePickWidget();

        template <typename T>
        void updateOrAssignComplexElement(std::vector<mxr::MeshSelectInfoData> meshIDs, MechElementData::mDataElement* elementData, ElementType elementType)
        {
            for (auto meshID : meshIDs)
            {
                int id = meshID.data_id.element_id;
                MechElementData::mElementBase* elementBase = elementData->getElementDataByMeshID(id);

                // 尝试转换为目标类型 (如果 elementBase 为空，强转也会安全地返回 nullptr)
                T* element = dynamic_cast<T*>(elementBase);

                if (element)
                {
                    // 类型匹配且存在：直接修改
                    element->setElementType(elementType);
                    // element; // 此处可做其他参数修改
                }
                else
                {
                    // 类型不匹配或不存在：重新创建并分配
                    element = new T();
                    element->setElementType(elementType);
                    elementData->assignElementToMesh(id, element);
                }
            }
        }

	protected:

		//总布局
		QVBoxLayout* _mainLayout;

        //当前种类
        QString _variety;

		//当前大类型
		QString _type;

        //当前形状
        QString _shape;

        //单元种类下拉框
        QComboBox* _comboEleVariety;

		//单元类型下拉框
		QComboBox* _comboEleType;

		//单元类型描述
		QLabel* _labelEleType;

        //修改单元数据按钮
        QPushButton* _buttonParam;

		//通用拾取控件
		mPickWidgetController* _pickController;

		//当前视口数据
		MechPreData::mViewportPrivate* _currentViewportPrivate;

		//形状折叠块的按钮组
		QButtonGroup* _buttonGroup;

        //单元参数表格
        mElemeneParmTableWidget* _elementParamTableWidget;
	};
}