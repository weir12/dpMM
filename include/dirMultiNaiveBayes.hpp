/* Copyright (c) 2015, Julian Straub <jstraub@csail.mit.edu>                    
 * Licensed under the MIT license. See the license file LICENSE.                
 */

#pragma once
#include <iostream>
#include <stdint.h>
#include <vector>
#include <Eigen/Dense>

#include <boost/shared_ptr.hpp>

#include "dpMM.hpp"
#include "cat.hpp"
#include "dir.hpp"
#include "niw.hpp"
#include "sampler.hpp"
#include "basemeasure.hpp"
#include "niwBaseMeasure.hpp"
#include "niwSphere.hpp"
using namespace Eigen;
using std::cout;
using std::endl; 
using boost::shared_ptr;
using std::vector; 

template<typename T=double>
class DirMultiNaiveBayes : public DpMM<T>{

public:
  DirMultiNaiveBayes(std::ifstream &in, boost::mt19937 *rng);
  DirMultiNaiveBayes(const Dir<Cat<T>, T>& alpha, const vector<boost::shared_ptr<BaseMeasure<T> > >&thetas);
  DirMultiNaiveBayes(const Dir<Cat<T>, T>& alpha, const vector< vector<boost::shared_ptr<BaseMeasure<T> > > >&thetas);
  virtual ~DirMultiNaiveBayes();

  virtual void loadData(const vector<vector<Matrix<T,Dynamic,Dynamic> > > &x);//does nothing other than load data
  virtual void initialize(const vector<vector< Matrix<T,Dynamic,Dynamic> > >&x);
  virtual void initialize(const vector<vector< Matrix<T,Dynamic,Dynamic> > >&x, VectorXu &z);
  virtual void initialize(const boost::shared_ptr<ClData<T> >&cld)
    {cout<<"not supported"<<endl; assert(false);};

  virtual void sampleLabels();
  virtual void MAPLabel();
  virtual void sampleParameters();
  
  virtual T logJoint(bool verbose=false);
  virtual const VectorXu& labels(){return z_;};
  virtual const VectorXu& getLabels(){return z_;};
  virtual uint32_t getK() const { return K_;};
  virtual uint32_t getM() const { return M_;};
  virtual uint32_t getN() const { return Nd_;};

//  virtual MatrixXu mostLikelyInds(uint32_t n);
  //virtual MatrixXu mostLikelyInds(uint32_t n, Matrix<T,Dynamic,Dynamic>& logLikes);

  Matrix<T,Dynamic,1> getCounts();

  virtual void inferAll(uint32_t nIter, bool verbose=false);
  
  virtual void dump(std::ofstream& fOutMeans, std::ofstream& fOutCovs); 
  virtual void dump_clean(std::ofstream &out);
  
  virtual vector<boost::shared_ptr<BaseMeasure<T> > > getThetas(uint32_t m) {
	  return(thetas_[m]);
  };
  virtual boost::shared_ptr<BaseMeasure<T> > getThetas(uint32_t m, uint32_t k) {
	  return(thetas_[m][k]);
  };

  virtual T evalLogLik(vector<Matrix<T,Dynamic,Dynamic> > xnew, uint32_t clusterInd, vector<uint32_t> comp2eval =vector<uint32_t>());
  virtual vector<T> evalLogLik(vector<Matrix<T,Dynamic,Dynamic> > xnew, vector<uint32_t> clusterInd, vector<uint32_t> comp2eval =vector<uint32_t>());
  
  virtual uint32_t sampleLabels(vector<Matrix<T,Dynamic,Dynamic> > xnew, vector<uint32_t> comp2eval =vector<uint32_t>());
  virtual vector<uint32_t> sampleLabels(vector<vector<Matrix<T,Dynamic,Dynamic> > > xnew, vector<uint32_t> comp2eval =vector<uint32_t>());
  virtual uint32_t MAPLabels(vector<Matrix<T,Dynamic,Dynamic> > xnew, vector<uint32_t> comp2eval =vector<uint32_t>());
  virtual vector<uint32_t> MAPLabels(vector<vector<Matrix<T,Dynamic,Dynamic> > > xnew, vector<uint32_t> comp2eval =vector<uint32_t>());
  virtual void updatePDF();

protected: 
  virtual uint32_t labels_sample_max(vector<Matrix<T,Dynamic,Dynamic> > xnew, vector<uint32_t> comp2eval, bool return_MAP_labels=false);

  uint32_t Nd_;  
  uint32_t K_; //num cluseters
  uint32_t M_; //num data sources
  Dir<Cat<T>, T> dir_;
  Cat<T> pi_;
#ifdef CUDA
  SamplerGpu<T>* sampler_;
#else 
  Sampler<T>* sampler_;
#endif
  virtual void initialize_sampler(); 
  Matrix<T,Dynamic,Dynamic> pdfs_;
//  Cat cat_;
  vector<vector<boost::shared_ptr<BaseMeasure<T> > > > thetas_;  // theta_[M][K]

  vector<vector<Matrix<T,Dynamic,Dynamic> > > x_; //x_[M][doc](:,word)
  VectorXu z_;

  virtual void helper_setDims();
  vector<VectorXu> dataDim; 
};

// --------------------------------------- impl -------------------------------

template<typename T>
void DirMultiNaiveBayes<T>::initialize_sampler() {
	if (sampler_ != NULL) {
		delete sampler_;
		sampler_ = NULL;
	}
	//initialize sampler 
	#ifdef CUDA
	  sampler_ = new SamplerGpu<T>(uint32_t(Nd_),K_,dir_.pRndGen_);
	#else 
	  sampler_ = new Sampler<T>(dir_.pRndGen_);
	#endif
}

template<typename T>
DirMultiNaiveBayes<T>::DirMultiNaiveBayes(std::ifstream &in, boost::mt19937 *rng) :
sampler_(NULL), dir_(Matrix<T,2,1>::Ones(),rng), pi_(dir_.sample()){
	//initialize the class from the file pointer given
	in >> M_; 
	in >> K_;
	in >> Nd_;

	vector<uint> dim;
	vector<baseMeasureType> type;
	Matrix<T,Dynamic,1> alpha(K_), pi(K_); 

	for(uint m = 0; m<M_; ++m){
		uint temp; 
		in >> temp; 
		dim.push_back(temp);
	}

	for(uint m = 0; m<M_; ++m){
		uint temp;
		in >> temp;
		type.push_back(baseMeasureType(temp));
	}

	z_ = VectorXu(Nd_);
	for(uint n=0; n<Nd_; ++n) 
		in >> z_(n);	

	for(uint k=0; k<K_; ++k)
		in >> alpha(k);

	for(uint k=0; k<K_; ++k)
		in >> pi(k); 

	pdfs_ = Matrix<T,Dynamic,Dynamic>(Nd_,K_);
	//for(uint n=0; n<Nd_*K_; ++n)
	//	in >> pdfs_(n%Nd_, (n-(n%Nd_))/Nd_); 
	//	//in >> pdfs_((n-(n%Nd_))/Nd_, n%Nd_); 
	////pdfs_ = pdfs_.transpose();

	//get parameters
	for(uint32_t m=0; m<M_; ++m) {
		baseMeasureType typeIter = baseMeasureType(type[m]);
		vector<boost::shared_ptr<BaseMeasure<T> > > thetaM; 
		if(typeIter==NIW_SAMPLED) {
			uint Diter = dim[m];
			T nu, kappa; 
			Matrix<T,Dynamic,Dynamic> scatter(Diter, Diter), sigma(Diter,Diter); 
			Matrix<T,Dynamic,1> theta(Diter), mu(Diter); 
			for(uint32_t k=0; k<K_; ++k) {
				//get nu and kappa
					in>> nu;
					in>> kappa;
				//get theta
					for(uint n=0; n<Diter; ++n)
						in >> theta(n); 
					theta = theta.transpose(); 
				//get scatter
					for(uint n=0; n<Diter*Diter; ++n)
						in >> scatter((n-(n%Diter))/Diter, n%Diter); 
				//get mean
					for(uint n=0; n<Diter; ++n)
						in >> mu(n); 
					mu = mu.transpose();
				//get sigma
					for(uint n=0; n<Diter*Diter; ++n)
						in >> sigma((n-(n%Diter))/Diter, n%Diter); 

				//build theta[m][k]
				NIW<T> niw(scatter,theta,nu,kappa,rng);
				Normal<T> normal(mu,sigma,rng);
				boost::shared_ptr<NiwSampled<T> > baseIter( new NiwSampled<T>(niw, normal));

				//set 
				thetaM.push_back(boost::shared_ptr<BaseMeasure<T> >(baseIter));
			}
		} else if(typeIter==NIW_SPHERE) {
			uint Diter = dim[m]-1;
			T nu; 
			T counts;
			Matrix<T,Dynamic,Dynamic> scatter(Diter, Diter), sigma(Diter,Diter),delta(Diter,Diter);  
			Matrix<T,Dynamic,1> mu(Diter+1), mu_prior(Diter), north(Diter+1); 
			for(uint32_t k=0; k<K_; ++k) {
				//get nu
					in>> nu;
					in>> counts;
				//get prior mean 
					for(uint n=0; n<Diter; ++n)
						in>> mu_prior(n);
				//get scatter
					for(uint n=0; n<Diter*Diter; ++n)
						in >> scatter((n-(n%Diter))/Diter, n%Diter); 
				//get delta
					for(uint n=0; n<Diter*Diter; ++n)
						in >> delta((n-(n%Diter))/Diter, n%Diter); 
				//get mean
					for(uint n=0; n<(Diter+1); ++n)
						in >> mu(n); 
					mu = mu.transpose();
				//get sigma
					for(uint n=0; n<Diter*Diter; ++n)
						in >> sigma((n-(n%Diter))/Diter, n%Diter); 
				//get north
					for(uint n=0; n<(Diter+1); ++n)
						in >> north(n); 
					north = north.transpose();

				//build theta[m][k]
				IW<T> iw(delta,nu, scatter, mu_prior, counts, rng);
				//IW<T> iw(delta,nu, rng);
				NiwSphere<T> niwSp(iw,rng);
				niwSp.S_ = Sphere<T>(north);
				niwSp.normalS_ = NormalSphere<T>(mu,sigma,rng);
				

				//set 
				thetaM.push_back(boost::shared_ptr<BaseMeasure<T> >(niwSp.copy()));
				//boost::shared_ptr<NiwSphere<T> > baseIter( new NiwSphere<T>(iw,rng));
				//thetaM.push_back(boost::shared_ptr<BaseMeasure<T> >(baseIter));
				
			}


		} else {
				std::cerr << "[DirMultiNaiveBayes::dump_clean] error saving...returning" << endl;
				return;
		}
		thetas_.push_back(thetaM);
	}

	dir_ =  Dir<Cat<T>, T>(alpha,rng);
	pi_ = Cat<T>(pi,rng); 


	this->initialize_sampler();

	//cout << "M:" << M_ << " K: " << K_ << " nd: " << Nd_ << endl;
	//for(uint m = 0; m<M_; ++m){
	//	cout << m << ": d="<< dim[m] << ", type=" << type[m] << endl;
	//}
	//cout << "z_ " << z_.transpose() << endl;
	//cout << "alpha " << alpha.transpose() << endl;
}

template<typename T>
DirMultiNaiveBayes<T>::DirMultiNaiveBayes(const Dir<Cat<T>,T>& alpha, 
    const vector<boost::shared_ptr<BaseMeasure<T> > >& thetas) :
  sampler_(NULL), K_(alpha.K_), dir_(alpha), pi_(dir_.sample()), M_(uint32_t(thetas.size())) 
{ 
	for (uint32_t m=0; m<M_; ++m) 
	{	
		vector<boost::shared_ptr<BaseMeasure<T> > > temp;
		for (uint32_t k=0; k<K_; ++k) 
		{   	
      		temp.push_back(boost::shared_ptr<BaseMeasure<T> >(thetas[m]->copy()));
      	}
      thetas_.push_back(temp); 
    }
};

template<typename T>
DirMultiNaiveBayes<T>::DirMultiNaiveBayes(const Dir<Cat<T>,T>& alpha, 
    const vector< vector<boost::shared_ptr<BaseMeasure<T> > > >& theta) :
 sampler_(NULL), K_(alpha.K_), dir_(alpha), pi_(dir_.sample()), M_(uint32_t(theta.size()), thetas_(theta) ) 
{ };

template<typename T>
DirMultiNaiveBayes<T>::~DirMultiNaiveBayes()
{
  if (sampler_ != NULL) {
	delete sampler_;
	sampler_ = NULL;
  }
};

template <typename T>
Matrix<T,Dynamic,1> DirMultiNaiveBayes<T>::getCounts()
{
  return counts<T,uint32_t>(z_,K_);
};

template<typename T>
void DirMultiNaiveBayes<T>::loadData(const vector<vector<Matrix<T,Dynamic,Dynamic> > > &x){
  x_ = x;
  this->helper_setDims();
}

template<typename T>
void DirMultiNaiveBayes<T>::initialize(const vector< vector< Matrix<T,Dynamic,Dynamic> > > &x)
{
  uint Nd= uint32_t(x.front().size());

  // randomly init labels from prior
  VectorXu z; 
  z.setZero(Nd);
  
  Cat<T> pi = dir_.sample(); 
  pi.sample(z);

  //delegate the initialization to the main intitialization function
  this->initialize(x,z); 
};

template<typename T>
void DirMultiNaiveBayes<T>::initialize(const vector< vector< Matrix<T,Dynamic,Dynamic> > > &x, VectorXu &z)
{
  Nd_= uint32_t(x.front().size());

  //init data and labels from given 
  x_ = x;
  z_ = z; 
  
  pi_ = dir_.sample(); 

  pdfs_.setZero(Nd_,K_);

this->initialize_sampler();
this->helper_setDims();

this->sampleParameters();
};

template<typename T>
void DirMultiNaiveBayes<T>::helper_setDims()
{
	dataDim.clear();
	dataDim.reserve(M_); 
	for(uint32_t m=0; m<M_; ++m) {
		VectorXu temp(x_[m].size()); 
		for(uint32_t t=0; t<x_[m].size(); ++t)
			temp(t) = x_[m][t].cols(); 
		dataDim.push_back(temp); 
	}
}

template<typename T>
void DirMultiNaiveBayes<T>::sampleLabels()
{
	// obtain posterior categorical under labels
	pi_ = dir_.posterior(z_).sample();
	//  cout<<pi_.pdf().transpose()<<endl;
	this->updatePDF();
	// sample z_i
	sampler_->sampleDiscPdf(pdfs_,z_);
};


template<typename T>
void DirMultiNaiveBayes<T>::updatePDF() {
// compute categorical distribution over label z_i 
// no need to re-compute the array and log every iteration)
VectorXd logPdf_z_value = pi_.pdf().array().log();

#pragma omp parallel for
  for(int32_t d=0; d<int32_t(Nd_); ++d)
  {
	VectorXd logPdf_z = logPdf_z_value; 
	for(uint32_t m=0; m<uint32_t(M_); ++m)
	{
		for(uint32_t k=0; k<K_; ++k)
		{
	//      cout<<thetas_[k].logLikelihood(x_.col(d))<<" ";
			for(uint32_t w=0; w<x_[m][d].cols(); ++w)
			{
				logPdf_z[k] += thetas_[m][k]->logLikelihood(x_[m][d],w);
			}
		}
	}
//    cout<<endl;
    // make pdf sum to 1. and exponentiate
    pdfs_.row(d) = (logPdf_z.array()-logSumExp(logPdf_z)).exp().matrix().transpose();
//    cout<<pi_.pdf().transpose()<<endl;
//    cout<<pdf.transpose()<<" |.|="<<pdf.sum();
//    cout<<" z_i="<<z_[d]<<endl;
  }

}


template<typename T>
void DirMultiNaiveBayes<T>::MAPLabel()
{
	/* it chooses the MAP label rather than sampling */
	this->updatePDF();

	#pragma omp parallel for
	 for(int32_t d=0; d<int32_t(Nd_); ++d) {
		 int r,c;
		 pdfs_.row(d).maxCoeff(&r, &c);
		 z_(d) = c; 
	 }

};



template<typename T>
void DirMultiNaiveBayes<T>::sampleParameters()
{
//unpacks the contains here vector<vector<Matrix>> into what the posterior expects Matrix
	MatrixXu dim(M_,K_); 
	for(uint32_t m=0; m<M_; ++m) {
		#pragma omp parallel for
		for(int32_t k=0; k<int32_t(K_); ++k) {
			VectorXu temp = (z_.array()==k).select(dataDim[m],0); 
			dim(m,k) = temp.sum();	
		}
	}

	for(uint32_t m=0; m<M_; ++m) {
		#pragma omp parallel for
		for(int32_t k=0; k<int32_t(K_); ++k) {
			if(dim(m,k)!=0) {
				Matrix<T,Dynamic,Dynamic> dataIn(x_[m].front().rows(),dim(m,k)); 

				uint32_t count=0;
				for(uint32_t d=0; d<Nd_; ++d) {
					if(z_[d]==k) {
						int add_size =int(x_[m][d].cols()); 
						dataIn.middleCols(count,add_size) = x_[m][d]; 
						count+=add_size;

						if(count==dim(m,k))
							break; //early out if you found them all
					}
				}
				//always sends zeros as index and look for index==0
				thetas_[m][k]->posterior(dataIn,VectorXu::Zero(dim(m,k)),0); 
			} else {
				Matrix<T,Dynamic,Dynamic> dataIn = Matrix<T,Dynamic,Dynamic>::Zero(x_[m].front().rows(),1); 
				//the posterior needs to reset 
				thetas_[m][k]->posterior(dataIn,VectorXu::Zero(1),1); 
				//passing in one data point (all zeros, with 1 index value=0 and looking for 1) 
			}
		}
	}
};


template<typename T>
T DirMultiNaiveBayes<T>::logJoint(bool verbose)
{
  T logJoint = dir_.logPdf(pi_);
  if(verbose)
  	cout<<"log p(pi)="<<logJoint<<" -> ";

  for (int32_t m=0; m<int32_t(M_); ++m)
  {
	  #pragma omp parallel for reduction(+:logJoint)  
	  for (int32_t k=0; k<int32_t(K_); ++k)
	  {
		logJoint = logJoint + thetas_[m][k]->logPdfUnderPrior();
	  }
  }

  if(verbose)
	cout<<"log p(pi)*p(theta)="<<logJoint<<" -> ";

	for (int32_t m=0; m<int32_t(M_); ++m) 
	{
		for (int32_t d=0; d<int32_t(Nd_); ++d)
		{
			#pragma omp parallel for reduction(+:logJoint)  
			for(int32_t w=0; w<x_[m][d].cols(); ++w)
			{
				logJoint = logJoint + thetas_[m][z_[d]]->logLikelihood(x_[m][d],w);
			}
		}
	}
  
  if(verbose)
  	cout<<"log p(phi)*p(theta)*p(x|z,theta)="<<logJoint<<"]"<<endl;
  
  return logJoint;
};


//template<typename T>
//MatrixXu DirMultiNaiveBayes<T>::mostLikelyInds(uint32_t n, Matrix<T,Dynamic,Dynamic>& logLikes)
//{
//  MatrixXu inds = MatrixXu::Zero(n,K_);
//  logLikes = Matrix<T,Dynamic,Dynamic>::Ones(n,K_);
//  logLikes *= -99999.0;
//  
//#pragma omp parallel for 
//  for (int32_t k=0; k<int32_t(K_); ++k)
//  {
//    for (uint32_t i=0; i<z_.size(); ++i)
//      if(z_(i) == k)
//      {
//        T logLike = thetas_[z_[i]]->logLikelihood(x_[i]);
//        for (uint32_t j=0; j<n; ++j)
//          if(logLikes(j,k) < logLike)
//          {
//            for(uint32_t l=n-1; l>j; --l)
//            {
//              logLikes(l,k) = logLikes(l-1,k);
//              inds(l,k) = inds(l-1,k);
//            }
//            logLikes(j,k) = logLike;
//            inds(j,k) = i;
////            cout<<"after update "<<logLike<<endl;
////            Matrix<T,Dynamic,Dynamic> out(n,K_*2);
////            out<<logLikes.cast<T>(),inds.cast<T>();
////            cout<<out<<endl;
//            break;
//          }
//      }
//  } 
//  cout<<"::mostLikelyInds: logLikes"<<endl;
//  cout<<logLikes<<endl;
//  cout<<"::mostLikelyInds: inds"<<endl;
//  cout<<inds<<endl;
//  return inds;
//};


template<typename T>
void DirMultiNaiveBayes<T>::inferAll(uint32_t nIter, bool verbose)
{ 
  if(verbose){
  	cout<<"[DirMultiNaiveBayes::inferALL] ------ inferingALL (nIter=" << nIter << ") ------"<<endl;
  	cout <<"initial labels:"<< endl;
  	cout<<this->labels().transpose()<<endl;
  }
  for(uint32_t t=0; t<nIter; ++t)
  {
    this->sampleLabels();
    this->sampleParameters();
    if(verbose || t%100==0){
		if(Nd_<=10) {
			cout << "[" << std::setw(3)<< std::setfill('0')  << t <<"] label: " 
    		<< this->labels().transpose()
      		<< " [joint= " << std::setw(6) << this->logJoint(false) << "]"<< endl;
		} else {
			cout << "[" << std::setw(3)<< std::setfill('0')  << t <<"] joint= " 
				 << std::setw(6) << this->logJoint(false) << endl;
		}
    }
  }
  //keeps the MAP label in memory
  this->MAPLabel();
}


template <typename T>
void DirMultiNaiveBayes<T>::dump(std::ofstream& fOutMeans, std::ofstream& fOutCovs)
{
	cout << "dumping MultiObs naiveBayes" << endl; 
	cout << "doc index: " << endl;  
	cout << this->labels().transpose() << endl; 
	
	cout << "printing num components: " << endl; 
	cout << M_ << endl;

	cout << "printing cluster params: " << endl; 
	cout << K_ << endl;

	for(uint32_t m=0; m<M_; ++m) {
		cout << "component: " << m  << endl;
		for(uint32_t k=0; k<K_; ++k) {
			cout << "theta: " << k  << endl;
			thetas_[m][k]->print();
		}
	}

	cout << "printing mixture params: " << endl;
	pi_.print();


}


template <typename T>
void DirMultiNaiveBayes<T>::dump_clean(std::ofstream &out){
	//clean dump, only data with specific format
//FORMAT:
	//M 1x1
	//K	1x1
	//Nd 1x1
	//D[m] 1xM
	//Type[m] 1xM
	//labels 1xNd
	//Dir alpha 1xK
	//pi pdf 1xK
	//pdf KxNd
	// mixture parameters 
	//params Loop over M then K each contains data type for specific type
	//for type 1 (NIWSampled)
		//---prior--- (NIW)
			//nu 1x1
			//kappa 1x1
			//theta 1xD
			//scatter DxD
		//---estimate (normal)
			//mu 1xD
			//Sigma DxD
	//for type 2 (NIWSphereFull)
		//----prior--- (IW)
			//nu 1x1
			//count 1x1
			//mean 1x(D-1)
			//scatter (D-1)x(D-1)
			//Delta	 (D-1)x(D-1)
		//--posterior (NormalSphere)
			//mean 1x(D-1)
			//Sigma (D-1)x(D-1)
		//--sphere--- (Sphere)
			//north 1x(D-1)
	//this fixes issues with eigen matrices printing (eg, 00-0.7 )
	int curPres = out.precision(); 
	out.precision(10); 
	IOFormat fullPresPrint(FullPrecision,DontAlignCols); 

	//prints headers
	out << M_ << endl 
		 << K_ << endl
		 << Nd_ << endl;

	//print dim
	for(uint m=0; m<M_; ++m) {
		vector<boost::shared_ptr<BaseMeasure<T> > >  theta_base = this->getThetas(m); 
		uint temp = theta_base.front()->getDim();
		out << temp << " "; 
		//out << x_[m].front().rows() << " "; 
	}
	out << endl;
	//print type
	for(uint m=0; m<M_; ++m) {
		out << thetas_[m].front()->getBaseMeasureType() << " "; 
	}
	out << endl;

	//print labels
	out << this->labels().transpose() << endl; 

	//print mixture parameters
	out << this->dir_.alpha_.transpose() << endl;
	out << this->pi_.pdf_.transpose().format(fullPresPrint) << endl;
	//out << this->pdfs_.transpose().format(fullPresPrint) << endl; 

	//print parameters
	for(uint32_t m=0; m<M_; ++m) {
		vector<boost::shared_ptr<BaseMeasure<T> > >  theta_base = this->getThetas(m); 
		for(uint32_t k=0; k<K_; ++k) {
			baseMeasureType type = theta_base[k]->getBaseMeasureType(); 
			if(type==NIW_SAMPLED) {
				boost::shared_ptr<NiwSampled<T> >  *theta_iter = 
						reinterpret_cast<boost::shared_ptr<NiwSampled<T> >* >( &theta_base[k]); 
					//printing prior 
					NIW<T> prior = theta_iter->get()->niw0_;
					out << prior.nu_				 << endl << 
						    prior.kappa_			 << endl <<
						    prior.theta_.transpose() << endl <<
						    prior.Delta_.format(fullPresPrint)	<<	endl;

					//printing posterior
					Normal<T> norm = theta_iter->get()->normal_; 
					out << norm.mu_.transpose() << endl;
					out << norm.Sigma().format(fullPresPrint) << endl;
			} else if(type==NIW_SPHERE) {
				boost::shared_ptr<NiwSphere<T> >  *theta_iter = 
						reinterpret_cast<boost::shared_ptr<NiwSphere<T> >* >( &theta_base[k]); 
				//prior
				IW<T> prior = theta_iter->get()->iw0_; 
				out << prior.nu_ 				 << endl 
					 << prior.count()			 << endl
				 	 << prior.mean().transpose() << endl
					 << prior.scatter().format(fullPresPrint)			 << endl 
					 << prior.Delta_.format(fullPresPrint)			 << endl;

				//posterior 	
				NormalSphere<T> norm = theta_iter->get()->normalS_; 
				out << norm.getMean().transpose().format(fullPresPrint) << endl;
				out << norm.Sigma().format(fullPresPrint) << endl;
				//sphere 	
				Sphere<T> sp = theta_iter->get()->S_; 
				out << sp.north().transpose() << endl; 
			} else {
					std::cerr << "[DirMultiNaiveBayes::dump_clean] error saving...returning" << endl;
					return;
			}
		}
	}


	out.precision(curPres);

}


//template <typename T>
//void DirMultiNaiveBayes<T>::dump_clean(std::ofstream &out){
//	streambuf *coutbuf = std::cout.rdbuf(); //save old cout buffer
//	cout.rdbuf(out.rdbuf()); //redirect std::cout to fout1 buffer
//	this->dump_clean(); //write using cout to the specified buffer
//	std::cout.rdbuf(coutbuf); //reset to standard output again
//}


template <typename T>
T DirMultiNaiveBayes<T>::evalLogLik(vector<Matrix<T,Dynamic,Dynamic> > xnew, 
							  uint32_t clusterInd, vector<uint32_t> comp2eval) 
{
	if(comp2eval.empty()) {
		for(uint m=0; m<M_; ++m)
			comp2eval.push_back(m); 
	}

	//T logJoint = pi_.pdf_(clusterInd);
	T logJoint  = 0; 
	for (int32_t m=0; m<int32_t(comp2eval.size()); ++m) 
	{
		if(xnew[m].cols()<24) { //use a single core if num words is small		
			for(int32_t w=0; w<xnew[m].cols(); ++w)
			{
				logJoint = logJoint + thetas_[comp2eval[m]][clusterInd]->logLikelihood(xnew[m],w);
			}
		} else { //switch to multi-core if lots of words. 
			#pragma omp parallel for reduction(+:logJoint)  
			for(int32_t w=0; w<xnew[m].cols(); ++w)
			{
				logJoint = logJoint + thetas_[comp2eval[m]][clusterInd]->logLikelihood(xnew[m],w);
			}
		}
	}
    
  return logJoint;
}


template <typename T>
vector<T> DirMultiNaiveBayes<T>::evalLogLik(vector<Matrix<T,Dynamic,Dynamic> > xnew, vector<uint32_t> clusterInd, vector<uint32_t> comp2eval) {
	vector<T> out; 
	for(uint32_t k=0; k<uint32_t(clusterInd.size()); ++k) {
		out.push_back(this->evalLogLik(xnew,clusterInd[k],comp2eval)); 
	}
	return(out);
}


template <typename T>
uint32_t DirMultiNaiveBayes<T>::labels_sample_max(vector<Matrix<T,Dynamic,Dynamic> > xnew, vector<uint32_t> comp2eval, bool return_MAP_labels)
{
	/* xnew in the form x[docs][m][words] */

VectorXd logPdf_z = pi_.pdf().array().log();

for(int32_t m=0; m<comp2eval.size(); ++m)
{
	#pragma omp parallel for
	for(int32_t k=0; k<int32_t(K_); ++k)
	{
		for(uint32_t w=0; w<xnew[m].cols(); ++w)
		{
			logPdf_z[k] += thetas_[comp2eval[m]][k]->logLikelihood(xnew[m],w);
		}
	}
}

// make pdf sum to 1. and exponentiate
Matrix<T,Dynamic,Dynamic> pdfLocal =  Matrix<T,Dynamic,Dynamic>(1,K_);

pdfLocal = (logPdf_z.array()-logSumExp(logPdf_z)).exp().matrix().transpose();

VectorXu zout = VectorXu(1);

if(return_MAP_labels) {
	// return MAP label
	int r,c; 
	pdfLocal.maxCoeff(&r, &c);
	zout(0) = c; 
} else {
	// sample z_i
	sampler_->sampleDiscPdf(pdfLocal,zout);
}

  return(zout(0)); 
};




template <typename T>
uint32_t DirMultiNaiveBayes<T>::sampleLabels(vector<Matrix<T,Dynamic,Dynamic> > xnew, vector<uint32_t> comp2eval) {
	return(this->labels_sample_max(xnew, comp2eval, false)); 
}

template <typename T>
vector<uint32_t> DirMultiNaiveBayes<T>::sampleLabels(vector<vector<Matrix<T,Dynamic,Dynamic> > > xnew, vector<uint32_t> comp2eval) 
{
	/* xnew in the form x[docs][m][words] */
	vector<uint32_t> out; 
	for(uint32_t d=0; d<xnew.size(); ++d) {
		out.push_back(this->sampleLabels(xnew[d],comp2eval)); 
	}
	return(out); 
}


template <typename T>
uint32_t DirMultiNaiveBayes<T>::MAPLabels(vector<Matrix<T,Dynamic,Dynamic> > xnew, vector<uint32_t> comp2eval) {
	return(this->labels_sample_max(xnew, comp2eval, true)); 
}


template <typename T>
vector<uint32_t> DirMultiNaiveBayes<T>::MAPLabels(vector<vector<Matrix<T,Dynamic,Dynamic> > > xnew, vector<uint32_t> comp2eval) {
	/* xnew in the form x[docs][m][words] */
	vector<uint32_t> out; 
	for(uint32_t d=0; d<xnew.size(); ++d) {
		out.push_back(this->MAPLabels(xnew[d],comp2eval)); 
	}
	return(out); 
}