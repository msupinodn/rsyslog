/* expr.c - an expression class.
 * This module contains all code needed to represent expressions. Most
 * importantly, that means code to parse and execute them. Expressions
 * heavily depend on (loadable) functions, so it works in conjunction
 * with the function manager.
 *
 * Module begun 2007-11-30 by Rainer Gerhards
 *
 * Copyright 2007, 2008 Rainer Gerhards and Adiscon GmbH.
 *
 * This file is part of rsyslog.
 *
 * Rsyslog is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Rsyslog is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Rsyslog.  If not, see <http://www.gnu.org/licenses/>.
 *
 * A copy of the GPL can be found in the file "COPYING" in this distribution.
 */

#include "config.h"
#include <stdlib.h>
#include <assert.h>

#include "rsyslog.h"
#include "template.h"
#include "expr.h"

/* static data */
DEFobjStaticHelpers


/* ------------------------------ parser functions ------------------------------ */
/* the following functions implement the parser. They are all static. For
 * simplicity, the function names match their ABNF definition. The ABNF is defined
 * in the doc set. See file expression.html for details. I do *not* reproduce it
 * here in an effort to keep both files in sync.
 * 
 * All functions receive the current expression object as parameter as well as the
 * current tokenizer.
 *
 * rgerhards, 2008-02-19
 */

/* forward definiton - thanks to recursive ABNF, we can not avoid at least one ;) */
static rsRetVal expr(expr_t *pThis, ctok_t *ctok);

#if 0
static rsRetVal
template(expr_t *pThis, ctok_t *ctok)
{
	DEFiRet;

	ISOBJ_TYPE_assert(pThis, expr);
	ISOBJ_TYPE_assert(ctok, ctok);


finalize_it:
	RETiRet;
}
#endif



static rsRetVal
terminal(expr_t *pThis, ctok_t *ctok)
{
	DEFiRet;
	ctok_token_t *pToken;

	ISOBJ_TYPE_assert(pThis, expr);
	ISOBJ_TYPE_assert(ctok, ctok);

	CHKiRet(ctokGetToken(ctok, &pToken));

	switch(pToken->tok) {
		case ctok_SIMPSTR:
			dbgoprint((obj_t*) pThis, "simpstr\n");
			break;
		case ctok_NUMBER:
			dbgoprint((obj_t*) pThis, "number\n");
			break;
		case ctok_FUNCTION:
			dbgoprint((obj_t*) pThis, "function\n");
			break;
		case ctok_MSGVAR:
			dbgoprint((obj_t*) pThis, "MSGVAR\n");
			break;
		case ctok_SYSVAR:
			dbgoprint((obj_t*) pThis, "SYSVAR\n");
			break;
		case ctok_LPAREN:
			dbgoprint((obj_t*) pThis, "expr\n");
			CHKiRet(ctok_tokenDestruct(&pToken)); /* "eat" processed token */
			CHKiRet(expr(pThis, ctok));
			CHKiRet(ctokGetToken(ctok, &pToken)); /* get next one */
			if(pToken->tok != ctok_RPAREN)
				ABORT_FINALIZE(RS_RET_SYNTAX_ERROR);
			CHKiRet(ctok_tokenDestruct(&pToken)); /* "eat" processed token */
			dbgoprint((obj_t*) pThis, "end expr, rparen eaten\n");
			/* fill structure */
			break;
		default:
			dbgoprint((obj_t*) pThis, "invalid token %d\n", pToken->tok);
			ABORT_FINALIZE(RS_RET_SYNTAX_ERROR);
			break;
	}

finalize_it:
	RETiRet;
}

static rsRetVal
factor(expr_t *pThis, ctok_t *ctok)
{
	DEFiRet;
	ctok_token_t *pToken;

	ISOBJ_TYPE_assert(pThis, expr);
	ISOBJ_TYPE_assert(ctok, ctok);

	CHKiRet(ctokGetToken(ctok, &pToken));
	if(pToken->tok == ctok_NOT) {
		/* TODO: fill structure */
		dbgprintf("not\n");
		CHKiRet(ctok_tokenDestruct(&pToken)); /* no longer needed */
		CHKiRet(ctokGetToken(ctok, &pToken)); /* get next one */
	} else {
		/* we could not process the token, so push it back */
		CHKiRet(ctokUngetToken(ctok, pToken));
	}
	CHKiRet(terminal(pThis, ctok));

finalize_it:
	RETiRet;
}


static rsRetVal
term(expr_t *pThis, ctok_t *ctok)
{
	DEFiRet;

	ISOBJ_TYPE_assert(pThis, expr);
	ISOBJ_TYPE_assert(ctok, ctok);

	CHKiRet(factor(pThis, ctok));

finalize_it:
	RETiRet;
}

static rsRetVal
val(expr_t *pThis, ctok_t *ctok)
{
	DEFiRet;
	ctok_token_t *pToken;

	ISOBJ_TYPE_assert(pThis, expr);
	ISOBJ_TYPE_assert(ctok, ctok);

	CHKiRet(ctokGetToken(ctok, &pToken));
	if(pToken->tok == ctok_PLUS || pToken->tok == ctok_MINUS) {
		/* TODO: fill structure */
		dbgprintf("plus/minus\n");
		CHKiRet(ctok_tokenDestruct(&pToken)); /* no longer needed */
		CHKiRet(ctokGetToken(ctok, &pToken)); /* get next one */
	} else {
		/* we could not process the token, so push it back */
		CHKiRet(ctokUngetToken(ctok, pToken));
	}

	CHKiRet(term(pThis, ctok));

finalize_it:
	RETiRet;
}


static rsRetVal
e_cmp(expr_t *pThis, ctok_t *ctok)
{
	DEFiRet;
	ctok_token_t *pToken;

	ISOBJ_TYPE_assert(pThis, expr);
	ISOBJ_TYPE_assert(ctok, ctok);

	CHKiRet(val(pThis, ctok));

 	/* 0*1(cmp_op val) part */
	CHKiRet(ctokGetToken(ctok, &pToken));
	if(ctok_tokenIsCmpOp(pToken)) {
		dbgoprint((obj_t*) pThis, "cmp\n");
		/* fill structure */
		CHKiRet(ctok_tokenDestruct(&pToken)); /* no longer needed */
		CHKiRet(val(pThis, ctok));
	} else {
		/* we could not process the token, so push it back */
		CHKiRet(ctokUngetToken(ctok, pToken));
	}


finalize_it:
	RETiRet;
}


static rsRetVal
e_and(expr_t *pThis, ctok_t *ctok)
{
	DEFiRet;
	ctok_token_t *pToken;

	ISOBJ_TYPE_assert(pThis, expr);
	ISOBJ_TYPE_assert(ctok, ctok);

	CHKiRet(e_cmp(pThis, ctok));

 	/* *("and" e_cmp) part */
	CHKiRet(ctokGetToken(ctok, &pToken));
	while(pToken->tok == ctok_AND) {
		dbgoprint((obj_t*) pThis, "and\n");
		/* fill structure */
		CHKiRet(ctok_tokenDestruct(&pToken)); /* no longer needed */
		CHKiRet(e_cmp(pThis, ctok));
		CHKiRet(ctokGetToken(ctok, &pToken));
	}

	/* unget the token that made us exit the loop - it's obviously not one
	 * we can process.
	 */
	CHKiRet(ctokUngetToken(ctok, pToken)); 

finalize_it:
	RETiRet;
}


static rsRetVal
expr(expr_t *pThis, ctok_t *ctok)
{
	DEFiRet;
	ctok_token_t *pToken;

	ISOBJ_TYPE_assert(pThis, expr);
	ISOBJ_TYPE_assert(ctok, ctok);

	CHKiRet(e_and(pThis, ctok));

 	/* *("or" e_and) part */
	CHKiRet(ctokGetToken(ctok, &pToken));
	while(pToken->tok == ctok_OR) {
		/* fill structure */
		dbgoprint((obj_t*) pThis, "found OR\n");
		CHKiRet(ctok_tokenDestruct(&pToken)); /* no longer needed */
		CHKiRet(e_and(pThis, ctok));
		CHKiRet(ctokGetToken(ctok, &pToken));
	}

	/* unget the token that made us exit the loop - it's obviously not one
	 * we can process.
	 */
	CHKiRet(ctokUngetToken(ctok, pToken)); 

finalize_it:
	RETiRet;
}


/* ------------------------------ end parser functions ------------------------------ */


/* ------------------------------ actual expr object functions ------------------------------ */

/* Standard-Constructor
 */
BEGINobjConstruct(expr) /* be sure to specify the object type also in END macro! */
ENDobjConstruct(expr)


/* ConstructionFinalizer
 * rgerhards, 2008-01-09
 */
rsRetVal exprConstructFinalize(expr_t *pThis)
{
	DEFiRet;

	ISOBJ_TYPE_assert(pThis, expr);

	RETiRet;
}


/* destructor for the expr object */
BEGINobjDestruct(expr) /* be sure to specify the object type also in END and CODESTART macros! */
CODESTARTobjDestruct(expr)
	/* ... then free resources */
ENDobjDestruct(expr)


/* evaluate an expression and store the result. pMsg is optional, but if
 * it is not given, no message-based variables can be accessed. The expression
 * must previously have been created.
 * rgerhards, 2008-02-09 (a rainy tenerife return flight day ;))
 */
rsRetVal
exprEval(expr_t *pThis, msg_t *pMsg)
{
	DEFiRet;
	
	ISOBJ_TYPE_assert(pThis, expr);
	ISOBJ_TYPE_assert(pMsg, msg);

	RETiRet;
}


/* returns the expression result as a string. The string is read-only and valid
 * only as long as the expression exists and is not newly evaluated. If the caller
 * needs to access it for an extended period of time, it must copy it. This is a
 * performance optimization, as most expression results are expected to be used
 * only for a brief period. In such cases, it saves us the need to copy the string.
 * Also, it is assumed that most callers will hold their private expression. If it 
 * is not shared, the caller can count on the string to remain stable as long as it
 * does not reevaluate the expression (via exprEval or other means) or destruct it.
 * rgerhards, 2008-02-09 (a rainy tenerife return flight day ;))
 */
rsRetVal
exprGetStr(expr_t *pThis, rsCStrObj **ppStr)
{
	DEFiRet;
	
	ISOBJ_TYPE_assert(pThis, expr);
	ASSERT(ppStr != NULL);

	RETiRet;
}


/* parse an expression object based on a given tokenizer
 * rgerhards, 2008-02-19
 */
rsRetVal
exprParse(expr_t *pThis, ctok_t *ctok)
{
	DEFiRet;

	ISOBJ_TYPE_assert(pThis, expr);
	ISOBJ_TYPE_assert(ctok, ctok);

	CHKiRet(expr(pThis, ctok));
	dbgoprint((obj_t*) pThis, "successfully parsed/created expression\n");

finalize_it:
	RETiRet;
}


/* Initialize the expr class. Must be called as the very first method
 * before anything else is called inside this class.
 * rgerhards, 2008-02-19
 */
BEGINObjClassInit(expr, 1) /* class, version */
	OBJSetMethodHandler(objMethod_CONSTRUCTION_FINALIZER, exprConstructFinalize);
ENDObjClassInit(expr)

/* vi:set ai:
 */
