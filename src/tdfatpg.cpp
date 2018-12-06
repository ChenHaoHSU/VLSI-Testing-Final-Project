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
  string vec;
  int current_detect_num = 0;
  int total_detect_num = 0;
  int total_no_of_backtracks = 0;  // accumulative number of backtracks
  int current_backtracks = 0;
  int no_of_aborted_faults = 0;
  int no_of_redundant_faults = 0;
  int no_of_calls = 0;

  fptr fault_under_test = select_primary_fault();

  /* generate test pattern */
  while (fault_under_test != nullptr) {
    switch (podem(fault_under_test, current_backtracks)) {
      case TRUE:
        /* form a vector */
        vec.clear();
        for (wptr w: cktin) {
          vec.push_back(itoc(w->value));
        }
        /*by defect, we want only one pattern per fault */
        /*run a fault simulation, drop ALL detected faults */
        if (total_attempt_num == 1) {
          fault_sim_a_vector(vec, current_detect_num);
          total_detect_num += current_detect_num;
        }
        /* If we want mutiple petterns per fault, 
         * NO fault simulation.  drop ONLY the fault under test */ 
        else {
          fault_under_test->detect = TRUE;
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


  display_undetect();
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
  fptr fault_selected = flist_undetect.front();

  return fault_selected;
}

/* generate test vector 1, considering fault/LOS constraints */
int ATPG::tdf_podem_v1(const fptr fault) 
{

  return FALSE;
}       

/* generate test vector 2, injection/activation/propagation */
int ATPG::tdf_podem_v2(const fptr fault) 
{

  return FALSE;
}       

/* dynamic test compression by podem-x */
int ATPG::tdf_podem_x()  
{

  return FALSE;
}

/* static test compression */
void ATPG::static_compression()  
{

}          
