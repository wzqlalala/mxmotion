#include <cmath>
#include <set>
#include <algorithm>
#include <vector>
#include <QDebug>
#include <QString>
#include "eig3.h"
#include"Stresslinearization.h"


namespace MIOFile
{

	void mPostDealData::ComputeCosystem2D(double ininode[3], double endnode[3], double Comatrix[3][3])
	{
		//使用用户选择的点计算直线的局部坐标系
		double vecx[3],vecy[3],vecz[3];
		vecz[0] = 0.; vecz[1] = 0.; vecz[2] = 1.;
		vecx[0] = endnode[0] - ininode[0];
		vecx[1] = endnode[1] - ininode[1];
		vecx[2] = endnode[2] - ininode[2];
		double lengx = sqrt(vecx[0] * vecx[0] + vecx[1] * vecx[1] + vecx[2] * vecx[2]);
		Comatrix[0][0] = vecx[0] / lengx; Comatrix[0][1] = vecx[1] / lengx; Comatrix[0][2] = vecx[2] / lengx;
		Comatrix[2][0] = vecz[0]; Comatrix[2][1] = vecz[1]; Comatrix[2][1] = vecz[2];
		double flag = vecx[0] * vecx[1];
		if (flag > 0)
		{
			vecy[0] = -(vecz[1] * vecx[2] - vecz[2] * vecx[1]);
			vecy[1] = -(vecz[2] * vecx[0] - vecz[0] * vecx[2]);
			vecy[2] = -(vecz[0] * vecx[1] - vecz[1] * vecx[0]);
		}
		else
		{
			vecy[0] = (vecz[1] * vecx[2] - vecz[2] * vecx[1]);
			vecy[1] = (vecz[2] * vecx[0] - vecz[0] * vecx[2]);
			vecy[2] = (vecz[0] * vecx[1] - vecz[1] * vecx[0]);
		}

		double lengy = sqrt(vecy[0] * vecy[0] + vecy[1] * vecy[1] + vecy[2] * vecy[2]);
		Comatrix[1][0] = vecy[0] / lengy; Comatrix[1][1] = vecy[1] / lengy; Comatrix[1][2] = vecy[2] / lengy;
	}


	void mPostDealData::Getstresslinear2D(double num, double lstress[7][8],vector <double>& stress11, vector <double>& stress22, vector <double>& stress33, vector <double>& stress12)
	{
		vector <double> Mises;
		vector <double> Tresca;
		//计算旋转后的Mise应力和Tresca应力
		for (int ii = 0; ii < num + 1; ++ii)
		{
			Mises[ii] = sqrt((0.5 * (stress11[ii] * stress11[ii] + stress22[ii] * stress22[ii] - stress11[ii] * stress22[ii] + 3. * stress12[ii] * stress12[ii])));
			double sigama1 = 0.5 * (stress11[ii] + stress22[ii]) + sqrt((0.25 * (stress11[ii] - stress22[ii]) * (stress11[ii] - stress22[ii]) + stress12[ii] * stress12[ii]));
			double sigama2 = 0.5 * (stress11[ii] + stress22[ii]) - sqrt((0.25 * (stress11[ii] - stress22[ii]) * (stress11[ii] - stress22[ii]) + stress12[ii] * stress12[ii]));
			Tresca[ii] = max(abs(sigama1), abs(sigama2));
			Tresca[ii] = max(Tresca[ii], abs(sigama1 - sigama2));
		}
		


		//计算膜应变
		double Memstress11 = 0.5 * (stress11[0] + stress11[num]);
		double Memstress22 = 0.5 * (stress22[0] + stress22[num]);
		double Memstress33 = 0.5 * (stress33[0] + stress33[num]);
		double Memstress12 = 0.5 * (stress12[0] + stress12[num]);
		double Memmises = 0.5 * (Mises[0] + Mises[num]);
		double Memtresca  = 0.5 * (Tresca[0] + Tresca[num]);

		for (int ii = 1; ii < num + 1; ++ii)
		{
			Memstress11 += stress11[ii];
			Memstress22 += stress22[ii];
			Memstress33 += stress33[ii];
			Memstress12 += stress12[ii];
			Memmises += Mises[ii];
			Memtresca += Tresca[ii];

		}
		Memstress11 = Memstress11 / num;
		Memstress22 = Memstress22 / num;
		Memstress33 = Memstress33 / num;
		Memstress12 = Memstress12 / num;
		Memmises = Memmises / num;
		Memtresca = Memtresca / num;


		//计算弯曲应变
		//弯曲部分的数据准备
		vector <double> midstress11;
		vector <double> midstress22;
		vector <double> midstress33;
		vector <double> midstress12;
		vector <double> midmises;
		vector <double> midtresca;
		for (int ii = 0; ii < num ; ++ii)
		{
			int jj = ii + 1;
			midstress11[ii] = (stress11[ii] + stress11[jj]) * 0.5;
			midstress22[ii] = (stress22[ii] + stress22[jj]) * 0.5;
			midstress33[ii] = (stress33[ii] + stress33[jj]) * 0.5;
			midmises[ii] = (Mises[ii] + Mises[jj]) * 0.5;
			midtresca[ii] = (Tresca[ii] + Tresca[jj]) * 0.5;
		}

		//计算弯曲应力
		double Bendstress11A, Bendstress22A, Bendstress33A, Bendstress12A;
		double Bendstress11B, Bendstress22B, Bendstress33B, Bendstress12B;
		double BendmisesA, BendmisesB;
		double BendtrescaA, BendtrescaB;

		double Mforce = num / 2.;
		for (int ii = 0; ii < num; ++ii)
		{
			Bendstress11A += midstress11[ii] * (Mforce - 1.);
			Bendstress22A += midstress22[ii] * (Mforce - 1.);
			Bendstress33A += midstress33[ii] * (Mforce - 1.);
			Bendstress12A += midstress12[ii] * (Mforce - 1.);
			BendmisesA += midmises[ii] * (Mforce - 1.);
			BendtrescaA += midtresca[ii] * (Mforce - 1.);

		}
		//计算用户选择第一个点的弯曲部分的线性应力
		Bendstress11A = Bendstress11A * 6. / (num * num);
		Bendstress22A = Bendstress22A * 6. / (num * num);
		Bendstress33A = Bendstress33A * 6. / (num * num);
		Bendstress12A = Bendstress12A * 6. / (num * num);
		BendmisesA = BendmisesA * 6. / (num * num);
		BendtrescaA = BendtrescaA * 6. / (num * num);

		//计算用户选择第二个点的弯曲部分的线性应力
		Bendstress11B = -Bendstress11A ;
		Bendstress22B = -Bendstress22A ;
		Bendstress33B = -Bendstress33A ;
		Bendstress12B = -Bendstress12A ;
		BendmisesB = -BendmisesA;
		BendtrescaB = -BendtrescaA;

		//计算第一个点膜部分加弯曲部分的应力应力
		double MemplusBend11_A = Memstress11 + Bendstress11A;
		double MemplusBend22_A = Memstress22 + Bendstress22A;
		double MemplusBend33_A = Memstress33 + Bendstress33A;
		double MemplusBend12_A = Memstress12 + Bendstress12A;
		double MemplusBendmise_A = Memmises + BendmisesA;
		double MemplusBendtresca_A = Memtresca + BendtrescaA;


		//计算第二个点膜部分加弯曲部分的线性应力
		double MemplusBend11_B = Memstress11 + Bendstress11B;
		double MemplusBend22_B = Memstress22 + Bendstress22B;
		double MemplusBend33_B = Memstress33 + Bendstress33B;
		double MemplusBend12_B = Memstress12 + Bendstress12B;
		double MemplusBendmise_B = Memmises + BendmisesB;
		double MemplusBendtresca_B = Memtresca + BendtrescaB;
		//计算第一个点峰顶应力
		double Peakstress11A, Peakstress22A, Peakstress33A, Peakstress12A;
		double PeakmisesA, PeaktrescaA;
		Peakstress11A = stress11[0] - MemplusBend11_A;
		Peakstress22A = stress22[0] - MemplusBend22_A;
		Peakstress33A = stress33[0] - MemplusBend33_A;
		Peakstress12A = stress12[0] - MemplusBend12_A;
		PeakmisesA = Mises[0] - MemplusBendmise_A;
		PeaktrescaA = Tresca[0] - MemplusBendtresca_A;
		//计算第二个点峰顶应力
		double Peakstress11B, Peakstress22B, Peakstress33B, Peakstress12B;
		double PeakmisesB, PeaktrescaB;
		Peakstress11B = stress11[num] - MemplusBend11_B;
		Peakstress22B = stress22[num] - MemplusBend22_B;
		Peakstress33B = stress33[num] - MemplusBend33_B;
		Peakstress12B = stress12[num] - MemplusBend12_B;
		PeakmisesB = Mises[num] - MemplusBendmise_B;
		PeaktrescaB = Tresca[num] - MemplusBendtresca_B;

		//目前暂时没有对不变量进行线性化
		lstress[0][0] = Memstress11; lstress[0][1] = Memstress22; lstress[0][2] = Memstress33;
		lstress[0][3] = Memstress12; lstress[0][6] = Memmises; lstress[0][7] = Memtresca;

		lstress[1][0] = Bendstress11A; lstress[1][1] = Bendstress22A; lstress[1][2] = Bendstress33A;
		lstress[1][3] = Bendstress12A; lstress[1][6] = BendmisesA; lstress[1][7] = BendtrescaA;

		lstress[2][0] = MemplusBend11_A; lstress[2][1] = MemplusBend22_A; lstress[2][2] = MemplusBend33_A;
		lstress[2][3] = MemplusBend12_A; lstress[2][6] = MemplusBendmise_A; lstress[2][7] = MemplusBendtresca_A;

		lstress[3][0] = Bendstress11B; lstress[3][1] = Bendstress22B; lstress[3][2] = Bendstress33B;
		lstress[3][3] = Bendstress12B; lstress[3][6] = BendmisesB; lstress[3][7] = BendtrescaB;

		lstress[4][0] = MemplusBend11_B; lstress[4][1] = MemplusBend22_B; lstress[4][2] = MemplusBend33_B;
		lstress[4][3] = MemplusBend12_B; lstress[4][6] = MemplusBendmise_B; lstress[4][7] = MemplusBendtresca_B;

		lstress[5][0] = Peakstress11A; lstress[5][1] = Peakstress22A; lstress[5][2] = Peakstress33A;
		lstress[5][3] = Peakstress12A; lstress[5][6] = PeakmisesA; lstress[5][7] = PeaktrescaA;

		lstress[6][0] = Peakstress11B; lstress[6][1] = Peakstress22B; lstress[6][2] = Peakstress33B;
		lstress[6][3] = Peakstress12B; lstress[6][6] = PeakmisesB; lstress[6][7] = PeaktrescaB;

	}

	void mPostDealData::GetlocStress(map< int, std::pair< int, int >> eledatas, map< int,std::vector< int >> elenode, map< int, std::vector< double >> nodestress,
		map< int, std::vector< double >> elnodecoor, std::vector< std::pair<int, std::set< int >>> elementset, map< int, std::vector< double >> nodecoor)
	{
		/*map<int, std::vector<double>>::iterator it;*/
		//double linearstress[7][8];
		//vector <double> locstress11 = { 0. }; vector <double> locstress22 = { 0. }; vector <double> locstress33 = { 0. };
		//vector <double> locstress12 = { 0. }; vector <double> locstress13 = { 0. }; vector <double> locstress23 = { 0. };
		const int num = nodecoor.size();
		double ininode[3];
		double endnode[3];
		ininode[0] = nodecoor[0][0]; ininode[1] = nodecoor[0][1]; ininode[2] = nodecoor[0][2];
		endnode[0] = nodecoor[num - 1][0]; endnode[1] = nodecoor[num - 1][1]; endnode[2] = nodecoor[num - 1][2];

		//计算旋转矩阵
		double Comatrix2D[3][3]; double Comatrix3D[3][3];
		ComputeCosystem2D(ininode, endnode, Comatrix2D);
		ComputeCosystem3D(ininode, endnode, Comatrix3D);
		locstress11.resize(num);
		locstress22.resize(num);
		locstress33.resize(num);
		locstress12.resize(num);
		locstress13.resize(num);
		locstress23.resize(num);


		for ( int it = 0 ; it < num; ++it)
		{
			//获得目标节点坐标
			double Targetpoint[3];
			Targetpoint[0] = nodecoor[it][0]; Targetpoint[1] = nodecoor[it][1]; Targetpoint[2] = nodecoor[it][2];
			//获得与目标节点相关的单元
			auto& nodeEle = elementset[it].second;
			//每个节点的相关单元个数
			double nodeelenum = 0.;
			for (auto ii = nodeEle.begin();ii !=nodeEle.end();ii++)
			{
				nodeelenum = nodeelenum + 1;
				//划分几点的全局应力
				double Gstress11 = 0.; double Gstress22 = 0.; double Gstress33 = 0.;
				double Gstress12 = 0.; double Gstress13 = 0.; double Gstress23 = 0.;

				//每个单元的节点数
				int nodenum;
				//单元序号
				int elenumber = *ii;
				vector <double> H;
				int eletype = eledatas[elenumber].first;
				int eledim = eledatas[elenumber].second;
				//选择插值形函数
				if (eledim == 2)
				{
					if (eletype == 11)// 3节点三角形"
					{ 
						nodenum = 3; 
					}
					else if (eletype == 12) // 6节点三角形"
					{ 
						nodenum = 6;
					}
					else if (eletype == 10) // 8节点四角形"
					{ 
						nodenum = 8; 
					}
					else if(eletype == 9) // 4节点四角形"
					{ 
						nodenum = 4;
					}
				}
				if (eledim == 3)
				{

					if (eletype == 1) //一阶四面体
					{ 
						nodenum = 4; 
					}
					else if (eletype == 2) //二阶四面体
					{ 
						nodenum = 10; 
					}
					else if (eletype == 4) //一阶三菱柱
					{ 
						nodenum = 6; 
					}
					else if (eletype == 6) //二阶三棱柱
					{ 
						nodenum = 15; 
					}
					else if (eletype == 8) //二阶六面体
					{ 
						nodenum = 20; 
					}
					else if (eletype == 7) //一阶六面体
					{
						nodenum = 8; 
					}
				}
				/*if (eledim == 2)
				{
					if (eletype.find('3') != std::string::npos){shapefun_3q(H, Hr, Hs, r, s);}
					else if (eletype.find('6') != std::string::npos){shapefun_6q(H, Hr, Hs, r, s);}
					else if (eletype.find('8') != std::string::npos){shapefun_8q(H, Hr, Hs, r, s);}
					else(eletype.find('4') != std::string::npos);{shapefun_4q(H, Hr, Hs, r, s);}
				}
				if (eledim == 3)
				{
					if (eletype.find('4') != std::string::npos){shapefun_Tet4(H, Hr, Hs, Ht, r, s, t);}
					else if (eletype.find('10') != std::string::npos){shapefun_Tet10(H, Hr, Hs, Ht, r, s, t);}
					else if (eletype.find('6') != std::string::npos){shapefun_Penta6(H, Hr, Hs, Ht, r, s, t);}
					else if(eletype.find('15') != std::string::npos){shapefun_Penta15(H, Hr, Hs, Ht, r, s, t);}
					else if (eletype.find('20') != std::string::npos){shapefun_Hex20(H, Hr, Hs, Ht, r, s, t);}
					else  (eletype.find('8') != std::string::npos);{shapefun_Hex8(H, Hr, Hs, Ht, r, s, t);}
				}*/

				vector <double> xcoord; vector <double> ycoord; vector <double> zcoord;
				xcoord.resize(nodenum);
				ycoord.resize(nodenum);
				zcoord.resize(nodenum);
				/*double elestress[nodenum][6];*/
				map< int, std::vector< double >> elestress;
				for (int ik = 0; ik < nodenum; ++ik)
				{
					int nodeId = elenode[elenumber][ik];
					xcoord[ik] = elnodecoor[nodeId][0];
					ycoord[ik] = elnodecoor[nodeId][1];
					zcoord[ik] = elnodecoor[nodeId][2];
					std::vector<double> temp{};
					temp.resize(6);
					//获取节点对应的应力分量
					temp[0] = nodestress[nodeId][0];
					temp[1] = nodestress[nodeId][1];
					temp[2] = nodestress[nodeId][2];
					temp[3] = nodestress[nodeId][3];
					temp[4] = nodestress[nodeId][4];
					temp[5] = nodestress[nodeId][5];
					elestress.insert({ ik,temp });
				}
				if (eledim == 2)
				{
					Getparamcoord2D(nodenum, H, xcoord, ycoord, Targetpoint);
					for (int ij = 0; ij < nodenum; ++ij)
					{
						Gstress11 += H[ij] * elestress[ij][0];
						Gstress22 += H[ij] * elestress[ij][1];
						Gstress12 += H[ij] * elestress[ij][3];
					}
					locstress11[it] += Gstress11 * Comatrix2D[0][0] * Comatrix2D[0][0] + 2. * Gstress12 * Comatrix2D[0][0] * Comatrix2D[0][1] + Gstress22 * Comatrix2D[0][1] * Comatrix2D[0][1];
					locstress22[it] += Gstress11 * Comatrix2D[1][0] * Comatrix2D[1][0] + 2. * Gstress12 * Comatrix2D[1][0] * Comatrix2D[1][1] + Gstress22 * Comatrix2D[1][1] * Comatrix2D[1][1];
					locstress12[it] += Gstress11 * Comatrix2D[0][0] * Comatrix2D[1][0] + Gstress12 * Comatrix2D[0][0] * Comatrix2D[1][1] +
						Gstress12 * Comatrix2D[0][1] * Comatrix2D[1][0] + Gstress22 * Comatrix2D[0][1] * Comatrix2D[1][1];
					locstress33[it] = 0.;

				}
				else if(eledim == 3)
				{
					Getparamcoord3D(nodenum, H, xcoord, ycoord, zcoord, Targetpoint);
					for (int ij = 0; ij < nodenum; ++ij)
					{
						Gstress11 += H[ij] * elestress[ij][0];
						Gstress22 += H[ij] * elestress[ij][1];
						Gstress33 += H[ij] * elestress[ij][2];
						Gstress12 += H[ij] * elestress[ij][3];
						Gstress13 += H[ij] * elestress[ij][4];
						Gstress23 += H[ij] * elestress[ij][5];
					}
					locstress11[it] += Gstress11 * Comatrix2D[0][0] * Comatrix2D[0][0] + 2.* Gstress12 * Comatrix2D[0][0] * Comatrix2D[0][1] +
						              2. * Gstress13 * Comatrix2D[0][0] * Comatrix2D[0][2] + 2. * Gstress23 * Comatrix2D[0][1] * Comatrix2D[0][2] + 
						              Gstress22 * Comatrix2D[0][1] * Comatrix2D[0][1] + Gstress33 * Comatrix2D[0][2] * Comatrix2D[0][2];
					locstress22[it] += Gstress11 * Comatrix2D[0][1] * Comatrix2D[0][1] + 2. * Gstress12 * Comatrix2D[1][0] * Comatrix2D[1][1] +
						              2. * Gstress13 * Comatrix2D[1][0] * Comatrix2D[1][2] + 2. * Gstress23 * Comatrix2D[1][1] * Comatrix2D[1][2] +
						              Gstress22 * Comatrix2D[1][1] * Comatrix2D[1][1] + Gstress33 * Comatrix2D[1][2] * Comatrix2D[1][2]; 
					locstress33[it] += Gstress11 * Comatrix2D[2][0] * Comatrix2D[2][0] + 2. * Gstress12 * Comatrix2D[2][0] * Comatrix2D[2][1] +
						              2. * Gstress13 * Comatrix2D[2][0] * Comatrix2D[2][2] + 2. * Gstress23 * Comatrix2D[2][1] * Comatrix2D[2][2] +
						              Gstress22 * Comatrix2D[2][1] * Comatrix2D[2][1] + Gstress33 * Comatrix2D[2][2] * Comatrix2D[2][2];
					locstress12[it] += Gstress11 * Comatrix2D[0][0] * Comatrix2D[1][0] + Gstress12 * Comatrix2D[0][0] * Comatrix2D[1][1] +
						              Gstress13 * Comatrix2D[0][0] * Comatrix2D[1][2] + Gstress12 * Comatrix2D[0][1] * Comatrix2D[1][0] + 
						              Gstress22 * Comatrix2D[0][1] * Comatrix2D[1][1] + Gstress23 * Comatrix2D[0][1] * Comatrix2D[0][2] +
						              Gstress13 * Comatrix2D[0][2] * Comatrix2D[1][0] + Gstress23 * Comatrix2D[0][2] * Comatrix2D[1][1]+ Gstress33 * Comatrix2D[0][2] * Comatrix2D[1][2];
					locstress13[it] += Gstress11 * Comatrix2D[0][0] * Comatrix2D[2][0] + Gstress12 * Comatrix2D[0][0] * Comatrix2D[2][1] +
						              Gstress13 * Comatrix2D[0][0] * Comatrix2D[2][2] + Gstress12 * Comatrix2D[0][1] * Comatrix2D[2][0] +
						              Gstress22 * Comatrix2D[0][1] * Comatrix2D[2][1] + Gstress23 * Comatrix2D[0][1] * Comatrix2D[2][2] +
						              Gstress13 * Comatrix2D[0][2] * Comatrix2D[2][0] + Gstress23 * Comatrix2D[0][2] * Comatrix2D[2][1] + Gstress33 * Comatrix2D[0][2] * Comatrix2D[2][2];
					locstress23[it] += Gstress11 * Comatrix2D[1][0] * Comatrix2D[2][0] + Gstress12 * Comatrix2D[1][0] * Comatrix2D[3][1] +
						              Gstress13 * Comatrix2D[1][0] * Comatrix2D[2][2] + Gstress12 * Comatrix2D[1][1] * Comatrix2D[2][0] +
						              Gstress22 * Comatrix2D[1][1] * Comatrix2D[2][1] + Gstress23 * Comatrix2D[1][1] * Comatrix2D[2][2] +
						              Gstress13 * Comatrix2D[1][2] * Comatrix2D[2][0] + Gstress23 * Comatrix2D[1][2] * Comatrix2D[2][1] + Gstress33 * Comatrix2D[1][2] * Comatrix2D[2][2];

				}

			}

			//平均化
			locstress11[it] = locstress11[it] / nodeelenum;
			locstress22[it] = locstress22[it] / nodeelenum;
			locstress33[it] = locstress33[it] / nodeelenum;
			locstress12[it] = locstress12[it] / nodeelenum;
			locstress13[it] = locstress13[it] / nodeelenum;
			locstress23[it] = locstress23[it] / nodeelenum;

			/*Getstresslinear2D(num, linearstress,  locstress11,locstress22, locstress33, locstress12);*/
		}
		Getstresslinear3D(num, linearstress, locstress11, locstress22, locstress33, locstress12, locstress13, locstress23);
	}



	void mPostDealData::shapefun_3q(vector <double>& H, vector <double>& Hr, vector <double>& Hs, double r, double s)
	{
		H.resize(3);
		H[0] = 1.0 - r - s;
		H[1] = r;
		H[2] = s;

		Hr.resize(3);
		//形函数的局部倒-xi
		Hr[0] = -1.0;
		Hr[1] = 1.0;
		Hr[2] = 0.0;

		Hs.resize(3);
		//形函数的局部倒-et
		Hs[0] = -1.0;
		Hs[1] = 0.0;
		Hs[2] = 1.0;

	}

	void mPostDealData::shapefun_6q(vector <double>& H, vector <double>& Hr, vector <double>& Hs, double r, double s)
	{
		double c1;
		c1 = 1.0 - r - s;

		//形函数值
		H[0] = 2.0 * (0.5 - r - s) * c1;
		H[1] = r * (2.0 * r - 1.0);
		H[2] = s * (2.0 * s - 1.0);
		H[3] = 4.0 * r * c1;
		H[4] = 4.0 * r * s;
		H[5] = 4.0 * s * c1;

		//形函数的局部倒-r
		Hr[0] = 4.0 * (r + s) - 3.0;
		Hr[1] = 4.0 * r - 1.0;
		Hr[2] = 0.0;
		Hr[3] = 4.0 * (c1 - r);
		Hr[4] = 4.0 * s;
		Hr[5] = -4.0 * s;

		//形函数的局部倒-s
		Hs[0] = 4.0 * (r + s) - 3.0;
		Hs[1] = 0.0;
		Hs[2] = 4.0 * s - 1.0;
		Hs[3] = -4.0 * r;
		Hs[4] = 4.0 * r;
		Hs[5] = 4.0 * (c1 - s);

	}

	void mPostDealData::shapefun_4q(vector <double>& H, vector <double>& Hr, vector <double>& Hs, double r, double s)
	{
		double pxi, mxi;
		double pet, met;

		double she[3];
		double xsi[2][3];

		pxi = 1.0 + r;
		mxi = 1.0 - r;

		pet = 1.0 + s;
		met = 1.0 - s;

		H.resize(4);
		//形函数值
		H[0] = mxi * met / 4.0;
		H[1] = pxi * met / 4.0;
		H[2] = pxi * pet / 4.0;
		H[3] = mxi * pet / 4.0;

		Hr.resize(4);
		//形函数的局部倒-xi
		Hr[0] = -met / 4.0;
		Hr[1] = met / 4.0;
		Hr[2] = pet / 4.0;
		Hr[3] = -pet / 4.0;

		Hs.resize(4);
		//形函数的局部倒-
		Hs[0] = -mxi / 4.0;
		Hs[1] = -pxi / 4.0;
		Hs[2] = pxi / 4.0;
		Hs[3] = mxi / 4.0;
	}

	//8节点四边形形函数和形函数导数
	void mPostDealData::shapefun_8q(vector <double>& H, vector <double>& Hr, vector <double>& Hs, double r, double s)
	{
		double pxi = 0.0, mxi = 0.0, mxi2 = 0.0;
		double pet = 0.0, met = 0.0, met2 = 0.0;
		double xipet = 0.0, ximet = 0.0;

		pxi = 1.0 + r;
		mxi = 1.0 - r;
		mxi2 = pxi * mxi;

		pet = 1.0 + s;
		met = 1.0 - s;
		met2 = pet * met;

		xipet = r + s;
		ximet = r - s;

		//形函数值
		H[0] = mxi * met * (-xipet - 1.0) / 4.0;
		H[1] = pxi * met * (ximet - 1.0) / 4.0;
		H[2] = pxi * pet * (xipet - 1.0) / 4.0;
		H[3] = mxi * pet * (-ximet - 1.0) / 4.0;
		H[4] = mxi2 * met / 2.0;
		H[5] = pxi * met2 / 2.0;
		H[6] = mxi2 * pet / 2.0;
		H[7] = mxi * met2 / 2.0;


		//形函数的局部倒-r
		Hr[0] = met * (r + xipet) / 4.0;
		Hr[1] = met * (r + ximet) / 4.0;
		Hr[2] = pet * (r + xipet) / 4.0;
		Hr[3] = pet * (r + ximet) / 4.0;
		Hr[4] = -r * met;
		Hr[5] = met2 / 2.0;
		Hr[6] = -r * pet;
		Hr[7] = -met2 / 2.0;
		//形函数的局部倒-s
		Hs[0] = mxi * (s + xipet) / 4.0;
		Hs[1] = pxi * (s - ximet) / 4.0;
		Hs[2] = pxi * (s + xipet) / 4.0;
		Hs[3] = mxi * (s - ximet) / 4.0;
		Hs[4] = -mxi2 / 2.0;
		Hs[5] = -s * pxi;
		Hs[6] = mxi2 / 2.0;
		Hs[7] = -s * mxi;
	}





	void mPostDealData::ComputeCosystem3D(double ininode[3], double endnode[3], double Comatrix[3][3])
	{
		//使用用户选择的点计算直线的局部坐标系
		double vecx[3], vecy[3], vecz[3];
		vecx[0] = endnode[0] - ininode[0];
		vecx[1] = endnode[1] - ininode[1];
		vecx[2] = endnode[2] - ininode[2];
		double lengx = sqrt(vecx[0] * vecx[0] + vecx[1] * vecx[1] + vecx[2] * vecx[2]);
		double II[3];
		II[0] = 0.; II[1] = 1.; II[2] = 0.;
		double flag = vecx[0] * II[0] + vecx[1] * II[1] + vecx[2] * II[2];
		flag = abs(flag / lengx);
		if (flag > cos(0.1))
		{
			II[0] = 1.; II[1] = 0.; II[2] = 0.;
		}

		vecz[0] = (vecx[1] * II[2] - vecx[2] * II[1]);
		vecz[1] = (vecx[2] * II[0] - vecx[0] * II[2]);
		vecz[2] = (vecx[0] * II[1] - vecx[1] * II[0]);
		double lengz = sqrt(vecz[0] * vecz[0] + vecz[1] * vecz[1] + vecz[2] * vecz[2]);

		vecy[0] = (vecz[1] * vecx[2] - vecz[2] * vecx[1]);
		vecy[1] = (vecz[2] * vecx[0] - vecz[0] * vecx[2]);
		vecy[2] = (vecz[0] * vecx[1] - vecz[1] * vecx[0]);
		double lengy = sqrt(vecy[0] * vecy[0] + vecy[1] * vecy[1] + vecy[2] * vecy[2]);

		
		Comatrix[0][0] = vecx[0] / lengx; Comatrix[0][1] = vecx[1] / lengx; Comatrix[0][2] = vecx[2] / lengx;
		Comatrix[1][0] = vecy[0] / lengy; Comatrix[1][1] = vecy[1] / lengy; Comatrix[1][2] = vecy[2] / lengy;
		Comatrix[2][0] = vecz[0] / lengz; Comatrix[2][1] = vecz[1] / lengz; Comatrix[2][1] = vecz[2] / lengz;
	}

	void mPostDealData::Getstresslinear3D(const int num, std::vector<double>& lstress, vector <double>& stress11, vector <double>& stress22, vector <double>& stress33, vector <double>& stress12,
		                  vector <double>& stress13, vector <double>& stress23)
	{
		vector <double> Mises;
		Mises.resize(num);
		vector <double> Tresca;
		Tresca.resize(num);
		//计算旋转后的Mise应力和Tresca应力
		for (int ii = 0; ii < num; ++ii)
		{
			double lamada1 = (stress11[ii] - stress22[ii]) * (stress11[ii] - stress22[ii]) + (stress22[ii] - stress33[ii]) * (stress22[ii] - stress33[ii]) + (stress33[ii] - stress11[ii]) * (stress33[ii] - stress11[ii]);
			double lamada2 = 6 * (stress12[ii] * stress12[ii] + stress13[ii] * stress13[ii] + stress23[ii] * stress23[ii]);
			Mises[ii] = sqrt((0.5 * (lamada1 +lamada2)));
			double Stress[3][3], V[3][3] = { 0. },sigama[3] = {0.};
			Stress[0][0] = stress11[ii]; Stress[1][1] = stress22[ii]; Stress[2][2] = stress33[ii];
			Stress[0][1] = stress12[ii]; Stress[1][0] = stress12[ii]; Stress[2][0] = stress13[ii];
			Stress[0][2] = stress13[ii]; Stress[1][2] = stress23[ii]; Stress[2][1] = stress23[ii];
			eigen_decomposition(Stress, V, sigama);
			//double A, B, C;
			//A = max(sigama[0], sigama[1]);
			//A = max(A, sigama[2]);
			//C = min(sigama[0], sigama[1]);
			//C = min(C, sigama[2]);
			//for (int jj = 0 ;jj < 3; ++jj)
			//{
			//	if((sigama[jj]-A<-1e-20) && (sigama[jj]-B>1e-20))
			//	{
			//		B = sigama[jj];
			//		break;
			//	}
			//}

			Tresca[ii] = max(abs(sigama[0] - sigama[1]), abs(sigama[0] - sigama[2]));
			Tresca[ii] = max(Tresca[ii], abs(sigama[1] - sigama[2]));
		}



		//计算膜应变
		double Memstress11 = 0.5 * (stress11[0] + stress11[num-1]);
		double Memstress22 = 0.5 * (stress22[0] + stress22[num-1]);
		double Memstress33 = 0.5 * (stress33[0] + stress33[num-1]);
		double Memstress12 = 0.5 * (stress12[0] + stress12[num-1]);
		double Memstress13 = 0.5 * (stress13[0] + stress13[num-1]);
		double Memstress23 = 0.5 * (stress23[0] + stress23[num-1]);
		double Memmises = 0.5 * (Mises[0] + Mises[num-1]);
		double Memtresca = 0.5 * (Tresca[0] + Tresca[num-1]);

		for (int ii = 1; ii < num -1; ++ii)
		{
			Memstress11 += stress11[ii];
			Memstress22 += stress22[ii];
			Memstress33 += stress33[ii];
			Memstress12 += stress12[ii];
			Memstress13 += stress13[ii];
			Memstress23 += stress23[ii];
			Memmises += Mises[ii];
			Memtresca += Tresca[ii];

		}
		Memstress11 = Memstress11 / (num - 1);
		Memstress22 = Memstress22 / (num - 1);
		Memstress33 = Memstress33 / (num - 1);
		Memstress12 = Memstress12 / (num - 1);
		Memstress13 = Memstress13 / (num - 1);
		Memstress23 = Memstress23 / (num - 1);
		Memmises = Memmises / (num - 1);
		Memtresca = Memtresca / (num - 1);


		//计算弯曲应变
		//弯曲部分的数据准备
		vector <double> midstress11;
		vector <double> midstress22;
		vector <double> midstress33;
		vector <double> midstress12;
		vector <double> midstress13;
		vector <double> midstress23;
		vector <double> midmises;
		vector <double> midtresca;
		midstress11.resize(num-1);
		midstress22.resize(num-1);
		midstress33.resize(num-1);
		midstress12.resize(num-1);
		midstress13.resize(num-1);
		midstress23.resize(num-1);
		midmises.resize(num-1);
		midtresca.resize(num-1);
		double Mforce = (num -1) / 2.;
		for (int ii = 0; ii < num-1;++ii)
		{
			int jj = ii + 1;
			midstress11[ii] = (stress11[ii] * (Mforce - ii)+ stress11[jj] * (Mforce - jj)) ;
			midstress22[ii] = (stress22[ii] * (Mforce - ii)+ stress22[jj] * (Mforce - jj)) ;
			midstress33[ii] = (stress33[ii] * (Mforce - ii)+ stress33[jj] * (Mforce - jj)) ;
			midstress12[ii] = (stress12[ii] * (Mforce - ii)+ stress12[jj] * (Mforce - jj)) ;
			midstress13[ii] = (stress13[ii] * (Mforce - ii)+ stress13[jj] * (Mforce - jj)) ;
			midstress23[ii] = (stress23[ii] * (Mforce - ii)+ stress23[jj] * (Mforce - jj)) ;
			midmises[ii] = (Mises[ii] * (Mforce - ii) + Mises[jj] * (Mforce - jj)) ;
			midtresca[ii] = (Tresca[ii] * (Mforce - ii) + Tresca[jj] * (Mforce - jj));
		}

		//计算弯曲应力
		double Bendstress11A = 0., Bendstress22A = 0., Bendstress33A = 0., Bendstress12A = 0., Bendstress13A = 0., Bendstress23A = 0.;
		double Bendstress11B = 0., Bendstress22B = 0., Bendstress33B = 0., Bendstress12B = 0., Bendstress13B = 0., Bendstress23B = 0.;
		double BendmisesA = 0., BendmisesB = 0.;
		double BendtrescaA = 0., BendtrescaB = 0.;

	
		for (int ii = 0; ii < num - 1; ++ii)
		{
			Bendstress11A += midstress11[ii] ;
			Bendstress22A += midstress22[ii] ;
			Bendstress33A += midstress33[ii] ;
			Bendstress12A += midstress12[ii] ;
			Bendstress13A += midstress13[ii] ;
			Bendstress23A += midstress23[ii] ;
			BendmisesA += midmises[ii] ;
			BendtrescaA += midtresca[ii] ;

		}
		//计算用户选择第一个点的弯曲部分的线性应力
		Bendstress11A = Bendstress11A * 3. / ((num-1) * (num - 1));
		Bendstress22A = Bendstress22A * 3 / ((num-1) * (num - 1));
		Bendstress33A = Bendstress33A * 3 / ((num-1) * (num - 1));
		Bendstress12A = Bendstress12A * 3 / ((num-1) * (num - 1));
		Bendstress13A = Bendstress13A * 3 / ((num-1) * (num - 1));
		Bendstress23A = Bendstress23A * 3 / ((num-1) * (num - 1));
		BendmisesA = BendmisesA * 3. / ((num - 1) * (num - 1));
		BendtrescaA = BendtrescaA * 3. / ((num - 1) * (num - 1));

		//计算用户选择第二个点的弯曲部分的线性应力
		Bendstress11B = -Bendstress11A;
		Bendstress22B = -Bendstress22A;
		Bendstress33B = -Bendstress33A;
		Bendstress12B = -Bendstress12A;
		Bendstress13B = -Bendstress13A;
		Bendstress23B = -Bendstress23A;
		BendmisesB = -BendmisesA;
		BendtrescaB = -BendtrescaA;

		//计算第一个点膜部分加弯曲部分的应力应力
		double MemplusBend11_A = Memstress11 + Bendstress11A;
		double MemplusBend22_A = Memstress22 + Bendstress22A;
		double MemplusBend33_A = Memstress33 + Bendstress33A;
		double MemplusBend12_A = Memstress12 + Bendstress12A;
		double MemplusBend13_A = Memstress13 + Bendstress13A;
		double MemplusBend23_A = Memstress23 + Bendstress23A;
		double MemplusBendmise_A = Memmises + BendmisesA;
		double MemplusBendtresca_A = Memtresca + BendtrescaA;


		//计算第二个点膜部分加弯曲部分的线性应力
		double MemplusBend11_B = Memstress11 + Bendstress11B;
		double MemplusBend22_B = Memstress22 + Bendstress22B;
		double MemplusBend33_B = Memstress33 + Bendstress33B;
		double MemplusBend12_B = Memstress12 + Bendstress12B;
		double MemplusBend13_B = Memstress13 + Bendstress13B;
		double MemplusBend23_B = Memstress23 + Bendstress23B;
		double MemplusBendmise_B = Memmises + BendmisesB;
		double MemplusBendtresca_B = Memtresca + BendtrescaB;
		//计算第一个点峰顶应力
		double Peakstress11A, Peakstress22A, Peakstress33A, Peakstress12A, Peakstress13A, Peakstress23A;
		double PeakmisesA, PeaktrescaA;
		Peakstress11A = stress11[0] - MemplusBend11_A;
		Peakstress22A = stress22[0] - MemplusBend22_A;
		Peakstress33A = stress33[0] - MemplusBend33_A;
		Peakstress12A = stress12[0] - MemplusBend12_A;
		Peakstress13A = stress13[0] - MemplusBend13_A;
		Peakstress23A = stress23[0] - MemplusBend23_A;
		PeakmisesA = Mises[0] - MemplusBendmise_A;
		PeaktrescaA = Tresca[0] - MemplusBendtresca_A;
		//计算第二个点峰顶应力
		double Peakstress11B, Peakstress22B, Peakstress33B, Peakstress12B, Peakstress13B, Peakstress23B;
		double PeakmisesB, PeaktrescaB;
		Peakstress11B = stress11[num - 1] - MemplusBend11_B;
		Peakstress22B = stress22[num - 1] - MemplusBend22_B;
		Peakstress33B = stress33[num - 1] - MemplusBend33_B;
		Peakstress12B = stress12[num - 1] - MemplusBend12_B;
		Peakstress13B = stress13[num - 1] - MemplusBend13_B;
		Peakstress23B = stress23[num - 1] - MemplusBend23_B;
		PeakmisesB = Mises[num - 1] - MemplusBendmise_B;
		PeaktrescaB = Tresca[num - 1] - MemplusBendtresca_B;

		lstress.reserve(56);
		//目前暂时没有对不变量进行线性化
		lstress.push_back(Memstress11); lstress.push_back(Memstress22); lstress.push_back(Memstress33);lstress.push_back(Memstress13);
		lstress.push_back(Memstress23); lstress.push_back(Memstress12); lstress.push_back(Memmises); lstress.push_back(Memtresca);

		lstress.push_back(Bendstress11A); lstress.push_back(Bendstress22A); lstress.push_back(Bendstress33A);lstress.push_back(Bendstress13A);
		lstress.push_back(Bendstress23A); lstress.push_back(Bendstress12A); lstress.push_back(BendmisesA); lstress.push_back(BendtrescaA);

		lstress.push_back(MemplusBend11_A); lstress.push_back(MemplusBend22_A); lstress.push_back(MemplusBend33_A);lstress.push_back(MemplusBend13_A);
		lstress.push_back(MemplusBend23_A); lstress.push_back(MemplusBend12_A); lstress.push_back(MemplusBendmise_A); lstress.push_back(MemplusBendtresca_A);

		lstress.push_back(Bendstress11B); lstress.push_back(Bendstress22B); lstress.push_back(Bendstress33B);lstress.push_back(Bendstress13B);
		lstress.push_back(Bendstress23B); lstress.push_back(Bendstress12B); lstress.push_back(BendmisesB); lstress.push_back(BendtrescaB);

		lstress.push_back(MemplusBend11_B); lstress.push_back(MemplusBend22_B); lstress.push_back(MemplusBend33_B);lstress.push_back(MemplusBend13_B);
		lstress.push_back(MemplusBend23_B); lstress.push_back(MemplusBend12_B); lstress.push_back(MemplusBendmise_B); lstress.push_back(MemplusBendtresca_B);

		lstress.push_back(Peakstress11A); lstress.push_back(Peakstress22A); lstress.push_back(Peakstress33A);lstress.push_back(Peakstress13A);
		lstress.push_back(Peakstress23A); lstress.push_back(Peakstress12A); lstress.push_back(PeakmisesA); lstress.push_back(PeaktrescaA);

		lstress.push_back(Peakstress11B); lstress.push_back(Peakstress22B); lstress.push_back(Peakstress33B);lstress.push_back(Peakstress13B);
		lstress.push_back(Peakstress23B); lstress.push_back(Peakstress12B); lstress.push_back(PeakmisesB); lstress.push_back(PeaktrescaB);

	}


	void mPostDealData::shapefun_Tet4(vector <double>& H, vector <double>& Hr, vector <double>& Hs, vector <double>& Ht, double r, double s, double t)
	{
		H.resize(4);
		H[0] = 1 - r - s - t;
		H[1] = r;
		H[2] = s;
		H[3] = t;

		Hr.resize(4);
		Hr[0] = -1; Hs[0] = -1; Ht[0] = -1;
		Hr[1] = 1;	Hs[1] = 0; Ht[1] = 0;
		Hr[2] = 0;	Hs[2] = 1; Ht[2] = 0;
		Hr[3] = 0;	Hs[3] = 0; Ht[3] = 1;
	}

	void mPostDealData::shapefun_Tet10(vector <double>& H, vector <double>& Hr, vector <double>& Hs, vector <double>& Ht, double r, double s, double t)
	{
		double r1 = 1.0 - r - s - t;
		double r2 = r;
		double r3 = s;
		double r4 = t;

		H[0] = r1 * (2.0 * r1 - 1.0);
		H[1] = r2 * (2.0 * r2 - 1.0);
		H[2] = r3 * (2.0 * r3 - 1.0);
		H[3] = r4 * (2.0 * r4 - 1.0);
		H[4] = 4.0 * r1 * r2;
		H[5] = 4.0 * r2 * r3;
		H[6] = 4.0 * r3 * r1;
		H[7] = 4.0 * r1 * r4;
		H[8] = 4.0 * r2 * r4;
		H[9] = 4.0 * r3 * r4;

		Hr[0] = -3.0 + 4.0 * r + 4.0 * (s + t);
		Hr[1] = 4.0 * r - 1.0;
		Hr[2] = 0.0;
		Hr[3] = 0.0;
		Hr[4] = 4.0 - 8.0 * r - 4.0 * (s + t);
		Hr[5] = 4.0 * s;
		Hr[6] = -4.0 * s;
		Hr[7] = -4.0 * t;
		Hr[8] = 4.0 * t;
		Hr[9] = 0.0;

		Hs[0] = -3.0 + 4.0 * s + 4.0 * (r + t);
		Hs[1] = 0.0;
		Hs[2] = 4.0 * s - 1.0;
		Hs[3] = 0.0;
		Hs[4] = -4.0 * r;
		Hs[5] = 4.0 * r;
		Hs[6] = 4.0 - 8.0 * s - 4.0 * (r + t);
		Hs[7] = -4.0 * t;
		Hs[8] = 0.0;
		Hs[9] = 4.0 * t;

		Ht[0] = -3.0 + 4.0 * t + 4.0 * (r + s);
		Ht[1] = 0.0;
		Ht[2] = 0.0;
		Ht[3] = 4.0 * t - 1.0;
		Ht[4] = -4.0 * r;
		Ht[5] = 0.0;
		Ht[6] = -4.0 * s;
		Ht[7] = 4.0 - 8.0 * t - 4.0 * (r + s);
		Ht[8] = 4.0 * r;
		Ht[9] = 4.0 * s;
	}

	void mPostDealData::shapefun_Penta6(vector <double>& H, vector <double>& Hr, vector <double>& Hs, vector <double>& Ht, double r, double s, double t)
	{
		H[0] = 0.5 * (1 - t) * (1 - r - s);
		H[1] = 0.5 * (1 - t) * r;
		H[2] = 0.5 * (1 - t) * s;
		H[3] = 0.5 * (1 + t) * (1 - r - s);
		H[4] = 0.5 * (1 + t) * r;
		H[5] = 0.5 * (1 + t) * s;

		Hr[0] = -0.5 * (1 - t);
		Hr[1] = 0.5 * (1 - t);
		Hr[2] = 0.0;
		Hr[3] = -0.5 * (1 + t);
		Hr[4] = 0.5 * (1 + t);
		Hr[5] = 0.0;

		Hs[0] = -0.5 * (1 - t);
		Hs[1] = 0.0;
		Hs[2] = 0.5 * (1 - t);
		Hs[3] = -0.5 * (1 + t);
		Hs[4] = 0.0;
		Hs[5] = 0.5 * (1 + t);

		Ht[0] = -0.5 * (1 - r - s);
		Ht[1] = -0.5 * r;
		Ht[2] = -0.5 * s;
		Ht[3] = 0.5 * (1 - r - s);
		Ht[4] = 0.5 * r;
		Ht[5] = 0.5 * s;
	}

	void mPostDealData::shapefun_Penta15(vector <double>& H, vector <double>& Hr, vector <double>& Hs, vector <double>& Ht, double r, double s, double t)
	{
		double a = 1 - r - s;
		H[0] = -0.5 * a * (1. - t) * (2. * r + 2. * s + t);
		H[1] = 0.5 * r * (1. - t) * (2. * r - 2. - t);
		H[2] = 0.5 * s * (1. - t) * (2. * s - 2. - t);
		H[3] = -0.5 * a * (1. + t) * (2. * r + 2. * s - t);
		H[4] = 0.5 * r * (1. + t) * (2. * r - 2. + t);
		H[5] = 0.5 * s * (1. + t) * (2. * s - 2. + t);
		H[6] = 2. * r * a * (1. - t);
		H[7] = 2. * r * s * (1. - t);
		H[8] = 2. * s * a * (1. - t);
		H[9] = 2. * r * a * (1. + t);
		H[10] = 2. * r * s * (1. + t);
		H[11] = 2. * s * a * (1. + t);
		H[12] = a * (1. - t * t);
		H[13] = r * (1. - t * t);
		H[14] = s * (1. - t * t);


		Hr[0] = 0.5 * (1. - t) * (4.0 * r + 4.0 * s + t - 2.);
		Hr[1] = 0.5 * (1. - t) * (4.0 * r - t - 2.);
		Hr[2] = 0.;
		Hr[3] = 0.5 * (1. + t) * (4.0 * r + 4.0 * s - t - 2.);
		Hr[4] = 0.5 * (1. + t) * (4.0 * r + t - 2.);
		Hr[5] = 0.;
		Hr[6] = 2. * (1. - t) * (1. - 2. * r - s);
		Hr[7] = 2. * s * (1. - t);
		Hr[8] = -2. * s * (1. - t);
		Hr[9] = 2. * (1. + t) * (1. - 2. * r - s);
		Hr[10] = 2. * s * (1. + t);
		Hr[11] = -2. * s * (1. + t);
		Hr[12] = -(1. - t * t);
		Hr[13] = (1. - t * t);
		Hr[14] = 0.;

		Hs[0] = 0.5 * (1. - t) * (4.0 * r + 4.0 * s + t - 2.);
		Hs[1] = 0.;
		Hs[2] = 0.5 * (1. - t) * (4.0 * s - t - 2.);
		Hs[3] = 0.5 * (1. + t) * (4.0 * r + 4.0 * s - t - 2.);
		Hs[4] = 0.;
		Hs[5] = 0.5 * (1. + t) * (4.0 * s + t - 2.);
		Hs[6] = -2. * r * (1. - t);
		Hs[7] = 2. * r * (1. - t);
		Hs[8] = 2. * (1. - t) * (1. - r - 2. * s);
		Hs[9] = -2. * r * (1. + t);
		Hs[10] = 2. * r * (1. + t);
		Hs[11] = 2. * (1. + t) * (1. - r - 2. * s);
		Hs[12] = -(1. - t * t);
		Hs[13] = 0.0;
		Hs[14] = (1. - t * t);

		Ht[0] = a * (r + s + t - 0.5);
		Ht[1] = r * (-r + t + 0.5);
		Ht[2] = s * (-s + t + 0.5);
		Ht[3] = a * (-r - s + t + 0.5);
		Ht[4] = r * (r + t - 0.5);
		Ht[5] = s * (s + t - 0.5);
		Ht[6] = -2 * r * a;
		Ht[7] = -2 * r * s;
		Ht[8] = -2 * s * a;
		Ht[9] = 2 * r * a;
		Ht[10] = 2 * r * s;
		Ht[11] = 2 * s * a;
		Ht[12] = -2 * a * t;
		Ht[13] = -2 * r * t;
		Ht[14] = -2 * s * t;
	}

	void mPostDealData::shapefun_Hex8(vector <double>& H, vector <double>& Hr, vector <double>& Hs, vector <double>& Ht, double r, double s, double t)
	{
		H[0] = 0.125 * (1 - r) * (1 - s) * (1 - t);
		H[1] = 0.125 * (1 + r) * (1 - s) * (1 - t);
		H[2] = 0.125 * (1 + r) * (1 + s) * (1 - t);
		H[3] = 0.125 * (1 - r) * (1 + s) * (1 - t);
		H[4] = 0.125 * (1 - r) * (1 - s) * (1 + t);
		H[5] = 0.125 * (1 + r) * (1 - s) * (1 + t);
		H[6] = 0.125 * (1 + r) * (1 + s) * (1 + t);
		H[7] = 0.125 * (1 - r) * (1 + s) * (1 + t);

		Hr[0] = -0.125 * (1 - s) * (1 - t);
		Hr[1] = 0.125 * (1 - s) * (1 - t);
		Hr[2] = 0.125 * (1 + s) * (1 - t);
		Hr[3] = -0.125 * (1 + s) * (1 - t);
		Hr[4] = -0.125 * (1 - s) * (1 + t);
		Hr[5] = 0.125 * (1 - s) * (1 + t);
		Hr[6] = 0.125 * (1 + s) * (1 + t);
		Hr[7] = -0.125 * (1 + s) * (1 + t);

		Hs[0] = -0.125 * (1 - r) * (1 - t);
		Hs[1] = -0.125 * (1 + r) * (1 - t);
		Hs[2] = 0.125 * (1 + r) * (1 - t);
		Hs[3] = 0.125 * (1 - r) * (1 - t);
		Hs[4] = -0.125 * (1 - r) * (1 + t);
		Hs[5] = -0.125 * (1 + r) * (1 + t);
		Hs[6] = 0.125 * (1 + r) * (1 + t);
		Hs[7] = 0.125 * (1 - r) * (1 + t);

		Ht[0] = -0.125 * (1 - r) * (1 - s);
		Ht[1] = -0.125 * (1 + r) * (1 - s);
		Ht[2] = -0.125 * (1 + r) * (1 + s);
		Ht[3] = -0.125 * (1 - r) * (1 + s);
		Ht[4] = 0.125 * (1 - r) * (1 - s);
		Ht[5] = 0.125 * (1 + r) * (1 - s);
		Ht[6] = 0.125 * (1 + r) * (1 + s);
		Ht[7] = 0.125 * (1 - r) * (1 + s);
	}

	void mPostDealData::shapefun_Hex20(vector <double>& H, vector <double>& Hr, vector <double>& Hs, vector <double>& Ht, double r, double s, double t)
	{
		H[8] = 0.25 * (1 - r * r) * (1 - s) * (1 - t);
		H[9] = 0.25 * (1 - s * s) * (1 + r) * (1 - t);
		H[10] = 0.25 * (1 - r * r) * (1 + s) * (1 - t);
		H[11] = 0.25 * (1 - s * s) * (1 - r) * (1 - t);
		H[12] = 0.25 * (1 - r * r) * (1 - s) * (1 + t);
		H[13] = 0.25 * (1 - s * s) * (1 + r) * (1 + t);
		H[14] = 0.25 * (1 - r * r) * (1 + s) * (1 + t);
		H[15] = 0.25 * (1 - s * s) * (1 - r) * (1 + t);
		H[16] = 0.25 * (1 - t * t) * (1 - r) * (1 - s);
		H[17] = 0.25 * (1 - t * t) * (1 + r) * (1 - s);
		H[18] = 0.25 * (1 - t * t) * (1 + r) * (1 + s);
		H[19] = 0.25 * (1 - t * t) * (1 - r) * (1 + s);

		H[0] = 0.125 * (1 - r) * (1 - s) * (1 - t) - 0.5 * (H[8] + H[11] + H[16]);
		H[1] = 0.125 * (1 + r) * (1 - s) * (1 - t) - 0.5 * (H[8] + H[9] + H[17]);
		H[2] = 0.125 * (1 + r) * (1 + s) * (1 - t) - 0.5 * (H[9] + H[10] + H[18]);
		H[3] = 0.125 * (1 - r) * (1 + s) * (1 - t) - 0.5 * (H[10] + H[11] + H[19]);
		H[4] = 0.125 * (1 - r) * (1 - s) * (1 + t) - 0.5 * (H[12] + H[15] + H[16]);
		H[5] = 0.125 * (1 + r) * (1 - s) * (1 + t) - 0.5 * (H[12] + H[13] + H[17]);
		H[6] = 0.125 * (1 + r) * (1 + s) * (1 + t) - 0.5 * (H[13] + H[14] + H[18]);
		H[7] = 0.125 * (1 - r) * (1 + s) * (1 + t) - 0.5 * (H[14] + H[15] + H[19]);

		Hr[8] = -0.5 * r * (1 - s) * (1 - t);
		Hr[9] = 0.25 * (1 - s * s) * (1 - t);
		Hr[10] = -0.5 * r * (1 + s) * (1 - t);
		Hr[11] = -0.25 * (1 - s * s) * (1 - t);
		Hr[12] = -0.5 * r * (1 - s) * (1 + t);
		Hr[13] = 0.25 * (1 - s * s) * (1 + t);
		Hr[14] = -0.5 * r * (1 + s) * (1 + t);
		Hr[15] = -0.25 * (1 - s * s) * (1 + t);
		Hr[16] = -0.25 * (1 - t * t) * (1 - s);
		Hr[17] = 0.25 * (1 - t * t) * (1 - s);
		Hr[18] = 0.25 * (1 - t * t) * (1 + s);
		Hr[19] = -0.25 * (1 - t * t) * (1 + s);

		Hr[0] = -0.125 * (1 - s) * (1 - t) - 0.5 * (Hr[8] + Hr[11] + Hr[16]);
		Hr[1] = 0.125 * (1 - s) * (1 - t) - 0.5 * (Hr[8] + Hr[9] + Hr[17]);
		Hr[2] = 0.125 * (1 + s) * (1 - t) - 0.5 * (Hr[9] + Hr[10] + Hr[18]);
		Hr[3] = -0.125 * (1 + s) * (1 - t) - 0.5 * (Hr[10] + Hr[11] + Hr[19]);
		Hr[4] = -0.125 * (1 - s) * (1 + t) - 0.5 * (Hr[12] + Hr[15] + Hr[16]);
		Hr[5] = 0.125 * (1 - s) * (1 + t) - 0.5 * (Hr[12] + Hr[13] + Hr[17]);
		Hr[6] = 0.125 * (1 + s) * (1 + t) - 0.5 * (Hr[13] + Hr[14] + Hr[18]);
		Hr[7] = -0.125 * (1 + s) * (1 + t) - 0.5 * (Hr[14] + Hr[15] + Hr[19]);

		Hs[8] = -0.25 * (1 - r * r) * (1 - t);
		Hs[9] = -0.5 * s * (1 + r) * (1 - t);
		Hs[10] = 0.25 * (1 - r * r) * (1 - t);
		Hs[11] = -0.5 * s * (1 - r) * (1 - t);
		Hs[12] = -0.25 * (1 - r * r) * (1 + t);
		Hs[13] = -0.5 * s * (1 + r) * (1 + t);
		Hs[14] = 0.25 * (1 - r * r) * (1 + t);
		Hs[15] = -0.5 * s * (1 - r) * (1 + t);
		Hs[16] = -0.25 * (1 - t * t) * (1 - r);
		Hs[17] = -0.25 * (1 - t * t) * (1 + r);
		Hs[18] = 0.25 * (1 - t * t) * (1 + r);
		Hs[19] = 0.25 * (1 - t * t) * (1 - r);

		Hs[0] = -0.125 * (1 - r) * (1 - t) - 0.5 * (Hs[8] + Hs[11] + Hs[16]);
		Hs[1] = -0.125 * (1 + r) * (1 - t) - 0.5 * (Hs[8] + Hs[9] + Hs[17]);
		Hs[2] = 0.125 * (1 + r) * (1 - t) - 0.5 * (Hs[9] + Hs[10] + Hs[18]);
		Hs[3] = 0.125 * (1 - r) * (1 - t) - 0.5 * (Hs[10] + Hs[11] + Hs[19]);
		Hs[4] = -0.125 * (1 - r) * (1 + t) - 0.5 * (Hs[12] + Hs[15] + Hs[16]);
		Hs[5] = -0.125 * (1 + r) * (1 + t) - 0.5 * (Hs[12] + Hs[13] + Hs[17]);
		Hs[6] = 0.125 * (1 + r) * (1 + t) - 0.5 * (Hs[13] + Hs[14] + Hs[18]);
		Hs[7] = 0.125 * (1 - r) * (1 + t) - 0.5 * (Hs[14] + Hs[15] + Hs[19]);

		Ht[8] = -0.25 * (1 - r * r) * (1 - s);
		Ht[9] = -0.25 * (1 - s * s) * (1 + r);
		Ht[10] = -0.25 * (1 - r * r) * (1 + s);
		Ht[11] = -0.25 * (1 - s * s) * (1 - r);
		Ht[12] = 0.25 * (1 - r * r) * (1 - s);
		Ht[13] = 0.25 * (1 - s * s) * (1 + r);
		Ht[14] = 0.25 * (1 - r * r) * (1 + s);
		Ht[15] = 0.25 * (1 - s * s) * (1 - r);
		Ht[16] = -0.5 * t * (1 - r) * (1 - s);
		Ht[17] = -0.5 * t * (1 + r) * (1 - s);
		Ht[18] = -0.5 * t * (1 + r) * (1 + s);
		Ht[19] = -0.5 * t * (1 - r) * (1 + s);

		Ht[0] = -0.125 * (1 - r) * (1 - s) - 0.5 * (Ht[8] + Ht[11] + Ht[16]);
		Ht[1] = -0.125 * (1 + r) * (1 - s) - 0.5 * (Ht[8] + Ht[9] + Ht[17]);
		Ht[2] = -0.125 * (1 + r) * (1 + s) - 0.5 * (Ht[9] + Ht[10] + Ht[18]);
		Ht[3] = -0.125 * (1 - r) * (1 + s) - 0.5 * (Ht[10] + Ht[11] + Ht[19]);
		Ht[4] = 0.125 * (1 - r) * (1 - s) - 0.5 * (Ht[12] + Ht[15] + Ht[16]);
		Ht[5] = 0.125 * (1 + r) * (1 - s) - 0.5 * (Ht[12] + Ht[13] + Ht[17]);
		Ht[6] = 0.125 * (1 + r) * (1 + s) - 0.5 * (Ht[13] + Ht[14] + Ht[18]);
		Ht[7] = 0.125 * (1 - r) * (1 + s) - 0.5 * (Ht[14] + Ht[15] + Ht[19]);
	}

	void mPostDealData::Getparamcoord2D(int nodenum, vector <double>& H, vector <double>& xcoord,vector <double>& ycoord, double Targetpoint[3])
	{
		double r = 0.; double s = 0.;
		//定义最大迭代步
		int max_iter = 10001;
		//满足的误差
		double tolerance = 1e-6;
		vector <double> Hr;
		vector <double> Hs;
		int iter;
		for (iter = 0; iter < max_iter; ++iter)
		{
			//选择插值函数
			if (nodenum == 3) { shapefun_3q(H, Hr, Hs, r, s); }
			else if (nodenum == 6) { shapefun_6q(H, Hr, Hs, r, s); }
			else if (nodenum == 8) { shapefun_8q(H, Hr, Hs, r, s); }
			else(nodenum == 4); {shapefun_4q(H, Hr, Hs, r, s); }

			double X_cal = 0., Y_cal = 0.;
			//计算当前参数坐标对应的物理坐标
			for (int ij = 0; ij < nodenum; ++ij)
			{
				X_cal += H[ij] * xcoord[ij];
				Y_cal += H[ij] * ycoord[ij];
			}
			double dX, dY;
			dX = Targetpoint[0] - X_cal;
			dY = Targetpoint[1] - Y_cal;

			double A = sqrt(dX *dX + dY * dY);
			if (sqrt(dX * dX + dY * dY) < tolerance)
			{
				if (nodenum == 3) { shapefun_3q(H, Hr, Hs, r, s); }
				else if (nodenum == 6) { shapefun_6q(H, Hr, Hs, r, s); }
				else if (nodenum == 8) { shapefun_8q(H, Hr, Hs, r, s); }
				else(nodenum == 4); {shapefun_4q(H, Hr, Hs, r, s); }
				break;
			}

			//计算雅克比矩阵
			double Jacobi[2][2] = { {0.} };
			for (int ij = 0; ij < nodenum; ++ij)
			{
				Jacobi[0][0] += Hr[ij] * xcoord[ij];
				Jacobi[0][1] += Hr[ij] * ycoord[ij];
				Jacobi[1][0] += Hs[ij] * xcoord[ij];
				Jacobi[1][1] += Hs[ij] * ycoord[ij];
			}
			double det = (Jacobi[0][0] * Jacobi[1][1]) - (Jacobi[0][1] * Jacobi[1][0]);
			if (det <= 0) { printf("单元负雅克比"); };
			double deti = 1. / det;
			double invJacobi[2][2];
			invJacobi[0][0] = Jacobi[1][1] * deti;
			invJacobi[0][1] = -Jacobi[0][1] * deti;
			invJacobi[1][0] = -Jacobi[1][0] * deti;
			invJacobi[1][1] = Jacobi[0][0] * deti;

			double deltaX = invJacobi[0][0] * dX + invJacobi[0][1] * dY;
			double deltaY = invJacobi[1][0] * dX + invJacobi[1][1] * dY;
			//if (abs(deltaX) > tolerance)
			//{
			//	r = r + deltaX;
			//}
			//else
			//{
			//	s = s + deltaY;
			//}
			r = r + deltaX;
			s = s + deltaY;

			


			//if (abs(deltaX) < 2){r = r + deltaX;}else{r = r;}
			//if (abs(deltaY) < 2) { s = s + deltaY; }else { s = s; }
		}
		if (iter > 10000)
		{
			qDebug() << tr("参数坐标未收敛，计算数值可能不准确");
		}
	}


	void mPostDealData::Getparamcoord3D(int nodenum, vector <double>& H, vector <double>& xcoord, vector <double>& ycoord, vector <double>& zcoord, double Targetpoint[3])
	{
		double r = 0.; double s = 0.; double t = 0.;
		//定义最大迭代步
		int max_iter = 10001;
		//阻尼因子
		double lambda = 0.001;
			//满足的误差
		double tolerance = 1e-6;
		vector <double> Hr;
		vector <double> Hs;
		vector <double> Ht;
		int iter;
		for (int iter = 0; iter < max_iter; ++iter)
		{
			//选择插值函数
			if (nodenum == 4) { shapefun_Tet4(H, Hr, Hs, Ht, r, s, t); }
			else if (nodenum == 10) { shapefun_Tet10(H, Hr, Hs, Ht, r, s, t); }
			else if (nodenum == 6) { shapefun_Penta6(H, Hr, Hs, Ht, r, s, t); }
			else if (nodenum == 15) { shapefun_Penta15(H, Hr, Hs, Ht, r, s, t); }
			else if (nodenum == 20) { shapefun_Hex20(H, Hr, Hs, Ht, r, s, t); }
			else  (nodenum == 8); {shapefun_Hex8(H, Hr, Hs, Ht, r, s, t); }

			double X_cal = 0., Y_cal = 0., Z_cal = 0.;
			//计算当前参数坐标对应的物理坐标
			for (int ij = 0; ij < nodenum; ++ij)
			{
				X_cal += H[ij] * xcoord[ij];
				Y_cal += H[ij] * ycoord[ij];
				Z_cal += H[ij] * zcoord[ij];
			}
			double dX, dY, dZ;
			dX = Targetpoint[0] - X_cal;
			dY = Targetpoint[1] - Y_cal;
			dZ = Targetpoint[2] - Z_cal;

			if (sqrt(dX * dX + dY * dY + dZ * dZ) < tolerance)
			{
				if (nodenum == 4) { shapefun_Tet4(H, Hr, Hs, Ht, r, s, t); }
				else if (nodenum == 10) { shapefun_Tet10(H, Hr, Hs, Ht, r, s, t); }
				else if (nodenum == 6) { shapefun_Penta6(H, Hr, Hs, Ht, r, s, t); }
				else if (nodenum == 15) { shapefun_Penta15(H, Hr, Hs, Ht, r, s, t); }
				else if (nodenum == 20) { shapefun_Hex20(H, Hr, Hs, Ht, r, s, t); }
				else  (nodenum == 8); {shapefun_Hex8(H, Hr, Hs, Ht, r, s, t); }
				break;
			}

			//计算雅克比矩阵
			double Jacobi[3][3] = { {0.} };
			for (int ij = 0; ij < nodenum; ++ij)
			{
				//Jacobi[0][0] += Hr[ij] * xcoord[ij];
				//Jacobi[0][1] += Hr[ij] * ycoord[ij];
				//Jacobi[0][2] += Hr[ij] * zcoord[ij];
				//Jacobi[1][0] += Hs[ij] * xcoord[ij];
				//Jacobi[1][1] += Hs[ij] * ycoord[ij];
				//Jacobi[1][2] += Hs[ij] * zcoord[ij];
				//Jacobi[2][0] += Ht[ij] * xcoord[ij];
				//Jacobi[2][1] += Ht[ij] * ycoord[ij];
				//Jacobi[2][2] += Ht[ij] * zcoord[ij];

				Jacobi[0][0] += Hr[ij] * xcoord[ij];
				Jacobi[0][1] += Hs[ij] * xcoord[ij];
				Jacobi[0][2] += Ht[ij] * xcoord[ij];
				Jacobi[1][0] += Hr[ij] * ycoord[ij];
				Jacobi[1][1] += Hs[ij] * ycoord[ij];
				Jacobi[1][2] += Ht[ij] * ycoord[ij];
				Jacobi[2][0] += Hr[ij] * zcoord[ij];
				Jacobi[2][1] += Hs[ij] * zcoord[ij];
				Jacobi[2][2] += Ht[ij] * zcoord[ij];


			}
			//计算梯度
			vector<double>grad;
			grad.assign(3, 0.0);
			grad[0] = Jacobi[0][0] * dX + Jacobi[1][0] * dY + Jacobi[2][0] * dZ;
			grad[1] = Jacobi[0][1] * dX + Jacobi[1][1] * dY + Jacobi[2][1] * dZ;
			grad[2] = Jacobi[0][2] * dX + Jacobi[1][2] * dY + Jacobi[2][2] * dZ;

			//计算Hessian近似H=J^T*J
			double H[3][3] = { {0,0,0},{0,0,0},{0,0,0} };
			for (int i = 0; i < 3; ++i)
			{
				for (int j = 0; j < 3; ++j)
				{
					H[i][j] = Jacobi[0][i] * Jacobi[0][j] + Jacobi[1][i] * Jacobi[1][j] + Jacobi[2][i] * Jacobi[2][j];
				}
			}
			// Levenberg - Marquardt修正: H + λ * diag(H)
			double diagH[3] = { H[0][0], H[1][1], H[2][2] };
			H[0][0] += lambda * diagH[0];
			H[1][1] += lambda * diagH[1];
			H[2][2] += lambda * diagH[2];

			// 解线性方程组 H * delta = grad (使用克莱默法则)
			double det = H[0][0] * (H[1][1] * H[2][2] - H[1][2] * H[2][1])
				- H[0][1] * (H[1][0] * H[2][2] - H[1][2] * H[2][0])
				+ H[0][2] * (H[1][0] * H[2][1] - H[1][1] * H[2][0]);

			if (std::fabs(det) < 1e-15) {
				return;  // 矩阵奇异
			}

			double invDet = 1.0 / det;
			double delta[3] = { 0, 0, 0 };

			// 计算delta[0]
			double mat0[3][3] = {
				  {grad[0], H[0][1], H[0][2]},
				  {grad[1], H[1][1], H[1][2]},
				  {grad[2], H[2][1], H[2][2]}
			};
			double det0 = mat0[0][0] * (mat0[1][1] * mat0[2][2] - mat0[1][2] * mat0[2][1])
				- mat0[0][1] * (mat0[1][0] * mat0[2][2] - mat0[1][2] * mat0[2][0])
				+ mat0[0][2] * (mat0[1][0] * mat0[2][1] - mat0[1][1] * mat0[2][0]);
			delta[0] = det0 * invDet;

			// 计算delta[1]
			double mat1[3][3] = {
				  {H[0][0], grad[0], H[0][2]},
				  {H[1][0], grad[1], H[1][2]},
				  {H[2][0], grad[2], H[2][2]}
			};
			double det1 = mat1[0][0] * (mat1[1][1] * mat1[2][2] - mat1[1][2] * mat1[2][1])
				- mat1[0][1] * (mat1[1][0] * mat1[2][2] - mat1[1][2] * mat1[2][0])
				+ mat1[0][2] * (mat1[1][0] * mat1[2][1] - mat1[1][1] * mat1[2][0]);
			delta[1] = det1 * invDet;
			// 计算delta[2]
			double mat2[3][3] = {
				  {H[0][0], H[0][1], grad[0]},
				  {H[1][0], H[1][1], grad[1]},
				  {H[2][0], H[2][1], grad[2]}
			};
			double det2 = mat2[0][0] * (mat2[1][1] * mat2[2][2] - mat2[1][2] * mat2[2][1])
				- mat2[0][1] * (mat2[1][0] * mat2[2][2] - mat2[1][2] * mat2[2][0])
				+ mat2[0][2] * (mat2[1][0] * mat2[2][1] - mat2[1][1] * mat2[2][0]);
			delta[2] = det2 * invDet;

			// 更新参数坐标
			double r_new = r + delta[0];
			double s_new = s + delta[1];
			double t_new = t + delta[2];

			// 更新阻尼因子
			if (sqrt(dX * dX + dY * dY + dZ * dZ) < 1.0)
			{
				lambda *= 0.1;
			}
			else
			{
				lambda *= 10;
			}
			// 更新参数坐标
			r = r_new;
			s = s_new;
			t = t_new;
		}
		if (iter > 10000)
		{
			qDebug() << tr("参数坐标未收敛，计算数值可能不准确");
		}

	}

	vector<double>  mPostDealData::getlinearstress() { return linearstress; }

	vector <double> mPostDealData::getlocstress11() { return locstress11; };
	vector <double> mPostDealData::getlocstress22() { return locstress22; };
	vector <double> mPostDealData::getlocstress33() { return locstress33; };
	vector <double> mPostDealData::getlocstress12() { return locstress12; };
	vector <double> mPostDealData::getlocstress13() { return locstress13; };
	vector <double> mPostDealData::getlocstress23() { return locstress23; };

}