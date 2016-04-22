%{
#include "node.hpp"
#include "parser.hpp"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <cstring>
extern int yylex();
extern int yylineno;
extern int charno;
extern size_t yyleng;
extern FILE *yyin;
void yyerror(const char *msg){
	std::cout<<yylineno<<":"<<(charno-yyleng)<<": error: "<<msg<<std::endl;
	if(yyin != NULL){
		fclose(yyin);
	}
	exit(1);
}
namespace LIL{
NBlock *programBlock; /* the top level root node of our final AST */

void setNodeLocation(Node *node,YYLTYPE *loc){
	node->firstLine = loc->first_line;
	node->firstColumn = loc->first_line;
	node->lastLine = loc->last_line;
	node->lastColumn = loc->last_column;
}
void setLocation(Node *node,YYLTYPE *loc,YYLTYPE *firstLoc,YYLTYPE *lastLoc){
	loc->first_line = firstLoc->first_line;
	loc->first_column = firstLoc->first_column;
	loc->last_line = lastLoc->last_line;
	loc->last_column = lastLoc->last_column;
	if(node != NULL){
		setNodeLocation(node,loc);
	}
}

void setLocation(Node *node,YYLTYPE *loc){
	loc->first_line = yylineno;
	loc->first_column = charno;
	loc->last_line = yylineno;
	loc->last_column = charno-1;
	if(node != NULL){
		setNodeLocation(node,loc);
	}
}



%}
%error-verbose
%debug

/* Represents the many different ways we can access our data */
%union {
	LIL::Node *node;
	LIL::NBlock *block;
	LIL::Node *expr;
	LIL::NIdentifier *ident;
	LIL::NVariableDeclaration *decl;
	LIL::NVariableList *varvec;
	LIL::NExpressionList *exprvec;
	std::string *lit;
	std::string *keyword;
	int32_t wchar;
	std::wstring *wstr;
	int token;
}



	%token <lit> IDENTIFIER FLOATPOINT INTEGER
	%token <wstr> STRING
	%token <wchar> CHAR
	%token <keyword> KUSING KFN KVAR KLET KRETURN KIF KWHILE KELSE KBTRUE KBFALSE KATTR
	%token <token> OLF ORF OFN OEX OTIL OPCS OANDS OCL ODCL OEQS
	%token <token> LPAREN RPAREN LSQBRACE RSQBRACE LBRACE RBRACE ATTR DOT COMMA
	%token <token> BAND BOR BCEQ BCNE BCLT BCLE BCGT BCGE BPOW BPLUS BMINUS BMUL BDIV BPP BMM

	/* Define the type of node our nonterminal symbols represent.
	   The types refer to the %union declaration above. Ex: when
	   we call an ident (defined by union type ident) we are really
	   calling an (NIdentifier*). It makes the compiler happy.*/

	%type <ident> ident
	%type <expr> numeric expr boolean if_block var_def func_decl while_block
	%type <varvec> func_decl_args
	%type <decl> decl
	%type <exprvec> call_args
	%type <block> program exprs block
	%type <token> comparison

	// Operator precedence for mathematical operators 
	%left OOR
	%left OAND
	%left BCEQ BCNE
	%left BCLT BCGT BCLE BCGE
	%left BPLUS BMINUS
	%left BMUL BDIV
	%left BPOW

	%start program

%%

	program : exprs { programBlock = $1; setLocation(nullptr,&@$);}
			;
		
	exprs :	{ $$ = new NBlock(); setLocation($$,&@$);}
		  | expr { $$ = new NBlock(); $$->expressions.push_back($<expr>1); setLocation($$,&@$,&@1,&@1);}
		  | exprs expr { $1->expressions.push_back($<expr>2); setLocation(nullptr,&@$,&@1,&@2);}
		  ;
	
	while_block : KWHILE expr block
				{ $$ = new NWhileBlock(*$2, *$3); setLocation($$,&@$,&@1,&@3);}
				;

	block : LBRACE exprs RBRACE { $$ = $2; setLocation($$,&@$,&@1,&@3);}
		  | LBRACE RBRACE { $$ = new NBlock(); setLocation($$,&@$,&@1,&@2);}
		  ;

	decl : ident OCL ident  { $$ = new NVariableDeclaration(*$3, *$1); setLocation($$,&@$,&@1,&@3);}
		 ;
	var_def : KVAR decl { $$ = new NVariableDefinition(*$2); setLocation($$,&@$,&@1,&@2);delete $2;}
			 | KVAR decl OEQS expr { $$ = new NVariableDefinition(*$2 ,$4); setLocation($$,&@$,&@1,&@4);delete $2;}
			 ;


	func_decl : KFN ident func_decl_args ORF ident OFN block 
				{ $$ = new NFunctionDeclaration(*$5, *$2, *$3, *$7); setLocation($$,&@$,&@1,&@7);delete $3; }
			  ;
	
	func_decl_args : /*blank*/ { $$ =new NVariableList(VariableList()); setLocation($$,&@$);}
			  | decl { $$ = new NVariableList(VariableList()); $$->list.push_back($1); setLocation($$,&@$,&@1,&@1); delete $1;}
			  | func_decl_args COMMA decl { $1->list.push_back($3); setLocation(nullptr,&@$,&@1,&@3);delete $3;}
			  ;
	if_block : KIF expr block KELSE block
			   { $$ = new NIfBlock(*$2,*$3,*$5);setLocation($$,&@$,&@1,&@5); }
			 ;

	boolean : KBTRUE { $$ = new NBool(true);  setLocation($$,&@$,&@1,&@1);}
			| KBFALSE { $$ = new NBool(false);setLocation($$,&@$,&@1,&@1); }
			;

	ident : IDENTIFIER { $$ = new NIdentifier(*$1); setLocation($$,&@$,&@1,&@1);delete $1; }
		  ;

	numeric : INTEGER { $$ = new NInteger(atol($1->c_str())); setLocation($$,&@$,&@1,&@1);delete $1; }
			| FLOATPOINT { $$ = new NDouble(atof($1->c_str()));setLocation($$,&@$,&@1,&@1); delete $1; }
			;

	expr : var_def { $$ = $1;setLocation(nullptr,&@$,&@1,&@1); } | func_decl { $$ = $1;setLocation(nullptr,&@$,&@1,&@1); } 
		 | KRETURN expr { $$ = new NReturn(*$2);setLocation($$,&@$,&@1,&@2); }
		 | while_block { $$ = $1;setLocation(nullptr,&@$,&@1,&@1); }
		 | ident OEQS expr { $$ = new NAssignment(*$<ident>1, *$3);setLocation($$,&@$,&@1,&@3); }
		 | if_block { $$ = $1;setLocation(nullptr,&@$,&@1,&@1); }
		 | ident LPAREN call_args RPAREN { $$ = new NMethodCall(*$1, *$3); setLocation($$,&@$,&@1,&@4);delete $3; }
		 | ident { $<ident>$ = $1; setLocation(nullptr,&@$,&@1,&@1);}
		 | boolean { $$ = $1;setLocation(nullptr,&@$,&@1,&@1); }
		 | numeric
			 | expr BPOW expr { $$ = new NBinaryOperator(*$1, $2, *$3); setLocation($$,&@$,&@1,&@3);}
			 | expr BMUL expr { $$ = new NBinaryOperator(*$1, $2, *$3); setLocation($$,&@$,&@1,&@3);}
			 | expr BDIV expr { $$ = new NBinaryOperator(*$1, $2, *$3); setLocation($$,&@$,&@1,&@3);}
			 | expr BPLUS expr { $$ = new NBinaryOperator(*$1, $2, *$3); setLocation($$,&@$,&@1,&@3);}
			 | expr BMINUS expr { $$ = new NBinaryOperator(*$1, $2, *$3); setLocation($$,&@$,&@1,&@3);}
 		 | expr comparison expr { $$ = new NBinaryOperator(*$1, $2, *$3); setLocation($$,&@$,&@1,&@3);}
		 | LPAREN expr RPAREN { $$ = $2; setLocation(nullptr,&@$,&@1,&@3);}
		 | block { $$ = $1; setLocation(nullptr,&@$,&@1,&@1);}
		 ;
	
	call_args : /*blank*/  { $$ = new NExpressionList(ExpressionList()); setLocation(nullptr,&@$);}
			  | expr { $$ = new NExpressionList(ExpressionList()); $$->list.push_back($1); setLocation($$,&@$,&@1,&@1);}
			  | call_args COMMA expr  { $1->list.push_back($3); setLocation(nullptr,&@$,&@1,&@3);}
			  ;

	comparison : BCEQ | BCNE | BCLT | BCLE | BCGT | BCGE;

	%%
	}