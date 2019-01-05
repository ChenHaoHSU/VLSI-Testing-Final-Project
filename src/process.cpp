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
  fprintf(stderr, "# Pre-process:\n");
  CASE c = identify_case();
  switch (c) {
    case C17:
      fprintf(stderr, "#   case: c17\n");
      this->backtrack_limit     = 500;
      this->backtrack_limit_v1  = 100;
      this->v2_loop_limit       = 1000;
      this->random_sim_num      = 50;
      this->random_sim_num_post = 50;
      break;
    case C432:
      fprintf(stderr, "#   case: c432\n");
      this->backtrack_limit     = 500;
      this->backtrack_limit_v1  = 100;
      this->v2_loop_limit       = 1000;
      this->random_sim_num      = 50;
      this->random_sim_num_post = 50;
      break;
    case C499:
      fprintf(stderr, "#   case: c499\n");
      this->backtrack_limit     = 500;
      this->backtrack_limit_v1  = 100;
      this->v2_loop_limit       = 1000;
      this->random_sim_num      = 50;
      this->random_sim_num_post = 50;
      break;
    case C880:
      fprintf(stderr, "#   case: c880\n");
      this->backtrack_limit     = 500;
      this->backtrack_limit_v1  = 100;
      this->v2_loop_limit       = 1000;
      this->random_sim_num      = 50;
      this->random_sim_num_post = 50;
      break;
    case C1355:
      fprintf(stderr, "#   case: c1355\n");
      this->backtrack_limit     = 500;
      this->backtrack_limit_v1  = 100;
      this->v2_loop_limit       = 1000;
      this->random_sim_num      = 50;
      this->random_sim_num_post = 50;
      break;
    case C1908:
      fprintf(stderr, "#   case: c1908\n");
      this->backtrack_limit     = 500;
      this->backtrack_limit_v1  = 100;
      this->v2_loop_limit       = 1000;
      this->random_sim_num      = 50;
      this->random_sim_num_post = 50;
      break;
    case C2670:
      fprintf(stderr, "#   case: c2670\n");
      this->backtrack_limit     = 500;
      this->backtrack_limit_v1  = 100;
      this->v2_loop_limit       = 1000;
      this->random_sim_num      = 50;
      this->random_sim_num_post = 50;
      break;
    case C3540:
      fprintf(stderr, "#   case: c3540\n");
      this->backtrack_limit     = 500;
      this->backtrack_limit_v1  = 100;
      this->v2_loop_limit       = 1000;
      this->random_sim_num      = 50;
      this->random_sim_num_post = 50;
      break;
    case C5315:
      fprintf(stderr, "#   case: c5315\n");
      this->backtrack_limit     = 500;
      this->backtrack_limit_v1  = 100;
      this->v2_loop_limit       = 1000;
      this->random_sim_num      = 50;
      this->random_sim_num_post = 50;
      break;
    case C6288:
      fprintf(stderr, "#   case: c6288\n");
      this->backtrack_limit     = 200;
      this->backtrack_limit_v1  = 100;
      this->v2_loop_limit       = 10;
      this->random_sim_num      = 20;
      this->random_sim_num_post = 20;
      break;
    case C7552:
      fprintf(stderr, "#   case: c7552\n");
      this->backtrack_limit     = 200;
      this->backtrack_limit_v1  = 100;
      this->v2_loop_limit       = 10;
      this->random_sim_num      = 20;
      this->random_sim_num_post = 20;
      break;
    default:
      fprintf(stderr, "#   case: other\n");
      this->backtrack_limit     = 200;
      this->backtrack_limit_v1  = 100;
      this->v2_loop_limit       = 10;
      this->random_sim_num      = 20;
      this->random_sim_num_post = 20;
      break;
  }
}

ATPG::CASE ATPG::identify_case() const {
  int nckt = sort_wlist.size();
  int ncktin = cktin.size();
  int ncktout = cktout.size();
  tuple<int, int, int> info(nckt, ncktin, ncktout); 
  if (info == make_tuple(11, 5, 2))            return C17;
  else if (info == make_tuple(281, 36, 7))     return C432;
  else if (info == make_tuple(595, 41, 32))    return C499;
  else if (info == make_tuple(605, 60, 26))    return C880;
  else if (info == make_tuple(595, 41, 32))    return C1355;
  else if (info == make_tuple(915, 33, 25))    return C1908;
  else if (info == make_tuple(2018, 233, 140)) return C2670;
  else if (info == make_tuple(2132, 50, 22))   return C3540;
  else if (info == make_tuple(2474, 178, 123)) return C5315;
  else if (info == make_tuple(4832, 32, 32))   return C6288;
  else if (info == make_tuple(5886, 207, 108)) return C7552;
  else return OTHER;
}

void ATPG::post_process() {
  fprintf(stderr, "# Post-process: (%lu)\n", vectors.size());
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
    random_sim_num = random_sim_num_post;
    static_compression();
  }
}

