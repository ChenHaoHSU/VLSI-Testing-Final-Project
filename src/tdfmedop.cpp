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
    int total_detect_num = 0;
    int total_backtrack_num = 0;
    int current_backtrack_num = 0;
    int aborted_fnum = 0;
    int redundant_fnum = 0;
    int call_num = 0;
    
    /* initialize fault primary members */
    initialize_fault_primary_record();

    LIFO d_tree;
    string vec, flag;

    /* generate test pattern */
    fptr fault_under_test = select_primary_fault();
    while (fault_under_test != nullptr) {
        cerr << "primary fault: fault no." << fault_under_test->fault_no << endl;
        /* test loop for this primary fault */
        bool is_test_generated = false;
        bool please_find_v2 = true;
        bool is_rollback = false; // The first time to v2 or once rejected from v1
        
        vec.resize(cktin.size()+1, 'x');
        flag.resize(cktin.size(), '0');
        
        int debug_counter = 0; // feel free to delete it
        while (!is_test_generated && (debug_counter < 1000)) {
            if (please_find_v2) {
                int find_v2 = tdf_medop_v2(fault_under_test, current_backtrack_num, d_tree, vec, flag,
                                              true /*primary*/, is_rollback);
                if (find_v2 == TRUE) {
                    if (!is_rollback)
                        cerr << "v2 found...";
                    please_find_v2 = false;
                } else {
                    cerr << "v2 failed" << endl;
                    break;
                }
            }
            else {
                LIFO tmp_d_tree = d_tree;
                string tmp_vec = vec;
                int find_v1 = tdf_medop_v1(fault_under_test, current_backtrack_num, tmp_d_tree, tmp_vec);
                if (find_v1 == TRUE) {
                    vectors.emplace_back(tmp_vec);
                    is_test_generated = true;
                    cerr << " v1 found! vector: " << tmp_vec << endl;
                } else {
                    please_find_v2 = true;
                    is_rollback = true;
                }
            }
            debug_counter++;
        }

        if (!is_test_generated) {
            cerr << " (" << debug_counter << ") ... ";
            cerr << "v1 failed finally!" << endl;
            }
        d_tree.clear();
        //switch (if_find_v2) {
        //    case TRUE:
        //        /* TODO: find v1 */
        //        vectors.emplace_back(vec_v2);
        //        tdfsim_a_vector(vec_v2, current_detect_num);
        //        total_detect_num += current_detect_num;
        //        break;
        //    case FALSE:
        //        fault_under_test->detect = REDUNDANT;
        //        redundant_fnum++;
        //        break;
        //    case MAYBE:
        //        aborted_fnum++;
        //        break;
        //}
        /* TODO: test v1 */
        if (is_test_generated) {
            tdfsim_a_vector(vectors.back(), current_detect_num);
        }
        fault_under_test->test_tried = true;
        fault_under_test = select_primary_fault();
        total_backtrack_num += current_backtrack_num; // accumulate number of backtracks
        call_num++;
    }

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
    /* is_primary : if this fault is a primary fault? */
    /* is_rollback : if this fault failed to find v1 and is rollbacked? */
    
    /* 1. primary and not rollback: we would restore from fault. */
    /* 2. primary and rollback: we would success from the last argument */
    /* 3. secondary and not rollback: we need empty d_tree, flag and success vector */
    /* 4. secondary and rollback: we would success from the last argument
    
    
    /* declare some parameters */
    int ncktin = cktin.size();
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
    
    bool rollback_trigger = (is_rollback) ? true : false;
    
    while ((current_backtrack_num < backtrack_limit) && (!no_test) && (!find_test)) {
        /* Check if test is possible. First find objective. */
        bool try_obj = find_objective(fault, obj_wire, obj_value);
        wpi = (try_obj) ? find_cool_pi(obj_wire, obj_value) : nullptr;
        if (rollback_trigger) {
            rollback_trigger = false;
            wpi = nullptr;
        }
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
            if (faulty_w = fault_evaluate(fault)) {
                forward_imply(faulty_w);
            }
            if (check_test()) {
                find_test = true;
            }
        }
    } // while (three conditions)
        
    /* Before exiting tdf_medop_v2, some information should be updated. */
    if (is_primary && find_test) {
        fault->primary_d_tree = d_tree;
        fault->primary_allassigned = extract_all_assigned_flag();
        fault->primary_vector = extract_vector_v2();
    }
    
    /* Before exiting tdf_medop_v2, some information should be cleared */
    initialize_all_assigned_flag();
    unmark_propagate_tree(fault->node);

    if (find_test) {
        assignments = extract_vector_v2();
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
    /* declare some shit */
    int ncktin = cktin.size();
    int desired_value = fault->fault_type;
    wptr faulty_w = sort_wlist[fault->to_swlist];

    /* initialize some shit */
    restore_vector_v1(assignments);

    /* get the fanin cone stuff */
    vector<wptr> fanin_cone_wlist;
    mark_fanin_cone(faulty_w, fanin_cone_wlist);
    for (wptr w : fanin_cone_wlist) {
        w->flag &= ~MARKED;
    }

    /* partial sim */
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

    int find_test = FALSE;
    current_backtrack_num = 0;
    wptr wpi;

    int tmp_desired_value = desired_value;
    wpi = find_cool_pi(faulty_w, tmp_desired_value);

    if (wpi != nullptr) {
        wpi->value = tmp_desired_value;
        wpi->flag |= CHANGED;
        d_tree.push_front(wpi);
    } else {
        while (!d_tree.empty() && current_backtrack_num <= backtrack_limit_v1) {
            wptr current_w = d_tree.front();
            /* CAREFUL!!! */
            /* pop */
            if (current_w->flag & ALL_ASSIGNED) {
                current_w->flag &= ~ALL_ASSIGNED;
                current_w->value = U;

                d_tree.pop_front();
                continue;
            }
            /* flip */
            else if (current_w->flag & MARKED) { // current_wire not all assigned
                current_w->value = current_w->value ^ 1;
                current_w->flag |= ALL_ASSIGNED;
                current_w->flag &= ~MARKED;
                ++current_backtrack_num;
            }
            /* first time */
            else {
                current_w->flag |= MARKED;
            }

            partial_sim(fanin_cone_wlist);
            if (faulty_w->value == desired_value) {
                find_test = TRUE;
                break;
            }
            else if (faulty_w->value == U) {
                tmp_desired_value = desired_value;
                wpi = find_cool_pi(faulty_w, tmp_desired_value);
                if (wpi != nullptr) {
                    wpi->value = tmp_desired_value;
                    wpi->flag |= CHANGED;
                    d_tree.push_front(wpi);
                }
            }
        }
    }

    /* initialize ALL_ASSIGNED flag */
    /* unmark fanin cone */
    for (int i = 0; i < ncktin; ++i) {
        cktin[i]->flag &= ~MARKED;
        cktin[i]->flag &= ~CHANGED;
        cktin[i]->flag &= ~ALL_ASSIGNED;
    }

    if (find_test) {
        assignments = extract_vector_v1();
        return TRUE;
    } else if (no_test) {
        return FALSE;
    } else {
        return MAYBE;
    }
}

void ATPG::mark_fanin_cone(const wptr w, vector<wptr> &fanin_cone_wlist) {
    if (w->flag & MARKED) {
        return;
    }
    int i, j, ninode, niwire;
    w->flag |= MARKED;
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
                new_obj_wire = (obj_value) ? find_easiest_control(obj_wire->inode.front())
                               : find_hardest_control(obj_wire->inode.front());
                break;
            case  NOR:
            case  AND:
                new_obj_wire = (obj_value) ? find_hardest_control(obj_wire->inode.front())
                               : find_easiest_control(obj_wire->inode.front());
                break;
            case  NOT:
            case  BUF:
                new_obj_wire = obj_wire->inode.front()->iwire.front();
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
