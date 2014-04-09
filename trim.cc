#include <math.h>
#include <stdio.h>

#include "trim.h"
#include "simconf.h"

#include "functions.h"
#include <iostream>
using namespace std;

//#define RANGECORRECT2

//
// all energies are in eV
//

// does a single ion cascade
void trimBase::trim( ionBase *pka_, queue<ionBase*> &recoils )
{
  // simconf should already be initialized
  pka = pka_;

  // make recoil queue available in overloadable functions
  recoil_queue_ptr = &recoils;

  //e = pka.e;
  double pl = 0.0;
  double max = 0.0;
  double e0kev = pka->e / 1000.0;
  int ic = 0;
  int nn, ie;
  double r1, r2, hh;
  double eps, eeg, ls, p, b, r, see, dee;
  double s2, c2, ct, st;
  double rr, ex1, ex2, ex3, ex4, v ,v1;
  double fr, fr1, q, roc, sqe;
  double cc, aa, ff, co, delta;
  double den;
  double rdir[3], perp[3], norm, psi;

  double p1, p2;
  double range;

  // generate random number for use in the first loop iteration only!
  r1 = dr250();

  do // cycle for each collision
  {
    // increase loop counter
    ic++;

    // which material is the ion currently in?
    material = sample->lookupMaterial( pka->pos );
    if (material==0) break; // TODO: add flight through vacuum

    // normalize direction vector
    v_norm(pka->dir);

    // setup max. impact parameter
    eps =  pka->e * material->f;
    eeg = sqrtf( eps * material->epsdg ); // [TRI02450]
    material->pmax = material->a / ( eeg + sqrtf( eeg ) + 0.125 * pow( eeg, 0.1 ) );

    ls = 1.0 / ( M_PI * pow(material->pmax, 2.0) * material->arho );
    if (ic==1) ls = r1 * fmin(ls, simconf->cw);

    // correct for maximum available range in current material by increasing maximum impact parameter
    #ifdef RANGECORRECT
    range = sample->rangeMaterial( pka->pos, pka->dir );
    if (range<ls)
    {
      /* cout << "range=" << range << " ls=" << ls
            << " pos[0]=" << pka->pos[0] << " dir[0]=" << pka->dir[0] << endl;
      cout << "CC " << pka->pos[0] << ' ' << pka->pos[1] << endl;
      cout << "CC " << pka->pos[0] + pka->dir[0] * range << ' ' << pka->pos[1] + pka->dir[1] * range << endl;
      cout << "CC " << endl;*/

      ls = range;

      // correct pmax to correspond with new ls
      material->pmax = 1.0 / sqrtf(  M_PI * ls * material->arho );
    }
    #endif

    // correct for maximum available range in current material by dropping recoils randomly (faster)
    #ifdef RANGECORRECT2
    range = sample->rangeMaterial( pka->pos, pka->dir );
    if( range < ls )
    {
      // skip this recoil, just advance the ion
      if( range/ls < dr250() )
      {
        // electronic stopping
        pka->e -= range * material->getrstop( pka );

        // free flight
        for( int i = 0; i < 3; i++ )
          pka->pos[i] += pka->dir[i] * range;

        // start over
        continue;
      }
      ls = range;
    }
    #endif

    // advance clock pathlength/velocity
    pka->t += 10.1811859 * ( ls - simconf->tau ) / sqrt( 2.0 * pka->e / pka->m1 );

    // time in fs! m in u, l in Ang, e in eV
    // 1000g/kg, 6.022e23/mol, 1.602e-19J/eV, 1e5m/s=1Ang/fs 1.0/0.09822038
    //printf( "se %d  %f [eV]  %f [keV/nm]  %f [nm]\n", pka->id, pka->e, see/100.0, pl/10.0 );

    // choose impact parameter
    r2 = dr250();
    p = material->pmax * sqrtf(r2);

    // which atom in the material will be hit
    hh = dr250(); // selects element inside material to scatter from
    for( nn = 0; nn < material->element.size(); nn++ )
    {
      hh -= material->element[nn]->t;
      if( hh <= 0 ) break;
    }
    element = material->element[nn];

    // epsilon and reduced impact parameter b
    eps = element->fi * pka->e;
    b = p / element->ai;

    ////ie = int( pka.e / e0kev - 0.5 ); // was +0.5 for fortran indices
    //ie = int( pka.e / material->semax - 0.5 ); // was +0.5 for fortran indices
    //see = material->se[ie];

    see = material->getrstop( pka );
    //if( pka.e < e0kev ) see = material->se[0] * sqrtf( pka.e / e0kev );
    dee = ls * see;

    if (eps>10.0)
    {
      // use rutherford scattering
      s2 = 1.0 / ( 1.0 + ( 1.0 + b * ( 1.0 + b ) ) * pow( 2.0 * eps * b , 2.0 ) );
      c2 = 1.0 - s2;
      ct = 2.0 * c2 - 1.0;
      st = sqrtf( 1.0 - ct*ct );
    }
    else
    {
      // first guess at ion c.p.a. [TRI02780]
      r = b;
      rr = -2.7 * logf( eps * b );
      if( rr >= b )
      {
        r = rr;
        rr = -2.7 * logf( eps * rr );
        if( rr >= b ) r = rr;
      }

      do
      {
        // universal potential
        ex1 = 0.18175 * exp( -3.1998 * r );
        ex2 = 0.50986 * exp( -0.94229 * r );
        ex3 = 0.28022 * exp( -0.4029 * r );
        ex4 = 0.028171 * exp( -0.20162 * r );
        v = ( ex1 + ex2 + ex3 + ex4 ) / r;
        v1 = -( v + 3.1998 *ex1 + 0.94229 * ex2 + 0.4029 * ex3 + 0.20162 * ex4 ) / r;

        fr = b*b / r + v * r / eps -r;
        fr1 = - b*b / ( r*r ) + ( v + v1 * r ) / eps - 1.0;
        q = fr / fr1;
        r -= q;
      } while (fabs(q/r) > 0.001); // [TRI03110]

      roc = -2.0 * ( eps - v ) / v1;
      sqe = sqrtf( eps );

      // 5-parameter magic scattering calculation (universal pot.)
      cc = ( 0.011615 + sqe ) / ( 0.0071222 + sqe );               // 2-87 beta
      aa = 2.0 * eps * ( 1.0 + ( 0.99229 / sqe ) ) * pow( b, cc ); // 2-87 A
      ff = ( sqrtf( aa*aa + 1.0 ) - aa ) * ( ( 9.3066 + eps ) / ( 14.813 + eps ) );

      delta = ( r - b ) * aa * ff / ( ff + 1.0 );
      co = ( b + delta + roc ) / ( r + roc );
      c2 = co*co;
      s2 = 1.0 - c2;
      //printf("nonrf\n");
      ct = 2.0 * c2 - 1.0;
      st = sqrtf( 1.0 - ct*ct );
    } // end non-rutherford scattering

    // energy transferred to recoil atom
    den = element->ec * s2 * pka->e;

    pka->e -= dee; // electronic energy loss
    if( pka->e < 0.0 && den > 100.0 )
      fprintf( stderr, " electronic energy loss stopped the ion. Broken recoil!!\n" );

    p1 = sqrtf(2.0 * pka->m1 * pka->e); // momentum before collision
    pka->e -= den;
    if (pka->e<0.0) pka->e = 0.0;
    p2 = sqrtf(2.0 * pka->m1 * pka->e); // momentum after collision

    // track maximum electronic energy loss TODO: might want to track max(see)!
    if (dee>max) max = dee;

    // total path lenght
    pl += ls - simconf->tau;

    // find new position, save old direction to recoil
    recoil = pka->spawnRecoil();
    for( int i = 0; i < 3; i++ )
    {
      // used to assign the new position to the recoil, but
      // we have to make sure the recoil starts in the appropriate material!
      pka->pos[i] += pka->dir[i] * ( ls - simconf->tau );
      recoil->dir[i] = pka->dir[i] * p1;
    }
    recoil->e = den;

    // recoil loses the lattice binding energy
    recoil->e -= element->Elbind;
    recoil->m1 = element->m;
    recoil->z1 = element->z;

    // create a random vector perpendicular to pka.dir
    // there is a cleverer way by using the azimuthal angle of scatter...
    do
    {
      for( int i = 0; i < 3; i++ ) rdir[i] = dr250() - 0.5;
      v_cross( pka->dir, rdir, perp );
      norm = sqrtf( v_dot( perp, perp) );
    }
    while( norm == 0.0 );
    v_scale( perp, 1.0 / norm );

    psi = atan( st / ( ct + element->my ) );
    v_scale( pka->dir, cos( psi ) );

    // calculate new direction, subtract from old dir (stored in recoil)
    for( int i = 0; i < 3; i++ )
    {
      pka->dir[i] += perp[i] * sin( psi );
      recoil->dir[i] -= pka->dir[i] * p2;
    }

    // end cascade if a CUT boundary is crossed
    for (int i = 0; i < 3; i++) {
      if ( sample->bc[i]==sampleBase::CUT &&
           (pka->pos[i]>sample->w[i] || pka->pos[i]<0.0) ) {
        pka->state = ionBase::LOST;
        break;
      }
    }

    // put the recoil on the stack
    if (pka->state!=ionBase::LOST)
    {
      if(recoil->e > element->Edisp)
      {
        if (followRecoil()) {
          v_norm( recoil->dir );
          recoil->tag = material->tag;
          recoil->id = simconf->id++;

          recoils.push(recoil);
          if( simconf->fullTraj ) printf( "spawn %d %d\n", recoil->id, pka->id );
        }
        else delete recoil;

        // if recoil exceeds displacement energy assume a vacancy was created
        // (we compare both ions agains the Edisp of the recoil atom!)
        if (pka->e > element->Edisp) {
          vacancyCreation();
        } else {
          if (pka->z1 == element->z)
            pka->state = ionBase::REPLACEMENT;
          else
            pka->state = ionBase::SUBSTITUTIONAL;
        }
      }
      else
      {
        delete recoil;
        if (pka->e < pka->ef) {
          pka->state = ionBase::INTERSTITIAL;
        }
      }
    }

    checkPKAState();

    // output the full trajectory TODO: the ion object should output itself!
    // TODO: and this shou;d be done in checkPKSState()
    if (simconf->fullTraj) {
      printf(
        "cont %f %f %f %d %d %d\n",
        pka->pos[0], pka->pos[1], pka->pos[2],
        pka->z1, pka->tag, pka->id
      );
    }

  } while (pka->state==ionBase::MOVING);
}

void trimBase::vacancyCreation()
{
  simconf->vacancies_created++;

  /*
  // modified Kinchin-Pease
  if( recoil->gen == 1 )
  {
    // calculate modified kinchin pease data http://www.iue.tuwien.ac.at/phd/hoessinger/node47.html
    double ed = 0.0115 * pow( material->az, -7.0/3.0) * recoil->e;
    double g = 3.4008 * pow( ed, 1.0/6.0 ) + 0.40244 * pow( ed, 3.0/4.0 ) + ed;
    double kd = 0.1337 * pow( material->az, 2.0/3.0 ) / pow( material->am, 0.5); //Z,M
    double Ev = recoil->e / ( 1.0 + kd * g );
    simconf->KP_vacancies += 0.8 * Ev / ( 2.0 * element->Edisp );
    // should be something like material->Edisp (average?)
  }
  */
}


/*
materialBase* sampleType::lookupLayer( const double* pos )
{
  double dif[3];

  dif[0] = pos[0] - 100.0;
  dif[1] = pos[1];
  dif[2] = pos[2];
  double r2 = v_dot( dif, dif );
  if( r2 < 2500.0 ) // r<50.0
    return material[1];
  else
    return material[0];
}
*/
