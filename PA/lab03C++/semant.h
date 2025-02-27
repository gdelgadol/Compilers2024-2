#ifndef SEMANT_H_
#define SEMANT_H_

#include <assert.h>
#include <iostream>  
#include "cool-tree.h"
#include "stringtab.h"
#include "symtab.h"
#include "list.h"
#include <map>
#include <list>

#define TRUE 1
#define FALSE 0

class ClassTable;
typedef ClassTable *ClassTableP;

// This is a structure that may be used to contain the semantic
// information such as the inheritance graph.  You may use it or not as
// you like: it is only here to provide a container for the supplied
// methods.

class ClassTable {
private:
	int semant_errors; //Número de errores semánticos
	void install_basic_classes(); // Meter clases básicas de COOL
	ostream& error_stream; //Flujo para imprimir errores
	
public:
	std::map<Symbol, Class_> m_classes; //Nombre de las clases mapean al nodo del AST de esa clase
	ClassTable(Classes); //Inicializar la tabla
	int errors() { return semant_errors; } //Método que devuelve la cantidad de errores
	ostream& semant_error(); //Imprimir errores
	ostream& semant_error(Class_ c); //Imprime Error en Clase c
	ostream& semant_error(Symbol filename, tree_node *t); //Imprime el archivo y el nodo con error

	// These methods are not in the starting code.
	bool CheckInheritance(Symbol ancestor, Symbol child); //Método para verificar la herencia entre dos clases
	Symbol FindCommonAncestor(Symbol type1, Symbol type2); //Encontrar el ancestro común de dos Clases LUB
	std::list<Symbol> GetInheritancePath(Symbol type); //Lista de ancestros de type hasta Object
};

#endif