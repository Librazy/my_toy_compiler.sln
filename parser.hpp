/* A Bison parser, made by GNU Bison 2.7.  */

/* Bison interface for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2012 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

#ifndef YY_YY_PARSER_HPP_INCLUDED
# define YY_YY_PARSER_HPP_INCLUDED
/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     IDENTIFIER = 258,
     FLOATPOINT = 259,
     INTEGER = 260,
     STRING = 261,
     CHAR = 262,
     KUSING = 263,
     KFN = 264,
     KVAR = 265,
     KLET = 266,
     KRETURN = 267,
     KIF = 268,
     KWHILE = 269,
     KELSE = 270,
     KBTRUE = 271,
     KBFALSE = 272,
     KATTR = 273,
     OLF = 274,
     ORF = 275,
     OFN = 276,
     OEX = 277,
     OTIL = 278,
     OPCS = 279,
     OANDS = 280,
     OCL = 281,
     ODCL = 282,
     OEQS = 283,
     LPAREN = 284,
     RPAREN = 285,
     LSQBRACE = 286,
     RSQBRACE = 287,
     LBRACE = 288,
     RBRACE = 289,
     ATTR = 290,
     DOT = 291,
     COMMA = 292,
     BAND = 293,
     BOR = 294,
     BCEQ = 295,
     BCNE = 296,
     BCLT = 297,
     BCLE = 298,
     BCGT = 299,
     BCGE = 300,
     BPOW = 301,
     BPLUS = 302,
     BMINUS = 303,
     BMUL = 304,
     BDIV = 305,
     BPP = 306,
     BMM = 307,
     OOR = 308,
     OAND = 309
   };
#endif


#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{
/* Line 2058 of yacc.c  */
#line 58 "parser.y"

	LIL::Node *node;
	LIL::NBlock *block;
	LIL::Node *expr;
	LIL::NIdentifier *ident;
	LIL::NIdentifierList *identvec;
	std::string *lit;
	std::string *keyword;
	int32_t wchar;
	std::wstring *wstr;
	int token;


/* Line 2058 of yacc.c  */
#line 125 "parser.hpp"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
} YYLTYPE;
# define yyltype YYLTYPE /* obsolescent; will be withdrawn */
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;
extern YYLTYPE yylloc;
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */

#endif /* !YY_YY_PARSER_HPP_INCLUDED  */
