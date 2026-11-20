#pragma once
//解决中文乱码
#pragma execution_character_set("utf-8")

#include "MXContactData_global.h"
#include "mPhysicalFieldEnum.h"
#include <QString>
#include <QPair>
#include <QList>
#include <mBasicEnum.h>

using namespace MxFunctions;
namespace MechContactData
{
	class MXCONTACTDATA_EXPORT mContactBase
	{
	public:
		/*
		* 重载构造函数
		*/
		mContactBase(int id,MxFunctions::ContactType type, const QString& name = "", bool isUserRename = false);

		/*
		* 析构函数
		*/
		~mContactBase();

		/*
		* 设置接触ID
		*/
		void setContactId(int id);

		/*
		* 获取接触ID
		*/
		int getContactId();

		/*
		* 获取是否为用户重命名
		*/
		bool getIsUserRename();

		/*
		* 设置接触名
		*/
		void setContactName(const QString& name, bool isUserRename = true);

		/*
		* 获取接触名
		*/
		QString getContactName();

		/*
		* 设置接触类型
		*/
		void setContactType(MxFunctions::ContactType type);
		/*
		* 获取接触类型
		*/
		MxFunctions::ContactType getContactType();

		/*
		* 获取接触英文
		*/
		QString contactTypeToStringEn(MxFunctions::ContactType type);

		/*
		* 获取接触类型的字符串
		*/
		QString getContactTypeStr();

		/*
		 * 设置接触的描述
		 */
		void setContactDescribe(const QString& describe);

		/*
		 * 获取接触的描述
		 */
		QString getContactDescribe();

		/*
		* 拷贝分析接触
		*/
		mContactBase* copyContact(const int & id,const QString& name);

		/*
		* 设置抑制状态
		*/
		void setSuppressed(bool suppressed);

		/*
		* 获取抑制状态
		*/
		bool isSuppressed();

		/*
		* 设置，获取拾取状态
		*/
		void setPickStateMaster(int pickstate);
		int getPickStateMaster();
		void setPickStateSlave(int pickstate);
		int getPickStateSlave();

		friend QDataStream& operator>>(QDataStream& in, mContactBase& obj);
		friend QDataStream& InStreammContactBaseV3(QDataStream& in, mContactBase& obj);
		friend QDataStream& operator<<(QDataStream& out, mContactBase& obj);

	private:
		
		//接触ID
		int _id;

		//接触名字
		QString _contactName;

		//接触类型（面面接触、点面接触、通用接触）
		MxFunctions::ContactType _contactType;

		//接触描述
		QString _contactDescribe;

		//是否为用户重命名
		bool _isUserRename;

		//是否被抑制
		bool _isSuppressed;

		//具有通用拾取控件的面板 需要设置此参数 0表示“无” 1表示“无临时数据” 2表示“有临时数据”
		int _pickStateMaster;   //主面 和 自接触
		int _pickStateSlave;	//从面
	public:
		/*************接触对*************/ //**************可选参数 通用接触不用该数据和折叠块
		//主面和从面数据存储到内部集合的***************
		//主面集合正向（true为正向，false为反向）
		bool masterDir = true;

		//从面集合反向（true为正向，false为反向）
		bool slaveDir = true;

		//从面区域类型（点面/线面接触用） 0单元面 1节点 —— 决定从面拾取控件的拾取类型与集合选择类型
		int slave_region = 0;

		/*************接触参数*************/ //****************必选参数
		//小滑移（开启、关闭）    ***************必选参数
		bool smallSlip = false;

		//自接触搜索（开启、关闭）    ***************可选参数 点面接触不用该数据
		bool selfContactSearch = false;

		/*************法向接触行为*************/ //**************可选参数 都用该数据和折叠块
		//压力过盈（硬接触、完全绑定接触、法向不分离接触、线性接触）    ***************必选参数
		QString overclosure = "硬接触";

		//约束方法（罚函数法、增广-摄动拉格朗日乘子法、多点约束法）    ***************可选参数 （最新关键字参数已经无区别）
		// 点面接触--------在压力过盈选项为硬接触可选罚函数法和增广-摄动拉格朗日乘子法；在压力过盈选项为完全绑定接触可选多点约束法；在压力过盈选项为法向不分离接触可选罚函数法和多点约束法；都不满足则不用该参数
		// 面面接触--------在压力过盈选项为硬接触可选罚函数法和增广-摄动拉格朗日乘子法；在压力过盈选项为完全绑定接触可选罚函数法和多点约束法；在压力过盈选项为法向不分离接触可选罚函数法和多点约束法和增广-摄动拉格朗日乘子法；都不满足则不用该参数
		QString constraint = "罚函数法";

		//缩放系数（值）***************可选参数 在约束方法选项为罚函数法用该数据，否则隐藏该参数
		QString scale_factor = "1";

		//罚系数（值）***************可选参数 在压力过盈选项为线性接触用该数据，否则隐藏该参数
		QString penalty_factor = "";

		/*************切向行为*************/ //******************可选参数
		//定义切向行为（开启、关闭）    ***************必选参数 关闭后则禁用切向行为的其它参数
		bool tangential_define = false;

		//摩擦公式（通用接触摩擦、无摩擦、粗糙）    ***************必选参数 只有定义切向行为为开启用该数据
		QString tangential_friction = "无摩擦"; 

		//静摩擦系数（值）    ***************可选参数 只有定义切向行为为开启且摩擦公式为通用接触摩擦用该数据，否则隐藏
		double tangential_static_coef = 0.0; 

		/*************热接触间隙传导*************/ //******************可选参数

		//定义热接触间隙传导（开启、关闭）    ***************必选参数 关闭后则禁用热接触间隙传导的其它参数
		bool heat_define = false;

		//导热系数——间隙值（表格）    ***************可选参数 只有定义热接触间隙传导开启用该数据
		QList<QPair<double, double>> heat_value;

		/*************接触间隙*************/ //******************可选参数 面面接触/通用接触用该数据和折叠块

		//定义接触间隙（开启、关闭）    ***************必选参数 关闭后则禁用接触间隙的其它参数
		bool gap_define = false;

		//过盈量（值）    ***************必选参数 只有定义接触间隙为开启用该数据
		double gap_overclosure = 0.0;

		/*************几何调整*************/ //******************可选参数 面面接触/通用接触用该数据和折叠块

		//定义几何调整（开启、关闭）    ***************必选参数 关闭后则禁用几何调整的其它参数
		bool geom_define = false;

		//几何调整值（值）    ***************可选参数 只有定义几何调整为开启用该数据
		double geom_value = 0.0;

		//间隙配合方式 0零间隙配合 1逐步消除间隙配合量 
		int clr_fit_typ = 0;

		//间隙配合方式数值
		QString size = "";

		/*************接触压力均匀性评估指标*************/ //******************可选参数 接触压力均匀性评估指标折叠块
		bool evaluation = false;
		bool eva_MP = false;
		bool eva_SD = false;
		bool eva_CV = false;
		bool eva_PC = false;
		bool eva_R = false;
	};
}
