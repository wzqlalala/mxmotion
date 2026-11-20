#pragma once
#pragma execution_character_set("utf-8")
#include "mmainwindow_global.h"
#include <QObject>
#include "mAPIServer.h"
namespace MechMainWindow
{
    class mMeshPythonAPIRegistrar :public QObject
    {
        Q_OBJECT

    public:
        static mMeshPythonAPIRegistrar* instance();

        mMeshPythonAPIRegistrar(const mMeshPythonAPIRegistrar&) = delete;
        mMeshPythonAPIRegistrar& operator=(const mMeshPythonAPIRegistrar&) = delete;

        QVariant onMeshGenerateAPI(const APIServer::APIParams& params, QString& error);

        void registerMeshAPIs();
        void unregisterMeshAPIs();

    private:
        mMeshPythonAPIRegistrar() = default;
    };
}