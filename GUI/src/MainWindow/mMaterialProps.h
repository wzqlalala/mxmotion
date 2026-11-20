#pragma once


#include "mmainwindow_global.h"
#include <QObject>
#include <QGroupBox>
#include <QRadioButton>

#include "mMaterialPropBase.h"
#include <memory>



namespace MechMainWindow
{

    // 材料属性输入校验模式静态映射
    extern QHash<QString, CalcMode> s_nameToCalcMode;

    class mDensityProp;
    class mThermalExpansivityProp;
    class mMaterialDampingProp;
    class mLoseEfficacyProp;
    class mFailureEffectProp;
    class mHeatGenerationRateProp;
    class mHeatProductionRateProp;
    class mCreepBehaviorProp;
    class mCyclicHardeningProp;
    class mBulkModulusProp;
    class mAcousticsDampingProp;
    class mTimeDomainPronyProp;
    class mThermalEquivalenceProp;
    class mShimProp;
    class mFlexProp;
    class mPlasticityProp;
    class mPlasticityOrderProp;
    class mThicknessBehaviorProp;
    class mFilmBehaviorProp;
    class mCutBehaviorProp;
    class mHeatProp;
    class mHeatConductivityCoefficientProp;
    class mHeatCapacityDensityProp;
    class mSoundVelocityProp;

    class mIsotropyProp;
    class mParamCurveControl;
    class mAnisotropyProp;
    class mOrthotropyProp;
    class mIsotropicThermalProp;
    class mIsotropicHardeningProp;
    class mKinematicHardeningProp;
    class mCompressiveHardeningProp;
    class mChabocheProp;
    class mNeoHookeanProp;
    class mNeoHookean2Prop;
    class mMooneyRivlinProp;
    class mOgdenProp;
    class mArrudaBoyceProp;
    class mYeohProp;
    class mPolynomialProp;
    class mReducedPolynomialProp;
    class mHyperfoamProp;
    class mRigidProp;
    class mAeroacousticsProp;
    class mDeformationPlasticityProp;
    class mViscoplasticProp;
    class mViscousElasticProp;
    class mBrittleLikeProp;
    class mPiezoelectricProp;
    class mFiberReinforcedProp;
    class mMatFromLibCommon;
    class mMatFromLibStructural;
    class mMatFromLibThermal;
    class mMatFromLibElectic;
    class mDefaultProperty;
    class mFailureHashinProp;
    class mEnergyDamageProp;

    class mCyclicHardeningWithTableProp;
    class mFlexProp;
    class mPlasticityProp;
    class mPlasticityOrderProp;
    class mThicknessBehaviorProp;
    class mFilmBehaviorProp;
    class mCutBehaviorProp;

    class mBreakProp;
    class mBreakParamProp;
    class mPhaseFieldProp;
    class MMAINWINDOW_EXPORT mMaterialProps :public QObject
    {
        Q_OBJECT
    public:
        /*
        * 获取单例指针
        */
        static mMaterialProps* getInstance();

        /*
        * 析构函数
        */
        ~mMaterialProps();

        /*
        * 获取属性指针
        */
        mMaterialPropBase* getPropByType(const MxFunctions::MaterialClass& propType, QWidget* parent = nullptr);

        /*
        * 更新所有界面单位
        */
        void updateAllPropUnit();

    private:
        /*
        * 构造函数
        */
        mMaterialProps(QWidget* parent = nullptr);

    private:
        //单例指针
        static mMaterialProps* m_pInstance;

        //属性组控件缓存
        QHash<MxFunctions::MaterialClass, mMaterialPropBase*> m_FoldCache;

    };

    class ClickableGroupBox : public QGroupBox
    {
        Q_OBJECT
    public:
        explicit ClickableGroupBox(const QString& title = {}, QWidget* parent = nullptr)
            : QGroupBox(title, parent) {
            hl = false;
            normalFont = font();
            boldFont = normalFont;
            boldFont.setBold(true);
        }
        void highlight(bool on)
        {
            hl = on;
            setFont(on ? boldFont : normalFont);
        }

    signals:
        void clicked();

        //protected:
        //	void mousePressEvent(QMouseEvent* ev) override
        //	{
        //		Q_UNUSED(ev);
        //		hl = !hl;
        //		highlight(hl);
        //		emit clicked();
        //	}
    private:
        QFont normalFont, boldFont;
        bool hl;
    };

    inline MxFunctions::MaterialType Index2Type(int typeIdx, int clasIdx)
    {
        using MT = MxFunctions::MaterialType;
        switch (typeIdx)
        {
        case 0: //线弹性
            switch (clasIdx) {
            case 0: return MT::LinearElastic_Isotropic;
            case 1: return MT::LinearElastic_Orthotropic;
            case 2: return MT::LinearElastic_Anisotropic;
            default: return MT::NONEMATERIALTYPE;
            }
        case 1: //塑性
            switch (clasIdx) {
            case 0: return MT::Plastic_Normal;
            case 1: return MT::Plastic_Cyclic;
            case 2: return MT::Plastic_Damage;
            case 3: return MT::Plastic_Johnson;
            default: return MT::NONEMATERIALTYPE;
            }
        case 2: //超弹性
            switch (clasIdx) {
            case 0: return MT::HyperElastic_MooneyRivlin;
            case 1: return MT::HyperElastic_NeoHookean;
            case 2: return MT::HyperElastic_Ogden;
            case 3: return MT::HyperElastic_Yeoh;
            case 4: return MT::HyperElastic_ArrudaBoyce;
            case 5: return MT::HyperElastic_Polynomial;
            default: return MT::NONEMATERIALTYPE;
            }
        case 3: //粘性
            switch (clasIdx) {
            case 0: return MT::Viscous_Viscoelastic;
            case 1: return MT::Viscous_Viscoplastic;
            default: return MT::NONEMATERIALTYPE;
            }
        case 5: //其它材料
            switch (clasIdx) {
            case 0: return MT::OtherMaterial_Shim;
            case 1: return MT::OtherMaterial_Brittle;
            case 2: return MT::OtherMaterial_Foam;
            case 3: return MT::OtherMaterial_Break;
            default: return MT::NONEMATERIALTYPE;
            }
        case 4: return MT::Thermal_Base;    //热学，无二级
        case 6: return MT::AcousticMaterial_Base; //声学，无二级

        default: return MT::NONEMATERIALTYPE;
        }
    };

    inline std::pair<int, int> GetIndexFromType(MaterialType type)
    {
        using MT = MaterialType;
        switch (type)
        {
            // 线弹性 classIdx=0
        case MT::LinearElastic_Isotropic:     return { 0, 0 };
        case MT::LinearElastic_Orthotropic:   return { 0, 1 };
        case MT::LinearElastic_Anisotropic:   return { 0, 2 };

                                          // 塑性 classIdx=1
        case MT::Plastic_Normal:              return { 1, 0 };
        case MT::Plastic_Cyclic:              return { 1, 1 };
        case MT::Plastic_Damage:              return { 1, 2 };
        case MT::Plastic_Johnson:             return { 1, 3 };

                               // 超弹性 classIdx=2
        case MT::HyperElastic_MooneyRivlin:   return { 2, 0 };
        case MT::HyperElastic_NeoHookean:     return { 2, 1 };
        case MT::HyperElastic_Ogden:          return { 2, 2 };
        case MT::HyperElastic_Yeoh:           return { 2, 3 };
        case MT::HyperElastic_ArrudaBoyce:    return { 2, 4 };
        case MT::HyperElastic_Polynomial:     return { 2, 5 };

                                        // 粘性 classIdx=3
        case MT::Viscous_Viscoelastic:        return { 3, 0 };
        case MT::Viscous_Viscoplastic:        return { 3, 1 };

                                     // 其它材料 classIdx=5
        case MT::OtherMaterial_Shim:        return { 5, 0 };
        case MT::OtherMaterial_Brittle:       return { 5, 1 };
        case MT::OtherMaterial_Foam:          return { 5, 2 };
        case MT::OtherMaterial_Break:      return { 5, 3 };

                                       // 热学、声学无二级子类型
        case MT::Thermal_Base:                return { 4, -1 };
        case MT::AcousticMaterial_Base:       return { 6, -1 };

        default:
            return { -1, -1 };
        }
    }

    /***********************************材料定义***************************************/
    class MMAINWINDOW_EXPORT mMaterialDefine : public mMaterialPropBase
    {
        Q_OBJECT
    public:
        /*
        * 构造函数
        */
        mMaterialDefine(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mMaterialDefine()override;

        /*
        * 初始化输入控件
        */
        void initInputs()override;

        //初始化选项
        void refreshTypePanel(int TypeIndex);

        /*
        * 所有输入控件
        */
        void InitFoldSection() override;

        /*
        * 清除输入控件
        */
        void clearInputs()override;

        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;

        virtual QHash<MxFunctions::MaterialPropType, QVariant>collectProperties()override { return QHash<MxFunctions::MaterialPropType, QVariant>(); };

        virtual void connectUnitSystemResponse();

        virtual void disconnectUnitSystemResponse();

        virtual void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx);

    private:
        PanelLabel* m_qLabelType = nullptr;
        QComboBox* m_qComboBoxType = nullptr;
        PanelLabel* m_qLabeClass = nullptr;
        QComboBox* m_qComboBoxClass = nullptr;

        // 大类索引 -> 子类型文本列表
        QMap<int, QStringList>m_materialTypeMap;
        // 记忆每一类上次选中的下标
        QMap<int, int> m_lastSelectIndex;

        bool m_bAutoRefreshUI = false; // true=代码自动回填界面，屏蔽信号

        MaterialType m_curType = MaterialType::LinearElastic_Isotropic;
    };




    /***********************************密度***************************************/
    class MMAINWINDOW_EXPORT mDensityProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mDensityProp(QWidget* parent = nullptr,bool isCheckable =true, MxFunctions::MaterialClass classKey = MxFunctions::MAT_Density);
        /*
        * 析构函数
        */
        ~mDensityProp()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;

        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

        /*
        * 控制表格信号连接
        */
        void connTable(bool conn);

        virtual void connectUnitSystemResponse();

        virtual void disconnectUnitSystemResponse();

        virtual void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx);

    private:
        /*
        * 表格连接
        */
        void slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title);

    private:
        //控件成员
        QVector<InputWidgetSet> inputWidgets;
        singleCheckBox checkboxtemp_;
        // 温度相关行封装（数据载体 checkboxtemp_ 的 "curve" 属性与其同步）
        mTempCurveRowWidget* m_tempCurveRow = nullptr;
        bool m_bCheckable = true;
    };

    /***********************************热膨胀系数***************************************/
    class MMAINWINDOW_EXPORT mThermalExpansivityProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mThermalExpansivityProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mThermalExpansivityProp()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

        /*
        * 控制表格信号连接
        */
        void connTable(bool conn);

        virtual void connectUnitSystemResponse();

        virtual void disconnectUnitSystemResponse();

        virtual void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx);

    private slots:
        /*
        * 表格连接
        */
        void slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title);
        /*
        * 类型改变
        */
        void slot_typeChanged(int i);

    private:
        //控件成员
        QVector<InputWidgetSet> inputWidgetsTmp;
        QVector<InputWidgetSet> inputWidgets1;
        QVector<InputWidgetSet> inputWidgets2;
        QVector<InputWidgetSet> inputWidgets;
        singleCheckBox checkboxtemp_;
        // 温度相关行封装（列随类型切换经 refreshCurveHeaders 更新）
        mTempCurveRowWidget* m_tempCurveRow = nullptr;
        // 按当前类型刷新封装行的曲线列标题（温度 + 当前 inputWidgets2 标签）
        void refreshCurveHeaders();

        singleComboBox combobox_;
        QHash<MaterialPropType, int>type2inp;
    };

    /***********************************材料阻尼***************************************/
    class MMAINWINDOW_EXPORT mMaterialDampingProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mMaterialDampingProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mMaterialDampingProp()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;
    private:
        //控件成员
        QVector<InputWidgetSet> inputWidgets;
    };

    /***********************************应力失效***************************************/
    class MMAINWINDOW_EXPORT mLoseEfficacyProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mLoseEfficacyProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mLoseEfficacyProp()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;
        /*
        * 控制表格信号连接
        */
        void connTable(bool conn);

        virtual void connectUnitSystemResponse();

        virtual void disconnectUnitSystemResponse();

        virtual void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx);

    private slots:
        /*
        * 表格连接
        */
        void slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title);

        /*
        * 切换输入
        */
        void slot_switchInputs(int i);
    private:
        QStringList units_;
        //控件成员
        singleComboBox combobox_;
        singleCheckBox checkboxtemp_;
        // 温度相关行封装（列随类型切换经 refreshCurveHeaders 更新）
        mTempCurveRowWidget* m_tempCurveRow = nullptr;
        // 按当前类型刷新封装行的曲线列标题（温度 + 当前 inputWidgets_ 标签）
        void refreshCurveHeaders();
        QVector<MaterialPropType>proptypes_;
        QVector<MaterialPropType>proptypes_2;
        QVector<InputWidgetSet> inputWidgetsTmp1;
        QVector<InputWidgetSet> inputWidgetsTmp2;
        QVector<InputWidgetSet> inputWidgets_;
        QHash<MaterialPropType, int>type2inp_;
        QHash<MaterialPropType, int>type2inp2_;
    };

    /***********************************应变失效***************************************/
    class MMAINWINDOW_EXPORT mFailureEffectProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mFailureEffectProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mFailureEffectProp()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

        /*
        * 控制表格信号连接
        */
        void connTable(bool conn);

    private slots:
        /*
        * 表格连接
        */
        void slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title);

        /*
        * 切换输入
        */
        void slot_switchInputs(int i);
    private:
        //控件成员
        singleComboBox combobox_;
        singleCheckBox checkboxtemp_;
        // 温度相关行封装（列随类型切换经 refreshCurveHeaders 更新）
        mTempCurveRowWidget* m_tempCurveRow = nullptr;
        // 按当前类型刷新封装行的曲线列标题（温度 + 当前 inputWidgets_ 标签）
        void refreshCurveHeaders();
        QVector<MaterialPropType>proptypes_;
        QVector<MaterialPropType>proptypes_2;
        QVector<InputWidgetSet> inputWidgetsTmp1;
        QVector<InputWidgetSet> inputWidgetsTmp2;
        QVector<InputWidgetSet> inputWidgets_;
        QHash<MaterialPropType, int>type2inp_;
        QHash<MaterialPropType, int>type2inp2_;
    };

    /***********************************Hashin失效***************************************/
    class MMAINWINDOW_EXPORT mFailureHashinProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mFailureHashinProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mFailureHashinProp()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;
    private:
        //控件成员
        QVector<MaterialPropType>proptypes_;
        QVector<InputWidgetSet> inputWidgets_;
        QHash<MaterialPropType, int>type2inp;

    };

    /***********************************Hashin渐进损伤***************************************/
    class MMAINWINDOW_EXPORT mEnergyDamageProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mEnergyDamageProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mEnergyDamageProp()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;
    private:
        //控件成员
        QVector<MaterialPropType>proptypes_;
        QVector<InputWidgetSet> inputWidgets_;
        QHash<MaterialPropType, int>type2inp;

    };

    /***********************************热生成率***************************************/
    class MMAINWINDOW_EXPORT mHeatGenerationRateProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mHeatGenerationRateProp(QWidget* parent = nullptr);
        /*
        * 单位系统变化：产热率按 PowerDensity 维度换算数值并同步标签
        */
        void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx) override;
        /*
        * 析构函数
        */
        ~mHeatGenerationRateProp()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;
    private:
        QStringList units_;
        //控件成员
        QVector<InputWidgetSet> inputWidgets;
    };

    /***********************************蠕变行为--蠕变参数***************************************/
    class MMAINWINDOW_EXPORT mCreepBehaviorProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mCreepBehaviorProp(QWidget* parent = nullptr, bool isCheckable = false);
        /*
        * 析构函数
        */
        ~mCreepBehaviorProp()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

        /*
        * 控制表格信号连接
        */
        void connTable(bool conn);

        virtual void connectUnitSystemResponse();

        virtual void disconnectUnitSystemResponse();

        virtual void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx);

    private slots:
        /*
        * 表格连接
        */
        void slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title);
        /*
        * 切换输入
        */
        void slot_switchInputs(int i);

    private:
        QStringList units_;
        //控件成员
        singleCheckBox checkboxtemp_;
        // 温度相关行封装（数据载体 checkboxtemp_ 的 "curve" 属性与其同步）
        mTempCurveRowWidget* m_tempCurveRow = nullptr;
        singleComboBox combobox_;
        QVector<InputWidgetSet> inputWidgets;
        QHash<MaterialPropType, int>type2inp;
        //Anand硬化类型(仅粘性-粘塑性入口)的15个模型参数输入组
        QVector<InputWidgetSet> inputWidgetsAnand;
        QHash<MaterialPropType, int> type2inpAnand;
        bool m_bIsCheckable = false;

    };

    /***********************************循环硬化***************************************/
    class MMAINWINDOW_EXPORT mCyclicHardeningProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mCyclicHardeningProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mCyclicHardeningProp()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

        virtual void connectUnitSystemResponse();

        virtual void disconnectUnitSystemResponse();

        virtual void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx);
    private:
        QStringList units_;
        //控件成员
        QVector<InputWidgetSet> inputWidgets;
    };

    /***********************************循环硬化带表格***************************************/
    class MMAINWINDOW_EXPORT mCyclicHardeningWithTableProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mCyclicHardeningWithTableProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mCyclicHardeningWithTableProp()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;
        /*
        * 控制表格信号连接
        */
        void connTable(bool conn);

    private slots:
        /*
        * 表格连接
        */
        void slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title);

    private:
        QStringList units_;
        QStringList header_;
        QStringList headerNoTemp_;
        //控件成员
        singleCheckBox checkboxtemp_;
        // 温度相关行封装（数据载体 checkboxtemp_ 的 "curve" 属性与其同步）
        mTempCurveRowWidget* m_tempCurveRow = nullptr;
        QLabel* tableName_;
    };

    /***********************************体积模量***************************************/
    class MMAINWINDOW_EXPORT mBulkModulusProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mBulkModulusProp(QWidget* parent = nullptr);
        /*
        * 单位系统变化：体模量按 Stress 维度换算数值并同步标签
        */
        void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx) override;
        /*
        * 析构函数
        */
        ~mBulkModulusProp()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

    private:
        QStringList units_;
        //控件成员
        QVector<InputWidgetSet> inputWidgets;
    };

    /***********************************声学材料阻尼（流体阻尼系数）***************************************/
    class MMAINWINDOW_EXPORT mAcousticsDampingProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mAcousticsDampingProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mAcousticsDampingProp()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;
    private:
        //控件成员
        QVector<InputWidgetSet> inputWidgets;
    };

    /***********************************时域prony***************************************/
    class MMAINWINDOW_EXPORT mTimeDomainPronyProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mTimeDomainPronyProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mTimeDomainPronyProp()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

    private slots:
        /*
        * 切换输入
        */
        void slot_switchInputs(int i);

    private:
        //控件成员
        QVector<InputWidgetSet> inputWidgets;
        QHash<MaterialPropType, int>type2inp;

        //时温等效
        singleCheckBox checkbox_;
        QStringList units_;
        singleComboBox combobox_;
        QVector<InputWidgetSet> inputWidgets2;
        QVector<InputWidgetSet> inputWidgetsTmp1;
        QVector<InputWidgetSet> inputWidgetsTmp2;
        QHash<MaterialPropType, int>type2inp1;
        QHash<MaterialPropType, int>type2inp2;

    };

    /***********************************时温等效***************************************/
    class MMAINWINDOW_EXPORT mThermalEquivalenceProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mThermalEquivalenceProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mThermalEquivalenceProp()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

        virtual void connectUnitSystemResponse();

        virtual void disconnectUnitSystemResponse();

        virtual void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx);
    private slots:
        /*
        * 切换输入
        */
        void slot_switchInputs(int i);

    private:
        QStringList units_;
        //控件成员
        singleComboBox combobox_;
        QVector<InputWidgetSet> inputWidgets;
        QVector<InputWidgetSet> inputWidgetsTmp1;
        QVector<InputWidgetSet> inputWidgetsTmp2;
        QHash<MaterialPropType, int>type2inp1;
        QHash<MaterialPropType, int>type2inp2;
    };

    /***********************************垫片材料 Shim层***************************************/
    class MMAINWINDOW_EXPORT mShimProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mShimProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mShimProp() override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;

        QVector<foldDetail*>  getAllInputsFold(QVBoxLayout* mainLayout,
            QList<FoldSection*>& folds) override;

        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override {};
    };

    /***********************************弹性参数***************************************/
    class MMAINWINDOW_EXPORT mFlexProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mFlexProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mFlexProp() override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

        /*
        * 单位系统变化：弹性参数杨氏模量按 Stress 维度换算数值并同步标签
        */
        void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx) override;
        /*
        * 返回多折叠名称
        */
        QVector<foldDetail*> getAllInputsFold(QVBoxLayout* mainLayout,
            QList<FoldSection*>& folds) override;
        /*
            * 控制表格信号连接
            */
        void connTable(bool conn);

    private slots:
        /*
        * 表格连接
        */
        void slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title);
        /*
        * 切换输入
        */
        void slot_paraswitchInputs(int i);
    private:
        QVector<InputWidgetSet> inputWidgets;
        singleCheckBox checkboxtemp_;
        // 温度相关行封装（数据载体 checkboxtemp_ 的 "curve" 属性与其同步）
        mTempCurveRowWidget* m_tempCurveRow = nullptr;
    };

    /***********************************塑性参数***************************************/
    class MMAINWINDOW_EXPORT mPlasticityProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mPlasticityProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mPlasticityProp() override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;
        /*
        * 返回多折叠名称
        */
        QVector<foldDetail*> getAllInputsFold(QVBoxLayout* mainLayout,
            QList<FoldSection*>& folds) override;
        /*
        * 控制表格信号连接
        */
        void connTable(bool conn);

    private slots:
        /*
        * 表格连接
        */
        void slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title);
        /*
        * 切换输入
        */
        void slot_paraswitchInputs(int i);

    private:
        singleComboBox singleComboHardModle_;
        singleCheckBox checkboxtemp2_;
        // 温度相关行封装（数据载体 checkboxtemp2_ 的 "curve" 属性与其同步）
        mTempCurveRowWidget* m_tempCurveRow = nullptr;
    };

    /***********************************Johnson-Cook塑性参数***************************************/
    class MMAINWINDOW_EXPORT mPlasticityJohnsonProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mPlasticityJohnsonProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mPlasticityJohnsonProp() override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

    private slots:


    private:
        QHash<MaterialPropType, int>type2inp;
        QVector<InputWidgetSet> inputWidgets;
    };

    /***********************************带阶数的塑性参数***************************************/
    class MMAINWINDOW_EXPORT mPlasticityOrderProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mPlasticityOrderProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mPlasticityOrderProp() override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

        /*
        * 单位系统变化：塑性参数0项及奇数项按 Stress 维度换算数值并同步标签(偶数项无量纲)
        */
        void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx) override;
        /*
        * 返回多折叠名称
        */
        QVector<foldDetail*> getAllInputsFold(QVBoxLayout* mainLayout,
            QList<FoldSection*>& folds) override;

    public:
        /*
        * 设置阶数
        */
        void setOrder(int order);
        /*
        * 获取阶数
        */
        int getOrder() const;
        /*
        * 获取所有输入控件（包括隐藏的）
        */
        QVector<InputWidgetSet> getAllInputWidgets() const;
        /*
        * 根据阶数获取可见输入控件
        */
        QVector<InputWidgetSet> getVisibleInputWidgets() const;
        /*
        * 获取阶数控件
        */
        singleSpinBox* getSpinBox();
        /*
        * 获取类型到索引映射
        */
        QHash<MaterialPropType, int> getType2Inp() const;

    private slots:
        /*
        * 阶数变化
        */
        void slot_stepChanged(int step);

    private:
        singleSpinBox spinbox_;
        QVector<InputWidgetSet> inputWidgets2;
        QVector<InputWidgetSet> inputWidgets2Tmp;
        QHash<MaterialPropType, int> type2inp;

        QVBoxLayout* mainLayout_ = nullptr;
        QList<FoldSection*>* folds_ = nullptr;
    };

    /***********************************率相关***************************************/
    class MMAINWINDOW_EXPORT mRateDependentProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mRateDependentProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mRateDependentProp()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

    private slots:
     
        /*
        * 类型改变
        */
        void slot_typeChanged(int i);

    private:
        //控件成员
        QVector<InputWidgetSet> inputWidgetsTmp;
        QVector<InputWidgetSet> inputWidgets;

        singleComboBox combobox_;
        QHash<MaterialPropType, int>type2inp;
    };

    /***********************************厚度行为***************************************/
    class MMAINWINDOW_EXPORT mThicknessBehaviorProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mThicknessBehaviorProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mThicknessBehaviorProp() override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;
        /*
        * 返回多折叠名称
        */
        QVector<foldDetail*> getAllInputsFold(QVBoxLayout* mainLayout,
            QList<FoldSection*>& folds) override;
        /*
        * 控制表格信号连接
        */
        void connTable(bool conn);

    private slots:
        /*
        * 表格连接
        */
        void slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title);
        /*
        * 切换输入
        */
        void slot_paraswitchInputs(int i);

    private:
        singleComboBox comboThicknessType_;
        singleLabelBtn loadingCurveBtn_;
        singleCheckBox unloadingCurveCheck_;
        QStringList loadingHeader_;
        QStringList unloadingHeader_;
    };

    /***********************************膜行为***************************************/
    class MMAINWINDOW_EXPORT mFilmBehaviorProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mFilmBehaviorProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mFilmBehaviorProp() override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

        /*
        * 单位系统变化：薄膜杨氏模量按 Stress 维度换算数值并同步标签
        */
        void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx) override;
        /*
        * 返回多折叠名称
        */
        QVector<foldDetail*> getAllInputsFold(QVBoxLayout* mainLayout,
            QList<FoldSection*>& folds) override;

    private:
        QVector<InputWidgetSet> inputWidgets;
    };

    /***********************************剪切行为***************************************/
    class MMAINWINDOW_EXPORT mCutBehaviorProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mCutBehaviorProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mCutBehaviorProp() override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

        /*
        * 单位系统变化：剪切刚度按 Stress 维度换算数值并同步标签
        */
        void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx) override;
        /*
        * 返回多折叠名称
        */
        QVector<foldDetail*> getAllInputsFold(QVBoxLayout* mainLayout,
            QList<FoldSection*>& folds) override;

    private:
        QVector<InputWidgetSet> inputWidgets;
    };

    /***********************************断裂材料-材料参数***************************************/
    class MMAINWINDOW_EXPORT mBreakParamProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mBreakParamProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mBreakParamProp() override;
        /*
        * 初始化输入控件
        */
        void initInputs() override;
        /*
        * 清除输入控件
        */
        void clearInputs() override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

        /*
        * 单位系统变化：杨氏模量按 Stress 维度换算数值并同步标签
        */
        void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx) override;

    private:
        //控件成员
        QVector<InputWidgetSet> inputWidgets;
        QHash<MaterialPropType, int> type2inp;
    };

    /***********************************断裂材料-相场参数***************************************/
    class MMAINWINDOW_EXPORT mPhaseFieldProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mPhaseFieldProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mPhaseFieldProp() override;
        /*
        * 初始化输入控件
        */
        void initInputs() override;
        /*
        * 清除输入控件
        */
        void clearInputs() override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

        /*
        * 单位系统变化：特征长度W按 Length 维度换算数值并同步标签
        */
        void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx) override;

    private:
        //控件成员
        singleComboBox comboPhaseFieldModel_;
        singleComboBox comboCohesionModel_;
        InputWidgetSet inputSpecifileW_;
    };

    /***********************************断裂材料***************************************/
    class MMAINWINDOW_EXPORT mBreakProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        * 第二个参数为子属性枚举列表，默认包含材料参数与相场参数
        */
        mBreakProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mBreakProp() override;
        /*
        * 初始化输入控件
        */
        void initInputs() override;
        /*
        * 清除输入控件
        */
        void clearInputs() override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;

        QVector<foldDetail*> getAllInputsFold(
            QVBoxLayout* mainLayout, QList<FoldSection*>& folds)
            override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

    private:
        //子属性
        QVector<MxFunctions::MaterialClass> subProps_;
        mBreakParamProp* m_BreakParamProp = nullptr;
        mPhaseFieldProp* m_PhaseFieldProp = nullptr;
    };

    /***********************************热传导系数***************************************/
    class MMAINWINDOW_EXPORT mHeatConductivityCoefficientProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mHeatConductivityCoefficientProp(QWidget* parent = nullptr, bool isCheckable = true, MxFunctions::MaterialClass classKey = MxFunctions::MAT_HeatConductivityCoefficient);
        /*
        * 单位系统变化：标签随温度单位(C/K)分支刷新；数值换算待管理器补 ThermalConductivity case
        */
        void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx) override;
        /*
        * 析构函数
        */
        ~mHeatConductivityCoefficientProp()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

        /*
        * 控制表格信号连接
        */
        void connTable(bool conn);

    private slots:
        /*
        * 表格连接
        */
        void slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title);

        /*
        * 热传导系数类型改变
        */
        void slot_typeChanged(int i);

    private:
        singleComboBox combobox_;
        QVector<InputWidgetSet> inputWidgetsTmp;
        QVector<InputWidgetSet> inputWidgets;
        singleCheckBox checkboxtemp_;
        // 温度相关行封装（列随类型切换经 refreshCurveHeaders 更新）
        mTempCurveRowWidget* m_tempCurveRow = nullptr;
        // 按当前类型刷新封装行的曲线列标题（温度 + 当前 inputWidgets 标签）
        void refreshCurveHeaders();
        QHash<MaterialPropType, int>type2inp;
        bool m_bIsCheckable = true;
    };

    /***********************************比热容系数（热容密度）***************************************/
    class MMAINWINDOW_EXPORT mHeatCapacityDensityProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mHeatCapacityDensityProp(QWidget* parent = nullptr);
        /*
        * 单位系统变化：标签随温度单位(C/K)分支刷新；数值换算待管理器补 HeatCapacityDensity case
        */
        void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx) override;
        /*
        * 析构函数
        */
        ~mHeatCapacityDensityProp()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

        /*
        * 控制表格信号连接
        */
        void connTable(bool conn);

    private slots:
        /*
        * 表格连接
        */
        void slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title);

    private:
        QVector<InputWidgetSet> inputWidgets2;
        singleCheckBox checkboxtemp2_;
        // 温度相关行封装（数据载体 checkboxtemp2_ 的 "curve" 属性与其同步）
        mTempCurveRowWidget* m_tempCurveRow = nullptr;
    };

    /***********************************声速***************************************/
    class MMAINWINDOW_EXPORT mSoundVelocityProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mSoundVelocityProp(QWidget* parent = nullptr);
        /*
        * 单位系统变化：声速按 VelocityOfSound 维度换算数值并同步标签
        */
        void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx) override;
        /*
        * 析构函数
        */
        ~mSoundVelocityProp()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

        /*
        * 控制表格信号连接
        */
        /*void connTable(bool conn);*/

    private slots:
        /*
        * 表格连接
        */
        /*       void slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title);*/

    private:
        QVector<InputWidgetSet> inputWidgets;
    };

    /***********************************各向同性***************************************/
    class MMAINWINDOW_EXPORT mIsotropyProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mIsotropyProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mIsotropyProp()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;

        /*
        * 清空数据
        */
        void clearData() override;

        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

        /*
        * 单位系统变化：各向同性杨氏模量按 Stress 维度换算数值并同步标签(泊松比无量纲)
        */
        void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx) override;

        /*
        * 控制表格信号连接
        */
        void connTable(bool conn);

    private slots:
        /*
        * 表格连接
        */
        void slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title);

    private:
        //控件成员
        singleCheckBox checkboxtemp_;
        QVector<InputWidgetSet> inputWidgets{ 2 };

        QStringList header_;

        // 温度相关行封装：标签 + 勾选框 + 吸附式曲线弹窗入口按钮
        mTempCurveRowWidget* m_tempCurveRow = nullptr;

        // 勾选状态以封装勾选框为准，数据载体 checkboxtemp_.cb 同步
        void setTempRelatedChecked(bool checked);
    };

    /***********************************各向异性***************************************/
    class MMAINWINDOW_EXPORT mAnisotropyProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mAnisotropyProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mAnisotropyProp()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

        /*
        * 单位系统变化：各向异性G系数按 Stress 维度换算数值并同步标签
        */
        void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx) override;

        /*
        * 控制表格信号连接
        */
        void connTable(bool conn);

    private slots:
        /*
        * 表格连接
        */
        void slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title);

        /*
        * 切换输入
        */
        void slot_switchInputs(int i);

    private:
        QHash<MaterialPropType, int>type2inp1;
        QHash<MaterialPropType, int>type2inp2;
        //singleComboBox combobox_;
        radioButBox radioButBox_;

        singleCheckBox checkboxtemp_;
        // 温度相关行封装（列随单选类型切换经 refreshCurveHeaders 更新）
        mTempCurveRowWidget* m_tempCurveRow = nullptr;
        // 按当前类型刷新封装行的曲线列标题（温度 + 当前 inputWidgets 标签）
        void refreshCurveHeaders();
        QVector<InputWidgetSet> inputWidgets;

        QVector<InputWidgetSet> inputWidgetsTmp1;
        QVector<InputWidgetSet> inputWidgetsTmp2;
    };

    /***********************************正交各向异性***************************************/
    class MMAINWINDOW_EXPORT mOrthotropyProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mOrthotropyProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mOrthotropyProp()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

        /*
        * 单位系统变化：正交各向异性工程常数按 Stress 维度换算数值并同步标签(泊松比无量纲)
        */
        void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx) override;
        /*
        * 控制表格信号连接
        */
        void connTable(bool conn);

    private slots:
        /*
        * 表格连接
        */
        void slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title);

        /*
        * 切换输入
        */
        void slot_switchInputs(int i);

    private:
        QHash<MaterialPropType, int>type2inp1;
        QHash<MaterialPropType, int>type2inp2;
        QHash<MaterialPropType, int>type2inp3;
        QStringList units_;
        singleComboBox combobox_;

        //把combobox换成radioButBox
        radioButBox radioButBox_;

        singleCheckBox checkboxtemp_;
        // 温度相关行封装（列随单选类型切换经 refreshCurveHeaders 更新）
        mTempCurveRowWidget* m_tempCurveRow = nullptr;
        // 按当前类型刷新封装行的曲线列标题（温度 + 当前 inputWidgets 标签）
        void refreshCurveHeaders();
        QVector<InputWidgetSet> inputWidgets;
        QVector<InputWidgetSet> inputWidgetsTmp1;
        QVector<InputWidgetSet> inputWidgetsTmp2;
        QVector<InputWidgetSet> inputWidgetsTmp3;
    };

    /***********************************各向同性热材料***************************************/
    class MMAINWINDOW_EXPORT mIsotropicThermalProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mIsotropicThermalProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mIsotropicThermalProp()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

        /*
        * 单位系统变化：热传导系数/热容密度标签随温度单位(C/K)分支刷新，数值按各自维度换算
        */
        void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx) override;

        /*
        * 控制表格信号连接
        */
        void connTable(bool conn);

    private slots:
        /*
        * 表格连接
        */
        void slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title);
        /*
        * 热传导系数类型改变
        */
        void slot_typeChanged(int i);
    private:
        ClickableGroupBox* group1 = nullptr;
        ClickableGroupBox* group2 = nullptr;
        QVBoxLayout* layout1 = nullptr;
        QVBoxLayout* layout2 = nullptr;
        QWidget* widget1 = nullptr;
        QWidget* widget2 = nullptr;
        QVector<InputWidgetSet> inputWidgetsTmp;
        QVector<InputWidgetSet> inputWidgets;
        QVector<InputWithCheckBox> inputWidgets2;
        singleCheckBox checkboxtemp_;
        singleCheckBox checkboxtemp2_;
        // 两个温度相关行封装：热传导系数（列随类型切换）+ 比热容系数
        mTempCurveRowWidget* m_tempCurveRow = nullptr;
        mTempCurveRowWidget* m_tempCurveRow2 = nullptr;
        // 按当前类型刷新热传导曲线列标题（温度 + 当前 inputWidgets 标签）
        void refreshCurveHeaders();
        singleComboBox combobox_;
        QHash<MaterialPropType, int>type2inp;
    };

    /***********************************弹塑性-各向同性硬化***************************************/
    class MMAINWINDOW_EXPORT mIsotropicHardeningProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mIsotropicHardeningProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mIsotropicHardeningProp()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;

        /*
        * 返回多折叠名称
        */
        QVector<foldDetail*>  getAllInputsFold(QVBoxLayout* mainLayout,
            QList<FoldSection*>& folds) override;

        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override {};
    };

    /***********************************弹塑性-随动硬化***************************************/
    class MMAINWINDOW_EXPORT mKinematicHardeningProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mKinematicHardeningProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mKinematicHardeningProp()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;

        QVector<foldDetail*> getAllInputsFold(QVBoxLayout* mainLayout,
            QList<FoldSection*>& folds) override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override {};
    };

    /***********************************弹塑性-组合硬化***************************************/
    class MMAINWINDOW_EXPORT mCompressiveHardeningProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mCompressiveHardeningProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mCompressiveHardeningProp()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;

        QVector<foldDetail*>  getAllInputsFold(QVBoxLayout* mainLayout,
            QList<FoldSection*>& folds) override;

        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override {};
    };

    /***********************************NeoHookean***************************************/
    class MMAINWINDOW_EXPORT mNeoHookeanProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mNeoHookeanProp(QWidget* parent = nullptr);
        /*
        * 单位系统变化：超弹性按 Stress 维度换算数值并同步标签
        */
        void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx) override;
        /*
        * 析构函数
        */
        ~mNeoHookeanProp()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;
        /*
        * 控制表格信号连接
        */
        void connTable(bool conn);

    private slots:
        /*
        * 表格连接
        */
        void slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title);

    private:
        QStringList units_;

        //控件成员
        singleCheckBox checkboxtemp_;
        // 温度相关行封装（数据载体 checkboxtemp_ 的 "curve" 属性与其同步）
        mTempCurveRowWidget* m_tempCurveRow = nullptr;
        QVector<InputWidgetSet> inputWidgets;
    };

    /***********************************NeoHookean2***************************************/
    class MMAINWINDOW_EXPORT mNeoHookean2Prop : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mNeoHookean2Prop(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mNeoHookean2Prop()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;

        /*
        * 控制表格信号连接
        */
        void connTable(bool conn);
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

        /*
        * 单位系统变化：C10按 Stress、D1按 StressReciprocal 维度换算数值并同步标签
        */
        void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx) override;

    private slots:
        /*
        * 切换输入
        */
        void slot_switchInputs(int i);

        /*
        * 切换参数
        */
        void slot_paraswitchInputs(int i);

        /*
        * 表格连接
        */
        void slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title);

    private:
        QStringList units_;

        //控件成员
        QHash<MaterialPropType, int>type2inp1;
        QHash<MaterialPropType, int>type2inp2;

        singleComboBox comboboxType_;

        //singleComboBox combobox_;
        singleCheckBox checkboxNeoPress_;
        singleCheckBox checkboxNeoV_;

        QVector<InputWidgetSet> inputWidgets;
        QVector<InputWidgetSet> inputWidgets1;
        QVector<InputWidgetSet> inputWidgets2;
        QVector<InputWidgetSet> inputWidgets3;
        //singleLabelBtn labelbtn_;

    };

    /***********************************Mooney-Rivlin***************************************/
    class MMAINWINDOW_EXPORT mMooneyRivlinProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mMooneyRivlinProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mMooneyRivlinProp()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 控制表格信号连接
        */
        void connTable(bool conn);
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

        /*
        * 单位系统变化：C10/C01按 Stress、D1按 StressReciprocal 维度换算数值并同步标签
        */
        void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx) override;
    private slots:
        /*
        * 切换输入
        */
        void slot_switchInputs(int i);

        /*
        * 切换参数
        */
        void slot_paraswitchInputs(int i);

        /*
        * 表格连接
        */
        void slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title);
    private:
        QStringList units_;

        //控件成员
        QHash<MaterialPropType, int>type2inp1;
        QHash<MaterialPropType, int>type2inp2;
        singleCheckBox checkboxNeoPress_;
        singleCheckBox checkboxNeoV_;
        singleComboBox comboboxType_;
        QVector<InputWidgetSet> inputWidgets;
        QVector<InputWidgetSet> inputWidgets1;
        QVector<InputWidgetSet> inputWidgets2;
        QVector<InputWidgetSet> inputWidgets3;
    };

    /***********************************Ogden***************************************/
    class MMAINWINDOW_EXPORT mOgdenProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mOgdenProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mOgdenProp()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

        /*
        * 单位系统变化：MU按 Stress、D按 StressReciprocal 维度换算数值并同步标签
        */
        void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx) override;
        /*
        * 控制表格信号连接
        */
        void connTable(bool conn);
    private slots:
        /*
        * 阶数变化
        */
        void slot_stepChanged(int step);
        /*
        * 切换输入
        */
        void slot_switchInputs(int i);

        /*
        * 切换参数
        */
        void slot_paraswitchInputs(int i);

        /*
        * 表格连接
        */
        void slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title);
    private:
        QStringList units_;

        //控件成员
        QHash<MaterialPropType, int>type2inp1;
        QHash<MaterialPropType, int>type2inp2;
        QHash<MaterialPropType, int>type2inpMU;
        QHash<MaterialPropType, int>type2inpAlpha;
        QHash<MaterialPropType, int>type2inpD;

        singleCheckBox checkboxNeoPress_;
        singleCheckBox checkboxNeoV_;
        singleComboBox comboboxType_;
        singleSpinBox spinbox_;
        QVector<InputWidgetSet> inputWidgets;
        QVector<InputWidgetSet> inputWidgets1;
        QVector<InputWidgetSet> inputWidgets2;
        QVector<InputWidgetSet> inputWidgetsMU;
        QVector<InputWidgetSet> inputWidgetsAlpha;
        QVector<InputWidgetSet> inputWidgetsD;
        QVector<InputWidgetSet> inputWidgetsTemp;
    };

    /***********************************Arruda-Boyce***************************************/
    class MMAINWINDOW_EXPORT mArrudaBoyceProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mArrudaBoyceProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mArrudaBoyceProp()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

        /*
        * 单位系统变化：MU按 Stress、D按 StressReciprocal 维度换算数值并同步标签
        */
        void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx) override;
        /*
        * 控制表格信号连接
        */
        void connTable(bool conn);
    private slots:
        /*
        * 切换输入
        */
        void slot_switchInputs(int i);

        /*
        * 切换参数
        */
        void slot_paraswitchInputs(int i);

        /*
        * 表格连接
        */
        void slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title);
    private:
        QStringList units_;

        //控件成员
                //控件成员
        QHash<MaterialPropType, int>type2inp1;
        QHash<MaterialPropType, int>type2inp2;
        singleCheckBox checkboxNeoPress_;
        singleCheckBox checkboxNeoV_;
        singleComboBox comboboxType_;
        QVector<InputWidgetSet> inputWidgets;
        QVector<InputWidgetSet> inputWidgets1;
        QVector<InputWidgetSet> inputWidgets2;
        QVector<InputWidgetSet> inputWidgets3;
    };

    /***********************************Yeoh***************************************/
    class MMAINWINDOW_EXPORT mYeohProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mYeohProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mYeohProp()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

        /*
        * 单位系统变化：C10~C30按 Stress、D1~D3按 StressReciprocal 维度换算数值并同步标签
        */
        void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx) override;
        /*
        * 控制表格信号连接
        */
        void connTable(bool conn);

    private slots:
        /*
        * 表格连接
        */
        void slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title);
        /*
       * 切换输入
       */
        void slot_switchInputs(int i);

        /*
        * 切换参数
        */
        void slot_paraswitchInputs(int i);
    private:
        QStringList units1_;
        QStringList units2_;
        //控件成员
        //singleCheckBox checkboxtemp_;
        singleCheckBox checkboxNeoPress_;
        singleCheckBox checkboxNeoV_;
        singleComboBox comboboxType_;
        QVector<InputWidgetSet> inputWidgets;
        QVector<InputWidgetSet> inputWidgets1;
        QVector<InputWidgetSet> inputWidgets2;
        QVector<InputWidgetSet> inputWidgets3;
        QHash<MaterialPropType, int>type2inp;

    };

    /***********************************Polynomial***************************************/
    class MMAINWINDOW_EXPORT mPolynomialProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mPolynomialProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mPolynomialProp()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

        /*
        * 单位系统变化：Cij按 Stress、Di按 StressReciprocal 维度换算数值并同步标签
        */
        void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx) override;
        /*
        * 控制表格信号连接
        */
        void connTable(bool conn);

    private slots:
        /*
        * 表格连接
        */
        void slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title);

        /*
        * 阶数变化
        */
        void slot_stepChanged(int step);
        /*
        * 切换输入
        */
        void slot_switchInputs(int i);
        /*
        * 切换参数
        */
        void slot_paraswitchInputs(int i);

    private:
        //控件成员
        QHash<MaterialPropType, int>type2inp1;
        QHash<MaterialPropType, int>type2inp2;
        singleCheckBox checkboxNeoPress_;
        singleCheckBox checkboxNeoV_;
        singleComboBox comboboxType_;
        singleSpinBox spinbox_;
        QVector<InputWidgetSet> inputWidgets;
        QVector<InputWidgetSet> inputWidgets1;
        QVector<InputWidgetSet> inputWidgets2;
        QVector<InputWidgetSet> inputWidgetsTmp1;
        QVector<InputWidgetSet> inputWidgetsTmp2;
        QVector<InputWidgetSet> inputWidgetsTemp;
    };

    /***********************************ReducedPolynomial***************************************/
    class MMAINWINDOW_EXPORT mReducedPolynomialProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mReducedPolynomialProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mReducedPolynomialProp()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

        /*
        * 单位系统变化：C10~C30按 Stress、D1~D6按 StressReciprocal 维度换算数值并同步标签
        */
        void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx) override;
        /*
        * 控制表格信号连接
        */
        void connTable(bool conn);

    private slots:
        /*
        * 表格连接
        */
        void slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title);

        /*
        * 阶数变化
        */
        void slot_stepChanged(int step);


    private:
        QStringList units1_;
        QStringList units2_;
        //控件成员
        singleCheckBox checkboxtemp_;
        // 温度相关行封装（列随应变势能阶数切换经 refreshCurveHeaders 更新）
        mTempCurveRowWidget* m_tempCurveRow = nullptr;
        // 按当前阶数刷新曲线列标题（温度 + step 个 C 系数 + step 个 D 系数）
        void refreshCurveHeaders();
        singleSpinBox spinbox_;
        singleLabelBtn labelbtn_;
        QVector<InputWidgetSet> inputWidgetsTmp1;
        QVector<InputWidgetSet> inputWidgetsTmp2;
        QVector<InputWidgetSet> inputWidgets;
        QHash<MaterialPropType, int>type2inp1;
        QHash<MaterialPropType, int>type2inp2;
    };

    /***********************************Hyperfoam 超弹泡沫***************************************/
    class MMAINWINDOW_EXPORT mHyperfoamProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mHyperfoamProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mHyperfoamProp()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

        /*
        * 单位系统变化：Mu按 Stress、Nu按 StressReciprocal 维度换算数值并同步标签(与collectProperties一致)
        */
        void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx) override;
        /*
        * 控制表格信号连接
        */
        void connTable(bool conn);
        /*
        * 控制输入界面转变
        */
        void slot_paraswitchInputs(int i);
        /*
        * 切换输入
        */
        void slot_switchInputs(int i);

    private slots:
        /*
        * 表格连接
        */
        void slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title);
        /*
        * 阶数变化
        */
        void slot_stepChanged(int step);

    private:
        QStringList units_;
        //控件成员
        //singleCheckBox checkboxtemp_;
        singleSpinBox spinbox_;
        singleComboBox combobox_;
        singleComboBox comboboxType_;
        singleLabelBtn labelbtn_;
        QVector<InputWidgetSet> inputWidgets;
        QVector<InputWidgetSet> inputWidgets1;
        QVector<InputWidgetSet> inputWidgets2;
        QVector<InputWidgetSet> inputWidgetsMU_Alpha;
        QVector<InputWidgetSet> inputWidgetsD;
        QVector<InputWidgetSet> inputWidgetsTemp;
        QHash<MaterialPropType, int>type2inp1;
        QHash<MaterialPropType, int>type2inp2;

    };

    /***********************************刚体***************************************/
    class MMAINWINDOW_EXPORT mRigidProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mRigidProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mRigidProp()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

        /*
        * 单位系统变化：刚体杨氏模量按 Stress 维度换算数值并同步标签(泊松比无量纲)
        */
        void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx) override;
        /*
        * 控制表格信号连接
        */
        void connTable(bool conn);

    private slots:
        /*
        * 表格连接
        */
        void slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title);

    private:
        QStringList units_;

        //控件成员
        singleCheckBox checkboxtemp_;
        // 温度相关行封装（数据载体 checkboxtemp_ 的 "curve" 属性与其同步）
        mTempCurveRowWidget* m_tempCurveRow = nullptr;
        QVector<InputWidgetSet> inputWidgets;
    };

    /***********************************声学***************************************/
    class MMAINWINDOW_EXPORT mAeroacousticsProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mAeroacousticsProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mAeroacousticsProp()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;
    private:
        QStringList units_;
        //控件成员
        QVector<InputWidgetSet> inputWidgets;
    };

    /***********************************变形塑性材料***************************************/
    class MMAINWINDOW_EXPORT mDeformationPlasticityProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mDeformationPlasticityProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mDeformationPlasticityProp()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

        /*
        * 单位系统变化：形变塑性应力项按 Stress 维度换算数值并同步标签
        */
        void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx) override;
        /*
        * 控制表格信号连接
        */
        void connTable(bool conn);

    private slots:
        /*
        * 表格连接
        */
        void slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title);

    private:
        QStringList units_;
        //控件成员
        QVector<InputWidgetSet> inputWidgets;

        QHash<MaterialPropType, int>type2inp;
    };

    /***********************************粘弹性材料***************************************/
    class MMAINWINDOW_EXPORT mViscousElasticProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mViscousElasticProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mViscousElasticProp()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

        /*
        * 单位系统变化：粘弹性杨氏模量按 Stress 维度换算数值并同步标签
        */
        void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx) override;
        /*
        * 控制表格信号连接
        */
        void connTable(bool conn);

    private slots:
        /*
        * 表格连接
        */
        void slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title);

    private slots:
        /*
        * 切换选项
        */
        void slot_switchInputs(int);

    private:
        QStringList units_;
        //控件成员
        singleLabelBtn lablebtn_;
        singleComboBox combobox_;
        singleComboBox combobox_trs_;
        QVector<InputWidgetSet> inputWidgets;
        QVector<InputWidgetSet> inputWidgets2;
        QVector<InputWidgetSet> inputWidgets_trs_wlf_;
        QVector<InputWidgetSet> inputWidgets_trs_arr_;
        FoldSection* fa = nullptr;
    };

    /***********************************(准)脆性材料***************************************/
    class MMAINWINDOW_EXPORT mBrittleLikeProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mBrittleLikeProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mBrittleLikeProp()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

        /*
        * 单位系统变化：应力项按 Stress、断裂能按 SurfaceEnergyDensity、特征长度按 Length 维度换算数值并同步标签
        */
        void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx) override;
    private slots:
        /*
        * 切换输入
        */
        void slot_switchInputs1(int i);

        void slot_switchInputs2(int i);


    private:
        QStringList units1_;
        QStringList units2_;

        //控件成员
        singleComboBox combobox_1;
        singleComboBox combobox_2;
        QVector<InputWidgetSet> inputWidgets;

        QHash<MaterialPropType, int>type2inp;
    };

    /***********************************压电材料***************************************/
    class MMAINWINDOW_EXPORT mPiezoelectricProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mPiezoelectricProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mPiezoelectricProp()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;
    private slots:
        /*
        * 切换输入
        */
        void slot_switchInputs1(int i);

        void slot_switchInputs2(int i);

        void slot_switchInputs3(int i);
    private:
        //控件成员
        QHash<QString, int>matname2id_;
        singleComboBox combobox_1;
        singleComboBox combobox_2;
        singleComboBox combobox_3;
        QVector<singleTableWidget> singleTableWidgets;
        QVector<InputWidgetSet> inputWidgets;
    };

    /***********************************纤维增强材料***************************************/
    class MMAINWINDOW_EXPORT mFiberReinforcedProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mFiberReinforcedProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mFiberReinforcedProp()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

        /*
        * 单位系统变化：弹性模量按 Stress、横向参数按 StressReciprocal 维度换算数值并同步标签
        */
        void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx) override;
        /*
        * 控制表格信号连接
        */
        void connTable(bool conn);

    private slots:
        /*
        * 表格连接
        */
        void slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title);

        /*
        * 层数变化
        */
        void slot_layerChanged(int value);

        /*
        * 坐标系选择
        */
        void slot_coorChanged(int i);

    private:
        QStringList units1_;
        QStringList units2_;
        //控件成员
        singleCheckBox checkboxtemp_;
        // 温度相关行封装（列随纤维层数切换经 refreshCurveHeaders 更新）
        mTempCurveRowWidget* m_tempCurveRow = nullptr;
        // 按当前层数刷新曲线列标题（温度 + C10/D1 + 每层 nx/ny/k1/k2）
        void refreshCurveHeaders();
        comboBoxWithPushBtn combobox_;
        singleSpinBox spinbox_;
        QVector<InputWidgetSet> inputWidgets;
        QVector<InputWidgetSet> inputWidgetsTmp;
        QHash<MaterialPropType, int>type2inp;

        int cnts = 1;
    };

    /***********************************通用***************************************/

    class MMAINWINDOW_EXPORT mMatFromLibCommon : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mMatFromLibCommon(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mMatFromLibCommon()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<QString, QPair<QVector<Unit>, QVariant>>collectLibProperties()override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<QString, QPair<QVector<Unit>, QVariant>>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

        /*
        * 单位系统变化：库材料属性按各输入自身维度换算数值并同步标签
        */
        void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx) override;

        virtual QHash<MxFunctions::MaterialPropType, QVariant>collectProperties()override { return QHash<MxFunctions::MaterialPropType, QVariant>(); };

    private:
        QVector<InputWidgetSet*> inputWidgets;
        QVector<QWidget*> curvebtns;
        QHash<QString, QPair<QVector<Unit>, QVariant>> curves_;

    };

    /***********************************结构***************************************/

    class MMAINWINDOW_EXPORT mMatFromLibStructural : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mMatFromLibStructural(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mMatFromLibStructural()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<QString, QPair<QVector<Unit>, QVariant>> collectLibProperties()override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<QString, QPair<QVector<Unit>, QVariant>>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

        /*
        * 单位系统变化：库材料属性按各输入自身维度换算数值并同步标签
        */
        void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx) override;

        virtual QHash<MxFunctions::MaterialPropType, QVariant>collectProperties()override { return QHash<MxFunctions::MaterialPropType, QVariant>(); };

    private:
        QStringList units_;
        QVector<InputWidgetSet*> inputWidgets;
        QVector<QWidget*> curvebtns;
        QHash<QString, QPair<QVector<Unit>, QVariant>> curves_;
    };

    /***********************************传热***************************************/

    class MMAINWINDOW_EXPORT mMatFromLibThermal : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mMatFromLibThermal(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mMatFromLibThermal()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<QString, QPair<QVector<Unit>, QVariant>>collectLibProperties()override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<QString, QPair<QVector<Unit>, QVariant>>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

        /*
        * 单位系统变化：库材料属性按各输入自身维度换算数值并同步标签
        */
        void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx) override;

        virtual QHash<MxFunctions::MaterialPropType, QVariant>collectProperties()override { return QHash<MxFunctions::MaterialPropType, QVariant>(); };

    private:
        QStringList units_;
        QStringList units1_;
        QStringList units2_;
        QStringList units3_;
        QStringList units4_;

        QVector<InputWidgetSet*> inputWidgets;
        QVector<QWidget*> curvebtns;
        QHash<QString, QPair<QVector<Unit>, QVariant>> curves_;

    };

    /***********************************导电***************************************/

    class MMAINWINDOW_EXPORT mMatFromLibElectic : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mMatFromLibElectic(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mMatFromLibElectic()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<QString, QPair<QVector<Unit>, QVariant>>collectLibProperties()override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<QString, QPair<QVector<Unit>, QVariant>>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

        /*
        * 单位系统变化：库材料属性按各输入自身维度换算数值并同步标签
        */
        void slot_unitSystemResponse(CommonData::UnitType type, QVector<int> idx) override;
    private:
        QVector<InputWidgetSet*> inputWidgets;
        QVector<QWidget*> curvebtns;
        QHash<QString, QPair<QVector<Unit>, QVariant>> curves_;

    };

    /***********************************默认属性***************************************/
    class MMAINWINDOW_EXPORT mDefaultProperty : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mDefaultProperty(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mDefaultProperty()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;

        /*
        * 设置颜色
        */
        void setColor(const QColor& color);

        /*
        * 所有输入控件
        */
        void InitFoldSection() override;

        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;

        virtual QHash<MxFunctions::MaterialPropType, QVariant>collectProperties()override { return QHash<MxFunctions::MaterialPropType, QVariant>(); };

    signals:
        void sg_colorSelected(QColor color);

    private:
        singleButton _colorBtn;
        QColor _currentColor = QColor(Qt::gray);
    };

    /***********************************剪切损伤***************************************/
    class MMAINWINDOW_EXPORT mShearDamageProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mShearDamageProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mShearDamageProp()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;

        /*
        * 清空数据
        */
        void clearData() override;

        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

        /*
        * 控制表格信号连接
        */
        void connTable(bool conn);

    private slots:
        /*
        * 表格连接
        */
        void slot_connTable(QVector<QVector<double>> x, QVector<QVector<double>> y, QString title);
        /*
        * 损伤演化复选框切换
        */
        void slot_evolutionToggled(int state);

    private:
        //控件成员
        singleCheckBox checkboxtemp_;
        // 断裂曲线行封装（数据载体 checkboxtemp_ 的 "curve" 属性与其同步）
        mTempCurveRowWidget* m_tempCurveRow = nullptr;
        QVector<InputWidgetSet> inputWidgets;

        QStringList header_;

        //损伤演化(原独立折叠框，内嵌为复选框子控件)
        singleCheckBox evolutionCheck_;
        singleComboBox evolutionTypeCombo_;
        singleComboBox evolutionModeCombo_;
        QVector<InputWidgetSet> evolutionWidgets_;

        /*
        * 按损伤演化勾选状态重建折叠框内行
        */
        void updateFoldRows();
    };


    /***********************************损伤演化***************************************/
    class MMAINWINDOW_EXPORT mDamageEvolutionProp : public mMaterialPropBase
    {
        Q_OBJECT

    public:
        /*
        * 构造函数
        */
        mDamageEvolutionProp(QWidget* parent = nullptr);
        /*
        * 析构函数
        */
        ~mDamageEvolutionProp()override;
        /*
        * 初始化输入控件
        */
        void initInputs()override;
        /*
        * 清除输入控件
        */
        void clearInputs()override;
        /*
        * 收集所有属性
        */
        QHash<MxFunctions::MaterialPropType, QVariant> collectProperties() override;
        /*
        * 控件设值
        */
        void setInputsValue(const QHash<MxFunctions::MaterialPropType, QVariant>& prop) override;
        /*
        * 所有输入控件
        */
        void InitFoldSection() override;
        /*
        * 清空数据
        */
        void clearData() override;
        /*
        * 更新当前界面单位制
        */
        void updatePropUnit() override;

    private slots:

        /*
        * 类型改变
        */
        void slot_typeChanged(int i);
        /*
        * 模式改变
        */
        void slot_modeChanged(int i);

    private:
        /*
        * 更新位移类型线性模式下的输入列表
        */
        void updateDispLinerInputWidgets();

        //控件成员
        QVector<InputWidgetSet> inputWidgetsTmp;
        QVector<InputWidgetSet> inputWidgets;

        singleComboBox combobox_type;
        singleComboBox combobox_mode;
        QHash<MaterialPropType, int>type2inp;
    };
}
