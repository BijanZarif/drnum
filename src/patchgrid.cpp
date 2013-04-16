#include <unistd.h>
#include "patchgrid.h"

PatchGrid::PatchGrid(size_t num_protectlayers, size_t num_overlaplayers)
{
  // patchgroups of same type and solver codes
  m_patchgroups = new PatchGroups(true); /// @todo need a handling for this
  // parametric settings
  m_NumProtectLayers = num_protectlayers;
  m_NumOverlapLayers = num_overlaplayers;
  // default settings
  m_NumFields = 0;
  m_NumVariables = 0;
  m_InterpolateData = false;
  m_InterpolateGrad1N = false;
  m_bbox_OK = false;
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

  vector<vector<size_t> > pot_neigh;
  pot_neigh.resize(m_patches.size());

  // 1) Build up a hash raster VectorHashRaster<size_t> as background grid, covering the whole geometric
  //    region of the mesh.
  { // start mem_block
    VectorHashRaster<size_t> m_HashRaster;
    buildHashRaster(10*m_patches.size(), true,   // resolution chosen upon testing, may also use fixed size
                    m_HashRaster);

    // 2) Fill in patch indicees. Save side inclusion: mark all hash raster cells covered by the bounding box
    //    of respective patches.
    for (size_t i_p = 0; i_p < m_patches.size(); i_p++) {
      //.. Find cell address range in hash raster covered by bounding box of patch
      //.... min-max in inertial coords
      vec3_t xyzo_min = m_patches[i_p]->accessBBoxXYZoMin();
      vec3_t xyzo_max = m_patches[i_p]->accessBBoxXYZoMax();
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
  for (size_t patch = 0; patch < m_patches.size(); patch++) {
    sort(pot_neigh[patch].begin(), pot_neigh[patch].end());
    typename vector<size_t>::iterator it;
    it = unique(pot_neigh[patch].begin(), pot_neigh[patch].end());
    pot_neigh[patch].resize(it - pot_neigh[patch].begin());
    vector<size_t>(pot_neigh[patch]).swap(pot_neigh[patch]);
  }
  // 4) Find real neighbour dependencies (e.g. interpolation partners) for all potentially dependend patches.
  //    - If a potential neighbour-dependency does not serve any interpol request, it will be excluded
  //      from Patch::m_neighbours.
  for (size_t i_p = 0; i_p < m_patches.size(); i_p++) {
    for (size_t ii_pn = 0; ii_pn < pot_neigh[i_p].size(); ii_pn++) {
      size_t i_pn = pot_neigh[i_p][ii_pn];
      m_patches[i_p]->insertNeighbour(m_patches[i_pn]);
    }
  }
  // 5) Finish building up inter-patch transfer lists.
  /// @todo Needs an error handling mechanism, at least a diagnose, if any receiving cell of a patch finds no donor at all.
  finalizeDependencies();
  m_dependencies_OK = true;
}


void PatchGrid::finalizeDependencies()
{
  for (size_t i_p = 0; i_p < m_patches.size(); i_p++) {
    m_patches[i_p]->finalizeDependencies();
  }
}


void PatchGrid::accessAllDonorData_WS(const size_t& field)
{
  for (size_t i_p = 0; i_p < m_patches.size(); i_p++) {
    m_patches[i_p]->accessDonorData_WS(field);
  }
}


void PatchGrid::buildHashRaster(size_t resolution, bool force,
                                VectorHashRaster<size_t>& m_HashRaster)
{
  // Build a bounding box
  buildBoundingBox(force);

  // Define resolution. Aim approx. same delatas in x, y, z .
  vec3_t delta_xyzo = m_bbox_xyzo_max - m_bbox_xyzo_min;
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
  m_HashRaster.setUp(m_bbox_xyzo_min[0], m_bbox_xyzo_min[1], m_bbox_xyzo_min[2],
                     m_bbox_xyzo_max[0], m_bbox_xyzo_max[1], m_bbox_xyzo_max[2],
                     i_np, j_np, k_np);

}


size_t PatchGrid::insertPatch(Patch* new_patch)
{
  // Hand over general attributes
  setGeneralAttributes(new_patch);
  // Insert in list
  m_patches.push_back(new_patch);
  m_dependencies_OK = false;  /// @todo global logics on dependencies?
  return m_patches.size() - 1;
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


void PatchGrid::deletePatch(size_t i_patch)
{
  BUG;
  /** @todo Other patches may be dependent. These dependencies must be cleared.
    * Most likely this method will never be required, unless for mesh generation purposes.
    */
}



void PatchGrid::readGrid()
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
  strcat(grid_file,"/grid/patches");
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
  while(!s_grid.eof()) {
    // Read int-code of patch
    size_t patch_type;
    s_grid >> patch_type;
    // End marker
    if(patch_type == 0) {
      break;
    }
    // Read contents for patch between delimiting { }
    char c;
    string patchcomment;
    while(s_grid.get(c)) {
      if(c=='{') {
        break;
      }
      else {
        patchcomment.push_back(c);
        // cout << c;
      }

//      if(c==' ' || c=='\n') {}
//      else if(c=='{') {
//        break;
//      }
//      else {
//        cout << "wrong grid file structure" << endl;
//        // BUG;
//      }
    }
    string line;
    getline(s_grid, line, '}');
    istringstream iss(line);
    // Hand over to patches as needed
    //.. CartesianPatch
    if(patch_type == 1001) {
      //.... Create a new CartesianPatch
      CartesianPatch* new_patch;
      new_patch = new CartesianPatch();
      size_t index = insertPatch(new_patch);
      new_patch->setIndex(index);
      new_patch->setPatchComment(patchcomment);
      new_patch->readFromFile(iss);
      m_patchgroups->insertPatch(new_patch);
    }
    // Unstructured Patch
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

void PatchGrid::scaleRefParental(real scfactor)
{
  for (vector<Patch*>::iterator p = m_patches.begin(); p != m_patches.end(); p++) {
    (*p)->scaleRefParental(scfactor);
  }
}

void PatchGrid::buildBoundingBox(const bool& force)
{
  if(!m_bbox_OK || force) {
    if(m_patches.size() > 0) {
      // Find max-min limits of all patches in this grid
      m_bbox_xyzo_min = m_patches[0]->accessBBoxXYZoMin();
      m_bbox_xyzo_max = m_patches[0]->accessBBoxXYZoMax();
      for (size_t i_p = 1; i_p < m_patches.size(); i_p++) {
        vec3_t bbmin_h = m_patches[i_p]->accessBBoxXYZoMin();
        vec3_t bbmax_h = m_patches[i_p]->accessBBoxXYZoMax();
        if(m_bbox_xyzo_min[0] > bbmin_h[0]) m_bbox_xyzo_min[0] = bbmin_h[0];
        if(m_bbox_xyzo_min[1] > bbmin_h[1]) m_bbox_xyzo_min[1] = bbmin_h[1];
        if(m_bbox_xyzo_min[2] > bbmin_h[2]) m_bbox_xyzo_min[2] = bbmin_h[2];
        if(m_bbox_xyzo_max[0] < bbmax_h[0]) m_bbox_xyzo_max[0] = bbmax_h[0];
        if(m_bbox_xyzo_max[1] < bbmax_h[1]) m_bbox_xyzo_max[1] = bbmax_h[1];
        if(m_bbox_xyzo_max[2] < bbmax_h[2]) m_bbox_xyzo_max[2] = bbmax_h[2];
        ///< @todo A max/min function for vec3_t ?
        // m_bbox_xyzo_min.coordMin(bbmin_h);
        // m_bbox_xyzo_max.coordMin(bbmax_h);
      }
      m_bbox_OK = true;
    }
  }
}

void PatchGrid::setFieldToConst(size_t i_field, real *var)
{
  for (size_t i_p = 1; i_p < m_patches.size(); i_p++) {
    m_patches[i_p]->setFieldToConst(i_field, var);
  }
}
