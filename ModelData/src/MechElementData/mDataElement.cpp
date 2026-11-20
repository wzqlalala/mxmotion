#include "mDataElement.h"
#include "mElementBase.h"

namespace MechElementData
{
	mDataElement::mDataElement()
	{

	}
	mDataElement::~mDataElement()
	{
		QHashIterator<int, mElementBase*> iter(_elements);
		while (iter.hasNext())
		{
			iter.next();
			mElementBase *element = iter.value();
			delete element;
		}
		QHash<int, mElementBase*>().swap(_elements);
		_elements.clear();

	}
	void mDataElement::assignElementTypeToMesh(int ID, ElementType elementType)
	{
		mElementBase *element = _elements.value(ID);
		if (element == nullptr)
		{
			element = new mElementBase();
			_elements[ID] = element;
		}
		element->elementType = elementType;

	}

	QList<int> mDataElement::getElementIDs(ElementType elementType)
	{
		QList<int> res;
		QHashIterator<int, mElementBase*> iter(_elements);
		while (iter.hasNext())
		{
			iter.next();
			if (iter.value() != nullptr && iter.value()->elementType == elementType)
			{
				res.append(iter.key());
			}
		}
		return res;
	}

	void mDataElement::assignElementToMesh(int ID, mElementBase * element)
	{
		mElementBase *lastElement = _elements.value(ID);
		if (lastElement!=nullptr)
		{
			delete lastElement;
		}
		_elements[ID] = element;
	}
	bool mDataElement::hasElementData(int ID)
	{
		return _elements.contains(ID);
	}
	mElementBase * mDataElement::getElementDataByMeshID(int ID)
	{
		return _elements.value(ID);
	}
	
	QHash<int, mElementBase*>& mDataElement::getElements()
	{
		return _elements;
	}
	ElementType mDataElement::getElementTypeByMeshID(int ID)
	{
		mElementBase* element = getElementDataByMeshID(ID);
		if (element != nullptr)
		{
			return element->getElementType();
		}
		return ElementType(NoneElement);
	}
}