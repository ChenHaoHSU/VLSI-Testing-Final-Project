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
