/**********************************************************************/
/*           Static Test Compression:                                 */
/*           statically compress the pattern list;                    */
/*                                                                    */
/*           Author: Hsiang-Ting Wen, Fu-Yu Chuang, and Chen-Hao Hsu  */
/*           Date : 12/27/2018 created                                */
/**********************************************************************/

#include "atpg.h"
#include "disjointSet.h"

#define RANDOM_SIMULATION_ITER_NUM  50

void ATPG::static_compression() {
  random_simulation();
  compatibility_graph();

  expand_vectors(19);

  random_simulation();
  random_fill_x();
  random_simulation();
}

/* Tseng-Siewiorek algorithm: solve minimum clique partition problem on compatibility graphs 
   return the number of nodes in the maximum super-node */
void ATPG::compatibility_graph() {
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
    for (int j = i + 1; j < min(nvec, i + 300); ++j) {
      if (isCompatible(vectors[i], vectors[j])) {
        vEdges.emplace_back(nEdges, i, j);
        vNodes[i].mNeighbors[j] = nEdges;
        vNodes[j].mNeighbors[i] = nEdges;
        ++nEdges;
      }
    }
  }

  auto cal_common_neighbor_set = [&] (const Edge& e, set<int>& s) -> void {
    assert(e.n1 != e.n2);
    const map<int, int>& mLarger = vNodes[e.n1].mNeighbors.size() > vNodes[e.n2].mNeighbors.size() ? 
                                   vNodes[e.n1].mNeighbors : vNodes[e.n2].mNeighbors;
    const map<int, int>& mSmaller = vNodes[e.n1].mNeighbors.size() > vNodes[e.n2].mNeighbors.size() ? 
                                    vNodes[e.n2].mNeighbors : vNodes[e.n1].mNeighbors;
    s.clear();
    for (const auto& p : mSmaller)
      if (mLarger.count(p.first) > 0)
        s.insert(p.first);
  };

  auto cal_common_neighbor_num = [&] (Edge& e) -> int {
    const map<int, int>& mLarger = vNodes[e.n1].mNeighbors.size() > vNodes[e.n2].mNeighbors.size() ? 
                                   vNodes[e.n1].mNeighbors : vNodes[e.n2].mNeighbors;
    const map<int, int>& mSmaller = vNodes[e.n1].mNeighbors.size() > vNodes[e.n2].mNeighbors.size() ? 
                                    vNodes[e.n2].mNeighbors : vNodes[e.n1].mNeighbors;
    int ret = 0;
    for (const auto& p : mSmaller)
      if (mLarger.count(p.first) > 0)
        ++ret;
    e.nCommon = ret;
    return ret;
  };

  /* typedef priority queue and its iterator */
  // typedef __gnu_pbds::priority_queue<Edge, Edge_Cmp, __gnu_pbds::thin_heap_tag> PQueue;
  typedef __gnu_pbds::priority_queue<Edge, Edge_Cmp, __gnu_pbds::pairing_heap_tag> PQueue;
  typedef PQueue::point_iterator PQueue_Iter;
  PQueue pq;

  /* calculate the number of common neighbors of each edge */
  vector<PQueue_Iter> vIterators(vEdges.size());
  for (int i = 0, n = vEdges.size(); i < n; ++i) {
    cal_common_neighbor_num(vEdges[i]);
    vIterators[i] = pq.push(vEdges[i]);
  }

  /* Tseng Siewiorek algorithm: iteratively merge the vertices with max common neighbors */
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
    const int new_n = node1.mNeighbors.size() > node2.mNeighbors.size() ? n1 : n2;
    const int del_n = node1.mNeighbors.size() > node2.mNeighbors.size() ? n2 : n1;
    Node& new_node = vNodes[new_n];
    Node& del_node = vNodes[del_n];
    vNodes[new_n].mNeighbors.erase(del_n);
    vNodes[del_n].mNeighbors.erase(new_n);
    ds.merge(new_n, del_n);
    assert(new_n != del_n);

    /* collect all new_node's neighbors and del_node's neighbors */
    set<int> sAllNeighbors;
    for (const auto& p : new_node.mNeighbors) {
      if (del_n == p.first) continue;
      sAllNeighbors.insert(p.first);
    }
    for (const auto& p : del_node.mNeighbors) {
      if (new_n == p.first) continue;
      sAllNeighbors.insert(p.first);
    }
    
    /* update common-neighbor edges */
    for (const int common_n : sCommonNeighbors) {
      Node& common_node = vNodes[common_n];
      assert(common_node.mNeighbors[del_n] == del_node.mNeighbors[common_n]);
      assert(common_node.mNeighbors.count(del_n) > 0);
      assert(common_node.mNeighbors.count(new_n) > 0);
      pq.erase(vIterators[common_node.mNeighbors[del_n]]);
      common_node.mNeighbors.erase(del_n);
      del_node.mNeighbors.erase(common_n);
    }

    /* remove non-common-neighbor edges*/
    for (const auto& p : new_node.mNeighbors) {
      if (sCommonNeighbors.count(p.first) == 0 && p.first != del_n) {
        assert(p.first != new_n);
        assert(vNodes[p.first].mNeighbors.count(del_n) == 0);
        vNodes[p.first].mNeighbors.erase(new_n);
        vNodes[new_n].mNeighbors.erase(p.first);
        pq.erase(vIterators[p.second]);
      }
    }
    for (const auto& p : del_node.mNeighbors) {
      if (sCommonNeighbors.count(p.first) == 0 && p.first != new_n) {
        assert(p.first != del_n);
        assert(vNodes[p.first].mNeighbors.count(new_n) == 0);
        vNodes[p.first].mNeighbors.erase(del_n);
        vNodes[del_n].mNeighbors.erase(p.first);
        pq.erase(vIterators[p.second]);
      }
    }
    
    /* (important!!) update all neighbors' edges */
    for (const int p : sAllNeighbors) {
      Node& neighbor = vNodes[p];
      for (auto q : neighbor.mNeighbors) {
        cal_common_neighbor_num(vEdges[q.second]);
        pq.modify(vIterators[q.second], vEdges[q.second]);
      }
    }

    assert(del_node.mNeighbors.empty());
  }

  /* print the results */
/*
  for (int i = 0; i < nvec; ++i) {
    fprintf(stderr, "[%d] %s : %d\n", i, vectors[i].c_str(), ds.find(i));
  }
  fprintf(stderr, "Total : %d\n", ds.nSets());
*/

  /* merge the vectors */
  for (int i = 0; i < nvec; ++i) {
    if (ds.find(i) == i) continue;
    const string& current_vec = vectors[i]; 
    string& root_vec = vectors[ds.find(i)];
    for (int j = 0; j < (int)current_vec.size(); ++j) {
      if (current_vec[j] != '2') {
        if (current_vec[j] == '0') {
          assert(root_vec[j] != '1');
          root_vec[j] = '0';
        }
        else if (current_vec[j] == '1') {
          assert(root_vec[j] != '0');
          root_vec[j] = '1';
        }
      }
    }
  }

  /* collect all merged vectors */
  set<string> new_vectors;
  for (int i = 0; i < nvec; ++i) {
    new_vectors.insert(vectors[ds.find(i)]);
  }

  /* print msg */
  fprintf(stderr, "# Compatibility graph:\n");
  fprintf(stderr, "#   drop %lu vector(s). (%lu)\n", vectors.size() - new_vectors.size(), new_vectors.size());

  /* update vectors */
  vectors.clear();
  for (const string& vec : new_vectors) {
    vectors.emplace_back(vec);
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

/* randomly fill all unknown values in vectors */
void ATPG::random_fill_x() {
  int i, len;
  len = cktin.size() + 1;
  for (string& vec : vectors) {
    assert(vec.size() == cktin.size() + 1);
    for (i = 0; i < len; ++i) {
      if (vec[i] == '2') {
        vec[i] = (rand() & 01) ? '1' : '0';
      }
    }
  }
}

/* duplicate vectors n times */
void ATPG::expand_vectors(const size_t n) {
  const size_t ori_size = vectors.size();
  vector<string> ori_vectors = vectors;
  vectors.clear();
  for (size_t i = 0; i < n; ++i) {
    for (size_t j = 0; j < ori_vectors.size(); ++j) {
      vectors.emplace_back(ori_vectors[j]);
    }
  }
  const size_t new_size = vectors.size();
  assert(new_size == ori_size * n);
  fprintf(stderr, "# Expand vectors:\n");
  fprintf(stderr, "#   %lu * %lu = %lu vector(s). (%lu)\n", ori_size, n, new_size, new_size);
}

/* use tdfsim to drop useless vectors */
void ATPG::random_simulation()
{
  int detect_num = 0;
  int iter;

  vector<int> order(vectors.size());
  vector<bool> dropped(vectors.size(), false);
  iota(order.begin(), order.end(), 0);

  fprintf(stderr, "# Random simulation:\n");
  int drop_count, init_total_count;
  init_total_count = vectors.size();
  for (iter = 0; iter < RANDOM_SIMULATION_ITER_NUM; ++iter) {
    drop_count = 0;
    if (iter == 0 || iter == RANDOM_SIMULATION_ITER_NUM) {
      iota(order.begin(), order.end(), 0);
    }
    else if (iter == 1) {
      reverse(order.begin(), order.end());
    }
    else {
      random_shuffle(order.begin(), order.end());
    }

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
    init_total_count -= drop_count;
    assert(init_total_count);
    if (drop_count > 0)
      fprintf(stderr, "#   Iter %d: drop %d vector(s). (%d)\n", iter, drop_count, init_total_count);
  }

  /* remove all useless vectors (dropped[i] == true => need to be dropped) */
  vector<string> compressed_vectors;
  for (size_t i = 0; i < vectors.size(); ++i) {
    if (!dropped[i]) {
      compressed_vectors.emplace_back(vectors[i]);
    }
  }
  vectors = compressed_vectors;
  assert((int)vectors.size() == init_total_count);
}
