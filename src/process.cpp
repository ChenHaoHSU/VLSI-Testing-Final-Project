/**********************************************************************/
/*           Preprocessing and Postprocessing:                      */
/*           Identify the case;                                       */
/*           Post-process the pattern list;                           */
/*                                                                    */
/*           Author: Hsiang-Ting Wen, Fu-Yu Chuang, and Chen-Hao Hsu  */
/*           Date : 1/4/2018 created                                  */
/**********************************************************************/

#include "atpg.h"

void ATPG::pre_process() {
  switch (identify_case()) {
    case C17:
    case C432:
    case C499:
    case C880:
    case C1355:
    case C1908:
    case C2670:
    case C3540:
    case C5315:
    case C6288:
    case C7552:
    default:
      this->backtrack_limit = 200;
      this->backtrack_limit_v1 = 100;
      this->v2_loop_limit = 10;
      this->random_sim_num = 20;
      break;
  }
}

ATPG::CASE ATPG::identify_case() const {
  int nckt = sort_wlist.size();
  int ncktin = cktin.size();
  int ncktout = cktout.size();
  tuple<int, int, int> info(nckt, ncktin, ncktout); 
  if (info == make_tuple(11, 5, 2)) { 
    fprintf(stderr, "# case: c17\n"); return C17;
  } else if (info == make_tuple(281, 36, 7)) {
    fprintf(stderr, "# case: c432\n"); return C432;
  } else if (info == make_tuple(595, 41, 32)) {
    fprintf(stderr, "# case: c499\n"); return C499;
  } else if (info == make_tuple(605, 60, 26)) {
    fprintf(stderr, "# case: c880\n"); return C880;
  } else if (info == make_tuple(595, 41, 32)) {
    fprintf(stderr, "# case: c1355\n"); return C1355;
  } else if (info == make_tuple(915, 33, 25)) {
    fprintf(stderr, "# case: c1908\n"); return C1908;
  } else if (info == make_tuple(2018, 233, 140)) {
    fprintf(stderr, "# case: c2670\n"); return C2670;
  } else if (info == make_tuple(2132, 50, 22)) {
    fprintf(stderr, "# case: c3540\n"); return C3540;
  } else if (info == make_tuple(2474, 178, 123)) {
    fprintf(stderr, "# case: c5315\n"); return C5315;
  } else if (info == make_tuple(4832, 32, 32)) {
    fprintf(stderr, "# case: c6288\n"); return C6288;
  } else if (info == make_tuple(5886, 207, 108)) {
    fprintf(stderr, "# case: c7552\n"); return C7552;
  } else { 
    fprintf(stderr, "# case: other\n"); return OTHER;
  }
}

void ATPG::post_process() {
  fprintf(stderr, "# Post-process:\n");
  int iter = 0;
  int detect_num;
  size_t nFaults;
  
  while (iter < 5) {

    /* tdf fault sim */
    generate_fault_list();
    for (const string& vec : vectors) {
      tdfsim_a_vector(vec, detect_num);
    }

    /* remove the fault with detected time 0 (i.e., unlucky faults) */
    flist_undetect.remove_if(
      [&](const fptr fptr_ele){
        return (fptr_ele->detected_time == 0);
      });

    /* calculate the number of lucky faults */
    nFaults = 0;
    for (const fptr f: flist_undetect) if (f) ++nFaults;
    fprintf(stderr, "#   Iter %d: Number of lucky faults: %lu\n", iter, nFaults);

    /* no more lucky faults, break out of the while loop */
    if (nFaults == 0) break;

    /* append the vectors that detects lucky fault to good_vectors*/
    vector<string> good_vectors;
    for (const string& vec : vectors) {
      detect_num = 0;
      tdfsim_a_vector(vec, detect_num, false);
      if (detect_num > 0) {
        good_vectors.emplace_back(vec);
      }
    }

    /* append n good vectors to vectors*/
    for (const string& vec : good_vectors) {
      for (int i = 0; i < detection_num; ++i) {
        vectors.emplace_back(vec);
      }
    }

    /* counter increases */
    ++iter;
  }

  /* do static compression if any good vector was found */
  if (iter > 0) {
    random_sim_num = 15;
    static_compression();
  }
}

