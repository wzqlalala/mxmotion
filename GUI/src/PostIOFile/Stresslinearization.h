#pragma once
#include <set>
#include <vector>
#include "FEMException.h"
#include "miofile_global.h"
#include <QObject>
using namespace std;
namespace MIOFile
{
	class MIOFILE_EXPORT mPostDealData :public QObject
	{
	public:
		mPostDealData() {};
		//计算局部应力计算
		void GetlocStress(map< int, std::pair< int, int >> eledatas, map< int, std::vector< int >> elenode, map< int, std::vector< double >> nodestress,
			map< int, std::vector< double >> elnodecoor, std::vector< std::pair<int, std::set< int >>> elementset, map< int, std::vector< double >> nodetcoor);
	private:
		//二维计算直线局部坐标系
		void ComputeCosystem2D(double ininode[3], double endnode[3], double Comatrix[3][3]);

		//二维应力线性化处理
		void Getstresslinear2D(double num, double lstress[7][8], vector <double>& stress11, vector <double>& stress22, vector <double>& stress33, vector <double>& stress12);



		//二维形函数和形函数的导数
		//8节点四边形
		void shapefun_8q(vector <double>& H, vector <double>& Hr, vector <double>& Hs, double r, double s);
		//4节点四边形
		void shapefun_4q(vector <double>& H, vector <double>& Hr, vector <double>& Hs, double r, double s);
		//6节点三角形
		void shapefun_6q(vector <double>& H, vector <double>& Hr, vector <double>& Hs, double r, double s);
		//3节点三角形
		void shapefun_3q(vector <double>& H, vector <double>& Hr, vector <double>& Hs, double r, double s);



		//三维计算直线局部坐标系
		void ComputeCosystem3D(double ininode[3], double endnode[3], double Comatrix[3][3]);

		//三维应力线性化处理
		void Getstresslinear3D(const int num, vector<double>& lstress, vector <double>& stress11, vector <double>& stress22, vector <double>& stress33, vector <double>& stress12, vector <double>& stress13, vector <double>& stress23);

		//三维形函数和形函数的导数
		//一阶四面体
		void shapefun_Tet4(vector <double>& H, vector <double>& Hr, vector <double>& Hs, vector <double>& Ht, double r, double s, double t);
		//二阶四面体
		void shapefun_Tet10(vector <double>& H, vector <double>& Hr, vector <double>& Hs, vector <double>& Ht, double r, double s, double t);
		//一阶三棱柱
		void shapefun_Penta6(vector <double>& H, vector <double>& Hr, vector <double>& Hs, vector <double>& Ht, double r, double s, double t);
		//二阶三棱柱
		void shapefun_Penta15(vector <double>& H, vector <double>& Hr, vector <double>& Hs, vector <double>& Ht, double r, double s, double t);
		//一阶六面体
		void shapefun_Hex8(vector <double>& H, vector <double>& Hr, vector <double>& Hs, vector <double>& Ht, double r, double s, double t);
		//二阶六面体
		void shapefun_Hex20(vector <double>& H, vector <double>& Hr, vector <double>& Hs, vector <double>& Ht, double r, double s, double t);

		//计算物理坐标对应的参数坐标
		void Getparamcoord2D(int nodenum, vector <double>& H, vector <double>& xcoord, vector <double>& ycoord, double Targetpoint[3]);
		void Getparamcoord3D(int nodenum, vector <double>& H, vector <double>& xcoord, vector <double>& ycoord, vector <double>& zcoord, double Targetpoint[3]);

	public:
		vector<double> getlinearstress();

		vector <double> getlocstress11();
		vector <double> getlocstress22();
		vector <double> getlocstress33();
		vector <double> getlocstress12();
		vector <double> getlocstress13();
		vector <double> getlocstress23();

	private:
		vector <double> linearstress{};
		vector <double> locstress11 = { 0. }; 
		vector <double> locstress22 = { 0. }; 
		vector <double> locstress33 = { 0. };
		vector <double> locstress12 = { 0. }; 
		vector <double> locstress13 = { 0. }; 
		vector <double> locstress23 = { 0. };
	};
}