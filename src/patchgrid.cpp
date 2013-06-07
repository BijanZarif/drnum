#include <unistd.h>
#include "patchgrid.h"

PatchGrid::PatchGrid(size_t num_protectlayers, size_t num_overlaplayers)
{
  // patchgroups of same type and solver codes
  m_PatchGroups = new PatchGroups(true); /// @todo need a handling for this
  // parametric settings
  m_NumProtectLayers = num_protectlayers;
  m_NumOverlapLayers = num_overlaplayers;
  // default settings
  m_NumFields = 0;
  m_NumVariables = 0;
  m_InterpolateData = false;
  m_InterpolateGrad1N = false;
  m_BboxOk = false;
  //  m_hashBox = NULL;
}

void  PatchGrid::setNumberOfFields(size_t num_fields)
{
  m_NumFields = num_fields;
}

void  PatchGrid::setNumberOfVariables(size_t num_variables)
{
  m_NumVariables = num_variables;
}

void PatchGrid::setInterpolateData(bool interpolatedata)
{
  m_InterpolateData = interpolatedata;
}


void PatchGrid::setInterpolateGrad1N(bool interpolategrad1N)
{
  m_InterpolateGrad1N = interpolategrad1N;
}


void PatchGrid::setTransferPadded(bool trans_padded)
{
  m_TransferPadded = trans_padded;
}


void PatchGrid::setNumProtectLayers(size_t num_protectlayers)
{
  m_NumProtectLayers = num_protectlayers;
}


void PatchGrid::setNumOverlapLayers(size_t num_overlaplayers)
{
  m_NumOverlapLayers = num_overlaplayers;
}


PatchGrid::~PatchGrid()
{
  /// @todo missing destructor. Might block mem needed for later computations.
  //  delete m_patchdependencies;  /// @todo delete method available for TInsectionList?
  //  delete m_patches;
  //  delete m_HashRaster;
}


void PatchGrid::computeDependencies(const bool& with_intercoeff)
{
  /// @todo Some performance issues in this member function. Might be optimized, if needed.

  /// @todo Needs an error handling mechanism, at least a diagnose, if any receiving cell of a patch finds no donor at all.

  /** @todo Hash search relates to outer nodes of the patches rather than to the outer cell centers. This is not
    * a problem, but might causes some extra neighbour checks, see below 4) since the nodal range is larger. */

  // Do the following
  //
  // 1) Build up a hash raster VectorHashRaster<size_t> as background grid, covering the whole geometric
  //    region of the mesh.
  //    PERF-NOTE: a) Most likely Splittree or Octree search algorithm would enhance performace, if ever needed.
  //               b) VectorHashRaster might be slow, due to large vector<vector<T> > implementation.
  //
  // 2) Fill in patch indicees. Save side inclusion: mark all hash raster cells covered by the bounding box
  //    of respective patches.
  //    PERF-NOTE: Very save side inclusion of patches on boxes of hash raster. might be improved, if needed.
  //
  // 3) Find potential neighbours on hash raster. If several patches have hit the same raster box, these patches
  //    are potentially depenedent from each other. Store potential dependencies on pot_neigh.
  //
  // 4) Find real neighbour dependencies (e.g. interpolation partners) for all potentially dependend patches.
  //    - If a potential neighbour-dependency does not serve any interpol request, it will be excluded
  //      from Patch::m_neighbours.
  //
  // 5) Finish building up inter-patch transfer lists.

  if(!with_intercoeff) {
    // with_intercoeff==false is an intended option for grid gen purposes to be used later.
    BUG;
  }

  vector<vector<size_t> > pot_neigh;
  pot_neigh.resize(m_Patches.size());

  // 1) Build up a hash raster VectorHashRaster<size_t> as background grid, covering the whole geometric
  //    region of the mesh.
  { // start mem_block
    VectorHashRaster<size_t> m_HashRaster;
    buildHashRaster(10*m_Patches.size(), true,   // resolution chosen upon testing, may also use fixed size
                    m_HashRaster);

    // 2) Fill in patch indicees. Save side inclusion: mark all hash raster cells covered by the bounding box
    //    of respective patches.
    for (size_t i_p = 0; i_p < m_Patches.size(); i_p++) {
      //.. Find cell address range in hash raster covered by bounding box of patch
      //.... min-max in inertial coords
      vec3_t xyzo_min = m_Patches[i_p]->accessBBoxXYZoMin();
      vec3_t xyzo_max = m_Patches[i_p]->accessBBoxXYZoMax();
      //.... min-max in m_HashRaster coords in
      vec3_t xyz_min = m_HashRaster.getTransformI2T()->transform(xyzo_min);
      vec3_t xyz_max = m_HashRaster.getTransformI2T()->transform(xyzo_max);
      size_t ic_min, jc_min, kc_min;
      size_t ic_max, jc_max, kc_max;
      bool inside_min, inside_max;
      inside_min = m_HashRaster.xyzToRefNode(xyz_min[0], xyz_min[1], xyz_min[2],
                                             ic_min, jc_min, kc_min);
      inside_max = m_HashRaster.xyzToRefNode(xyz_max[0], xyz_max[1], xyz_max[2],
                                             ic_max, jc_max, kc_max);
      //#ifdef DEBUG
      // Folowing should never happen, check anyway
      if(!inside_min || !inside_max) {
        BUG;
      }
      //#endif
      //.. Insert patch index into all boxes of hash raster covered by bounding box of patch
      for (size_t ic_r = ic_min; ic_r <= ic_max; ic_r++) {
        for (size_t jc_r = jc_min; jc_r <= jc_max; jc_r++) {
          for (size_t kc_r = kc_min; kc_r <= kc_max; kc_r++) {
            m_HashRaster.insert(ic_r, jc_r, kc_r, i_p);
          }
        }
      }
    }

    // 3) Find potential neighbours on hash raster. If several patches have hit the same raster box, these patches
    //    are potentially depenedent from each other. Store potential dependencies on pot_neigh.
    // vector<vector<size_t> > pot_neigh;
    // pot_neigh.resize(m_patches.size());
    for (size_t l_r = 0; l_r < m_HashRaster.sizeL(); l_r++) {
      size_t n_patches_in_box = m_HashRaster.getNumItems(l_r);
      if (n_patches_in_box > 1) {
        for (size_t ll_p1 = 0; ll_p1 < (n_patches_in_box - 1); ll_p1++) {  // double loop for vice-versa insertion
          for (size_t ll_p2 = (ll_p1 + 1); ll_p2 < n_patches_in_box; ll_p2++) {
            size_t patch_1 = m_HashRaster.at(l_r, ll_p1);
            size_t patch_2 = m_HashRaster.at(l_r, ll_p2);
            if(patch_1 != patch_2) {
              pot_neigh[patch_1].push_back(patch_2);
              pot_neigh[patch_2].push_back(patch_1);
            } else {
              BUG;
            }
          }
        }
        //.. keep mem low: remove duplicates
        for (size_t ll_p = 0; ll_p < m_HashRaster.getNumItems(l_r); ll_p++) {
          size_t patch = m_HashRaster.at(l_r, ll_p);
          if(pot_neigh[patch].size() > 100) {  // try to avoid unifying over and over
            // unify(pot_neigh[patch]);
            sort(pot_neigh[patch].begin(), pot_neigh[patch].end());
            typename vector<size_t>::iterator it;
            it = unique(pot_neigh[patch].begin(), pot_neigh[patch].end());
            pot_neigh[patch].resize(it - pot_neigh[patch].begin());
          }
        }
      }
    }
  } // end mem_block
  // unify all potential neighbour lists
  for (size_t patch = 0; patch < m_Patches.size(); patch++) {
    sort(pot_neigh[patch].begin(), pot_neigh[patch].end());
    typename vector<size_t>::iterator it;
    it = unique(pot_neigh[patch].begin(), pot_neigh[patch].end());
    pot_neigh[patch].resize(it - pot_neigh[patch].begin());
    vector<size_t>(pot_neigh[patch]).swap(pot_neigh[patch]);
  }
  // 4) Find real neighbour dependencies (e.g. interpolation partners) for all potentially dependend patches.
  //    - If a potential neighbour-dependency does not serve any interpol request, it will be excluded
  //      from Patch::m_neighbours.
  for (size_t i_p = 0; i_p < m_Patches.size(); i_p++) {
    for (size_t ii_pn = 0; ii_pn < pot_neigh[i_p].size(); ii_pn++) {
      size_t i_pn = pot_neigh[i_p][ii_pn];
      m_Patches[i_p]->insertNeighbour(m_Patches[i_pn]);
    }
  }
  // 5) Finish building up inter-patch transfer lists.
  /// @todo Needs an error handling mechanism, at least a diagnose, if any receiving cell of a patch finds no donor at all.
  finalizeDependencies();
  m_DependenciesOk = true;
}


void PatchGrid::finalizeDependencies()
{
  for (size_t i_p = 0; i_p < m_Patches.size(); i_p++) {
    m_Patches[i_p]->finalizeDependencies();
  }
}


void PatchGrid::accessAllDonorData_WS(const size_t& field)
{
  for (size_t i_p = 0; i_p < m_Patches.size(); i_p++) {
    m_Patches[i_p]->accessDonorData_WS(field);
  }
}


void PatchGrid::buildHashRaster(size_t resolution, bool force,
                                VectorHashRaster<size_t>& m_HashRaster)
{
  // Build a bounding box
  buildBoundingBox(force);

  // Define resolution. Aim approx. same delatas in x, y, z .
  vec3_t delta_xyzo = m_BboxXyzoMax - m_BboxXyzoMin;
  real delta_resolve = pow(real(resolution) / (delta_xyzo[0] * delta_xyzo[1] * delta_xyzo[2]), (1./3.));
  real resolve_xo = delta_resolve * delta_xyzo[0];
  real resolve_yo = delta_resolve * delta_xyzo[1];
  real resolve_zo = delta_resolve * delta_xyzo[2];
  size_t i_np = size_t(resolve_xo);
  size_t j_np = size_t(resolve_yo);
  size_t k_np = size_t(resolve_zo);
  if(i_np < 1) {i_np = 1;}
  if(j_np < 1) {j_np = 1;}
  if(k_np < 1) {k_np = 1;}
  m_HashRaster.setUp(m_BboxXyzoMin[0], m_BboxXyzoMin[1], m_BboxXyzoMin[2],
                     m_BboxXyzoMax[0], m_BboxXyzoMax[1], m_BboxXyzoMax[2],
                     i_np, j_np, k_np);

}


size_t PatchGrid::insertPatch(Patch* new_patch)
{
  // Hand over general attributes
  setGeneralAttributes(new_patch);
  // Insert in list
  m_Patches.push_back(new_patch);
  m_DependenciesOk = false;  /// @todo global logics on dependencies?
  return m_Patches.size() - 1;
}


void PatchGrid::setGeneralAttributes(Patch* patch)
{
  patch->setNumberOfFields(m_NumFields);
  patch->setNumberOfVariables(m_NumVariables);
  patch->setNumOverlapLayers(m_NumOverlapLayers);
  patch->setNumProtectLayers(m_NumProtectLayers);
  patch->setInterpolateData(m_InterpolateData);
  patch->setInterpolateGrad1N(m_InterpolateGrad1N);
  patch->setTransferPadded(m_TransferPadded);
}


void PatchGrid::readGrid(string gridfilename)
{
  // Get file path
  /** @todo Use a better file path definition. */
  char base_files_path[4096];
  char grid_file[4096];
  if(getcwd(base_files_path, sizeof(base_files_path)) == NULL) {
    cout << " Could not get current working directory" << endl;
    BUG;
  };
  strcpy(grid_file, base_files_path);
  strcat(grid_file, "/");
  strcat(grid_file, gridfilename.c_str());

  // Open grid file
  ifstream s_grid;
  s_grid.open(grid_file);
  if (s_grid.fail()) {
    cout << " faild to open grid file:" << grid_file << endl;
    BUG;
  };

  // Say something
  cout << "Reading PatchGrid::readGrid() from file " << grid_file << " ... ";
  /** @todo Preliminary format. */

  // Read file contents
  while(!s_grid.eof()) {
    //.. Read int-code of patch
    size_t patch_type;
    s_grid >> patch_type;
    // End marker
    if(patch_type == 0) {
      break;
    }
    //.. Read contents for patch between delimiting { }
    char c;
    string patchcomment;
    while(s_grid.get(c)) {
      if(c=='{') {
        break;
      }
      else {
        patchcomment.push_back(c);
      }
    }
    string line;
    {
      string read_line;
      getline(s_grid, read_line, '}');
      line.clear();
      for (size_t i_read_line = 0; i_read_line < read_line.size(); ++i_read_line) {
        char c = read_line[i_read_line];
        if (isspace(c)) {
          c = ' ';
        }
        line.push_back(c);
      }
    }
    istringstream iss(line);
    //.. Hand over to patches as needed
    //.... CartesianPatch
    if (patch_type == 1001) {
      //...... Create a new CartesianPatch
      CartesianPatch* new_patch;
      new_patch = new CartesianPatch();
      size_t index = insertPatch(new_patch);
      new_patch->setIndex(index);
      new_patch->setPatchComment(patchcomment);
      new_patch->readFromFile(iss);
      m_PatchGroups->insertPatch(new_patch); /// @todo better outside of reading loop?
    }
    //.... Unstructured Patch
    else if(patch_type == 1010) {
      // ...
    }
    else {
      cout << "unknown patch type code" << patch_type << endl;
      BUG;
    }
  }
  cout << "done. " << endl;
}

void PatchGrid::writeData(QString base_data_filename, int count)
{
  QString str_patch_index;
  QString str_patch_filename;
  // loop for patches iof patch_grid
  for (size_t i_p = 0; i_p < getNumPatches(); i_p++) {
    str_patch_index.setNum(i_p);
    while (str_patch_index.size() < 6) {
      str_patch_index = "0" + str_patch_index;
    }
    str_patch_filename = base_data_filename + "_ip" + str_patch_index;
    getPatch(i_p)->writeData(str_patch_filename, count);
  }
}


void PatchGrid::scaleRefParental(real scfactor)
{
  for (vector<Patch*>::iterator p = m_Patches.begin(); p != m_Patches.end(); p++) {
    (*p)->scaleRefParental(scfactor);
  }
}


void PatchGrid::buildBoundingBox(const bool& force)
{
  if(!m_BboxOk || force) {
    if(m_Patches.size() > 0) {
      // Find max-min limits of all patches in this grid
      m_BboxXyzoMin = m_Patches[0]->accessBBoxXYZoMin();
      m_BboxXyzoMax = m_Patches[0]->accessBBoxXYZoMax();
      for (size_t i_p = 1; i_p < m_Patches.size(); i_p++) {
        vec3_t bbmin_h = m_Patches[i_p]->accessBBoxXYZoMin();
        vec3_t bbmax_h = m_Patches[i_p]->accessBBoxXYZoMax();
        if(m_BboxXyzoMin[0] > bbmin_h[0]) m_BboxXyzoMin[0] = bbmin_h[0];
        if(m_BboxXyzoMin[1] > bbmin_h[1]) m_BboxXyzoMin[1] = bbmin_h[1];
        if(m_BboxXyzoMin[2] > bbmin_h[2]) m_BboxXyzoMin[2] = bbmin_h[2];
        if(m_BboxXyzoMax[0] < bbmax_h[0]) m_BboxXyzoMax[0] = bbmax_h[0];
        if(m_BboxXyzoMax[1] < bbmax_h[1]) m_BboxXyzoMax[1] = bbmax_h[1];
        if(m_BboxXyzoMax[2] < bbmax_h[2]) m_BboxXyzoMax[2] = bbmax_h[2];
        ///< @todo A max/min function for vec3_t ?
        // m_bbox_xyzo_min.coordMin(bbmin_h);
        // m_bbox_xyzo_max.coordMin(bbmax_h);
      }
      m_BboxOk = true;
    }
  }
}


void PatchGrid::setFieldToConst(size_t i_field, real *var)
{
  for (size_t i_p = 0; i_p < m_Patches.size(); i_p++) {
    m_Patches[i_p]->setFieldToConst(i_field, var);
  }
}


SinglePatchGroup* PatchGrid::getSinglePatchGroup(const size_t& ipg)
{
  return m_PatchGroups->accessSinglePatchGroup(ipg);
}

real PatchGrid::computeMinChLength()
{
  real min_ch_len_all = 0;
  bool first = true;
  for (size_t i_p = 0; i_p < m_Patches.size(); i_p++) {
    real min_ch_len = m_Patches[i_p]->computeMinChLength();
    if(first || min_ch_len<min_ch_len_all) {
      min_ch_len_all = min_ch_len;
    }
    first = false;
  }
  return min_ch_len_all;
}

