#pragma once
#include "MXElementData_global.h"

#include "mElementFunction.h"

//Qt
#include <QHash>

using namespace MxFunctions;
namespace MechElementData
{
	class mElementBase;
	class MXELEMENTDATA_EXPORT mDataElement
	{
	public:
		mDataElement();

		~mDataElement();

		//给指定网格赋予单元类型（没有参数的）
		void assignElementTypeToMesh(int ID, ElementType elementType);

		//根据类型获取单元ID
		QList<int> getElementIDs(ElementType elementType);

		//给指定网格赋予单元
		void assignElementToMesh(int ID, mElementBase* element);

		//通过网格ID获取是否有单元
		bool hasElementData(int ID);

		//通过网格ID获取单元
		mElementBase* getElementDataByMeshID(int ID);

		//获取所有的网格数据
		QHash<int, mElementBase*>& getElements();

		//获取单元类型
		ElementType getElementTypeByMeshID(int ID);

		int getElementsDataCount(){return _elements.size();}

		friend QDataStream& operator>>(QDataStream& in, mDataElement& mgr);
		friend QDataStream& InStreammDataElementV3(QDataStream& in, mDataElement& mgr);
		friend QDataStream& operator<<(QDataStream& out, mDataElement& mgr);
	private:
		//所有的网格数据
		QHash<int, mElementBase*> _elements;


	};
	
}