/*  Boolector: Satisfiablity Modulo Theories (SMT) solver.
 *  Copyright (C) 2010  Robert Daniel Brummayer, Armin Biere
 *
 *  This file is part of Boolector.
 *
 *  Boolector is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Boolector is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "stdio.h"
#include "btorbtor.h"
#include "btoraig.h"
#include "btoraigvec.h"
#include "btorhash.h"

#include <string.h>
#include <stdio.h>

#define BTOR_HAVE_ISATTY

#ifdef BTOR_HAVE_ISATTY
#include <unistd.h>
#endif 

static void
die (int prefix, const char * fmt, ...)
{
  va_list ap;
  if (prefix)
    fputs ("*** synthebtor: ", stdout);
  va_start (ap, fmt);
  vfprintf (stdout, fmt, ap);
  va_end (ap);
  fputc ('\n', stdout);
  exit (1);
}

int
main (int argc, char ** argv)
{
  int i, j, verbosity, close_input, close_output, binary, merge;
  const char * input_name, * output_name;
  FILE * input_file, * output_file, * file;
  BtorAIG * aig, * tmp, * merged, ** p;
  BtorPtrHashTable * back_annotation;
  const char * parse_error;
  BtorPtrHashBucket *b;
  BtorParseResult model;
  BtorAIGVecMgr * avmgr;
  BtorAIGPtrStack regs;
  BtorAIGPtrStack nexts;
  BtorAIGPtrStack aigs;
  BtorParser * parser;
  BtorAIGMgr * amgr;
  BtorMemMgr * mem;
  BtorAIGVec * av;
  Btor * btor;

  verbosity = 0;
  close_input = 0;
  close_output = 0;
  binary = 0;
  merge = 0;
  input_name = "<stdin>";
  output_name = "<stdout>";
  input_file = stdin;
  output_file = stdout;

  for (i = 1; i < argc; i++)
    {
      if (!strcmp (argv[i], "-h"))
	{
	  printf ("usage: synthebor [-h][-v][-m][<input>[<output>]]\n");
	  exit (0);
	}
      else if (!strcmp (argv[i], "-v"))
	verbosity++;
      else if (!strcmp (argv[i], "-m"))
	merge = 1;
      else if (argv[i][0] == '-')
	die (1, "invalid command line option '%s'", argv[i]);
      else if (close_output)
	die (1, "too many files");
      else if (close_input)
	{
	  if (!strcmp (argv[i], input_name))
	    die (1, "input and output are the same");

	  if (!(file = fopen (argv[i], "w")))
	    die (1, "can not write '%s'", argv[i]);

	  output_file = file;
	  output_name = argv[i];
	  close_output = 1;
	}
      else if (!(file = fopen (argv[i], "r")))
	die (1, "can not read '%s'", argv[i]);
      else
	{
	  input_file = file;
	  input_name = argv[i];
	  close_input = 1;
	}
    }

  btor = btor_new_btor ();
  btor_set_verbosity_btor (btor, verbosity);
  btor_set_rewrite_level_btor (btor, 1);
  parser = btor_btor_parser_api ()->init (btor, verbosity);

  parse_error = btor_btor_parser_api()->parse (parser,
                                             input_file,
					     input_name,
					     &model);
  if (parse_error)
    die (0, parse_error);

  if (!model.noutputs)
    die (1, "no roots in '%s'", input_name);

  if (model.nregs && merge)
    die (1, "can not merge registers");

  mem = btor->mm;
  avmgr = btor->avmgr;
  amgr = btor_get_aig_mgr_aigvec_mgr (avmgr);

  back_annotation = btor_new_ptr_hash_table (mem, 0, 0);

  BTOR_INIT_STACK (regs);
  BTOR_INIT_STACK (nexts);

  for (i = 0; i < model.nregs; i++)
    {
      if (btor_is_array_exp (btor, model.regs[i]))
	die (1, "can not synthesize memories (yet)");

      av = btor_exp_to_aigvec (btor, model.regs[i], back_annotation);
      for (j = 0; j < av->len; j++)
	{
	  aig = btor_copy_aig (amgr, av->aigs[j]);
	  BTOR_PUSH_STACK (mem, regs, aig);
	}
      btor_release_delete_aigvec (avmgr, av);

      av = btor_exp_to_aigvec (btor, model.nexts[i], back_annotation);
      for (j = 0; j < av->len; j++)
	{
	  aig = btor_copy_aig (amgr, av->aigs[j]);
	  BTOR_PUSH_STACK (mem, nexts, aig);
	}
      btor_release_delete_aigvec (avmgr, av);
    }

  BTOR_INIT_STACK (aigs);
  merged = BTOR_AIG_TRUE;

  for (i = 0; i < model.noutputs; i++)
    {
      av = btor_exp_to_aigvec (btor, model.outputs[i], back_annotation);
      for (j = 0; j < av->len; j++)
	{
	  aig = av->aigs[j];

	  if (merge)
	    {
	      tmp = btor_and_aig (amgr, merged, aig);
	      btor_release_aig (amgr, merged);
	      merged = tmp;
	    }
	  else
	    {
	      aig = btor_copy_aig (amgr, aig);
	      BTOR_PUSH_STACK (mem, aigs, aig);
	    }
	}
      btor_release_delete_aigvec (avmgr, av);
    }

  if (merge)
    BTOR_PUSH_STACK (mem, aigs, merged);

#ifdef BTOR_HAVE_ISATTY
  if (close_output || !isatty (1))
    binary = 1;
#endif
  assert (BTOR_COUNT_STACK (regs) == BTOR_COUNT_STACK (nexts));
  btor_dump_aiger (amgr,
                  binary, output_file, 
		  BTOR_COUNT_STACK (aigs), aigs.start,
		  BTOR_COUNT_STACK (regs), regs.start, nexts.start,
		  back_annotation);

  for (p = aigs.start; p < aigs.top; p++)
    btor_release_aig (amgr, *p);
  BTOR_RELEASE_STACK (mem, aigs);

  for (p = regs.start; p < regs.top; p++)
    btor_release_aig (amgr, *p);
  BTOR_RELEASE_STACK (mem, regs);

  for (p = nexts.start; p < nexts.top; p++)
    btor_release_aig (amgr, *p);
  BTOR_RELEASE_STACK (mem, nexts);

  for (b = back_annotation->first; b; b = b->next)
    btor_freestr (mem, b->data.asStr);

  btor_delete_ptr_hash_table (back_annotation);

  btor_btor_parser_api()->reset (parser);
  btor_delete_btor (btor);

  if (close_input)
    fclose (input_file);

  if (close_output)
    fclose (output_file);

  return 0;
}
