#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <vector>
#include <list>
#include <set>
#include <sstream>
#include <string>
#include <cstring>
#include "semant.h"
#include "utilities.h"

static bool TESTING = false;
static std::ostringstream nop_sstream;
static std::ostream &log = TESTING ? std::cout : nop_sstream;


extern int semant_debug;
extern char *curr_filename;

//////////////////////////////////////////////////////////////////////
//
// Symbols
//
// For convenience, a large number of symbols are predefined here.
// These symbols include the primitive type and method names, as well
// as fixed names used by the runtime system.
//
//////////////////////////////////////////////////////////////////////

static Symbol 
    // Nombres de argumentos genéricos utilizados en métodos  
    arg,         // Nombre de argumento genérico (puede representar cualquier parámetro de método)  
    arg2,        // Segundo argumento genérico  

    // Tipos primitivos en Cool  
    Bool,        // Representa el tipo "Bool"  
    Int,         // Representa el tipo "Int"  
    Str,         // Representa el tipo "String"  

    // Métodos para manipulación de cadenas de texto  
    concat,      // Representa el método `concat()`, que concatena cadenas  
    length,      // Representa el método `length()`, que obtiene la longitud de una cadena  
    substr,      // Representa el método `substr()`, que extrae una subcadena  

    // Métodos de la clase `Object`  
    cool_abort,  // Representa `abort()`, que finaliza la ejecución del programa  
    copy,        // Representa `copy()`, que crea una copia del objeto actual  
    type_name,   // Representa `type_name()`, que devuelve el nombre de la clase del objeto  

    // Métodos de entrada/salida de la clase `IO`  
    in_int,      // Representa `in_int()`, que lee un número entero desde la entrada  
    in_string,   // Representa `in_string()`, que lee una cadena desde la entrada  
    out_int,     // Representa `out_int()`, que imprime un número entero  
    out_string,  // Representa `out_string()`, que imprime una cadena de texto  

    // Nombres especiales de clases  
    IO,          // Representa la clase `IO`, que maneja entrada y salida  
    Object,      // Representa la clase `Object`, la raíz de la jerarquía de herencia  
    Main,        // Representa la clase `Main`, que debe estar presente en cada programa Cool  
    No_class,    // Se usa para indicar una clase indefinida o inválida (ej. errores de herencia)  
    No_type,     // Se usa cuando una expresión no tiene un tipo válido (para manejo de errores)  

    // Métodos importantes  
    main_meth,   // Representa el método `main()` dentro de la clase `Main`  
    prim_slot,   // Símbolo interno del compilador para operaciones primitivas  

    // Variables especiales  
    self,        // Representa la palabra clave `self` (referencia al objeto actual)  
    SELF_TYPE,   // Representa el tipo dinámico de `self` dentro de un método  
    str_field,   // Probablemente representa el almacenamiento interno de una cadena (`String`)  
    val          // Puede representar un campo para almacenar valores enteros o booleanos 
; 



// La clase actual que se está analizando en este momento.
// Se usa para verificar atributos, métodos y reglas de herencia dentro de la clase activa.
static Class_ curr_class = NULL;

// Puntero a la tabla de clases (ClassTable).
// Contiene información sobre todas las clases del programa, incluyendo la jerarquía de herencia.
// Se utiliza para validar la existencia de clases y verificar restricciones semánticas.
static ClassTable* classtable;

// Tabla de símbolos que almacena los atributos (variables de instancia) dentro de la clase actual.
// Permite verificar el alcance de las variables y detectar errores como nombres duplicados.
static SymbolTable<Symbol, Symbol> attribtable;

// Definición de un alias (typedef) para simplificar la declaración de tablas de métodos.
// Una MethodTable es una tabla de símbolos que asocia nombres de métodos con su definición.
typedef SymbolTable<Symbol, method_class> MethodTable;

// Mapa que asocia cada clase (Symbol) con su respectiva tabla de métodos (MethodTable).
// Se usa para almacenar y consultar métodos definidos en cada clase, validando herencia y sobreescritura.
static std::map<Symbol, MethodTable> methodtables;

//Partes extraídas de: https://github.com/skyzluo/CS143-Compilers-Stanford
//
// Initializing the predefined symbols.
//
static void initialize_constants(void)
{
    arg         = idtable.add_string("arg");
    arg2        = idtable.add_string("arg2");
    Bool        = idtable.add_string("Bool");
    concat      = idtable.add_string("concat");
    cool_abort  = idtable.add_string("abort");
    copy        = idtable.add_string("copy");
    Int         = idtable.add_string("Int");
    in_int      = idtable.add_string("in_int");
    in_string   = idtable.add_string("in_string");
    IO          = idtable.add_string("IO");
    length      = idtable.add_string("length");
    Main        = idtable.add_string("Main");
    main_meth   = idtable.add_string("main");
    //   _no_class is a symbol that can't be the name of any 
    //   user-defined class.
    No_class    = idtable.add_string("_no_class");
    No_type     = idtable.add_string("_no_type");
    Object      = idtable.add_string("Object");
    out_int     = idtable.add_string("out_int");
    out_string  = idtable.add_string("out_string");
    prim_slot   = idtable.add_string("_prim_slot");
    self        = idtable.add_string("self");
    SELF_TYPE   = idtable.add_string("SELF_TYPE");
    Str         = idtable.add_string("String");
    str_field   = idtable.add_string("_str_field");
    substr      = idtable.add_string("substr");
    type_name   = idtable.add_string("type_name");
    val         = idtable.add_string("_val");
}


// "Classes classes" es una lista de clases definidas en el programa Cool.
ClassTable::ClassTable(Classes classes) : semant_errors(0) , error_stream(cerr) {

    // Se instalan las clases base del lenguaje (Object, IO, Int, Bool, String).
    install_basic_classes();

    // ==============================================
    // Construcción del grafo de herencia
    // ==============================================
    log << "Now building the inheritance graph:" << std::endl;

    // Se insertan todas las clases definidas por el usuario en el mapa `m_classes`
    for (int i = classes->first(); classes->more(i); i = classes->next(i)) {

        // Error: Una clase NO puede llamarse SELF_TYPE
        if (classes->nth(i)->GetName() == SELF_TYPE) {
            semant_error(classes->nth(i)) << "Error! SELF_TYPE redeclared!" << std::endl;
        }

        // Si la clase no ha sido definida antes, se agrega al mapa `m_classes`
        if (m_classes.find(classes->nth(i)->GetName()) == m_classes.end()) {
            m_classes.insert(std::make_pair(classes->nth(i)->GetName(), classes->nth(i)));
        } else {
            // Error: La clase ya ha sido definida antes
            semant_error(classes->nth(i)) << "Error! Class " << classes->nth(i)->GetName() 
                                           << " has been defined!" << std::endl;
            return;
        }
    }

    // Verifica que la clase `Main` esté definida
    if (m_classes.find(Main) == m_classes.end()) {
        semant_error() << "Class Main is not defined." << std::endl;
    }

    // ==============================================
    // Validación de la herencia despues de ingresar todas las clases
    // ==============================================
    // classes-> first() Devuelve el indice del primer elemento de la lista (Object)
    // classes-> more(i) Verifica si hay mas clases
    // classes-> next(i) Avanza al siguiente indice
    for (int i = classes->first(); classes->more(i); i = classes->next(i)) {

        // Se establece la clase actual para la verificación de herencia
        curr_class = classes->nth(i);
        log << "    " << curr_class->GetName();

        // Se obtiene el nombre de la clase padre
        Symbol parent_name = curr_class->GetParent();

        // Se recorre la jerarquía de herencia hasta llegar a Object o encontrar un error
        while (parent_name != Object && parent_name != classes->nth(i)->GetName()) {

            // Error: La clase padre no está definida en `m_classes`
            if (m_classes.find(parent_name) == m_classes.end()) {
                semant_error(curr_class) << "Error! Cannot find class " << parent_name << std::endl;
                return;
            }

            // Error: No se permite heredar de Int, Str, Bool o SELF_TYPE
            if (parent_name == Int || parent_name == Str || parent_name == SELF_TYPE || parent_name == Bool) {
                semant_error(curr_class) << "Error! Class " << curr_class->GetName() 
                                         << " cannot inherit from " << parent_name << std::endl;
                return;
            }

            // Se imprime la relación de herencia en el log
            log << " <- " << parent_name;

            // Se avanza en la jerarquía de herencia
            curr_class = m_classes[parent_name];
            parent_name = curr_class->GetParent();
        }

        // Si se llegó a Object, se finaliza la validación
        if (parent_name == Object) {
            log << " <- " << parent_name << std::endl;
        } else {
            // Error: Se ha detectado un ciclo en la herencia
            semant_error(curr_class) << "Error! Cycle inheritance!" << std::endl;
            return;
        }
    }

    log << std::endl;
}



// ClassTable::CheckInheritance
// ============================
// check whether ancestor is a (direct or indirect) ancestor of child
// 
// input:
//     Symbol ancestor, Symbol child
// 
// output:
//     bool
// 
// note on SELF_TYPE:
//     When some object o in class C is of SELF_TYPE,
//     it means the real(dynamic) type of o might be C,
//     or any subclass of C, depending on the dynamic type of the containing object.
//     Then, how do we check the inheritance in case of SELF_TYPE?
// 
//     1. ancestor = child = SELF_TYPE
//        In this case, we know that the 2 objects have the same dynamic type.
// 
//     2. ancestor = A, child = SELF_TYPE
//        In this case, we don't know what the dynamic type of child.
//        So we just assume child is C.
//        If we know that C <= A, then even child's dynamic type isn't C,
//        it can only be a subclass of C. so we are still safe.
// 
//        However, this makes the type checker more strict than the real world.
//        Consider this scenario:
//        A < C, and child's dynamic type is A (but the type check can't know this!)
//        then the type checker will complain, even though the program should work.
// 
//     3. ancestor = SELF_TYPE, child = A
//        In this case, we have to say that it doesn't type check in any case.
//        Even if A <= C, ancestor's dynamic type could be a subclass of C,
//        which might not be an ancestor of A.
// 
//     To sum up, the type checker is more strict than the real world: it might reject
//     some valid programs, but it will not tolerate any invalid program.
// 
bool ClassTable::CheckInheritance(Symbol ancestor, Symbol child) {
    //Si ambos son SELF_TYPE, true porque xd mismo tipo dinámico
    if (ancestor == SELF_TYPE) {
        return child == SELF_TYPE;
    }
    //Si el hijo de SELF_TYPE, entonces se le asigna el nombre de la clase actual
    if (child == SELF_TYPE) {
        child = curr_class->GetName();
    }
    //Itera todo el recorrido de herencia hasta llegar a No_class. Si llega, falso porque no hay ancestro.
    for (; child != No_class; child = m_classes.find(child)->second->GetParent()) {
        if (child == ancestor) {
            return true;
        }
    }
    return false;
}


// ClassTable::GetInheritancePath
// ==============================
// get a path from type to Object, inclusive
//
// input: Symbol type
//
// output: std::list<Symbol>
// 
std::list<Symbol> ClassTable::GetInheritancePath(Symbol type) {
    if (type == SELF_TYPE) {
        type = curr_class->GetName();
    }

    std::list<Symbol> path;

    // note that Object's father is No_class
    for (; type != No_class; type = m_classes[type]->GetParent()) {
        path.push_front(type);  
    }

    return path;
}


// ClassTable::FindCommonAncestor
// ==============================
// find the first common ancestor of two types
//
// input:
//     Symbol type1, Symbol type2
//
// output:
//     Symbol
//
// note that this function can always return something,
// because any two types have Object as their common ancestor
// 
Symbol ClassTable::FindCommonAncestor(Symbol type1, Symbol type2) {

    std::list<Symbol> path1 = GetInheritancePath(type1);
    std::list<Symbol> path2 = GetInheritancePath(type2);

    Symbol ret;
    std::list<Symbol>::iterator iter1 = path1.begin(),
                                iter2 = path2.begin();

    //Iterar e incrementar las dos listas hasta encontrarse con el mismo elemento
    while (iter1 != path1.end() && iter2 != path2.end()) {
        if (*iter1 == *iter2) {
            ret = *iter1;
        } else {
            break;
        }

        iter1++;
        iter2++;
    }
    //Retorna object si no hay ancestro común
    return ret;
}


// ClassTable::install_basic_classes
// =================================
// put Object, IO, Int, Bool, Str into ClassTable::m_classes
// 
// input:
//     void
// 
// return:
//     void
//
void ClassTable::install_basic_classes() {

    // The tree package uses these globals to annotate the classes built below.
   // curr_lineno  = 0;
    Symbol filename = stringtable.add_string("<basic class>");

    // The following demonstrates how to create dummy parse trees to
    // refer to basic Cool classes.  There's no need for method
    // bodies -- these are already built into the runtime system.

    // IMPORTANT: The results of the following expressions are
    // stored in local variables.  You will want to do something
    // with those variables at the end of this method to make this
    // code meaningful.

    // 
    // The Object class has no parent class. Its methods are
    //        abort() : Object    aborts the program
    //        type_name() : Str   returns a string representation of class name
    //        copy() : SELF_TYPE  returns a copy of the object
    //
    // There is no need for method bodies in the basic classes---these
    // are already built in to the runtime system.

    Class_ Object_class =
    class_(
        Object, 
        No_class,
        append_Features(
            append_Features(
                single_Features(method(cool_abort, nil_Formals(), Object, no_expr())),
                single_Features(method(type_name, nil_Formals(), Str, no_expr()))
            ),
            single_Features(method(copy, nil_Formals(), SELF_TYPE, no_expr()))
        ),
        filename
    );

    // 
    // The IO class inherits from Object. Its methods are
    //        out_string(Str) : SELF_TYPE       writes a string to the output
    //        out_int(Int) : SELF_TYPE            "    an int    "  "     "
    //        in_string() : Str                 reads a string from the input
    //        in_int() : Int                      "   an int     "  "     "
    //
    Class_ IO_class = 
    class_(
        IO, 
        Object,
        append_Features(
            append_Features(
                append_Features(
                    single_Features(method(out_string, single_Formals(formal(arg, Str)),
                        SELF_TYPE, no_expr())
                ),
                    single_Features(method(out_int, single_Formals(formal(arg, Int)),
                        SELF_TYPE, no_expr()))
                ),
                single_Features(method(in_string, nil_Formals(), Str, no_expr()))
            ),
            single_Features(method(in_int, nil_Formals(), Int, no_expr()))
        ),
        filename
    );  

    //
    // The Int class has no methods and only a single attribute, the
    // "val" for the integer. 
    //
    Class_ Int_class =
    class_(
        Int, 
        Object,
        single_Features(attr(val, prim_slot, no_expr())),
        filename
    );

    //
    // Bool also has only the "val" slot.
    //
    Class_ Bool_class =
    class_(Bool, Object, single_Features(attr(val, prim_slot, no_expr())), filename);

    //
    // The class Str has a number of slots and operations:
    //       val                                  the length of the string
    //       str_field                            the string itself
    //       length() : Int                       returns length of the string
    //       concat(arg: Str) : Str               performs string concatenation
    //       substr(arg: Int, arg2: Int): Str     substring selection
    //       
    Class_ Str_class =
    class_(
        Str, 
        Object,
        append_Features(
            append_Features(
                append_Features(
                    append_Features(
                        single_Features(attr(val, Int, no_expr())),
                        single_Features(attr(str_field, prim_slot, no_expr())) //Prim_slot representa espacio de memoria reservados
                        ),
                    single_Features(method(length, nil_Formals(), Int, no_expr()))
                    ),
                single_Features(method(
                    concat, 
                    single_Formals(formal(arg, Str)),
                    Str, 
                    no_expr()
                    ))
                ),
            single_Features(method(
                substr, 
                append_Formals(
                    single_Formals(formal(arg, Int)), 
                    single_Formals(formal(arg2, Int))
                ),
                Str, 
                no_expr()
            ))
        ),
        filename
    );

    m_classes.insert(std::make_pair(Object, Object_class));
    m_classes.insert(std::make_pair(IO, IO_class));
    m_classes.insert(std::make_pair(Int, Int_class));
    m_classes.insert(std::make_pair(Bool, Bool_class));
    m_classes.insert(std::make_pair(Str, Str_class));

}

////////////////////////////////////////////////////////////////////
//
// semant_error is an overloaded function for reporting errors
// during semantic analysis.  There are three versions:
//
//    ostream& ClassTable::semant_error()                
//
//    ostream& ClassTable::semant_error(Class_ c)
//       print line number and filename for `c'
//
//    ostream& ClassTable::semant_error(Symbol filename, tree_node *t)  
//       print a line number and filename
//
///////////////////////////////////////////////////////////////////

ostream& ClassTable::semant_error(Class_ c)
{                                 
    if (c == NULL)
        return semant_error();                         
    return semant_error(c->get_filename(),c);
}    

ostream& ClassTable::semant_error(Symbol filename, tree_node *t)
{
    error_stream << filename << ":" << t->get_line_number() << ": ";
    return semant_error();
}

ostream& ClassTable::semant_error()                  
{                                                 
    semant_errors++;                            
    return error_stream;
}


///////////////////////////////////////////////////////////////////
// Add to attrib / method table
///////////////////////////////////////////////////////////////////

//Tanto method como attribute son Features, por lo tanto, features debe tener una
//función para meter a la tabla de métodos y otro para meter a la tabla de atributos.
//Como heredan, entonces deben tener el otro método tal que no haga nada.
//Por ejemplo, AddMethodTable está definido en method_class pero AddAttribToTable no hace nada.
//Debe declararse, de todas formas.

void method_class::AddMethodToTable(Symbol class_name) {
    log << "    Adding method " << name << std::endl;
    methodtables[class_name].addid(name, new method_class(copy_Symbol(name), formals->copy_list(), copy_Symbol(return_type), expr->copy_Expression()));
}

void method_class::AddAttribToTable(Symbol class_name) { }

void attr_class::AddMethodToTable(Symbol class_name) { }

void attr_class::AddAttribToTable(Symbol class_name) {
    log << "Adding attrib " << name << std::endl;

    if (name == self) {
        classtable->semant_error(curr_class) << "Error! 'self' cannot be the name of an attribute in class " << curr_class->GetName() << std::endl;
    }
    if (attribtable.lookup(name) != NULL) {
        classtable->semant_error(curr_class) << "Error! attribute '" << name << "' already exists!" << std::endl;
        return;
    }

    attribtable.addid(name, new Symbol(type_decl));
}

///////////////////////////////////////////////////////////////////
// Type checking functions
///////////////////////////////////////////////////////////////////
// EN ESTO DEFINIMOS LAS FUNCIONES PARA VERIFICAR LOS TIPOS DE CADA EXPRESIÓN

void method_class::CheckFeatureType() {
    // Imprime en el log que se está verificando el método actual
    log << "    Checking method \"" << name << "\"" << std::endl;

    // Verifica que el tipo de retorno del método exista en la tabla de clases
    // SELF_TYPE es una excepción y no necesita estar en la tabla
    if (classtable->m_classes.find(return_type) == classtable->m_classes.end() && return_type != SELF_TYPE) {
        classtable->semant_error(curr_class) 
            << "Error! return type " << return_type << " doesn't exist." << std::endl;
    }

    // Inicia un nuevo scope en la tabla de atributos para registrar los parámetros del método
    attribtable.enterscope();

    // Conjunto para almacenar los nombres de los parámetros y detectar duplicados
    std::set<Symbol> used_names;

    // Itera sobre todos los parámetros del método
    for (int i = formals->first(); formals->more(i); i = formals->next(i)) {
        Symbol name = formals->nth(i)->GetName();

        // Verifica si el nombre del parámetro ya ha sido usado en este método
        if (used_names.find(name) != used_names.end()) {
            classtable->semant_error(curr_class) 
                << "Error! formal name duplicated. " << std::endl;
        } else {
            used_names.insert(name);
        }

        // Obtiene el tipo del parámetro y verifica que exista en la tabla de clases
        Symbol type = formals->nth(i)->GetType();
        if (classtable->m_classes.find(type) == classtable->m_classes.end()) {
            classtable->semant_error(curr_class) 
                << "Error! Cannot find class " << type << std::endl;
        }

        // Verifica que el nombre del parámetro no sea 'self'
        if (formals->nth(i)->GetName() == self) {
            classtable->semant_error(curr_class) 
                << "Error! self in formal " << std::endl;
        }

        // Agrega el parámetro a la tabla de atributos para que pueda ser usado dentro del método
        attribtable.addid(formals->nth(i)->GetName(), new Symbol(formals->nth(i)->GetType()));
    }
    
    // Obtiene el tipo de la expresión de retorno del método
    Symbol expr_type = expr->CheckExprType();

    // Verifica que el tipo de retorno del método sea un ancestro válido del tipo de la expresión
    if (classtable->CheckInheritance(return_type, expr_type) == false) {
        classtable->semant_error(curr_class) 
            << "Error! return type is not ancestor of expr type. " << std::endl;
    }

    // Sale del scope del método, eliminando los parámetros de la tabla de atributos
    attribtable.exitscope();
}


void attr_class::CheckFeatureType() {
    log << "    Checking attribute \"" << name << "\"" << std::endl;

    // Verifica si la expresión de asignación tiene tipo No_type (no está inicializada)
    if (init->CheckExprType() == No_type) {
        log << "NO INIT!" << std::endl;
    }
}


Symbol assign_class::CheckExprType() { 
    // Busca el tipo de la variable en la tabla de atributos
    Symbol* lvalue_type = attribtable.lookup(name);

    // Obtiene el tipo de la expresión del lado derecho
    Symbol rvalue_type = expr->CheckExprType();

    // Verifica si la variable existe en la tabla de atributos
    if (lvalue_type == NULL) {
        // Si la variable no está definida, genera un error semántico
        classtable->semant_error(curr_class) << "Error! Cannot find lvalue " << name << std::endl;

        // Asigna el tipo "Object" como valor por defecto para evitar más errores
        type = Object;
        return type;
    }

    // Verifica si el tipo del lvalue es un ancestro válido del rvalue
    if (classtable->CheckInheritance(*lvalue_type, rvalue_type) == false) {
        // Si no es un ancestro válido, genera un error semántico
        classtable->semant_error(curr_class) 
            << "Error! lvalue is not an ancestor of rvalue. " << std::endl;

        // Asigna el tipo "Object" para evitar más errores
        type = Object;
        return type;
    }

    // Si no hay errores, asigna el tipo del rvalue como tipo final de la expresión
    type = rvalue_type;
    return type;
}
//(obj@Clase).metodo(param1, param2, ...)
Symbol static_dispatch_class::CheckExprType() {
    // Variable para rastrear si se encuentra algún error
    bool error = false;

    // Arroja el tipo sobre el que llamamos el método (obj) 
    Symbol expr_class = expr->CheckExprType();

    // Verifica que el type_name (Clase) sea ancestro de expr_class (obj)
    if (classtable->CheckInheritance(type_name, expr_class) == false) {
        error = true;
        classtable->semant_error(curr_class) 
            << "Error! Static dispatch class is not an ancestor." << std::endl;
    }

    log << "Static dispatch: class = " << type_name << std::endl;

    // Busca el método en la jerarquía de herencia
    // Se recorre el camino de herencia de type_name (Class) para encontrar la definición del método
    std::list<Symbol> path = classtable->GetInheritancePath(type_name);
    method_class* method = NULL;

    for (std::list<Symbol>::iterator iter = path.begin(); iter != path.end(); ++iter) {
        log << "Looking for method in class " << *iter << std::endl;

        // Busca el método en la tabla de métodos de la clase actual en la jerarquía
        if ((method = methodtables[*iter].lookup(name)) != NULL) {
            break;  // Si se encuentra el método, se detiene la búsqueda
        }
    }

    // Si no se encuentra el método en la jerarquía de herencia, se genera un error
    if (method == NULL) {
        error = true;
        classtable->semant_error(curr_class) 
            << "Error! Cannot find method '" << name << "'" << std::endl;
    }

    // Verificación de los parámetros pasados al método
    for (int i = actual->first(); actual->more(i); i = actual->next(i)) {
        // Obtiene el tipo del parámetro de la iteración
        Symbol actual_type = actual->nth(i)->CheckExprType();

        // Como el parser sigue, solo ejecutamos si encontramos el método
        if (method != NULL) {
            // Obtenemos el tipo del formal en la posición actual
            Symbol formal_type = method->GetFormals()->nth(i)->GetType();

            // Verifica que el tipo del argumento sea un subtipo del tipo esperado
            if (classtable->CheckInheritance(formal_type, actual_type) == false) {
                classtable->semant_error(curr_class) 
                    << "Error! Actual type " << actual_type 
                    << " doesn't suit formal type " << formal_type << std::endl;
                error = true;
            }
        }
    }

    // Si hubo errores, se asigna Object como tipo de retorno para evitar problemas
    if (error) {
        type = Object;
    } else {
        // Si no hay errores, el tipo de la expresión es el tipo de retorno del método
        type = method->GetType();

        // Si el método retorna SELF_TYPE, se reemplaza por la clase sobre la que se hizo dispatch
        if (type == SELF_TYPE) {
            type = type_name;
        }
    }

    return type;
}

//obj.metodo(param1, param2, ...)
Symbol dispatch_class::CheckExprType() {
    // Variable para rastrear si hay errores durante la verificación del dispatch
    bool error = false;

    // Arroja el tipo sobre el que llamamos el método
    Symbol expr_type = expr->CheckExprType();

    // Si el tipo de la expresión es SELF_TYPE, se imprime en el log con el nombre de la clase actual
    if (expr_type == SELF_TYPE) {
        log << "Dispatch: class = " << SELF_TYPE << "_" << curr_class->GetName() << std::endl;
    } else {
        log << "Dispatch: class = " << expr_type << std::endl;
    }

    // Busca el método en la jerarquía de herencia de la clase del objeto en el dispatch
    // Se busca la definición del método en la clase más cercana posible en la jerarquía
    std::list<Symbol> path = classtable->GetInheritancePath(expr_type);
    method_class* method = NULL;

    // Se recorre la jerarquía de herencia para encontrar el método
    for (std::list<Symbol>::iterator iter = path.begin(); iter != path.end(); ++iter) {
        log << "Looking for method in class " << *iter << std::endl;

        // Busca el método en la tabla de métodos de la clase actual de la jerarquía
        if ((method = methodtables[*iter].lookup(name)) != NULL) {
            break; // Si encuentra el método, se detiene la búsqueda
        }
    }

    // Si el método no se encuentra en ninguna clase de la jerarquía, se genera un error
    if (method == NULL) {
        error = true;
        classtable->semant_error(curr_class) 
            << "Error! Cannot find method '" << name << "'" << std::endl;
    }

    // Verificación de los parámetros de la llamada al método
    for (int i = actual->first(); actual->more(i); i = actual->next(i)) {
        // Se obtiene el tipo del argumento pasado en la llamada al método
        Symbol actual_type = actual->nth(i)->CheckExprType();

        // Solo se realiza la verificación si el método fue encontrado
        if (method != NULL) {
            // Se obtiene el tipo esperado del parámetro en la declaración del método
            Symbol formal_type = method->GetFormals()->nth(i)->GetType();

            // Se verifica que el argumento pasado sea un subtipo válido del parámetro esperado
            if (classtable->CheckInheritance(formal_type, actual_type) == false) {
                classtable->semant_error(curr_class) 
                    << "Error! Actual type " << actual_type 
                    << " doesn't suit formal type " << formal_type << std::endl;
                error = true;
            }
        }
    }

    // Si hubo errores en el dispatch, se asigna Object como tipo de retorno para evitar problemas
    if (error) {
        type = Object;
    } else {
        // Si no hubo errores, el tipo de la expresión será el tipo de retorno del método
        type = method->GetType();

        // Si el método retorna SELF_TYPE, se reemplaza por el tipo de la expresión original
        if (type == SELF_TYPE) {
            type = expr_type;
        }
    }

    return type;
}


// condition
// =========
// Expression pred;
// Expression then_exp;
// Expression else_exp;
// 
Symbol cond_class::CheckExprType() {
    //Si el predicado del condicional no es booleano, retorna error
    if (pred->CheckExprType() != Bool) {
        classtable->semant_error(curr_class) << "Error! Type of pred is not Bool." << std::endl;
    }

    //Miramos el tipo de las expresiones en el then y en el else
    Symbol then_type = then_exp->CheckExprType();
    Symbol else_type = else_exp->CheckExprType();

    if (else_type == No_type) {
        // Si no hay un else, el tipo del condicional es el del then
        type = then_type;
    } else {
        type = classtable->FindCommonAncestor(then_type, else_type);
        //Verificamos el ancestro común entre el then y el else. Ese será el tipo de esta expresión.
    }
    return type;
}

Symbol loop_class::CheckExprType() {
    //Si el predicado del condicional no es booleano, retorna error
    if (pred->CheckExprType() != Bool) {
        classtable->semant_error(curr_class) << "Error! Type of pred is not Bool." << std::endl;
    }
    //Miramos el tipo de la expresión del cuerpo
    body->CheckExprType();
    type = Object;
    return type;
}

// case ... of ...
// ===============
// Expression expr;
// Cases cases;
// 
Symbol typcase_class::CheckExprType() {
    //Obtener el tipo de la expresión que evalúa el case
    Symbol expr_type = expr->CheckExprType();

    Case branch;
    //Almacena los retornos de cada rama
    std::vector<Symbol> branch_types;
    //Almacena los tipos declarados en cada rama
    std::vector<Symbol> branch_type_decls;

    //Recorrer cada rama, obtener el retorno de cada rama, obtener el retorno de la declaración
    for (int i = cases->first(); cases->more(i); i = cases->next(i)) {
        branch = cases->nth(i);
        Symbol branch_type = branch->CheckBranchType();
        branch_types.push_back(branch_type);
        branch_type_decls.push_back(((branch_class *)branch)->GetTypeDecl());
    }
    //Verificar que no haya tipos duplicados en las ramas
    // case x of
    //     p: Perro => "Guau";
    //     g: Perro => "Error";  -- Perro está repetido
    // esac
    for (int i = 0; i < branch_types.size() - 1; ++i) {
        for (int j = i + 1; j < branch_types.size(); ++j) {
            if (branch_type_decls[i] == branch_type_decls[j]) {
                classtable->semant_error(curr_class) << "Error! Two branches have same type." << std::endl;
            }
        }
    }
    //Verificar que los tipos de retorno y los tipos declarados tengan ancestro común, o sea, sean válidos.
    type = branch_types[0];
    for (int i = 1; i < branch_types.size(); ++i) {
        type = classtable->FindCommonAncestor(type, branch_types[i]);
    }
    return type;
}

// branch
// ======
// Symbol name;
// Symbol type_decl;
// Expression expr;
// 
Symbol branch_class::CheckBranchType() {
    attribtable.enterscope();

    attribtable.addid(name, new Symbol(type_decl));
    Symbol type = expr->CheckExprType();

    attribtable.exitscope();

    return type;
}

//Obtiene el tipo de la última expresión en el bloque
Symbol block_class::CheckExprType() {
    for (int i = body->first(); body->more(i); i = body->next(i)) {
        type = body->nth(i)->CheckExprType();
    }
    return type;
}

// let
// ===
// Symbol identifier;
// Symbol type_decl;
// Expression init;
// Expression body;
// 
Symbol let_class::CheckExprType() {
    // Verifica que el identificador no sea 'self', ya que 'self' no puede ser redefinido
    if (identifier == self) {
        classtable->semant_error(curr_class) << "Error! self in let binding." << std::endl;
    }

    // Crea un nuevo scope para la variable declarada en el 'let'
    attribtable.enterscope();

    // Agrega la variable a la tabla de atributos con su tipo declarado
    attribtable.addid(identifier, new Symbol(type_decl));

    // Obtiene el tipo de la expresión de inicialización (si existe)
    Symbol init_type = init->CheckExprType();

    // Si hay una expresión de inicialización, verifica que sea un subtipo válido
    if (init_type != No_type) {
        if (classtable->CheckInheritance(type_decl, init_type) == false) {
            classtable->semant_error(curr_class) << "Error! init value is not child." << std::endl;
        }
    }

    // Evalúa el tipo del cuerpo del 'let'
    type = body->CheckExprType();

    // Sale del scope del 'let', eliminando la variable de la tabla de atributos
    attribtable.exitscope();
    
    return type;
}


Symbol plus_class::CheckExprType() {
    Symbol e1_type = e1->CheckExprType();
    Symbol e2_type = e2->CheckExprType();
    if (e1_type != Int || e2_type != Int) {
        classtable->semant_error(curr_class) << "Error! '+' meets non-Int value." << std::endl;
        type = Object;
    } else {
        type = Int;
    }
    return type;
}

Symbol sub_class::CheckExprType() {
    Symbol e1_type = e1->CheckExprType();
    Symbol e2_type = e2->CheckExprType();
    if (e1_type != Int || e2_type != Int) {
        classtable->semant_error(curr_class) << "Error! '-' meets non-Int value." << std::endl;
        type = Object;
    } else {
        type = Int;
    }
    return type;
}

Symbol mul_class::CheckExprType() {
    Symbol e1_type = e1->CheckExprType();
    Symbol e2_type = e2->CheckExprType();
    if (e1_type != Int || e2_type != Int) {
        classtable->semant_error(curr_class) << "Error! '*' meets non-Int value." << std::endl;
        type = Object;
    } else {
        type = Int;
    }
    return type;
}

Symbol divide_class::CheckExprType() {
    Symbol e1_type = e1->CheckExprType();
    Symbol e2_type = e2->CheckExprType();
    if (e1_type != Int || e2_type != Int) {
        classtable->semant_error(curr_class) << "Error! '/' meets non-Int value." << std::endl;
        type = Object;
    } else {
        type = Int;
    }
    return type;
}
//Negación por bits
Symbol neg_class::CheckExprType() {
    if (e1->CheckExprType() != Int) {
        classtable->semant_error(curr_class) << "Error! '~' meets non-Int value." << std::endl;
        type = Object;
    } else {
        type = Int;
    }
    return type;
}

Symbol lt_class::CheckExprType() {
    Symbol e1_type = e1->CheckExprType();
    Symbol e2_type = e2->CheckExprType();
    if (e1_type != Int || e2_type != Int) {
        classtable->semant_error(curr_class) << "Error! '<' meets non-Int value." << std::endl;
        type = Object;
    } else {
        type = Bool;
    }
    return type;
}

// equal
// =====
// any types may be freely compared except for Int, Bool, and Str.
// 
Symbol eq_class::CheckExprType() {
    Symbol e1_type = e1->CheckExprType();
    Symbol e2_type = e2->CheckExprType();
    if (e1_type == Int || e2_type == Int || e1_type == Bool || e2_type == Bool || e1_type == Str || e2_type == Str) {
        if (e1_type != e2_type) {
            classtable->semant_error(curr_class) << "Error! '=' meets different types." << std::endl;
            type = Object;
        } else {
            type = Bool;
        }
    } else {
        type = Bool;
    }
    return type;
}

Symbol leq_class::CheckExprType() {
    Symbol e1_type = e1->CheckExprType();
    Symbol e2_type = e2->CheckExprType();
    if (e1_type != Int || e2_type != Int) {
        classtable->semant_error(curr_class) << "Error! '<=' meets non-Int value." << std::endl;
        type = Object;
    } else {
        type = Bool;
    }
    return type;
}
//Negación lógica
Symbol comp_class::CheckExprType() {
    if (e1->CheckExprType() != Bool) {
        classtable->semant_error(curr_class) << "Error! 'not' meets non-Bool value." << std::endl;
        type = Object;
    } else {
        type = Bool;
    }
    return type;
}

Symbol int_const_class::CheckExprType() {
    type = Int;
    return type;
}

Symbol bool_const_class::CheckExprType() {
    type = Bool;
    return type;
}

Symbol string_const_class::CheckExprType() {
    type = Str;
    return type;
}

Symbol new__class::CheckExprType() {
    // Verifica si la clase instanciada existe en la tabla de clases
    // Se permite SELF_TYPE sin validación adicional, ya que representa la clase actual en tiempo de ejecución
    if (type_name != SELF_TYPE && classtable->m_classes.find(type_name) == classtable->m_classes.end()) {
        classtable->semant_error(curr_class) << "Error! type " << type_name << " doesn't exist." << std::endl;
    }

    type = type_name;

    return type;
}


Symbol isvoid_class::CheckExprType() {
    e1->CheckExprType();
    type = Bool;
    return type;
}

Symbol no_expr_class::CheckExprType() {
    return No_type;
}


Symbol object_class::CheckExprType() {
    if (name == self) {
        type = SELF_TYPE;
        return type;
    }

    Symbol* found_type = attribtable.lookup(name);
    if (found_type == NULL) {
        classtable->semant_error(curr_class) << "Cannot find object " << name << std::endl;
        type = Object;
    } else {
        type = *found_type;
    }
    
    return type;
}

/*   This is the entry point to the semantic checker.
     Your checker should do the following two things:
     1) Check that the program is semantically correct
     2) Decorate the abstract syntax tree with type information
        by setting the `type' field in each Expression node.
        (see `tree.h')
     You are free to first do 1), make sure you catch all semantic
     errors. Part 2) can be done in a second stage, when you want
     to build mycoolc.
 */
void program_class::semant() {
    initialize_constants();

    // Se crea una tabla de clases
    classtable = new ClassTable(classes);

    // Si hubo errores en la construcción de la tabla de clases, se detiene la compilación.
    if (classtable->errors()) {
        cerr << "Compilation halted due to static semantic errors." << endl;
        exit(1);
    }

    log << "Now constructing the methodtables:" << std::endl;

    //Recorrer las clases y crear las methodtables
    for (std::map<Symbol, Class_>::iterator iter = classtable->m_classes.begin(); iter != classtable->m_classes.end(); ++iter) {
        log << "class " << iter->first << ":" << std::endl;

        Symbol class_name = iter->first;
        methodtables[class_name].enterscope();
        Features curr_features = classtable->m_classes[class_name]->GetFeatures();
        for (int j = curr_features->first(); curr_features->more(j); j = curr_features->next(j)) {
             Feature curr_feature = curr_features->nth(j);
             curr_feature->AddMethodToTable(class_name);
        }
    }

    log << "Now searching for illegal method overriding:" << std::endl;

    // Recorrer todas las clases y verificar si redefinen métodos heredados. 
    for (std::map<Symbol, Class_>::iterator iter = classtable->m_classes.begin(); iter != classtable->m_classes.end(); ++iter) {
        
        // Para una clase, obtener todos los métodos.
        Symbol class_name = iter->first;
        curr_class = classtable->m_classes[class_name];
        log << "    Consider class " << class_name << ":" << std::endl;

        Features curr_features = classtable->m_classes[class_name]->GetFeatures();

        for (int j = curr_features->first(); curr_features->more(j); j = curr_features->next(j)) {
            
            // Verificar método de la iteración actual.
            Feature curr_method = curr_features->nth(j);

            if (curr_method->IsMethod() == false) {
                continue;
            }
            
            log << "        method " << curr_method->GetName() << std::endl;

            Formals curr_formals = ((method_class*)(curr_method))->GetFormals();
            
            std::list<Symbol> path = classtable->GetInheritancePath(class_name);
            // Verificar el método actual con los métodos, con el mismo nombre, de los ancestros.
            for (std::list<Symbol>::reverse_iterator iter = path.rbegin(); iter != path.rend(); ++iter) {
                
                Symbol ancestor_name = *iter;
                log << "            ancestor " << ancestor_name << std::endl;
                method_class* method = methodtables[ancestor_name].lookup(curr_method->GetName());
                
                if (method != NULL) {
                    //Si encontramos ese método, verificamos sus parámetros
                    Formals formals = method->GetFormals();

                    int k1 = formals->first(), k2 = curr_formals->first();
                    for (; formals->more(k1) && curr_formals->more(k2); k1 = formals->next(k1), k2 = formals->next(k2)) {
                        if (formals->nth(k1)->GetType() != curr_formals->nth(k2)->GetType()) {
                            log << "error" << std::endl;
                            classtable->semant_error(classtable->m_classes[class_name]) << "Method override error: formal type not match." << std::endl;
                        }
                    }

                    if (formals->more(k1) || curr_formals->more(k2)) {
                        log << "error" << std::endl;
                        classtable->semant_error(classtable->m_classes[class_name]) << "Method override error: length of formals not match." << std::endl;
                    }
                }
            }
        }
    }

    log << std::endl;
    
    //Verificamos los tipos
    log << "Now checking all the types:" << std::endl;
    //Recorrer todas las clases y los atributos
    for (int i = classes->first(); classes->more(i); i = classes->next(i)) {
        curr_class = classes->nth(i);

        log << "Checking class " << curr_class->GetName() << ":" << std::endl;

        // Revisar la herencia la clase actual
        std::list<Symbol> path = classtable->GetInheritancePath(curr_class->GetName());

        // Recorrer cada ancestro de la clase y verificar sus atributos
        for (std::list<Symbol>::iterator iter = path.begin(); iter != path.end(); iter++) {
            curr_class = classtable->m_classes[*iter];
            Features curr_features = curr_class->GetFeatures();
            attribtable.enterscope();
            for (int j = curr_features->first(); curr_features->more(j); j = curr_features->next(j)) {
                Feature curr_feature = curr_features->nth(j);
                curr_feature->AddAttribToTable(curr_class->GetName());
            }
        }
        
        // Restablecer la clase actual
        curr_class = classes->nth(i);
        Features curr_features = curr_class->GetFeatures();

        // Verificar cada atributo y método de la clase actual
        for (int j = curr_features->first(); curr_features->more(j); j = curr_features->next(j)) {
            Feature curr_feature = curr_features->nth(j);
            curr_feature->CheckFeatureType();
        }
        //Salir del scope de cada ancestro
        for (int j = 0; j < path.size(); ++j) {
            attribtable.exitscope();
        }

        log << std::endl;
    }

    // Si hubo errores durante el análisis semántico, se detiene la compilación
    if (classtable->errors()) {
        cerr << "Compilation halted due to static semantic errors." << endl;
        exit(1);
    }

}
