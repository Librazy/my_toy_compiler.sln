%{
#include <string>
#include "node.h"
#include "parser.hpp"

//Some code copied from https://github.com/elite-lang/RedApple


#define SAVE_TOKEN	   yylval.lit = new std::string(yytext, yyleng)
#define SAVE_LOC	   saveLocation()
#define TOKEN(t)	   (yylval.token = t)
#define STATUS_BACK    if(status == STRINGSTATUS){BEGIN STRINGSTATUS;}else{BEGIN CHARSTATUS;}

int charno = 1;
int status;
std::wstring strBuf;
extern YYLTYPE yylloc;
extern void yyerror(const char*);
void saveLocation(){
	yylloc.first_line = yylineno;
	yylloc.first_column = charno;
	yylloc.last_line = yylineno;
	charno += yyleng;
	yylloc.last_column = charno-1;
}
int32_t getUnicode(){
	return strtol(yytext+1,NULL,16);
}

int32_t getWchar(){
	if(strBuf.size() == 0){
		yyerror("empty character constant");
	}else if(strBuf.size() > 1){
		yyerror("multi-character in char syntax");
	}
	return strBuf[0];
}

%}

%option noyywrap
%option yylineno

%x CMNT ESCAPE CHARSTATUS STRINGSTATUS
%%
[\t ]*	                {SAVE_LOC;}
\n	                    {charno = 1;}
\/\/.*\n	            {charno = 1;}
"/*"	                {SAVE_LOC; BEGIN CMNT;}
<CMNT>.	                {SAVE_LOC;}
<CMNT>\n	            {charno = 1;}
<CMNT>"*/"	            {SAVE_LOC; BEGIN INITIAL;}

'	                    {SAVE_LOC; BEGIN CHARSTATUS; strBuf.clear();}
<CHARSTATUS>\\	        {SAVE_LOC; status = CHARSTATUS; BEGIN ESCAPE;}
<CHARSTATUS>\n	        {yyerror("missing terminating ' character");}
<CHARSTATUS>'	        {SAVE_LOC; BEGIN INITIAL; yylval.wchar = getWchar(); return CHAR;}
<CHARSTATUS>.	        {SAVE_LOC; strBuf.push_back(yytext[0]);}

\"	                    {SAVE_LOC; BEGIN STRINGSTATUS; strBuf.clear();}
<STRINGSTATUS>\\	    {SAVE_LOC; status = STRINGSTATUS; BEGIN ESCAPE;}
<STRINGSTATUS>\n	    {yyerror("missing terminating \" character");}
<STRINGSTATUS>\"	    {SAVE_LOC; BEGIN INITIAL; yylval.wstr = new std::wstring(strBuf); return STRING;}
<STRINGSTATUS>.	        {SAVE_LOC; strBuf.push_back(yytext[0]);}

<ESCAPE>n	            {SAVE_LOC; STATUS_BACK; strBuf.push_back('\n');}
<ESCAPE>t	            {SAVE_LOC; STATUS_BACK; strBuf.push_back('\t');}
<ESCAPE>b	            {SAVE_LOC; STATUS_BACK; strBuf.push_back('\b');}
<ESCAPE>r	            {SAVE_LOC; STATUS_BACK; strBuf.push_back('\r');}
<ESCAPE>f	            {SAVE_LOC; STATUS_BACK; strBuf.push_back('\f');}
<ESCAPE>'	            {SAVE_LOC; STATUS_BACK; strBuf.push_back('\'');}
<ESCAPE>\"	            {SAVE_LOC; STATUS_BACK; strBuf.push_back('"');}
<ESCAPE>\\	            {SAVE_LOC; STATUS_BACK; strBuf.push_back('\\');}
<ESCAPE>u[0-9a-fA-F]{4} {SAVE_LOC; STATUS_BACK; strBuf.push_back(getUnicode());}
<ESCAPE>.	            {yyerror("unknown escape sequence");}


"<-"                            SAVE_LOC;return TOKEN(OLF);
"->"                            SAVE_LOC;return TOKEN(ORF);
"=>"                            SAVE_LOC;return TOKEN(OFN);
"!"                             SAVE_LOC;return TOKEN(OEX);
"~"                             SAVE_LOC;return TOKEN(OTIL);
"%"                             SAVE_LOC;return TOKEN(OPCS);
"&"                             SAVE_LOC;return TOKEN(OANDS);
":"								SAVE_LOC;return TOKEN(OCL);
"::"                            SAVE_LOC;return TOKEN(ODCL);
"="						        SAVE_LOC;return TOKEN(OEQS);

"("	          					SAVE_LOC;return TOKEN(LPAREN);
")"					          	SAVE_LOC;return TOKEN(RPAREN);
"["         					SAVE_LOC;return TOKEN(LSQBRACE);
"]"         					SAVE_LOC;return TOKEN(RSQBRACE);
"{"         					SAVE_LOC;return TOKEN(LBRACE);
"}"					          	SAVE_LOC;return TOKEN(RBRACE);
"[#"							SAVE_LOC;return TOKEN(ATTR);
"."         					SAVE_LOC;return TOKEN(DOT);
","				          		SAVE_LOC;return TOKEN(COMMA);

"using"							SAVE_LOC;return TOKEN(KUSING);
"fn"							SAVE_LOC;return TOKEN(KFN);
"var"							SAVE_LOC;return TOKEN(KVAR);
"let"							SAVE_LOC;return TOKEN(KLET);
"return"						SAVE_LOC;return TOKEN(KRETURN);
"if"							SAVE_LOC;return TOKEN(KIF);
"while"							SAVE_LOC;return TOKEN(KWHILE);
"else"							SAVE_LOC;return TOKEN(KELSE);
"true"							SAVE_LOC;return TOKEN(KBTRUE);
"false"							SAVE_LOC;return TOKEN(KBFALSE);

[a-zA-Z_$][a-zA-Z0-9_]*			SAVE_LOC;SAVE_TOKEN; return IDENTIFIER;
[0-9]+\.[0-9]* 					SAVE_LOC;SAVE_TOKEN; return FLOATPOINT;
[0-9]+					        SAVE_LOC;SAVE_TOKEN; return INTEGER;

"&&"                            SAVE_LOC;return TOKEN(BAND);
"||"                            SAVE_LOC;return TOKEN(BOR);
"=="				          	SAVE_LOC;return TOKEN(BCEQ);
"!="			          		SAVE_LOC;return TOKEN(BCNE);
"<"				          		SAVE_LOC;return TOKEN(BCLT);
"<="	          				SAVE_LOC;return TOKEN(BCLE);
">"				          		SAVE_LOC;return TOKEN(BCGT);
">="					        SAVE_LOC;return TOKEN(BCGE);
"^^"				          	SAVE_LOC;return TOKEN(BPOW);
"+"				          		SAVE_LOC;return TOKEN(BPLUS);
"-"		          				SAVE_LOC;return TOKEN(BMINUS);
"*"		          				SAVE_LOC;return TOKEN(BMUL);
"/"				          		SAVE_LOC;return TOKEN(BDIV);
"++"                            SAVE_LOC;return TOKEN(BPP);
"--"                            SAVE_LOC;return TOKEN(BMM);



.								printf("Unknown token!\n"); yyterminate();

%%
