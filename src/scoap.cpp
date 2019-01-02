/**********************************************************************/
/*           Sandia controllability observability analysis program;   */
/*           SCOAP for fault selection / backtrace decision;          */
/*                                                                    */
/*           Author: Hsiang-Ting Wen, Fu-Yu Chuang, and Chen-Hao Hsu  */
/*           Date : 12/15/2018 created                                */
/**********************************************************************/

#include "atpg.h"

void ATPG::calculate_cc(void)
{
  int nckt = sort_wlist.size();
  int ncktin = cktin.size();
  
  /* for all PIs */
  for (int i = 0; i < ncktin; ++i) {
    sort_wlist[i]->cc0 = 1;
    sort_wlist[i]->cc1 = 1;
  }

  /* for all other nodes */
  for (int i = ncktin; i < nckt; ++i) {
    wptr wire = sort_wlist[i];
    nptr node = wire->inode.front();
    int node_type = node->type;
    int sum_cc = 0, min_cc = INT_MAX;
    int dif_cc = 0, eqv_cc = 0; // for XOR and EQV

    switch (node_type) {
      case NOT:
        assert(node->iwire.size() == 1);
        wire->cc0 = node->iwire.front()->cc1 + 1;
        wire->cc1 = node->iwire.front()->cc0 + 1;
        break;

      case BUF:
        assert(node->iwire.size() == 1);
        wire->cc0 = node->iwire.front()->cc0 + 1;
        wire->cc1 = node->iwire.front()->cc1 + 1;
        break;

      case AND:
      case NAND:
        for (wptr w : node->iwire) {
          sum_cc += w->cc1;
          min_cc = (w->cc0 < min_cc)? w->cc0: min_cc;
        }
        wire->cc0 = (node_type == AND)? (min_cc + 1): (sum_cc + 1);
        wire->cc1 = (node_type == AND)? (sum_cc + 1): (min_cc + 1);
        break;      
      
      case OR:
      case NOR:
        for (wptr w : node->iwire) {
          sum_cc += w->cc0;
          min_cc = (w->cc1 < min_cc)? w->cc1: min_cc;
        }
        wire->cc0 = (node_type == OR)? (sum_cc + 1): (min_cc + 1);
        wire->cc1 = (node_type == OR)? (min_cc + 1): (sum_cc + 1);
        break;
      
      case XOR:
      case EQV:
        assert(node->iwire.size() == 2);  // handles only XOR/EQV with 2 inputs
        eqv_cc = min(node->iwire.front()->cc0 + node->iwire.back()->cc0, 
                     node->iwire.front()->cc1 + node->iwire.back()->cc1) + 1;
        dif_cc = min(node->iwire.front()->cc0 + node->iwire.back()->cc1, 
                     node->iwire.front()->cc1 + node->iwire.back()->cc0) + 1;
        wire->cc0 = (node_type == XOR)? eqv_cc: dif_cc;
        wire->cc1 = (node_type == XOR)? dif_cc: eqv_cc;
        break;


      default:
        assert(0);  // should not happen
        break;    
    }
  }

  return;
}

void ATPG::calculate_co(void)
{
  int nckt = sort_wlist.size();

  for (int i = nckt - 1; i >= 0; --i) {
    wptr wire = sort_wlist[i];

    for (nptr n : wire->onode) {
      int sum_cc = 0;
      int node_type = n->type;

      switch (node_type) {
        case NOT:
        case BUF:
        case OUTPUT:
          break;

        case AND:
        case NAND:
          for (wptr w : n->iwire) {
            if (w != wire) {
              sum_cc += w->cc1;
            }
          }
          break;

        case OR:
        case NOR:
          for (wptr w : n->iwire) {
            if (w != wire) {
              sum_cc += w->cc0;
            }
          }
          break;

        case XOR:
        case EQV:
          assert(n->iwire.size() == 2);   // handles only XOR/EQV with 2 inputs
          for (wptr w : n->iwire) {
            if (w != wire) {
              sum_cc = min(w->cc0, w->cc1);
            }
          }
          break;

        default:
          assert(0);  // should not happen
          break;
      }
     
      if (node_type == OUTPUT) {
        wire->co.push_back(0);
      } else {
        assert(!(n->owire.front()->co.empty()));
        int co = n->owire.front()->co.back();   // notice that co.back() can be CO of a fanout stem
        wire->co.push_back(co + sum_cc + 1);
      }  
    }

    // deal with fanout stem
    if (wire->onode.size() > 1) {
      assert(wire->onode.size() == wire->co.size());
      int min_co = INT_MAX;
      for (int co : wire->co) {
        min_co = (co < min_co)? co: min_co;
      }
      wire->co.push_back(min_co);
    }
  }

  return;
}

void ATPG::calculate_scoap(void) 
{
  calculate_cc();
  calculate_co();

  for (fptr f : flist_ranked) {
    const wptr w = sort_wlist[f->to_swlist];
    int cc = (f->fault_type == STUCK0) ? w->cc1 : w->cc0;
    int co = (f->io == GO) ? w->co.back() : w->co[f->index];
    f->scoap = cc * co;
  }

  return;
}

void ATPG::rank_fault_by_scoap(void)
{
  calculate_scoap();

  sort(flist_ranked.begin(), flist_ranked.end(),
      [&](const fptr& a, const fptr& b) {
          return (a->scoap > b->scoap);
      });

  return;
}

void ATPG::rank_fault_by_detect(void)
{
  calculate_scoap();
  try_pattern_gen();

  sort(flist_ranked.begin(), flist_ranked.end(),
      [&](const fptr& a, const fptr& b) {
          return (a->score > b->score);
      });

  return;
}

void ATPG::try_pattern_gen(void)
{
  int current_detect_num = 0;
  int current_backtrack_num = 0;
  
  /* tune parameters !!! */
  int v2_loop_limit = 1000;
  
  /* declare parameters */
  LIFO   d_tree;           // decision tree. must be clear in each outmost loop.
  string d_tree_flag;      // string of PI "ALL_ASSIGNED" flag. for recording decision tree state.
  string tdf_vec;          // tdf vector. must be clear in each outmost loop.
  fptr   primary_fault;    // primary fault
  
  bool   test_found;       // test found (for both v2, v1)
  bool   next_is_v2;       // test v2 in next loop
  bool   v1_not_found;     // failed to test v1 under a previous v2
  bool   is_primary;       // indicate if the fault is primary
  
  int    v2_loop_counter;
  
  /* initialize fault primary members */
  initialize_fault_primary_record();
  
  /* select primary fault */
  primary_fault = select_primary_fault_by_order();
  
  while (primary_fault != nullptr) {
    /* test this primary fault */
    /* initialize the controlling boolean parameters */
    test_found = false;
    next_is_v2 = true;
    v1_not_found = false;
    is_primary = true;
    v2_loop_counter = 0;
    
    /* initialize tdf_vec and d_tree_flag for primary fault */
    tdf_vec.resize(cktin.size()+1, '2');
    d_tree_flag.resize(cktin.size(), '0');
    
    /* main loop for finding primary fault v2-v1 pattern */
    while (!test_found && (v2_loop_counter < v2_loop_limit)) {
      /* find v2 */
      if (next_is_v2) {
        int find_v2 = tdf_medop_v2(primary_fault, current_backtrack_num, d_tree, tdf_vec, d_tree_flag, is_primary, v1_not_found);
        if (find_v2 == TRUE) {
            next_is_v2 = false;
            v2_loop_counter++;
        } else { break; }
      }
      /* find v1 */
      else {
        LIFO tmp_d_tree;          // new empty tree. the old tree must not be cleared since we may not find v1
        string tmp_vec = tdf_vec; // success v2 vector
        int find_v1 = tdf_medop_v1(primary_fault, current_backtrack_num, tmp_d_tree, tmp_vec);
        if (find_v1 == TRUE) {
          tdf_vec = tmp_vec;
          vectors.emplace_back(tdf_vec);
          test_found = true;
        } else {
          next_is_v2 = true;
          v1_not_found = true;
        }
      }
    }

    /* evaluate how good the pattern is */
    if (test_found) {
      tdfsim_a_vector(vectors.back(), current_detect_num, false);
      // primary_fault->score = (double)detection_score.back() / detection_count.back();  
      size_t x_bit_count = 0;
      for (size_t i = 0; i < vectors.back().size(); ++i) {
        if (vectors.back()[i] == '2') {
            ++x_bit_count;
        }
      }
      primary_fault->score = detection_score.back(); // detection_count or detection_score
      // primary_fault->score = x_bit_count;
      v2_loop_max_trial = (v2_loop_counter > v2_loop_max_trial) ? v2_loop_counter : v2_loop_max_trial;
    } else {
      primary_fault->score = INT_MIN;
    }
    
    /* select next primary fault */
    primary_fault->test_tried = true;
    primary_fault = select_primary_fault_by_order();
  } // while-loop for primary fault

  // this is just a trial, should not actually generate test patterns
  // clear it!
  vectors.clear();
  for (fptr f : flist_undetect) {
    f->test_tried = false;
  }

  return;
}

void ATPG::display_scoap(void) 
{
  for (wptr w : sort_wlist) {
    cout << left << setw(10) << w->name << ": cc0 = " << w->cc0 << ", cc1 = " << setw(5) << w->cc1 << ", co = ";
    for (int a : w->co) {
      cout << a << " ";
    }
    cout << endl;
  }

  return;
}