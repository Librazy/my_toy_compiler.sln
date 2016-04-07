%{
	#include "node.h"
	#include "parser.hpp"
    #include <cstdio>
    #include <cstdlib>
    #include <string>
    #include <cstring>
	NBlock *programBlock; /* the top level root node of our final AST */

	extern int yylex();
	extern int yylineno;
	extern int charno;
	extern int yyleng;
	extern FILE *yyin;

void yyerror(const char *msg){
	std::cout<<yylineno<<":"<<(charno-yyleng)<<": error: "<<msg<<std::endl;
	if(yyin != NULL){
		fclose(yyin);
	}
	exit(1);
}
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
	Node *node;
	NBlock *block;
	Node *expr;
	NIdentifier *ident;
	NVariableDeclaration *decl;
	std::vector<NVariableDeclaration*> *varvec;
	std::vector<Node*> *exprvec;
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

	program : exprs { programBlock = $1; }
			;
		
	exprs :	{ $$ = new NBlock(); }
		  | expr { $$ = new NBlock(); $$->expressions.push_back($<expr>1); }
		  | exprs expr { $1->expressions.push_back($<expr>2); }
		  ;
	
	while_block : KWHILE expr block
				{ $$ = new NWhileBlock(*$2, *$3); }
				;

	block : LBRACE exprs RBRACE { $$ = $2; }
		  | LBRACE RBRACE { $$ = new NBlock(); }
		  ;

	decl : ident OCL ident  { $$ = new NVariableDeclaration(*$3, *$1); }
		 ;
	var_def : KVAR decl { $$ = new NVariableDefinition(*$2); delete $2;}
			 | KVAR decl OEQS expr { $$ = new NVariableDefinition(*$2 ,$4); delete $2;}
			 ;


	func_decl : KFN ident func_decl_args ORF ident OFN block 
				{ $$ = new NFunctionDeclaration(*$5, *$2, *$3, *$7); delete $3; }
			  ;
	
	func_decl_args : /*blank*/ { $$ = new VariableList(); }
			  | decl { $$ = new VariableList(); $$->push_back($1); delete $1;}
			  | func_decl_args COMMA decl { $1->push_back($3); delete $3;}
			  ;
	if_block : KIF expr block KELSE block
			   { $$ = new NIfBlock(*$2,*$3,*$5); }
			 ;

	boolean : KBTRUE { $$ = new NBool(true);  }
			| KBFALSE { $$ = new NBool(false); }
			;

	ident : IDENTIFIER { $$ = new NIdentifier(*$1); delete $1; }
		  ;

	numeric : INTEGER { $$ = new NInteger(atol($1->c_str())); delete $1; }
			| FLOATPOINT { $$ = new NDouble(atof($1->c_str())); delete $1; }
			;

	expr : var_def { $$ = $1; } | func_decl { $$ = $1; } 
		 | KRETURN expr { $$ = new NReturn(*$2); }
		 | while_block { $$ = $1; }
		 | ident OEQS expr { $$ = new NAssignment(*$<ident>1, *$3); }
		 | if_block { $$ = $1; }
		 | ident LPAREN call_args RPAREN { $$ = new NMethodCall(*$1, *$3); delete $3; }
		 | ident { $<ident>$ = $1; }
		 | boolean { $$ = $1; }
		 | numeric
			 | expr BPOW expr { $$ = new NBinaryOperator(*$1, $2, *$3); }
			 | expr BMUL expr { $$ = new NBinaryOperator(*$1, $2, *$3); }
			 | expr BDIV expr { $$ = new NBinaryOperator(*$1, $2, *$3); }
			 | expr BPLUS expr { $$ = new NBinaryOperator(*$1, $2, *$3); }
			 | expr BMINUS expr { $$ = new NBinaryOperator(*$1, $2, *$3); }
 		 | expr comparison expr { $$ = new NBinaryOperator(*$1, $2, *$3); }
		 | LPAREN expr RPAREN { $$ = $2; }
		 | block { $$ = $1; }
		 ;
	
	call_args : /*blank*/  { $$ = new ExpressionList(); }
			  | expr { $$ = new ExpressionList(); $$->push_back($1); }
			  | call_args COMMA expr  { $1->push_back($3); }
			  ;

	comparison : BCEQ | BCNE | BCLT | BCLE | BCGT | BCGE;

	%%