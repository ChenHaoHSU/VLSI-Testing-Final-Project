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
    string vec;
    int current_detect_num = 0;
    int total_detect_num = 0;
    int total_backtrack_num = 0;
    int current_backtrack_num = 0;
    int aborted_fnum = 0;
    int redundant_fnum = 0;
    int call_num = 0;
    
    fptr fault_under_test = select_primary_fault();
    
    LIFO d_tree_v2;
    string vec_v2;
    /* generate test pattern */
    while (fault_under_test != nullptr) {
        int if_find_v2 = tdf_medop_v2(fault_under_test, current_backtrack_num, d_tree_v2, vec_v2);
        switch (if_find_v2) {
            case TRUE:
                /* TODO: find v1 */
                tdfsim_a_vector(vec_v2, current_detect_num);
                total_detect_num += current_detect_num;
                break;
            case FALSE:
                fault_under_test->detect = REDUNDANT;
                redundant_fnum++;
                break;
            case MAYBE:
                aborted_fnum++;
                break;
        }
        /* TODO: test v1 */
        
        fault_under_test->test_tried = true;
        fault_under_test = nullptr;
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

int ATPG::tdf_medop_v2(const fptr fault, int& current_backtrack_num, LIFO& d_tree, string& assignments)
{
    /* declare some shit */
    int ncktin = cktin.size();
    wptr wpi, faulty_w;
    int obj_value;
    wptr obj_wire;
    
    /* initialize some shit */
    for (int i = 0; i < sort_wlist.size(); i++) {
        sort_wlist[i]->value = U;
    }
    for (int i = 0; i < ncktin; i++) {
        int vec_index = (i == 0) ? ncktin : (i-1);
        switch(assignments[vec_index]) {
            case '0':
                cktin[i]->value = 0;
                cktin[i]->flag |= CHANGED;
                break;
            case '1':
                cktin[i]->value = 1;
                cktin[i]->flag |= CHANGED;
                break;
            case 'x':
                cktin[i]->value = U;
                cktin[i]->flag |= CHANGED;
                break;
            default:
                assert(0);
        }
    }
    sim();
    
    current_backtrack_num = 0;
    find_test = false;
    no_test = false;
    assignments.clear();
    assignments.resize(ncktin + 1, 'x');
    
    /* This is actually "mark fanout cone". */
    mark_propagate_tree(fault->node);
    
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
    
    while ((current_backtrack_num < backtrack_limit) && (!no_test) && (!find_test)) {
        /* check if test is possible. first find objective. */
        bool try_obj = find_objective(fault, obj_wire, obj_value);
        if (try_obj) {
            wpi = find_cool_pi(obj_wire, obj_value);
        } else {
            wpi = nullptr;
        }
        /* finally we get a PI to assign */
        if (wpi != nullptr) {
            wpi->value = obj_value;
            wpi->flag |= CHANGED;
            d_tree.push_front(wpi);
        }
        /* no PI can be assigned. we should backtrack. */
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
            /* decision tree empty -> no test possible */
            if (wpi == nullptr) {
                no_test = true;
            }
        }
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
    
    /* clear everthing */
    for (wptr wptr_ele: d_tree) {
        wptr_ele->flag &= ~ALL_ASSIGNED;
    }
    /* This is actually "unmark fanout cone". */
    unmark_propagate_tree(fault->node);
    
    if (find_test) {
        for (int i = 0; i < ncktin; i++) {
            int vec_index = (i == 0) ? ncktin : (i-1);
            switch (cktin[i]->value) {
                case 0:
                case B:
                    assignments[vec_index] = '0';
                    break;
                case 1:
                case D:
                    assignments[vec_index] = '1';
                    break;
                case U:
                    assignments[vec_index] = 'x';
                    break;
            }
        }
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
    int obj_value;
    wptr faulty_w = sort_wlist[fault->to_swlist];
    
    /* reset */
    for (int i = 0; i < ncktin; ++i) {
        cktin[i]->flag &= ~MARKED;
        cktin[i]->flag &= ~CHANGED;
        cktin[i]->flag &= ~ALL_ASSIGNED;
    }
    
    /* initialize some shit */
    for (int i = 0; i < sort_wlist.size(); i++) {
        sort_wlist[i]->value = U;
    }
    for (int i = 0; i < ncktin; i++) {
        switch(assignments[i]) {
            case '0':
                cktin[i]->value = 0;
                cktin[i]->flag |= CHANGED;
                break;
            case '1':
                cktin[i]->value = 1;
                cktin[i]->flag |= CHANGED;
                break;
            case 'x':
                cktin[i]->value = U;
                cktin[i]->flag |= CHANGED;
                break;
            default:
                assert(0);
        }
    }
    sim();

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
    int current_backtrack_num = 0;
    wptr wpi;

    int tmp_desired_value = desired_value;
    wpi = find_cool_pi(faulty_w, tmp_desired_value);

    if (wpi != nullptr) {
        wpi->value = obj_value;
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
                    wpi->value = obj_value;
                    wpi->flag |= CHANGED;
                    d_tree.push_front(wpi);
                }
            }
        }
    }
    
    for (int i = 0; i < ncktin; ++i) {
        cktin[i]->flag &= ~MARKED;
        cktin[i]->flag &= ~CHANGED;
        cktin[i]->flag &= ~ALL_ASSIGNED;
    }
    
    if (find_test) {
        for (int i = 0; i < ncktin; ++i) {
            switch (cktin[i]->value) {
                case 0:
                case B:
                    if (assignments[i] != 'x') {
                        assert(assignments[i] == '0');
                    }
                    assignments[i] = '0';
                    break;
                case 1:
                case D:
                    if (assignments[i] != 'x') {
                        assert(assignments[i] == '1');
                    }
                    assignments[i] = '1';
                    break;
                case U:
                    assert(assignments[i] == 'x');
                    assignments[i] = 'x';
                    break;
            }
        }
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
