/**********************************************************************/
/*           Static Test Compression:                                 */
/*           statically compress the pattern list;                    */
/*                                                                    */
/*           Author: Hsiang-Ting Wen, Fu-Yu Chuang, and Chen-Hao Hsu  */
/*           Date : 12/27/2018 created                                */
/**********************************************************************/

#include "atpg.h"

#define RANDOM_SIMULATION_ITER_NUM  5

void ATPG::static_compression() {
  compatibility_graph();
  fill_x();
  random_simulation();
}

void ATPG::fill_x() {
  int i, len;
  len = cktin.size() + 1;
  for (string& vec : vectors) {
    assert(vec.size() == len);
    for (i = 0; i < len; ++i) {
      if (vec[i] == 'x') {
        vec[i] = (rand() & 01) ? '1' : '0';
      }
    }
  }
}

void ATPG::compatibility_graph() {

  // typedef __gnu_pbds::priority_queue<Edge*, , __gnu_pbds::thin_heap_tag> PQueue;

  int i, j, nvec;
  nvec = vectors.size();
  for (i = 0; i < nvec; ++i) {
    for (j = i + 1; j < nvec; ++j) {
      if (isCompatible(vectors[i], vectors[j])) {

      }
    }
  }

}

bool ATPG::isCompatible(const string& vec1, const string& vec2) const {
  assert(vec1.size() == vec2.size());
  int i, len;
  len = vec1.size();
  for (i = 0; i < len; ++i) {
    if ((vec1[i] == '1' && vec2[i] == '0') || (vec1[i] == '0' && vec2[i] == '1')) {
      return false;
    }
  }
  return true;
}


/* static test compression */
void ATPG::random_simulation()  
{
  int detect_num = 0;
  int iter;

  vector<int> order(vectors.size());
  vector<bool> dropped(vectors.size(), false);
  iota(order.begin(), order.end(), 0);
  reverse(order.begin(), order.end());

  fprintf(stderr, "# Static compression:\n");
  int drop_count, total_drop_count;
  total_drop_count = 0;
  for (iter = 0; iter < RANDOM_SIMULATION_ITER_NUM; ++iter) {
    drop_count = 0;
    generate_fault_list();
    for (const int s : order) {
      if (!dropped[s]) {
        detect_num = 0;
        tdfsim_a_vector(vectors[s], detect_num);
        if (detect_num == 0) {
          dropped[s] = true;
          ++drop_count;
        }
      }
    }
    total_drop_count += drop_count;
    fprintf(stderr, "#   Iter %d: drop %d vector(s). (%d)\n", iter, drop_count, total_drop_count);
    random_shuffle(order.begin(), order.end());
  }

  iota(order.begin(), order.end(), 0);
  detect_num = 0;
  generate_fault_list();
  for (const int s : order) {
    if (!dropped[s]) {
      tdfsim_a_vector(vectors[s], detect_num);
      if (detect_num == 0) {
        dropped[s] = true;
        ++drop_count;
      }
    }
  }
  total_drop_count += drop_count;
  fprintf(stderr, "#   Iter %d: drop %d vector(s). (%d)\n", iter, drop_count, total_drop_count);

  vector<string> compressed_vectors;
  for (size_t i = 0; i < vectors.size(); ++i) {
    if (!dropped[i]) {
      compressed_vectors.emplace_back(vectors[i]);
    }
  }
  vectors = compressed_vectors;
}



