#ifndef TRIM_H
#define TRIM_H 1

#include <vector>
#include <queue>
#include <cmath>

#include "material.h"
#include "sample.h"
#include "simconf.h"

class trimBase {
public:
  void trim( ionBase *pka, queue<ionBase*> &recoils );
  trimBase( sampleBase *sample_ ) : sample( sample_ ) {};

protected:
  sampleBase *sample;
  ionBase *pka, *recoil;
  materialBase *material;
  elementBase *element;
  queue<ionBase*> *recoil_queue_ptr;
  bool terminate;

  // by default only follow recoils with E > 12eV
  virtual bool followRecoil() {
    // TODO: find a better place for this!
    /*if( pka->md > 0 )
      recoil->md = pka->md +1;
    else
      recoil->md = 0;
    */
    return true;
  };
  virtual void vacancyCreation();
  virtual void checkPKAState() {};
  virtual void dissipateRecoilEnergy() {};
};

// //
// // Do a breadth first rather than depth first recoil simulation
// //
// class trimBreadthFirst : public trimBase {
// public:
//   trimBreadthFirst( sampleBase *sample_ ) : trimBase( sample_ ) {};
// protected:
//   virtual bool followRecoil() {
//     recoil_queue_ptr->push(pka);
//     terminate = true;
//     return recoil->e > 12.0;
//   };
// };


//
// Only follow the primary knock ons (i.e. fission fragments)
//
class trimPrimaries : public trimBase {
public:
  trimPrimaries( sampleBase *sample_ ) : trimBase( sample_ ) {};
protected:
  virtual int maxGen() { return 1; };
  virtual bool followRecoil() { return (recoil->gen < maxGen()); };

  void vacancyCreation()
  {
    simconf->vacancies_created++;

    // Modified Kinchin-Pease
    if (recoil->gen == maxGen())
    {
      // calculate modified kinchin pease data
      // http://www.iue.tuwien.ac.at/phd/hoessinger/node47.html
      double ed = 0.0115 * pow( material->az, -7.0/3.0) * recoil->e;
      double g = 3.4008 * pow( ed, 1.0/6.0 ) + 0.40244 * pow( ed, 3.0/4.0 ) + ed;
      double kd = 0.1337 * pow( material->az, 2.0/3.0 ) / pow( material->am, 0.5); //Z,M
      double Ev = recoil->e / ( 1.0 + kd * g );
      simconf->vacancies_created += int(0.8 * Ev / (2.0*element->Edisp));

      // TODO: this is missing the energy threshold of 2.5Ed!!!!
      // TODO: should be something like material->Edisp (average?)
    }
  }
};


//
// Only follow the first generation of recoils
//
class trimRecoils : public trimPrimaries {
  public:
    trimRecoils( sampleBase *sample_ ) : trimPrimaries( sample_ ) {};
  protected:
    virtual int maxGen() { return 2; };
};



//
// store a history of all recoils
//
class trimHistory : public trimBase {
public:
  trimHistory( sampleBase *sample_ ) : trimBase( sample_ ) {};
  vector<double> pos_hist[3];
protected:
  virtual bool followRecoil()
  {
    pos_hist[0].push_back(pka->pos[0]);
    pos_hist[1].push_back(pka->pos[1]);
    pos_hist[2].push_back(pka->pos[2]);
    return true;
  };
};



//
// Full damage cascade, plus log vaccancy creation
//
class trimDefectLog : public trimBase {
public:
  trimDefectLog(sampleBase *sample_, std::ostream &os_) : os(os_), trimBase(sample_) {};
protected:
  std::ostream &os;

  // ions being removed from lattice sites
  virtual void vacancyCreation() {
    os << "V " << *recoil << endl;
  };

  // ions coming to rest
  virtual void checkPKAState() {
    if (pka->state==ionBase::INTERSTITIAL)
      os << "I " << *pka << endl;
    else if (pka->state==ionBase::SUBSTITUTIONAL)
      os << "S " << *pka << endl;
    else if (pka->state==ionBase::REPLACEMENT)
      os << "R " << *pka << endl;
  };
};


//
// Full damage cascade, plus map vaccancy creation
//
class trimVacMap : public trimBase {
  static const int mx = 20, my = 20;
public:
  int vmap[mx][my][3];
  trimVacMap( sampleBase *sample_, int z1_, int z2_, int z3_ = -1 ) : trimBase( sample_ ), z1(z1_), z2(z2_), z3(z3_)
  {
    for( int e = 0; e < 3; e++ )
      for( int x = 0; x < mx; x++ )
        for( int y = 0; y < my; y++ )
          vmap[x][y][e] = 0;
  };
protected:
  int z1, z2, z3;
  virtual void vacancyCreation()
  {
    // both atoms have enough energy to leave the site
    int x, y;

    x = ( ( recoil->pos[0] * mx ) / sample->w[0] );
    y = ( ( recoil->pos[1] * my ) / sample->w[1] );
    x -= int(x/mx) * mx;
    y -= int(y/my) * my;

    // keep track of vaccancies for the two constituents
    if( recoil->z1 == z1 ) vmap[x][y][0]++;
    else if( recoil->z1 == z2 ) vmap[x][y][1]++;
    else if( recoil->z1 == z3 ) vmap[x][y][2]++;
  };
};


//
// Output all phonon energy losses
//
class trimPhononOut : public trimBase {
public:
  trimPhononOut(sampleBase *sample_,  std::ostream &os_) : os(os_), trimBase( sample_ ) {};
protected:
  std::ostream &os;

  // recoil atom is not leaving its site
  // (make sure it keeps its binding enrgy and dissipate emeining E)
  virtual void dissipateRecoilEnergy()
  {
    double Edep = recoil->e + element->Elbind;
    os << Edep << ' ' <<  *recoil << endl;
    simconf->EnucTotal += Edep;
  };

  // dissipate lattice binding energy where recoil branches off
  virtual bool followRecoil() {
    os << element->Elbind << ' ' <<  *recoil << endl;
    simconf->EnucTotal += element->Elbind;
    return true;
  }
};

#endif
