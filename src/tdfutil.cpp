#include "atpg.h"
void ATPG::print_PI_assignments()
{
    int ncktin = cktin.size();
    cerr << "=========================" << endl;
    for (int i = 0; i < ncktin; ++i) {
        cerr << cktin[i]->name << " = " << cktin[i]->value << endl;
    }
    cerr << "=========================" << endl;
}

void ATPG::print_fault_description(fptr fault)
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

string ATPG::extract_all_assigned_flag()
{
    int ncktin = cktin.size();
    string flags;
    flags.resize(ncktin, '0');
    for (int i = 0; i < ncktin; ++i) {
        flags[i] = (cktin[i]->flag & ALL_ASSIGNED) ? '1' : '0';
    }
    return flags;
}

void ATPG::restore_all_assigned_flag(const string flags)
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
string ATPG::extract_vector_v1(const string vec_v2)
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

void ATPG::restore_vector_v1(const string vec_v1)
{
    int ncktin = cktin.size();
    for (int i = 0; i < sort_wlist.size(); i++) {
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
string ATPG::extract_vector_v2(const string accumulated)
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

void ATPG::restore_vector_v2(const string vec_v2)
{
    int ncktin = cktin.size();
    for (int i = 0; i < sort_wlist.size(); i++) {
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
