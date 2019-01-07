/**********************************************************************/
/*           Static Test Compression:                                 */
/*           statically compress the pattern list;                    */
/*                                                                    */
/*           Author: Hsiang-Ting Wen, Fu-Yu Chuang, and Chen-Hao Hsu  */
/*           Date : 12/27/2018 created                                */
/**********************************************************************/

#include "atpg.h"
#include "disjointSet.h"

void ATPG::static_compression() {
  if (!compression) {
    random_fill_x();
    return;
  }

  fprintf(stderr, "# Static compression: (%lu)\n", vectors.size());

  random_simulation();
  int max_supernode = compatibility_graph();
  expand_vectors(max_supernode);
  random_simulation();
  random_fill_x();
  random_simulation();
}

/* Tseng-Siewiorek algorithm: solve minimum clique partition problem on compatibility graphs 
   return the number of nodes in the maximum super-node */
int ATPG::compatibility_graph() {
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
    for (int j = i + 1; j < min(nvec, i + 500); ++j) {
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
    int rm_e = 0;
    for (const int p : sAllNeighbors) {
      if (sCommonNeighbors.count(p) > 0) continue;
      if (new_node.mNeighbors.count(p) > 0) {
        assert(p != new_n && p != del_n);
        assert(vNodes[p].mNeighbors.count(del_n) == 0);
        rm_e = vNodes[p].mNeighbors[new_n];
        pq.erase(vIterators[rm_e]);
        vNodes[p].mNeighbors.erase(new_n);
        vNodes[new_n].mNeighbors.erase(p);
      }
      if (del_node.mNeighbors.count(p) > 0) {
        assert(p != new_n && p != del_n);
        assert(vNodes[p].mNeighbors.count(new_n) == 0);
        rm_e = vNodes[p].mNeighbors[del_n];
        pq.erase(vIterators[rm_e]);
        vNodes[p].mNeighbors.erase(del_n);
        vNodes[del_n].mNeighbors.erase(p);
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

  /* collect all merged vectors */
  set<string> new_vectors;
  vector<int> supernode_num_list(vectors.size(), 0);
  for (int i = 0; i < nvec; ++i) {
    supernode_num_list[ds.find(i)] += 1;
    new_vectors.insert(vectors[ds.find(i)]);
  }
  int max_supernode = *max_element(supernode_num_list.begin(), supernode_num_list.end());

  /* print msg */
  fprintf(stderr, "#   Compatibility graph:\n");
  fprintf(stderr, "#     max supernode = %d; drop %lu vector(s). (%lu)\n",
                  max_supernode, vectors.size() - new_vectors.size(), new_vectors.size());

  /* update vectors */
  vectors.clear();
  for (const string& vec : new_vectors) {
    vectors.emplace_back(vec);
  }

  return max_supernode;
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
  int i, j, nvec, len;
  nvec = vectors.size();
  len = cktin.size() + 1;
  double x_count = 0;
  double total_count = 0;
  for (i = 0; i < nvec; ++i) {
    string& vec = vectors[i];
    assert(vec.size() == cktin.size() + 1);
    for (j = 0; j < len; ++j) {
      total_count += 1;
      if (vec[j] == '2') {
        x_count += 1;
        vec[j] = (rand() & 01) ? '1' : '0';
      }
    }
  }

  /* print msg */
  fprintf(stderr, "#   Randomly fill x:\n");
  fprintf(stderr, "#     x-ratio = %2.0f / %2.0f = %2.2f%% (%lu)\n",
                   x_count, total_count, 100 * (x_count / total_count), vectors.size());
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

  /* print msg */
  fprintf(stderr, "#   Expand vectors:\n");
  fprintf(stderr, "#     %lu * %lu = %lu vector(s). (%lu)\n", ori_size, n, new_size, new_size);
}

/* use tdfsim to drop useless vectors */
void ATPG::random_simulation()
{
  int detect_num = 0;
  int drop_num = 0;
  int iter;
  int nprint = 0;

  vector<int> order(vectors.size());
  vector<bool> dropped(vectors.size(), false);
  iota(order.begin(), order.end(), 0);

  fprintf(stderr, "#   Random simulation:\n");
  fprintf(stderr, "#    ");
  int drop_count;
  for (iter = 0; iter < random_sim_num; ++iter) {
    drop_count = 0;
    if (iter == 0) {
      if (stc_use_sorted) {
        sorted_vector_order(order);
      } else {
        iota(order.begin(), order.end(), 0);
        reverse(order.begin(), order.end());
      }
    } else if (iter == 1) {
      if (stc_use_sorted) {
        random_shuffle(order.begin(), order.end());
      } else {
        reverse(order.begin(), order.end());
      }
    } else if (iter == random_sim_num) {
      iota(order.begin(), order.end(), 0);
    } else {
      random_shuffle(order.begin(), order.end());
    }

    generate_fault_list();
    for (const int s : order) {
      if (!dropped[s]) {
        detect_num = 0;
        tdfsim_a_vector(vectors[s], detect_num, drop_num);
        if (detect_num == 0) {
          dropped[s] = true;
          ++drop_count;
        }
      }
    }
    if (drop_count > 0) {
      fprintf(stderr, " i%d: %d;", iter, drop_count);
      if (++nprint >= 5) {
        nprint = 0;
        fprintf(stderr, "\n#    ");
      }
    }
  }

  /* remove all useless vectors (dropped[i] == true => need to be dropped) */
  vector<string> compressed_vectors;
  for (size_t i = 0; i < vectors.size(); ++i) {
    if (!dropped[i]) {
      compressed_vectors.emplace_back(vectors[i]);
    }
  }
  vectors = compressed_vectors;
  fprintf(stderr, " (%lu)\n", vectors.size());
}

/* sort vectors by the number of detected faults */
void ATPG::sorted_vector_order(vector<int>& order) {

  /* generate a new fault list */
  generate_fault_list();

  /* calculate the number of detected faults of every vector */
  vector<tuple<int, int, int> > detected_num_list; // (idx, detected_num, x_count)
  detected_num_list.reserve(vectors.size());
  int detect_num, drop_num;
  int x_cnt;
  for (int i = 0, n = vectors.size(); i < n; ++i) {
    const string& vec = vectors[i];
    detect_num = 0;
    drop_num = 0;
    tdfsim_a_vector(vec, detect_num, drop_num, false);
    x_cnt = x_count(vec);
    detected_num_list.emplace_back(i, detect_num, x_cnt);
  }

  assert(vectors.size() == detected_num_list.size());

  /* sort vectors by the number of detected faults */
  sort(detected_num_list.begin(), detected_num_list.end(),
    [] (const tuple<int, int, int> t1, const tuple<int, int, int> t2) {
      if (get<1>(t1) != get<1>(t2)) return get<1>(t1) > get<1>(t2);
      else if (get<2>(t1) != get<2>(t2)) return get<2>(t1) > get<2>(t2);
      else return get<0>(t1) > get<0>(t2);
    });

  /* restore to order */
  order.clear();
  for (const tuple<int, int, int>& t : detected_num_list) {
    order.emplace_back(get<0>(t));
  }
}
