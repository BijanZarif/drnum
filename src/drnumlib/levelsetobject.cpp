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
#include "levelsetobject.h"


LevelSetObject::LevelSetObject(LevelSetDefinition* levelset,
                               PatchGrid *patch_grid,
                               size_t field_index,
                               size_t var_index,
                               size_t num_inner_layers,
                               size_t num_outer_layers,
                               real min_innerreldist,
                               real min_outerreldist)
{
  m_LevelSetDefinition = levelset;
  m_PatchGrid          = patch_grid;
  m_FieldIndex         = field_index;
  m_VarIndex           = var_index;
  m_NumInnerLayers     = num_inner_layers;
  m_NumOuterLayers     = num_outer_layers;
  m_MinInnerRelDist    = min_innerreldist;
  m_MinOuterRelDist    = min_outerreldist;
}


void LevelSetObject::update()
{

  m_AffectedPatchIDs.clear();    // just to be sure
  m_FullyBlackPatchIDs.clear();  // just to be sure

  // Loop for patches
  for (size_t i_p = 0; i_p < m_PatchGrid->getNumPatches(); i_p++) {

    Patch* patch = m_PatchGrid->getPatch(i_p);
    bool patch_affected = false;
    bool fully_black = true;
    real* var = patch->getVariable(m_FieldIndex, m_VarIndex);

    // Loop for cells of patch
    /// @todo parallel omp made corresponding loop in BlockObject extremely slow ???
    //#pragma omp parallel for
    for (size_t l_c = 0; l_c < patch->variableSize(); l_c++) {
      real xc, yc, zc;
      patch->xyzoCell(l_c,
                      xc, yc, zc);
      var[l_c] = m_LevelSetDefinition->calcDistance(xc, yc, zc);
      if (var[l_c] < 0.) patch_affected = true;
      if (var[l_c] > 0.) fully_black = false;
    }

    if (patch_affected) {
      m_AffectedPatchIDs.push_back(i_p);
    }

    if (fully_black) {
      m_FullyBlackPatchIDs.push_back(i_p);
    }
  }
  // Create levelset layer data sets
  extractBCellLayers();
}


void LevelSetObject::extractBCellLayers()
{
  // Build 1st and 2nd dimension of cell layer data sets
  m_InnerCellsLayers.resize(m_PatchGrid->getNumPatches());
  m_OuterCellsLayers.resize(m_PatchGrid->getNumPatches());
  for (size_t i_p = 0; i_p < m_PatchGrid->getNumPatches(); i_p++) {
    m_InnerCellsLayers[i_p].resize(m_NumInnerLayers);
    m_OuterCellsLayers[i_p].resize(m_NumOuterLayers);
  }

  /** @todo switch to m_AffectedPatchIDs later. Use direct patch indexing at
    * present for simplicity. This means for non affected patches, empty
    * lists m_InnerCellsLayers[i_p][i_layer][...cells...] and
    * m_OuterCellsLayers[i_p][i_layer][...cells...] */
  //  // Loop for patches affected
  //  for (size_t ii_p = 0; ii_p < m_AffectedPatchIDs.size(); ii_p++) {
  //    size_t i_p = m_AffectedPatchIDs[ii_p];

  // Loop for all patches. Note: direct indexing on patches!
  for (size_t i_p = 0; i_p < m_PatchGrid->getNumPatches(); i_p++) {
    Patch* patch = m_PatchGrid->getPatch(i_p);
    real* var = patch->getVariable(m_FieldIndex, m_VarIndex);
    vector<size_t> ind_cell_neighbours;
    vector<bool> cell_marker;
    cell_marker.resize(patch->variableSize(), false);

    // Need an Eps for g to prevent precision hazards
    /// @todo better Eps handling needed (efficiency)
    real g_max = var[0];
    real g_min = var[0];
    for (size_t l_c = 0; l_c < patch->variableSize(); l_c++) {
      if (g_min > var[l_c]) g_min = var[l_c];
      if (g_max < var[l_c]) g_max = var[l_c];
    }
    real g_eps = (g_max - g_min) * 1.e-7;

    // Find cells, that have at least one face neighbour with different sign of
    // the levelset function. These are 0th layer cells.
    //.. Loop for cells of patch
    for (size_t l_c = 0; l_c < patch->variableSize(); l_c++) {
      real g = var[l_c];
      //.... check face neighbours
      patch->cellOverFaceNeighbours(l_c,
                                    ind_cell_neighbours);
      bool any_other_sign = false;
      for (size_t ll_cn = 0; ll_cn < ind_cell_neighbours.size(); ll_cn++) {
        size_t l_cn = ind_cell_neighbours[ll_cn];
        real test_1 = (var[l_cn] + g_eps) * (g - g_eps);
        real test_2 = (var[l_cn] - g_eps) * (g + g_eps);
        if (test_1 < 0. || test_2 < 0.) {
          any_other_sign = true;
          cell_marker[l_c] = true;
          break;
        }
      }
      if (any_other_sign) {
        LSLayerDataExtrapol lslde_h;
        lslde_h.m_Data = patch->getData();
        lslde_h.m_FieldSize = patch->fieldSize();
        lslde_h.m_VariableSize = patch->variableSize();
        lslde_h.m_Cell = l_c;
        lslde_h.m_G = g;
        if (g < 0.) {
          if(m_NumInnerLayers > 0) { // only if al least a 0th layer is requested
            // m_InnerCellsLayers[i_p][0].push_back(LSLayerDataExtrapol(l_c, g));
            m_InnerCellsLayers[i_p][0].push_back(lslde_h);
          }
        } else {
          if(m_NumOuterLayers > 0) { // only if al least a 0th layer is requested
            m_OuterCellsLayers[i_p][0].push_back(lslde_h);
          }
        }
      }
    }

    // Correct 0th inner layer, if distance from boundary is not sufficient
    // In case of insufficient distance:
    //  => replace cell by all non marked neighbour cells with sufficient distance
    /** @todo When a cell is excluded due to a too short distance to the surface, neighbour cells
      * with higher distance are push_backed at the end of the list. This may cause bad memory
      * access sequences. */
    if(m_MinInnerRelDist > 0.) {
      bool go_on = true;

      size_t count = 0;

      //      while (go_on) {  /// @todo recursion while really needed?
      //        go_on = false;
      size_t ll_c = 0;
      while(ll_c < m_InnerCellsLayers[i_p][0].size()) {
        //        for(size_t ll_c = 0; ll_c < m_InnerCellsLayers[i_p][0].size(); ll_c++) {
        size_t l_c = m_InnerCellsLayers[i_p][0][ll_c].m_Cell;
        //.. find min and max g-values in neighbourship as reference and
        //   compute minimal distance from g=0 (the surface) allowed
        patch->cellOverFaceNeighbours(l_c,
                                      ind_cell_neighbours);
        size_t l_cn0 = ind_cell_neighbours[0];
        real min_g = var[l_cn0];
        real max_g = var[l_cn0];
        for (size_t ll_cn = 1; ll_cn < ind_cell_neighbours.size(); ll_cn++) {
          size_t l_cn = ind_cell_neighbours[ll_cn];
          if(min_g > var[l_cn]) min_g = var[l_cn];
          if(max_g < var[l_cn]) max_g = var[l_cn];
        }
        real g_difference = 0.5*(max_g - min_g); // 0.5 to consider double distance from l_c
        real min_distance = m_MinInnerRelDist * g_difference;
        //.. check distance of cell
        if(var[l_c] > -min_distance) { // must replace
          //.... eliminate cell from list and unmark it
          m_InnerCellsLayers[i_p][0].erase(m_InnerCellsLayers[i_p][0].begin()+ll_c);
          cell_marker[l_c] = false;

          count++;

          //.... check neighbours and insert all the non marked ones with
          //     lower g-value
          for (size_t ll_cn = 0; ll_cn < ind_cell_neighbours.size(); ll_cn++) {
            size_t l_cn = ind_cell_neighbours[ll_cn];
            if(!cell_marker[l_cn]) {
              if(var[l_cn] < var[l_c]) {
                LSLayerDataExtrapol lslde_h;
                lslde_h.m_Data = patch->getData();
                lslde_h.m_FieldSize = patch->fieldSize();
                lslde_h.m_VariableSize = patch->variableSize();
                lslde_h.m_Cell = l_cn;
                lslde_h.m_G = var[l_cn];
                // m_InnerCellsLayers[i_p][0].push_back(LSLayerDataExtrapol(l_cn, var[l_cn]));
                m_InnerCellsLayers[i_p][0].push_back(lslde_h);
                cell_marker[l_cn] = true;
                go_on = true;  // must recheck, in case it is recursively to close to g=0
              }
            }
          }
        } else { // cell remains in list
          ll_c++; // Note: increment ll_c only, if element has not been erased
        }
      }
      cout << "cell replacements: " << count << endl;
      // }
    }
    // Improve memory access sequence
    /** @todo Why sort will not work on LSLayerDataExtrapol::operator< ? */
    //sort(m_InnerCellsLayers[i_p][0].begin(), m_InnerCellsLayers[i_p][0].end());

    // Correct 0th outer layer, if distance from boundary is not sufficient
    // In case of insufficient distance:
    //  => recursively replace cell by non marked neighbour cells until all
    //     layer cells have sufficient distances
    if(m_MinOuterRelDist > 0.) {
      // Not implemented. Ever needed ???
      BUG;
    }

    // Find further cell layers
    //.. Inside
    for (size_t i_layer = 1; i_layer < m_NumInnerLayers; i_layer++) {
      //.... loop for cells in layer below
      size_t below_layer = i_layer - 1;
      for (size_t ll_c = 0; ll_c < m_InnerCellsLayers[i_p][below_layer].size(); ll_c++) {
        size_t l_c = m_InnerCellsLayers[i_p][below_layer][ll_c].m_Cell;
        //...... check face neighbours and insert if these belong to i_layer
        patch->cellOverFaceNeighbours(l_c,
                                      ind_cell_neighbours);
        for (size_t ll_cn = 0; ll_cn < ind_cell_neighbours.size(); ll_cn++) {
          size_t l_cn = ind_cell_neighbours[ll_cn];
          if (!cell_marker[l_cn]) {
            if (var[l_cn] < 0.) { // eventually false due to the eps
              // real g_n = var[l_cn];
              // m_InnerCellsLayers[i_p][i_layer].push_back(LSLayerDataExtrapol(l_cn, g_n));
              LSLayerDataExtrapol lslde_h;
              lslde_h.m_Data = patch->getData();
              lslde_h.m_FieldSize = patch->fieldSize();
              lslde_h.m_VariableSize = patch->variableSize();
              lslde_h.m_Cell = l_cn;
              lslde_h.m_G = var[l_cn];
              m_InnerCellsLayers[i_p][i_layer].push_back(lslde_h);
              cell_marker[l_cn] = true;
            }
          }
        }
      }
    }
    //.. Outside
    for (size_t i_layer = 1; i_layer < m_NumOuterLayers; i_layer++) {
      //.... loop for cells in layer below
      size_t below_layer = i_layer - 1;
      for (size_t ll_c = 0; ll_c < m_OuterCellsLayers[i_p][below_layer].size(); ll_c++) {
        size_t l_c = m_OuterCellsLayers[i_p][below_layer][ll_c].m_Cell;
        //...... check face neighbours and insert if these belong to i_layer
        patch->cellOverFaceNeighbours(l_c,
                                      ind_cell_neighbours);
        for (size_t ll_cn = 0; ll_cn < ind_cell_neighbours.size(); ll_cn++) {
          size_t l_cn = ind_cell_neighbours[ll_cn];
          if (!cell_marker[l_cn]) {
            if (var[l_cn] > 0.) { // eventually false due to the eps
//              real g_n = var[l_cn];
//              m_OuterCellsLayers[i_p][i_layer].push_back(LSLayerDataExtrapol(l_cn, g_n));
              LSLayerDataExtrapol lslde_h;
              lslde_h.m_Data = patch->getData();
              lslde_h.m_FieldSize = patch->fieldSize();
              lslde_h.m_VariableSize = patch->variableSize();
              lslde_h.m_Cell = l_cn;
              lslde_h.m_G = var[l_cn];
              m_OuterCellsLayers[i_p][i_layer].push_back(lslde_h);
              cell_marker[l_cn] = true;
            }
          }
        }
      }
    }

    // Compute levelset gradients and mirror points
    //.. Inside
    for (size_t i_layer = 0; i_layer < m_InnerCellsLayers[i_p].size(); i_layer++) {
      for (size_t ll_c = 0; ll_c < m_InnerCellsLayers[i_p][i_layer].size(); ll_c++) {
        size_t l_c = m_InnerCellsLayers[i_p][i_layer][ll_c].m_Cell;
        vec3_t g_xyz;
        patch->computeNablaVar(m_FieldIndex, m_VarIndex, l_c,
                               g_xyz);
        g_xyz.normalise();
        //        real gx, gy, gz;
        //        patch->computeNablaVar(m_FieldIndex, m_VarIndex, l_c,
        //                               gx, gy, gz);
        m_InnerCellsLayers[i_p][i_layer][ll_c].m_Gx = g_xyz[0];
        m_InnerCellsLayers[i_p][i_layer][ll_c].m_Gy = g_xyz[1];
        m_InnerCellsLayers[i_p][i_layer][ll_c].m_Gz = g_xyz[2];

        //.. Compute mirror points for opposite side data access
        //   Note: local coordinate system of the patch i_p

        /** @todo Switch to a one-click-debug vector3_type and consequently
          *       use it throughout the program. */

        real shift_len = -2. * m_InnerCellsLayers[i_p][i_layer][ll_c].m_G;
        vec3_t mirror_shift = shift_len * g_xyz;
        real xc, yc, zc;
        patch->xyzCell(l_c,
                       xc, yc, zc);
        vec3_t xyz_c(xc, yc, zc);
        vec3_t mirror_xyz = xyz_c + mirror_shift;
        WeightedSet<real> ws;
        patch->computeCCDataInterpolCoeffs_V1(mirror_xyz[0], mirror_xyz[1], mirror_xyz[2],
                                              ws);
        if(ws.tranferToFixedArrays(8,
                                   m_InnerCellsLayers[i_p][i_layer][ll_c].m_MirrorDonor,
                                   m_InnerCellsLayers[i_p][i_layer][ll_c].m_MirrorWeight)) {
          m_InnerCellsLayers[i_p][i_layer][ll_c].m_ExOK = true;
        } else {
          m_InnerCellsLayers[i_p][i_layer][ll_c].m_ExOK = false;
        }
      }
      //.. Clean up m_InnerCellsLayers: Eliminate all entries with
      //   m_InnerCellsLayers[i_p][i_layer][ll_c].m_ExOK == false
      //   These are nodes that extrapolate to "outside" of the core patch bounds. The
      //   variables in these cells will later be overwritten by overlap transfer.
      //   On oblique surfaces crossing patch borders, the extrapolation access may
      //   fail and the cell will suffer from not getting the adequate boundary condition.
      /** Interpolate on total patch grid and not solely on the patch being to avoid the above
        * safety issue.
        * Postpone this operation until having the common data pointer. Then access
        * neighbour patch info directly.*/
      vector<size_t> shift_down;
      shift_down.resize(m_InnerCellsLayers[i_p][i_layer].size());
      size_t all_shift_down = 0;
      for (size_t ll_c = 0; ll_c < m_InnerCellsLayers[i_p][i_layer].size(); ll_c++) {
        shift_down[ll_c] = all_shift_down;
        if (!m_InnerCellsLayers[i_p][i_layer][ll_c].m_ExOK) {
          all_shift_down++;
        }
      }
      for (size_t ll_c = 0; ll_c < m_InnerCellsLayers[i_p][i_layer].size(); ll_c++) {
        size_t ll_c_down = ll_c - shift_down[ll_c];
        m_InnerCellsLayers[i_p][i_layer][ll_c_down] = m_InnerCellsLayers[i_p][i_layer][ll_c];
      }
      size_t old_size = m_InnerCellsLayers[i_p][i_layer].size();
      size_t new_size = old_size - all_shift_down;
      m_InnerCellsLayers[i_p][i_layer].resize(new_size);
#ifdef DEBUG
      bool error = false;
      for (size_t ll_c = 0; ll_c < m_InnerCellsLayers[i_p][i_layer].size(); ll_c++) {
        if (!m_InnerCellsLayers[i_p][i_layer][ll_c].m_ExOK) {
          error = true;
        }
      }
      if(error){BUG;}
#endif
    }

    //.. Outside
    for (size_t i_layer = 0; i_layer < m_OuterCellsLayers[i_p].size(); i_layer++) {
      for (size_t ll_c = 0; ll_c < m_OuterCellsLayers[i_p][i_layer].size(); ll_c++) {
        size_t l_c = m_OuterCellsLayers[i_p][i_layer][ll_c].m_Cell;
        vec3_t g_xyz;
        patch->computeNablaVar(m_FieldIndex, m_VarIndex, l_c,
                               g_xyz);
        g_xyz.normalise();
        //        real gx, gy, gz;
        //        patch->computeNablaVar(m_FieldIndex, m_VarIndex, l_c,
        //                               gx, gy, gz);
        m_OuterCellsLayers[i_p][i_layer][ll_c].m_Gx = g_xyz[0];
        m_OuterCellsLayers[i_p][i_layer][ll_c].m_Gy = g_xyz[1];
        m_OuterCellsLayers[i_p][i_layer][ll_c].m_Gz = g_xyz[2];
      }
    }
  }
}
