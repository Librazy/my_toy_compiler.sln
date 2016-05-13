%{
#include <string>
#include "node.hpp"
#include "parser.hpp"

//Some code copied from https://github.com/elite-lang/RedApple
using namespace LIL;
#define SAVE_TOKEN	   (term[1]=term[0],yylval.lit = new std::string(yytext, yyleng))
#define SAVE_LOC	   saveLocation()
#define TOKEN(t)	   (term[1]=term[0],term[0]=false,yylval.token = t)
#define STATUS_BACK    if(status == STRINGSTATUS){BEGIN STRINGSTATUS;}else{BEGIN CHARSTATUS;}
#define TERM		   term[0]=true
bool term[2];
int charno = 1;
int status;
std::wstring strBuf;
extern void yyerror(const char*);
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
[\t ]*	                {}
\n	                    { charno = 1;TERM;}
\/\/.*\n	            { charno = 1;TERM;}
"/*"	                { BEGIN CMNT;}
<CMNT>.	                {}
<CMNT>\n	            { charno = 1;}
<CMNT>"*/"	            { BEGIN INITIAL;}

'	                    { BEGIN CHARSTATUS; strBuf.clear();}
<CHARSTATUS>\\	        { status = CHARSTATUS; BEGIN ESCAPE;}
<CHARSTATUS>\n	        { yyerror("missing terminating ' character");}
<CHARSTATUS>'	        { BEGIN INITIAL; yylval.wchar = getWchar(); return CHAR;}
<CHARSTATUS>.	        { strBuf.push_back(yytext[0]);}

\"	                    { BEGIN STRINGSTATUS; strBuf.clear();}
<STRINGSTATUS>\\	    { status = STRINGSTATUS; BEGIN ESCAPE;}
<STRINGSTATUS>\n	    { yyerror("missing terminating \" character");}
<STRINGSTATUS>\"	    { BEGIN INITIAL; yylval.wstr = new std::wstring(strBuf); return STRING;}
<STRINGSTATUS>.	        { strBuf.push_back(yytext[0]);}

<ESCAPE>n	            { STATUS_BACK; strBuf.push_back('\n');}
<ESCAPE>t	            { STATUS_BACK; strBuf.push_back('\t');}
<ESCAPE>b	            { STATUS_BACK; strBuf.push_back('\b');}
<ESCAPE>r	            { STATUS_BACK; strBuf.push_back('\r');}
<ESCAPE>f	            { STATUS_BACK; strBuf.push_back('\f');}
<ESCAPE>'	            { STATUS_BACK; strBuf.push_back('\'');}
<ESCAPE>\"	            { STATUS_BACK; strBuf.push_back('"');}
<ESCAPE>\\	            { STATUS_BACK; strBuf.push_back('\\');}
<ESCAPE>u[0-9a-fA-F]{4} { STATUS_BACK; strBuf.push_back(getUnicode());}
<ESCAPE>.	            { yyerror("unknown escape sequence");}


"<-"                            return TOKEN(OLF);
"->"                            return TOKEN(ORF);
"=>"                            return TOKEN(OFN);
"!"                             return TOKEN(OEX);
"~"                             return TOKEN(OTIL);
"%"                             return TOKEN(OPCS);
"&"                             return TOKEN(OANDS);
":"								return TOKEN(OCL);
"::"                            return TOKEN(ODCL);
"="						        return TOKEN(OEQS);
";"								TERM;return TOKEN(OCL);

"("	          					return TOKEN(LPAREN);
")"					          	return TOKEN(RPAREN);
"["         					return TOKEN(LSQBRACE);
"]"         					return TOKEN(RSQBRACE);
"{"         					return TOKEN(LBRACE);
"}"					          	return TOKEN(RBRACE);
"[#"							return TOKEN(ATTR);
"."         					return TOKEN(DOT);
","				          		return TOKEN(COMMA);

"using"							return TOKEN(KUSING);
"fn"							return TOKEN(KFN);
"var"							return TOKEN(KVAR);
"let"							return TOKEN(KLET);
"return"						return TOKEN(KRETURN);
"if"							return TOKEN(KIF);
"while"							return TOKEN(KWHILE);
"else"							return TOKEN(KELSE);
"true"							return TOKEN(KBTRUE);
"false"							return TOKEN(KBFALSE);

[a-zA-Z_$][a-zA-Z0-9_]*			SAVE_TOKEN; return IDENTIFIER;
[0-9]+\.[0-9]* 					SAVE_TOKEN; return FLOATPOINT;
[0-9]+					        SAVE_TOKEN; return INTEGER;

"&&"                            return TOKEN(BAND);
"||"                            return TOKEN(BOR);
"=="				          	return TOKEN(BCEQ);
"!="			          		return TOKEN(BCNE);
"<"				          		return TOKEN(BCLT);
"<="	          				return TOKEN(BCLE);
">"				          		return TOKEN(BCGT);
">="					        return TOKEN(BCGE);
"^^"				          	return TOKEN(BPOW);
"+"				          		return TOKEN(BPLUS);
"-"		          				return TOKEN(BMINUS);
"*"		          				return TOKEN(BMUL);
"/"				          		return TOKEN(BDIV);
"++"                            return TOKEN(BPP);
"--"                            return TOKEN(BMM);



.								printf("Unknown token!\n"); yyterminate();

%%
