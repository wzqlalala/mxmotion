#include "mMotionContact.h"

namespace MechMotionData {

    mMotionContact::mMotionContact()
        : m_id(-1),
        m_name(),
        m_contactType(MotionContactType::CADContact),
        m_stiffness(100000.0),
        m_forceExponent(2.0),
        m_materialDamping(10.0),
        m_penetrationDepth(0.0254),
        m_frictionEnabled(false),
        m_staticFriction(0.3),
        m_dynamicFriction(0.2),
        m_forceModel("自动"),
        m_forceModelTolerance(5.0),
        m_actionUseFacetFile(false),
        m_actionDistanceTolerance(0.0),
        m_angleTolerance(25.0),
        m_actionUseMaxFacetSize(false),
        m_actionMaxFacetSize(1000.0),
        m_baseUseFacetFile(false),
        m_baseDistanceTolerance(0.0),
        m_baseUseMaxFacetSize(false),
        m_baseMaxFacetSize(1000.0),
        m_useConvexHullAction(true),
        m_useConvexHullBase(true)
    {
    }

    mMotionContact::mMotionContact(int id, const QString& name)
        : m_id(id),
        m_name(name),
        m_contactType(MotionContactType::CADContact),
        m_stiffness(100000.0),
        m_forceExponent(2.0),
        m_materialDamping(10.0),
        m_penetrationDepth(0.0254),
        m_frictionEnabled(false),
        m_staticFriction(0.3),
        m_dynamicFriction(0.2),
        m_forceModel("自动"),
        m_forceModelTolerance(5.0),
        m_actionUseFacetFile(false),
        m_actionDistanceTolerance(0.0),
        m_angleTolerance(25.0),
        m_actionUseMaxFacetSize(false),
        m_actionMaxFacetSize(1000.0),
        m_baseUseFacetFile(false),
        m_baseDistanceTolerance(0.0),
        m_baseUseMaxFacetSize(false),
        m_baseMaxFacetSize(1000.0),
        m_useConvexHullAction(true),
        m_useConvexHullBase(true)
    {
        if (m_name.isEmpty()) {
            m_name = QString("Contact_%1").arg(m_id);
        }
    }

    mMotionContact::~mMotionContact() {}

    void mMotionContact::setId(int id) { m_id = id; }
    int mMotionContact::getId() const { return m_id; }

    void mMotionContact::setName(const QString& name) { m_name = name; }
    QString mMotionContact::getName() const { return m_name; }

    void mMotionContact::setContactType(MotionContactType type) { m_contactType = type; }
    MotionContactType mMotionContact::getContactType() const { return m_contactType; }



    void mMotionContact::setStiffness(double k) { m_stiffness = k; }
    double mMotionContact::getStiffness() const { return m_stiffness; }

    void mMotionContact::setForceExponent(double e) { m_forceExponent = e; }
    double mMotionContact::getForceExponent() const { return m_forceExponent; }

    void mMotionContact::setMaterialDamping(double c) { m_materialDamping = c; }
    double mMotionContact::getMaterialDamping() const { return m_materialDamping; }

    void mMotionContact::setPenetrationDepth(double d) { m_penetrationDepth = d; }
    double mMotionContact::getPenetrationDepth() const { return m_penetrationDepth; }

    void mMotionContact::setFrictionEnabled(bool enabled) { m_frictionEnabled = enabled; }
    bool mMotionContact::getFrictionEnabled() const { return m_frictionEnabled; }

    void mMotionContact::setStaticFriction(double mu) { m_staticFriction = mu; }
    double mMotionContact::getStaticFriction() const { return m_staticFriction; }

    void mMotionContact::setDynamicFriction(double mu) { m_dynamicFriction = mu; }
    double mMotionContact::getDynamicFriction() const { return m_dynamicFriction; }

    void mMotionContact::setForceModel(const QString& model) { m_forceModel = model; }
    QString mMotionContact::getForceModel() const { return m_forceModel; }

    void mMotionContact::setForceModelTolerance(double tol) { m_forceModelTolerance = tol; }
    double mMotionContact::getForceModelTolerance() const { return m_forceModelTolerance; }

    void mMotionContact::setActionUseFacetFile(bool use) { m_actionUseFacetFile = use; }
    bool mMotionContact::getActionUseFacetFile() const { return m_actionUseFacetFile; }

    void mMotionContact::setActionDistanceTolerance(double tol) { m_actionDistanceTolerance = tol; }
    double mMotionContact::getActionDistanceTolerance() const { return m_actionDistanceTolerance; }

    void mMotionContact::setActionAngleTolerance(double tol) { m_angleTolerance = tol; }
    double mMotionContact::getActionAngleTolerance() const { return m_angleTolerance; }

    void mMotionContact::setActionUseMaxFacetSize(bool use) { m_actionUseMaxFacetSize = use; }

    void mMotionContact::setUseConvexHullAction(bool use) { m_useConvexHullAction = use; }
    bool mMotionContact::getUseConvexHullAction() const { return m_useConvexHullAction; }

    void mMotionContact::setUseConvexHullBase(bool use) { m_useConvexHullBase = use; }
    bool mMotionContact::getUseConvexHullBase() const { return m_useConvexHullBase; }
    bool mMotionContact::getActionUseMaxFacetSize() const { return m_actionUseMaxFacetSize; }

    void mMotionContact::setActionMaxFacetSize(double size) { m_actionMaxFacetSize = size; }
    double mMotionContact::getActionMaxFacetSize() const { return m_actionMaxFacetSize; }

    void mMotionContact::setBaseUseFacetFile(bool use) { m_baseUseFacetFile = use; }
    bool mMotionContact::getBaseUseFacetFile() const { return m_baseUseFacetFile; }

    void mMotionContact::setBaseDistanceTolerance(double tol) { m_baseDistanceTolerance = tol; }
    double mMotionContact::getBaseDistanceTolerance() const { return m_baseDistanceTolerance; }

    void mMotionContact::setBaseUseMaxFacetSize(bool use) { m_baseUseMaxFacetSize = use; }
    bool mMotionContact::getBaseUseMaxFacetSize() const { return m_baseUseMaxFacetSize; }

    void mMotionContact::setBaseMaxFacetSize(double size) { m_baseMaxFacetSize = size; }
    double mMotionContact::getBaseMaxFacetSize() const { return m_baseMaxFacetSize; }

    QString mMotionContact::getContactTypeName() const {
        return getContactTypeName(m_contactType);
    }

    QString mMotionContact::getContactTypeName(MotionContactType type) {
        switch (type) {
        case MotionContactType::CADContact:  return QString("CAD \u63a5\u89e6");  // CAD 接触
        case MotionContactType::FacetContact: return QString("\u5c0f\u9762\u63a5\u89e6"); // 小面接触
        default: return QString("Unknown");
        }
    }

    mMotionContact* mMotionContact::copyContact(int newId, const QString& newName)
    {
        mMotionContact* newContact = new mMotionContact(newId, newName);
        newContact->setContactType(this->getContactType());
        newContact->setStiffness(this->getStiffness());
        newContact->setForceExponent(this->getForceExponent());
        newContact->setMaterialDamping(this->getMaterialDamping());
        newContact->setPenetrationDepth(this->getPenetrationDepth());
        newContact->setFrictionEnabled(this->getFrictionEnabled());
        newContact->setStaticFriction(this->getStaticFriction());
        newContact->setDynamicFriction(this->getDynamicFriction());
        newContact->setForceModel(this->getForceModel());
        newContact->setForceModelTolerance(this->getForceModelTolerance());
        newContact->setActionUseFacetFile(this->getActionUseFacetFile());
        newContact->setActionDistanceTolerance(this->getActionDistanceTolerance());
        newContact->setActionAngleTolerance(this->getActionAngleTolerance());
        newContact->setActionUseMaxFacetSize(this->getActionUseMaxFacetSize());
        newContact->setUseConvexHullAction(this->getUseConvexHullAction());
        newContact->setUseConvexHullBase(this->getUseConvexHullBase());
        newContact->setActionMaxFacetSize(this->getActionMaxFacetSize());
        newContact->setBaseUseFacetFile(this->getBaseUseFacetFile());
        newContact->setBaseDistanceTolerance(this->getBaseDistanceTolerance());
        newContact->setBaseUseMaxFacetSize(this->getBaseUseMaxFacetSize());
        newContact->setBaseMaxFacetSize(this->getBaseMaxFacetSize());
        return newContact;
    }

}
