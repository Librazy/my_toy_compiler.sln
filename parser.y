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
extern bool term[2];
#define CHK if(!term[1]){printf("\nNeed new line or semicolon \n");}
void yyerror(const char *msg){
	std::cout<<yylineno<<":"<<(charno-yyleng)<<": error: "<<msg<<std::endl;
	if(yyin != nullptr){
		fclose(yyin);
	}
	exit(1);
}
using namespace LIL;
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
	if(node != nullptr){
		setNodeLocation(node,loc);
	}
}

void setLocation(Node *node,YYLTYPE *loc){
	loc->first_line = yylineno;
	loc->first_column = charno;
	loc->last_line = yylineno;
	loc->last_column = charno-1;
	if(node != nullptr){
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
	LIL::NIdentifierList *identvec;
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
	%type <identvec> names
	%type <expr> using expr
	//%type <exprvec> 
	//%type <varvec> 
	//%type <decl> 
	%type <block> exprs program
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
	program : exprs { programBlock = $1;setLocation(programBlock,&@$); }
			;
			
	exprs   : /**/ { $$ = new NBlock();setLocation($$,&@$); }
			| expr { CHK;$$ = new NBlock($<expr>1);setLocation($$,&@$,&@1,&@1); }
	    	| exprs expr { CHK;$1->exprs.push_back($<expr>2);setLocation(nullptr,&@$,&@1,&@2); }
	        ;

	expr    : using ;

	using   : KUSING names { $$ = new NUsing(*$2); delete $2; setLocation($$,&@$,&@1,&@2); };

	names   : ident { $$ = new NIdentifierList($1->ident); delete $1; setLocation($$,&@$,&@1,&@1); }
			| names ODCL ident { $$->idents.push_back($3->ident); delete $3; setLocation($$,&@$,&@1,&@3); }

	ident   : IDENTIFIER { $$ = new NIdentifier(*$1); delete $1; setLocation($$,&@$,&@1,&@1); }
			;


	comparison : BCEQ | BCNE | BCLT | BCLE | BCGT | BCGE;

	%%
