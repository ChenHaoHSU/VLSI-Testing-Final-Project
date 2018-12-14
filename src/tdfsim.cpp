/**********************************************************************/
/*  Parallel-Fault Event-Driven Transition Delay Fault Simulator      */
/*                                                                    */
/**********************************************************************/

#include "atpg.h"
#include <cassert>

/* pack 16 faults into one packet.  simulate 16 faults togeter. 
 * the following variable name is somewhat misleading */
#define num_of_pattern 16

void ATPG::transition_delay_fault_simulation() {
  int i;
  int current_detect_num = 0;
  int total_detect_num = 0;

  // debug = 1;

  /* for every vector */
  for (i = vectors.size() - 1; i >= 0; i--) {
    tdfsim_a_vector(vectors[i], current_detect_num);
    total_detect_num += current_detect_num;
    fprintf(stdout, "vector[%d] detects %d faults (%d)\n", i, current_detect_num, total_detect_num);
  }

  /* print results */
  fprintf(stdout, "\n# Result:\n");
  fprintf(stdout, "-----------------------\n");
  fprintf(stdout, "# total transition delay faults: %d\n", num_of_gate_fault);
  fprintf(stdout, "# total detected faults: %d\n", total_detect_num);
  fprintf(stdout, "# fault coverage: %f %%\n", ((double)total_detect_num/(double)num_of_gate_fault)*100);
}

/* transition delay fault simulate a single test vector */
void ATPG::tdfsim_a_vector(const string& vec, int& num_of_current_detect) {
  
  wptr w,faulty_wire;
  /* array of 16 fptrs, which points to the 16 faults in a simulation packet  */
  fptr simulated_fault_list[num_of_pattern];
  fptr f;
  int fault_type;
  int i, start_wire_index, nckt;
  int num_of_fault;
  bool fault_active;
  
  num_of_fault = 0; // counts the number of faults in a packet

  /* num_of_current_detect is used to keep track of the number of undetected
   * faults detected by this vector, initialize it to zero */
  num_of_current_detect = 0;

  /* Keep track of the minimum wire index of 16 faults in a packet.
   * the start_wire_index is used to keep track of the
   * first gate that needs to be evaluated.
   * This reduces unnecessary check of scheduled events.*/
  start_wire_index = 1e9;
  
  /*************************
   * V1 simulation
   *************************/
  /* for every input, set its value to the current vector value */
  for(i = 0; i < cktin.size(); i++) {
    cktin[i]->value = ctoi(vec[i]);
  }
  /* initialize the circuit - mark all inputs as changed and all other
   * nodes as unknown (2) */
  nckt = sort_wlist.size();
  for (i = 0; i < nckt; i++) {
    sort_wlist[i]->flag &= ~CHANGED;
    if (i < cktin.size()) {
      sort_wlist[i]->flag |= CHANGED;
    }
    else {
      sort_wlist[i]->value = U;
    }
  }
  sim(); /* do a fault-free simulation, see sim.c */
  if (debug) { display_io(); }

  /* expand the fault-free 0,1,2 value into 32 bits (2 = unknown)  
   * and store it in wire_value1 */
  for (i = 0; i < nckt; i++) {
    sort_wlist[i]->value_v1 = sort_wlist[i]->value;
  } // for i

  /*************************
   * V2 simulation
   *************************/
  /* for every input, set its value to V2 */
  for(i = 0; i < cktin.size(); i++) {
    cktin[i]->value = (i == 0 ? ctoi(vec.back()) : ctoi(vec[i-1]));
  }
  /* initialize the circuit - mark all inputs as changed and all other
   * nodes as unknown (2) */
  nckt = sort_wlist.size();
  for (i = 0; i < nckt; i++) {
    sort_wlist[i]->flag &= ~CHANGED;
    if (i < cktin.size()) {
      sort_wlist[i]->flag |= CHANGED;
    }
    else {
      sort_wlist[i]->value = U;
    }
  }
  sim(); /* do a fault-free simulation, see sim.c */
  if (debug) { display_io(); }

  /* expand the fault-free 0,1,2 value into 32 bits (2 = unknown)  
   * and store it in wire_value2 */
  for (i = 0; i < nckt; i++) {
    switch (sort_wlist[i]->value) {
      case 1: sort_wlist[i]->wire_value1 = ALL_ONE; // 11 represents logic one
              sort_wlist[i]->wire_value2 = ALL_ONE; break;
      case 2: sort_wlist[i]->wire_value1 = 0x55555555; // 01 represents unknown
              sort_wlist[i]->wire_value2 = 0x55555555; break;
      case 0: sort_wlist[i]->wire_value1 = ALL_ZERO; // 00 represents logic zero
              sort_wlist[i]->wire_value2 = ALL_ZERO; break;
    }
  } // for i

  /* walk through every undetected fault
   * the undetected fault list is linked by pnext_undetect */
  for (auto pos = flist_undetect.cbegin(); pos != flist_undetect.cend(); ++pos) {
    f = *pos;
    if (f->detect == REDUNDANT) { continue; } /* ignore redundant faults */

    /* consider only active (aka. excited) fault
     * (STR(0) with correct output of 0 or STF(1) with correct output of 1) */

    fault_active = false;
    switch (f->fault_type) {
      case STR: fault_active = (sort_wlist[f->to_swlist]->value_v1 == 0 &&
                                sort_wlist[f->to_swlist]->value == 1);
                break;
      case STF: fault_active = (sort_wlist[f->to_swlist]->value_v1 == 1 &&
                                sort_wlist[f->to_swlist]->value == 0);
                break;
    }

    if (fault_active) {
	    /* if f is a primary output or is directly connected to an primary output
	     * the fault is detected */
      if ((f->node->type == OUTPUT) ||
	        (f->io == GO && sort_wlist[f->to_swlist]->flag & OUTPUT)) {
        f->detect = TRUE;
      }
      else {
      
        /* if f is an gate output fault */
        if (f->io == GO) {
      
	        /* if this wire is not yet marked as faulty, mark the wire as faulty
	         * and insert the corresponding wire to the list of faulty wires. */
	        if (!(sort_wlist[f->to_swlist]->flag & FAULTY)) {
	          sort_wlist[f->to_swlist]->flag |= FAULTY;
            wlist_faulty.push_front(sort_wlist[f->to_swlist]);
	        }
      
          /* add the fault to the simulated fault list and inject the fault */
          simulated_fault_list[num_of_fault] = f;
          tdf_inject_fault_value(sort_wlist[f->to_swlist], num_of_fault, f->fault_type); 
	    
	        /* mark the wire as having a fault injected 
	         * and schedule the outputs of this gate */
          sort_wlist[f->to_swlist]->flag |= FAULT_INJECTED;
          for (auto pos_n = sort_wlist[f->to_swlist]->onode.cbegin(), end_n = sort_wlist[f->to_swlist]->onode.cend(); pos_n != end_n; ++pos_n) {
            (*pos_n)->owire.front()->flag |= SCHEDULED;
          }
	    
          /* increment the number of simulated faults in this packet */
          num_of_fault++;
          /* start_wire_index keeps track of the smallest level of fault in this packet.
           * this saves simulation time.  */
          start_wire_index = min(start_wire_index, f->to_swlist);
        }  // if gate output fault
	    
        /* the fault is a gate input fault */
        else {
	    
	        /* if the fault is propagated, set faulty_wire equal to the faulty wire.
	         * faulty_wire is the gate output of f.  */
          faulty_wire = tdf_get_faulty_wire(f, fault_type);
	        if (faulty_wire != nullptr) {
            
            /* if the faulty_wire is a primary output, it is detected */
            if (faulty_wire->flag & OUTPUT) {
              f->detect = TRUE;
            }
            else {
		          /* if faulty_wire is not already marked as faulty, mark it as faulty
		           * and add the wire to the list of faulty wires. */
		          if (!(faulty_wire->flag & FAULTY)) {
		            faulty_wire->flag |= FAULTY;
                wlist_faulty.push_front(faulty_wire);
		          }
		  
              /* add the fault to the simulated list and inject it */
              simulated_fault_list[num_of_fault] = f;
              tdf_inject_fault_value(faulty_wire, num_of_fault, fault_type);
		  
		          /* mark the faulty_wire as having a fault injected
		           *  and schedule the outputs of this gate */
		          faulty_wire->flag |= FAULT_INJECTED;
              for (auto pos_n = faulty_wire->onode.cbegin(), end_n = faulty_wire->onode.cend(); pos_n != end_n; ++pos_n) {
                (*pos_n)->owire.front()->flag |= SCHEDULED;
              }
		   
              num_of_fault++;
              start_wire_index = min(start_wire_index, f->to_swlist);
            }
          }
        }
      } // if  gate input fault
    } // if fault is active

    /*
     * fault simulation of a packet 
     */
    
    /* if this packet is full (16 faults)
     * or there is no more undetected faults remaining (pos points to the final element of flist_undetect),
     * do the fault simulation */
    if ((num_of_fault == num_of_pattern) || (next(pos,1) == flist_undetect.cend())) {
	  
	    /* starting with start_wire_index, evaulate all scheduled wires
	     * start_wire_index helps to save time. */
	    for (i = start_wire_index; i < nckt; i++) {
	      if (sort_wlist[i]->flag & SCHEDULED) {
          sort_wlist[i]->flag &= ~SCHEDULED;
          fault_sim_evaluate(sort_wlist[i]);
	      }
	    } /* event evaluations end here */
	  
	   /* pop out all faulty wires from the wlist_faulty
		 * if PO's value is different from good PO's value, and it is not unknown
		 * then the fault is detected.
		 * 
		 * IMPORTANT! remember to reset the wires' faulty values back to fault-free values.
	   */
      while(!wlist_faulty.empty()) {
        w = wlist_faulty.front();
        wlist_faulty.pop_front();
	      //printf("before : %d\n", w->flag);
	      w->flag &= ~FAULTY;
	      w->flag &= ~FAULT_INJECTED;
	      w->fault_flag &= ALL_ZERO;
	      //printf("after  : %d\n", w->flag);
        /*TODO*/
        //Hint:Use mask to get the value of faulty wire and check every fault in packet
	      if (w->flag & OUTPUT) { // if primary output 
          for (i = 0; i < num_of_fault; i++) { // check every undetected fault
            if (!(simulated_fault_list[i]->detect)) {
              if ((w->wire_value1 & Mask[i]) ^    // if value1 != value2
                  (w->wire_value2 & Mask[i])) {
                if (((w->wire_value1 & Mask[i]) ^ Unknown[i])&&  // and not unknowns
                    ((w->wire_value2 & Mask[i]) ^ Unknown[i])){
                  simulated_fault_list[i]->detect = TRUE;  // then the fault is detected
                }
              }
            }
          }
	      }
	      w->wire_value2 = w->wire_value1; // reset to fault-free values
        /*TODO*/
	    } // pop out all faulty wires
      num_of_fault = 0;  // reset the counter of faults in a packet
      start_wire_index = 10000;  //reset this index to a very large value.
    } // end fault sim of a packet
  } // end loop. for f = flist

  /* fault dropping  */
  flist_undetect.remove_if(
    [&](const fptr fptr_ele){
      if (fptr_ele->detect == TRUE) {
        fptr_ele->detected_time += 1;
        if (fptr_ele->detected_time >= detection_num) {
          num_of_current_detect += fptr_ele->eqv_fault_num;
          return true;
        }
        else {
          fptr_ele->detect = FALSE;
          return false;
        }
      }
      else {
        return false;
      }
    });
}

/* Given a gate-input fault f, check if f is propagated to the gate output.
   If so, returns the gate output wire as the faulty_wire.
   Also returns the gate output fault type. 
*/
ATPG::wptr ATPG::tdf_get_faulty_wire(const fptr f, int& fault_type) {
  int i, nin;
  bool is_faulty;

  is_faulty = true;
  wptr faulty_wire;
  nin = f->node->iwire.size();
  switch(f->node->type) {
    case NOT:
      switch (f->fault_type) {
        case STR: fault_type = STF; break;
        case STF: fault_type = STR; break;
      }
      break;
    case BUF:
      switch (f->fault_type) {
        case STR: fault_type = STR; break;
        case STF: fault_type = STF; break;
      }
      break;
    case AND:
      for (i = 0; i < nin; i++) {
        if (f->node->iwire[i] != sort_wlist[f->to_swlist]) {
          if (f->node->iwire[i]->value != 1) {
            is_faulty = false;  // not propagated
          }
        }
      }
      switch (f->fault_type) {
        case STR: fault_type = STR; break;
        case STF: fault_type = STF; break;
      }
      break;
    case NAND:
      for (i = 0; i < nin; i++) {
        if (f->node->iwire[i] != sort_wlist[f->to_swlist]) {
          if (f->node->iwire[i]->value != 1) {
            is_faulty = false;
          }
        }
      }
      switch (f->fault_type) {
        case STR: fault_type = STF; break;
        case STF: fault_type = STR; break;
      }
      break;
    case OR:
      for (i = 0; i < nin; i++) {
        if (f->node->iwire[i] != sort_wlist[f->to_swlist]) {
          if (f->node->iwire[i]->value != 0) {
            is_faulty = false;
          }
        }
      }
      switch (f->fault_type) {
        case STR: fault_type = STR; break;
        case STF: fault_type = STF; break;
      }
      break;
    case  NOR:
      for (i = 0; i < nin; i++) {
        if (f->node->iwire[i] != sort_wlist[f->to_swlist]) {
          if (f->node->iwire[i]->value != 0) {
            is_faulty = false;
          }
        }
      }
      switch (f->fault_type) {
        case STR: fault_type = STF; break;
        case STF: fault_type = STR; break;
      }
      break;
    case XOR:
      for (i = 0; i < nin; i++) {
        if (f->node->iwire[i] != sort_wlist[f->to_swlist]) {
          if (f->node->iwire[i]->value == 0) {
            fault_type = f->fault_type;
          }
          else {
            fault_type = f->fault_type ^ 1;
          }
        }
      }
      break;
    case EQV:
      for (i = 0; i < nin; i++) {
        if (f->node->iwire[i] != sort_wlist[f->to_swlist]) {
          if (f->node->iwire[i]->value == 0) {
            fault_type = f->fault_type ^ 1;
          }
          else {
            fault_type = f->fault_type;
          }
        }
      }
      break;
  }
  if (is_faulty) {
    return(f->node->owire.front());
  }
  return(nullptr);
}/* end of tdf_get_faulty_wire */

/* This function injects a fault 
 * 32 bits(16 faults) in a word, 
 * mask[bit position]=11 is the bit position of the fault
 */
void ATPG::tdf_inject_fault_value(const wptr faulty_wire, const int& bit_position, const int& fault_type) {
  /*TODO*/
  //Hint use mask to inject fault to the right position
  if (fault_type == STF) faulty_wire->wire_value2 |= Mask[bit_position];
  if (fault_type == STR) faulty_wire->wire_value2 &= ~Mask[bit_position];
  faulty_wire->fault_flag |= Mask[bit_position];// bit position of the fault 
  /*TODO*/
}/* end of tdf_inject_fault_value */
