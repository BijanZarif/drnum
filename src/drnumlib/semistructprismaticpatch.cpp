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
#include "cartesianpatch.h"

//#ifdef WITH_VTK
//#include <vtkSmartPointer.h>
//#include <vtkRectilinearGrid.h>
//#include <vtkXMLRectilinearGridWriter.h>
//#include <vtkFloatArray.h>
//#include <vtkCellData.h>
//#include <QVector>
//#endif


CartesianPatch::CartesianPatch(size_t num_protectlayers, size_t num_overlaplayers)
  : Patch(num_protectlayers, num_overlaplayers)
{
  m_mytypecode = 1001;

  m_NumI = 1;
  m_NumJ = 1;
  m_NumK = 1;
  //do allways with computeDeltas() m_Interpol_Initialized = false;
  /// @todo need a better eps-handling.
  m_Eps = 1.e-5;
  setNumProtectLayers(m_NumProtectLayers);
}

//bool CartesianPatch::readFromFile(ifstream &s_mesh)
//{
//  Patch::readFromFile(s_mesh);
//  // number of nodes in block axis directions
//  size_t num_i, num_j, num_k;
//  s_mesh >> num_i;
//  s_mesh >> num_j;
//  s_mesh >> num_k;
//  // protection exceptions
//  size_t numProtXmin, numProtXmax, numProtYmin, numProtYmax, numProtZmin, numProtZmax;
//  s_mesh >> numProtXmin;
//  s_mesh >> numProtXmax;
//  s_mesh >> numProtYmin;
//  s_mesh >> numProtYmax;
//  s_mesh >> numProtZmin;
//  s_mesh >> numProtZmax;
//  setNumProtectException(numProtXmin, numProtXmax, numProtYmin, numProtYmax, numProtZmin, numProtZmax);
//  // physical size of cartesian block
//  real ilength, jlength, klength;
//  s_mesh >> ilength;
//  s_mesh >> jlength;
//  s_mesh >> klength;
//  // scale length according to IO-scaling factor
//  ilength *= m_ioscale;
//  jlength *= m_ioscale;
//  klength *= m_ioscale;
//  // apply patch modifiers
//  resize(num_i, num_j, num_k);
//  setupMetrics(ilength, jlength, klength);
//}

bool CartesianPatch::readFromFile(istringstream iss_input)
{
  Patch::readFromFile(iss_input);
  // number of nodes in block axis directions
  size_t num_i, num_j, num_k;
  iss_input >> num_i;
  iss_input >> num_j;
  iss_input >> num_k;
  // protection exceptions
  size_t numProtXmin, numProtXmax, numProtYmin, numProtYmax, numProtZmin, numProtZmax;
  iss_input >> numProtXmin;
  iss_input >> numProtXmax;
  iss_input >> numProtYmin;
  iss_input >> numProtYmax;
  iss_input >> numProtZmin;
  iss_input >> numProtZmax;
  setNumProtectException(numProtXmin, numProtXmax, numProtYmin, numProtYmax, numProtZmin, numProtZmax);
  // physical size of cartesian block
  real ilength, jlength, klength;
  iss_input >> ilength;
  iss_input >> jlength;
  iss_input >> klength;
  // scale length according to IO-scaling factor
  ilength *= m_ioscale;
  jlength *= m_ioscale;
  klength *= m_ioscale;
  // apply patch modifiers
  resize(num_i, num_j, num_k);
  setupMetrics(ilength, jlength, klength);
}

bool CartesianPatch::writeToFile(ifstream &s_mesh)
{
}

void CartesianPatch::scaleRefParental(real scfactor)
{
  Patch::scaleRefParental(scfactor);
  m_Lx = scfactor * m_Lx;
  m_Ly = scfactor * m_Ly;
  m_Lz = scfactor * m_Lz;
  computeDeltas();
}

void CartesianPatch::setNumProtectLayers(size_t num_protectlayers)
{
  m_ProtectException = false;
  m_NumProtectLayers = num_protectlayers;
  m_numProtXmin = num_protectlayers;
  m_numProtXmax = num_protectlayers;
  m_numProtYmin = num_protectlayers;
  m_numProtYmax = num_protectlayers;
  m_numProtZmin = num_protectlayers;
  m_numProtZmax = num_protectlayers;
}

void CartesianPatch::setNumProtectException(const size_t& numProtXmin, const size_t& numProtXmax,
                                            const size_t& numProtYmin, const size_t& numProtYmax,
                                            const size_t& numProtZmin, const size_t& numProtZmax)
{
  m_ProtectException = true;
  m_numProtXmin = numProtXmin;
  m_numProtXmax = numProtXmax;
  m_numProtYmin = numProtYmin;
  m_numProtYmax = numProtYmax;
  m_numProtZmin = numProtZmin;
  m_numProtZmax = numProtZmax;
}

void CartesianPatch::computeDeltas()
{
  /// @todo delete this block
  //  m_Lx = sqrt(sqr(m_Uxo) + sqr(m_Uyo) + sqr(m_Uzo));
  //  m_Ly = sqrt(sqr(m_Vxo) + sqr(m_Vyo) + sqr(m_Vzo));
  //  m_Lz = sqrt(sqr(m_Wxo) + sqr(m_Wyo) + sqr(m_Wzo));
  //  countFlops(15);
  //  countSqrts(3);

  m_Dx = m_Lx/m_NumI;
  m_Dy = m_Ly/m_NumJ;
  m_Dz = m_Lz/m_NumK;
  countFlops(3);

  m_InvDx = 1.0/m_Dx;
  m_InvDy = 1.0/m_Dy;
  m_InvDz = 1.0/m_Dz;
  countFlops(3);

  m_xCCMin = 0.5 * m_Dx;
  m_yCCMin = 0.5 * m_Dy;
  m_zCCMin = 0.5 * m_Dz;
  m_xCCMax = m_Lx - 0.5 * m_Dx;
  m_yCCMax = m_Ly - 0.5 * m_Dy;
  m_zCCMax = m_Lz - 0.5 * m_Dz;
  countFlops(6);

  // Set interpolation responsibility box for CC data.
  // Attention: Allow interpolation in total region of cells
  //            in responibility domain (+ 0.) instead of (+0.5).
  //            Reason: Adaptivity. Precision actually handled via epsing.
  m_xCCInterMin = (m_numProtXmin + 0.) * m_Dx;
  m_yCCInterMin = (m_numProtYmin + 0.) * m_Dy;
  m_zCCInterMin = (m_numProtZmin + 0.) * m_Dz;
  m_xCCInterMax = m_Lx - (m_numProtXmax + 0.) * m_Dx;
  m_yCCInterMax = m_Ly - (m_numProtYmax + 0.) * m_Dy;
  m_zCCInterMax = m_Lz - (m_numProtZmax + 0.) * m_Dz;
  //
  //  m_xCCInterMin = (m_numProtXmin + 0.5) * m_Dx;
  //  m_yCCInterMin = (m_numProtYmin + 0.5) * m_Dy;
  //  m_zCCInterMin = (m_numProtZmin + 0.5) * m_Dz;
  //  m_xCCInterMax = m_Lx - (m_numProtXmax + 0.5) * m_Dx;
  //  m_yCCInterMax = m_Ly - (m_numProtYmax + 0.5) * m_Dy;
  //  m_zCCInterMax = m_Lz - (m_numProtZmax + 0.5) * m_Dz;
  //
  //  m_xCCInterMin = (m_NumProtectLayers + 0.5) * m_Dx;
  //  m_yCCInterMin = (m_NumProtectLayers + 0.5) * m_Dy;
  //  m_zCCInterMin = (m_NumProtectLayers + 0.5) * m_Dz;
  //  m_xCCInterMax = m_Lx - (m_NumProtectLayers + 0.5) * m_Dx;
  //  m_yCCInterMax = m_Ly - (m_NumProtectLayers + 0.5) * m_Dy;
  //  m_zCCInterMax = m_Lz - (m_NumProtectLayers + 0.5) * m_Dz;
  countFlops(6);

  m_EpsDX = m_Dx * m_Eps;
  m_EpsDY = m_Dy * m_Eps;
  m_EpsDZ = m_Dz * m_Eps;
  countFlops(3);
}

/// @todo Get rid of setupAligned, if setupMetrics works correctly
void CartesianPatch::setupAligned(real xo1, real yo1, real zo1, real xo2, real yo2, real zo2)
{

  /** @todo Use a more general alignment method with trafo-matrix and internal coord system.
   *  I assume these are co-aligned here. */

  cout << "WARNING: CartesianPatch::setupAligned to be omitted" << endl;

  m_Xo = xo1;
  m_Yo = yo1;
  m_Zo = zo1;

  m_Uxo = xo2 - xo1;
  m_Uyo = 0;
  m_Uzo = 0;
  countFlops(1);

  m_Vxo = 0;
  m_Vyo = yo2 - yo1;
  m_Vzo = 0;
  countFlops(1);

  m_Wxo = 0;
  m_Wyo = 0;
  m_Wzo = zo2 - zo1;
  countFlops(1);

  // position relative to inertial system
  vec3_t shift_inertial2this;
  shift_inertial2this[0] = -m_Xo;
  shift_inertial2this[1] = -m_Yo;
  shift_inertial2this[2] = -m_Zo;
  m_transformInertial2This.setVector(shift_inertial2this);
  /// @todo rotation NOT implemented!!!


  /// @todo following 5 lines copied here to get it gone from computeDeltas, as readFromFile(ifstream &s_mesh) gets length directly
  m_Lx = sqrt(sqr(m_Uxo) + sqr(m_Uyo) + sqr(m_Uzo));
  m_Ly = sqrt(sqr(m_Vxo) + sqr(m_Vyo) + sqr(m_Vzo));
  m_Lz = sqrt(sqr(m_Wxo) + sqr(m_Wyo) + sqr(m_Wzo));
  countFlops(15);
  countSqrts(3);
  /// until here

  computeDeltas();

  //=======
  Transformation t;    /// @todo keep for compatibility
  t.setVector(vec3_t(xo1, yo1, zo1));
  setTransformation(t.inverse());
  //>>>>>>> master
}

void CartesianPatch::setupMetrics(real ilength, real jlength, real klength)
{
  // copy physical size (lengths) of block
  m_Lx = ilength;
  m_Ly = jlength;
  m_Lz = klength;
  // build up increments, etc ...
  computeDeltas();
}

//{
//  // copy reference position, coords of point i=j=k=0
//  //  m_Xo = xyzoref[0];
//  //  m_Yo = xyzoref[1];
//  //  m_Zo = xyzoref[2];
//  // copy lengths
//  m_Lx = ijklength[0];
//  m_Ly = ijklength[1];
//  m_Lz = ijklength[2];
//  // build up transformation matrix
//  //.. translation
//  vec3_t shift_inertial2this;
//  shift_inertial2this[0] = -xyzoref[0];  /** @TODO implement operator - for Mathvector (?) */
//  shift_inertial2this[1] = -xyzoref[1];
//  shift_inertial2this[2] = -xyzoref[2];
//  m_transformInertial2This.setVector(shift_inertial2this);
//  //.. turn matrix
//  m_transformInertial2This.setMatrixFromBaseIJ(base_i, base_j);
//  // build up increments, etc ...
//  computeDeltas();

//  Transformation t;    /// @todo keep for compatibility. get rid if no longer needed
//  t.setVector(xyzoref);
//  setTransformation(t.inverse());
//}

void CartesianPatch::resize(size_t num_i, size_t num_j, size_t num_k)
{
  m_NumI  = num_i;
  m_NumJ  = num_j;
  m_NumK  = num_k;
  m_NumJK = num_j*num_k;
  deleteData();
  Patch::resize(m_NumI*m_NumJ*m_NumK);
  computeDeltas();
}

void CartesianPatch::buildBoundingBox()
{
  vec3_t bbox_xyz_min(0., 0., 0.);
  vec3_t bbox_xyz_max(m_Lx, m_Ly, m_Lz);
  m_bbox_xyzo_min = m_transformInertial2This.transformReverse(bbox_xyz_min);
  m_bbox_xyzo_max = m_transformInertial2This.transformReverse(bbox_xyz_max);
}

void CartesianPatch::extractReceiveCells()
{
  bool any_error = false;
  bool error;
  size_t cell_h;
  /** @todo check!! very prone to cut&paste errors.
    * Can do better than below looping: inserts many duplicates, that will be eliminated later.
    */
  for(size_t layer=0; layer<m_NumOverlapLayers; layer++) {
    // i_cell=[0, m_NumOverlapLayers-1] and i_cell=[m_NumI-1, m_NumI-1-m_NumOverlapLayers-1]
    for(size_t j_cell=0; j_cell<m_NumJ; j_cell++) {
      for(size_t k_cell=0; k_cell<m_NumK; k_cell++) {
        cell_h = save_index(layer, j_cell, k_cell,
                            error);
        any_error = any_error || error;
        if(!error) m_receive_cells.push_back(cell_h);
        cell_h = save_index(m_NumI-1-layer, j_cell, k_cell,
                            error);
        any_error = any_error || error;
        if(!error) m_receive_cells.push_back(cell_h);
      }
    }
    // j_cell=[0, m_NumOverlapLayers-1] and j_cell=[m_NumJ-1, m_NumJ-1-m_NumOverlapLayers-1]
    for(size_t k_cell=0; k_cell<m_NumK; k_cell++) {
      for(size_t i_cell=0; i_cell<m_NumI; i_cell++) {
        cell_h = save_index(i_cell, layer, k_cell,
                            error);
        any_error = any_error || error;
        if(!error) m_receive_cells.push_back(cell_h);
        cell_h = save_index(i_cell, m_NumJ-1-layer, k_cell,
                            error);
        any_error = any_error || error;
        if(!error) m_receive_cells.push_back(cell_h);
      }
    }
    // k_cell=[0, m_NumOverlapLayers-1] and k_cell=[m_NumK-1, m_NumK-1-m_NumOverlapLayers-1]
    for(size_t i_cell=0; i_cell<m_NumI; i_cell++) {
      for(size_t j_cell=0; j_cell<m_NumJ; j_cell++) {
        cell_h = save_index(i_cell, j_cell, layer,
                            error);
        any_error = any_error || error;
        if(!error) m_receive_cells.push_back(cell_h);
        cell_h = save_index(i_cell, j_cell, m_NumK-1-layer,
                            error);
        any_error = any_error || error;
        if(!error) m_receive_cells.push_back(cell_h);
      }
    }
  }
}

bool CartesianPatch::computeDependencies(const size_t& i_neighbour)
{
  bool found_dependency = false;
  Patch* neighbour_patch = m_neighbours[i_neighbour].first;
  CoordTransformVV trans = m_neighbours[i_neighbour].second;
  // patch bounding normal vectors in coord syst of donor
  vector<vec3_t> nxxyyzz_ijk;
  nxxyyzz_ijk.resize(3);
  nxxyyzz_ijk[0] = trans.transfree(vec3_t(1., 0., 0.));
  nxxyyzz_ijk[1] = trans.transfree(vec3_t(0., 1., 0.));
  nxxyyzz_ijk[2] = trans.transfree(vec3_t(0., 0., 1.));
  // Process all receiving cells and get interpolation sets
  //  - get own coeffs
  //  - transform into system of neighbour patch
  //  - interpolate there if receiving cell hits neighbours core region
  for(size_t ll_rc=0; ll_rc < m_receive_cells.size(); ll_rc++) {
    size_t l_rc = m_receive_cells[ll_rc];
    //    real x_rc = cellX(l_rc);
    //    real y_rc = cellY(l_rc);
    //    real z_rc = cellZ(l_rc);
    //    real xx_rc, yy_rc, zz_rc;
    vec3_t xyz_rc = xyzCell(l_rc);
    vec3_t xxyyzz_rc = trans.transform(xyz_rc);
    WeightedSet<real> w_set;
    if(m_InterpolateData) {
      if(neighbour_patch->computeCCDataInterpolCoeffs(xxyyzz_rc,
                                                      w_set)) {
        m_InterCoeffData_WS[i_neighbour].push(ll_rc, l_rc, w_set);  // note: since l_rc are unique, no "add" is required
        m_receive_cell_data_hits[ll_rc]++;
        found_dependency = true;
      }
      if(m_InterpolateGrad1N) {
        int direction = boundingNVecDirection(l_rc); // note: trivial function, that ommits normal storage
        //        if(neighbour_patch->computeCCGrad1NInterpolCoeffs(xx_rc, yy_rc, zz_rc,
        //                                                          nxxyyzz_ijk[direction][0], nxxyyzz_ijk[direction][1], nxxyyzz_ijk[direction][2],
        //                                                          w_set)) {
        if(neighbour_patch->computeCCGrad1NInterpolCoeffs(xxyyzz_rc, nxxyyzz_ijk[direction],
                                                          w_set)) {
          m_InterCoeffGrad1N_WS[i_neighbour].push(ll_rc, l_rc, w_set);
          m_receive_cell_grad1N_hits[ll_rc]++;
        }
      }
    }
  }
  return found_dependency;
}


void CartesianPatch::setupInterpolators()
{
  // nothing to be done here.
  /** @note it is likely that setupInterpolators will be meaningful for other
      * patch types. If allways envoqued, shift stuff from computeDeltas back here.
      */
}

// void CartesianPatch::setupInterpolators(size_t num_protectlayers)
// {

//   m_NumProtectLayers = num_protectlayers;

//   // Cell center coords of lowest address cell (i,j,k) = (0,0,0)
//   //  m_xCCMin = 0.5 * m_Dx;
//   //  m_yCCMin = 0.5 * m_Dy;
//   //  m_zCCMin = 0.5 * m_Dz;

//   // CC-coordinates limits to access data from foreign patches
//   m_xCCInterMin = (m_NumProtectLayers + 0.5) * m_Dx;
//   m_yCCInterMin = (m_NumProtectLayers + 0.5) * m_Dy;
//   m_zCCInterMin = (m_NumProtectLayers + 0.5) * m_Dz;
//   m_xCCInterMax = m_Lx - (m_NumProtectLayers + 0.5) * m_Dx;
//   m_yCCInterMax = m_Ly - (m_NumProtectLayers + 0.5) * m_Dy;
//   m_zCCInterMax = m_Lz - (m_NumProtectLayers + 0.5) * m_Dz;

//   m_EpsDX = m_Eps * m_Dx;
//   m_EpsDY = m_Eps * m_Dy;
//   m_EpsDZ = m_Eps * m_Dz;

//   //  m_Interpol_Initialized = true;
// }

bool CartesianPatch::computeCCDataInterpolCoeffs(real x, real y, real z,
                                                 WeightedSet<real>& w_set)
{
  size_t ic_ref, jc_ref, kc_ref;
  bool inside;
  real w_octet[8];

  //  // May not request a value, if m_NumProtectLayers<1
  //  if(m_NumProtectLayers < 1) {
  //    cout << "CartesianPatch::computeCCDataInterpolCoeffs, m_NumProtectLayers = " << m_NumProtectLayers << endl;
  //  }

  // Check inside and get reference cell
  inside = xyzToRefInterCell(x, y, z,
                             ic_ref, jc_ref, kc_ref);
  // Get interpol coeff sets
  if(inside) {
    //.. Interpolation weights
    computeCCInterpolWeights(x, y, z,
                             ic_ref, jc_ref, kc_ref,
                             w_octet);
    //.. Fill into w_set
    //.... Set upper cell addresses
    //     NOTE: It is ensured, that ic_ref, jc_ref, kc_ref are in bounds, but
    //           meshes could eventually be flat, (1D, 2D, testing). Incremented
    //           indicees must eventually be shifted back.
    size_t ic_ref_1 = ic_ref + 1;
    if((ic_ref_1 + 1) > m_NumI) {ic_ref_1 = ic_ref;} // must be flat in i-coords
    size_t jc_ref_1 = jc_ref + 1;
    if((jc_ref_1 + 1) > m_NumJ) {jc_ref_1 = jc_ref;} // must be flat in j-coords
    size_t kc_ref_1 = kc_ref + 1;
    if((kc_ref_1 + 1) > m_NumK) {kc_ref_1 = kc_ref;} // must be flat in k-coords
    //.... fill in
    w_set.clearWS();
    w_set.pushBack(index(ic_ref,   jc_ref,   kc_ref  ), w_octet[0]);
    w_set.pushBack(index(ic_ref,   jc_ref,   kc_ref_1), w_octet[1]);
    w_set.pushBack(index(ic_ref,   jc_ref_1, kc_ref  ), w_octet[2]);
    w_set.pushBack(index(ic_ref,   jc_ref_1, kc_ref_1), w_octet[3]);
    w_set.pushBack(index(ic_ref_1, jc_ref,   kc_ref  ), w_octet[4]);
    w_set.pushBack(index(ic_ref_1, jc_ref,   kc_ref_1), w_octet[5]);
    w_set.pushBack(index(ic_ref_1, jc_ref_1, kc_ref  ), w_octet[6]);
    w_set.pushBack(index(ic_ref_1, jc_ref_1, kc_ref_1), w_octet[7]);

    //    w_set.pushBack(index(ic_ref,   jc_ref,   kc_ref  ), w_octet[0]);
    //    w_set.pushBack(index(ic_ref,   jc_ref,   kc_ref+1), w_octet[1]);
    //    w_set.pushBack(index(ic_ref,   jc_ref+1, kc_ref  ), w_octet[2]);
    //    w_set.pushBack(index(ic_ref,   jc_ref+1, kc_ref+1), w_octet[3]);
    //    w_set.pushBack(index(ic_ref+1, jc_ref,   kc_ref  ), w_octet[4]);
    //    w_set.pushBack(index(ic_ref+1, jc_ref,   kc_ref+1), w_octet[5]);
    //    w_set.pushBack(index(ic_ref+1, jc_ref+1, kc_ref  ), w_octet[6]);
    //    w_set.pushBack(index(ic_ref+1, jc_ref+1, kc_ref+1), w_octet[7]);

    //.. Eliminate round of contributors with below eps-weight
    /// @todo m_Eps is used twice, see also computeDeltas. Potentially conflicting.
    w_set.EliminateBelowEps(10*m_Eps, false, false);
    //.. Ensure sum of weights to be 1. to correct eps-errors
    w_set.adjustWeightSumShift(1.);
  }
  return inside;
}

bool CartesianPatch::computeCCGrad1NInterpolCoeffs(real x, real y, real z,
                                                   real nx, real ny, real nz,
                                                   WeightedSet<real>& d_dn)
{
  size_t ic_ref, jc_ref, kc_ref;
  bool inside;
  real w_octet[8];


  /**
    * @todo
    * There is not much advantage to transfer a gradient, if the one computed at the
    * patch of origin requires NumProtectLayers=2 . The total number of overlapping
    * mesh layers would then be 3 (2 on the donor, 1 an the receiving patch). With
    * simple data exchange only, 4 overlapping layers would be required.
    *
    * Optionally, transfer a one sided gradient only. This is possible for NumProtectLayers=1 ,
    * while the receiving patch might reconstruct the central gradient with its own data.
    *
    * NOT YET IMPLEMENTED !!
    *
    */

  //  // May not request a value, if m_NumProtectLayers<2
  //  if(m_NumProtectLayers<2) {
  //    cout << "CartesianPatch::computeCCGrad1NInterpolCoeffs, m_NumProtectLayers = " << m_NumProtectLayers << endl;
  //  }
  // Check inside and get reference cell
  inside = xyzToRefInterCell(x, y, z,
                             ic_ref, jc_ref, kc_ref);
  // Get interpol coeff sets
  if(inside) {
    //.. Interpolation weights
    computeCCInterpolWeights(x, y, z,
                             ic_ref, jc_ref, kc_ref,
                             w_octet);
    //.. Normalize n-vector
    real inv_n_abs = 1./sqrt(nx*nx + ny*ny + nz*nz);
    real nnx = nx * inv_n_abs;
    real nny = ny * inv_n_abs;
    real nnz = nz * inv_n_abs;

    //.. Convenience
    real div_x = nnx/(2*m_Dx);
    real div_y = nny/(2*m_Dy);
    real div_z = nnz/(2*m_Dz);

    d_dn.clearWS();
    //   0:       (ic_ref,   jc_ref,   kc_ref  )
    size_t ii = ic_ref;
    size_t jj = jc_ref;
    size_t kk = kc_ref;
    d_dn.pushBack(index(ii+1, jj  , kk  ),  w_octet[0]*div_x);
    d_dn.pushBack(index(ii-1, jj  , kk  ), -w_octet[0]*div_x);
    d_dn.pushBack(index(ii  , jj+1, kk  ),  w_octet[0]*div_y);
    d_dn.pushBack(index(ii  , jj-1, kk  ), -w_octet[0]*div_y);
    d_dn.pushBack(index(ii  , jj  , kk+1),  w_octet[0]*div_z);
    d_dn.pushBack(index(ii  , jj  , kk-1), -w_octet[0]*div_z);
    //   1:       (ic_ref,   jc_ref,   kc_ref+1)
    ii = ic_ref;
    jj = jc_ref;
    kk = kc_ref+1;
    d_dn.pushBack(index(ii+1, jj  , kk  ),  w_octet[1]*div_x);
    d_dn.pushBack(index(ii-1, jj  , kk  ), -w_octet[1]*div_x);
    d_dn.pushBack(index(ii  , jj+1, kk  ),  w_octet[1]*div_y);
    d_dn.pushBack(index(ii  , jj-1, kk  ), -w_octet[1]*div_y);
    d_dn.pushBack(index(ii  , jj  , kk+1),  w_octet[1]*div_z);
    d_dn.pushBack(index(ii  , jj  , kk-1), -w_octet[1]*div_z);
    //   2:       (ic_ref,   jc_ref+1, kc_ref  )
    ii = ic_ref;
    jj = jc_ref+1;
    kk = kc_ref;
    d_dn.pushBack(index(ii+1, jj  , kk  ),  w_octet[2]*div_x);
    d_dn.pushBack(index(ii-1, jj  , kk  ), -w_octet[2]*div_x);
    d_dn.pushBack(index(ii  , jj+1, kk  ),  w_octet[2]*div_y);
    d_dn.pushBack(index(ii  , jj-1, kk  ), -w_octet[2]*div_y);
    d_dn.pushBack(index(ii  , jj  , kk+1),  w_octet[2]*div_z);
    d_dn.pushBack(index(ii  , jj  , kk-1), -w_octet[2]*div_z);
    //   3:       (ic_ref,   jc_ref+1, kc_ref+1)
    ii = ic_ref;
    jj = jc_ref+1;
    kk = kc_ref+1;
    d_dn.pushBack(index(ii+1, jj  , kk  ),  w_octet[3]*div_x);
    d_dn.pushBack(index(ii-1, jj  , kk  ), -w_octet[3]*div_x);
    d_dn.pushBack(index(ii  , jj+1, kk  ),  w_octet[3]*div_y);
    d_dn.pushBack(index(ii  , jj-1, kk  ), -w_octet[3]*div_y);
    d_dn.pushBack(index(ii  , jj  , kk+1),  w_octet[3]*div_z);
    d_dn.pushBack(index(ii  , jj  , kk-1), -w_octet[3]*div_z);
    //   4:       (ic_ref+1, jc_ref,   kc_ref  )
    ii = ic_ref+1;
    jj = jc_ref;
    kk = kc_ref;
    d_dn.pushBack(index(ii+1, jj  , kk  ),  w_octet[4]*div_x);
    d_dn.pushBack(index(ii-1, jj  , kk  ), -w_octet[4]*div_x);
    d_dn.pushBack(index(ii  , jj+1, kk  ),  w_octet[4]*div_y);
    d_dn.pushBack(index(ii  , jj-1, kk  ), -w_octet[4]*div_y);
    d_dn.pushBack(index(ii  , jj  , kk+1),  w_octet[4]*div_z);
    d_dn.pushBack(index(ii  , jj  , kk-1), -w_octet[4]*div_z);
    //   5:       (ic_ref+1, jc_ref,   kc_ref+1)
    ii = ic_ref+1;
    jj = jc_ref;
    kk = kc_ref+1;
    d_dn.pushBack(index(ii+1, jj  , kk  ),  w_octet[5]*div_x);
    d_dn.pushBack(index(ii-1, jj  , kk  ), -w_octet[5]*div_x);
    d_dn.pushBack(index(ii  , jj+1, kk  ),  w_octet[5]*div_y);
    d_dn.pushBack(index(ii  , jj-1, kk  ), -w_octet[5]*div_y);
    d_dn.pushBack(index(ii  , jj  , kk+1),  w_octet[5]*div_z);
    d_dn.pushBack(index(ii  , jj  , kk-1), -w_octet[5]*div_z);
    //   6:       (ic_ref+1, jc_ref+1, kc_ref  )
    ii = ic_ref+1;
    jj = jc_ref+1;
    kk = kc_ref;
    d_dn.pushBack(index(ii+1, jj  , kk  ),  w_octet[6]*div_x);
    d_dn.pushBack(index(ii-1, jj  , kk  ), -w_octet[6]*div_x);
    d_dn.pushBack(index(ii  , jj+1, kk  ),  w_octet[6]*div_y);
    d_dn.pushBack(index(ii  , jj-1, kk  ), -w_octet[6]*div_y);
    d_dn.pushBack(index(ii  , jj  , kk+1),  w_octet[6]*div_z);
    d_dn.pushBack(index(ii  , jj  , kk-1), -w_octet[6]*div_z);
    //   7:       (ic_ref+1, jc_ref+1, kc_ref+1)
    ii = ic_ref+1;
    jj = jc_ref+1;
    kk = kc_ref+1;
    d_dn.pushBack(index(ii+1, jj  , kk  ),  w_octet[7]*div_x);
    d_dn.pushBack(index(ii-1, jj  , kk  ), -w_octet[7]*div_x);
    d_dn.pushBack(index(ii  , jj+1, kk  ),  w_octet[7]*div_y);
    d_dn.pushBack(index(ii  , jj-1, kk  ), -w_octet[7]*div_y);
    d_dn.pushBack(index(ii  , jj  , kk+1),  w_octet[7]*div_z);
    d_dn.pushBack(index(ii  , jj  , kk-1), -w_octet[7]*div_z);
    //
    //.. compact to unique addresses
    d_dn.Unify();
    //.. Eliminate round off contributors with below eps-weight
    /// @todo m_Eps is used twice, see also computeDeltas. Potentially conflicting.
    d_dn.EliminateBelowEps(10*m_Eps, true, false);
    //.. Ensure sum of weights to be 0. to correct eps-errors
    /// @todo check function carefully. Sum of Weights must be close to zero anyway.
    d_dn.adjustWeightSumShift(0.);
  }
  return inside;
}


void CartesianPatch::computeCCInterpolWeights(real& x, real& y, real& z,
                                              size_t& ic_ref, size_t& jc_ref, size_t& kc_ref,
                                              real* w_octet)
{
  // Perform a tri-linear interpolation for a point with coordinates (x,y,z)
  // Interpolate values using a box of the inverse mesh build up of geometric centers of cells
  // box corner    cell indicees of box corners
  //   0:         ( ic_ref,   jc_ref,   kc_ref   )
  //   1:         ( ic_ref,   jc_ref,   kc_ref+1 )
  //   2:         ( ic_ref,   jc_ref+1, kc_ref   )
  //   3:         ( ic_ref,   jc_ref+1, kc_ref+1 )
  //   4:         ( ic_ref+1, jc_ref,   kc_ref   )
  //   5:         ( ic_ref+1, jc_ref,   kc_ref+1 )
  //   6:         ( ic_ref+1, jc_ref+1, kc_ref   )
  //   7:         ( ic_ref+1, jc_ref+1, kc_ref+1 )

  // cartesian distances to bounding element sides
  real x_ref = 0.5*m_Dx + ic_ref*m_Dx;
  real y_ref = 0.5*m_Dy + jc_ref*m_Dy;
  real z_ref = 0.5*m_Dz + kc_ref*m_Dz;

  computeInterpolWeights(x, y, z,
                         x_ref, y_ref, z_ref,
                         w_octet);
}

void CartesianPatch::computeNCInterpolWeights(real& x, real& y, real& z,
                                              size_t& in_ref, size_t& jn_ref, size_t& kn_ref,
                                              real* w_octet)
{
  // Perform a tri-linear interpolation for a point with coordinates (x,y,z)
  // Interpolate values using natural mesh nodes
  // box corner    node indicees of box corners
  //   0:         ( in_ref,   jn_ref,   kn_ref   )
  //   1:         ( in_ref,   jn_ref,   kn_ref+1 )
  //   2:         ( in_ref,   jn_ref+1, kn_ref   )
  //   3:         ( in_ref,   jn_ref+1, kn_ref+1 )
  //   4:         ( in_ref+1, jn_ref,   kn_ref   )
  //   5:         ( in_ref+1, jn_ref,   kn_ref+1 )
  //   6:         ( in_ref+1, jn_ref+1, kn_ref   )
  //   7:         ( in_ref+1, jn_ref+1, kn_ref+1 )

  // cartesian distances to bounding element sides
  real x_ref = in_ref*m_Dx;
  real y_ref = jn_ref*m_Dy;
  real z_ref = kn_ref*m_Dz;

  computeInterpolWeights(x, y, z,
                         x_ref, y_ref, z_ref,
                         w_octet);
}

void CartesianPatch::computeInterpolWeights(real& x, real& y, real& z,
                                            real& x_ref, real& y_ref, real& z_ref,
                                            real* w_octet)
{
  // Perform a tri-linear interpolation for a point with coordinates (x,y,z)
  // in a box given by the frame corner coordinates.

  // NOTE: no range checking. Will perform an false extrapolation, if
  //       (x,y,z) is outside box.

  // box corner:       coords:
  //   0:          ( x_ref,      y_ref,      z_ref      )
  //   1:          ( x_ref,      y_ref,      z_ref+m_Dz )
  //   2:          ( x_ref,      y_ref+m_Dy, z_ref      )
  //   3:          ( x_ref,      y_ref+m_Dy, z_ref+m_Dz )
  //   4:          ( x_ref+m_Dx, y_ref,      z_ref      )
  //   5:          ( x_ref+m_Dx, y_ref,      z_ref+m_Dz )
  //   6:          ( x_ref+m_Dx, y_ref+m_Dy, z_ref      )
  //   7:          ( x_ref+m_Dx, y_ref+m_Dy, z_ref+m_Dz )

  // cartesian distances to box sides
  real low_diff_x_ref = x - x_ref;
  real low_diff_y_ref = y - y_ref;
  real low_diff_z_ref = z - z_ref;
  real up_diff_x_ref = m_Dx - low_diff_x_ref;
  real up_diff_y_ref = m_Dy - low_diff_y_ref;
  real up_diff_z_ref = m_Dz - low_diff_z_ref;

  real inv_cell_vol = m_InvDx * m_InvDy * m_InvDz;

  //.. Init weights
  w_octet[0] = inv_cell_vol;
  w_octet[1] = inv_cell_vol;
  w_octet[2] = inv_cell_vol;
  w_octet[3] = inv_cell_vol;
  w_octet[4] = inv_cell_vol;
  w_octet[5] = inv_cell_vol;
  w_octet[6] = inv_cell_vol;
  w_octet[7] = inv_cell_vol;

  //.. lower x-side, box corners: 0,1,2,3
  w_octet[0] *= up_diff_x_ref;
  w_octet[1] *= up_diff_x_ref;
  w_octet[2] *= up_diff_x_ref;
  w_octet[3] *= up_diff_x_ref;

  //.. upper x-side, box corners: 4,5,6,7
  w_octet[4] *= low_diff_x_ref;
  w_octet[5] *= low_diff_x_ref;
  w_octet[6] *= low_diff_x_ref;
  w_octet[7] *= low_diff_x_ref;

  //.. lower y-side, box corners: 0,1,4,5
  w_octet[0] *= up_diff_y_ref;
  w_octet[1] *= up_diff_y_ref;
  w_octet[4] *= up_diff_y_ref;
  w_octet[5] *= up_diff_y_ref;

  //.. upper y-side, box corners: 2,3,6,7
  w_octet[2] *= low_diff_y_ref;
  w_octet[3] *= low_diff_y_ref;
  w_octet[6] *= low_diff_y_ref;
  w_octet[7] *= low_diff_y_ref;

  //.. lower z-side, box corners: 0,2,4,6
  w_octet[0] *= up_diff_z_ref;
  w_octet[2] *= up_diff_z_ref;
  w_octet[4] *= up_diff_z_ref;
  w_octet[6] *= up_diff_z_ref;

  //.. upper z-side, box corners: 1,3,5,7
  w_octet[1] *= low_diff_z_ref;
  w_octet[3] *= low_diff_z_ref;
  w_octet[5] *= low_diff_z_ref;
  w_octet[7] *= low_diff_z_ref;

}


// void CartesianPatch::computeCCGrad1NInterpolCoeffs(real& x, real& y, real& z,
// 						   size_t& ic_ref, size_t& jc_ref, size_t& kc_ref,
// 						   real nx, real ny, real nz,
// 						   WeightedSet<real>* d_dn)
// {
//   // Perform a tri-linear interpolation for a point with coordinates (x,y,z)
//   // Interpolate values using inverse mesh build up of geometric centers of cells
//   // w_octet       cell
//   //   0:       (ic_ref,   jc_ref,   kc_ref  )
//   //   1:       (ic_ref,   jc_ref,   kc_ref+1)
//   //   2:       (ic_ref,   jc_ref+1, kc_ref  )
//   //   3:       (ic_ref,   jc_ref+1, kc_ref+1)
//   //   4:       (ic_ref+1, jc_ref,   kc_ref  )
//   //   5:       (ic_ref+1, jc_ref,   kc_ref+1)
//   //   6:       (ic_ref+1, jc_ref+1, kc_ref  )
//   //   7:       (ic_ref+1, jc_ref+1, kc_ref+1)
//   //
//   // ATTENTION: Assume inside inverse mesh

//   // Perform a bi-linear interpolation of 4 edge gradients in discrete coord-directions

//   /// @todo Not sure, if this interpolation holds the formal order of a computation

// // X-gradient
// //.. cell-cell difference grad for 0->4
// WeightedSet<real> d_dx_04();
// d_dx_04.pushBack(index(ic_ref,   jc_ref,   kc_ref  ), -m_InvDx);
// d_dx_04.pushBack(index(ic_ref+1, jc_ref,   kc_ref  ),  m_InvDx);
// d_dx_04 *= up_diff_y_ref;
// d_dx_04 *= up_diff_z_ref;
// //.. cell-cell difference grad for 1->5
// WeightedSet<real> d_dx_15();
// d_dx_15.pushBack(index(ic_ref,   jc_ref,   kc_ref  ), -m_InvDx);
// d_dx_15.pushBack(index(ic_ref+1, jc_ref,   kc_ref  ),  m_InvDx);
// d_dx_15 *= up_diff_y_ref;
// d_dx_15 *= low_diff_z_ref;
// //.. cell-cell difference grad for 2->6
// WeightedSet<real> d_dx_26();
// d_dx_26.pushBack(index(ic_ref,   jc_ref,   kc_ref  ), -m_InvDx);
// d_dx_26.pushBack(index(ic_ref+1, jc_ref,   kc_ref  ),  m_InvDx);
// d_dx_26 *= low_diff_y_ref;
// d_dx_26 *= up_diff_z_ref;
// //.. cell-cell difference grad for 3->7
// WeightedSet<real> d_dx_37();
// d_dx_37.pushBack(index(ic_ref,   jc_ref,   kc_ref  ), -m_InvDx);
// d_dx_37.pushBack(index(ic_ref+1, jc_ref,   kc_ref  ),  m_InvDx);
// d_dx_37 *= low_diff_y_ref;
// d_dx_37 *= low_diff_z_ref;
// //.. combined x-grad
// WeightedSet<real> d_dx();
// d_dx += d_dx_04;
// d_dx += d_dx_15;
// d_dx += d_dx_26;
// d_dx += d_dx_37;

// // Y-gradient
// //.. cell-cell difference grad for 0->2
// WeightedSet<real> d_dx_02();
// d_dx_02.pushBack(index(ic_ref,   jc_ref,   kc_ref  ), -m_InvDy);
// d_dx_02.pushBack(index(ic_ref+1, jc_ref,   kc_ref  ),  m_InvDy);
// d_dx_02 *= up_diff_x_ref;
// d_dx_02 *= up_diff_z_ref;

// // X-gradient
// //.. proj average on "inverse mesh face" i=ic_ref
// WeightedSet<real> face_i_low();
// face_i_low.pushBack(index(ic_ref,   jc_ref,   kc_ref  ), up_diff_y_ref  * up_diff_z_ref ); // 0
// face_i_low.pushBack(index(ic_ref,   jc_ref,   kc_ref+1), up_diff_y_ref  * low_diff_z_ref); // 1
// face_i_low.pushBack(index(ic_ref,   jc_ref+1, kc_ref  ), low_diff_y_ref * up_diff_z_ref ); // 2
// face_i_low.pushBack(index(ic_ref,   jc_ref+1, kc_ref+1), low_diff_y_ref * low_diff_z_ref); // 3
// //.. proj average on "inverse mesh face" i=ic_ref+1
// WeightedSet<real> face_i_up();
// face_i_low.pushBack(index(ic_ref+1, jc_ref,   kc_ref  ), up_diff_y_ref  * up_diff_z_ref ); // 4
// face_i_low.pushBack(index(ic_ref+1, jc_ref,   kc_ref+1), up_diff_y_ref  * low_diff_z_ref); // 5
// face_i_low.pushBack(index(ic_ref+1, jc_ref+1, kc_ref  ), low_diff_y_ref * up_diff_z_ref ); // 6
// face_i_low.pushBack(index(ic_ref+1, jc_ref+1, kc_ref+1), low_diff_y_ref * low_diff_z_ref); // 7
// //.. x-grad as difference from above
// face_i_low *= (m_InvDy * m_InvDz);
// face_i_up  *= (m_InvDy * m_InvDz);
// WeightedSet<real> d_dx();
// d_dx.clearWS();
// d_dx.Concatenate(face_i_up ,  m_InvDx);
// d_dx.Concatenate(face_i_low, -m_InvDx);

// // Y-gradient
// //.. proj average on "inverse mesh face" j=jc_ref
// WeightedSet<real> face_j_low();
// face_i_low.pushBack(index(ic_ref,   jc_ref,   kc_ref  ), up_diff_x_ref  * up_diff_z_ref ); // 0
// face_i_low.pushBack(index(ic_ref,   jc_ref,   kc_ref+1), up_diff_x_ref  * low_diff_z_ref); // 1
// face_i_low.pushBack(index(ic_ref+1, jc_ref,   kc_ref  ), low_diff_x_ref * up_diff_z_ref ); // 4
// face_i_low.pushBack(index(ic_ref+1, jc_ref,   kc_ref+1), low_diff_x_ref * low_diff_z_ref); // 5
// //.. proj average on "inverse mesh face" j=jc_ref+1
// WeightedSet<real> face_j_up();
// face_i_low.pushBack(index(ic_ref,   jc_ref+1, kc_ref  ), up_diff_x_ref  * up_diff_z_ref ); // 2
// face_i_low.pushBack(index(ic_ref,   jc_ref+1, kc_ref+1), up_diff_x_ref  * low_diff_z_ref); // 3
// face_i_low.pushBack(index(ic_ref+1, jc_ref+1, kc_ref  ), low_diff_x_ref * up_diff_z_ref ); // 6
// face_i_low.pushBack(index(ic_ref+1, jc_ref+1, kc_ref+1), low_diff_x_ref * low_diff_z_ref); // 7
// //.. x-grad as difference from above
// face_j_low *= (m_InvDx * m_InvDz);
// face_j_up  *= (m_InvDx * m_InvDz);
// WeightedSet<real> d_dx();
// d_dx.clearWS();
// d_dx.Concatenate(face_j_up ,  m_InvDy);
// d_dx.Concatenate(face_j_low, -m_InvDy);

// // X-gradient
// //.. proj average on "inverse mesh face" i=ic_ref
// WeightedSet<real> face_i_low();
// face_i_low.pushBack(index(ic_ref,   jc_ref,   kc_ref  ), up_diff_y_ref  * up_diff_z_ref ); // 0
// face_i_low.pushBack(index(ic_ref,   jc_ref,   kc_ref+1), up_diff_y_ref  * low_diff_z_ref); // 1
// face_i_low.pushBack(index(ic_ref,   jc_ref+1, kc_ref  ), low_diff_y_ref * up_diff_z_ref ); // 2
// face_i_low.pushBack(index(ic_ref,   jc_ref+1, kc_ref+1), low_diff_y_ref * low_diff_z_ref); // 3
// //.. proj average on "inverse mesh face" i=ic_ref+1
// WeightedSet<real> face_i_up();
// face_i_low.pushBack(index(ic_ref+1, jc_ref,   kc_ref  ), up_diff_y_ref  * up_diff_z_ref ); // 4
// face_i_low.pushBack(index(ic_ref+1, jc_ref,   kc_ref+1), up_diff_y_ref  * low_diff_z_ref); // 5
// face_i_low.pushBack(index(ic_ref+1, jc_ref+1, kc_ref  ), low_diff_y_ref * up_diff_z_ref ); // 6
// face_i_low.pushBack(index(ic_ref+1, jc_ref+1, kc_ref+1), low_diff_y_ref * low_diff_z_ref); // 7
// //.. x-grad as difference from above
// face_i_low *= (m_InvDy * m_InvDz);
// face_i_up  *= (m_InvDy * m_InvDz);
// WeightedSet<real> d_dx();
// d_dx.clearWS();
// d_dx.Concatenate(face_i_up ,  m_InvDx);
// d_dx.Concatenate(face_i_low, -m_InvDx);

// // Directed gradient
// WeightedSet<real> d_
// //.. normalize n-vector
// real inv_n_abs = 1./sqrt(nx*nx + ny*ny + nz*nz);
// nx *= inv_n_abs;
// ny *= inv_n_abs;
// nz *= inv_n_abs;
// //..
// d_dn-clearWS();
// d_dn->Concatenate(d_dx, nx);
// d_dn->Concatenate(d_dy, ny);
// d_dn->Concatenate(d_dz, nz);
// d_dn->Unify();

bool CartesianPatch::xyzToRefInterCell(real x, real y, real z,
                                       size_t& ic_ref, size_t& jc_ref, size_t& kc_ref)
{
  bool inside = true;
  //
  // Check limits. Allow x,y,z in eps-bounds of interpolation range as well.
  // NOTE: if protective layers m_numProt.... == 0 (possible as boundary exception),
  // locations of (x,y,z) outside the box bounding CCs are possible.
  //
  if(x < m_xCCInterMin-m_EpsDX) {ic_ref = 0; inside = false;};
  if(y < m_yCCInterMin-m_EpsDY) {jc_ref = 0; inside = false;};
  if(z < m_zCCInterMin-m_EpsDZ) {kc_ref = 0; inside = false;};
  if(x > m_xCCInterMax+m_EpsDX) {ic_ref = m_NumI-1; inside = false;};
  if(y > m_yCCInterMax+m_EpsDY) {jc_ref = m_NumJ-1; inside = false;};
  if(z > m_zCCInterMax+m_EpsDZ) {kc_ref = m_NumK-1; inside = false;};
  //
  // Indicees pick, if inside
  if(inside) {
    //..Position relative to lowest CC-coords (indicees hashing)
    real ric_ref = (x - m_xCCMin) / m_Dx;
    real rjc_ref = (y - m_yCCMin) / m_Dy;
    real rkc_ref = (z - m_zCCMin) / m_Dz;
    //.. Correct relative position to ensure indicees are in range
    //   (lower bounds correction: ensure non-negative)
    if(ric_ref < m_Eps) {ric_ref = m_Eps;}
    if(rjc_ref < m_Eps) {rjc_ref = m_Eps;}
    if(rkc_ref < m_Eps) {rkc_ref = m_Eps;}
    //.. Convert to size_t
    ic_ref = size_t(ric_ref);
    jc_ref = size_t(rjc_ref);
    kc_ref = size_t(rkc_ref);
    //.. Correct to upper bounds index limits
    //   Robustness: Must check, if mesh sizes i,j,k are sufficient.
    if(ic_ref > m_NumI-1) {ic_ref = m_NumI-1;}
    if(jc_ref > m_NumJ-1) {jc_ref = m_NumJ-1;}
    if(kc_ref > m_NumK-1) {kc_ref = m_NumK-1;}

    //    //.... i-coord (x)
    //    if(m_NumI > 1) { // not flat in I
    //        if(ic_ref > m_NumI-1) {ic_ref = m_NumI-1;}
    //    } else {
    //        ic_ref = 0;
    //    }
    //    //.... j-coord (y)
    //    if(m_NumJ > 1) { // not flat in J
    //        if(jc_ref > m_NumJ-1) {jc_ref = m_NumJ-1;}
    //    } else {
    //        jc_ref = 0;
    //    }
    //    //.... k-coord (z)
    //    if(m_NumK > 1) { // not flat in K
    //        if(kc_ref > m_NumK-1) {kc_ref = m_NumK-1;}
    //    } else {
    //        kc_ref = 0;
    //    }
  }
  return inside;
}

bool CartesianPatch::xyzToRefCell(real x, real y, real z,
                                  size_t& ic_ref, size_t& jc_ref, size_t& kc_ref)
{
  bool inside = true;

  /// @todo Eps-stuff is quite ugly down here

  // Check inside limits. Allow x,y,z in eps-bounds as well.
  if(x < m_xCCMin-m_EpsDX) {ic_ref = 0; inside = false;};
  if(y < m_yCCMin-m_EpsDY) {jc_ref = 0; inside = false;};
  if(z < m_zCCMin-m_EpsDZ) {kc_ref = 0; inside = false;};
  if(x > m_xCCMax+m_EpsDX) {ic_ref = m_NumI-1; inside = false;};
  if(y > m_yCCMax+m_EpsDY) {jc_ref = m_NumJ-1; inside = false;};
  if(z > m_zCCMax+m_EpsDZ) {kc_ref = m_NumK-1; inside = false;};
  // get reference cell, if inside
  if(inside) {
    //.. Manipulate x,y,z if these coords depass the borders
    //   NOTE: inside==true means that x,y,z is in eps-bounds anyway
    //         if close, shift x,y,z to inside to ensure correct address pick
    if(x < m_xCCMin+m_EpsDX) {x = m_xCCMin+m_EpsDX;}
    if(y < m_yCCMin+m_EpsDY) {y = m_yCCMin+m_EpsDY;}
    if(z < m_zCCMin+m_EpsDZ) {z = m_zCCMin+m_EpsDZ;}
    if(x > m_xCCMax-m_EpsDX) {x = m_xCCMax-m_EpsDX;}
    if(y > m_yCCMax-m_EpsDY) {y = m_yCCMax-m_EpsDY;}
    if(z > m_zCCMax-m_EpsDZ) {z = m_zCCMax-m_EpsDZ;}
    //.. Indicees pick
    //   NOTE: Getting here, means "inside" anyway.
    //.... Position relative to lowest CC-coords (indicees hashing)
    real ric_ref = (x - m_xCCMin) / m_Dx;
    real rjc_ref = (y - m_yCCMin) / m_Dy;
    real rkc_ref = (z - m_zCCMin) / m_Dz;
    //.... Correct relative position to ensure indicees are in range
    //     (lower bounds correction: ensure non-negative)
    if(ric_ref < m_Eps) {ric_ref = m_Eps;}
    if(rjc_ref < m_Eps) {rjc_ref = m_Eps;}
    if(rkc_ref < m_Eps) {rkc_ref = m_Eps;}
    //.... Convert to size_t
    ic_ref = size_t(ric_ref);
    jc_ref = size_t(rjc_ref);
    kc_ref = size_t(rkc_ref);
    //.... Correct to upper bounds index limits
    if(ic_ref > m_NumI-2) {ic_ref = m_NumI-2;}
    if(jc_ref > m_NumJ-2) {jc_ref = m_NumJ-2;}
    if(kc_ref > m_NumK-2) {kc_ref = m_NumK-2;}
  }
  return inside;
}

bool CartesianPatch::xyzToRefNode(real x, real y, real z,
                                  size_t& in_ref, size_t& jn_ref, size_t& kn_ref)
{
  bool inside = true;

  /// @todo Eps-stuff is quite ugly down here

  // check limits
  if(x < -m_EpsDX) {in_ref = 0; inside = false;};
  if(y < -m_EpsDY) {jn_ref = 0; inside = false;};
  if(z < -m_EpsDZ) {kn_ref = 0; inside = false;};
  if(x > m_Lx+m_EpsDX) {in_ref = m_NumI-1; inside = false;};
  if(y > m_Ly+m_EpsDY) {jn_ref = m_NumJ-1; inside = false;};
  if(z > m_Lz+m_EpsDZ) {kn_ref = m_NumK-1; inside = false;};

  // get reference cell, if inside
  if(inside) {
    //.. Manipulate x,y,z if these coords depass the borders
    //   NOTE: inside==true means that x,y,z is in eps-bounds anyway
    //         if close, shift x,y,z to inside to ensure correct address pick
    if(x < m_EpsDX) {x = m_EpsDX;}
    if(y < m_EpsDY) {y = m_EpsDY;}
    if(z < m_EpsDZ) {z = m_EpsDZ;}
    if(x > m_Lx-m_EpsDX) {x = m_Lx-m_EpsDX;}
    if(y > m_Ly-m_EpsDY) {y = m_Ly-m_EpsDY;}
    if(z > m_Lz-m_EpsDZ) {z = m_Lz-m_EpsDZ;}
    //.. Position relative to lowest CC-coords.
    real rin_ref = (x - m_xCCMin) / m_Dx;
    real rjn_ref = (y - m_yCCMin) / m_Dy;
    real rkn_ref = (z - m_zCCMin) / m_Dz;
    //    iin_ref = lrint(floor(rin_ref));
    //    jjn_ref = lrint(floor(rjn_ref));
    //    kkn_ref = lrint(floor(rkn_ref));
    //.. Compute reference address. Ensure positive.
    ///@todo: Check, if this is aceptable from robustnes
    //   NOTE: Due to Eps-errors, negative values for rin_ref, rjn_ref, rkn_ref are possible
    //         if Eps_DXYZ==0.
    in_ref = size_t(rin_ref);
    jn_ref = size_t(rjn_ref);
    kn_ref = size_t(rkn_ref);
    //.. Error checking, actually impossible: Be sure to be in addressing range
    bool error = false;
    if(rin_ref < 0.) {
      cout << "CartesianPatch::xyzToRefNode, i_min"; error = true;
      in_ref = 0;
    }
    if(rjn_ref < 0.) {
      cout << "CartesianPatch::xyzToRefNode, j_min"; error = true;
      jn_ref = 0;
    }
    if(rkn_ref < 0.) {
      cout << "CartesianPatch::xyzToRefNode, k_min"; error = true;
      kn_ref = 0;
    }
    if(in_ref > m_NumI-2) {
      cout << "CartesianPatch::xyzToRefNode, i_max"; error = true;
    }
    if(jn_ref > m_NumJ-2) {
      cout << "CartesianPatch::xyzToRefNode, j_max"; error = true;
    }
    if(kn_ref > m_NumK-2) {
      cout << "CartesianPatch::xyzToRefNode, k_max"; error = true;
    }
    if(error) {
      /// @todo may put this in debug condition
      cout << endl;
      cout << " m_xyzCCMin = " << m_xCCMin << ", " << m_yCCMin << ", " << m_zCCMin << endl;
      cout << " m_xyzCCMax = " << m_xCCMax << ", " << m_yCCMax << ", " << m_zCCMax << endl;
      cout << " (x,y,z)    = " << x        << ", " << y        << ", " << z             << endl;
      exit(EXIT_FAILURE);
    }
  }
  return inside;
}


/*
#ifdef WITH_VTK

void CartesianPatch::writeToVtk(QString file_name)
{
  vtkSmartPointer<vtkRectilinearGrid> grid = vtkSmartPointer<vtkRectilinearGrid>::New();

  vtkSmartPointer<vtkFloatArray> xc = vtkSmartPointer<vtkFloatArray>::New();
  for (size_t i = 0; i < m_NumI + 1; ++i) {
    xc->InsertNextValue(i*dx());
  }

  vtkSmartPointer<vtkFloatArray> yc = vtkSmartPointer<vtkFloatArray>::New();
  for (size_t j = 0; j < m_NumJ + 1; ++j) {
    yc->InsertNextValue(j*dy());
  }

  vtkSmartPointer<vtkFloatArray> zc = vtkSmartPointer<vtkFloatArray>::New();
  for (size_t k = 0; k < m_NumK + 1; ++k) {
    zc->InsertNextValue(k*dz());
  }

  grid->SetDimensions(m_NumI + 1, m_NumJ + 1, m_NumK + 1);
  grid->SetXCoordinates(xc);
  grid->SetYCoordinates(yc);
  grid->SetZCoordinates(zc);

  for (size_t i_field = 0; i_field < numFields(); ++i_field) {
    QString field_name;
    field_name.setNum(i_field + 1);
    if (field_name.size() < 2) {
      field_name = "0" + field_name;
    }
    for (size_t i_var = 0; i_var < numVariables(); ++i_var) {
      QString var_name;
      var_name.setNum(i_var + 1);
      if (var_name.size() < 2) {
        var_name = "0" + var_name;
      }
      vtkSmartPointer<vtkFloatArray> var = vtkSmartPointer<vtkFloatArray>::New();
      var->SetName(qPrintable("f_" + field_name + "_" + var_name));
      var->SetNumberOfValues(variableSize());
      grid->GetCellData()->AddArray(var);
      vtkIdType id = 0;
      for (size_t k = 0; k < m_NumK; ++k) {
        for (size_t j = 0; j < m_NumJ; ++j) {
          for (size_t i = 0; i < m_NumI; ++i) {
            var->SetValue(id, f(getVariable(i_field, i_var), i, j, k));
            ++id;
          }
        }
      }
    }
  }

  vtkSmartPointer<vtkXMLRectilinearGridWriter> vtr = vtkSmartPointer<vtkXMLRectilinearGridWriter>::New();
  vtr->SetFileName(qPrintable(file_name + ".vtr"));
  vtr->SetInput(grid);
  vtr->Write();
}

#endif
*/
//=======
//>>>>>>> master
