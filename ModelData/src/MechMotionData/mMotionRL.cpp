#include "mMotionRL.h"

namespace MechMotionData {

    mMotionRL::mMotionRL()
        : m_id(-1),
        m_name(),
        m_algorithm(RLAlgorithm::PPO),
        m_envType(RLEnvType::Motion),
        m_stateSpace(),
        m_actionSpace(),
        m_hiddenLayers({ 256, 256 }),
        m_learningRate(3e-4),
        m_discountFactor(0.99),
        m_maxEpisodes(1000),
        m_maxSteps(500),
        m_datasetPath(),
        m_modelSavePath()
    {
    }

    mMotionRL::mMotionRL(int id, const QString& name)
        : m_id(id),
        m_name(name),
        m_algorithm(RLAlgorithm::PPO),
        m_envType(RLEnvType::Motion),
        m_stateSpace(),
        m_actionSpace(),
        m_hiddenLayers({ 256, 256 }),
        m_learningRate(3e-4),
        m_discountFactor(0.99),
        m_maxEpisodes(1000),
        m_maxSteps(500),
        m_datasetPath(),
        m_modelSavePath()
    {
        if (m_name.isEmpty()) {
            m_name = QString("RLModel_%1").arg(m_id);
        }
    }

    mMotionRL::~mMotionRL() {}

    void mMotionRL::setId(int id) { m_id = id; }
    int mMotionRL::getId() const { return m_id; }

    void mMotionRL::setName(const QString& name) { m_name = name; }
    QString mMotionRL::getName() const { return m_name; }

    void mMotionRL::setAlgorithm(RLAlgorithm algo) { m_algorithm = algo; }
    RLAlgorithm mMotionRL::getAlgorithm() const { return m_algorithm; }

    void mMotionRL::setEnvType(RLEnvType type) { m_envType = type; }
    RLEnvType mMotionRL::getEnvType() const { return m_envType; }

    void mMotionRL::setStateSpace(const QVector<QString>& states) { m_stateSpace = states; }
    QVector<QString> mMotionRL::getStateSpace() const { return m_stateSpace; }

    void mMotionRL::setActionSpace(const QVector<QString>& actions) { m_actionSpace = actions; }
    QVector<QString> mMotionRL::getActionSpace() const { return m_actionSpace; }

    void mMotionRL::setHiddenLayers(const QVector<int>& layers) { m_hiddenLayers = layers; }
    QVector<int> mMotionRL::getHiddenLayers() const { return m_hiddenLayers; }

    void mMotionRL::setLearningRate(double lr) { m_learningRate = lr; }
    double mMotionRL::getLearningRate() const { return m_learningRate; }

    void mMotionRL::setDiscountFactor(double gamma) { m_discountFactor = gamma; }
    double mMotionRL::getDiscountFactor() const { return m_discountFactor; }

    void mMotionRL::setMaxEpisodes(int episodes) { m_maxEpisodes = episodes; }
    int mMotionRL::getMaxEpisodes() const { return m_maxEpisodes; }

    void mMotionRL::setMaxSteps(int steps) { m_maxSteps = steps; }
    int mMotionRL::getMaxSteps() const { return m_maxSteps; }

    void mMotionRL::setDatasetPath(const QString& path) { m_datasetPath = path; }
    QString mMotionRL::getDatasetPath() const { return m_datasetPath; }

    void mMotionRL::setModelSavePath(const QString& path) { m_modelSavePath = path; }
    QString mMotionRL::getModelSavePath() const { return m_modelSavePath; }

}
