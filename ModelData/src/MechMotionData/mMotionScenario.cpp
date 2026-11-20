#include "mMotionScenario.h"
#include "mMotionCurveData.h"
#include "mMotionResultData.h"
#include "mMotionCurveRequest.h"

#include <algorithm>

namespace MechMotionData {

    mMotionScenario::mMotionScenario(int id, const QString& name)
        : m_id(id),
        m_name(name),
        m_endTime(1.0),                  // 默认仿真50秒
        m_stepSize(0.01),                 // 默认步长0.1秒
        m_gravity(0.0, 0.0, -9810.0),   // mm/s²（mm-tonne-s 自洽制：9810 mm/s² = 9.81 m/s²，方向 -Z）
        m_usePreset(true),
        m_presetMode(PresetMode::FAST),
        m_integratorType(IntegratorType::HHT),
        m_integratorAlpha(-0.2),
        m_integratorBeta(0.25),
        m_integratorMaxIters(50),
        m_integratorTol(1e-8),
        m_solverType(SolverType::PSOR),
        m_solverMaxIters(100),
        m_solverTolerance(1e-10),
        m_solverVerbose(false)
    {
        if (m_name.isEmpty()) {
            m_name = QString("ScenarioConfig_%1").arg(m_id);
        }
        m_result = nullptr;
        m_curveNode = nullptr;
    }

    mMotionScenario::~mMotionScenario() {
        if (m_result) delete m_result;
        clearCurveNode();
    }

    void mMotionScenario::setId(int id) { m_id = id; }
    int mMotionScenario::getId() const { return m_id; }

    void mMotionScenario::setName(const QString& name) { m_name = name; }
    QString mMotionScenario::getName() const { return m_name; }

    void mMotionScenario::setEndTime(double endTime) { m_endTime = endTime; }
    double mMotionScenario::getEndTime() const { return m_endTime; }

    void mMotionScenario::setStepSize(double stepSize) { m_stepSize = stepSize; }
    double mMotionScenario::getStepSize() const { return m_stepSize; }

    void mMotionScenario::setGravity(const QVector3D& gravity) { m_gravity = gravity; }
    QVector3D mMotionScenario::getGravity() const { return m_gravity; }

    void mMotionScenario::setIntegratorType(IntegratorType type) { m_integratorType = type; }
    IntegratorType mMotionScenario::getIntegratorType() const { return m_integratorType; }

    // --- 求解模式（预设） ---
    void mMotionScenario::setUsePreset(bool use) { m_usePreset = use; }
    bool mMotionScenario::getUsePreset() const { return m_usePreset; }

    void mMotionScenario::setPresetMode(PresetMode mode) { m_presetMode = mode; }
    PresetMode mMotionScenario::getPresetMode() const { return m_presetMode; }

    // --- 积分器高级设置 ---
    void mMotionScenario::setIntegratorAlpha(double alpha) { m_integratorAlpha = alpha; }
    double mMotionScenario::getIntegratorAlpha() const { return m_integratorAlpha; }

    void mMotionScenario::setIntegratorBeta(double beta) { m_integratorBeta = beta; }
    double mMotionScenario::getIntegratorBeta() const { return m_integratorBeta; }

    void mMotionScenario::setIntegratorMaxIters(int iters) { m_integratorMaxIters = iters; }
    int mMotionScenario::getIntegratorMaxIters() const { return m_integratorMaxIters; }

    void mMotionScenario::setIntegratorTol(double tol) { m_integratorTol = tol; }
    double mMotionScenario::getIntegratorTol() const { return m_integratorTol; }

    // --- 求解器高级设置 ---
    void mMotionScenario::setSolverType(SolverType type) { m_solverType = type; }
    SolverType mMotionScenario::getSolverType() const { return m_solverType; }

    void mMotionScenario::setSolverMaxIters(int iters) { m_solverMaxIters = iters; }
    int mMotionScenario::getSolverMaxIters() const { return m_solverMaxIters; }

    void mMotionScenario::setSolverTolerance(double tol) { m_solverTolerance = tol; }
    double mMotionScenario::getSolverTolerance() const { return m_solverTolerance; }

    void mMotionScenario::setSolverVerbose(bool verbose) { m_solverVerbose = verbose; }
    bool mMotionScenario::getSolverVerbose() const { return m_solverVerbose; }

    mMotionScenario* mMotionScenario::copyScenario(int newId, const QString& newName)
    {
        mMotionScenario* newScenario = new mMotionScenario(newId, newName);

        // 拷贝物理求解配置
        newScenario->setEndTime(this->getEndTime());
        newScenario->setStepSize(this->getStepSize());
        newScenario->setGravity(this->getGravity());
        newScenario->setIntegratorType(this->getIntegratorType());

        // 拷贝新增参数
        newScenario->setUsePreset(this->getUsePreset());
        newScenario->setPresetMode(this->getPresetMode());
        newScenario->setIntegratorAlpha(this->getIntegratorAlpha());
        newScenario->setIntegratorBeta(this->getIntegratorBeta());
        newScenario->setIntegratorMaxIters(this->getIntegratorMaxIters());
        newScenario->setIntegratorTol(this->getIntegratorTol());
        newScenario->setSolverType(this->getSolverType());
        newScenario->setSolverMaxIters(this->getSolverMaxIters());
        newScenario->setSolverTolerance(this->getSolverTolerance());
        newScenario->setSolverVerbose(this->getSolverVerbose());

        // 不拷贝 m_result 和 m_curveNode！
        return newScenario;
    }

    void mMotionScenario::setCurveNode(mMotionCurveData* curve)
    {
        if (m_curveNode != curve) {
            clearCurveNode();
            m_curveNode = curve;
        }
    }

    void mMotionScenario::clearCurveNode()
    {
        if (m_curveNode) {
            delete m_curveNode;
            m_curveNode = nullptr;
        }
    }

    void mMotionScenario::addTrackedMarkerId(int markerId)
    {
        if (std::find(m_trackedMarkerIds.begin(), m_trackedMarkerIds.end(), markerId) == m_trackedMarkerIds.end()) {
            m_trackedMarkerIds.push_back(markerId);
        }
    }

    void mMotionScenario::removeTrackedMarkerId(int markerId)
    {
        auto it = std::find(m_trackedMarkerIds.begin(), m_trackedMarkerIds.end(), markerId);
        if (it != m_trackedMarkerIds.end()) {
            m_trackedMarkerIds.erase(it);
        }
    }

    bool mMotionScenario::isTrackingMarker(int markerId) const
    {
        return std::find(m_trackedMarkerIds.begin(), m_trackedMarkerIds.end(), markerId) != m_trackedMarkerIds.end();
    }

    void mMotionScenario::addCurveRequest(const mMotionCurveRequest& req)
    {
        mMotionCurveRequest newReq = req;
        // requestId 无效或重复时重新分配，保证唯一
        if (newReq.m_requestId < 0 || hasCurveRequest(newReq.m_requestId)) {
            int maxId = -1;
            for (const auto& item : m_curveRequests)
                maxId = std::max(maxId, item.m_requestId);
            newReq.m_requestId = maxId + 1;
        }
        m_curveRequests.push_back(newReq);
        m_currentCurveRequest = newReq; // 同步为最近编辑状态
    }

    void mMotionScenario::updateCurveRequest(const mMotionCurveRequest& req)
    {
        for (auto& item : m_curveRequests) {
            if (item.m_requestId == req.m_requestId) {
                item = req;
                m_currentCurveRequest = req;
                return;
            }
        }
    }

    void mMotionScenario::removeCurveRequest(int requestId)
    {
        m_curveRequests.erase(
            std::remove_if(m_curveRequests.begin(), m_curveRequests.end(),
                [requestId](const mMotionCurveRequest& item) { return item.m_requestId == requestId; }),
            m_curveRequests.end());
    }

    bool mMotionScenario::hasCurveRequest(int requestId) const
    {
        for (const auto& item : m_curveRequests) {
            if (item.m_requestId == requestId)
                return true;
        }
        return false;
    }

}
