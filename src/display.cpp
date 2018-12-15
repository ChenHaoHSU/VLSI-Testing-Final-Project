#include "atpg.h"

void ATPG::display_line(fptr fault) {
  int i;

  for (i = 0; i < cktin.size(); i++) {
    switch (cktin[i]->value) {
      case 0: fprintf(stdout,"0"); break;
      case 1: fprintf(stdout,"1"); break;
      case U: fprintf(stdout,"x"); break;
      case D: fprintf(stdout,"D"); break;
      case B: fprintf(stdout,"B"); break;
    }
  }
  fprintf(stdout," ");
  for (i = 0 ; i < fault->node->iwire.size(); i++) {
    fprintf(stdout,"%s = ",fault->node->iwire[i]->name.c_str());
    switch (fault->node->iwire[i]->value) {
      case 0: fprintf(stdout,"0"); break;
      case 1: fprintf(stdout,"1"); break;
      case U: fprintf(stdout,"x"); break;
      case D: fprintf(stdout,"D"); break;
      case B: fprintf(stdout,"B"); break;
    }
    fprintf(stdout,"; ");
  }
  fprintf(stdout,"output = ");
  switch (fault->node->owire.front()->value) {
    case 0: fprintf(stdout,"0"); break;
    case 1: fprintf(stdout,"1"); break;
    case U: fprintf(stdout,"x"); break;
    case D: fprintf(stdout,"D"); break;
    case B: fprintf(stdout,"B"); break;
  }
  fprintf(stdout,"\n");
}/* end of display_line */

/*
* print primary input and output lines' values
*/
void ATPG::display_io(void) {
  int i;

  fprintf(stdout,"T\'");
  for (i = 0; i < cktin.size(); i++) {
    switch (cktin[i]->value) {
      case 0: fprintf(stdout,"0"); break;
      case 1: fprintf(stdout,"1"); break;
      case U: fprintf(stdout,"x"); break;
      case D: fprintf(stdout,"1"); break;
      case B: fprintf(stdout,"0"); break;
    }
  }
  fprintf(stdout,"'");

  // fprintf(stdout," ");
  // for (i = 0; i < cktout.size(); i++) {
  //   switch (cktout[i]->value) {
  //     case 0: fprintf(stdout,"0"); break;
  //     case 1: fprintf(stdout,"1"); break;
  //     case U: fprintf(stdout,"x"); break;
  //     case D: fprintf(stdout,"D"); break;
  //     case B: fprintf(stdout,"B"); break;
  //   }
  // }

  fprintf(stdout,"\n");
}/* end of display_io */

void ATPG::display_io_v1(void) {
  int i;

  fprintf(stdout,"D\'");
  for (i = 0; i < cktin.size(); i++) {
    switch (cktin[i]->value_v1) {
      case 0: fprintf(stdout,"0"); break;
      case 1: fprintf(stdout,"1"); break;
      case U: fprintf(stdout,"x"); break;
      case D: fprintf(stdout,"1"); break;
      case B: fprintf(stdout,"0"); break;
    }
  }
  fprintf(stdout,"'");
  fprintf(stdout,"\n");
}

void ATPG::display_undetect(void) {
  int i;
  wptr w;
  string ufile = filename + ".uf";

  ofstream file(ufile, std::ifstream::out | std::ofstream::app); // open the input vectors' file
  if(!file) { // if the ofstream obj does not exist, fail to open the file
    fprintf(stderr,"File %s could not be opened\n",ufile.c_str());
    exit(EXIT_FAILURE);
  }
  for (fptr f: flist_undetect) {
    switch (f->node->type) {
      case INPUT:
        file << "primary input: " << f->node->owire.front()->name << endl;
        break;
      case OUTPUT:
        file << "primary output: " << f->node->iwire.front()->name << endl;
        break;
      default:
        file << "gate: " << f->node->name << " ;";
        if (f->io == GI) {
          file << "input wire name: " << f->node->iwire[f->index]->name << endl;
        }
        else {
          file << "output wire name: " << f->node->owire.front()->name << endl;
        }
        break;
    }
    file << "fault_type = ";
    switch (f->fault_type) {
      // case STUCK0:
      //   file << "s-a-0\n"; break;
      // case STUCK1:
      //   file << "s-a-1\n"; break;
      case STR:
        file << "STR\n"; break;
      case STF:
        file << "STF\n"; break;
    }
    file << "detection flag =";
    switch (f->detect) {
      case FALSE:
        file << " aborted\n";
        break;
      case REDUNDANT:
        file << " redundant\n";
        break;
      case TRUE:
        file << " internal error\n";
        break;
    }
    file << "fault no. = " << f->fault_no <<endl;
    file << endl;
  }
  file.close();
}/* end of display_undetect */


void ATPG::display_fault(fptr f) {
  switch (f->node->type) {
    case INPUT:
      fprintf(stdout,"primary input: %s\n",f->node->owire.front()->name.c_str());
      break;
    case OUTPUT:
      fprintf(stdout,"primary output: %s\n",f->node->iwire.front()->name.c_str());
      break;
    default:
      fprintf(stdout,"gate: %s ;",f->node->name.c_str());
      if (f->io == GI) {
        fprintf(stdout,"input wire name: %s\n",
            f->node->iwire[f->index]->name.c_str());
      }
      else {
        fprintf(stdout,"output wire name: %s\n",
            f->node->owire.front()->name.c_str());
      }
      break;
  }
  fprintf(stdout,"fault_type = ");
  switch (f->fault_type) {
    // case STUCK0:
    //   fprintf(stdout,"s-a-0\n"); break;
    // case STUCK1:
    //   fprintf(stdout,"s-a-1\n"); break;
    case STR:
      fprintf(stdout,"STR\n"); break;
    case STF:
      fprintf(stdout,"STF\n"); break;
  }
  fprintf(stdout,"detection flag =");
  switch (f->detect) {
    case FALSE:
      fprintf(stdout," aborted\n");
      break;
    case REDUNDANT:
      fprintf(stdout," redundant\n");
      break;
    case TRUE:
      fprintf(stdout," internal error\n");
      break;
  }
  fprintf(stdout,"\n");
}/* end of display_fault */

void ATPG::display_test_patterns() const {
  int i, j;
  int ncktin;
  
  ncktin = cktin.size();

  for (i = vectors.size() - 1; i >= 0; --i) {
    const string& vec = vectors[i];
    assert(vec.size() == cktin.size() + 1);
    fprintf(stdout, "T\'");
    for (j = 0; j < ncktin; ++j) {
      fprintf(stdout, "%c", vec[j]);
    }
    fprintf(stdout, " %c\'\n", vec.back());
  }
}


void ATPG::display_sort_wlist() const {
  auto itoc2 = [] (const int i) {
    switch(i) {
      case 0: return '0';
      case 1: return '1';
      case U: return 'x';
      case D: return 'D';
      case B: return 'B';
    }
    return 'G';
  };

  for (const wptr w : sort_wlist) {
    if (w->flag & INPUT)
      fprintf(stderr, "[%2d] i %s %c %c\n", w->wlist_index, w->name.c_str(), itoc2(w->value_v1), itoc2(w->value));
    else if (w->flag & OUTPUT)
      fprintf(stderr, "[%2d] o %s %c %c\n", w->wlist_index, w->name.c_str(), itoc2(w->value_v1), itoc2(w->value));
    else
      fprintf(stderr, "[%2d] - %s %c %c\n", w->wlist_index, w->name.c_str(), itoc2(w->value_v1), itoc2(w->value));
  }
}