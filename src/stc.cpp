/**********************************************************************/
/*           Static Test Compression:                                 */
/*           statically compress the pattern list;                    */
/*                                                                    */
/*           Author: Hsiang-Ting Wen, Fu-Yu Chuang, and Chen-Hao Hsu  */
/*           Date : 12/27/2018 created                                */
/**********************************************************************/

#include "atpg.h"
#include "disjointSet.h"

#define RANDOM_SIMULATION_ITER_NUM  5

void ATPG::static_compression() {
  compatibility_graph();
  // fill_x();
  // random_simulation();
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

  typedef __gnu_pbds::priority_queue<Edge, Edge_Cmp, __gnu_pbds::thin_heap_tag>          PQueue;
  typedef PQueue::point_iterator PQueue_Iter;

  int nvec = vectors.size();

  vector<Node> vNodes(0); /* all nodes are here; vNodes.size() == vectors.size() */
  vector<Edge> vEdges(0); /* all edges are here; */
  DisjointSet ds(nvec);  /* use for supernodes */
  assert(vNodes.empty() && vEdges.empty());

  /* initialize vNodes; create a node for each test pattern; 
     idx is the same as that in vectors */
  for (int i = 0; i < nvec; ++i) {
    vNodes.emplace_back(i);
  }

  /* build edges for two compatible nodes */
  int nEdges = 0;
  for (int i = 0; i < nvec; ++i) {
    for (int j = i + 1; j < nvec; ++j) {
      if (isCompatible(vectors[i], vectors[j])) {
        vEdges.emplace_back(nEdges, i, j);
        vNodes[i].mNeighbors[j] = nEdges;
        vNodes[j].mNeighbors[i] = nEdges;
        ++nEdges;
      }
    }
  }

/*
  for (int i = 0, n = vNodes.size(); i < n; ++i) {
    fprintf(stderr, "[%d]\n", i);
    for (auto& p : vNodes[i].mNeighbors) {
      fprintf(stderr, "n%d => e%d (n%d, n%d)\n", p.first, p.second, 
              vEdges[p.second].node_pair.first, vEdges[p.second].node_pair.second);
    }
    fprintf(stderr, "\n");
  }
*/

  auto cal_common_neighbor_set = [&] (const Edge& e, set<int>& s) -> void {
    const map<int, int>& mLarger = vNodes[e.n1].mNeighbors.size() > vNodes[e.n2].mNeighbors.size() ? 
                                   vNodes[e.n1].mNeighbors : vNodes[e.n2].mNeighbors;
    const map<int, int>& mSmaller = vNodes[e.n1].mNeighbors.size() > vNodes[e.n2].mNeighbors.size() ? 
                                    vNodes[e.n2].mNeighbors : vNodes[fe.n1].mNeighbors;
    s.clear();
    for (const auto& p : mSmaller)
      if (mLarger.count(p.first) > 0)
        s.insert(p.first)
  };

  auto cal_common_neighbor_num = [&] (Edge& e) -> int {
    const map<int, int>& mLarger = vNodes[e.n1].mNeighbors.size() > vNodes[e.n2].mNeighbors.size() ? 
                                   vNodes[e.n1].mNeighbors : vNodes[e.n2].mNeighbors;
    const map<int, int>& mSmaller = vNodes[e.n1].mNeighbors.size() > vNodes[e.n2].mNeighbors.size() ? 
                                    vNodes[e.n2].mNeighbors : vNodes[fe.n1].mNeighbors;
    int ret = 0;
    for (const auto& p : mSmaller)
      if (mLarger.count(p.first) > 0)
        ++ret;
    e.nCommon = ret;
    return ret;
  };

  for (int i = 0, n = vEdges.size(); i < n; ++i) {
    fprintf(stderr, "e%d (n%d, n%d): %d\n", i, vEdges[i].n1, vEdges[i].n2, cal_common_neighbor_num(vEdges[i]));
  }

  PQueue pq;

  vector<PQueue_Iter> vIterators(vEdges.size());
  for (int i = 0, n = vEdges.size(); i < n; ++i) {
    cal_common_neighbor_num(vEdges[i]);
    vIterators[i] = pq.push(vEdges[i]);
  }

  while (!pq.empty()) {
    /* find the edge with largest set of common neighbors */
    Edge& top_edge = vEdges[pq.top().edge_idx];
    pq.pop();
    const int n1 = top_edge.n1;
    const int n2 = top_edge.n2;
    Node& node1 = vNodes[n1];
    Node& node2 = vNodes[n2];

    /* set of common neighbors of n1 and n2 */ 
    set<int> sCommonNeighbors;
    cal_common_neighbor_set(top_edge, sCommonNeighbors);

    /* s <- i union j */
    const int del_n = node1.mNeighbors.size() > node2.mNeighbors.size() ? node2 : node1;
    Node& new_node = node1.mNeighbors.size() > node2.mNeighbors.size() ? node1 : node2;

    /* update common-neighbor edges */
    for (const int common_n : sCommonNeighbors) {
      Node& common_node = vNodes[common_n];
      assert(common_node.mNeighbors.count(n1) > 0);
      assert(common_node.mNeighbors.count(n2) > 0);
      common_node.mNeighbors.erase(del_n);
      pq.modify()

    }


    /* remove non-common-neighbor edges*/
    for (const auto& p : node1.mNeighbors) {
      if (sCommonNeighbors.count(p.first) == 0) {
        vNodes[p.first].mNeighbors.erase(n1);
        pq.erase(vIterators[p.second]);
      }
    }
    for (const auto& p : node2.mNeighbors) {
      if (sCommonNeighbors.count(p.first) == 0) {
        vNodes[p.first].mNeighbors.erase(n2);
        pq.erase(vIterators[p.second]);
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



