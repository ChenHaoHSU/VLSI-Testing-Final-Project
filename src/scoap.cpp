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