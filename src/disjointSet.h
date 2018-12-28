/**********************************************************************/
/*           Disjoint Set:                                            */
/*           weighted union and path compression,                     */
/*                                                                    */
/*           Author: Hsiang-Ting Wen, Fu-Yu Chuang, and Chen-Hao Hsu  */
/*           Date : 12/05/2018 created                                */
/**********************************************************************/

#ifndef DISJOINTSET_H
#define DISJOINTSET_H

using namespace std;

class DisjointSet {
public:
  DisjointSet()
    : parent(0), rnk(0), n(0) {}
  DisjointSet(const int n) { init(n); }
  ~DisjointSet() {
    delete [] parent;
    delete [] rnk;
  }

  // Initialize n disjoint sets
  void init(const int n) {
    this->n = n;
    parent = new int[n];
    rnk = new int[n];
    for (int i = 0; i < n; ++i) {
      rnk[i] = 0;
      parent[i] = i;
    }
  }

  // Find set
  int find(const int u) const {
    return (u == parent[u] ? u : parent[u] = find(parent[u]));
  }

  // Union by rank
  void merge(int x, int y) {
    x = find(x), y = find(y);
    if (x == y)
      return;
    if (rnk[x] > rnk[y])
      parent[y] = x;
    else // If rnk[x] <= rnk[y]
      parent[x] = y;
    if (rnk[x] == rnk[y])
      rnk[y]++;
  }

  // Number of disjoint sets
  int nSets() const {
    int nSets = 0;
    for (int i = 0; i < n; ++i)
      if (parent[i] == i) ++nSets;
    return nSets;
  }

private:
  int *parent, *rnk;
  int n;
};

#endif // DISJOINTSET_H