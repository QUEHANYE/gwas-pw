/*
 * SNP_PW.cpp
 *
 */
#include "SNP_PW.h"
using namespace std;

SNP_PW::SNP_PW(){

}



SNP_PW::SNP_PW(string rs, string c, int p, double Z, double ZZ, double V, double VV, vector<bool> an, vector<int> ds, vector<vector<pair<int, int> > > dmodels, double prior, double cor){
	//for pairwise
	id = rs;
	chr = c;
	pos = p;
	Z1 = Z;
	Z2 = ZZ;
	V1 = V;
	V2 = VV;
	W = prior;
	for (vector<bool>::iterator it = an.begin(); it != an.end(); it++) {
		annot.push_back(*it);
		if (*it) annot_weight.push_back(1.0);
		else annot_weight.push_back(0.0);
	}

	//distribute weights
	float s = 0;
	for (vector<float>::iterator it = annot_weight.begin(); it != annot_weight.end(); it++) s += *it;
	if (s > 0) for (int i = 0; i < annot_weight.size(); i++) annot_weight[i]  = annot_weight[i]/s;

	for (vector<int>::iterator it = ds.begin(); it != ds.end(); it++) dists.push_back(*it);
	// append distance annotations
	append_distannots(dmodels);
	nannot = annot.size();
	BF1 = calc_logBF1(cor);
	BF2 = calc_logBF2(cor);
	BF3 = calc_logBF3(cor);
}


void SNP_PW::append_distannots(vector<vector<pair<int, int> > > dmodels){
	for (int i = 0; i < dists.size(); i++){
		int dist = dists[i];
		bool found = false;
		vector<pair<int, int> > model = dmodels[i];
		for (vector<pair<int, int> >::iterator it = model.begin(); it != model.end(); it++){
			int st = it->first;
			int sp = it->second;
			if (dist >= st && dist < sp) {
				if (found){
					cerr << "ERROR: SNP "<< id << " is in more than one distance bin for distance measure number "<< i << "\n";
					exit(1);
				}
				annot.push_back(true);
				annot_weight.push_back(1.0);
				found = true;
			}
			else {
				annot.push_back(false);
				annot_weight.push_back(0.0);
			}
		}
	}
}

double SNP_PW::get_x(vector<double> lambda){
	//no annotations yet
	return 0.0;
	/*
	if (lambda.size() != nannot){
		cerr << "ERROR: SNP "<< id << ". Lambda has "<< lambda.size()<< " entries. nannot is " << nannot << "\n";
		exit(1);
	}
	if (nannot == 0) return 0;
	double toreturn = 0;
	for (int i = 0; i < nannot; i++) {
		if (annot[i]) toreturn += lambda[i];
	}
	return toreturn;
	*/
}

double SNP_PW::get_beta1(){
	return Z1*sqrt(V1);
}

double SNP_PW::get_beta2(){
	return Z2* sqrt(V2);
}
/*
double SNP_PW::get_x_cond(vector<double> lambda, double lambdac){
	if (lambda.size() != nannot){
		cerr << "ERROR: SNP "<< id << ". Lambda has "<< lambda.size()<< " entries. nannot is " << nannot << "\n";
		exit(1);
	}
	if (nannot == 0) return 0;
	double toreturn = 0;
	for (int i = 0; i < nannot; i++) {
		if (annot[i]) toreturn += lambda[i];
	}
	if (condannot) toreturn+= lambdac;
	return toreturn;
}
*/
double SNP_PW::calc_logBF1(double C){
	double toreturn = 0;
	double r = W/ (V1+W);
	toreturn += log ( sqrt(1-r) );

	double tmp = Z1*Z1*r- 2*C*Z1*Z2*(1-sqrt(1-r));
	toreturn += tmp/ (2*(1-C*C)) ;

	//toreturn += - (Z*Z*r/2);
	return toreturn;
}

double SNP_PW::BF1_C(SNP_PW* s1, double C, pair<double, double> R){
	// testing for an effect only on phenotype 1. Regress out effect beta1_1 from phenotype 1, beta1_2 from phenotype 2.
	// Z_cor1 = (beta_1 - beta1_1*D/ tmpV)/SE
	// Z_cor2 = (beta_2 - beta1_2*D/ tmpV)/SE
	double toreturn = 0;

	//get betas
	double tmpB1 = Z1*sqrt(V1);
	double tmpB2 = Z2*sqrt(V2);

	//get other betas
	double beta1_1 = s1->get_beta1();
	double beta1_2 = s1->get_beta2();

	//correct betas


	tmpB1 = tmpB1 - beta1_1*R.first;
	tmpB2 = tmpB2 - beta1_2*R.first;

	//new Z-scores
	double newV1 = V1+ s1->V1*( 2*R.second - R.first *R.first);
	double newV2 = V2+ s1->V2*( 2*R.second - R.first *R.first);

	double tmpZ1 = tmpB1/ sqrt(newV1);
	double tmpZ2 = tmpB2/sqrt(newV2);

	//calc_BF
	double r = W/ (newV1+W);
	toreturn += log ( sqrt(1-r) );
	double tmp = tmpZ1*tmpZ1*r- 2*C*tmpZ1*tmpZ2*(1-sqrt(1-r));
	toreturn += tmp/ (2*(1-C*C)) ;
	return toreturn;

}
double SNP_PW::calc_logBF2(double C){
	double toreturn = 0;
	double r = W/ (V2+W);
	toreturn += log ( sqrt(1-r) );

	double tmp = Z2*Z2*r- 2*C*Z1*Z2*(1-sqrt(1-r));
	toreturn += tmp/ (2*(1-C*C)) ;

	return toreturn;
}

double SNP_PW::BF2_C(SNP_PW * s1,  double C, pair<double, double> R, double VarR){
	double toreturn = 0;

	//get betas
	double tmpB1 = Z1*sqrt(V1);
	double tmpB2 = Z2*sqrt(V2);

	//get other betas
	double beta1_1 = s1->get_beta1();
	double beta1_2 = s1->get_beta2();

	//correct betas

	double newV1, newV2;

	//if highly correlated, set conditional effect to 0;
	if (fabs(R.first) > 0.8){
		tmpB1 = 0;
		tmpB2 = 0;
		newV1 = V1;
		newV2 = V2;

	}
	else{
	//cout<< tmpB1 << " "<< tmpB2 << " ";
		tmpB1 = tmpB1 - beta1_1*R.first;
		tmpB2 = tmpB2 - beta1_2*R.first;
		cout << tmpB1 << " "<< tmpB2 << " "<< R.first << " betas before after\n";
		//correct variances
		double ratio1 = s1->V1/V1; // ~ N1 p1 ( 1-p1)/  N2 p2 (1-p2)
		ratio1 = ratio1/VarR; //VarR = p1(1-p1)/ p2(1-p2), so ~ N1/N2
		if (ratio1 < 1) ratio1 = 1;

		double ratio2 = s1->V2/V2;
		ratio2 = ratio2/VarR;
		if (ratio2 < 1) ratio2 = 1;

		//cout << "\n"<< id << " "<< ratio1 << " " <<V1 << " "<< s1->V1 << " "<< ratio2 << " "<< VarR<<" ratios\n";

		newV1 = ratio1*V1+ s1->V1*( 2*R.second - (1/ratio1)* R.first *R.first);
		newV2 = ratio2*V2+ s1->V2*( 2*R.second - (1/ratio2)* R.first *R.first);

	}
	//new Z-scores
	//cout << id << " "<<V1<< " " <<  s1->V1 << " "<< newV1 << " "<< newV2 << " "<< R.first << " "<< R.second << "\n";
	double tmpZ1 = tmpB1/ sqrt(newV1);
	double tmpZ2 = tmpB2/sqrt(newV2);
	//cout << id << " " <<  Z1 << " "<< Z2 << " "<< tmpZ1 << " "<< tmpZ2 << " Zs\n";
	//BF
	double r = W/ (newV2+W);
	toreturn += log ( sqrt(1-r) );

	double tmp = tmpZ2*tmpZ2*r- 2*C*tmpZ1*tmpZ2*(1-sqrt(1-r));
	toreturn += tmp/ (2*(1-C*C)) ;

	return toreturn;
}

double SNP_PW::calc_logBF3( double C){
	double toreturn = 0;
	double r1 = W/ (V1+W);
	double r2 = W/ (V2+W);
	toreturn += log ( sqrt(1-r1) ) + log(sqrt(1-r2));

	double tmp = Z1*Z1*r1+Z2*Z2*r2- 2*C*Z1*Z2*(1-sqrt(1-r1)*sqrt(1-r2));
	toreturn += tmp/ (2*(1-C*C)) ;

	return toreturn;
}

double SNP_PW::BF3_C(SNP_PW* s1, double C, pair<double, double> R){
	double toreturn = 0;

	//get betas
	double tmpB1 = Z1*sqrt(V1);
	double tmpB2 = Z2*sqrt(V2);

	//get other betas
	double beta1_1 = s1->get_beta1();
	double beta1_2 = s1->get_beta2();

	//correct betas


	tmpB1 = tmpB1 - beta1_1*R.first;
	tmpB2 = tmpB2 - beta1_2*R.first;


	//new Z-scores
	double newV1 = V1+ s1->V1*( 2*R.second - R.first *R.first);
	double newV2 = V2+ s1->V2*( 2*R.second - R.first *R.first);

	double tmpZ1 = tmpB1/ sqrt(newV1);
	double tmpZ2 = tmpB2/sqrt(newV2);

	//BF
	double r1 = W/ (newV1+W);
	double r2 = W/ (newV2+W);
	toreturn += log ( sqrt(1-r1) ) + log(sqrt(1-r2));

	double tmp = tmpZ1*tmpZ1*r1+tmpZ2*tmpZ2*r2- 2*C*tmpZ1*tmpZ2*(1-sqrt(1-r1)*sqrt(1-r2));
	toreturn += tmp/ (2*(1-C*C)) ;

	return toreturn;

}

double SNP_PW::approx_v1(){
	double toreturn;
	toreturn = 2*f*(1-f) * (double) N1;
	toreturn = 1.0/toreturn;
	return toreturn;
}


double SNP_PW::approx_v2(){
	double toreturn;
	toreturn = 2*f*(1-f) * (double) N2;
	toreturn = 1.0/toreturn;
	return toreturn;
}

double SNP_PW::ln_MVN(vector<double> beta, vector<vector<double> > S){
	// log bivariate normal density
	// assume mu = [0,0]
	// S = covariance matrix
	// beta = [\hat \beta_1, \hat \beta_2]


	double toreturn = 0;

	//determinant
	double det = S.at(0).at(0) * S.at(1).at(1) - S.at(0).at(1)*S.at(1).at(0);
	cout << "det " << det << "\n";
	//invert
	vector<vector<double> > invS;
	vector<double> tmp (2,0.0);
	vector<double> tmp2 (2, 0.0);
	invS.push_back(tmp); invS.push_back(tmp2);
	invS.at(0).at(0) = S.at(1).at(1) / det;
	invS.at(1).at(1) = S.at(0).at(0)/ det;
	invS.at(0).at(1) = -S.at(0).at(1)/ det;
	invS.at(1).at(0) = -S.at(1).at(0)/ det;
	cout << "\n";
	for (int i = 0; i <2 ; i++){
		for (int j = 0; j < 2; j++){
			cout << invS[i][j]<< " ";
		}
		cout << "\n";
	}
	//exponent
	double t = beta.at(0)*beta.at(0)*invS.at(0).at(0) + beta.at(1)*beta.at(1)*invS.at(1).at(1) + 2*beta.at(0)*beta.at(1)*invS.at(0).at(1);
	cout << "\nt "<< t<< "\n\n";
	// density
	toreturn = -log(2*M_PI) - log(sqrt(det));
	toreturn+= -0.5 * t;
	return toreturn;
}

