/**********************************************************************/
/*           Transition Delay Fault ATPG;                             */
/*           building the pattern list;                               */
/*           calculating the total fault coverage.                    */
/*                                                                    */
/*           Author: Hsiang-Ting Wen, Fu-Yu Chuang, and Chen-Hao Hsu  */
/*           Date : 12/05/2018 created                                */
/**********************************************************************/

#include "atpg.h"

void ATPG::transition_delay_fault_atpg(void) {
  srand(0); // what's your lucky number?

  string vec;
  int current_detect_num = 0;
  int total_detect_num = 0;
  int total_no_of_backtracks = 0; // accumulative number of backtracks
  int current_backtracks = 0;
  int no_of_aborted_faults = 0;
  int no_of_redundant_faults = 0;
  int no_of_calls = 0;

  calculate_cc();
  calculate_co();

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

  // display_undetect();

  static_compression();

  display_test_patterns();

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
  enum DYNAMIC_TYPE { RANDOM, DETECTED_TIME, SCOPE };
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
    SCOPE:
      calculate_cc();
      calculate_co();
      std::sort(secondary_fault_list.begin(), secondary_fault_list.end(),
                [&](const fptr& a, const fptr& b) {
                    const wptr wa = sort_wlist[a->to_swlist], wb = sort_wlist[b->to_swlist];
                    double cca = (a->fault_type == STUCK0) ? wa->cc1 : wa->cc0;
                    double ccb = (b->fault_type == STUCK0) ? wb->cc1 : wb->cc0;
                    double coa = (a->io == GO) ? wa->co.back() : wa->co[a->index];
                    double cob = (b->io == GO) ? wb->co.back() : wb->co[b->index];
                    return ((cca * coa) < (ccb * cob));
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
      if (check_test()) {
        find_test = true;
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
	  
      for (i = 0; i < ncktin; i++) {
        switch (cktin[i]->value) {
          case 0:
          case 1: break;
          case D: cktin[i]->value = 1; break;
          case B: cktin[i]->value = 0; break;
          // case U: cktin[i]->value = rand()&01; break; // random fill U
        }
      }
      // display_io();

      if (tdf_podem_v1(fault) == TRUE) {
        string vec(ncktin + 1, '0');
        for (i = 0; i < ncktin; i++) {
          vec[i] = itoc(cktin[i]->value_v1);
        }
        vec.back() = itoc(cktin[0]->value);
        vectors.emplace_back(vec);
        // cerr << "vec = " << vec << endl;
        return(TRUE);
      }
    }
    else fprintf(stdout, "\n");  // do not random fill when multiple patterns per fault
  }
  else if (no_test) {
    /*fprintf(stdout,"redundant fault...\n");*/
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
  int i, ncktin;

  ncktin = cktin.size();

  /* shift v2, assign to value_v1, set fixed */
  for (i = 1; i < ncktin; ++i) {
    if (cktin[i]->value != U) {
      cktin[i - 1]->value_v1 = cktin[i]->value;
      cktin[i - 1]->fixed = true;
      cktin[i - 1]->flag |= CHANGED;
    }
    else {
      cktin[i - 1]->value_v1 = U;
      cktin[i - 1]->fixed = false;
      cktin[i - 1]->flag &= ~CHANGED;
    }
  }

  int ret = tdf_set_uniquely_implied_value(fault);

  for (i = 1; i < ncktin; ++i) {
    cktin[i]->flag &= ~CHANGED;
  }

  return ret;
}

int ATPG::tdf_set_uniquely_implied_value(const fptr fault) {
  wptr w;
  int pi_is_reach = FALSE;
  int i,nin;

  nin = fault->node->iwire.size();
  if (fault->io) w = fault->node->owire.front();  //  gate output fault, Fig.8.3
  else { // gate input fault.  Fig. 8.4 
    w = fault->node->iwire[fault->index]; 

    switch (fault->node->type) {
      case NOT:
      case BUF:
      break;
	    // return(pi_is_reach);

	    /* assign all side inputs to non-controlling values */
      case AND:
      case NAND:
         for (i = 0; i < nin; i++) {
           if (fault->node->iwire[i] != w) {
             switch (backward_imply(fault->node->iwire[i],1)) {
                case TRUE: pi_is_reach = TRUE; break;
                case CONFLICT: return(CONFLICT); break;
                case FALSE: break;
             }
           }
         }
         break;

      case OR:
      case NOR:
         for (i = 0; i < nin; i++) {
           if (fault->node->iwire[i] != w) {
             switch (backward_imply(fault->node->iwire[i],0)) {
                case TRUE: pi_is_reach = TRUE; break;
                case CONFLICT: return(CONFLICT); break;
                case FALSE: break;
             }
           }
         }
         break;
    }
  } // else , gate input fault 
  
	switch(backward_imply(w, fault->fault_type))
	{
		case TRUE: pi_is_reach = TRUE; break; // if the backward implication reaches any PI
		case CONFLICT: return CONFLICT; break; // if it is impossible to achieve or set the initial objective
		case FALSE: break; // if it has not reached any PI, let pi_is_reach remain FALSE
	}
   //----------------------------------------------------------------------------------

  return(pi_is_reach);
}/* end of tdf_set_uniquely_implied_value */

/* for backtrace */
int ATPG::tdf_backward_imply(const wptr current_wire, const int& desired_logic_value) {
  int pi_is_reach = FALSE;
  int i, nin;

  nin = current_wire->inode.front()->iwire.size();
  if (current_wire->flag & INPUT) { // if PI
    if (current_wire->value_v1 != U &&  
      current_wire->value_v1 != desired_logic_value) { 
      return(CONFLICT); // conlict with previous assignment
    }
    current_wire->value_v1 = desired_logic_value; // assign PI to the objective value
    current_wire->flag |= CHANGED; 
    // CHANGED means the logic value on this wire has recently been changed
    return(TRUE);
  }
  else { // if not PI
    switch (current_wire->inode.front()->type) {
      /* assign NOT input opposite to its objective ouput */
      /* go backward iteratively.  depth first search */
      case NOT:
        switch (backward_imply(current_wire->inode.front()->iwire.front(), (desired_logic_value ^ 1))) {
          case TRUE: pi_is_reach = TRUE; break;
          case CONFLICT: return(CONFLICT); break;
          case FALSE: break;
        }
        break;

		/* if objective is NAND output=zero, then NAND inputs are all ones  
		 * keep doing this back implication iteratively  */
      case NAND:
        if (desired_logic_value == 0) {
          for (i = 0; i < nin; i++) {
            switch (backward_imply(current_wire->inode.front()->iwire[i],1)) {
              case TRUE: pi_is_reach = TRUE; break;
              case CONFLICT: return(CONFLICT); break;
              case FALSE: break;
            }
          }
        }
        break;

      case AND:
        if (desired_logic_value == 1) {
          for (i = 0; i < nin; i++) {
            switch (backward_imply(current_wire->inode.front()->iwire[i],1)) {
              case TRUE: pi_is_reach = TRUE; break;
              case CONFLICT: return(CONFLICT); break;
              case FALSE: break;
            }
          }
        }
        break;

      case OR:
        if (desired_logic_value == 0) {
          for (i = 0; i < nin; i++) {
            switch (backward_imply(current_wire->inode.front()->iwire[i],0)) {
              case TRUE: pi_is_reach = TRUE; break;
              case CONFLICT: return(CONFLICT); break;
              case FALSE: break;
            }
          }
        }
        break;

      case NOR:
        if (desired_logic_value == 1) {
          for (i = 0; i < nin; i++) {
            switch (backward_imply(current_wire->inode.front()->iwire[i],0)) {
              case TRUE: pi_is_reach = TRUE; break;
              case CONFLICT: return(CONFLICT); break;
              case FALSE: break;
            }
          }
        }
        break;

      case BUF:
        switch (backward_imply(current_wire->inode.front()->iwire.front(),desired_logic_value)) {
          case TRUE: pi_is_reach = TRUE; break;
          case CONFLICT: return(CONFLICT); break;
          case FALSE: break;
        }
        break;
    }
	
    return(pi_is_reach);
  }
}/* end of tdf_backward_imply */

/* dynamic test compression by podem-x */
int ATPG::tdf_podem_x()  
{
  return FALSE;
}

/* static test compression */
void ATPG::static_compression()  
{
  int detect_num = 0;

  vector<int> order(vectors.size());
  vector<bool> dropped(vectors.size(), false);
  iota(order.begin(), order.end(), 0);
  reverse(order.begin(), order.end());

  for (int i = 0; i < 20; ++i) {
    generate_fault_list();
    int drop_count = 0;
    for (int s : order) {
      if (!dropped[s]) {
        tdfsim_a_vector(vectors[s], detect_num);
        if (detect_num == 0) {
          dropped[s] = true;
          ++drop_count;
        }
      }
    }
    // cerr << drop_count << endl;
    random_shuffle(order.begin(), order.end());
  }

  vector<string> compressed_vectors;
  for (size_t i = 0; i < vectors.size(); ++i) {
    if (!dropped[i]) {
      compressed_vectors.emplace_back(vectors[i]);
    }
  }
  vectors = compressed_vectors;
}

void ATPG::random_pattern_generation() {
  unordered_set<string> sVector;
  string vec1(cktin.size() + 1, '0');
  string vec2(cktin.size() + 1, '0');
  for (int i = 0; i < 100000; ++i) {
    for (int j = 0; j < cktin.size() + 1; ++j) {
      if (rand() % 2) {
        vec1[j] = '0';
        vec2[j] = '1';
      }
      else {
        vec1[j] = '1';
        vec2[j] = '0';
      }
    }
    if (sVector.find(vec1) == sVector.end()) {
      vectors.emplace_back(vec1);
      vectors.emplace_back(vec2);
      sVector.emplace(vec1);
      sVector.emplace(vec2);
    }
  }
}
