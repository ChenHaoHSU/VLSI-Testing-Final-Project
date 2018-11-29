/**********************************************************************/
/*           routines building the dummy input and output gate list;  */
/*           building the fault list;                                 */
/*           calculating the total fault coverage.                    */
/*                                                                    */
/*           Author: Bing-Chen (Benson) Wu                            */
/*           last update : 01/21/2018                                 */
/**********************************************************************/

#include "atpg.h"

/* the way of fault collapsing is different from what we teach in class
   need modification */
void ATPG::generate_fault_list(void) {
  int fault_num;
  wptr w;
  nptr n;
  fptr_s f;
  
  /* walk through every wire in the circuit*/
  for (auto pos = sort_wlist.crbegin(); pos != sort_wlist.crend(); ++pos) {
    w = *pos;
    n = w->inode.front();
    
    /* for each gate, create a gate output slow-to-rise (STR) fault */
    f = move(fptr_s(new(nothrow) FAULT));
    if (f == nullptr) error("No more room!");
    f->node = n;
    f->io = GO;
    f->fault_type = STR;
    f->to_swlist = w->wlist_index;
    f->eqv_fault_num = 1;
    num_of_gate_fault += f->eqv_fault_num; // accumulate total fault count
    flist_undetect.push_front(f.get()); // initial undetected fault list contains all faults
    flist.push_front(move(f));  // push into the fault list
    
    /* for each gate, create a gate output slow-to-fall (STF) fault */
    f = move(fptr_s(new(nothrow) FAULT));
    if (f == nullptr) error("No more room!");
    f->node = n;
    f->io = GO;
    f->fault_type = STF;
    f->to_swlist = w->wlist_index;
    f->eqv_fault_num = 1;
    num_of_gate_fault += f->eqv_fault_num;
    flist_undetect.push_front(f.get());
    flist.push_front(move(f));

    /*if w has multiple fanout branches */
    if (w->onode.size() > 1) {
      for (nptr nptr_ele: w->onode) {
        /* create STR for gate inputs  */
        f = move(fptr_s(new(nothrow) FAULT));
        if (f == nullptr) error("No more room!");
        f->node = nptr_ele;
        f->io = GI;
        f->fault_type = STR;
        f->to_swlist = w->wlist_index;
        f->eqv_fault_num = 1;
        /* f->index is the index number of gate input, 
            which GI fault is associated with*/
        for (int k = 0; k < nptr_ele->iwire.size(); k++) {  
          if (nptr_ele->iwire[k] == w) f->index = k;
        }
        num_of_gate_fault++;
        flist_undetect.push_front(f.get());
        flist.push_front(move(f));
        /* create STF for gate inputs  */
        f = move(fptr_s(new(nothrow) FAULT));
        if (f == nullptr) error("No more room!");
        f->node = nptr_ele;
        f->io = GI;
        f->fault_type = STF;
        f->to_swlist = w->wlist_index;
        f->eqv_fault_num = 1;
        for (int k = 0; k < nptr_ele->iwire.size(); k++) {
          if (nptr_ele->iwire[k] == w) f->index = k;
        }
        num_of_gate_fault++;
        flist_undetect.push_front(f.get());
        flist.push_front(move(f));
      }
    }
  }

  /*walk through all fautls, assign fault_no one by one  */
  fault_num = 0;
  for (fptr f: flist_undetect) {
    f->fault_no = fault_num;
    fault_num++;
    //cout << f->fault_no << f->node->name << ":" << (f->io?"O":"I") << (f->io?9:(f->index)) << "SA" << f->fault_type << endl;
  }

  // fprintf(stdout,"#number of equivalent faults = %d\n", fault_num);
}/* end of generate_fault_list */

/* computing the actual fault coverage */
void ATPG::compute_fault_coverage(void) {
  double gate_fault_coverage,eqv_gate_fault_coverage;
  int no_of_detect,eqv_no_of_detect,eqv_num_of_gate_fault;
  fptr f;

  debug = 0;
  no_of_detect = 0;
  gate_fault_coverage = 0.0;
  eqv_no_of_detect = 0;
  eqv_gate_fault_coverage = 0.0;
  eqv_num_of_gate_fault = 0;
  
  for (auto pos = flist.cbegin(); pos != flist.cend(); ++pos) {
    f = (*pos).get();
    if (debug) {
      if (f->detect != 0) {
        switch (f->node->type) {
          case INPUT:
            fprintf(stdout,"primary input: %s\n",f->node->owire.front()->name.c_str());
            break;
          case OUTPUT:
            fprintf(stdout,"primary output: %s\n",f->node->iwire.front()->name.c_str());
            break;
          default:
            fprintf(stdout,"gate: %s ;",f->node->name.c_str());
            if (f->io == GI) {
              fprintf(stdout,"input wire name: %s\n",f->node->iwire[f->index]->name.c_str());
            }
            else {
              fprintf(stdout,"output wire name: %s\n",f->node->owire.front()->name.c_str());
            }
            break;
        }
        fprintf(stdout,"fault_type = ");
        switch (f->fault_type) {
           case STUCK0:
            fprintf(stdout,"s-a-0\n"); break;
           case STUCK1:
            fprintf(stdout,"s-a-1\n"); break;
        }
        fprintf(stdout,"no of equivalent fault = %d\n",f->eqv_fault_num);
        fprintf(stdout,"detection flag = %d\n",f->detect);
        fprintf(stdout,"\n");
      }
    }
    if (f->detect == TRUE) {
      no_of_detect += f->eqv_fault_num;
      eqv_no_of_detect++;
    }
    eqv_num_of_gate_fault++;
  }
  if (num_of_gate_fault != 0) 
  gate_fault_coverage = (((double) no_of_detect) / num_of_gate_fault) * 100;
  if (eqv_num_of_gate_fault != 0) 
  eqv_gate_fault_coverage = (((double) eqv_no_of_detect) / eqv_num_of_gate_fault) * 100;
  
  /* print out fault coverage results */
  fprintf(stdout,"\n");
  fprintf(stdout,"#FAULT COVERAGE RESULTS :\n");
  fprintf(stdout,"#number of test vectors = %d\n",in_vector_no);
  fprintf(stdout,"#total number of gate faults = %d\n",num_of_gate_fault);
  fprintf(stdout,"#total number of detected faults = %d\n",no_of_detect);
  fprintf(stdout,"#total gate fault coverage = %5.2f%%\n",gate_fault_coverage);
  fprintf(stdout,"#number of equivalent gate faults = %d\n",eqv_num_of_gate_fault);
  fprintf(stdout,"#number of equivalent detected faults = %d\n",eqv_no_of_detect);
  fprintf(stdout,"#equivalent gate fault coverage = %5.2f%%\n",eqv_gate_fault_coverage);
  fprintf(stdout,"\n");
}/* end of compute_fault_coverage */

/* for each PI and PO in the whole circuit,
   create a dummy PI gate to feed each PI wire 
   create a dummy PO gate to feed each PO wire. */
/* why do we need dummy gate? */
void ATPG::create_dummy_gate(void) {
  int i;
  int num_of_dummy;
  nptr n;
  char sgate[25];
  char intstring[25];

  num_of_dummy = 0;

  /* create a dummy PI gate for each PI wire */
  for (i = 0; i < cktin.size(); i++) {
    num_of_dummy++;
	  
    /* the dummy gate name is  "dummay_gate#"  */
    sprintf(intstring, "%d", num_of_dummy); 
    sprintf(sgate,"dummy_gate%s",intstring);

    /* n is the dummy PI gate, cktin[i]is the original PI wire.  feed n to cktin[i] */ 
    n = getnode(string(sgate));
    n->type = INPUT;
    n->owire.push_back(cktin[i]);
    cktin[i]->inode.push_back(n);
  } // for i

  /* create a dummy PO gate for each PO wire */
  for (i = 0; i < cktout.size(); i++) {
    num_of_dummy++;

    /* the dummy gate name is  "dummay_gate#"  */
    sprintf(intstring, "%d", num_of_dummy); 
    sprintf(sgate,"dummy_gate%s",intstring);

    /* n is the dummy PO gate, cktout[i] is the original PO wire.  feed cktout[i] to n */
    n = getnode(sgate);
    n->type = OUTPUT;
    n->iwire.push_back(cktout[i]);
    cktout[i]->onode.push_back(n);
  } // for i
}/* end of create_dummy_gate */

char ATPG::itoc(const int& i) {
  switch (i) {
    case 1: return '1';
    case 2: return 'U';
    case 0: return '0';
  }
}
