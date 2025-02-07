/*
*  cool.y
*              Parser definition for the COOL language.
*
*/
%{
    #include <iostream>
    #include "cool-tree.h"
    #include "stringtab.h"
    #include "utilities.h"

    extern char *curr_filename;
  
  
    /* Locations */
    #define YYLTYPE int              /* the type of locations */
    #define cool_yylloc curr_lineno  /* use the curr_lineno from the lexer
                                        for the location of tokens */
    
    extern int node_lineno;          /* set before constructing a tree node
                                        to whatever you want the line number
                                        for the tree node to be */
      
      
    #define YYLLOC_DEFAULT(Current, Rhs, N)         \
    Current = Rhs[1];                             \
    node_lineno = Current;
    
    
    #define SET_NODELOC(Current)  \
    node_lineno = Current;
    
    /* IMPORTANT NOTE ON LINE NUMBERS
    *********************************
    * The above definitions and macros cause every terminal in your grammar to 
    * have the line number supplied by the lexer. The only task you have to
    * implement for line numbers to work correctly, is to use SET_NODELOC()
    * before constructing any constructs from non-terminals in your grammar.
    * Example: Consider you are matching on the following very restrictive 
    * (fictional) construct that matches a plus between two integer constants. 
    * (SUCH A RULE SHOULD NOT BE  PART OF YOUR PARSER):
    
    plus_consts : INT_CONST '+' INT_CONST 
    
    * where INT_CONST is a terminal for an integer constant. Now, a correct
    * action for this rule that attaches the correct line number to plus_const
    * would look like the following:
    
    plus_consts : INT_CONST '+' INT_CONST 
    {
        // Set the line number of the current non-terminal:
        // ***********************************************
        // You can access the line numbers of the i'th item with @i, just
        // like you acess the value of the i'th exporession with $i.
        //
        // Here, we choose the line number of the last INT_CONST (@3) as the
        // line number of the resulting expression (@$). You are free to pick
        // any reasonable line as the line number of non-terminals. If you 
        // omit the statement @$=..., bison has default rules for deciding which 
        // line number to use. Check the manual for details if you are interested.
        @$ = @3;


        // Observe that we call SET_NODELOC(@3); this will set the global variable
        // node_lineno to @3. Since the constructor call "plus" uses the value of 
        // this global, the plus node will now have the correct line number.
        SET_NODELOC(@3);

        // construct the result node:
        $$ = plus(int_const($1), int_const($3));
    }
    
    */
    
    
    
    void yyerror(char *s);        /*  defined below; called for each parse error */
    extern int yylex();           /*  the entry point to the lexer  */
    
    /************************************************************************/
    /*                DONT CHANGE ANYTHING IN THIS SECTION                  */
    
    Program ast_root;         /* the result of the parse  */
    Classes parse_results;        /* for use in semantic analysis */
    int omerrs = 0;               /* number of errors in lexing and parsing */
%}
    
/* A union of all the types that can be the result of parsing actions. */
%union {
    Boolean boolean;
    Symbol symbol;
    Program program;
    Class_ class_;
    Classes classes;
    Feature feature;
    Features features;
    Formal formal;
    Formals formals;
    Case case_;
    Cases cases;
    Expression expression;
    Expressions expressions;
    char *error_msg;
}

/* 
  Declare the terminals; a few have types for associated lexemes.
  The token ERROR is never used in the parser; thus, it is a parse
  error when the lexer returns it.

  The integer following token declaration is the numeric constant used
  to represent that token internally.  Typically, Bison generates these
  on its own, but we give explicit numbers to prevent version parity
  problems (bison 1.25 and earlier start at 258, later versions -- at
  257)
*/
%token CLASS 258 ELSE 259 FI 260 IF 261 IN 262 
%token INHERITS 263 LET 264 LOOP 265 POOL 266 THEN 267 WHILE 268
%token CASE 269 ESAC 270 OF 271 DARROW 272 NEW 273 ISVOID 274
%token <symbol>  STR_CONST 275 INT_CONST 276 
%token <boolean> BOOL_CONST 277
%token <symbol>  TYPEID 278 OBJECTID 279 
%token ASSIGN 280 NOT 281 LE 282 ERROR 283

/*  DON'T CHANGE ANYTHING ABOVE THIS LINE, OR YOUR PARSER WONT WORK       */
/**************************************************************************/

/* Complete the nonterminal list below, giving a type for the semantic
value of each non terminal. (See section 3.6 in the bison 
documentation for details). */

/* Partes extraídas de https://github.com/skyzluo/CS143-Compilers-Stanford/blob/master/PA3/cool.y */

/* Lista de símbolos no terminales de la gramática. */
%type <program> program
%type <classes> class_list
%type <class_> class
%type <formals> formal_list
%type <formals> nonempty_formal_list
%type <formal> formal
%type <expression> expression
%type <expressions> expression_list
%type <expressions> nonempty_block
%type <cases> case_list
%type <case_> branch
%type <expression> inner_let
%type <expression> nonempty_expr
%type <features> feature_list
%type <features> nonempty_feature_list
%type <feature> feature
%type <expression> while_exp

/* 
    Declaración de precedencia que van acorde
    a la gramática en el manual de COOL.
    No están en el mismo orden debido a como
    Bison maneja la precedencia, la cual es de menor
    a mayor, a diferencia del manual de COOL.
*/
%left ASSIGN
%left NOT
%nonassoc LE '<' '='
%left '+' '-'
%left '*' '/'
%left ISVOID
%left '~'
%left '@'
%left '.'

%%

/*
    Comentarios sobre notación:
    - $: Se refiere al valor semántico de la regla actual.
    - $i: Se refiere al valor semántico del i-ésimo símbolo en el lado derecho de la producción.
    - @i: Se refiere a la ubicación/dirección en memoria del i-ésimo símbolo en el lado derecho de la producción.
    - $$: Se refiere al valor semántico de la parte izquierda de la producción.
    - @$: Se refiere a la ubicación de la parte izquierda de la producción.
    - $$ = $i: Asigna el valor semántico del i-ésimo símbolo al valor semántico de la parte izquierda de la producción.
    - @$ = @i: Asigna la ubicación del i-ésimo símbolo a la ubicación de la parte izquierda de la producción.
    - $$ = function($1, $2, ...): Asigna el valor de la función con los valores semánticos 
           de los símbolos al valor semántico de la parte izquierda de la producción.
    - function($1, $2, ...): Llamada a la función con los valores semánticos de los símbolos.
*/

/*  
    El string table es para almacenar strings de manera compacta y eficiente, 
    evitando redundancias y habilitando búsquedas rápidas. 
    Hay para identificadores (idtable), strings (stringtable) y enteros (inttable).

    De la misma manera, el symbol table es para almacenar los 
    valores de los símbolo de manera compacta y eficiente.
*/

/*
Guarda la raíz del AST en la variable global.
El resultado de la lista de clases se pasa a la raíz.
De aquí en adelantes, se crean nodos del AST, excepto por las listas, las cuales son listas de nodos.
*/
program : class_list {@$ = @1; ast_root = program($1) ; } ;

/* Lista de clases
    - Puede ser una sola clase o varias clases.
    - parse_results se usa para almacenar el resultado de la lista de clases
      y asignarlo al valor semántico de la regla actual.
    - single_Classes: Crea una lista de clases con una sola clase $1.
    - append_Classes: Agrega una clase $2 a la lista de clases $1 de manera recursiva.
*/
class_list : class { $$ = single_Classes($1); parse_results = $$; }

            | class_list class { $$ = append_Classes($1, single_Classes($2)); parse_results = $$; };

/* Clases.
    - Puede ser una clase sin herencia o con herencia.
    - class_: Crea una clase con o sin herencia. Recibe el nombre de la clase $2,
      el nombre de la clase padre $4 (si no hay, por defecto es Object y se pasa al idtable),
      , la lista de características $4 y el nombre del archivo actual.
    - idtable.add_string("Object"): Se agrega la clase Object al idtable.
    - stringtable.add_string(curr_filename): Se agrega el nombre del archivo actual al stringtable.
                                             Esto para asociar a la clase el nombre del archivo para tener
                                             contexto de donde se encuentra la clase al momento de reportar errores 
                                             o depurar.
*/
class : CLASS TYPEID '{' feature_list '}' ';'
      { $$ = class_($2, idtable.add_string("Object"), $4, stringtable.add_string(curr_filename));}
      
      | CLASS TYPEID INHERITS TYPEID '{' feature_list '}' ';'
      { $$ = class_($2, $4, $6, stringtable.add_string(curr_filename)); }

      | error ;


/*  
   La lista de características:
    - Puede estar vacia, tener una característica o varias características.
    - single_Features: Crea una lista de características con una sola característica $1.
    - append_Features: Agrega una característica $2 a la lista de características $1 de manera recursiva.
    - nil_Features: Crea una lista de características vacía.

    Se parte en dos reglas porque la lista de características puede ser vacía, y esto puede
    causar conflictos de shift-reduce.
 */

// feature_list
feature_list : nonempty_feature_list
             { $$ = $1; }

             |
             { $$ = nil_Features(); } //feature1; feature2;
             ;

nonempty_feature_list : nonempty_feature_list ';' feature
                      { $$ = append_Features(single_Features($1), $3); }

                      | feature ';'
                      { $$ = single_Features($1); }
                      ;


/* Características:
    - Puede ser un método o un atributo.
    - method: Crea un método con el nombre del método $1, la lista de formales $3, el tipo $6, y la expresión $8.
    - attr: Crea un atributo con el nombre del atributo $1, el tipo del atributo $3 y la expresión $5
      (no_expr() si no se le asigna una expresión).
    - no_expr(): Crea una expresión nula.
*/
feature : OBJECTID '(' formal_list ')' ':' TYPEID '{' nonempty_expr '}'
        { $$ = method($1, $3, $6, $8); }

        | OBJECTID ':' TYPEID
        { $$ = attr($1, $3, no_expr()); }

        | OBJECTID ':' TYPEID ASSIGN expression
        { $$ = attr($1, $3, $5); }

        | error ;

/* Lista de formales/parámetros:
    - Puede ser una lista de formales no vacía o vacía.
    - single_Formals: Crea una lista de formales con un solo formal $1.
    - append_Formals: Agrega un formal $1 a la lista de formales $3 de manera recursiva.
    - nil_Formals: Crea una lista de formales vacía.
*/
formal_list : nonempty_formal_list
            { $$ = $1; }

            |
            { $$ = nil_Formals(); }
            
            ;

// nonempty_formal_list
nonempty_formal_list : formal ',' nonempty_formal_list
                     { $$ = append_Formals(single_Formals($1), $3); }

                     | formal
                     { $$ = single_Formals($1); }

                     ;

/* Formal/parámetro:
    - Crea un formal con el nombre del parámetro $1 y el tipo del parámetro $3.
*/
formal : OBJECTID ':' TYPEID { $$ = formal($1, $3); } ;

/* Lista de expresiones:
    - Puede ser una lista de expresiones no vacía o vacía.
    - single_Expressions: Crea una lista de expresiones con una sola expresión $1.
    - append_Expressions: Agrega una expresión $3 a la lista de expresiones $1 de manera recursiva.
    - nil_Expressions: Crea una lista de expresiones vacía.
*/
expression_list : expression_list ',' nonempty_expr
                { $$ = append_Expressions($1, single_Expressions($3)); }

                | nonempty_expr
                { $$ = single_Expressions($1); }

                |
                { $$ = nil_Expressions(); }
                
                ;

expression : nonempty_expr
           { $$ = $1; }

           |
           { $$ = no_expr(); }

           ;
/* Bloque de expresiones:
    - Puede ser un bloque de expresiones no vacío o vacío.
    - single_Expressions: Crea un bloque de expresiones con una sola expresión $1.
    - append_Expressions: Agrega una expresión $3 a la lista de expresiones $1 de manera recursiva.
    - nil_Expressions: Crea un bloque de expresiones vacío.
*/
nonempty_block : nonempty_expr ';'
               { $$ = single_Expressions($1); }

               | nonempty_expr ';' nonempty_block
               { $$ = append_Expressions(single_Expressions($1), $3); }
               
               | error
               
               ;

/* Lista de casos/branches:
    - Puede ser una lista de casos no vacía o vacía.
    - single_Cases: Crea una lista de casos con un solo caso $1.
    - append_Cases: Agrega un caso $2 a la lista de casos $1 de manera recursiva.
    - nil_Cases: Crea una lista de casos vacía.
*/
case_list : case_list branch ';'
          { $$ = append_Cases($1, single_Cases($2)); }

          | branch ';'
          { $$ = single_Cases($1); }
          
          ;

/* Caso:
    - Crea un caso/branch con el nombre de un objeto $1, el tipo del objeto $3 y asigna la expresión $5.
*/
branch : OBJECTID ':' TYPEID DARROW expression { $$ = branch($1, $3, $5); } ;

/* Ciclo while:
    - Crea un ciclo while con la expresión de condición $2 y la expresión $4.
    - loop: Crea un ciclo while con la expresión de condición $2 y  ejecuta la expresión $4.
*/
while_exp : WHILE nonempty_expr LOOP expression POOL
          { $$ = loop($2, $4); }

          | WHILE nonempty_expr LOOP error
          { }

          ;

/* Lógica de Let:
    - Se separa de la regla expresión para manejar la lógica de let en su propio alcance. Evita ambigüedades.
    - Puede ser una asignación con o sin expresión, o una lista de asignaciones.
    - let: Crea una asignación con el nombre de la variable $1, el tipo de la variable $3,
      la expresión $5 y la expresión de la asignación $7.
    - no_expr(): Crea una expresión nula.
*/
inner_let : OBJECTID ':' TYPEID ASSIGN expression IN expression
          { $$ = let($1, $3, $5, $7); }

          | OBJECTID ':' TYPEID IN expression
          { $$ = let($1, $3, no_expr(), $5); }

          | OBJECTID ':' TYPEID ASSIGN expression ',' inner_let
          { $$ = let($1, $3, $5, $7); }

          | OBJECTID ':' TYPEID ',' inner_let
          { $$ = let($1, $3, no_expr(), $5); }
          
          ;

/* Expresiones:
    - Puede ser una expresión no vacía o vacía.
    - no_expr(): Crea una expresión nula.
    - assign: Crea una expresión de asignación con el nombre de la variable $1 y la expresión $3.
    - static_dispatch: Hace un dispatch estático de la expresión $1, de clase tipo $3, 
                       con el nombre del método $5 y la lista de parámetros/expresiones $7.
                       Se le especifica qué clase debe ejecutar el método.
    - dispatch: Crea una expresión de dispatch con la expresión $1, el nombre del método $3 
                y la lista de parámtros/expresiones $5.
                object(idtable.add_string("self")): Se le pasa el objeto actual al dispatch.
                Por ejemplo, object.method() especifica que el método se ejecuta en el objeto actual.
                method() no especifica el objeto y se asume que es el objeto actual, y por ende es de "self".
    - cond: Crea una expresión condicional con la condición $2, la expresión verdadera $4 y la expresión falsa $6.
    - while_exp: Crea una expresión de ciclo while y luego entramos en la regla while_exp, separando el alcance.
    - block: Crea una expresión de bloque con la lista de expresiones $2.
    - let: Crea una expresión de let, pasando a la regla inner_let para ejecutar la lógica dentro del scope.
    - typcase: Crea una expresión de un switch case, evaluando la expresión $2 y con los casos/branches $4.
    - new_: Crea una nueva espresión de tipo $2.
    - isvoid: Crea una expresión isvoid, evaluando la expresión $2.
    - plus: Crea una expresión de suma con la expresión $1 y la expresión $3.
    - sub: Crea una expresión de resta con la expresión $1 y la expresión $3.
    - mul: Crea una expresión de multiplicación con la expresión $1 y la expresión $3.
    - divide: Crea una expresión de división con la expresión $1 y la expresión $3.
    - neg: Crea una expresión de complemento/negación a nivel de bits con la expresión $2.
    - lt: Crea una expresión de menor que con la expresión $1 y la expresión $3.
    - leq: Crea una expresión de menor o igual que con la expresión $1 y la expresión $3.
    - eq: Crea una expresión de igualdad con la expresión $1 y la expresión $3.
    - comp: Crea una expresión de negación lógica con la expresión $2.
    - object: Crea una expresión de objeto con el nombre del objeto $1.
    - int_const: Crea una expresión de constante entera con el valor de la constante $1.
    - string_const: Crea una expresión de constante de cadena con el valor de la constante $1.
    - bool_const: Crea una expresión de constante booleana con el valor de la constante $1.
    - error: Se usa para manejar errores.
*/

// non-empty expression
nonempty_expr : OBJECTID ASSIGN nonempty_expr
              { $$ = assign($1, $3); }

              | nonempty_expr '@' TYPEID '.' OBJECTID '(' expression_list ')'
              { $$ = static_dispatch($1, $3, $5, $7); }

              | nonempty_expr '.' OBJECTID '(' expression_list ')'
              { $$ = dispatch($1, $3, $5); }

              | OBJECTID '(' expression_list ')'
              { $$ = dispatch(object(idtable.add_string("self")), $1, $3); }

              | IF nonempty_expr THEN nonempty_expr ELSE nonempty_expr FI
              { $$ = cond($2, $4, $6); }

              | while_exp
              { $$ = $1; }

              | '{' nonempty_block '}'
              { $$ = block($2); }

              | LET inner_let
              { $$ = $2; }

              | CASE nonempty_expr OF case_list ESAC
              { $$ = typcase($2, $4); }

              | NEW TYPEID
              { $$ = new_($2); }

              | ISVOID nonempty_expr
              { $$ = isvoid($2); }

              | nonempty_expr '+' nonempty_expr
              { $$ = plus($1, $3); }

              | nonempty_expr '-' nonempty_expr
              { $$ = sub($1, $3); }

              | nonempty_expr '*' nonempty_expr
              { $$ = mul($1, $3); }

              | nonempty_expr '/' nonempty_expr
              { $$ = divide($1, $3); }

              | '~' nonempty_expr
              { $$ = neg($2); }

              | nonempty_expr '<' nonempty_expr
              { $$ = lt($1, $3); }

              | nonempty_expr LE nonempty_expr
              { $$ = leq($1, $3); }

              | nonempty_expr '=' nonempty_expr
              { $$ = eq($1, $3); }

              | NOT nonempty_expr
              { $$ = comp($2); }

              | '(' nonempty_expr ')'
              { $$ = $2; }

              | OBJECTID
              { $$ = object($1); }

              | INT_CONST
              { $$ = int_const($1); }
              
              | STR_CONST
              { $$ = string_const($1); }

              | BOOL_CONST
              { $$ = bool_const($1); }

              | error
              { }
              
              ;

/* end of grammar */
%%

/* This function is called automatically when Bison detects a parse error. */
void yyerror(char *s) {
    extern int curr_lineno;

    cerr << "\"" << curr_filename << "\", line " << curr_lineno << ": "
         << s << " at or near ";
         
    print_cool_token(yychar);
    
    cerr << endl;
    
    omerrs++;

    if (omerrs > 50) {
        fprintf(stdout, "More than 50 errors\n");
        exit(1);
    }
    
}
