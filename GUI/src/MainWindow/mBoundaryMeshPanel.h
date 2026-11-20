#include "mmainwindow_global.h"
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

namespace MechMainWindow
{

    //面网格面板
    class MMAINWINDOW_EXPORT mBoundaryMeshPanel :public mMeshPanel
    {
    public:

        mBoundaryMeshPanel(QWidget* parent = Q_NULLPTR);

        ~mBoundaryMeshPanel();

        void newMeshPanel() override;
        //更新面板
        void editMeshPanel(int meshId);
        //多选网格子节点面板
        void getMeshInfoById(const std::vector<int> ids);
        //根据数据初始化面板
        void updatePanel() override;
        //初始化网格尺寸
        void setMeshSize();
        //初始化网格尺寸
        void initSize();
    private:
        //全局尺寸
        FoldSection* m_rangeFold = nullptr;
        QListWidget* _wallListAssignPart;
        QListWidget* _nonWallListAssignPart;
    };
}