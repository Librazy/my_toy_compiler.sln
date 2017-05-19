%{
	#include "node.h"
	#include <cstdio>
	#include <cstdlib>
	#include <iostream>
	NBlock *programBlock; /* the top level root node of our final AST */

	extern int yylex();
	void yyerror(const char *s) { std::printf("Error: %s\n", s);std::exit(1); }
	extern bool term[2];
%}

/* Represents the many different ways we can access our data */
%union {
	Node *node;
	NBlock *block;
	NExpression *expr;
	NStatement *stmt;
	NIdentifier *ident;
	NVariableDeclaration *decl;
	NVariableDefinition *var_def;
	std::vector<NVariableDefinition*> *varvec;
	std::vector<NExpression*> *exprvec;
	std::string *string;
	std::string *keyword;
	int token;
}

/* Define our terminal symbols (tokens). This should
   match our tokens.l lex file. We also define the node type
   they represent.
 */
%token <string> TIDENTIFIER TINTEGER TDOUBLE
%token <keyword> KIF KTHEN KELSE KRETURN KEXTERN KBFALSE KBTRUE KWHILE KVAR KFN KLKFN
%token <token> TCEQ TCNE TCLT TCLE TCGT TCGE TEQUAL TCL TNOT
%token <token> TLPAREN TRPAREN TLBRACE TRBRACE TCOMMA TDOT
%token <token> TPLUS TMINUS TMUL TDIV TPOW


/* Define the type of node our nonterminal symbols represent.
   The types refer to the %union declaration above. Ex: when
   we call an ident (defined by union type ident) we are really
   calling an (NIdentifier*). It makes the compiler happy.
 */
%type <ident> ident
%type <expr> numeric expr boolean if_block
%type <varvec> func_decl_args
%type <decl> decl
%type <exprvec> call_args
%type <block> program stmts block
%type <stmt> stmt var_def func_decl extern_decl while_block
%type <token> comparison

/* Operator precedence for mathematical operators */
%left TPLUS TMINUS
%left TMUL TDIV
%left TPOW

%start program

%%

program : stmts { programBlock = $1; }
		;
		
stmts : stmt {if(!term[1]){printf("\nNeed new line or semicolon \n");}$$ = new NBlock(); $$->statements.push_back($<stmt>1); }
	  | stmts stmt { if(!term[1]){printf("\nNeed new line or semicolon \n");}$1->statements.push_back($<stmt>2); }
	  ;

stmt : var_def | func_decl | extern_decl
	 | expr { $$ = new NExpressionStatement(*$1); }
	 | KRETURN expr { $$ = new NReturnStatement(*$2); }
	 | while_block
     ;
	
while_block : KWHILE expr block
			{ $$ = new NWhileBlock(*$2, *$3); }
			;

block : TLBRACE stmts TRBRACE { $$ = $2; }
	  | TLBRACE TRBRACE { $$ = new NBlock(); }
	  ;

decl : ident TCL ident  { $$ = new NVariableDeclaration(*$3, *$1); }
	 ;
var_def : KVAR decl { $$ = new NVariableDefinition($2->type, $2->id); delete $2;}
		 | KVAR decl TEQUAL expr { $$ = new NVariableDefinition($2->type, $2->id ,$4); delete $2;}
		 ;

extern_decl : KEXTERN ident ident TLPAREN func_decl_args TRPAREN
                { $$ = new NExternDeclaration(*$2, *$3, *$5); delete $5; }
            ;

func_decl : KFN decl TLPAREN func_decl_args TRPAREN block 
			{ $$ = new NFunctionDeclaration($2->type, $2->id, *$4, *$6, false); delete $4; }
		  | KLKFN decl TLPAREN func_decl_args TRPAREN block 
			{ $$ = new NFunctionDeclaration($2->type, $2->id, *$4, *$6, true); delete $4; }
		  ;
	
func_decl_args : /*blank*/  { $$ = new VariableList(); }
		  | decl { $$ = new VariableList(); $$->push_back(new NVariableDefinition($1->type, $1->id)); delete $1;}
		  | func_decl_args TCOMMA decl { $1->push_back(new NVariableDefinition($3->type, $3->id)); delete $3;}
		  ;
if_block : KIF expr KTHEN expr KELSE expr
		   { $$ = new NIfBlock(*$2,*$4,*$6); }
		 | KIF expr block KELSE block
		   { $$ = new NIfBlock(*$2,*$3,*$5); }
		 ;

boolean : KBTRUE { $$ = new NBool(true);  }
		| KBFALSE { $$ = new NBool(false); }
		;

ident : TIDENTIFIER { $$ = new NIdentifier(*$1); delete $1; }
	  ;

numeric : TINTEGER { $$ = new NInteger(atol($1->c_str())); delete $1; }
		| TDOUBLE { $$ = new NDouble(atof($1->c_str())); delete $1; }
		;

expr : ident TEQUAL expr { $$ = new NAssignment(*$<ident>1, *$3); }
	 | if_block { $$ = $1; }
	 | ident TLPAREN call_args TRPAREN { $$ = new NMethodCall(*$1, *$3); delete $3; }
	 | ident { $<ident>$ = $1; }
	 | boolean
	 | numeric
         | expr TPOW expr { $$ = new NBinaryOperator(*$1, $2, *$3); }
         | expr TMUL expr { $$ = new NBinaryOperator(*$1, $2, *$3); }
         | expr TDIV expr { $$ = new NBinaryOperator(*$1, $2, *$3); }
         | expr TPLUS expr { $$ = new NBinaryOperator(*$1, $2, *$3); }
         | expr TMINUS expr { $$ = new NBinaryOperator(*$1, $2, *$3); }
 	 | expr comparison expr { $$ = new NBinaryOperator(*$1, $2, *$3); }
 	 | TNOT expr { $$ = new NBinaryOperator(*$2, $1, *new NBool(true)); }
     | TLPAREN expr TRPAREN { $$ = $2; }
	 | block { $$ = $1; }
	 ;
	
call_args : /*blank*/  { $$ = new ExpressionList(); }
		  | expr { $$ = new ExpressionList(); $$->push_back($1); }
		  | call_args TCOMMA expr  { $1->push_back($3); }
		  ;

comparison : TCEQ | TCNE | TCLT | TCLE | TCGT | TCGE;

%%
