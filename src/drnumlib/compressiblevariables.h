// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// +                                                                      +
// + This file is part of DrNUM.                                          +
// +                                                                      +
// + Copyright 2013 numrax GmbH, enGits GmbH                              +
// +                                                                      +
// + DrNUM is free software: you can redistribute it and/or modify        +
// + it under the terms of the GNU General Public License as published by +
// + the Free Software Foundation, either version 3 of the License, or    +
// + (at your option) any later version.                                  +
// +                                                                      +
// + DrNUM is distributed in the hope that it will be useful,             +
// + but WITHOUT ANY WARRANTY; without even the implied warranty of       +
// + MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        +
// + GNU General Public License for more details.                         +
// +                                                                      +
// + You should have received a copy of the GNU General Public License    +
// + along with DrNUM. If not, see <http://www.gnu.org/licenses/>.        +
// +                                                                      +
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#ifndef COMPRESSIBLEVARIABLES_H
#define COMPRESSIBLEVARIABLES_H

#include "drnum.h"
#include "postprocessingvariables.h"

#include <QString>

template <typename TGas>
class CompressibleVariables : public PostProcessingVariables
{

  TGas m_Gas;
  real m_RefPressure;
  real m_RefTemperature;

public:

  CompressibleVariables();

  virtual int numScalars() const { return 5; }
  virtual int numVectors() const { return 1; }

  void setReferenceTemperature(real T) { m_RefTemperature = T; }
  void setReferencePressure(real p)    { m_RefPressure = p; }

  virtual string getScalarName(int i) const;
  virtual string getVectorName(int i) const;
  virtual real   getScalar(int i, real* var, vec3_t) const;
  virtual vec3_t getVector(int i, real* var, vec3_t) const;

};

template <typename TGas>
CompressibleVariables<TGas>::CompressibleVariables()
{
  m_RefPressure = 1e5;
  m_RefTemperature = 300;
}

template <typename TGas>
string CompressibleVariables<TGas>::getScalarName(int i) const
{
  if (i == 0) return "Ma";
  if (i == 1) return "p";
  if (i == 2) return "T";
  if (i == 3) return "rho";
  if (i == 4) return "S";
  BUG;
  return "N/A";
}

template <typename TGas>
string CompressibleVariables<TGas>::getVectorName(int i) const
{
  if (i == 0) return "U";
  BUG;
  return "N/A";
}

template <typename TGas>
real CompressibleVariables<TGas>::getScalar(int i, real *var, vec3_t) const
{
  real p, T, u, v, w;
  m_Gas.conservativeToPrimitive(var, p, T, u, v, w);

  if (i == 0) return sqrt((u*u + v*v + w*w)/(m_Gas.gamma()*m_Gas.R()*T));
  if (i == 1) return p;
  if (i == 2) return T;
  if (i == 3) return var[0];
  if (i == 4) return TGas::cp(var)*log(T/m_RefTemperature) - TGas::R(var)*log(p/m_RefPressure);
  BUG;
  return 0;
}

template <typename TGas>
vec3_t CompressibleVariables<TGas>::getVector(int i, real *var, vec3_t) const
{
  real p, T, u, v, w;
  m_Gas.conservativeToPrimitive(var, p, T, u, v, w);

  if (i == 0) return vec3_t(u, v, w);
  BUG;
  return vec3_t(0,0,0);
}

#endif // COMPRESSIBLEVARIABLES_H
