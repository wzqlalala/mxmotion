#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include <QString>
#include <QVector>
#include <QPair>

#include "MXMotionData_global.h"

namespace MechMotionData {

    enum class MXMOTIONDATA_EXPORT RLAlgorithm {
        PPO,       ///< Proximal Policy Optimization
        SAC,       ///< Soft Actor-Critic
        DDPG,      ///< Deep Deterministic Policy Gradient
        TD3,       ///< Twin Delayed DDPG
        A3C        ///< Asynchronous Advantage Actor-Critic
    };

    enum class MXMOTIONDATA_EXPORT RLEnvType {
        Motion,         ///< 运动控制
        Trajectory,     ///< 轨迹规划
        Force,          ///< 力控制
        Hybrid          ///< 混合控制
    };

    class MXMOTIONDATA_EXPORT mMotionRL {
    public:
        mMotionRL(int id, const QString& name = "");
        mMotionRL();
        ~mMotionRL();

        void setId(int id);
        int getId() const;

        void setName(const QString& name);
        QString getName() const;

        void setAlgorithm(RLAlgorithm algo);
        RLAlgorithm getAlgorithm() const;

        void setEnvType(RLEnvType type);
        RLEnvType getEnvType() const;

        void setStateSpace(const QVector<QString>& states);
        QVector<QString> getStateSpace() const;

        void setActionSpace(const QVector<QString>& actions);
        QVector<QString> getActionSpace() const;

        void setHiddenLayers(const QVector<int>& layers);
        QVector<int> getHiddenLayers() const;

        void setLearningRate(double lr);
        double getLearningRate() const;

        void setDiscountFactor(double gamma);
        double getDiscountFactor() const;

        void setMaxEpisodes(int episodes);
        int getMaxEpisodes() const;

        void setMaxSteps(int steps);
        int getMaxSteps() const;

        void setDatasetPath(const QString& path);
        QString getDatasetPath() const;

        void setModelSavePath(const QString& path);
        QString getModelSavePath() const;

        friend QDataStream& operator>>(QDataStream& in, mMotionRL& obj);
        friend QDataStream& operator<<(QDataStream& out, mMotionRL& obj);

    private:
        int m_id;
        QString m_name;
        RLAlgorithm m_algorithm;
        RLEnvType m_envType;
        QVector<QString> m_stateSpace;
        QVector<QString> m_actionSpace;
        QVector<int> m_hiddenLayers;
        double m_learningRate;
        double m_discountFactor;
        int m_maxEpisodes;
        int m_maxSteps;
        QString m_datasetPath;
        QString m_modelSavePath;
    };

}
