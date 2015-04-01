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

#ifndef GPU_CARTESIANLEVELSETBC_H
#define GPU_CARTESIANLEVELSETBC_H

template <unsigned int DIM, typename LS, typename BC>
class GPU_CartesianLevelSetBC;

#include "gpu_levelsetbc.h"

#include "drnum.h"
#include "genericoperation.h"
#include "gpu_cartesianpatch.h"

template <unsigned int DIM, typename LS, typename BC>
class GPU_CartesianLevelSetBC : public GPU_LevelSetBC<DIM, CartesianPatch, GPU_CartesianPatch>
{

protected: // attributes


public: // methods

  GPU_CartesianLevelSetBC(PatchGrid* patch_grid, int cuda_device = 0, size_t thread_limit = 0);

  CUDA_DO static void  grad(GPU_CartesianPatch &patch, size_t i, size_t j, size_t k, real &gx, real &gy, real &gz);

  CUDA_HO virtual void operator()();

};


template <unsigned int DIM, typename LS, typename BC>
void GPU_CartesianLevelSetBC<DIM,LS,BC>::grad(GPU_CartesianPatch &patch, size_t i, size_t j, size_t k, real &gx, real &gy, real &gz)
{
  gx = 0.5*patch.idx()*(LS::G(patch, i+1, j, k) - LS::G(patch, i-1, j, k));
  gy = 0.5*patch.idy()*(LS::G(patch, i, j+1, k) - LS::G(patch, i, j+1, k));
  gz = 0.5*patch.idz()*(LS::G(patch, i, j, k+1) - LS::G(patch, i, j, k+1));
}

template <unsigned int DIM, typename LS, typename BC>
GPU_CartesianLevelSetBC<DIM,LS,BC>::GPU_CartesianLevelSetBC(PatchGrid* patch_grid, int cuda_device, size_t thread_limit)
  : GPU_LevelSetBC<DIM, CartesianPatch, GPU_CartesianPatch>(patch_grid, cuda_device, thread_limit)
{
}



template <unsigned int DIM, typename LS, typename BC>
__global__ void GPU_CartesianLevelSetBC_kernelOperate(GPU_CartesianPatch patch)
{
  size_t i = 1 + blockIdx.x;
  size_t j = 1 + blockIdx.y;
  size_t k = 1 + threadIdx.x;

  dim_t<DIM> dim;

  if (i >= (patch.sizeI() - 1) || j >= (patch.sizeJ() - 1) || k >= (patch.sizeK() - 1)) {
    return;
  }

  if (LS::G(patch, i, j, k) < 0) {

    real var_neigh[DIM], var[DIM];
    for (size_t i_var = 0; i_var < DIM; ++i_var) {
      var[i_var] = 0.0;
    }

    real gx_c, gy_c, gz_c;
    int count = 0;
    real total_weight = 0;
    GPU_CartesianLevelSetBC<DIM,LS,BC>::grad(patch, i, j, k, gx_c, gy_c, gz_c);
    if (LS::G(patch, i+1, j, k) >= 0) {
      real gx     = patch.idx()*(LS::G(patch, i+1, j, k) - LS::G(patch, i, j, k));
      real weight = fabs(gx);
      BC::operateX(&patch, var_neigh, LS::G(patch, i+1, j, k), i, j, k, 1, gx, gy_c, gz_c);
      total_weight += weight;
      ++count;
      for (size_t i_var = 0; i_var < DIM; ++i_var) {
        var[i_var] += weight*var_neigh[i_var];
      }
    }
    if (LS::G(patch, i-1, j, k) >= 0) {
      real gx     = patch.idx()*(LS::G(patch, i, j, k) - LS::G(patch, i-1, j, k));
      real weight = fabs(gx);
      BC::operateX(&patch, var_neigh, LS::G(patch, i-1, j, k), i, j, k, -1, gx, gy_c, gz_c);
      total_weight += weight;
      ++count;
      for (size_t i_var = 0; i_var < DIM; ++i_var) {
        var[i_var] += weight*var_neigh[i_var];
      }
    }
    if (LS::G(patch, i, j+1, k) >= 0) {
      real gy     = patch.idy()*(LS::G(patch, i, j+1, k) - LS::G(patch, i, j, k));
      real weight = fabs(gy);
      BC::operateY(&patch, var_neigh, LS::G(patch, i, j+1, k), i, j, k, 1, gx_c, gy, gz_c);
      total_weight += weight;
      ++count;
      for (size_t i_var = 0; i_var < DIM; ++i_var) {
        var[i_var] += weight*var_neigh[i_var];
      }
    }
    if (LS::G(patch, i, j-1, k) >= 0) {
      real gy     = patch.idy()*(LS::G(patch, i, j, k) - LS::G(patch, i, j-1, k));
      real weight = fabs(gy);
      BC::operateY(&patch, var_neigh, LS::G(patch, i, j-1, k), i, j, k, -1, gx_c, gy, gz_c);
      total_weight += weight;
      ++count;
      for (size_t i_var = 0; i_var < DIM; ++i_var) {
        var[i_var] += weight*var_neigh[i_var];
      }
    }
    if (LS::G(patch, i, j, k+1) >= 0) {
      real gz     = patch.idz()*(LS::G(patch, i, j, k+1) - LS::G(patch, i, j, k));
      real weight = fabs(gz);
      BC::operateZ(&patch, var_neigh, LS::G(patch, i, j, k+1), i, j, k, 1, gx_c, gy_c, gz);
      total_weight += weight;
      ++count;
      for (size_t i_var = 0; i_var < DIM; ++i_var) {
        var[i_var] += weight*var_neigh[i_var];
      }
    }
    if (LS::G(patch, i, j, k-1) >= 0) {
      real gz     = patch.idz()*(LS::G(patch, i, j, k) - LS::G(patch, i, j, k-1));
      real weight = fabs(gz);
      BC::operateZ(&patch, var_neigh, LS::G(patch, i, j, k-1), i, j, k, -1, gx_c, gy_c, gz);
      total_weight += weight;
      ++count;
      for (size_t i_var = 0; i_var < DIM; ++i_var) {
        var[i_var] += weight*var_neigh[i_var];
      }
    }

    if (count > 0) {
      for (size_t i_var = 0; i_var < DIM; ++i_var) {
        var[i_var] /= total_weight;
      }
      real G_self = LS::G(patch, i, j, k);
      patch.setVar(dim, 0, i, j, k, var);
      LS::updateG(patch, i, j, k, G_self);
    }

  }
}

template <unsigned int DIM, typename LS, typename BC>
__global__ void GPU_CartesianLevelSetBC_kernelExtrapolate(GPU_CartesianPatch patch)
{
  size_t i = 1 + blockIdx.x;
  size_t j = 1 + blockIdx.y;
  size_t k = 1 + threadIdx.x;

  dim_t<DIM> dim;

  if (i >= (patch.sizeI() - 1) || j >= (patch.sizeJ() - 1) || k >= (patch.sizeK() - 1)) {
    return;
  }

  real G_self = LS::G(patch, i, j, k);

  if (G_self < 0) {

    real var_neigh[DIM], var[DIM];
    for (size_t i_var = 0; i_var < DIM; ++i_var) {
      var[i_var] = 0.0;
    }

    size_t idx = patch.index(i, j, k);
    real gx_c, gy_c, gz_c;
    int count = 0;
    real total_weight = 0;
    GPU_CartesianLevelSetBC<DIM,LS,BC>::grad(patch, i, j, k, gx_c, gy_c, gz_c);
    if (LS::G(patch, i+1, j, k) < 0 && LS::G(patch, i+1, j, k) > G_self) {
      real   gx     = patch.idx()*(LS::G(patch, i+1, j, k) - LS::G(patch, i, j, k));
      real   weight = fabs(gx);
      total_weight += weight;
      patch.getVar(dim, 0, i+1, j, k, var_neigh);
      ++count;
      for (size_t i_var = 0; i_var < DIM; ++i_var) {
        var[i_var] += weight*var_neigh[i_var];
      }
    }
    if (LS::G(patch, i-1, j, k) < 0 && LS::G(patch, i-1, j, k) > G_self) {
      real   gx     = patch.idx()*(LS::G(patch, i, j, k) - LS::G(patch, i-1, j, k));
      real   weight = fabs(gx);
      total_weight += weight;
      patch.getVar(dim, 0, i-1, j, k, var_neigh);
      ++count;
      for (size_t i_var = 0; i_var < DIM; ++i_var) {
        var[i_var] += weight*var_neigh[i_var];
      }
    }
    if (LS::G(patch, i, j+1, k) < 0 && LS::G(patch, i, j+1, k) > G_self) {
      real   gy     = patch.idy()*(LS::G(patch, i, j+1, k) - LS::G(patch, i, j, k));
      real   weight = fabs(gy);
      total_weight += weight;
      patch.getVar(dim, 0, i, j+1, k, var_neigh);
      ++count;
      for (size_t i_var = 0; i_var < DIM; ++i_var) {
        var[i_var] += weight*var_neigh[i_var];
      }
    }
    if (LS::G(patch, i, j-1, k) < 0 && LS::G(patch, i, j-1, k) > G_self) {
      real   gy     = patch.idy()*(LS::G(patch, i, j, k) - LS::G(patch, i, j-1, k));
      real   weight = fabs(gy);
      total_weight += weight;
      patch.getVar(dim, 0, i, j-1, k, var_neigh);
      ++count;
      for (size_t i_var = 0; i_var < DIM; ++i_var) {
        var[i_var] += weight*var_neigh[i_var];
      }
    }
    if (LS::G(patch, i, j, k+1) < 0 && LS::G(patch, i, j, k+1) > G_self) {
      real   gz     = patch.idz()*(LS::G(patch, i, j, k+1) - LS::G(patch, i, j, k));
      real   weight = fabs(gz);
      total_weight += weight;
      patch.getVar(dim, 0, i, j, k+1, var_neigh);
      ++count;
      for (size_t i_var = 0; i_var < DIM; ++i_var) {
        var[i_var] += weight*var_neigh[i_var];
      }
    }
    if (LS::G(patch, i, j, k-1) < 0 && LS::G(patch, i, j, k-1) > G_self) {
      real   gz     = patch.idz()*(LS::G(patch, i, j, k) - LS::G(patch, i, j, k-1));
      real   weight = fabs(gz);
      total_weight += weight;
      patch.getVar(dim, 0, i, j, k-1, var_neigh);
      ++count;
      for (size_t i_var = 0; i_var < DIM; ++i_var) {
        var[i_var] += weight*var_neigh[i_var];
      }
    }

    if (count > 0) {
      for (size_t i_var = 0; i_var < DIM; ++i_var) {
        var[i_var] /= total_weight;
      }
      real G_self = LS::G(patch, i, j, k);
      patch.setVar(dim, 2, i, j, k, var);
      LS::updateG(patch, i, j, k, G_self, 2);
    }

  }
}

template <unsigned int DIM, typename LS, typename BC>
__global__ void GPU_CartesianLevelSetBC_kernelPre(GPU_CartesianPatch patch)
{
  size_t i = 1 + blockIdx.x;
  size_t j = 1 + blockIdx.y;
  size_t k = 1 + threadIdx.x;

  dim_t<DIM> dim;

  if (i >= (patch.sizeI() - 1) || j >= (patch.sizeJ() - 1) || k >= (patch.sizeK() - 1)) {
    return;
  }

  if (LS::G(patch, i, j, k) >= 0) {

    real var[DIM];
    for (size_t i_var = 0; i_var < DIM; ++i_var) {
      var[i_var] = 0.0;
    }

    real gx, gy, gz;
    bool crossover = false;
    GPU_CartesianLevelSetBC<DIM,LS,BC>::grad(patch, i, j, k, gx, gy, gz);
    if      (LS::G(patch, i+1, j, k) < 0) crossover = true;
    else if (LS::G(patch, i-1, j, k) < 0) crossover = true;
    else if (LS::G(patch, i, j+1, k) < 0) crossover = true;
    else if (LS::G(patch, i, j-1, k) < 0) crossover = true;
    else if (LS::G(patch, i, j, k+1) < 0) crossover = true;
    else if (LS::G(patch, i, j, k-1) < 0) crossover = true;

    if (crossover) {
      BC::pre(&patch, var, i, j, k, gx, gy, gz);
      patch.setVar(dim, 0, i, j, k, var);
    }

  }
}

template <unsigned int DIM, typename LS, typename BC>
__global__ void GPU_CartesianLevelSetBC_kernelPost(GPU_CartesianPatch patch)
{
  size_t i = 1 + blockIdx.x;
  size_t j = 1 + blockIdx.y;
  size_t k = 1 + threadIdx.x;

  dim_t<DIM> dim;

  if (i >= (patch.sizeI() - 1) || j >= (patch.sizeJ() - 1) || k >= (patch.sizeK() - 1)) {
    return;
  }

  if (LS::G(patch, i, j, k) >= 0) {

    real var[DIM];
    for (size_t i_var = 0; i_var < DIM; ++i_var) {
      var[i_var] = 0.0;
    }

    real gx, gy, gz;
    bool crossover = false;
    GPU_CartesianLevelSetBC<DIM,LS,BC>::grad(patch, i, j, k, gx, gy, gz);
    if      (LS::G(patch, i+1, j, k) < 0) crossover = true;
    else if (LS::G(patch, i-1, j, k) < 0) crossover = true;
    else if (LS::G(patch, i, j+1, k) < 0) crossover = true;
    else if (LS::G(patch, i, j-1, k) < 0) crossover = true;
    else if (LS::G(patch, i, j, k+1) < 0) crossover = true;
    else if (LS::G(patch, i, j, k-1) < 0) crossover = true;

    if (crossover) {
      BC::post(&patch, var, i, j, k, gx, gy, gz);
      patch.setVar(dim, 0, i, j, k, var);
    }

  }
}

template <unsigned int DIM, typename LS, typename BC>
void GPU_CartesianLevelSetBC<DIM,LS,BC>::operator()()
{
  cudaDeviceSetCacheConfig(cudaFuncCachePreferL1);
  CUDA_CHECK_ERROR;

  //GPU_LevelSetBC<DIM,CartesianPatch,GPU_CartesianPatch,BC>::copyField(0, 2);
  this->copyField(0, 2);
  cudaDeviceSynchronize();

  for (size_t i_patch = 0; i_patch < this->m_Patches.size(); ++i_patch) {

    CUDA_CHECK_ERROR;

    size_t max_num_threads = this->m_MaxNumThreads;
    size_t k_lines = max(size_t(1), size_t(max_num_threads/this->m_Patches[i_patch]->sizeK()));

    {
      dim3 blocks(this->m_Patches[i_patch]->sizeI(), this->m_Patches[i_patch]->sizeJ()/k_lines+1, 1);
      dim3 threads(this->m_Patches[i_patch]->sizeK(), k_lines, 1);

      if (BC::usePre()) {
        GPU_CartesianLevelSetBC_kernelPre<DIM,LS,BC> <<<blocks, threads>>>(this->m_GpuPatches[i_patch]);
        CUDA_CHECK_ERROR;
        cudaDeviceSynchronize();
      }

      GPU_CartesianLevelSetBC_kernelOperate<DIM,LS,BC> <<<blocks, threads>>>(this->m_GpuPatches[i_patch]);
      CUDA_CHECK_ERROR;
      cudaDeviceSynchronize();


      if (BC::usePost()) {
        GPU_CartesianLevelSetBC_kernelPost<DIM,LS,BC> <<<blocks, threads>>>(this->m_GpuPatches[i_patch]);
        CUDA_CHECK_ERROR;
        cudaDeviceSynchronize();
      }

      /*
      GPU_CartesianLevelSetBC_kernelExtrapolate<DIM,LS,BC> <<<blocks, threads>>>(this->m_GpuPatches[i_patch]);
      CUDA_CHECK_ERROR;
      cudaDeviceSynchronize();
      cudaMemcpy(this->m_GpuPatches[i_patch].getField(0), this->m_GpuPatches[i_patch].getField(2), this->m_GpuPatches[i_patch].fieldSize()*sizeof(real) ,cudaMemcpyDeviceToDevice);
      */
    }

  }
}

#endif // GPU_CARTESIANLEVELSETBC_H
