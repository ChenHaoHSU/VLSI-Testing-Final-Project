#include "atpg.h"
void ATPG::print_PI_assignments() const
{
    int ncktin = cktin.size();
    cerr << "=========================" << endl;
    for (int i = 0; i < ncktin; ++i) {
        cerr << cktin[i]->name << " = " << cktin[i]->value << endl;
    }
    cerr << "=========================" << endl;
}

void ATPG::print_fault_description(fptr fault) const
{
    string type = (fault->fault_type == STR) ? "STR" : "STF";
    string node = fault->node->name;
    string io   = (fault->io == GI) ? "GI" : "GO";
    string wire = sort_wlist[fault->to_swlist]->name;
    cerr << "=========================" << endl;
    cerr << "=   fault information   =" << endl;
    cerr << "=-----------------------=" << endl;
    cerr << "= [No. " << fault->fault_no << "] node " << node << endl;
    cerr << "= " << io << " " << type << " at " << wire << endl;
    cerr << "=========================" << endl << endl;
}

void ATPG::initialize_fault_primary_record()
{
    int ncktin = cktin.size();
    LIFO empty_tree;
    string empty_flag, empty_vector;
    empty_flag.resize(ncktin, '0');
    empty_vector.resize(ncktin + 1, '2');
    for (fptr fptr_ele: flist_undetect) {
        fptr_ele->primary_d_tree = empty_tree;
        fptr_ele->primary_allassigned = empty_flag;
        fptr_ele->primary_vector = empty_vector;
    }
    return;
}

/* handle with PI ALL_ASSIGNED flag*/
void ATPG::initialize_all_assigned_flag()
{
    int ncktin = cktin.size();
    for (int i = 0; i < ncktin; ++i) {
        cktin[i]->flag &= ~ALL_ASSIGNED;
    }
    return;
}

string ATPG::extract_all_assigned_flag() const
{
    int ncktin = cktin.size();
    string flags;
    flags.resize(ncktin, '0');
    for (int i = 0; i < ncktin; ++i) {
        flags[i] = (cktin[i]->flag & ALL_ASSIGNED) ? '1' : '0';
    }
    return flags;
}

void ATPG::restore_all_assigned_flag(const string& flags)
{
    int ncktin = cktin.size();
    for (int i = 0; i < ncktin; ++i) {
        assert(flags[i] == '1' || flags[i] == '0');
        if (flags[i] == '1') {
            cktin[i]->flag |= ALL_ASSIGNED;
        } else {
            cktin[i]->flag &= ~ALL_ASSIGNED;
        }
    }
    return;
}

/* These functions may be optimized since now is not fully event-driven sim !!! */
void ATPG::initialize_vector()
{
    int ncktin = cktin.size();
    for (int i = 0; i < ncktin; ++i) {
        cktin[i]->value = U;
        cktin[i]->flag |= CHANGED;
    }
    sim();
    return;
}

/* extract_vector_v1 */
/* extract a string from the circuit as v1 pattern */
string ATPG::extract_vector_v1(const string& vec_v2) const
{
    int ncktin = cktin.size();
    string vec_v1;
    vec_v1.resize(ncktin + 1, '2');
    vec_v1[ncktin] = vec_v2[ncktin];
    for (int i = 0; i < ncktin; ++i) {
        switch (cktin[i]->value) {
            case 0:
            case B:
                vec_v1[i] = '0';
                break;
            case 1:
            case D:
                vec_v1[i] = '1';
                break;
            case U:
                vec_v1[i] = '2';
                break;
        }
    }
    return vec_v1;
}

void ATPG::restore_vector_v1(const string& vec_v1)
{
    int ncktin = cktin.size();
    int nckt = sort_wlist.size();
    for (int i = 0; i < nckt; i++) {
        sort_wlist[i]->value = U;
    }
    for (int i = 0; i < ncktin; i++) {
        switch(vec_v1[i]) {
            case '0':
                cktin[i]->value = 0;
                cktin[i]->flag |= CHANGED;
                break;
            case '1':
                cktin[i]->value = 1;
                cktin[i]->flag |= CHANGED;
                break;
            case '2':
                cktin[i]->value = U;
                cktin[i]->flag |= CHANGED;
                break;
            default:
                assert(0);
        }
    }
    sim();
}

/* extract_vector_v2 */
/* extract a string from the circuit as v2 pattern */
string ATPG::extract_vector_v2(const string& accumulated) const
{
    int ncktin = cktin.size();
    string vec_v2;
    vec_v2.resize(ncktin + 1, '2');
    vec_v2[ncktin-1] = accumulated[ncktin-1];
    for (int i = 0; i < ncktin; i++) {
        int vec_index = (i == 0) ? ncktin : (i-1);
        switch (cktin[i]->value) {
            case 0:
            case B:
                vec_v2[vec_index] = '0';
                break;
            case 1:
            case D:
                vec_v2[vec_index] = '1';
                break;
            case U:
                vec_v2[vec_index] = '2';
                break;
        }
    }
    return vec_v2;
}

void ATPG::restore_vector_v2(const string& vec_v2)
{
    int ncktin = cktin.size();
    int nckt = sort_wlist.size();
    for (int i = 0; i < nckt; i++) {
        sort_wlist[i]->value = U;
    }
    for (int i = 0; i < ncktin; i++) {
        int vec_index = (i == 0) ? ncktin : (i-1);
        switch(vec_v2[vec_index]) {
            case '0':
                cktin[i]->value = 0;
                cktin[i]->flag |= CHANGED;
                break;
            case '1':
                cktin[i]->value = 1;
                cktin[i]->flag |= CHANGED;
                break;
            case '2':
                cktin[i]->value = U;
                cktin[i]->flag |= CHANGED;
                break;
            default:
                assert(0);
        }
    }
    sim();
}

bool ATPG::tdf_hard_constraint_v1(const fptr fault)
{
    int nckt = sort_wlist.size();
    for (int i = 0; i < nckt; i++) {
        sort_wlist[i]->value = U;
    }
    
    wptr w;
    if (fault->io) {
        w = fault->node->owire.front();
    } else {
        w = fault->node->iwire[fault->index];
    }
    
    int if_initial_objective_reached = backward_imply(w, fault->fault_type);
    
    string must_vec;
    must_vec.resize(cktin.size()+1, '2');
    must_vec = extract_vector_v1(must_vec);
    fault->primary_vector = must_vec;

    if (if_initial_objective_reached == CONFLICT) {
        return false;
    } else { 
        return true; 
    }
}

bool ATPG::tdf_check_sticky_constraint_v1(const fptr fault, const string& vec_2, vector<wptr> fanin_cone_wlist)
{
    int ncktin = cktin.size();
    int nckt = sort_wlist.size();
    /* success the value from v2 candidate pattern */
    for (int i = 0; i < ncktin-1; i++) {
        switch(cktin[i+1]->value) {
            case 0:
            case B:
                cktin[i]->value_v1 = 0;
                break;
            case 1:
            case D:
                cktin[i]->value_v1 = 1;
                break;
            case U:
                cktin[i]->value_v1 = U;
                break;
        }
        cktin[i]->flag |= CHANGED2;
    }
    if (cktin[ncktin-1]->value_v1 != ctoi(vec_2[ncktin-1])) {
        cktin[ncktin-1]->value_v1 = ctoi(vec_2[ncktin-1]);
        cktin[ncktin-1]->flag |= CHANGED2;
    }

    /* partial sim */
    auto partial_sim = [&] (vector<wptr> &wlist) -> void {
        for (int i = 0, nwire = wlist.size(); i < nwire; ++i) {
            if (wlist[i]->flag & INPUT) continue;
            evaluate_v1c2(wlist[i]->inode.front());
        }
        return;
    };
    partial_sim(fanin_cone_wlist);
    
    /* check */
    for (int i = 0, nwire = fanin_cone_wlist.size(); i < nwire; ++i) {
        int stick_value = fanin_cone_wlist[i]->stick_v1;
        int current_value = fanin_cone_wlist[i]->value_v1;
        if (((current_value == 0) && (stick_value == 1)) || ((current_value == 1) && (stick_value == 0))) {
            //cerr << fanin_cone_wlist[i]->name << " must be " << fanin_cone_wlist[i]->stick_v1 << " but " << fanin_cone_wlist[i]->value_v1 << endl;
            return false;
        }
    }
    return true;
}

bool ATPG::tdf_set_sticky_constraint_v1(const fptr fault)
{
    int nckt = sort_wlist.size();
    for (int i = 0; i < nckt; i++) {
        sort_wlist[i]->stick_v1 = U;
    }
    
    wptr w;
    if (fault->io) {
        w = fault->node->owire.front();
    } else {
        w = fault->node->iwire[fault->index];
    }
    
    int if_initial_objective_reached = sticky_backward_imply(w, fault->fault_type);

    if (if_initial_objective_reached == CONFLICT) {
        return false;
    } else { return true; }
}

/* sticky_backward_imply */
/* stick the v1 constraint in the circuit to accelerate v2 generation */
bool ATPG::sticky_backward_imply(const wptr w, const int& desired_logic_value) {
    int nin = w->inode.front()->iwire.size();
    bool tf = true;

    if (w->stick_v1 != U &&  
        w->stick_v1 != desired_logic_value) { 
        return false;
    }
    w->stick_v1 = desired_logic_value;

    if (!(w->flag & INPUT)) {
        switch (w->inode.front()->type) {

        case NOT:
            tf = sticky_backward_imply(w->inode.front()->iwire.front(), (desired_logic_value ^ 1));
            if (!tf) return false;
            break;
        case NAND:
            if (desired_logic_value == 0) {
                for (int i = 0; i < nin; i++) {
                    tf = sticky_backward_imply(w->inode.front()->iwire[i], 1);
                    if (!tf) return false;
                }
            }
            break;
        case AND:
            if (desired_logic_value == 1) {
                for (int i = 0; i < nin; i++) {
                    tf = sticky_backward_imply(w->inode.front()->iwire[i], 1);
                    if (!tf) return false;
                }
            }
            break;
        case OR:
            if (desired_logic_value == 0) {
                for (int i = 0; i < nin; i++) {
                    tf = sticky_backward_imply(w->inode.front()->iwire[i], 0);
                    if (!tf) return false;
                }
            }
            break;
        case NOR:
            if (desired_logic_value == 1) {
                for (int i = 0; i < nin; i++) {
                    tf = sticky_backward_imply(w->inode.front()->iwire[i], 0);
                    if (!tf) return false;
                }
            }
            break;
        case BUF:
            tf = sticky_backward_imply(w->inode.front()->iwire.front(),desired_logic_value);
            if (!tf) return false;
            break;
        }
    }
    return tf;
}

bool ATPG::pattern_has_enough_x(const string& pattern) const
{
    int required_x_bit = (int)ceil(log2((double)detection_num));
    int x_bit_count = x_count(pattern);

    // for (size_t i = 0; i < pattern.size(); ++i) {
    //     if (pattern[i] == '2') {
    //         ++x_bit_count;
    //     }
    // }

    return (x_bit_count > required_x_bit);
}

void ATPG::expand_pattern(vector<string>& expanded_patterns, const string& pattern)
{
    int x_bit_count = x_count(pattern);

    // for (size_t i = 0; i < pattern.size(); ++i) {
    //     if (pattern[i] == '2') {
    //         ++x_bit_count;
    //     }
    // }

    size_t pos = pattern.find_first_of('2');
    if (x_bit_count <= 3 && pos != string::npos) {
        expand_pattern_rec(expanded_patterns, pattern, '0', pos);
        expand_pattern_rec(expanded_patterns, pattern, '1', pos);
    } else if (x_bit_count > 3 && pos != string::npos) {
        expand_pattern_rec_limited(expanded_patterns, pattern, '0', pos, 0);
        expand_pattern_rec_limited(expanded_patterns, pattern, '1', pos, 0);
    } else {
        expanded_patterns.push_back(pattern);
    }
}

void ATPG::expand_pattern_rec(vector<string>& patterns, string pattern, char bit, size_t pos)
{
    pattern[pos] = bit;
    pos = pattern.find_first_of('2', pos+1);
    if (pos != string::npos) {
        expand_pattern_rec(patterns, pattern, '0', pos);
        expand_pattern_rec(patterns, pattern, '1', pos);
    } else {
        patterns.push_back(pattern);
    }
}

void ATPG::expand_pattern_rec_limited(vector<string>& patterns, string pattern, char bit, size_t pos, size_t depth)
{
    ++depth;
    pattern[pos] = bit;
    pos = pattern.find_first_of('2', pos+1);
    if (pos != string::npos && depth <= 2) {
        expand_pattern_rec_limited(patterns, pattern, '0', pos, depth);
        expand_pattern_rec_limited(patterns, pattern, '1', pos, depth);
    } else {
        patterns.push_back(pattern);
    }
}

int ATPG::x_count(const string& pattern) const
{
    int ret = 0;
    for (size_t i = 0; i < pattern.size(); ++i) {
        if (pattern[i] == '2') {
            ++ret;
        }
    }
    return ret;
}

int ATPG::detected_fault_num() {
    generate_fault_list();
    int total_num = 0;
    int detect_num;
    int drop_num;
    for (const string& vec : vectors) {
        detect_num = 0;
        drop_num = 0;
        tdfsim_a_vector(vec, detect_num, drop_num);
        total_num += drop_num;
    }
    return total_num;
}
