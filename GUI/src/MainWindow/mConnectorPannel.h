#pragma once
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"
#include <QObject>
#include <memory>
#include "mConnector.h"
#include "mPanelBase.h"
#include "mMixSet.h"

//BasicData
#include "mConnectorBase.h"
#include "mConnectorManager.h"


#include "mConnectorRigid.h"
#include "mConnectorFlexible.h"
#include "mConnectorMPC.h"
#include "mConnectingDeputy.h"

#include "mConnectorBind.h"

using namespace MechConnectorData;
using namespace MechSetData;
namespace MechMainWindow
{
    class MMAINWINDOW_EXPORT mConnectorPannel : public mPanelBase
    {
        Q_OBJECT

    public:
        mConnectorPannel(QWidget* parent = nullptr);
        ~mConnectorPannel();

        /*
        * 创建连接
        */
        void createConnector(MxFunctions::ConnectorType type);

        /*
        * 编辑连接
        */
        void editConnector(const int& connId);

        /*
        * 取消连接编辑
        */
        void unEditConnector();

        /*
        * 渲染当前临时节点
        */
        void rendFreeNodes();

        /*
        * 取消当前临时节点渲染
        */
        void disRendFreeNodes();

    private:
        /*
        * 连接面板
        */
        void connectorPanel(MxFunctions::ConnectorType type);

        /*
        * 创建内部集合
        */
        mMixSet* createSet();
    private:
        QVBoxLayout* _mainLayout;

        std::shared_ptr<MechConnectorData::mConnectorBase> _connectorData;
    };
}