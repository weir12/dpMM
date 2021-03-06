/* Copyright (c) 2015, Julian Straub <jstraub@csail.mit.edu>
 * Licensed under the MIT license. See the license file LICENSE.
 */
 

#pragma once

#include <Eigen/Dense>

#include <dpMM/basemeasure.hpp>
#include <dpMM/dir.hpp>

/*
 * NIW base measure; samples Normal distribution parameters
 * templated on a discrete distribution (Cat<T> or Mult<T>)
 */
template<class Disc, typename T>
class DirSampled : public BaseMeasure<T>
{
public:
  Dir<Disc,T> dir0_; 
  Disc disc_; // discrete distribution - Cat<T> or Mult<T>

  T count_; // number of datapoints associated with this

  DirSampled(const Dir<Disc,T>& dir);
  ~DirSampled();

  virtual baseMeasureType getBaseMeasureType() const {return(DIR_SAMPLED); }

  virtual BaseMeasure<T>* copy();
  virtual DirSampled<Disc,T>* copyNative();

  T logLikelihood(const Matrix<T,Dynamic,1>& x) const;
  T logLikelihood(const Matrix<T,Dynamic,Dynamic>& x, uint32_t i) const 
    {return logLikelihood(x.col(i));};
  // assumes vector [counts]
  T logLikelihoodFromSS(const Matrix<T,Dynamic,1>& x) const;
  void posterior(const Matrix<T,Dynamic,Dynamic>& x, const VectorXu& z, 
    uint32_t k);
  // assumes vectors [counts]
  virtual void posteriorFromSS(const vector<Matrix<T,Dynamic,1> >&x, const
      VectorXu& z, uint32_t k);
  virtual void posteriorFromSS(const Matrix<T,Dynamic,1> &x);

  void sample();

  T logPdfUnderPrior() const;
  T logPdfUnderPriorMarginalized() const;
  T logPdfUnderPriorMarginalizedMerged(const boost::shared_ptr<DirSampled<Disc,T> >& other) const;

  virtual DirSampled<Disc,T>* merge(const DirSampled<Disc,T>& other);
  void fromMerge(const DirSampled<Disc,T>& dirA, const DirSampled<Disc,T>& dirB);

  void print() const;
  virtual uint32_t getDim() const {return(uint32_t(dir0_.alpha_.size()));};

//  const Matrix<T,Dynamic,Dynamic>& scatter() const {return dir0_.scatter();};
//  const Matrix<T,Dynamic,1>& mean() const {return dir0_.mean();};
  T count() const {return count_;};
  const Matrix<T,Dynamic,1>& counts() const {return dir0_.counts();};
  const Matrix<T,Dynamic,1>& pdf() const {return disc_.pdf();};
//  T& count() {return dir0_.count_;};
//  const Matrix<T,Dynamic,1>& getMean() const {return disc_.mu_;};
//
//  const Matrix<T,Dynamic,Dynamic>& Sigma() const {return disc_.Sigma();};
private:

};

typedef DirSampled<Cat<double>, double> DirCatSampledd;
typedef DirSampled<Cat<float>, float> DirCatSampledf;
typedef DirSampled<Mult<double>, double> DirMultSampledd;
typedef DirSampled<Mult<float>, float> DirMultSampledf;

// ---------------------------------------------------------------------------


template<class Disc, typename T>
DirSampled<Disc,T>::DirSampled(const Dir<Disc,T>& dir)
  : dir0_(dir), disc_(dir0_.sample()), count_(0)
{};

template<class Disc, typename T>
DirSampled<Disc,T>::~DirSampled()
{};

template<class Disc, typename T>
BaseMeasure<T>* DirSampled<Disc,T>::copy()
{
  DirSampled<Disc,T>* dirSampled = new DirSampled<Disc,T>(dir0_);
  dirSampled->disc_ = disc_;
  return dirSampled;
};

template<class Disc, typename T>
DirSampled<Disc,T>* DirSampled<Disc,T>::copyNative()
{
  DirSampled<Disc,T>* dirSampled = new DirSampled<Disc,T>(dir0_);
  dirSampled->disc_ = disc_;
  return dirSampled;
};


template<class Disc, typename T>
T DirSampled<Disc,T>::logLikelihood(const Matrix<T,Dynamic,1>& x) const
{
//  disc_.print();
  T logLike = disc_.logPdf(x);
//  cout<<x.transpose()<<" -> " <<logLike<<endl;
//  cout<<x.transpose()<<" -> " <<disc_.logPdfSlower(x)<<endl;
  return logLike;
};

template<class Disc, typename T>
T DirSampled<Disc,T>::logLikelihoodFromSS(const Matrix<T,Dynamic,1>& x) const
{
//  disc_.print();
  T logLike = disc_.logPdfOfSS(x);
//  cout<<x.transpose()<<" -> " <<logLike<<endl;
//  cout<<x.transpose()<<" -> " <<disc_.logPdfSlower(x)<<endl;
  return logLike;
};

template<class Disc, typename T>
void DirSampled<Disc,T>::posterior(const Matrix<T,Dynamic,Dynamic>& x, 
    const VectorXu& z, uint32_t k)
{
  count_ = 0;
  for(uint32_t i=0; i<z.size(); ++i) if(z(i)==k) ++count_;
  disc_ = dir0_.posterior(x,z,k).sample();
};

template<class Disc, typename T>
void DirSampled<Disc,T>::posteriorFromSS(const vector<Matrix<T,Dynamic,1> > & x, const VectorXu& z, uint32_t k)
{
  disc_ = dir0_.posteriorFromCounts(x,z,k).sample();
};

template<class Disc, typename T>
void DirSampled<Disc,T>::posteriorFromSS(const Matrix<T,Dynamic,1> &x)
{
	disc_ = dir0_.posteriorFromCounts(x).sample();
}

template<class Disc, typename T>
T DirSampled<Disc,T>::logPdfUnderPrior() const
{
  return dir0_.logPdf(disc_);
};

template<class Disc, typename T>
T DirSampled<Disc,T>::logPdfUnderPriorMarginalized() const
{
  // evaluates log pdf of sufficient statistics stored within dir0_
//  dir0_.print();
  return dir0_.logPdfMarginalized();
};

template<class Disc, typename T>
T DirSampled<Disc,T>::logPdfUnderPriorMarginalizedMerged(
    const boost::shared_ptr<DirSampled<Disc,T> >& other) const
{
  return dir0_.logPdfUnderPriorMarginalizedMerged(other->dir0_);
};

template<class Disc, typename T>
void DirSampled<Disc,T>::fromMerge(const DirSampled<Disc,T>& dirA, 
  const DirSampled<Disc,T>& dirB) 
{
  count_ = dirA.count() + dirB.count();
  dir0_.fromMerge(dirA.dir0_,dirB.dir0_);
  disc_ = dir0_.posterior().sample();
};

template<class Disc, typename T>
DirSampled<Disc,T>* DirSampled<Disc,T>::merge(const DirSampled<Disc,T>& other)
{
  DirSampled<Disc,T>* newNiw = this->copyNative();
  newNiw->dir0_.fromMerge(dir0_,other.dir0_);
  newNiw->sample();
  return newNiw;
};


template<class Disc, typename T>
void DirSampled<Disc,T>::sample() 
{
  disc_ = dir0_.posterior().sample();
};

template<class Disc, typename T>
void DirSampled<Disc,T>::print() const
{
  dir0_.posterior().print();
  disc_.print();
};
