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

using namespace Eigen;
using namespace std;
using boost::shared_ptr;

template<typename T>
class DirMM : public DpMM<T>
{
public:
  DirMM(const Dir<T>& alpha, const shared_ptr<BaseMeasure<T> >& theta);
  DirMM(const Dir<T>& alpha, const vector<shared_ptr<BaseMeasure<T> > >& thetas);
  virtual ~DirMM();

  virtual void initialize(const Matrix<T,Dynamic,Dynamic>& x);
  virtual void initialize(const shared_ptr<ClData<T> >& cld)
    {cout<<"not supported"<<endl; assert(false);};

  virtual void sampleLabels();
  virtual void sampleParameters();

  virtual T logJoint();
  virtual const VectorXu& labels(){return z_;};
  virtual const VectorXu& getLabels(){return z_;};
  virtual uint32_t getK() const { return K_;};

//  virtual MatrixXu mostLikelyInds(uint32_t n);
  virtual MatrixXu mostLikelyInds(uint32_t n, Matrix<T,Dynamic,Dynamic>& logLikes);

  Matrix<T,Dynamic,1> getCounts();

protected: 
  uint32_t K_;
  Dir<T> dir_;
  Cat<T> pi_;
#ifdef CUDA
  SamplerGpu<T>* sampler_;
#else 
  Sampler<T>* sampler_;
#endif
  Matrix<T,Dynamic,Dynamic> pdfs_;
//  Cat cat_;
  shared_ptr<BaseMeasure<T> > theta0_;
  vector<shared_ptr<BaseMeasure<T> > > thetas_;

  Matrix<T,Dynamic,Dynamic> x_;
  VectorXu z_;
};

// --------------------------------------- impl -------------------------------


template<typename T>
DirMM<T>::DirMM(const Dir<T>& alpha, const shared_ptr<BaseMeasure<T> >& theta) :
  K_(alpha.K_), dir_(alpha), pi_(dir_.sample()), //cat_(dir_.sample()),
  theta0_(theta)
{};


template<typename T>
DirMM<T>::DirMM(const Dir<T>& alpha, 
    const vector<shared_ptr<BaseMeasure<T> > >& thetas) :
  K_(alpha.K_), dir_(alpha), pi_(dir_.sample()), //cat_(dir_.sample()),
  thetas_(thetas)
{};


template<typename T>
DirMM<T>::~DirMM()
{
  if (sampler_ != NULL) delete sampler_;
};

template <typename T>
Matrix<T,Dynamic,1> DirMM<T>::getCounts()
{
  return counts(z_,K_);
};


template<typename T>
void DirMM<T>::initialize(const Matrix<T,Dynamic,Dynamic>& x)
{
  cout<<"init"<<endl;
  x_ = x;
  // randomly init labels from prior
  z_.setZero(x.cols());
  cout<<"sample pi"<<endl;
  pi_ = dir_.sample(); 
  cout<<"init pi="<<pi_.pdf().transpose()<<endl;
  pi_.sample(z_);

  pdfs_.setZero(x.cols(),K_);
#ifdef CUDA
  sampler_ = new SamplerGpu<T>(x.cols(),K_,dir_.pRndGen_);
#else 
  sampler_ = new Sampler<T>(dir_.pRndGen_);
#endif

  // init the parameters
  if(thetas_.size() == 0)
  {
    cout<<"creating thetas"<<endl;
    for (uint32_t k=0; k<K_; ++k)
      thetas_.push_back(shared_ptr<BaseMeasure<T> >(theta0_->copy()));
  }
#pragma omp parallel for
  for(uint32_t k=0; k<K_; ++k)
    thetas_[k]->posterior(x_,z_,k);
//  for (uint32_t k=0; k<K_; ++k)
//    thetas_[k].initialize(x_,z_);
};

template<typename T>
void DirMM<T>::sampleLabels()
{
  // obtain posterior categorical under labels
  pi_ = dir_.posterior(z_).sample();
//  cout<<pi_.pdf().transpose()<<endl;
  
#pragma omp parallel for
  for(uint32_t i=0; i<z_.size(); ++i)
  {
    //TODO: could buffer this better
    // compute categorical distribution over label z_i
    VectorXd logPdf_z = pi_.pdf().array().log();
    for(uint32_t k=0; k<K_; ++k)
    {
//      cout<<thetas_[k].logLikelihood(x_.col(i))<<" ";
      logPdf_z[k] += thetas_[k]->logLikelihood(x_.col(i));
    }
//    cout<<endl;
    // make pdf sum to 1. and exponentiate
    pdfs_.row(i) = (logPdf_z.array()-logSumExp(logPdf_z)).exp().matrix().transpose();
//    cout<<pi_.pdf().transpose()<<endl;
//    cout<<pdf.transpose()<<" |.|="<<pdf.sum();
//    cout<<" z_i="<<z_[i]<<endl;
  }
  // sample z_i
  sampler_->sampleDiscPdf(pdfs_,z_);
};


template<typename T>
void DirMM<T>::sampleParameters()
{
#pragma omp parallel for 
  for(uint32_t k=0; k<K_; ++k)
  {
    thetas_[k]->posterior(x_,z_,k);
//    cout<<"k:"<<k<<endl;
//    thetas_[k]->print();
  }
};


template<typename T>
T DirMM<T>::logJoint()
{
  T logJoint = dir_.logPdf(pi_);
  cout<<"  [logJoint="<<logJoint<<" -> ";
#pragma omp parallel for reduction(+:logJoint)  
  for (uint32_t k=0; k<K_; ++k)
    logJoint = logJoint + thetas_[k]->logPdfUnderPrior();
  cout<<" "<<logJoint<<" -> ";
#pragma omp parallel for reduction(+:logJoint)  
  for (uint32_t i=0; i<z_.size(); ++i)
    logJoint = logJoint + thetas_[z_[i]]->logLikelihood(x_.col(i));
  cout<<" "<<logJoint<<"]"<<endl;
  return logJoint;
};


template<typename T>
MatrixXu DirMM<T>::mostLikelyInds(uint32_t n, Matrix<T,Dynamic,Dynamic>& logLikes)
{
  MatrixXu inds = MatrixXu::Zero(n,K_);
  logLikes = Matrix<T,Dynamic,Dynamic>::Ones(n,K_);
  logLikes *= -99999.0;
  
#pragma omp parallel for 
  for (uint32_t k=0; k<K_; ++k)
  {
    for (uint32_t i=0; i<z_.size(); ++i)
      if(z_(i) == k)
      {
        T logLike = thetas_[z_[i]]->logLikelihood(x_.col(i));
        for (uint32_t j=0; j<n; ++j)
          if(logLikes(j,k) < logLike)
          {
            for(uint32_t l=n-1; l>j; --l)
            {
              logLikes(l,k) = logLikes(l-1,k);
              inds(l,k) = inds(l-1,k);
            }
            logLikes(j,k) = logLike;
            inds(j,k) = i;
//            cout<<"after update "<<logLike<<endl;
//            Matrix<T,Dynamic,Dynamic> out(n,K_*2);
//            out<<logLikes.cast<T>(),inds.cast<T>();
//            cout<<out<<endl;
            break;
          }
      }
  } 
  cout<<"::mostLikelyInds: logLikes"<<endl;
  cout<<logLikes<<endl;
  cout<<"::mostLikelyInds: inds"<<endl;
  cout<<inds<<endl;
  return inds;
};

//template<class T>
//T DirMM<T>::avgIntraClusterDeviation()
//{
//  Matrix<T,Dynamic,1> deviates(K_);
//  deviates.setZero(K_);
//#pragma omp parallel for 
//  for (uint32_t k=0; k<K_; ++k)
//  {
//    T N_k = 0.0;
//    for (uint32_t i=0; i<N_; ++i)
//      if(z_(i) == k)
//      {
//        T dot = thetas_[k]->transpose()*spx_->col(i);
//        deviates(k) += acos(min(1.0,max(-1.0,dot)));
//        N_k ++;
//      }
//    if(N_k > 0.0) deviates(k) /= N_k;
//  }
//  return deviates.sum()/static_cast<T>(K_);
//}