#pragma once
#pragma execution_character_set("utf-8")

//独立后处理树模型（mPostTreeModel）的索引/项访问辅助：
// 后处理树视图（mPostTreeView）专用；经模型按角色查询，数据层不归属任何视图。
// 角色常量沿用 MechPreData::Roles（与 GUIFoundation 的 mTreeDelegate 数值一致）
#include "mPostTreeModel.h"

//兼容：本库代码引用库外 MechMainWindow 命名空间符号（mPanelBase/FoldSection 等）；
//空定义保证未包含MechMainWindow头文件的编译单元中该命名空间亦已声明（命名空间定义可合并）
namespace MechMainWindow { }
using namespace MechMainWindow;


namespace PostWindow
{
    namespace mTreeItemUtil
    {
        using PostWindow::mPostTreeModel;
        using PostWindow::mPostTreeItem;

        inline mPostTreeItem* itemOf(mPostTreeModel* model, const QModelIndex& srcIdx)
        {
            if (model == nullptr || !srcIdx.isValid()) return nullptr;
            return model->itemFromIndex(srcIdx);
        }

        inline MxFunctions::BrowserTreeType treeTypeOf(mPostTreeModel* model, const QModelIndex& srcIdx)
        {
            if (model == nullptr || !srcIdx.isValid()) return MxFunctions::ModelRootItem;
            return (MxFunctions::BrowserTreeType)model->data(srcIdx, MechPreData::Roles::TypeRole).toInt();
        }

        inline int phyIdOf(mPostTreeModel* model, const QModelIndex& srcIdx)
        {
            if (model == nullptr || !srcIdx.isValid()) return -1;
            return model->data(srcIdx, MechPreData::Roles::PhyidRole).toInt();
        }

        inline int modelIdOf(mPostTreeModel* model, const QModelIndex& srcIdx)
        {
            if (model == nullptr || !srcIdx.isValid()) return -1;
            return model->data(srcIdx, MechPreData::Roles::ModelRole).toInt();
        }

        inline int caseIdOf(mPostTreeModel* model, const QModelIndex& srcIdx)
        {
            if (model == nullptr || !srcIdx.isValid()) return -1;
            return model->data(srcIdx, MechPreData::Roles::CaseidRole).toInt();
        }

        inline QString textOf(mPostTreeModel* model, const QModelIndex& srcIdx)
        {
            if (model == nullptr || !srcIdx.isValid()) return QString();
            return model->data(srcIdx, Qt::DisplayRole).toString();
        }

        inline MxFunctions::BrowserTreeStatus statusOf(mPostTreeModel* model, const QModelIndex& srcIdx)
        {
            if (model == nullptr || !srcIdx.isValid()) return MxFunctions::Tree_Yes;
            return (MxFunctions::BrowserTreeStatus)model->data(srcIdx, MechPreData::Roles::StatusRole).toInt();
        }

        inline int childrenSizeOf(mPostTreeModel* model, const QModelIndex& srcIdx)
        {
            if (model == nullptr || !srcIdx.isValid()) return 0;
            return model->rowCount(srcIdx);
        }

        //项形式的访问（部分既有代码以 mPostTreeItem 为输入；项的字段为私有，经模型按角色查询）
        inline MxFunctions::BrowserTreeType treeTypeOfItem(mPostTreeModel* model, mPostTreeItem* item)
        {
            if (model == nullptr || item == nullptr) return MxFunctions::ModelRootItem;
            return treeTypeOf(model, model->indexFromItem(item));
        }

        inline int phyIdOfItem(mPostTreeModel* model, mPostTreeItem* item)
        {
            if (model == nullptr || item == nullptr) return -1;
            return phyIdOf(model, model->indexFromItem(item));
        }

        inline QString textOfItem(mPostTreeModel* model, mPostTreeItem* item)
        {
            if (model == nullptr || item == nullptr) return QString();
            return textOf(model, model->indexFromItem(item));
        }
    }
}
