#pragma once
#pragma execution_character_set("utf-8")

#include "mmainwindow_global.h"

//QT
#include <QVBoxLayout>


#include "mPanelBase.h"


#include "mMaterialBase.h"
#include "mMaterialProps.h"
#include "mUnitSystemManager.h"

#include<memory>

using namespace MechMaterialData;
namespace MechMainWindow
{
	class mCommondBase;
	class MMAINWINDOW_EXPORT mMaterialPanel : public mPanelBase, public std::enable_shared_from_this<mMaterialPanel>
	{
		Q_OBJECT

	public:
		/*
		* 构造函数
		*/
		mMaterialPanel(QWidget* parent = nullptr);
		
		/*
		* 析构函数
		*/
		~mMaterialPanel();


        /*
        * 创建材料
        */
        void createMaterial(MxFunctions::MaterialType matType = LinearElastic_Isotropic);

        /*
        * 编辑材料
        */
        void editMaterial(int matId);

        /*
        * 添加材料属性
        */
        void addMatProp(const MxFunctions::MaterialClass& prop);

        /*
        * 移除材料属性
        */
        void remMatProp(const MxFunctions::MaterialClass& prop);

        /*
        * 添加材料
        */
        void addMaterial(const QString& name, const QHash<MxFunctions::MaterialClass, QHash<MxFunctions::MaterialPropType, QVariant>>& props);

        /*
        * 从材料库添加材料
        */
        void addMaterialFromLib(const QString& name, const QHash<MxFunctions::MaterialClass, QHash<QString, QPair<QVector<Unit>, QVariant>>>& props);

        /*
        * 移除属性
        */
        void removeMaterialProp(const MxFunctions::MaterialClass& propname);

        /*
        * 隐藏所有已添加的材料属性折叠块（保留缓存，不释放）
        */
        void clearAllPropFolds();

        /*
        * 取消编辑
        */
        void unEditMaterial();

        /*
        * 完成编辑后属性存储与差异比对
        */
        void updatePropsAndFindDiff();

        /*
        * 清空材料属性数据
        */
        void clearPropData(const MxFunctions::MaterialClass& propname);

        /*
        * 控件单位制更新
        */
        void updatePanelUnit();

    public slots:
        /*
        * 编辑完成
        */
        void slot_finishEdit();
        /*
        * 颜色选择完成
        */
        void slot_colorSelectedFinished(QColor color);

        /*
        * 隐藏折叠块
        */
        void slot_hideFold1(int index);

        /*
        * 材料类型切换
        */
        void slot_TypeChanged(MxFunctions::MaterialType type);

        //硬化模型
        void slot_type1_1Changed(int index);

    private:
        /*
        * 连接信号
        */
        void connectSig(mMaterialPropBase* prop);

        /*
        * 断开信号
        */
        void disconnectSig(mMaterialPropBase* prop);

        //勾选状态改变
        void updateFoldCheckState(MxFunctions::MaterialClass matClass, bool encheckable);

    private:

        //存储每个材料类型 一个材料有两个foldSection时存两个相同的材料type
        QVector<MxFunctions::MaterialClass> _props;

        //存储每个材料类型对应的foldSection的id
        QVector<QPair<MxFunctions::MaterialClass, int>> _propids;


        int propcnt = 1;
        //增加维护每个foldSection的必选状态
        QVector<QPair<MxFunctions::MaterialClass, bool>> m_fold2PropsVec;

        QVBoxLayout* m_pMainLayout = nullptr;
        //材料数据
        std::shared_ptr<MechMaterialData::mMaterialBase> m_MatData;

        //界面展示配置
        QHash<MxFunctions::MaterialType, QVector<MxFunctions::MaterialClass>> m_PanelConfig;

        //属性组控件缓存
        QHash<MxFunctions::MaterialClass, FoldSection*> m_FoldCache;

        //用来重新勾选折叠框
        QHash<int, QHash<MxFunctions::MaterialClass, bool>> m_PropsCheckHash;

        QVector<MxFunctions::MaterialClass> m_SelectProps;
    };

}