/**********************************************************************/
/*           automatic test pattern generation                        */
/*           ATPG class header file                                   */
/*                                                                    */
/*           Author: Bing-Chen (Benson) Wu                            */
/*           last update : 01/21/2018                                 */
/**********************************************************************/

#include <algorithm>
#include <string>
#include <vector>
#include <list>
#include <forward_list>
#include <array>
#include <memory>
#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cassert>

#define HASHSIZE 3911

/* types of gate */
#define NOT       1
#define NAND      2
#define AND       3
#define INPUT     4
#define NOR       5
#define OR        6
#define OUTPUT    8
#define XOR      11
#define BUF      17
#define EQV	      0	/* XNOR gate */
#define SCVCC    20

/* possible values for wire flag */
#define SCHEDULED       1
#define ALL_ASSIGNED    2
/*#define INPUT         4*/
/*#define OUTPUT        8*/
#define MARKED         16
#define FAULT_INJECTED 32
#define FAULTY         64
#define CHANGED       128
#define FICTITIOUS    256
#define PSTATE       1024

/* miscellaneous substitutions */
#define MAYBE          2
#define TRUE           1
#define FALSE          0
#define CONFLICT       2 // originally in podem.cpp (by Chen-Hao)
#define REDUNDANT      3
#define STUCK0         0
#define STUCK1         1
#define STR            0
#define STF            1
#define ALL_ONE        0xffffffff // for parallel fault sim; 2 ones represent a logic one
#define ALL_ZERO       0x00000000 // for parallel fault sim; 2 zeros represent a logic zero

/* possible values for fault->faulty_net_type */
#define GI 0
#define GO 1

/* 4-valued logic */
#define U  2
#define D  3
#define B  4

using namespace std;

class ATPG {
public:
  
  ATPG();
  
  /* defined in tpgmain.cpp */
  void set_fsim_only(const bool&);
  void set_tdfsim_only(const bool&);
  void set_tdfatpg_only(const bool&);
  void set_compression(const bool&);
  void set_detection_num(const int&);
  void read_vectors(const string&);
  void set_total_attempt_num(const int&);
  void set_backtrack_limit(const int&);
  
  /* defined in input.cpp */
  void input(const string&);
  void timer(FILE*, const string&);
  
  /* defined in level.cpp */
  void level_circuit(void);
  void rearrange_gate_inputs(void);
  
  /* defined in init_flist.cpp */
  void create_dummy_gate(void);
  void generate_fault_list(void);
  void compute_fault_coverage(void);

  /*defined in tdfsim.cpp*/
  void transition_delay_fault_simulation();

  bool get_tdfsim_only(){ return tdfsim_only; }

  /* defined in test.cpp */
  void test(void);
  
private:

  /* alias declaration */
  class WIRE;
  class NODE;
  class FAULT;
  typedef WIRE*  wptr;                 /* using pointer to access/manipulate the instances of WIRE */
  typedef NODE*  nptr;                 /* using pointer to access/manipulate the instances of NODE */
  typedef FAULT* fptr;                 /* using pointer to access/manipulate the instances of FAULT */
  typedef unique_ptr<WIRE>  wptr_s;    /* using smart pointer to hold/maintain the instances of WIRE */
  typedef unique_ptr<NODE>  nptr_s;    /* using smart pointer to hold/maintain the instances of NODE */
  typedef unique_ptr<FAULT> fptr_s;    /* using smart pointer to hold/maintain the instances of FAULT */

  /* orginally declared in miscell.h */
  forward_list<fptr_s> flist;          /* fault list */
  forward_list<fptr> flist_undetect;   /* undetected fault list */

  /* orginally declared in global.h */
  vector<wptr> sort_wlist;             /* sorted wire list with regard to level */
  vector<wptr> cktin;                  /* input wire list */
  vector<wptr> cktout;                 /* output wire list */
  array<forward_list<wptr_s>,HASHSIZE> hash_wlist;   /* hashed wire list */
  array<forward_list<nptr_s>,HASHSIZE> hash_nlist;   /* hashed node list */
  int in_vector_no;                    /* number of test vectors generated */
  vector<string> vectors;              /* vector set */
  
  /* orginally declared in tpgmain.c */
  int backtrack_limit;
  int total_attempt_num;
  bool fsim_only;                      /* flag to indicate fault simulation only */
  bool tdfsim_only;                    /* flag to indicate tdfault simulation only */
  bool tdfatpg_only;                   /* flag to indicate tdfault atpg only */
  bool compression;                    /* flag to indicate test compression mode on */
  int detection_num;                   /* number of detection */

  /* orginally declared input.c */
  int debug;                           /* != 0 if debugging;  this is a switch of debug mode */
  string filename;                     /* current input file */
  int lineno;                          /* current line number */
  string targv[100];                   /* tokens on current command line */
  int targc;                           /* number of args on current command line */
  int file_no;                         /* number of current file */
  double StartTime, LastTime;

  /* orginally declared tdfatpg.c */
  vector<string> vPatterns; // test patterns by tdfatpg
  
  int hashcode(const string&);
  wptr wfind(const string&);
  nptr nfind(const string&);
  wptr getwire(const string&);
  nptr getnode(const string&);
  void newgate(void);
  void set_output(void);
  void set_input(const bool&);
  void parse_line(const string&);
  void create_structure(void);
  int FindType(const string&);
  void error(const string&);
  void display_circuit(void);
  //void create_structure(void);
  
  /* orginally declared in init_flist.c */
  int num_of_gate_fault;
  
  char itoc(const int&);
  
  /* orginally declared in sim.c */
  void sim(void);
  void evaluate(nptr);
  int ctoi(const char&);
  
  /* orginally declared in faultsim.c */
  unsigned int Mask[16] = {0x00000003, 0x0000000c, 0x00000030, 0x000000c0,
                           0x00000300, 0x00000c00, 0x00003000, 0x0000c000,
                           0x00030000, 0x000c0000, 0x00300000, 0x00c00000,
                           0x03000000, 0x0c000000, 0x30000000, 0xc0000000,};
  unsigned int Unknown[16] = {0x00000001, 0x00000004, 0x00000010, 0x00000040,
                              0x00000100, 0x00000400, 0x00001000, 0x00004000,
                              0x00010000, 0x00040000, 0x00100000, 0x00400000,
                              0x01000000, 0x04000000, 0x10000000, 0x40000000,};
  forward_list<wptr> wlist_faulty;  // faulty wire linked list
  
  void fault_simulate_vectors(int&);
  void fault_sim_a_vector(const string&, int&);
  void fault_sim_evaluate(const wptr);
  wptr get_faulty_wire(const fptr, int&);
  void inject_fault_value(const wptr, const int&, const int&);
  void combine(const wptr, unsigned int&);
  unsigned int PINV(const unsigned int&);
  unsigned int PEXOR(const unsigned int&, const unsigned int&);
  unsigned int PEQUIV(const unsigned int&, const unsigned int&);
  
  /* orginally declared in podem.c */
  int no_of_backtracks;  // current number of backtracks
  bool find_test;        // true when a test pattern is found
  bool no_test;          // true when it is proven that no test exists for this fault
  
  int podem(fptr, int&);
  wptr fault_evaluate(const fptr);
  void forward_imply(const wptr);
  wptr test_possible(const fptr);
  wptr find_pi_assignment(const wptr, const int&);
  wptr find_hardest_control(const nptr);
  wptr find_easiest_control(const nptr);
  nptr find_propagate_gate(const int&);
  bool trace_unknown_path(const wptr);
  bool check_test(void);
  void mark_propagate_tree(const nptr);
  void unmark_propagate_tree(const nptr);
  int set_uniquely_implied_value(const fptr);
  int backward_imply(const wptr, const int&);
  
  /* orginally declared in display.c */
  void display_line(fptr);
  void display_io(void);
  void display_undetect(void);
  void display_fault(fptr);

  /*defined in tdfsim.cpp*/
  void tdfsim_a_vector(const string&, int&);
  void tdf_inject_fault_value(const wptr, const int&, const int&);
  wptr tdf_get_faulty_wire(const fptr, int&);
  
  /*defined in tdfatpg.cpp*/
  void transition_delay_fault_atpg(void);  /* transition delay fault ATPG, test patterns stored in ATPG::vPatterns */
  fptr select_primary_fault(void);         /* select a primary fault for podem */
  fptr select_secondary_fault(void);       /* select a secondary fault for podem_x */
  int tdf_podem_v1(const fptr, int&);      /* generate test vector 1, considering fault/LOS constraints */
  int tdf_podem_v2(const fptr);            /* generate test vector 2, injection/activation/propagation */
  int tdf_podem_x(void);                   /* dynamic test compression by podem-x */
  void static_compression(void);           /* static test compression */

  /* detail declaration of WIRE, NODE, and FAULT classes */
  class WIRE {
  public:
    WIRE();
    
    string name;               /* asciz name of wire */
    vector<nptr> inode;        /* nodes driving this wire */
    vector<nptr> onode;        /* nodes driven by this wire */
    int value;                 /* logic value [0|1|2] of the wire, fault-free sim */
    int flag;                  /* flag word */
    int level;                 /* level of the wire */
    /*short *pi_reach;*/           /* array of no. of paths reachable from each pi, for podem
                                      (this variable is not used in CCL's class) */
    
    int wire_value1;           /* (32 bits) represents fault-free value for this wire. 
                                  the same [00|11|01] replicated by 16 times (for pfedfs) */
    int wire_value2;           /* (32 bits) represents values of this wire 
                                  in the presence of 16 faults. (for pfedfs) */
    
    int fault_flag;            /* indicates the fault-injected bit position, for pfedfs */
    int wlist_index;           /* index into the sorted_wlist array */
    int value_v1;
    bool fixed;
  };
  
  class NODE {
  public:
    NODE();
    
    string name;               /* ascii name of node */
    vector<wptr> iwire;        /* wires driving this node */
    vector<wptr> owire;        /* wires driven by this node */
    int type;                  /* node type */
    int flag;                  /* flag word */
  };
  
  class FAULT {
  public:
    FAULT();
    
    nptr node;                 /* gate under test(NIL if PI/PO fault) */
    short io;                  /* 0 = GI; 1 = GO */
    short index;               /* index for GI fault. it represents the  
			                            associated gate input index number for this GI fault */   
    short fault_type;          /* s-a-1 or s-a-0 or slow-to-rise or slow-to-fall fault */
    short detect;              /* detection flag */
    short activate;            /* activation flag */
    bool test_tried;           /* flag to indicate test is being tried */
    int eqv_fault_num;         /* number of equivalent faults */
    int to_swlist;             /* index to the sort_wlist[] */ 
    int fault_no;              /* fault index */
    int detected_time;
  };
};
