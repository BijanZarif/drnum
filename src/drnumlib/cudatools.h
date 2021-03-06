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
#ifndef CUDATOOLS_H
#define CUDATOOLS_H

#ifdef CUDA

#include "drnum.h"

#include <cuda.h>
#include <cuda_runtime_api.h>

#ifdef DEBUG
  #define CUDA_CHECK_ERROR                                     \
  {                                                            \
    cudaThreadSynchronize();                                   \
    cudaError_t err = cudaGetLastError();                      \
    if (err != cudaSuccess) {                                  \
      cerr << "\n" << cudaGetErrorString(err) << "\n" << endl; \
      cerr << "  file: " << __FILE__ << "\n";                  \
      cerr << "  line: " << __LINE__ << "\n";                  \
      abort();                                                 \
    }                                                          \
  }
#else
#define CUDA_CHECK_ERROR                                     \
{                                                            \
  cudaError_t err = cudaGetLastError();                      \
  if (err != cudaSuccess) {                                  \
    cerr << "\n" << cudaGetErrorString(err) << "\n" << endl; \
    cerr << "  file: " << __FILE__ << "\n";                  \
    cerr << "  line: " << __LINE__ << "\n";                  \
    abort();                                                 \
  }                                                          \
}
#endif

namespace CudaTools
{

inline void info()
{
  int count;
  if (cudaGetDeviceCount(&count) != cudaSuccess) {
    cerr << "error detecting CUDA devices" << endl;
    exit(EXIT_FAILURE);
  }
  if (count == 0) {
    cerr << "no CUDA devices found" << endl;
    exit(EXIT_FAILURE);
  }
  for (int i = 0; i < count; ++i) {
    cout << endl;
    cout << "device #" << i << ":" << endl;
    cout << "================================================" << endl;
    cudaDeviceProp prop;
    if (cudaGetDeviceProperties(&prop, i) != cudaSuccess) {
      cerr << "error fetching device properties" << endl;
      exit(EXIT_FAILURE);
    }
    cout << "name                   : " << prop.name << endl;
    cout << "warpSize               : " << prop.warpSize << endl;
    cout << "maxThreadsPerBlock     : " << prop.maxThreadsPerBlock << endl;
    cout << "maxThreadsDim[0]       : " << prop.maxThreadsDim[0] << endl;
    cout << "maxThreadsDim[1]       : " << prop.maxThreadsDim[1] << endl;
    cout << "maxThreadsDim[2]       : " << prop.maxThreadsDim[2] << endl;
    cout << "maxGridSize[0]         : " << prop.maxGridSize[0] << endl;
    cout << "maxGridSize[1]         : " << prop.maxGridSize[1] << endl;
    cout << "maxGridSize[2]         : " << prop.maxGridSize[2] << endl;
    cout << "compute capability     : " << prop.major << "." << prop.minor << endl;
    cout << "sharedMemPerBlock [kb] : " << double(prop.sharedMemPerBlock)/1024 << endl;
    cout << "totalGlobalMem [Mb]    : " << double(prop.totalGlobalMem)/(1024*1024) << endl;
    cout << endl;
  }
}

inline void checkError()
{
  cudaError_t err = cudaGetLastError();
  if (err != cudaSuccess) {
    info();
    cerr << "\n" << cudaGetErrorString(err) << "\n" << endl;
    BUG;
  }
}

}

#endif

#endif // CUDATOOLS_H
