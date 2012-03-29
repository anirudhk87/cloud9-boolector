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

#ifndef BTORPARSE_H_INCLUDED
#define BTORPARSE_H_INCLUDED

#include "btorexp.h"
#include "btorlogic.h"

#include <stdio.h>

typedef struct BtorParser BtorParser;
typedef struct BtorParseResult BtorParseResult;
typedef struct BtorParserAPI BtorParserAPI;
typedef BtorParser * (*BtorInitParser)(Btor *, int verbosity);
typedef void (*BtorResetParser)(void*);

typedef char * (*BtorParse) (BtorParser *,
                             FILE *, const char *, BtorParseResult *);

enum BtorParseSATStatus
{
  BTOR_PARSE_SAT_STATUS_UNKNOWN,
  BTOR_PARSE_SAT_STATUS_SAT,
  BTOR_PARSE_SAT_STATUS_UNSAT
};

typedef enum BtorParseSATStatus BtorParseSATStatus;

struct BtorParseResult
{
  BtorLogic logic;
  BtorParseSATStatus status;

  int ninputs;
  BtorExp **inputs;

  int noutputs;
  BtorExp **outputs;

  int nregs;
  BtorExp **regs;
  BtorExp **nexts;
};

struct BtorParserAPI
{
  BtorInitParser init;
  BtorResetParser reset;
  BtorParse parse;
};

#endif
