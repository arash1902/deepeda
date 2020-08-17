static const char * USAGE =
"usage: aigmultopoly [ <option> ... ] [ <aiger> [ <poly> ] ]\n"
"\n"
"where <option> is one of the following\n"
"\n"
"-h | --help             print this command line option summary\n"
"-v | --verbose          increase verbose level\n"
"-b | --backannotate     add 'Lit<aigerlit>' suffix\n"
"\n"
"--singular              produce 'Singular' output instead of 'Mathematica'\n"
"--mathematica11         use version 'Mathematica 11' (default is 10.4)'\n"
"\n"
"--row-wise\n"
"--non-incremental\n"
"--no-xor-rewriting\n"
"--do-not-promote-gates\n"
"--do-not-merge-gates\n"
"--no-common-rewriting\n"
"--no-vanishing-constraints\n"
"--no-internal-fields-polynomials\n"
"--force-partial-product-order\n"
"\n"
"and the input <aiger> is a combinational multiplier in AIGER format.\n"
"The output file is written to <poly> compatible with Mathematica or\n"
"Singular.  If no output file is given messages are printed on '<stderr>'\n"
"otherwise on '<stdout>'.\n"
;

#include "../aiger/aiger.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

typedef struct Slice Slice;
typedef struct Var Var;

struct Var {
  unsigned aiger;
  int input;
  int idx;
  int pp;
  int carry;
  int occs;
  int xor;
  int slice;
  int used;
  int level;
  int distance;
  int order;
  int promoted;
  char * pos;
  char * neg;
};

struct Slice {
  int slice;
  int order;
  int level;
  int pps;
  unsigned * lits;
  unsigned * carries;
  unsigned ncarries;
};

static Var * vars;
static aiger * model;
static Slice * slices;
static unsigned * pps;

static unsigned M, NN, N;
static int a0, al, ainc;
static int b0, bl, binc;
static int s0, sl, sinc;

static FILE * output_file;
static FILE * message_file;

static int backannotate = 0;
static const char * backannotate_option;

static int singular = 0;
static int mathematica = 1;
static int mathematica11 = 0;
static int common_rewriting = 1;
static int vanishing_constraints = 1;
static int internal_field_polynomials = 1;

static int verbose = 0;
static int incremental = 1;
static int row_wise = 0;
static int promote_gates = 1;
static int merge_gates = 1;
static int xor_rewriting = 1;

static int found_all_partial_products;
static int force_partial_product_order;

static void die (const char *fmt, ...) {
  va_list ap;
  if (output_file) fflush (output_file);
  fputs ("*** [aigmultopoly] ", stderr);
  va_start (ap, fmt);
  vfprintf (stderr, fmt, ap);
  va_end (ap);
  fputc ('\n', stderr);
  fflush (stderr);
  exit (1);
}

static void msg (const char *fmt, ...) {
  va_list ap;
  if (output_file) fflush (output_file);
  assert (message_file);
  fputs ("[aigmultopoly] ", message_file);
  va_start (ap, fmt);
  vfprintf (message_file, fmt, ap);
  va_end (ap);
  fputc ('\n', message_file);
  fflush (message_file);
}

static int match_and (unsigned lhs, unsigned rhs0, unsigned rhs1) {
  if (lhs == aiger_false) return 0;
  if (aiger_sign (lhs)) return 0;
  assert (lhs != aiger_true);
  aiger_and * and = aiger_is_and (model, lhs);
  if (!and) return 0;
  if (and->rhs0 == rhs0 && and->rhs1 == rhs1) return 1;
  if (and->rhs0 == rhs1 && and->rhs1 == rhs0) return 1;
  return 0;
}

static unsigned alit (unsigned i) {
  assert (i < N);
  return model->inputs[a0 + i*ainc].lit;
}

static unsigned blit (unsigned i) {
  assert (i < N);
  return model->inputs[b0 + i*binc].lit;
}

static unsigned slit (unsigned i) {
  assert (i < NN);
  return model->outputs[s0 + i*sinc].lit;
}

static Var * var (unsigned lit) {
  assert (lit < 2*M);
  return vars + lit/2;
}

static char LP = '[';	// default left parenthesis for Mathematica
static char RP = ']';	// default right parenthesis for Mathematica
static char * LB = "[";	// default left bracket for Mathematica
static char * RB = "]";	// default right bracket for Mathematica

static void set_input_name (Var * v, const char * prefix, int x) {
  char buffer[80];
  assert (aiger_is_input (model, v->aiger));
  sprintf (buffer, "%s%c%d%c", prefix, LP, x, RP);
  v->pos = strdup (buffer);
  sprintf (buffer, "(1 - %s)", v->pos);
  v->neg = strdup (buffer);
}

static void set_and_gate_name (Var * v) {
  char buffer[80];
  assert (!v->pos);
  assert (aiger_is_and (model, v->aiger));
  sprintf (buffer,
    "%s%sSlice%dLevel%dOrder%dDistance%d",
    v->xor ? "Xor" : "And",
    v->occs == 1 ? "Occ1" : "",
    v->slice, v->level, v->order, v->distance);
  if (backannotate)
    sprintf (buffer + strlen (buffer),
      "Lit%u",
      2*(unsigned)(v-vars));
  v->pos = strdup (buffer);

  assert (!v->neg);
  sprintf (buffer, "(1 - %s)", v->pos);
  v->neg = strdup (buffer);
}

static const char * lit2str (unsigned lit) {
  Var * v = var (lit);
  unsigned sign = aiger_sign (lit);
  assert (!aiger_is_and (model, lit) || v->occs);
  const char * res = sign ? v->neg : v->pos;
  assert (res);
  return res;
}

// We try to syntactically find two input XOR gates, which are quite
// frequent in arithmetic circuits such as multipliers.  There are
// heuristics which try to reduce XORs differently.  This check finds all
// two-level implementations of XOR, XORs and their negations.

static int is_xor (aiger_and * and) {
  unsigned l = and->rhs0, r = and->rhs1;
  if (!aiger_sign (l)) return 0;
  if (!aiger_sign (r)) return 0;
  l = aiger_strip (l);
  r = aiger_strip (r);
  if (l == r || l == aiger_not (r)) return 0;
  aiger_and * land = aiger_is_and (model, l);
  if (!land) return 0;
  aiger_and * rand = aiger_is_and (model, r);
  if (!rand) return 0;
  unsigned ll = land->rhs0, lr = land->rhs1;
  unsigned rl = rand->rhs0, rr = rand->rhs1;
  if (ll == aiger_not (rl)) {
    return lr == aiger_not (rr);
  } else if (ll == aiger_not (rr)) {
    return lr == aiger_not (rl);
  } else return 0;
}

static unsigned order_var (unsigned lit) {
  if (lit == aiger_true || lit == aiger_false) return 0;
  if (aiger_is_input (model, lit)) return 0;
  Var * v = var (lit);
  if (v->occs++) return 0;
  aiger_and * and = aiger_is_and (model, lit);
  if (is_xor (and)) {
    if (verbose) msg ("found XOR AIGER literal %u", aiger_strip (lit));
    v->xor = 1;
  } else assert (!v->xor);
  if (xor_rewriting && v->xor) and = aiger_is_and (model, and->rhs0);
  assert (and);
  unsigned res = order_var (and->rhs1);
  res += order_var (and->rhs0);
  res++;
  return res;
}

static unsigned order_slice (int k) {
  unsigned lit = slit (k);
  unsigned res = order_var (lit);
  msg ("found %u new non-input variables in COI of slice %d", res, k);
  return res;
}

static int ignore_var (Var * v) { return !v->occs; }

static unsigned merged_gates;

static void traverse_column_backward (Slice * S, unsigned lit) {
  if (lit == aiger_false) return;
  if (lit == aiger_true) return;
  lit = aiger_strip (lit);
  Var * v = var (lit);
  if (v->input) return;
  if (v->slice >= 0) return;
  aiger_and * and = aiger_is_and (model, lit);
  if (v->xor && xor_rewriting) and = aiger_is_and (model, and->rhs0);
  assert (and);
  unsigned rhs0 = and->rhs0;
  unsigned rhs1 = and->rhs1;
  traverse_column_backward (S, rhs1);
  traverse_column_backward (S, rhs0);
  if (ignore_var (v)) return;
  Slice * T;
  Var * v0 = var (rhs0);
  Var * v1 = var (rhs1);
  aiger_and * a0 = aiger_is_and (model, rhs0);
  aiger_and * a1 = aiger_is_and (model, rhs1); 
  if (!merge_gates) T = S;
  else if (!a0 && !a1) T = S;
  else if (a0 && v0->slice == S->slice) T = S;
  // else if (a0 && v0->occs > 1) T = S;		// TODO bad!
  else if (a1 && v1->slice == S->slice) T = S;
  // else if (a1 && v1->occs > 1) T = S;		// TODO bad!
  else {
    assert (!a0 || v0->slice < S->slice);
    assert (!a1 || v1->slice < S->slice);
    int slice = a0 ? v0->slice : 0;
    if (a1 && v1->slice > slice) slice = v1->slice;
    assert (slice < S->slice);
    T = slices + slice;
  }
  v->slice = T->slice;
  v->order = T->order++;
  if(T!=S) {
     merged_gates++;
  }
  if (verbose && T != S) {
       msg ("moved back AIGER literal %u from slice %u to slice %u",
      lit, S->slice, T->slice);
  }
}

static unsigned order_columnwise_backward (Slice * S, int k) {
  assert (S == slices + k);
  assert (!S->order);
  unsigned lit = slit (k);
  traverse_column_backward (S, lit);
  msg ("ordered columnwise backward %d AND gates in slice %d",
    S->order, k);
  return S->order;
}

static void used () {
    for (unsigned i = 0; i < M; i++) {
      unsigned lit = 2*i;
      assert (!aiger_sign (lit));
      Var * v = var (lit);
      if (ignore_var (v)) continue;
      aiger_and * and = aiger_is_and (model, lit);
      if (xor_rewriting && v->xor) and = aiger_is_and (model, and->rhs0);
      assert (and);
      unsigned rhs0 = and->rhs0;
      unsigned rhs1 = and->rhs1;
      aiger_and * a0 = aiger_is_and (model, rhs0);
      aiger_and * a1 = aiger_is_and (model, rhs1);
      Var * v0 = a0 ? var (rhs0) : 0;
      Var * v1 = a1 ? var (rhs1) : 0;
      if (v0 && v0->used < v->slice) v0->used = v->slice;
      if (v1 && v1->used < v->slice) v1->used = v->slice;
    }

    for (unsigned i=0; i < NN; i++){
      unsigned output = slit (i);
      Var * out = var (output);
      out->used = i;
    }



}

static void do_promote_gates () {
  unsigned promotion_round = 0, promoted_gates = 0;

  while (promote_gates) {
    promotion_round++;
    unsigned promoted_gates_this_round = 0;

    // First (re)compute the largest slice in which an AND gate is used.
    used ();

    if (verbose) {
      for (unsigned i = 0; i < M; i++) {
	unsigned lit = 2*i;
	Var * v = var (lit);
	if (!v->occs) continue;
	if (v->used < 0) continue;
	if (ignore_var (v)) continue;
	assert (aiger_is_and (model, lit));
	if (v->slice == v->used) continue;
	msg ("literal %u in slice %d used in slice %d",
	  lit, v->slice, v->used);
      }
    }

    // Now promote those AND gates which have children, which are AND gates
    // used later (TODO refine?)

    for (unsigned i = 0; i < M; i++) {
      unsigned lit = 2*i;
      Var * v = var (lit);
      if (!v->occs) continue;
      if (v->occs > 1) continue;
      if (v->used < 0) continue;
      if (v->used == v->slice) continue;
      assert (v->used > v->slice);
      if (ignore_var (v)) continue;
      aiger_and * and = aiger_is_and (model, lit);
      if (xor_rewriting && v->xor) and = aiger_is_and (model, and->rhs0);
      assert (and);
      unsigned rhs0 = and->rhs0;
      unsigned rhs1 = and->rhs1;
      aiger_and * a0 = aiger_is_and (model, rhs0);
      if (!a0) continue;
      aiger_and * a1 = aiger_is_and (model, rhs1);
      if (!a1) continue;
      Var * v0 = var (rhs0);
      assert (v0);
      if (v0->used < v->used) continue;
      assert (v0->occs > 1);
      Var * v1 = var (rhs1);
      assert (v1);
      if (v1->used < v->used) continue;
      assert (v1->occs > 1);
      int slice = v0->used;
      if (v1->used > slice) slice = v1->used;
      assert (slice > v->slice);
      promoted_gates_this_round++;
      Slice * S = slices + v->slice;
      assert (S->order > 0);
      Slice * T = slices + slice;
      v->slice = slice;
      v->order = T->order++;
      v->promoted = 1;
      if (verbose)
	msg ("promoted %s literal %u from slice %d to slice %d",
	  (xor_rewriting && v->xor) ? "XOR" : "AND",
	  lit, S->slice, T->slice);
    }

    if (!promoted_gates_this_round) break;

    msg ("promoted %u literals in promotion round %u",
      promoted_gates_this_round, promotion_round);

    promoted_gates += promoted_gates_this_round;
  }

  msg ("promoted %u gates in total", promoted_gates);
}

static void name_lit (Slice * s, unsigned lit) {
  Var * v = var (lit);
  assert (!ignore_var (v));
  assert (aiger_is_and (model, lit));
  set_and_gate_name (v);
}

static unsigned name_slice (Slice * S, int k) {
  assert (S == slices + k);
  assert (S->slice == k);
  for (unsigned i = 0; i < S->order; i++)
    name_lit (S, S->lits[i]);
  return S->order + 1;
}

// 'hs' indicates if we are printing for helper slice or not

static void print_lit (Slice * S, int k,
                       unsigned lit, unsigned hs, unsigned last) {
  Var * v = var (lit);
  assert (!ignore_var (v));
  assert (v->pos), assert (v->neg);
  aiger_and * and = aiger_is_and (model, lit);
  assert (and);
  if (xor_rewriting && v->xor) {
    unsigned l = and->rhs0;
    unsigned r = and->rhs1;
    assert (aiger_sign (l));
    assert (aiger_sign (r));
    aiger_and * land = aiger_is_and (model, l);
    aiger_and * rand = aiger_is_and (model, r);
    assert (land), assert (rand);
    assert (land->rhs0 == aiger_not (rand->rhs0) ||
	    land->rhs0 == aiger_not (rand->rhs1));
    assert (land->rhs1 == aiger_not (rand->rhs0) ||
	    land->rhs1 == aiger_not (rand->rhs1));
    aiger_and * less_negations =
      aiger_sign (land->rhs0) && aiger_sign (land->rhs1) ? rand : land;
    assert (aiger_sign (less_negations->rhs0) +
	    aiger_sign (less_negations->rhs1) < 2);
    Var * lhs = var(less_negations->rhs0);
    Var * rhs = var(less_negations->rhs1);
    if (!common_rewriting || hs == 1 || ((lhs->xor || lhs->occs!=1) &&
	(rhs->xor || rhs->occs!=1))) {
      fprintf (output_file, "  -%s + %s + %s - 2 * %s * %s,\n",
	v->pos,
	lit2str (less_negations->rhs1),
	lit2str (less_negations->rhs0),
	lit2str (less_negations->rhs1),
	lit2str (less_negations->rhs0));
    } else if (singular) {
        fprintf (output_file,
	  "  -%s + reduce (%s + %s - 2 * %s * %s, HT%d)%s\n",
          v->pos,
	  lit2str (less_negations->rhs1),
	  lit2str (less_negations->rhs0),
	  lit2str (less_negations->rhs1),
	  lit2str (less_negations->rhs0),
	  k,
	  last == lit ? "" : ", ");
    } else {
      fprintf (output_file,
        "  -%s + PolynomialReduce[%s + %s - 2 * %s * %s, "
	"Join[",
        v->pos,
	lit2str (less_negations->rhs1),
	lit2str (less_negations->rhs0),
	lit2str (less_negations->rhs1),
	lit2str (less_negations->rhs0));
      
     if (internal_field_polynomials) {
        if (k > 0 ) fprintf(output_file, "FS[%d], ", k-1);
        fprintf(output_file, "FS[%d], ", k);
      }

      if (vanishing_constraints) {
       fprintf (output_file, "CS[%d], ", k);
      }

     fprintf (output_file, "HS[%d]], MS[%d]", k, k);
     if(mathematica11) fprintf(output_file, ", CoefficientDomain->Rationals");
     fprintf(output_file, "][[2]]%s\n", last == lit ? "" : ", ");
    }
  } else {
    Var * lhs = var(and->rhs1);
    Var * rhs = var(and->rhs0);
    if (!common_rewriting || hs == 1 || ((lhs->xor || lhs->occs!=1) &&
	(rhs->xor || rhs->occs!=1))) {
      fprintf (output_file, "  -%s + %s * %s%s\n",
	v->pos, lit2str (and->rhs1), lit2str (and->rhs0),
	last == lit ? "" : ", ");
    } else if (singular){
      fprintf (output_file, "  -%s + reduce (%s * %s, HT%d)%s\n",
	v->pos, lit2str (and->rhs1), lit2str (and->rhs0), k,
	last == lit ? "" : ", ");
    } else {
       fprintf (output_file,
        "  -%s + PolynomialReduce[%s * %s, Join[",
        v->pos, lit2str (and->rhs1), lit2str (and->rhs0));
    
      if (internal_field_polynomials) {
        if (k > 0 ) fprintf(output_file, "FS[%d], ", k-1);
        fprintf(output_file, "FS[%d], ", k);
      }

      if (vanishing_constraints) {
       fprintf (output_file, "CS[%d], ", k);
      }

      fprintf (output_file,"HS[%d]], MS[%d]", k, k);
      if(mathematica11) fprintf(output_file, ", CoefficientDomain->Rationals");
      fprintf (output_file,"][[2]],\n");
    }
  }
}

static unsigned print_slice (Slice * S, int k) {

  if (common_rewriting) {
    if (mathematica) fprintf (output_file, "HS[%d] = {\n", k);
    else fprintf (output_file, "ideal HS%d =\n", k);
    assert (S == slices + k);
    assert (S->slice == k);
    unsigned last = 0;
    for (unsigned i = 0; i < S->order; i++) {
      Var * v = var (S->lits[i]);
      if (!v->carry && !v->xor && v->occs == 1)
	last = S->lits[i];
    }
    for (unsigned i = 0; i < S->order; i++) {
      Var * v = var (S->lits[i]);
      if (!v->carry && !v->xor && v->occs == 1)
	print_lit (S, k, S->lits[i], 1, last);
    }
    if (mathematica) fprintf (output_file, "};\n");
    else {
      if (last!=0) fprintf (output_file, ";\n");
      else fprintf (output_file, "  0;\n");
    }

    if (singular) {
      fprintf (output_file, "ideal HT%d = \n", k);
      fprintf (output_file, "  ");
     
      if (internal_field_polynomials) {
        if (k > 0) fprintf (output_file, "FS%d, ", k-1);
        fprintf (output_file, "FS%d, ", k);
      }
     
      if (vanishing_constraints) {
        if (k > 0) fprintf (output_file, "CS%d, ", k-1);
        fprintf (output_file, "CS%d, ", k);
      }
      
      if (k == 0) fprintf (output_file, "HS%d", k);
      else fprintf (output_file, "HS%d", k);
      
     fprintf (output_file, "\n;\n");
  
    }
  }

  if (mathematica) fprintf (output_file, "S[%d] = {\n", k);
  else fprintf (output_file, "ideal S%d =\n", k);
  assert (S == slices + k);
  assert (S->slice == k);
  for (unsigned i = 0; i < S->order; i++) {
    Var * v = var (S->lits[i]);
    if (!common_rewriting || v->carry || v->xor || v->occs != 1)
      print_lit (S, k, S->lits[i], 0, 0);
  }

  unsigned output = slit (k);
  Var * out = var (output);
  if (common_rewriting) {
    if (out->xor || out->occs!=1)
      fprintf (output_file, "  -s%c%d%c + %s\n", LP, k, RP, lit2str (output));
    else if (singular) {
      fprintf (output_file,
        "  -s%c%d%c + reduce (%s, HT%d)\n ",
         LP, k, RP, lit2str (output), k);
    }
    else {
      fprintf (output_file,
        "  -s%c%d%c + PolynomialReduce[%s, Join[",
         LP, k, RP, lit2str (output));
      
      if (internal_field_polynomials) {
        if (k > 0 ) fprintf(output_file, "FS[%d], ", k-1);
        fprintf(output_file, "FS[%d], ", k);
      }

      if (vanishing_constraints) {
       fprintf (output_file, "CS[%d], ", k);
      }

      fprintf (output_file,"HS[%d]], MS[%d]", k, k);
      if(mathematica11) fprintf(output_file, ", CoefficientDomain->Rationals");
      fprintf (output_file,"][[2]]\n");
    } 
    if (mathematica) fprintf (output_file, "};\n");
    else fprintf (output_file, ";\n");

  } else if (mathematica) {
    fprintf (output_file, "  -s%c%d%c + %s\n", LP, k, RP, lit2str (output));
    fprintf (output_file, "};\n");
  } else {
    fprintf (output_file, "  -s%c%d%c + %s\n", LP, k, RP, lit2str (output));
    fprintf (output_file, ";\n");
  }

  return S->order + 1;
}

static int lit2level (Slice * S, unsigned lit) {
  if (lit == aiger_true || lit == aiger_false) return 0;
  if (aiger_is_input (model, lit)) return 0;
  assert (aiger_is_and (model, lit));
  Var * v = var (lit);
  if (S->slice == v->slice) return v->level;
  else return 0;
}

static void levelize_lit (Slice * S, unsigned lit) {
  lit = aiger_strip (lit);
  if (lit == aiger_false) return;
  if (aiger_is_input (model, lit)) return;
  Var * v = var (lit);
  if (v->slice != S->slice) return;
  if (v->level > 0) return;
  aiger_and * and = aiger_is_and (model, lit);
  assert (and);
  if (v->xor && xor_rewriting) {
    and = aiger_is_and (model, and->rhs0);
    assert (and);
  }
  unsigned r0 = and->rhs0;
  unsigned r1 = and->rhs1;
  levelize_lit (S, r0);		// redundant?
  levelize_lit (S, r1);		// redundant?
  int level = lit2level (S, r0);
  int tmp = lit2level (S, r1);
  if (tmp > level) level = tmp;
  v->level = level + 1;
}

static void levelize_slice (Slice * S, unsigned k) {
  assert (S == slices + k);
  assert (S->slice == k);
  for (unsigned i = 0; i < S->order; i++) {
    unsigned lit = S->lits[i];
    levelize_lit (S, lit);
    int level = lit2level (S, lit);
    if (level > S->level) S->level = level;
  }
  msg ("maximum level %d in slice %d", S->level, k);
}

// TODO recursive?

static void propagate_distance_upward (int round, unsigned lit) {
  aiger_and * and = aiger_is_and (model, lit);
  if (!and) return;
  Var * parent = var (lit);
  assert (!ignore_var (parent));
  if (parent->xor) and = aiger_is_and (model, and->rhs0);
  unsigned distance = ~0u;
  for (unsigned i = 0; i <= 1; i++) {
    unsigned rhs = i ? and->rhs1 : and->rhs0;
    Var * child = var (rhs);
    if (child->distance < distance) distance = child->distance;
  }
  if (distance) distance--;
  assert (parent->distance <= distance);
  if (distance == parent->distance) return;
  // if (parent->distance && verbose)
    msg ("distance updated of %d in round %d from %d to %d",
      lit, round, parent->distance, distance + 1);
  parent->distance = distance;
}

static void propagate_all_distances_downward (int round) {
  int changed = 1;
  while (changed) {
    changed = 0;
    for (unsigned i = M-1; i; i--) {
      unsigned lit = 2*i;
      aiger_and * and = aiger_is_and (model, lit);
      if (!and) continue;
      Var * parent = var (lit);
      if (parent->xor) and = aiger_is_and (model, and->rhs0);
      for (unsigned i = 0; i <= 1; i++) {
	unsigned rhs = i ? and->rhs1 : and->rhs0;
	Var * child = var (rhs);
	if (child->distance > parent->distance) continue;
	if (round < NN)
	  msg ("distance updated of %d in round %d from %d to %d",
	    lit, round, child->distance, parent->distance + 1);
	child->distance = parent->distance + 1;
	changed = 1;
      }
    }
  }
}

static void collect_lit (unsigned lit) {
  assert (!aiger_sign (lit));
  Var * v = var (lit);
  if (v->slice < 0) return;
  assert (v->slice < NN);
  Slice * S = slices + v->slice;
  assert (v->order < S->order);
  assert (!S->lits[v->order]);
  S->lits[v->order] = lit;
}

static int cmp_lit (const void * p, const void * q) {
  int l0 = * (const int *) p;
  int l1 = * (const int *) q;
  assert (aiger_is_and (model, l0));
  assert (aiger_is_and (model, l1));
  if (aiger_not (l0) == l1) return aiger_sign (l1) ? -1 : 1;
  Var * v0 = var (l0);
  Var * v1 = var (l1);
  assert (v0->slice == v1->slice);
  if (v0->level < v1->level) return -1;
  if (v0->level > v1->level) return 1;
  if (v0->aiger < v1->aiger) return -1;
  assert (v0->aiger > v1->aiger);
  return 1;
}

static void sort_and_reorder_slice (Slice * S) {
  qsort (S->lits, S->order, sizeof *S->lits, cmp_lit);
  for (int i = 0; i < S->order; i++) {
    unsigned lit = S->lits[i];
    assert (!aiger_sign (lit));
    Var * v = var (lit);
    if (verbose && v->order != i)
      msg ("new order %d instead of %d of AIGER literal %u in slice %d",
	i, v->order, lit, (int)(S - slices));
    v->order = i;
  }
}

static void reduce_order (Slice * S, unsigned k) {
  assert (slices + k == S);
  unsigned j = 0;
  for (unsigned i = 0; i < S->order; i++) {
    unsigned lit = S->lits[i];
    if (lit) S->lits[j++] = lit;
  }
  if (j == S->order) return;
  msg ("reducing size of slice %u from %u to %u",
    k, S->order, j);
  S->order = j;
}

static double percent (double a, double b) { return b ? 100.0 * a / b : 0; }
// static double relative (double a, double b) { return b ? a / b : 0; }

static size_t maximum_resident_set_size () {
  struct rusage u;
  if (getrusage (RUSAGE_SELF, &u)) return 0;
  return ((size_t) u.ru_maxrss) << 10;
}

static double process_time () {
  struct rusage u;
  if (getrusage (RUSAGE_SELF, &u)) return 0;
  double res = u.ru_utime.tv_sec + 1e-6 * u.ru_utime.tv_usec;
  res += u.ru_stime.tv_sec + 1e-6 * u.ru_stime.tv_usec;
  return res;
}

static void print_statistics () {
  msg ("");
  msg ("maximum resident set size:  %12.2f MB",
    maximum_resident_set_size () / (double)(1<<20));
  msg ("used process time:          %12.2f seconds",
    process_time ());
}

static const char * signal_name (int sig) {
  switch (sig) {
    case SIGINT: return "SIGINT";
    case SIGSEGV: return "SIGSEGV";
    case SIGABRT: return "SIGABRT";
    case SIGTERM: return "SIGTERM";
    default: return "SIGUNKNOWN";
  }
}

static void (*original_SIGINT_handler)(int);
static void (*original_SIGSEGV_handler)(int);
static void (*original_SIGABRT_handler)(int);
static void (*original_SIGTERM_handler)(int);

void reset_all_signal_handlers () {
  (void) signal (SIGINT, original_SIGINT_handler);
  (void) signal (SIGSEGV, original_SIGSEGV_handler);
  (void) signal (SIGABRT, original_SIGABRT_handler);
  (void) signal (SIGTERM, original_SIGTERM_handler);
}

static void catch_signal (int sig) {
  printf ("c\nc caught signal '%s' (%d)\nc\n", signal_name (sig), sig);
  print_statistics ();
  printf ("c\nc raising signal '%s' (%d) again\n", signal_name (sig), sig);
  reset_all_signal_handlers ();
  fflush (stdout);
  raise (sig);
}

void init_all_signal_handers () {
  original_SIGINT_handler = signal (SIGINT, catch_signal);
  original_SIGSEGV_handler = signal (SIGSEGV, catch_signal);
  original_SIGABRT_handler = signal (SIGABRT, catch_signal);
  original_SIGTERM_handler = signal (SIGTERM, catch_signal);
}

/*------------------------------------------------------------------------*/


int main (int argc, char ** argv) {
  const char * output_name = 0;
  init_all_signal_handers ();
  const char * input_name = 0;
  for (int i = 1; i < argc; i++) {
    if (!strcmp (argv[i], "-h") ||
        !strcmp (argv[i], "--help")) {
      fputs (USAGE, stdout);
      fflush (stdout);
      exit (0);
    } else if (!strcmp (argv[i], "-v") ||
               !strcmp (argv[i], "--verbose")) {
      verbose = 1;
    } else if (!strcmp (argv[i], "-b") ||
               !strcmp (argv[i], "--backannotate")) {
      backannotate_option = argv[i];
      backannotate = 1;
    } else if (!strcmp (argv[i], "--singular")) {
      mathematica = 0;
      singular = 1;
    } else if (!strcmp (argv[i], "--mathematica11")) {
      mathematica11 = 1;
    } else if (!strcmp (argv[i], "--no-common-rewriting")) {
      common_rewriting = 0;
    } else if (!strcmp (argv[i], "--no-vanishing-constraints")) {
      vanishing_constraints = 0;
    } else if (!strcmp (argv[i], "--no-internal-fields-polynomials")) {
      internal_field_polynomials = 0;
    } else if (!strcmp (argv[i], "--row-wise")) {
      row_wise = 1;
    } else if (!strcmp (argv[i], "--non-incremental")) {
      incremental = 0;
    } else if (!strcmp (argv[i], "--do-not-merge-gates")) {
      merge_gates = 0;
    } else if (!strcmp (argv[i], "--do-not-promote-gates")) {
      promote_gates = 0;
    } else if (!strcmp (argv[i], "--force-partial-product-order")) {
      force_partial_product_order = 1;
    } else if (!strcmp (argv[i], "--no-xor-rewriting")) {
      xor_rewriting = 0;
    } else if (argv[i][0] == '-')
      die ("invalid command line option '%s' (try '-h')", argv[i]);
    else if (output_name)
      die ("two many arguments '%s', '%s' and '%s' (try '-h')",
        input_name, output_name, argv[i]);
    else if (input_name) output_name = argv[i];
    else input_name = argv[i];
  }
  message_file = output_name ? stdout : stderr;

  if (singular && mathematica11)
    die ("can not combine '%s' and '%s'",
      "--singular","--mathematica11");

  model = aiger_init ();
  if (input_name) {
    msg ("reading '%s'", input_name);
    const char * err = aiger_open_and_read_from_file (model, input_name);
    if (err) die ("error parsing '%s': %s", input_name, err);
  } else {
    msg ("reading from '<stdin>'");
    const char * err = aiger_read_from_file (model, stdin);
    if (err) die ("error parsing '<stdin>': %s", err);
  }
  if (model->num_latches) die ("can not handle latches");
  if (!model->num_inputs) die ("no inputs");
  if ((model->num_inputs & 1)) die ("odd number of inputs");
  if (!model->num_outputs) die ("no outputs");
  if (model->num_outputs != model->num_inputs)
    die ("expected %u but got %u outputs",
      model->num_inputs, model->num_outputs);
  if (output_name) {
    if (!(output_file = fopen (output_name, "w")))
      die ("can not write output to '%s'", output_name);
  } else output_file = stdout;

  M = model->maxvar + 1;

  for (unsigned i = 0; i < M; i++) {
    aiger_and * and = aiger_is_and (model, 2*i);
    if (!and) continue;
    if (aiger_lit2var (and->rhs0) >= i || aiger_lit2var (and->rhs1) >= i)
      die ("only binary or sorted AIGER models supported");
  }

  msg ("MILOA %u %u %u %u %u",
    model->maxvar,
    model->num_inputs,
    model->num_latches,
    model->num_outputs,
    model->num_ands);

  if (backannotate)
    msg ("backannotating literals ('%s' specified)", backannotate_option);
  else
    msg ("not backannotating literals "
      "(no '-b' nor '--backannotate' option)");

   if (singular) {
    LP = '(';
    RP = ')';
    LB = "";
    RB = "";
    assert (!mathematica);
    msg ("producing Singular output");
  } else {
    msg ("producing Mathematica output");
  }

  // TODO add other option messages.

  NN = model->num_inputs;
  N = NN/2;
  if (match_and (
        model->outputs[0].lit,
        model->inputs[0].lit,
        model->inputs[N].lit)) {
    a0 = 0, al = N-1,   ainc = 1;
    b0 = N, bl = 2*N-1, binc = 1;
    s0 = 0, sl = NN-1,   sinc = 1;
    msg ("assuming ordering as in the AIKO benchmarks");
  } else if (match_and (
        model->outputs[0].lit,
        model->inputs[0].lit,
        model->inputs[1].lit)) {
    a0 = 0, al = 2*N-2, ainc = 2;
    b0 = 1, bl = 2*N-1, binc = 2;
    s0 = 0, sl = NN-1,   sinc = 1;
    msg ("assuming ordering as in BTOR generated benchmarks");
  } else die ("input / output matching failed");
  if (N == 1) {
    msg ("a[0] = input[%d]", a0);
    msg ("b[0] = input[%d]", b0);
    msg ("s[0] = output[%d]", s0);
  } else if (N == 2) {
    msg ("(a[0], a[1]) = (input[%d], input[%d])", a0, al);
    msg ("(b[0], b[1]) = (input[%d], input[%d])", b0, bl);
    msg ("(s[0], ..., s[3]) = (output[%d], ..., output[%d])", s0, sl);
  } else if (N == 3) {
    msg ("(a[0], a[1], a[2]) = (input[%d], input[%d], input[%d])",
      a0, a0 + ainc, al);
    msg ("(b[0], b[1], b[2]) = (input[%d], input[%d], input[%d])",
      b0, b0 + binc, bl);
    msg ("(s[0], ..., s[5]) = (output[%d], ..., output[%d])", s0, sl);
  } else {
    msg ("(a[0], a[1], ..., a[%d]) = (input[%d], input[%d], ..., input[%d])",
      N-1, a0, a0 + ainc, al);
    msg ("(b[0], b[1], ..., b[%d]) = (input[%d], input[%d], ..., input[%d])",
      N-1, b0, b0 + binc, bl);
    msg ("(s[0], ..., s[%d]) = (output[%d], ..., output[%d])",
      NN-1, s0, sl);
  }

  vars = calloc (M, sizeof *vars);
  if (!vars) die ("failed to allocate vars");
  for (unsigned i = 0; i < M; i++) {
    Var * v = vars + i;
    v->aiger = 2*i;
    v->slice = -1;
    v->order = -1;
    v->pp = -1;
    v->level = 0;
    v->distance = 0;
    v->used = -1;
    v->promoted = 0;
  }

  assert (!vars[0].aiger);
  assert (!vars[0].input);
  assert (vars[0].slice < 0);
  assert (vars[0].level == 0);
  assert (vars[0].order < 0);
  vars[0].pos = strdup ("0");
  vars[0].neg = strdup ("1");

  for (unsigned i = 0; i < N; i++) {
    Var * v = var (alit (i));
    set_input_name (v, "a", i);
    v->input = 1;
    v->idx = i;
  }

  for (unsigned i = 0; i < N; i++) {
    Var * v = var (blit (i));
    set_input_name (v, "b", i);
    v->input = 2;
    v->idx = i;
  }

  /*----------------------------------------------------------------------*/

  slices = calloc (NN, sizeof *slices);
  if (!slices) die ("failed to allocate slices");
  for (int i = 0; i < NN; i++)
    slices[i].slice = i;

  /*----------------------------------------------------------------------*/

  pps = calloc (N*N, 1);
  if (!pps) die ("failed to allocated partial products");

  // Find partial products.

  for (unsigned i = 0; i < M; i++) {
    unsigned lit = 2*i;
    Var * v = var (lit);
    if (v->pp >= 0) continue;
    aiger_and * and = aiger_is_and (model, lit);
    if (!and) continue;
    if (aiger_sign (and->rhs0)) continue;
    if (aiger_sign (and->rhs1)) continue;
    if (!aiger_is_input (model, and->rhs0)) continue;
    if (!aiger_is_input (model, and->rhs1)) continue;
    Var * v0 = var (and->rhs0);
    Var * v1 = var (and->rhs1);
    assert (v0->input);
    assert (v1->input);
    if (v0->input + v1->input != 3) continue;
    int slice = v0->idx + v1->idx;
    assert (slice < NN);
    v->pp = slice;
    Slice * S = slices + slice;
    assert (S->pps <= slice);
    S->pps++;
  }

  unsigned pps_found = 0;
  for (unsigned i = 0; i < NN; i++) {
    Slice * S = slices + i;
    int expected = i < N ? i + 1 : NN - i - 1;
    if (S->pps == expected)
      msg ("found all %u partial products for slice %u", S->pps, i);
    else
      msg ("only found %u partial produces for slice %u (expected %u)",
        S->pps, i, expected);
    pps_found += S->pps;
  }
  if (pps_found == N*N) {
    found_all_partial_products = 1;
    msg ("found all %u partial products in total", N*N);
    for (int k = NN-1; k >= 0; k--) {
      for (unsigned i = 0; i < M; i++) {
	unsigned lit = 2*i;
        Var * v = var (lit);
	if (v->pp >= 0) continue;
	aiger_and * and = aiger_is_and (model, lit);
	if (!and) continue;
	Var * v0 = var (and->rhs0);
	Var * v1 = var (and->rhs1);
	if (v0->pp == k || v1->pp == k) v->pp = k;
      }
    }
  } else
    msg ("found only %u partial products in total (%u expected)",
      pps_found, N*N);

  /*----------------------------------------------------------------------*/

  // Then determine and order all AND gate literals (positive and negative)
  // in the cone of influence (COI) of the output bits.  Then count
  // occurrences of gates (sum of positive and negative occurrences).

  unsigned lits = 0;
  for (unsigned i = 0; i < NN; i++)
    lits += order_slice (i);
  msg ("found %u non-input literals in total", lits);

  /*----------------------------------------------------------------------*/

  // Similarly we try to find AND gates which actually correspond to the
  // output of an XOR gate (see discussion above before 'is_xor' for more
  // details of what an XOR gate actually is in this context).   We further
  // mark those AND vars which only occur once.

  unsigned ands = 0, ands1 = 0, xors = 0;
  for (unsigned i = 1; i < M; i++) {
    unsigned lit = aiger_var2lit (i);
    aiger_and * and = aiger_is_and (model, lit);
    if (!and) continue;
    Var * v = vars + i;
    if (!v->occs) continue;
    ands++;
    if (v->occs == 1) ands1++;
    if (v->xor) xors++;
  }
  msg ("found %u AND gates in COI of which %u are XORs %.0f%%",
    ands, xors, percent (xors, ands));
  msg ("where %u AND gates occur only once %.0f%%",
    ands1, percent (ands1, ands));

  /*----------------------------------------------------------------------*/

  if (found_all_partial_products && force_partial_product_order) {

    // If we have found all partial products it is better to generate to
    // allocate gates to slices based on the output cones of partial
    // products.  A partial product 'ai*bj' belongs to slice 'k = i + j'.

    for (unsigned i = 1; i < M; i++) {
      unsigned lit = 2*i;
      Var * v = var (lit);
      if (ignore_var (v)) continue;
      if (v->input) continue;
      assert (aiger_is_and (model, lit));
      int slice = v->pp;
      if (slice < 0) slice = 0;
      assert (0 <= slice), assert (slice < NN);
      Slice * S = slices + slice;
      v->slice = slice;
      v->order = S->order++;
    }

  } else {

    // Traverse gates backward starting from the outputs to allocate gates
    // in columnwise slices in postfix order.   Also move back some gates if
    // necessary to smaller slices, as well as promote some of them upward
    // to higher slices.

    unsigned traversed_gates = 0;
    for (unsigned i = 0; i < NN; i++)
      traversed_gates += order_columnwise_backward (slices + i, i);

    msg ("ordered %u gates in total", traversed_gates);
    msg ("merged %u gates in total", merged_gates);
  }

  if (promote_gates) do_promote_gates ();

  /*----------------------------------------------------------------------*/

  if (found_all_partial_products) {	// check anyway ...

    for (unsigned i = 1; i < M; i++) {
      unsigned lit = 2*i;
      Var * v = var (lit);
      if (ignore_var (v)) continue;
      if (v->input) continue;
      assert (aiger_is_and (model, lit));
      if (v->pp < 0)
	msg (
	  "*** WARNING *** "
	  "AIGER literal %u on slice %d not reached by any partial product",
	  lit, v->slice);
      else if (v->pp != v->slice) {
	msg (
	  "*** WARNING *** AIGER literal %u on slice %d but reached by "
	  "partial product of slice %d",
	  lit, v->slice, v->pp);
        if (v->promoted)
	  msg ( "AIGER literal %u was promoted", lit);
      }      
    }
  }

  /*----------------------------------------------------------------------*/

  for (unsigned i = 0; i < NN; i++) {
    Slice * S = slices + i;
    assert (S->slice == i);
    S->lits = calloc (S->order, sizeof *S->lits);
    S->carries = calloc (S->order, sizeof *S->carries);	// too much ...
  }

  /*----------------------------------------------------------------------*/

  // Collect for each slice the literals.

  for (unsigned i = 0; i < M; i++)
    collect_lit (2*i);

  for (unsigned i = 0; i < NN; i++)
    reduce_order (slices + i, i);

  /*----------------------------------------------------------------------*/

  // Levelize the literals in each slice.  It is necessary to postpone
  // levelization until we have all the literals of one slice (since some
  // might have been moved back in traverse).

  for (unsigned i = 0; i < NN; i++)
    levelize_slice (slices + i, i);

  /*----------------------------------------------------------------------*/

  propagate_all_distances_downward (NN);
  for (int i = NN-1; i >= 0; i--) {		// TODO 'NN - 1'?
    propagate_distance_upward (i, slit (i));
    propagate_all_distances_downward (i);
  }
  unsigned max_distance = 0;
  for (unsigned i = 0; i < M; i++) {
    unsigned distance = var (2*i)->distance;
    if (distance > max_distance) max_distance = distance;
  }
  msg ("maximum output distance %u", max_distance);

  for (unsigned d = 0; d <= max_distance; d++) {
    unsigned count = 0;
    for (unsigned i = 0; i < M; i++) {
      unsigned lit = 2*i;
      Var * v = var (lit);
      if (!aiger_is_and (model, lit) && !v->input) continue;
      if (ignore_var (v) && !v->input) continue;
      if (v->distance == d) count++;
    }
    msg ("found %u literals with output distance %u", count, d);
  }

  /*----------------------------------------------------------------------*/

  // Sort and reorder literals per slice;

  for (unsigned i = 0; i < NN; i++)
    sort_and_reorder_slice (slices + i);

  /*----------------------------------------------------------------------*/

  // Name all literals per sliced;

  unsigned named_and_gates = 0;
  for (unsigned i = 0; i < NN; i++)
    named_and_gates += name_slice (slices + i, i);
  msg ("named %u polynomials in total", named_and_gates);

  /*======================================================================*/
  // Here starts the part that actually prints to the output file         */
  /*======================================================================*/

  if (mathematica)
    fprintf (output_file, "size = %d;\n",N);

  /*----------------------------------------------------------------------*/

  // Here we determine the reduction order.


  if (mathematica)
    fprintf (output_file, "M = {\n");
  else
    fprintf (output_file, "option(notWarnSB);\n"
         "ring R  = 0, (\n");

  if (row_wise) {

    for (unsigned distance = 0; distance <= max_distance; distance++) {

    for (int i = NN-1; i >= 0; i--) {
	unsigned lit = slit (i);
	Var * v = var (lit);
	assert (!ignore_var (v));
	if (v->distance != distance) continue;
	fprintf (output_file, "  s%c%d%c,\n", LP, i, RP);
      }
      for (int i = M-1; i >= 0; i--) {
	unsigned lit = 2*i;
	if (!aiger_is_and (model, lit)) continue;
	Var * v = var (lit);
	if (ignore_var (v)) continue;
	if (v->distance != distance) continue;
	fprintf (output_file, "  %s,\n", lit2str (lit));
      }
    }

  } else {

    for (int i = NN-1; i >= 0; i--) {
      fprintf (output_file, "  s%c%d%c,\n", LP, i, RP);
      Slice * S = slices + i;
      for (int j = S->order-1; j >= 0; j--) {
	unsigned lit = S->lits[j];
	assert (lit);
	fprintf (output_file, "  %s,\n", lit2str (lit));
      }
    }
  }

  // Second input 'b' vector.

  for (int i = N-1; i >= 0; i--)
    fprintf (output_file, "  %s,\n", lit2str (blit (i)));

  // First input 'a' vector.

  for (int i = N-1; i > 0; i--)
    fprintf (output_file, "  %s,\n", lit2str (alit (i)));
  fprintf (output_file, "  %s\n", lit2str (alit (0)));

  if (mathematica)
    fprintf (output_file, "};\n");
  else
    fprintf (output_file, "), lp;\n");

  if (mathematica) {

    // Order for slices - for incremental method.

    for (unsigned k = 0; k < NN; k++) {
      fprintf (output_file, "MS[%u] = {\n", k);
      fprintf (output_file, "  s%c%d%c", LP, k, RP);
      Slice * S = slices + k;
      for (int j = S->order-1; j >= 0; j--) {
	unsigned lit = S->lits[j];
	fprintf (output_file, ",\n  %s", lit2str (lit));
      }
      fprintf (output_file, "\n};\n");
    }
  }

  /*----------------------------------------------------------------------*/

  if (internal_field_polynomials) {

    for (unsigned k = 0; k < NN; k++) {

      if (mathematica) fprintf (output_file, "FS[%u] = {\n", k);
      else fprintf (output_file, "ideal FS%u =\n", k);

      Slice * S = slices + k;
      for (unsigned i = 0; i < S->order; i++) {
	unsigned lit = S->lits[i];
	if (i) fputs (",\n", output_file);
        fprintf (output_file, "  -%s + %s^2",
	  lit2str (lit), lit2str (lit));
      }
      if (S->order) fputc ('\n', output_file);
      else if (singular) fputs ("  0\n", output_file);
      if (mathematica) fprintf (output_file, "};\n");
      else fprintf (output_file, ";\n");
    }
  }

  /*----------------------------------------------------------------------*/

  if (mathematica) fprintf (output_file, "FI = {\n");
  else fprintf (output_file, "ideal FI =\n");
  for (unsigned i = 0; i < N; i++) {
    Var * v = var (alit (i));
    fprintf (output_file, "  -%s + %s^2,\n", v->pos, v->pos);
  }

  for (unsigned i = 0; i < N; i++) {
    Var * v = var (blit (i));
    fprintf (output_file, "  -%s + %s^2", v->pos, v->pos);
    if (i+1 < N) fputc (',', output_file);
    fputc ('\n', output_file);
  }
  if (mathematica) fprintf (output_file, "};\n");
  else fprintf (output_file, ";\n");

  /*----------------------------------------------------------------------*/


  // Collect carries, which are non-input vars used in later slices.
  used (); 
  
  for (unsigned k = 0; k < NN; k++) {
    Slice * S = slices + k;
    for (unsigned l = 0; l < S->order; l++) {
      unsigned lit = S->lits[l];
      aiger_and * and = aiger_is_and (model, lit);
      if (!and) continue;
      Var * v = var (lit);
      if (v -> used != v -> slice) {
        if (verbose ) 
          msg("found carry literal %d in slice %d", v->aiger, v->slice);
        Slice * T = slices + v->slice;
	assert (T->ncarries < T->order);
	T->carries[T->ncarries++] = lit;
	v->carry = 1;

      }  
    }
  }
  
  // Now print the carries

  for (unsigned k = 0; k < NN; k++) {
    Slice * S = slices + k;
    msg ("found %u carries in slice %u", S->ncarries, k);
    if (mathematica) fprintf (output_file, "Ca[%u] = {\n", k);
    else fprintf (output_file, "ideal Ca%u =\n", k);
    for (unsigned i = 0; i < S->ncarries; i++) {
      fprintf (output_file, "  %s", lit2str (S->carries[i]));
      if (i + 1 < S->ncarries) fputc (',', output_file);
      fputc ('\n', output_file);
    }
    if (mathematica) fprintf (output_file, "};\n");
    else {
      if (!S->ncarries) fprintf (output_file, "  0\n");
      fprintf (output_file, ";\n");
    }
  }
   /*----------------------------------------------------------------------*/

  if (vanishing_constraints) {

    // Vanishing constraints among carries/gates from the same level.

    unsigned total_vanishing_constraints = 0;

    for (unsigned k = 0; k < NN; k++) {

      if (mathematica) fprintf (output_file, "CS[%u] = {\n", k);
      else fprintf (output_file, "ideal CS%u =\n", k);

      Slice * S = slices + k;
      unsigned vanishing_constraints_this_slice = 0;

      // First find immediate sub-children relations
      // (upper = lower & ...)
      // (upper = !lower & ...)

      for (unsigned i = 0; i < S->ncarries; i++) {
	unsigned upper = S->carries[i];
	aiger_and * upper_and = aiger_is_and (model, upper);
	if (!upper_and) continue;
	assert (!aiger_sign (upper));
	Var * upper_var = var (upper);
	assert (!ignore_var (upper_var));		// ?
	if (ignore_var (upper_var)) continue;

	for (unsigned j = 0; j < S->ncarries; j++) {
	  unsigned lower = S->carries[j];
	  unsigned not_lower = aiger_not (lower);
	  int found = 0;
	       if (upper_and->rhs0 == lower) found = 1;
	  else if (upper_and->rhs1 == lower) found = 1;
	  else if (upper_and->rhs0 == not_lower) found = -1;
	  else if (upper_and->rhs1 == not_lower) found = -1;
	  else found = 0;
	  if (!found) continue;
	  if (found > 0) lower = not_lower;

	  if (vanishing_constraints_this_slice++)
	    fputs (",\n", output_file);

	  fprintf (output_file,
	    "  %s * %s", upper_var->pos, lit2str (lower));

	  if (verbose)
	    msg ("immediate vanishing constraint %s * %s in slice %d",
	      upper_var->pos, lit2str (lower), k);

	}
      }

      // Second find grand-children relations
      // (upper = (lower & ...) & ...)
      // (upper = (!lower & ...) & ...)

      for (unsigned i = 0; i < S->order; i++) {
	unsigned upper = S->lits[i];
	aiger_and * upper_and = aiger_is_and (model, upper);
	if (!upper_and) continue;
	assert (!aiger_sign (upper));
	Var * upper_var = var (upper);
	assert (!ignore_var (upper_var));		// ?
	if (ignore_var (upper_var)) continue;

	aiger_and * left_and =
	  aiger_sign (upper_and->rhs1) ?
	    0 : aiger_is_and (model, upper_and->rhs1);

	aiger_and * right_and =
	  aiger_sign (upper_and->rhs0) ?
	    0 : aiger_is_and (model, upper_and->rhs0);

	if (!left_and && !right_and) continue;

	for (unsigned j = 0; j < S->order; j++) {
	  unsigned lower = S->lits[j];
	  aiger_and * lower_and = aiger_is_and (model, lower);
	  if (!lower_and) continue;
	  assert (!aiger_sign (lower));
	  Var * lower_var = var (lower);
	  assert (!ignore_var (lower_var));		// ?
	  if (ignore_var (lower_var)) continue;
	  unsigned not_lower = aiger_not (lower);
	  int found;
	       if (left_and && left_and->rhs0 == lower) found = 1;
	  else if (left_and && left_and->rhs1 == lower) found = 1;
	  else if (right_and && right_and->rhs0 == lower) found = 1;
	  else if (right_and && right_and->rhs1 == lower) found = 1;
	  else if (left_and && left_and->rhs0 == not_lower) found = -1;
	  else if (left_and && left_and->rhs1 == not_lower) found = -1;
	  else if (right_and && right_and->rhs0 == not_lower) found = -1;
	  else if (right_and && right_and->rhs1 == not_lower) found = -1;
	  else found = 0;
	  if (!found) continue;
	  if (found > 0) lower = not_lower;

	  if (vanishing_constraints_this_slice++)
	    fputs (",\n", output_file);

	  fprintf (output_file, "  %s * %s",
	    upper_var->pos, lit2str (lower));

	  if (verbose)
	    msg ("indirect vanishing constraint %s * %s in slice %d",
	      upper_var->pos, lit2str (lower), k);
	}
      }
      if (vanishing_constraints_this_slice) fputc ('\n', output_file);
      else if (singular) fputs ("  0\n", output_file);
      if (mathematica) fprintf (output_file, "};\n");
      else fprintf (output_file, ";\n");

      msg ("found %u vanishing constraints in slice %u",
	vanishing_constraints_this_slice, k);

      total_vanishing_constraints += vanishing_constraints_this_slice;
    }
    msg ("found %u vanishing XOR constraints in total",
      total_vanishing_constraints);
  }



  /*----------------------------------------------------------------------*/

 // Print polynomials for AND gates.

  unsigned printed_pols = 0;
  for (unsigned i = 0; i < NN; i++)
    printed_pols += print_slice (slices + i, i);
  msg ("printed %u polynomials in total", printed_pols);

  /*----------------------------------------------------------------------*/

  // Non incremental Word-level spec.

  if (!incremental) {

    if (singular) fprintf (output_file, "poly ");
    fprintf (output_file, "spec =  (\n");
    for (unsigned i = 0; i < N; i++) {
      Var * v = var (alit (i));
      if(mathematica) fprintf (output_file, "    2^%d * %s", i, v->pos);
      else fprintf (output_file, "    number(2)^%d * %s", i, v->pos);
      if (i + 1 < N) fputs (" +", output_file);
      fputc ('\n', output_file);
    }
    fprintf (output_file, "  ) * (\n");
    for (unsigned i = 0; i < N; i++) {
      Var * v = var (blit (i));
      if (mathematica) fprintf (output_file, "   2^%d * %s", i, v->pos);
      else fprintf (output_file, "    number(2)^%d * %s", i, v->pos);
      if (i + 1 < N) fputs (" +", output_file);
      fputc ('\n', output_file);
    }
    fprintf (output_file, "  ) -\n");
    for (unsigned i = 0; i < NN; i++) {
      if (mathematica) fprintf (output_file, " 2^%d * s%c%d%c", i, LP, i, RP);
      else fprintf (output_file, "  number(2)^%d * s%c%d%c", i, LP, i, RP);
      if (i + 1 < NN) fputs (" -", output_file);
      else fputs(";", output_file);
      fputc ('\n', output_file);
    }

  }

  /*----------------------------------------------------------------------*/

  if (mathematica) fprintf (output_file, "slices = Join [\n");
  else fprintf (output_file, "ideal slices =\n");
  fprintf (output_file, "  FI,\n");
  for (int i = 0; i < NN; i++) {
    if (i) fputs (",\n", output_file);
    fprintf (output_file, "  S%s%d%s", LB, i, RB);
    if (vanishing_constraints)
      fprintf (output_file, ", CS%s%d%s", LB, i, RB);
    if (internal_field_polynomials)
      fprintf (output_file, ", FS%s%d%s", LB, i, RB);
  }
  if (mathematica) fprintf (output_file, "\n];\n");
  else fprintf (output_file, "\n;\n");

  if (!incremental) {

    msg ("using total non-incremental word-level spec");

    if (mathematica) {
      fprintf (output_file,
	"Print[AbsoluteTiming[PolynomialReduce[spec, slices, M] "
	"[[2]]]]\n");
      fprintf (output_file, "Print[TimeUsed[]] \n");
    } else {
      fprintf (output_file, "reduce (spec, slices);\n");
    }

  } else {

    msg ("using column wise incremental word-level reasoning");

    if(mathematica) {
      for(int i = 2*N-1; i>=0; i--) {
        fprintf(output_file,"PP[%d] =", i);
        int is_zero = 0;
        for(int j= 0; j<=i; j++){
          if(j<N && i-j<N)  {
            fprintf(output_file," +a[%d]*b[%d]", j,i-j);
            is_zero=1;
          }
        }

        if(is_zero == 0) fprintf(output_file,"0");
        fprintf(output_file,"; \n");
      }

      fprintf (output_file,
        "Car[2 size] = 0; \n"
        "Print[AbsoluteTiming[ \n");

      for(int i = 2*N-1; i>=0; i--){
  	  fprintf(output_file, "Print[Car[%d] =", i);
  	  fprintf(output_file,
	    "PolynomialReduce[s[%d] + 2 * Car[%d] - PP[%d], ", i, i+1, i);
          fprintf(output_file, "Join[FI, ");
          
      if (internal_field_polynomials) {
        if (i > 0 ) fprintf(output_file, "FS[%d], ", i-1);
        fprintf(output_file, "FS[%d], ", i);
      }

      if (vanishing_constraints) {
       if (i > 0 ) fprintf(output_file, "CS[%d], ", i-1);
       fprintf (output_file, "CS[%d], ", i);
      }


      fprintf(output_file, " S[%d]], MS[%d]", i, i);
      if(mathematica11) fprintf(output_file, ", CoefficientDomain->Rationals");
      fprintf(output_file, "][[2]], \" Bit\", %d] \n", i);
     }

      fprintf (output_file, "]] \n");
      fprintf (output_file, "Print[TimeUsed[]] \n");
    }
    else {
      for(int i = 2*N-1; i>=0; i--) {
        fprintf(output_file,"poly PP(%d) =", i);
        int is_zero = 0;
	int one_mon = 0;
        for(int j= 0; j<=i; j++){
	  if (j<N && i-j<N)  {
            if (one_mon == 1) fprintf(output_file, "+");
            fprintf(output_file," a(%d)*b(%d)", j,i-j);
            is_zero=1;
	    one_mon=1;
          }
        }

        if(is_zero == 0) fprintf(output_file,"0");
        fprintf(output_file,"; \n");
      }

      fprintf (output_file, "poly Car(%d) = 0; \n",2*N);

      for(int i = 2*N-1; i >= 0; i--) {
	fprintf(output_file, "ideal T%d = FI, ", i);
	if (internal_field_polynomials) {
	  if (i > 0) fprintf (output_file, "FS%d, ", i-1);
	  fprintf (output_file, "FS%d, ", i);
        }
        if (vanishing_constraints) {
          if (i > 0) fprintf (output_file, "CS%d, ", i-1);
	  fprintf (output_file, "CS%d, ", i);
	}
         fprintf(output_file, "S%d;\n", i);
        
  	fprintf(output_file, "poly Car(%d) = ", i);
  	fprintf(output_file,
	  "reduce (s(%d) + 2 * Car(%d) - PP(%d), T%d);\n ", i, i+1, i, i);
	fprintf(output_file, "Car(%d);\n",i);
      }
    }
  }

  if (singular) fprintf (output_file, "quit;\n");

  /*----------------------------------------------------------------------*/

  if (output_name) fclose (output_file);

  for (unsigned i = 0; i < M; i++) {
    Var * v = vars + i;
    if (v->pos) free (v->pos);
    if (v->neg) free (v->neg);
  }
  free (vars);

  free (pps);

  for (unsigned i = 0; i < NN; i++) {
    if (slices[i].lits) free (slices[i].lits);
    if (slices[i].carries) free (slices[i].carries);
  }
  free (slices);

  aiger_reset (model);

  reset_all_signal_handlers ();
  print_statistics ();

  return 0;
}
