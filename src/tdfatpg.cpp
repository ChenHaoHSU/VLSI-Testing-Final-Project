/**********************************************************************/
/*           Transition Delay Fault ATPG:                             */
/*           generating the test pattern list,                        */
/*           calculating the total fault coverage.                    */
/*                                                                    */
/*           Author: Hsiang-Ting Wen, Fu-Yu Chuang, and Chen-Hao Hsu  */
/*           Date : 12/05/2018 created                                */
/**********************************************************************/

#include "atpg.h"

#define RANDOM_PATTERN_NUM           10

void ATPG::transition_delay_fault_atpg(void) {
  srand(0); // what's your lucky number?

  // calculate_cc();
  // calculate_co();
  // tdf_podem_x();

  // random_pattern_generation(true);

  // vectors.clear();
  // vectors.emplace_back("0xx10x");
  // vectors.emplace_back("0xx1xx");
  // vectors.emplace_back("0x01xx");
  // vectors.emplace_back("01xx0x");
  // vectors.emplace_back("x0xx0x");
  // vectors.emplace_back("1xxxxx");
  // vectors.emplace_back("x1x00x");
  // vectors.emplace_back("11xx0x");
  // for (auto& s : vectors) {
  //   cerr << s << endl;
  // }

  fprintf(stderr, "# number of test patterns = %lu\n", vectors.size());
  static_compression();

  fprintf(stderr, "# number of test patterns = %lu\n", vectors.size());
  display_test_patterns();
  // display_undetect();
}

/* dynamic test compression by podem-x */
int ATPG::tdf_podem_x()
{
  string vec;
  int current_detect_num = 0;
  int total_detect_num = 0;
  int total_no_of_backtracks = 0; // accumulative number of backtracks
  int current_backtracks = 0;
  int no_of_aborted_faults = 0;
  int no_of_redundant_faults = 0;
  int no_of_calls = 0;

  fptr fault_under_test = select_primary_fault();

  /* generate test pattern */
  while (fault_under_test != nullptr) {
    switch (tdf_podem_v2(fault_under_test, current_backtracks)) {
      case TRUE:
        /* form a vector */
        vec.clear();
        assert(!vectors.empty());
        vec = vectors.back();
        /*by defect, we want only one pattern per fault */
        /*run a fault simulation, drop ALL detected faults */
        if (total_attempt_num == 1) {
          tdfsim_a_vector(vec, current_detect_num);
          total_detect_num += current_detect_num;
        }
        /* If we want mutiple petterns per fault, 
         * NO fault simulation.  drop ONLY the fault under test */ 
        else {
          // fault_under_test->detect = TRUE;
          /* drop fault_under_test */
          flist_undetect.remove(fault_under_test);
        }
        in_vector_no++;
        break;

      case FALSE:
          fault_under_test->detect = REDUNDANT;
          no_of_redundant_faults++;
          break;
    
      case MAYBE:
          no_of_aborted_faults++;
          break;
    }
    fault_under_test->test_tried = true;
    fault_under_test = nullptr;
    for (fptr fptr_ele: flist_undetect) {
      if (!fptr_ele->test_tried) {
        fault_under_test = fptr_ele;
        break;
      }
    }
    total_no_of_backtracks += current_backtracks; // accumulate number of backtracks
    no_of_calls++;
  }

  fprintf(stdout,"\n");
  fprintf(stdout,"#number of aborted faults = %d\n",no_of_aborted_faults);
  fprintf(stdout,"\n");
  fprintf(stdout,"#number of redundant faults = %d\n",no_of_redundant_faults);
  fprintf(stdout,"\n");
  fprintf(stdout,"#number of calling podem1 = %d\n",no_of_calls);
  fprintf(stdout,"\n");
  fprintf(stdout,"#total number of backtracks = %d\n",total_no_of_backtracks);
}

/* select a primary fault for podem */
ATPG::fptr ATPG::select_primary_fault()
{
  fptr fault_selected = flist_undetect.front();

  return fault_selected;
}

/* select a secondary fault for podem_x */
ATPG::fptr ATPG::select_secondary_fault()
{
  /* configurations */
  bool PODEM_X = true;
  enum DYNAMIC_TYPE { RANDOM, DETECTED_TIME, SCOAP };
  DYNAMIC_TYPE dtype = RANDOM;
  
  /* if wfmap is not constructed, construct it first */
  std::pair<fptr, fptr> empty_fpair;
  if (wfmap.empty()) {
    for (fptr f: flist_undetect) {
      if (f->io == GO) {
          if (!wfmap.count(std::pair<nptr, short>(f->node, -1)))
            wfmap[std::pair<nptr, short>(f->node, -1)] = empty_fpair;
          if (f->fault_type == STUCK0) wfmap[std::pair<nptr, short>(f->node, -1)].first = f;
          else wfmap[std::pair<nptr, short>(f->node, -1)].second = f;
      }
      else {
          if (!wfmap.count(std::pair<nptr, short>(f->node, f->index)))
            wfmap[std::pair<nptr, short>(f->node, f->index)] = empty_fpair;
          if (f->fault_type == STUCK0) wfmap[std::pair<nptr, short>(f->node, f->index)].first = f;
          else wfmap[std::pair<nptr, short>(f->node, f->index)].second = f;
      }
    }
  }
  
  /* check if there is any PI = U. if not, return nullptr */
  bool have_u = false;
  for (int i = 0; i < cktin.size(); ++i) {
    if (cktin[i]->value == U) {
      have_u = true;
      break;
    }
  }
  if (!have_u) return nullptr;
  
  /* [PODEM-X] check if there is any PO = U. if not, return nullptr */
  if (PODEM_X) {
    have_u = false;
    for (int i = 0; i < cktout.size(); ++i) {
      if (cktout[i]->value == U) {
        have_u = true;
        break;
      }
    }
    if (!have_u) return nullptr;
  }
  
  /* select possible faults */
  std::vector<fptr> secondary_fault_list;
  for (fptr f: flist_undetect) {
    wptr w = sort_wlist[f->to_swlist];
    if (f->fault_type == STUCK0) {
      if (w->value == U && (w->value_v1 == U || w->value_v1 == 0)) {
        secondary_fault_list.push_back(f);
      }
    }
    else {
      if (w->value == U && (w->value_v1 == U || w->value_v1 == 1)) {
        secondary_fault_list.push_back(f);
      }
    }
  }
  
  /* select a prefered fault */
  switch (dtype) {
    RANDOM:
      std::random_shuffle(secondary_fault_list.begin(), secondary_fault_list.end());
      break;
    DETECTED_TIME:
      std::sort(secondary_fault_list.begin(), secondary_fault_list.end(),
                [&](const fptr& a, const fptr& b) {
                    return (a->detected_time < b->detected_time);
                });
      break;
    SCOAP:
      std::sort(secondary_fault_list.begin(), secondary_fault_list.end(),
                [&](const fptr& a, const fptr& b) {
                    const wptr wa = sort_wlist[a->to_swlist], wb = sort_wlist[b->to_swlist];
                    double cca = (a->fault_type == STUCK0) ? wa->cc1 : wa->cc0;
                    double ccb = (b->fault_type == STUCK0) ? wb->cc1 : wb->cc0;
                    double coa = (a->io == GO) ? wa->co.back() : wa->co[a->index];
                    double cob = (b->io == GO) ? wb->co.back() : wb->co[b->index];
                    return ((cca * coa) > (ccb * cob));
                });
      break;
    default:
      break;
  }

  if (secondary_fault_list.empty()) return nullptr;
  fptr fault_selected = secondary_fault_list.front();
  return fault_selected;
}
  
/* generate test vector 2, considering fault/LOS constraints */
int ATPG::tdf_podem_v2(const fptr fault, int& current_backtracks) 
{
  int i,ncktwire,ncktin;
  wptr wpi; // points to the PI currently being assigned
  forward_list<wptr> decision_tree; // design_tree (a LIFO stack)
  wptr wfault;
  int attempt_num = 0;  // counts the number of pattern generated so far for the given fault

  /* initialize all circuit wires to unknown */
  ncktwire = sort_wlist.size();
  ncktin = cktin.size();
  for (i = 0; i < ncktwire; i++) {
    sort_wlist[i]->value = U;
    sort_wlist[i]->value_v1 = U;
  }
  no_of_backtracks = 0;
  find_test = false;
  no_test = false;
  
  mark_propagate_tree(fault->node);

  /* Fig 7 starts here */
  /* set the initial goal, assign the first PI.  Fig 7.P1 */
  switch (set_uniquely_implied_value(fault)) {
    case TRUE: // if a  PI is assigned 
      sim();  // Fig 7.3
      wfault = fault_evaluate(fault);
      if (wfault != nullptr) forward_imply(wfault);// propagate fault effect
      if (check_test()) find_test = true; // if fault effect reaches PO, done. Fig 7.10
      break;
    case CONFLICT:
      no_test = true; // cannot achieve initial objective, no test
      break;
    case FALSE:
      break;  //if no PI is reached, keep on backtracing. Fig 7.A 
  }

  /* loop in Fig 7.ABC 
   * quit the loop when either one of the three conditions is met: 
   * 1. number of backtracks is equal to or larger than limit
   * 2. no_test
   * 3. already find a test pattern AND no_of_patterns meets required total_attempt_num */
  while ((no_of_backtracks < backtrack_limit) && !no_test &&
    !(find_test && (attempt_num == total_attempt_num))) {
    
    /* check if test possible.   Fig. 7.1 */
    if (wpi = test_possible(fault)) {
      wpi->flag |= CHANGED;
      /* insert a new PI into decision_tree */
      decision_tree.push_front(wpi);
    }
    else { // no test possible using this assignment, backtrack.
      while (!decision_tree.empty() && (wpi == nullptr)) {
        /* if both 01 already tried, backtrack. Fig.7.7 */
        if (decision_tree.front()->flag & ALL_ASSIGNED) {
          decision_tree.front()->flag &= ~ALL_ASSIGNED;  // clear the ALL_ASSIGNED flag
          decision_tree.front()->value = U; // do not assign 0 or 1
          decision_tree.front()->flag |= CHANGED; // this PI has been changed
          /* remove this PI in decision tree.  see dashed nodes in Fig 6 */
          decision_tree.pop_front();
        }  
        /* else, flip last decision, flag ALL_ASSIGNED. Fig. 7.8 */
        else {
          decision_tree.front()->value = decision_tree.front()->value ^ 1; // flip last decision
          decision_tree.front()->flag |= CHANGED; // this PI has been changed
          decision_tree.front()->flag |= ALL_ASSIGNED;
          no_of_backtracks++;
          wpi = decision_tree.front();
        }
      } // while decision tree && ! wpi
      if (wpi == nullptr) no_test = true; //decision tree empty,  Fig 7.9
    } // no test possible

/* this again loop is to generate multiple patterns for a single fault 
 * this part is NOT in the original PODEM paper  */
again:  if (wpi) {
      sim();
      if (wfault = fault_evaluate(fault)) forward_imply(wfault);
      if (check_test() && (tdf_podem_v1(fault) == TRUE)) {
        /* set find_test true */
        find_test = true;
        /* fill value and value_v1 with 0 or 1. (no D, B, U) */
        for (i = 0; i < ncktin; i++) {
          switch (cktin[i]->value) {
            case 0:
            case B:
              cktin[i]->value = 0; break;
            case 1:
            case D: 
              cktin[i]->value = 1; break;
            case U: 
              cktin[i]->value = rand()&01; break;
          }
          switch (cktin[i]->value_v1) {
            case 0:
            case B:
              cktin[i]->value_v1 = 0; break;
            case 1:
            case D: 
              cktin[i]->value_v1 = 1; break;
            case U: 
              cktin[i]->value_v1 = rand()&01; break;
          }
        }
        /* form the generated pattern and push back to vectors */
        string vec(ncktin + 1, '0');
        for (i = 0; i < ncktin; i++) {
          vec[i] = itoc(cktin[i]->value_v1);
        }
        vec.back() = itoc(cktin[0]->value);
        vectors.emplace_back(vec);
        /* if multiple patterns per fault, print out every test cube */
        if (total_attempt_num > 1) {
          if (attempt_num == 0) {
            display_fault(fault);
          }
          display_io();
        }
        attempt_num++; // increase pattern count for this fault

		    /* keep trying more PI assignments if we want multiple patterns per fault
		     * this is not in the original PODEM paper*/
        if (total_attempt_num > attempt_num) {
          wpi = nullptr;
          while (!decision_tree.empty() && (wpi == nullptr)) {
            /* backtrack */
            if (decision_tree.front()->flag & ALL_ASSIGNED) {
              decision_tree.front()->flag &= ~ALL_ASSIGNED;
              decision_tree.front()->value = U;
              decision_tree.front()->flag |= CHANGED;
              decision_tree.pop_front();
            }
            /* flip last decision */
            else {
              decision_tree.front()->value = decision_tree.front()->value ^ 1;
              decision_tree.front()->flag |= CHANGED;
              decision_tree.front()->flag |= ALL_ASSIGNED;
              no_of_backtracks++;
              wpi = decision_tree.front();
            }
          }
          if (!wpi) no_test = true;
          goto again;  // if we want multiple patterns per fault
        } // if total_attempt_num > attempt_num
      }  // if check_test()
    } // again
  } // while (three conditions)

  /* clear everthing */
  for (wptr wptr_ele: decision_tree) {
    wptr_ele->flag &= ~ALL_ASSIGNED;
  }
  decision_tree.clear();
  
  unmark_propagate_tree(fault->node);
  
  if (find_test) {
    /* normally, we want one pattern per fault */
    if (total_attempt_num == 1) {
      // for (i = 0; i < ncktin; i++) {
      //   switch (cktin[i]->value) {
      //     case 0:
      //     case 1: break;
      //     case D: cktin[i]->value = 1; break;
      //     case B: cktin[i]->value = 0; break;
      //     // case U: cktin[i]->value = rand()&01; break; // random fill U
      //   }
      // }
      // display_io();
    }
    else fprintf(stdout, "\n");  // do not random fill when multiple patterns per fault
  }
  else if (no_test) {
    // fprintf(stdout,"redundant fault...\n");
    return(FALSE);
  }
  else {
    /*fprintf(stdout,"test aborted due to backtrack limit...\n");*/
    return(MAYBE);
  }
  return FALSE;
}     

/* generate test vector 1, injection/activation/propagation */
int ATPG::tdf_podem_v1(const fptr fault) 
{
   int i, ncktin, desired_value;
  wptr faulty_wire;

  ncktin = cktin.size();
  desired_value = fault->fault_type;
  faulty_wire = sort_wlist[fault->to_swlist];

  /* shift v2, assign to value_v1 */
  for (i = 1; i < ncktin; ++i) {
    switch (cktin[i]->value) {
      case 0:
      case B:
        cktin[i-1]->value_v1 = 0;
        break;
      case 1:
      case D:
        cktin[i-1]->value_v1 = 1;
        break;
      case U:
        cktin[i-1]->value_v1 = U;
        break;
    }
  }

  for (i = 0; i < ncktin; ++i) {
    cktin[i]->flag &= ~MARKED2;
    cktin[i]->flag &= ~CHANGED2;
    cktin[i]->flag &= ~ALL_ASSIGNED2;
  }

  vector<wptr> fanin_cone_wlist; // follow topological order
  vector<wptr> pi_wlist; // only non-fixed PIs

  /* mark all wires in the fanin cone */
  mark_propagate_tree(faulty_wire, fanin_cone_wlist, pi_wlist);

  /* unmark all wires in the fanin cone */
  for (wptr w : fanin_cone_wlist) {
    w->flag &= ~MARKED2;
  }

  auto partial_sim = [&] (vector<wptr> &wlist) -> void {
    for (int i = 0, nwire = wlist.size(); i < nwire; ++i) {
      if (wlist[i]->flag & INPUT) continue;
      evaluate_v1(wlist[i]->inode.front());
    }
    return;
  };

  /* sim fixed value_v1 */
  partial_sim(fanin_cone_wlist);
  if (faulty_wire->value_v1 != U) {
    if (faulty_wire->value_v1 == desired_value) {
      // fprintf(stderr, "TRUE\n");
      return TRUE;
    }
    else {
      // fprintf(stderr, "FALSE\n");
      return FALSE;
    }
  }

  // display_sort_wlist();

  // no decision can be made; return FALSE
  if (pi_wlist.empty()) return FALSE;

  int ret = FALSE;
  int no_of_backtracks_v1 = 0;
  forward_list<wptr> decision_tree;
  wptr current_wire, new_wire; // must be pi

  new_wire = find_pi_assignment_v1(faulty_wire, desired_value);

  if (new_wire == nullptr)
    return FALSE;
  else
    decision_tree.push_front(new_wire);

  while (!decision_tree.empty() && no_of_backtracks_v1 <= backtrack_limit_v1) {

    current_wire = decision_tree.front();

    if (current_wire->flag & ALL_ASSIGNED2) {
      current_wire->flag &= ~ALL_ASSIGNED2;
      current_wire->flag &= ~MARKED2;
      current_wire->value_v1 = U;
      decision_tree.pop_front();
      continue;
    }
    else if (current_wire->flag & MARKED2) { // current_wire not all assigned
      current_wire->value_v1 = current_wire->value_v1 ^ 1;
      current_wire->flag |= ALL_ASSIGNED2;
      current_wire->flag &= ~MARKED2;
      ++no_of_backtracks_v1;
    }
    else {
      current_wire->flag |= MARKED2;
    }

    partial_sim(fanin_cone_wlist);
    if (faulty_wire->value_v1 == desired_value) {
      ret = TRUE;
      break;
    }
    else if (faulty_wire->value_v1 == U) {
      new_wire = find_pi_assignment_v1(faulty_wire, desired_value);
      if (new_wire != nullptr) {
        decision_tree.push_front(new_wire);
      }
    }
  }

/*
  int ret = FALSE;
  int no_of_backtracks_v1 = 0;
  int decision_level = 0; // 0 <= decision_level <= pi_wlist.size() - 1
  wptr current_wire; // must be pi

  while (decision_level >= 0 && no_of_backtracks_v1 <= backtrack_limit_v1) {
    // reach the last decision level 
    if (decision_level >= pi_wlist.size()) {
      --decision_level;
      continue;
    }

    current_wire = pi_wlist[decision_level];

    if (current_wire->value_v1 == U) {
      current_wire->value_v1 = 0;
      current_wire->flag &= ~ALL_ASSIGNED2;
    }
    else if (current_wire->flag & ALL_ASSIGNED2) {
      current_wire->flag &= ~ALL_ASSIGNED2;
      current_wire->value_v1 = U;
      --decision_level;
      continue;
    }
    else {
      current_wire->value_v1 = current_wire->value_v1 ^ 1;
      current_wire->flag |= ALL_ASSIGNED2;
      ++no_of_backtracks_v1;
    }

    partial_sim(fanin_cone_wlist); // forward imply?
    if (faulty_wire->value_v1 == desired_value) {
      ret = TRUE;
      break;
    }
    else if (faulty_wire->value_v1 == U) {
      ++decision_level;
    }
  }
*/

/*
  cerr << pi_wlist.size() << " / " << cktin.size() 
       << " (" << ((double)pi_wlist.size()/(double)cktin.size() * 100) << "%) " << endl;

  fprintf(stderr, "%s\n", (ret == TRUE ? "TRUE" : "FALSE"));
  fprintf(stderr, "faulty wire: %s %d\n", faulty_wire->name.c_str(), desired_value);
  fprintf(stderr, "Fanins:\n");
  for (const wptr w : fanin_cone_wlist) {
    fprintf(stderr, " %s", w->name.c_str());
  }
  fprintf(stderr, "\n");
  fprintf(stderr, "PIs:\n");
  for (const wptr w : pi_wlist) {
    assert(w->flag & INPUT);
    fprintf(stderr, " %s", w->name.c_str());
  }
  fprintf(stderr, "\n");
  getchar();
*/

  for (i = 0; i < ncktin; ++i) {
    cktin[i]->flag &= ~MARKED2;
    cktin[i]->flag &= ~CHANGED2;
    cktin[i]->flag &= ~ALL_ASSIGNED2;
  }

  return ret;
}

void ATPG::mark_propagate_tree(const wptr w, vector<wptr> &fanin_cone_wlist, vector<wptr> &pi_wlist) {
  if (w->flag & MARKED2) return;

  int i, j, ninode, niwire;
  w->flag |= MARKED2;
  for (i = 0, ninode = w->inode.size(); i < ninode; ++i) {
    for (j = 0, niwire = w->inode[i]->iwire.size(); j < niwire; ++j) {
      mark_propagate_tree(w->inode[i]->iwire[j], fanin_cone_wlist, pi_wlist);
    }
  }
  fanin_cone_wlist.emplace_back(w);
  if ((w->flag & INPUT) && (w->value_v1 == U)) pi_wlist.emplace_back(w);
}

void ATPG::random_pattern_generation(const bool use_unknown) {
  unordered_set<string> sVector;
  string vec1(cktin.size() + 1, '0');
  string vec2(cktin.size() + 1, '0');
  for (int i = 0; i < RANDOM_PATTERN_NUM; ++i) {
    for (int j = 0; j < cktin.size() + 1; ++j) {
      if (use_unknown && (rand() % 2)) {
        vec1[j] = 'x';
        vec2[j] = 'x';
      }
      else {
        if (rand() % 2) {
          vec1[j] = '0';
          vec2[j] = '1';
        }
        else {
          vec1[j] = '1';
          vec2[j] = '0';
        }
      }
    }
    if (sVector.find(vec1) == sVector.end()) {
      vectors.emplace_back(vec1);
      // vectors.emplace_back(vec2);
      sVector.emplace(vec1);
      // sVector.emplace(vec2);
    }
  }
}

void ATPG::forward_imply_v1(const wptr w) {
  int i,nout;

  for (i = 0, nout = w->onode.size(); i < nout; i++) {
    if (w->onode[i]->type != OUTPUT) {
      evaluate_v1(w->onode[i]);
      if (w->onode[i]->owire.front()->flag & CHANGED2)
	      forward_imply(w->onode[i]->owire.front()); // go one level further
      w->onode[i]->owire.front()->flag &= ~CHANGED2;
    }
  }
}/* end of forward_imply */

ATPG::wptr ATPG::find_pi_assignment_v1(const wptr object_wire, const int& object_level) {
  wptr new_object_wire;
  int new_object_level;
  
  /* if PI, assign the same value as objective Fig 9.1, 9.2 */
  if (object_wire->flag & INPUT) {
    assert(object_wire->value_v1 == U);
    object_wire->value_v1 = object_level;
    return(object_wire);
  }

  /* if not PI, backtrace to PI  Fig 9.3, 9.4, 9.5*/
  else {
    switch(object_wire->inode.front()->type) {
      case   OR:
      case NAND:
        if (object_level) new_object_wire = find_easiest_control_v1(object_wire->inode.front());  // decision gate
        else new_object_wire = find_hardest_control_v1(object_wire->inode.front()); // imply gate
        break;
      case  NOR:
      case  AND:
		// TODO  similar to OR and NAND but different polarity
        if (object_level) new_object_wire = find_hardest_control_v1(object_wire->inode.front());
        else new_object_wire = find_easiest_control_v1(object_wire->inode.front());
        break;
		//  TODO END
      case  NOT:
      case  BUF:
        new_object_wire = object_wire->inode.front()->iwire.front();
        break;
    }

    switch (object_wire->inode.front()->type) {
      case  BUF:
      case  AND:
      case   OR: new_object_level = object_level; break;
      /* flip objective value  Fig 9.6 */
      case  NOT:
      case  NOR:
      case NAND: new_object_level = object_level ^ 1; break;
    }
    if (new_object_wire) return(find_pi_assignment_v1(new_object_wire,new_object_level));
    else return(nullptr);
  }
}/* end of find_pi_assignment */


/* Fig 9.4 */
ATPG::wptr ATPG::find_hardest_control_v1(const nptr n) {
  int i;
  
  /* because gate inputs are arranged in a increasing level order,
   * larger input index means harder to control */
  for (i = n->iwire.size() - 1; i >= 0; i--) {
    if (n->iwire[i]->value_v1 == U) return(n->iwire[i]);
  }
  return(nullptr);
}/* end of find_hardest_control */


/* Fig 9.5 */
ATPG::wptr ATPG::find_easiest_control_v1(const nptr n) {
  int i, nin;
  // TODO similar to hardiest_control but increasing level order
  for (i = 0, nin = n->iwire.size(); i < nin; i++) {
    if (n->iwire[i]->value_v1 == U) return(n->iwire[i]);
  }
  // TODO END
  return(nullptr);
}/* end of find_easiest_control */
