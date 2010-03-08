#include <math.h>
#include "malloc.h"

#include "sample_wire.h"
#include "r250c/r250.h"


sampleWire::sampleWire( float x, float y, float z )  : sampleBase( x, y, z)
{
 bc[0] = CUT;
 bc[1] = CUT;
}

// look if we are within dr of a cluster
// dr == 0.0 means looking if we are inside the cluster
materialBase* sampleWire::lookupMaterial( float* pos ) 
{
  float x = ( pos[0] / w[0] ) * 2.0 - 1.0;
  float y = ( pos[1] / w[1] ) * 2.0 - 1.0;

  if( ( x*x + y*y ) >= 1.0 ) return 0;
  return material[0];
}
