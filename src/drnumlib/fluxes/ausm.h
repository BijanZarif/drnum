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
#ifndef AUSM_H
#define AUSM_H

#include "fluxes/compressibleflux.h"
#include "cartesianpatch.h"

template <typename TReconstruction, typename TGas>
class Ausm : public CompressibleFlux
{

  TReconstruction* m_Reconstruction;

public: // methods

  Ausm(TReconstruction* reconstruction) { m_Reconstruction = reconstruction; }

  void xField(CartesianPatch *patch,
              size_t i, size_t j, size_t k,
              real x, real y, real z,
              real A, real* flux);
  void yField(CartesianPatch *patch,
              size_t i, size_t j, size_t k,
              real x, real y, real z,
              real A, real* flux);
  void zField(CartesianPatch *patch,
              size_t i, size_t j, size_t k,
              real x, real y, real z,
              real A, real* flux);


};


template <typename TReconstruction, typename TGas>
void Ausm<TReconstruction, TGas>::xField(CartesianPatch *patch,
                                         size_t i, size_t j, size_t k,
                                         real x, real y, real z,
                                         real A, real* flux)
{
  COMPRESSIBLE_LEFT_PROJX;
  COMPRESSIBLE_RIGHT_PROJX;

  real a   = 0.5*(a_l + a_r);
  real u   = 0.5*(u_l + u_r);
  real M   = u/a;
  real k_l = max(0.0, min(1.0, 0.5*(M + 1)));
  real k_r = 1 - k_l;
  real p   = k_l*p_l + k_r*p_r;
  real Vf  = u*A;
  countFlops(12);

  if (M > 0) {
    flux[0] += Vf*r_l;
    flux[1] += Vf*ru_l;
    flux[2] += Vf*rv_l;
    flux[3] += Vf*rw_l;
    flux[4] += Vf*r_l*H_l;
  } else {
    flux[0] += Vf*r_r;
    flux[1] += Vf*ru_r;
    flux[2] += Vf*rv_r;
    flux[3] += Vf*rw_r;
    flux[4] += Vf*r_r*H_r;
  }
  flux[1] += A*p;
  countFlops(11);
}

template <typename TReconstruction, typename TGas>
void Ausm<TReconstruction, TGas>::yField(CartesianPatch *patch,
                                         size_t i, size_t j, size_t k,
                                         real x, real y, real z,
                                         real A, real* flux)
{
  COMPRESSIBLE_LEFT_PROJY;
  COMPRESSIBLE_RIGHT_PROJY;

  real a   = 0.5*(a_l + a_r);
  real v   = 0.5*(v_l + v_r);
  real M   = v/a;
  real k_l = min(0.0, max(1.0, 0.5*(M + 1)));
  real k_r = 1 - k_l;
  real p   = k_l*p_l + k_r*p_r;
  real Vf  = v*A;
  countFlops(12);

  if (M > 0) {
    flux[0] += Vf*r_l;
    flux[1] += Vf*ru_l;
    flux[2] += Vf*rv_l;
    flux[3] += Vf*rw_l;
    flux[4] += Vf*r_l*H_l;
  } else {
    flux[0] += Vf*r_r;
    flux[1] += Vf*ru_r;
    flux[2] += Vf*rv_r;
    flux[3] += Vf*rw_r;
    flux[4] += Vf*r_r*H_r;
  }
  flux[2] += A*p;
  countFlops(11);
}

template <typename TReconstruction, typename TGas>
void Ausm<TReconstruction, TGas>::zField(CartesianPatch *patch,
                                         size_t i, size_t j, size_t k,
                                         real x, real y, real z,
                                         real A, real* flux)
{
  COMPRESSIBLE_LEFT_PROJZ;
  COMPRESSIBLE_RIGHT_PROJZ;

  real a   = 0.5*(a_l + a_r);
  real w   = 0.5*(w_l + w_r);
  real M   = w/a;
  real k_l = min(0.0, max(1.0, 0.5*(M + 1)));
  real k_r = 1 - k_l;
  real p   = k_l*p_l + k_r*p_r;
  real Vf  = w*A;
  countFlops(12);

  if (M > 0) {
    flux[0] += Vf*r_l;
    flux[1] += Vf*ru_l;
    flux[2] += Vf*rv_l;
    flux[3] += Vf*rw_l;
    flux[4] += Vf*r_l*H_l;
  } else {
    flux[0] += Vf*r_r;
    flux[1] += Vf*ru_r;
    flux[2] += Vf*rv_r;
    flux[3] += Vf*rw_r;
    flux[4] += Vf*r_r*H_r;
  }
  flux[3] += A*p;
  countFlops(11);
}

#endif // AUSM_H
