#include "mPostBrowserWindow.h"
#include "mCenterWindowProvider.h"
#include "mxr/view/REBUILD_PostRend.h"
#include "mCaseBase.h"	//mCaseBase完整类型（原经mTreeView.h传递提供，断链后自含）
#include "mTreeDelegate.h"
#include "mTreeModel.h"
#include "mTreeItemUtil.h"
#include "mPanelHostProvider.h"	//Info dock容器经IPanelHost接口解耦（原直调mInfoWindow）
#include "mMechGlobalSignals.h"
#include "mPostDockProvider.h"
#include "mPostInfoHub.h"
#include "mAppConfiguration.h"
#include "mOpenCurveDlg.h"
#include "mDockLayoutManager.h"
#include <QFileDialog>
#include <QDateTime>
#include "mGlobalSignals.h"
#include "mViewportModelManager.h"
#include "mViewportModel.h"
#include "mPostDataBase.h"
#include "mPostDataManager.h"
#include "mPostModelData.h"
#include "mPostParseMesh.h"
#include "mModelStruct.h"
#include "mViewTreeFunction.h"
#include "mPhysicalFieldEnum.h"
// mIONativeFile.h 的裸 include "mIOEnum.h" 靠调用方 include 栈先行解析（同
// mPostAdvancedQuery.cpp 的顺序约定）。注意必须显式取 PostIOFile 版（MIOFile
// 命名空间）：../Common 下同名头是 MechCommon 命名空间的另一套枚举
#include "../PostIOFile/mIOEnum.h"
#include "NativeReadersIO/mIONativeFile.h"
#include "mPost3DResult.h"

//与 mTreeView.cpp 的搬迁代码环境保持一致
using namespace MxFunctions;
using namespace MechModelData;
using namespace MechCommon;
using namespace MechPreData;
using namespace MechCaseData;
using namespace PostData;

//Qt
#include <QVBoxLayout>
#include <QGridLayout>
#include <QToolBar>
#include <QLabel>
#include <QCollator>
#include <QMessageBox>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QSet>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QTableWidget>

using namespace MxFunctions;

//右键菜单构建族（自 mPostBrowserWindow.cpp 拆出，批次C1：同类多cpp物理拆分，零行为变化）
namespace PostWindow
{
    namespace
    {
        // 当前视口后处理模型的来源元信息 reader（无后处理模型/非原生链路返回空）：
        // 链路同 mCustomColorBar 的 postDatabase 获取 + mPostAdvancedQuery 的
        // nativeReaderFor 查找
        MIOFile::mIONativeFile* sourceMetadataReader()
        {
            REBUILD_PostRend* postRend = getCVM->getPostRenderWidget();
            if (postRend == nullptr)
                return nullptr;
            const QString postName = postRend->getPostDataName();
            QPointer<mPostDataBase> postModelDB =
                getCVM->getModelData()->getManager<mPostDataManager>(Physical_Post).getPostDatabase(postName);
            if (postModelDB == nullptr)
                return nullptr;
            return mPostInfoHub::getInstance()->cloudResultPanel()->nativeReaderFor(
                postModelDB->getFile());
        }

        QString sourceMetadataSummaryText(const QList<qlonglong>& integers)
        {
            QStringList parts;
            const int limit = qMin<int>(3, integers.size());
            for (int index = 0; index < limit; ++index)
                parts.append(QString::number(integers[index]));
            if (integers.size() > limit)
                parts.append(QStringLiteral("..."));
            return parts.join(QStringLiteral(", "));
        }

        QString sourceMetadataSummaryText(const QList<double>& reals)
        {
            QStringList parts;
            const int limit = qMin<int>(3, reals.size());
            for (int index = 0; index < limit; ++index)
                parts.append(QString::number(reals[index], 'g', 8));
            if (reals.size() > limit)
                parts.append(QStringLiteral("..."));
            return parts.join(QStringLiteral(", "));
        }

        // 来源元信息查看对话框：记录明细表 + 截断横幅。decoded=false 的行标注
        // "结构视图"——库契约：未完全解码的记录不得当作完整材料/载荷定义展示
        void showSourceMetadataDialog(QWidget* parent, MIOFile::mIONativeFile* reader)
        {
            const QList<MIOFile::mIONativeFile::SourceMetadataEntry> entries =
                reader->sourceMetadata();

            QDialog dialog(parent);
            dialog.setWindowTitle(QStringLiteral("来源信息"));
            dialog.resize(900, 500);
            QVBoxLayout* layout = new QVBoxLayout(&dialog);

            if (reader->sourceMetadataTruncated())
            {
                QLabel* truncatedLabel = new QLabel(
                    QStringLiteral("元信息超出保留预算已截断（保留 %1 条 / %2 字节），以下为保留部分")
                        .arg(entries.size())
                        .arg(reader->sourceMetadataRetainedBytes()),
                    &dialog);
                truncatedLabel->setWordWrap(true);
                layout->addWidget(truncatedLabel);
            }

            QTableWidget* table = new QTableWidget(&dialog);
            table->setColumnCount(6);
            table->setHorizontalHeaderLabels(QStringList()
                << QStringLiteral("类别") << QStringLiteral("名称") << QStringLiteral("ID")
                << QStringLiteral("解码状态") << QStringLiteral("文本") << QStringLiteral("数值"));
            table->setRowCount(static_cast<int>(entries.size()));
            table->setEditTriggers(QAbstractItemView::NoEditTriggers);
            table->setSelectionBehavior(QAbstractItemView::SelectRows);
            table->setAlternatingRowColors(true);
            for (int row = 0; row < static_cast<int>(entries.size()); ++row)
            {
                const MIOFile::mIONativeFile::SourceMetadataEntry& entry = entries[row];
                const QString decodedText = entry.decoded ? QStringLiteral("已解码")
                                                          : QStringLiteral("结构视图（未完全解码）");
                QStringList valueParts;
                if (!entry.integers.isEmpty())
                    valueParts.append(QStringLiteral("整数: %1").arg(sourceMetadataSummaryText(entry.integers)));
                if (!entry.reals.isEmpty())
                    valueParts.append(QStringLiteral("实数: %1").arg(sourceMetadataSummaryText(entry.reals)));
                const QString idText = entry.id != 0 ? QString::number(entry.id) : QString();
                QTableWidgetItem* const items[] = {
                    new QTableWidgetItem(entry.category),
                    new QTableWidgetItem(entry.name),
                    new QTableWidgetItem(idText),
                    new QTableWidgetItem(decodedText),
                    new QTableWidgetItem(entry.texts.join(QStringLiteral(" | "))),
                    new QTableWidgetItem(valueParts.join(QStringLiteral("\n"))),
                };
                for (int column = 0; column < 6; ++column)
                    table->setItem(row, column, items[column]);
            }
            table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Stretch);
            table->setSortingEnabled(true);
            layout->addWidget(table);

            QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
            QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
            layout->addWidget(buttons);

            dialog.exec();
        }
    }

    void mPostTreeView::createPostModelRootItem()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除模型");
        connect(action, &QAction::triggered, this, [this]() {
            QMessageBox msgBox(this);
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setWindowTitle("警告");
            msgBox.setText("确定要删除当前模型吗？");
            QPushButton* okBtn = msgBox.addButton("确定", QMessageBox::AcceptRole);
            QPushButton* cancelBtn = msgBox.addButton("取消", QMessageBox::RejectRole);
            msgBox.setDefaultButton(cancelBtn);
            msgBox.exec();
            if (msgBox.clickedButton() != okBtn)
            {
                return;
            }
            deletePostModelBrowserTree();
            });

        // 来源元信息（MxNativeReaders 0.3.4+ 原生链路）：查看与 JSON 侧车导出。
        // 非原生导入（mxdb/VTK）或库未保留任何记录时不出菜单项
        if (MIOFile::mIONativeFile* metadataReader = sourceMetadataReader())
        {
            if (!metadataReader->sourceMetadata().isEmpty())
            {
                m_Menu->addSeparator();
                action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"),
                                           QStringLiteral("来源信息..."));
                connect(action, &QAction::triggered, this, [this, metadataReader]() {
                    showSourceMetadataDialog(this, metadataReader);
                    });
                action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"),
                                           QStringLiteral("导出来源元信息..."));
                connect(action, &QAction::triggered, this, [this, metadataReader]() {
                    // 库侧车为原子写、拒绝覆盖已存在目标（防误替换）：文件对话框
                    // 选中已存在文件时给出明确指引而非让导出静默失败
                    const QString target = QFileDialog::getSaveFileName(
                        this, QStringLiteral("导出来源元信息"), QString(),
                        QStringLiteral("JSON (*.json)"));
                    if (target.isEmpty())
                        return;
                    if (QFile::exists(target))
                    {
                        QMessageBox::warning(this, QStringLiteral("导出来源元信息"),
                            QStringLiteral("目标文件已存在。侧车导出不覆盖既有文件，请另选路径。"));
                        return;
                    }
                    QString exportError;
                    if (!metadataReader->exportSourceMetadata(target, &exportError))
                    {
                        QMessageBox::warning(this, QStringLiteral("导出来源元信息"),
                            QStringLiteral("导出失败：%1").arg(exportError));
                        return;
                    }
                    QMessageBox::information(this, QStringLiteral("导出来源元信息"),
                        QStringLiteral("已导出：%1").arg(target));
                    });
            }
        }
        m_Menu->exec(QCursor::pos());
    }

    void mPostTreeView::createPostModelParentMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/display/xianshi.png"), "显示全部");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuShowAllPostPart()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/display/yincangdangqian.png"), "隐藏全部");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuHideAllPostPart()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/display/fanzhuanxianshi.png"), "反向显示");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuReverseShowPostPart()));
        m_Menu->exec(QCursor::pos());
    }

    void mPostTreeView::createPostModelChildMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/xinjian-23x.png"), "创建分组");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_createPostModelGroup()));
        m_Menu->addSeparator();
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/display/xianshi.png"), "显示当前");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuShowCurrentPostPart()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/display/yincangdangqian.png"), "隐藏当前");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuHideCurrentPostPart()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/display/yincangqita.png"), "隐藏其他");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuHideOtherPostPart()));
        m_Menu->addSeparator();
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        m_Menu->exec(QCursor::pos());
    }

    void mPostTreeView::createPostModelGroupParentMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/display/yincangdangqian.png"), "显示");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_showPostModelGroup()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/display/yincangdangqian.png"), "隐藏");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_hidePostModelGroup()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_deletePostModelGroup()));
        m_Menu->exec(QCursor::pos());
    }

    void mPostTreeView::createPostModelGroupRootMenu()
    {
        // 分组根节点不显示菜单；创建分组功能只在部件子节点右键菜单中触发
        m_Menu->exec(QCursor::pos());
    }

    void mPostTreeView::createPostMaterialRootMenu()
    {
        //材料/属性树是部件的分组视图，根节点批量操作部件显隐，与子节点"显示当前"语义一致
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/display/xianshi.png"), "显示全部部件");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuShowAllPostPart()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/display/yincangdangqian.png"), "隐藏全部部件");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuHideAllPostPart()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/display/fanzhuanxianshi.png"), "反向显示");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuReverseShowPostPart()));
        m_Menu->exec(QCursor::pos());
    }

    void mPostTreeView::createPostMaterialChildMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/display/xianshi.png"), "显示当前");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_showPostMaterialSectionParts()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/display/yincangdangqian.png"), "隐藏当前");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_hidePostMaterialSectionParts()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/display/yincangqita.png"), "隐藏其他");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_hideOtherPostMaterialSectionParts()));
        m_Menu->exec(QCursor::pos());
    }

    void mPostTreeView::createPostMaterialPartMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/display/xianshi.png"), "显示当前");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_showPostMaterialSectionParts()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/display/yincangdangqian.png"), "隐藏当前");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_hidePostMaterialSectionParts()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/display/yincangqita.png"), "隐藏其他");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_hideOtherPostMaterialSectionParts()));
        m_Menu->exec(QCursor::pos());
    }

    void mPostTreeView::createPostSetRootMenu()
    {
        //集合总根不提供整体显隐入口；节点/面/边集显隐已取消（用户定案 2026-11-12），
        //单元/部件集显隐保留（在各集合类型根节点菜单中）
    }

    void mPostTreeView::createPostSetElementRootMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/display/xianshi.png"), "显示全部单元集合");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_showAllPostElementSet()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/display/yincangdangqian.png"), "隐藏全部单元集合");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_hideAllPostElementSet()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/display/yincangqita.png"), "仅显示集合单元");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_showOnlyPostElementSet()));
        m_Menu->exec(QCursor::pos());
    }

    void mPostTreeView::createPostSetNodeRootMenu()
    {
        //节点集显隐功能取消（用户定案 2026-11-12）：点显示机制随面/边级显隐一并下线
    }

    void mPostTreeView::createPostSetFaceRootMenu()
    {
        //面集显隐功能取消（用户定案 2026-11-12）：面级隐藏渲染链未实现
        //（渲染库不消费 face_is_show），单元级/部件级显隐不受影响
    }

    void mPostTreeView::createPostSetEdgeRootMenu()
    {
        //边集显隐功能取消（用户定案 2026-11-12）：边级隐藏渲染链未实现
        //（渲染库不消费 hidden_edge_pairs）
    }

    void mPostTreeView::createPostSetPartRootMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/display/xianshi.png"), "显示全部部件集合");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_showAllPostPartSet()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/display/yincangdangqian.png"), "隐藏全部部件集合");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_hideAllPostPartSet()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/display/yincangqita.png"), "仅显示集合部件");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_showOnlyPostPartSet()));
        m_Menu->exec(QCursor::pos());
    }

    void mPostTreeView::createPostSetElementChildMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/display/xianshi.png"), "显示当前");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_showPostElementSet()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/display/yincangdangqian.png"), "隐藏当前");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_hidePostElementSet()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/display/yincangqita.png"), "隐藏其他");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_hideOtherPostElementSet()));
        m_Menu->addSeparator();
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_deletePostSet()));
        m_Menu->exec(QCursor::pos());
    }

    void mPostTreeView::createPostSetNodeChildMenu()
    {
        //节点集显隐功能取消（用户定案 2026-11-12）；保留 重命名/删除
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_deletePostSet()));
        m_Menu->exec(QCursor::pos());
    }

    void mPostTreeView::createPostSetFaceChildMenu()
    {
        //面集显隐功能取消（用户定案 2026-11-12）；保留 重命名/删除
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_deletePostSet()));
        m_Menu->exec(QCursor::pos());
    }

    void mPostTreeView::createPostSetEdgeChildMenu()
    {
        //边集显隐功能取消（用户定案 2026-11-12）；保留 重命名/删除
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_deletePostSet()));
        m_Menu->exec(QCursor::pos());
    }

    void mPostTreeView::createPostSetPartChildMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/display/xianshi.png"), "显示当前");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_showPostPartSet()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/display/yincangdangqian.png"), "隐藏当前");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_hidePostPartSet()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/display/yincangqita.png"), "隐藏其他");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_hideOtherPostPartSet()));
        m_Menu->addSeparator();
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_deletePostSet()));
        m_Menu->exec(QCursor::pos());
    }

    void mPostTreeView::createCloudRootMenu()
    {
        //新建云图入口已取消（用户定案 2026-11-04）：云图实例只由结果导入与"缓存云图"
        //产生，不手动新建——菜单仅承载"更新"（有状态过期云图时才出现，无项不弹菜单）
        QModelIndexList modelIndexs = getSelectIndex();
        if (modelIndexs.size() != 1)
        {
            return;
        }

        bool isUpdate = false;
        auto parentItem = _treeModel->itemFromIndex(toSourceIndex(modelIndexs[0]));
        for (auto item : parentItem->getChildrenItem())
        {
            if (getTreeTypeByItem(item) == threeDResultParentItem && getStatusByItem(item) != MxFunctions::Tree_Yes)
            {
                isUpdate = true;
                break;
            }
        }
        if (!isUpdate)
        {
            return;
        }

        QAction* action = m_Menu->addAction("更新");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menu3DResultUpdate()));
        m_Menu->exec(QCursor::pos());
    }

    //派生工况分支头菜单（批次A0）："新建"三入口复用 Ribbon 结果页总线信号
    //（与按钮点击行为一致：每次点击新建一条定义进入编辑）
    void mPostTreeView::createPostCaseDerivedRootMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/post/svg/buxulie.svg"), QString::fromUtf8("新建步序列"));
        connect(action, &QAction::triggered, this, [=]() {
            emit MechCommon::mMechGlobalSignals::getInstance()->clickPostStepSequenceSig();
            });
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/post/svg/xianxingdiejia.svg"), QString::fromUtf8("新建线性叠加"));
        connect(action, &QAction::triggered, this, [=]() {
            emit MechCommon::mMechGlobalSignals::getInstance()->clickLinearSuperpositionSig();
            });
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/post/svg/baoluo.svg"), QString::fromUtf8("新建包络"));
        connect(action, &QAction::triggered, this, [=]() {
            emit MechCommon::mMechGlobalSignals::getInstance()->clickPostEnvelopeSig();
            });
        m_Menu->exec(QCursor::pos());
    }

    //派生工况定义节点菜单（批次A0）：编辑=回编对应面板（同单击）；删除=store 删除
    //（被引用时拦截并提示引用方）；重命名走面板名称行（即时提交，树随 store 通知刷新）
    void mPostTreeView::createPostCaseDerivedChildMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/post/svg/buxulie.svg"), QString::fromUtf8("编辑"));
        connect(action, &QAction::triggered, this, &mPostTreeView::slot_editDerivedCaseDef);
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), QString::fromUtf8("删除"));
        connect(action, &QAction::triggered, this, &mPostTreeView::slot_removeDerivedCaseDef);
        m_Menu->exec(QCursor::pos());
    }

    //测量记录节点菜单（V1.3 测量树联动，用户定案加全套）：编辑=回编面板（同单击，
    //统一编辑契约三选一确认内建）；显示/隐藏/删除=批量作用于选中节点（经面板公开
    //接口，树不直接改数据）；重命名走面板名称格（树随记录变更信号重建）
    void mPostTreeView::createMeasureChildMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/post/svg/celianggongju.svg"), QString::fromUtf8("编辑"));
        connect(action, &QAction::triggered, this, &mPostTreeView::slot_editMeasureRecord);
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/display/xianshi.png"), QString::fromUtf8("显示当前"));
        connect(action, &QAction::triggered, this, &mPostTreeView::slot_showMeasureRecords);
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/display/yincangdangqian.png"), QString::fromUtf8("隐藏当前"));
        connect(action, &QAction::triggered, this, &mPostTreeView::slot_hideMeasureRecords);
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), QString::fromUtf8("删除"));
        connect(action, &QAction::triggered, this, &mPostTreeView::slot_removeMeasureRecords);
        m_Menu->exec(QCursor::pos());
    }

    //坐标系根菜单（对齐前处理坐标系树父节点口径）：创建坐标系（默认直角，走模型页
    //三入口同款链路含结果守卫）/删除所有用户坐标系（管理器保留全局坐标系）
    void mPostTreeView::createCoordinateRootMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/post/svg/zuobiaoxi.svg"), QString::fromUtf8("创建坐标系"));
        connect(action, &QAction::triggered, this, &mPostTreeView::slot_menuCoordinateNew);
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), QString::fromUtf8("删除所有坐标系"));
        connect(action, &QAction::triggered, this, &mPostTreeView::slot_menuCoordinateDeleteAll);
        m_Menu->exec(QCursor::pos());
    }

    //用户坐标系节点菜单（对齐前处理子节点口径）：编辑=回编面板（同单击）；
    //重命名=树内联编辑（经 renameValidator 同步管理器）；复制/删除=管理器操作+分支重建。
    //全局坐标系节点（phyID=-1）不挂菜单（只读固有定义，对齐前处理"全局无右键"口径）
    void mPostTreeView::createCoordinateChildMenu()
    {
        QModelIndexList idxs = getSelectIndex();
        if (idxs.isEmpty()
            || getTreeTypeByModelIndex(idxs[0]) != MxFunctions::PostCoordinateChildItem
            || getPhyIdByModelIndex(idxs[0]) == PostGlobalCoordinatePhyID)
            return;

        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/post/svg/zuobiaoxi.svg"), QString::fromUtf8("编辑"));
        connect(action, &QAction::triggered, this, &mPostTreeView::slot_editCoordinateNode);
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), QString::fromUtf8("重命名"));
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/fuzhi.png"), QString::fromUtf8("复制"));
        connect(action, &QAction::triggered, this, &mPostTreeView::slot_copyCoordinateNode);
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), QString::fromUtf8("删除"));
        connect(action, &QAction::triggered, this, &mPostTreeView::slot_removeCoordinateNode);
        m_Menu->exec(QCursor::pos());
    }

    void mPostTreeView::create3DResultParentMenu()
    {
        QAction* action = nullptr;
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/post/svg/xianxingdiejia.svg"), "线性叠加");
        connect(action, &QAction::triggered, this, [=]() {
            emit MechCommon::mMechGlobalSignals::getInstance()->clickLinearSuperpositionSig();
            });
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/post/svg/biaoliangyunshuan.svg"), "标量运算");
        connect(action, &QAction::triggered, this, [=]() {
            emit MechCommon::mMechGlobalSignals::getInstance()->clickScalarOperationSig();
            });
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/post/svg/donghua.svg"), "一键生成动画");
        connect(action, &QAction::triggered, this, [=]() {
            emit MechCommon::mMechGlobalSignals::getInstance()->clickAnimationSig(1);
            });
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/post/svg/donghua.svg"), "自定义动画");
        connect(action, &QAction::triggered, this, [=]() {
            emit MechCommon::mMechGlobalSignals::getInstance()->clickAnimationSig(2);
            });
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/post/svg/qiemiantu.svg"), "切面图");
        connect(action, &QAction::triggered, this, [=]() {
            emit MechCommon::mMechGlobalSignals::getInstance()->clickSectionSig();
            });
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/post/svg/dengzhimian.svg"), "等值面图");
        connect(action, &QAction::triggered, this, [=]() {
            emit MechCommon::mMechGlobalSignals::getInstance()->clickIsosurfaceSig();
            });
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/post/svg/xiangliangtu.svg"), "矢量图");
        connect(action, &QAction::triggered, this, [=]() {
            emit MechCommon::mMechGlobalSignals::getInstance()->clickVectorSig();
            });
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/post/svg/liuxiantu.svg"), "流线图");
        connect(action, &QAction::triggered, this, [=]() {
            emit MechCommon::mMechGlobalSignals::getInstance()->clickStreamLineSig();
            });
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/post/svg/fenqu.svg"), "分区");
        connect(action, &QAction::triggered, this, [=]() {
            emit MechCommon::mMechGlobalSignals::getInstance()->clickPartitionSig();
            });
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/post/svg/jihe.svg"), "后处理-集合");
        connect(action, &QAction::triggered, this, [=]() {
            emit MechCommon::mMechGlobalSignals::getInstance()->clickPostSetSig();
            });
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/maxframe.png"), "切换到结果最大帧");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_switchToMaxFrame()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/chongminmin.png"), "重命名");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menuChildRename()));
        action = m_Menu->addAction(QIcon(":/mMainWidgets/Icon/BrowserTree/shanchu.png"), "删除");
        connect(action, SIGNAL(triggered()), this, SLOT(slot_menu3DResultDelete()));
        m_Menu->exec(QCursor::pos());
    }

    
    
    
    //createAnimationChildMenu/Linear/ScalarOperation/Section/Isosurface/Vector 六个派生子菜单随
//派生去节点化移除（用户定案 2026-11-04）：派生节点不存在，无右键菜单
//createCurveChildMenu/createExportWordChildMenu/createPathChildMenu 随去节点化移除：
    //曲线/Word报告/路径不再建树节点，无右键菜单

    
    
    }
