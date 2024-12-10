/*
 *  The scanner definition for COOL.
 */

/*
 *  Stuff enclosed in %{ %} in the first section is copied verbatim to the
 *  output, so headers and global definitions are placed here to be visible
 * to the code in the file.  Don't remove anything that was here initially
 */
%{
#include <cool-parse.h>
#include <stringtab.h>
#include <utilities.h>

/* The compiler assumes these identifiers. */
#define yylval cool_yylval
#define yylex  cool_yylex

/* Max size of string constants */
#define MAX_STR_CONST 1025
#define YY_NO_UNPUT   /* keep g++ happy */

extern FILE *fin; /* we read from this file */

/* define YY_INPUT so we read from the FILE fin:
 * This change makes it possible to use this scanner in
 * the Cool compiler.
 */
#undef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
	if ( (result = fread( (char*)buf, sizeof(char), max_size, fin)) < 0) \
		YY_FATAL_ERROR( "read() in flex scanner failed");

char string_buf[MAX_STR_CONST]; /* to assemble string constants */
char *string_buf_ptr;

extern int curr_lineno;
extern int verbose_flag;

extern YYSTYPE cool_yylval;

static int comment_level;  /*Variable para contar los niveles de anidación del comentarios*/
/*
 *  Add Your own definitions here
 */

%}

/*
 * Define names for regular expressions here.
 */

DARROW          =>
LEQ             <=
ASSIGN          <-

LETTER          [a-zA-Z]
LETTERS         {LETTER}+

WHITESPACE      [ \t\f\v\r\x0b]+
NEWLINE         [\n]

/*Palabras clave, con todas las combinaciones de mayúsculas y minúsuclas*/
DIGIT           [0-9]
INTEGER         {DIGIT}+
CLASS           [cC][lL][aA][sS][sS]
ELSE            [eE][lL][sS][eE]
IF              [iI][fF]
FI              [fF][iI]
IN              [iI][nN]
INHERITS        [iI][nN][hH][eE][rR][iI][tT][sS]
LET             [lL][eE][tT]
LOOP            [lL][oO][oO][pP]
POOL            [pP][oO][oO][lL]
THEN            [tT][hH][eE][nN]
WHILE           [wW][hH][iI][lL][eE]
CASE            [cC][aA][sS][eE]
ESAC            [eE][sS][aA][cC]
OF              [oO][fF]
NEW             [nN][eE][wW]
ISVOID          [iI][sS][vV][oO][iI][dD]
NOT             [nN][oO][tT]
/*Booleanos, con la primera en minúscula*/
TRUE            [t][rR][uU][eE]
FALSE           [f][aA][lL][sS][eE]

/*Identificadores*/
OBJECTIDEN      [a-z][a-zA-Z0-9_]*
TYPEIDEN        [A-Z][a-zA-Z0-9_]*



%Start COMMENT
%Start STRING
%Start INLINE_COMMENT
%%

<INITIAL,INLINE_COMMENT,COMMENT>"(*" {/*Nested comments (Partes extraídas de github.com/skyzluo/CS143-Compilers-Stanford)*/
                                      comment_level++;
                                      BEGIN COMMENT; /*Iniciar comentario y sumar un nivel de anidación*/}

<COMMENT>"*)"  {comment_level--; if (comment_level == 0){ BEGIN 0;}} /*Salir de un nivel de comentario. Comenzar de nuevo el estado inicial si salimos del primer nivel*/

<COMMENT>[^\*\(\)\n]* { } /*Consumir caracteres en el comentario, excepto el salto de línea, paréntesis y asterisco*/

<COMMENT>[\n] {curr_lineno++;}/*Salto de línea*/

<COMMENT>. { } /*Consumir los caracteres restantes*/ 

<COMMENT><<EOF>> { cool_yylval.error_msg = "EOF in comment"; BEGIN 0; return ERROR; } /*Fin de archivo en medio de un comentario*/

<INITIAL>"--" { BEGIN INLINE_COMMENT; } /*Comenzar comentarios de línea*/

<INLINE_COMMENT>[^\n]+ { } /*Consumir todo menos salto de línea*/

<INLINE_COMMENT>{NEWLINE} { curr_lineno++; BEGIN 0; } /*Saltar la línea y salir del comentario*/

"*)" {cool_yylval.error_msg = "Unmatched *)"; return ERROR; } /*Manejar error de cierre de comentario sin iniciar*/

<INITIAL>'\"' {/*Inicialización de un string al detectar \"*/
                string_buf[0] = '\0';
                string_buf_ptr = string_buf;
                BEGIN STRING;
              }

<STRING><<EOF>>  {
                    string_buf[0] = '\0';
                    cool_yylval.error_msg = "EOF in string constant";
                    BEGIN 0;
                    return ERROR;
                  }

<STRING>'\"'  { 
                BEGIN INITIAL;
                if (strlen(string_buf) > MAX_STR_CONST){
                  string_buf[0] = '\0';
                  cool_yylval.error_msg = "Undetermined string constant";
                  return ERROR;
                } else {
                  cool_yylval.symbol = stringtable.add_string(string_buf);
                  return STR_CONST;
                }
              }

<STRING>\n  {
              curr_lineno++;
              string_buf[0] = '\0';
              cool_yylval.error_msg = "Undetermined string constant";
              BEGIN 0;
              return ERROR;
            }

<STRING>\0  {
              string_buf[0] = '\0';
              cool_yylval.error_msg = "String contains null character";
              BEGIN 0;
              return ERROR;
            }

<STRING>[^\\\n"\0]+ {
                      *string_buf_ptr = yytext[0];
                      string_buf_ptr++;
                    }

{DARROW}		{ return DARROW; }
{LEQ}       { return LE; }
{ASSIGN}    { return ASSIGN; }

{NEWLINE} { curr_lineno++; }

{WHITESPACE} { }

{CLASS}   { return CLASS; }
{ELSE}    { return ELSE; }
{IF}      { return IF; }
{FI}      { return FI; }
{IN}      { return IN; }
{INHERITS} { return INHERITS; }
{LET}     { return LET; }
{LOOP}    { return LOOP; }
{POOL}    { return POOL; }
{THEN}    { return THEN; }
{WHILE}   { return WHILE; }
{CASE}    { return CASE; }
{ESAC}    { return ESAC; }
{OF}      { return OF; }
{NEW}     { return NEW; }
{ISVOID}  { return ISVOID; }
{NOT}     { return NOT; }

{TRUE}  { cool_yylval.boolean = true; return BOOL_CONST; }
{FALSE} { cool_yylval.boolean = false; return BOOL_CONST; }

{INTEGER} {cool_yylval.symbol = inttable.add_string(yytext); return INT_CONST; }

{OBJECTIDEN} {cool_yylval.symbol = idtable.add_string(yytext); return OBJECTID; }

{TYPEIDEN} {cool_yylval.symbol = idtable.add_string(yytext); return TYPEID; }

"+" { return int('+'); }
"-" { return int('-'); }
"*" { return int('*'); }
"/" { return int('/'); }
"<" { return int('<'); }
"=" { return int('='); }
"." { return int('.'); }
";" { return int(';'); }
"~" { return int('~'); }
"{" { return int('{'); }
"}" { return int('}'); }
"(" { return int('('); }
")" { return int(')'); }
":" { return int(':'); }
"@" { return int('@'); }
"," { return int(','); }

. {cool_yylval.error_msg = yytext; return ERROR; }
  
%%

void append_string(char * yytext, int yyleng) {
  int len = strlen(string_buf);
  strncat(string_buf, yytext, MAX_STR_CONST - len);
}

void set_error_message(char * msg) {
  string_buf[0] = '\0';
  strcat(string_buf, msg);
  cool_yylval.error_msg = string_buf;
}