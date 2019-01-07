/**********************************************************************/
/*           Transition Delay Fault ATPG:                             */
/*           podem-x implementation                                   */
/*                                                                    */
/*           Author: Hsiang-Ting Wen, Fu-Yu Chuang, and Chen-Hao Hsu  */
/*           Date : 12/27/2018 created                                */
/**********************************************************************/

#include "atpg.h"

int ATPG::tdf_medop_x()
{
    int current_detect_num = 0;
    int current_drop_num = 0;
    int total_backtrack_num = 0;
    int current_backtrack_num = 0;
    int aborted_fnum = 0;
    int redundant_fnum = 0;
    int call_num = 0;

    /* tune parameters !!! */
    int v2_loop_limit = (v2_loop_max_trial == INT_MIN)? 1000: v2_loop_max_trial+1;
    int secondary_limit = 100;
    
    /* declare parameters */
    LIFO   d_tree;           // decision tree. must be clear in each outmost loop.
    string d_tree_flag;      // string of PI "ALL_ASSIGNED" flag. for recording decision tree state.
    string tdf_vec;          // tdf vector. must be clear in each outmost loop.
    fptr   primary_fault;    // primary fault
    fptr   secondary_fault;  // secondary fault
    
    bool   test_found;       // test found (for both v2, v1)
    bool   next_is_v2;       // test v2 in next loop
    bool   v1_not_found;     // failed to test v1 under a previous v2
    bool   is_primary;       // indicate if the fault is primary
    
    int    v2_loop_counter;
    int    secondary_counter;
    
    /* initialize fault primary members */
    initialize_fault_primary_record();
    
    /* select primary fault */
    primary_fault = select_primary_fault();
    
    while ((primary_fault != nullptr) && (primary_fault->score != INT_MIN)) {  // speed up
    // while (primary_fault != nullptr) {
        // print_fault_description(primary_fault);
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
        
        bool not_violate_constraint = tdf_hard_constraint_v1(primary_fault);
        
        /* main loop for finding primary fault v2-v1 pattern */
        while ((!test_found) && (v2_loop_counter < v2_loop_limit) && not_violate_constraint) {
            /* find v2 */
            if (next_is_v2) {
                int find_v2 = tdf_medop_v2(primary_fault, current_backtrack_num, d_tree, tdf_vec, 
                                           d_tree_flag, is_primary, v1_not_found);
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
                    test_found = true;
                } else {
                    next_is_v2 = true;
                    v1_not_found = true;
                }
            }
        }

        bool test_found_primary = test_found;
        
        if (test_found && pattern_has_enough_x(tdf_vec)) { 
            /* secondary fault */
            secondary_counter = 0;
            do {
                /* select secondary fault */
                secondary_fault = select_secondary_fault();
                secondary_counter++;
                
                /* initialize the controlling boolean parameters */
                test_found = false;
                next_is_v2 = true;
                v1_not_found = false;
                is_primary = false;
                v2_loop_counter = 0;
                
                /* initialize d_tree and d_tree_flag for secondary fault */
                /* success tdf_vec from primary */
                d_tree.clear();
                d_tree_flag.resize(cktin.size(), '0');
                
                /* main loop for finding secondary fault v2-v1 pattern */
                if (secondary_fault != nullptr) {
                    while (!test_found  && (v2_loop_counter < v2_loop_limit)) {
                        /* find v2 */
                        if (next_is_v2) {
                            int find_v2 = tdf_medop_v2(secondary_fault, current_backtrack_num, d_tree, tdf_vec, 
                                                    d_tree_flag, is_primary, v1_not_found);
                            if (find_v2 == TRUE) {
                                next_is_v2 = false;
                                v2_loop_counter++;
                            } else { break; }
                        }
                        /* find v1 */
                        else {
                            LIFO tmp_d_tree;
                            string tmp_vec = tdf_vec;
                            int find_v1 = tdf_medop_v1(secondary_fault, current_backtrack_num, tmp_d_tree, tmp_vec);
                            if (find_v1 == TRUE) {
                                tdf_vec = tmp_vec;
                                test_found = true;
                            } else {
                                next_is_v2 = true;
                                v1_not_found = true;
                            }
                        }
                    }
                }
            } while ((secondary_fault != nullptr) && (secondary_counter < secondary_limit) && pattern_has_enough_x(tdf_vec));
        }

        if (test_found_primary) {
            vector<string> expanded_patterns;
            expand_pattern(expanded_patterns, tdf_vec);
            int detect_time = primary_fault->detected_time;
            while (detect_time < detection_num) {
                for (string pattern : expanded_patterns) {
                    vectors.emplace_back(pattern);
                    tdfsim_a_vector(vectors.back(), current_detect_num, current_drop_num);
                    if (++detect_time >= detection_num) break;
                }
            }
        }      
        
        /* drop and select next primary fault */
        primary_fault->test_tried = true;
        primary_fault = select_primary_fault();
        total_backtrack_num += current_backtrack_num; // accumulate number of backtracks
        call_num++;
    } // while-loop for primary fault

    fprintf(stdout,"\n");
    fprintf(stdout,"#number of aborted faults = %d\n",aborted_fnum);
    fprintf(stdout,"\n");
    fprintf(stdout,"#number of redundant faults = %d\n",redundant_fnum);
    fprintf(stdout,"\n");
    fprintf(stdout,"#number of calling podem1 = %d\n",call_num);
    fprintf(stdout,"\n");
    fprintf(stdout,"#total number of backtracks = %d\n",total_backtrack_num);
    return 0;
}

int ATPG::tdf_medop_v2(const fptr fault, int& current_backtrack_num, LIFO& d_tree, string& assignments,
                       string& flags, const bool is_primary, const bool is_rollback)
{
    /* declare some parameters */
    wptr wpi, faulty_w;
    int obj_value;
    wptr obj_wire;
    
    /* initialize the circuit */
    /* load d_tree, vector, and ALL_ASSIGN flag */
    if (is_primary && (!is_rollback)) {
        d_tree      = fault->primary_d_tree;
        assignments = fault->primary_vector;
        flags       = fault->primary_allassigned;
    }
    restore_vector_v2(assignments);
    restore_all_assigned_flag(flags);
    mark_propagate_tree(fault->node);
    
    vector<wptr> fanin_cone_wlist;
    if (v1_strict_check) {
        int nckt = sort_wlist.size();
        for (int i = 0; i < nckt; ++i) {
            sort_wlist[i]->value_v1 = U;
        }

        tdf_set_sticky_constraint_v1(fault);

        faulty_w = sort_wlist[fault->to_swlist];
        mark_fanin_cone(faulty_w, fanin_cone_wlist);
        for (wptr w : fanin_cone_wlist) {
            w->flag &= ~MARKED2;
        }
        
        tdf_check_sticky_constraint_v1(fault, assignments, fanin_cone_wlist);
    }
    
    /* initial state */
    find_test = false;
    no_test = false;
    current_backtrack_num = 0; // This should be tuned later !!!

    /* If this fault is not rollback */
    /* we should set initial objective */ 
    if (!is_rollback) {
        /* set initial objective and assign one PI */
        int if_initial_objective_reached = set_uniquely_implied_value(fault);
        switch (if_initial_objective_reached) {
            /* TRUE: v2 test possible */
            case TRUE:
                sim();
                faulty_w = fault_evaluate(fault);
                if (faulty_w != nullptr) {
                    forward_imply(faulty_w);
                }
                if (check_test()) {
                    find_test = true;
                }
                break;
            /* CONFLICT: fault untestable */
            case CONFLICT:
                no_test = true;
                break;
            /* FALSE: need more assignments */
            case FALSE:
                break;
        }
    }
    
    while ((current_backtrack_num < backtrack_limit) && (!no_test) && (!find_test)) {
        /* Check if test is possible. First find objective. */
        bool try_obj = find_objective(fault, obj_wire, obj_value);
        wpi = (try_obj) ? find_cool_pi(obj_wire, obj_value) : nullptr;
        /* Finally we get a PI to assign. We should assign it and push it into d_tree. */
        if (wpi != nullptr) {
            wpi->value = obj_value;
            wpi->flag |= CHANGED;
            d_tree.push_front(wpi);
        }
        /* Finally no PI can be assigned. We should backtrack acccording to d_tree. */
        else {
            while (!d_tree.empty() && (wpi == nullptr)) {
                wptr current_w = d_tree.front();
                /* pop */
                if (current_w->flag & ALL_ASSIGNED) {
                    current_w->flag &= ~ALL_ASSIGNED;
                    current_w->value = U;
                    current_w->flag |= CHANGED;
                    d_tree.pop_front();
                }
                /* flip */
                else {
                    current_w->value = current_w->value ^ 1;
                    current_w->flag |= CHANGED;
                    current_w->flag |= ALL_ASSIGNED;
                    current_backtrack_num++;
                    wpi = current_w;
                }
            }
            /* d_tree empty -> no test possible */
            if (wpi == nullptr) {
                no_test = true;
            }
        }
        /* Here we get a newly assigned PI. */
        /* Now we should verify if this could generate a test. */
        /* If a test is generated, find_test is true and we could leave this while-loop. */
        if (wpi != nullptr) {
            sim();
            faulty_w = fault_evaluate(fault);
            if (faulty_w != nullptr) {
                forward_imply(faulty_w);
            }
            if (check_test()) {
                find_test = true;
            }
        }
        if (v1_strict_check) {
            if (!tdf_check_sticky_constraint_v1(fault, assignments, fanin_cone_wlist)) {
                //cerr << "findtest is ffffffffff" << endl;
                //cerr << extract_vector_v2(assignments) << endl;
                find_test = false;
            }
        }
    } // while (three conditions)
        
    /* Before exiting tdf_medop_v2, some information should be updated. */
    if (is_primary && find_test) {
        fault->primary_d_tree = d_tree;
        fault->primary_allassigned = extract_all_assigned_flag();
        fault->primary_vector = extract_vector_v2(assignments);
    }
    
    if (find_test) {
        flags = extract_all_assigned_flag();
    }
    
    /* Before exiting tdf_medop_v2, some information should be cleared */
    initialize_all_assigned_flag();
    unmark_propagate_tree(fault->node);

    if (find_test) {
        assignments = extract_vector_v2(assignments);
        initialize_vector();
        return TRUE;
    }
    else if (no_test) {
        return FALSE;
    }
    else {
        return MAYBE;
    }
}

int ATPG::tdf_medop_v1(const fptr fault, int& current_backtrack_num, LIFO& d_tree, string& assignments)
{
    /* declare some parameters */
    wptr wpi;
    int ncktin = cktin.size();
    int desired_value = fault->fault_type;
    wptr faulty_w = sort_wlist[fault->to_swlist];

    /* initialize the circuit */
    restore_vector_v1(assignments);
    
    /* initial state */
    find_test = false;
    no_test = false;
    current_backtrack_num = 0;

    /* get the fanin cone stuff */
    vector<wptr> fanin_cone_wlist;
    mark_fanin_cone(faulty_w, fanin_cone_wlist);
    for (wptr w : fanin_cone_wlist) {
        w->flag &= ~MARKED2;
    }
    
    /* define partial sim */
    auto partial_sim = [&] (vector<wptr> &wlist) -> void {
        for (int i = 0, nwire = wlist.size(); i < nwire; ++i) {
            if (wlist[i]->flag & INPUT) continue;
                /* CAREFUL!!! */
                evaluate(wlist[i]->inode.front());
            }
        return;
    };

    /* sim */
    partial_sim(fanin_cone_wlist);
    
    /* determined if the faulty wire is already assigned */
    if (faulty_w->value != U) {
        return (faulty_w->value == desired_value) ? TRUE : FALSE;
    }

    while ((current_backtrack_num <= backtrack_limit_v1) && (!no_test) && (!find_test)) {
        int obj_value = desired_value;
        wpi = find_cool_pi(faulty_w, obj_value);
        /* Finally we get a PI to assign. We should assign it and push it into d_tree. */
        if (wpi != nullptr) {
            wpi->value = obj_value;
            wpi->flag |= CHANGED;
            d_tree.push_front(wpi);
        }
        /* Finally no PI can be assigned. We should backtrack acccording to d_tree. */
        else {
            while (!d_tree.empty() && (wpi == nullptr)) {
                wptr current_w = d_tree.front();
                /* pop */
                if (current_w->flag & ALL_ASSIGNED) {
                    current_w->flag &= ~ALL_ASSIGNED;
                    current_w->value = U;
                    current_w->flag |= CHANGED;
                    d_tree.pop_front();
                }
                /* flip */
                else {
                    current_w->value = current_w->value ^ 1;
                    current_w->flag |= CHANGED;
                    current_w->flag |= ALL_ASSIGNED;
                    current_backtrack_num++;
                    wpi = current_w;
                }
            }
            /* d_tree empty -> no test possible */
            if (wpi == nullptr) {
                no_test = true;
            }
        }
        /* Here we get a newly assigned PI. */
        /* Now we should verify if this could generate a test. */
        /* If a test is generated, find_test is true and we could leave this while-loop. */
        if (wpi != nullptr) {
            partial_sim(fanin_cone_wlist);
            if (faulty_w->value == desired_value) {
                find_test = true;
            }
        }
    } // while (three conditions)

    /* Before exiting tdf_medop_v1, some information should be cleared */
    for (int i = 0; i < ncktin; ++i) {
        cktin[i]->flag &= ~MARKED2;
        cktin[i]->flag &= ~CHANGED;
        cktin[i]->flag &= ~ALL_ASSIGNED;
    }

    if (find_test) {
        assignments = extract_vector_v1(assignments);
        return TRUE;
    } else if (no_test) {
        return FALSE;
    } else {
        return MAYBE;
    }
}

void ATPG::mark_fanin_cone(const wptr w, vector<wptr> &fanin_cone_wlist) {
    if (w->flag & MARKED2) {
        return;
    }
    int i, j, ninode, niwire;
    w->flag |= MARKED2;
    for (i = 0, ninode = w->inode.size(); i < ninode; ++i) {
        for (j = 0, niwire = w->inode[i]->iwire.size(); j < niwire; ++j) {
            mark_fanin_cone(w->inode[i]->iwire[j], fanin_cone_wlist);
        }
    }
    fanin_cone_wlist.emplace_back(w);
}

bool ATPG::find_objective(const fptr fault, wptr& obj_wire, int& obj_value)
{
    nptr n;
    /* if the fault is not on PO */
    if (fault->node->type != OUTPUT) {

        /* if the faulty gate output is not U, it should be D or D' */
        if (fault->node->owire.front()->value ^ U) {
            if (!((fault->node->owire.front()->value == B) || (fault->node->owire.front()->value == D))) {
                return false;
            }
            /* find the next gate n to propagate */
            if (!(n = find_propagate_gate(fault->node->owire.front()->level))) {
                return false;
            }
            /* determine objective level according to the type of n. */
            switch(n->type) {
                case  AND:
                case  NOR:
                    obj_value = 1;
                    break;
                case NAND:
                case   OR:
                    obj_value = 0;
                    break;
                default:
                    return false;
            }
            obj_wire = n->owire.front();
        } else { // if faulty gate output is U

            /* if X path disappear, return false  */
            if (!(trace_unknown_path(fault->node->owire.front()))) {
                return false;
            }

            /* if fault is a GO fault */
            if (fault->io) {
                obj_value = (fault->fault_type) ? 0 : 1;
                obj_wire = fault->node->owire.front();
            }
            /* if fault is a GI fault */
            else {
                /* if faulted input is not U */
                if (fault->node->iwire[fault->index]->value ^ U) {
                    /* determine objective value according to GUT type. Fig 8.9*/
                    switch (fault->node->type) {
                        case  AND:
                        case  NOR:
                            obj_value = 1;
                            break;
                        case NAND:
                        case   OR:
                            obj_value = 0;
                            break;
                        default:
                            return false;
                    }
                    obj_wire = fault->node->owire.front();
                } else { // if faulted input is U
                    obj_value = (fault->fault_type) ? 0 : 1;
                    obj_wire = fault->node->iwire[fault->index];
                }
            }
        }
    }
    /* if the fault is on PO */
    else {
        /* if faulty PO is still unknown */
        if (fault->node->iwire.front()->value == U) {
            /*objective level is opposite to the stuck fault */
            obj_value = (fault->fault_type) ? 0 : 1;
            obj_wire = fault->node->iwire.front();
        } else {
            return false;
        }
    }
    return true;
}

ATPG::wptr ATPG::find_cool_pi(const wptr obj_wire, int& obj_value)
{
    wptr new_obj_wire;

    /* if PI, assign the same value as objective */
    if (obj_wire->flag & INPUT) {
        return obj_wire;
    }

    /* if not PI, backtrace to PI */
    else {
        switch(obj_wire->inode.front()->type) {
            case   OR:
            case NAND:
                //new_obj_wire = (obj_value) ? find_easiest_control(obj_wire->inode.front())
                //               : find_hardest_control(obj_wire->inode.front());
                new_obj_wire = (obj_value) ? find_easiest_control_scoap(obj_wire->inode.front(), obj_value)
                                 : find_hardest_control_scoap(obj_wire->inode.front(), obj_value);
                break;
            case  NOR:
            case  AND:
                //new_obj_wire = (obj_value) ? find_hardest_control(obj_wire->inode.front())
                //               : find_easiest_control(obj_wire->inode.front());
                new_obj_wire = (obj_value) ? find_hardest_control_scoap(obj_wire->inode.front(), obj_value)
                               : find_easiest_control_scoap(obj_wire->inode.front(), obj_value);
                break;
            case  NOT:
            case  BUF:
                new_obj_wire = obj_wire->inode.front()->iwire.front();
                break;
            default:
                fprintf(stderr,"Something wrong in find_cool_pi(const wptr obj_wire, int& obj_value)\n");
                new_obj_wire = nullptr;
                break;
        }

        switch (obj_wire->inode.front()->type) {
            case  NOT:
            case  NOR:
            case NAND:
                obj_value = obj_value ^ 1;
                break;
            default:
                break;
        }
        if (new_obj_wire) {
            return find_cool_pi(new_obj_wire, obj_value);
        }
        else {
            return nullptr;
        }
    }
}

ATPG::wptr ATPG::find_hardest_control_scoap(const nptr n, const int value) 
{
    int max_scoap = INT_MIN;
    wptr hardest_control = nullptr;

    for (wptr w : n->iwire) {
        if (w->value == U) {
            if (value == 0) {
                hardest_control = (w->cc0 > max_scoap)? w: hardest_control;
                max_scoap = (w->cc0 > max_scoap)? w->cc0: max_scoap;
            } else {
                assert(value == 1);
                hardest_control = (w->cc1 > max_scoap)? w: hardest_control;
                max_scoap = (w->cc1 > max_scoap)? w->cc1: max_scoap;
            }
        }
    }

    return hardest_control;
}



ATPG::wptr ATPG::find_easiest_control_scoap(const nptr n, const int value) 
{
    int min_scoap = INT_MAX;
    wptr easiest_control = nullptr;

    for (wptr w : n->iwire) {
        if (w->value == U) {
            if (value == 0) {
                easiest_control = (w->cc0 < min_scoap)? w: easiest_control;
                min_scoap = (w->cc0 < min_scoap)? w->cc0: min_scoap;
            } else {
                assert(value == 1);
                easiest_control = (w->cc1 < min_scoap)? w: easiest_control;
                min_scoap = (w->cc1 < min_scoap)? w->cc1: min_scoap;
            }
        }
    }

    return easiest_control;
}