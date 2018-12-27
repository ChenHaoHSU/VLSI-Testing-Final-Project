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

    return 0;
}

int ATPG::tdf_medop_v2(const fptr fault, int& current_backtracks, LIFO& d_tree, string& assignments)
{

    return 0;
}

int ATPG::tdf_medop_v1(const fptr fault, int& current_backtracks)
{

    return 0;
}

ATPG::wptr ATPG::find_objective(const fptr fault, wptr& obj_wire, int& obj_value)
{

    return (nullptr);
}

ATPG::wptr ATPG::find_cool_pi(const wptr obj_wire, int& obj_value)
{

    return (nullptr);
}
