/*
MyTRIM - a three dimensional binary collision Monte Carlo library.
Copyright (C) 2008-2018  Daniel Schwen <daniel@schwen.de>

This library is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation; either version 2.1 of the
License, or (at your option) any later version.

This library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301 USA
*/

#ifndef MYTRIM_SAMPLE_BURRIED_WIRE_H
#define MYTRIM_SAMPLE_BURRIED_WIRE_H

#include "sample_wire.h"

namespace MyTRIM_NS
{

class SampleBurriedWire : public SampleWire
{
public:
  SampleBurriedWire(Real x = 10000.0, Real y = 10000.0, Real z = 10000.0);

  virtual MaterialBase * lookupMaterial(Point & pos);
};
}

#endif
