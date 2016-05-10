#pragma once
#include <iostream>
#include <vector>
#include "ltype.hpp"
#include "lvalue.hpp"


#ifndef YYLTYPE_IS_DECLARED
#define YYLTYPE_IS_DECLARED
typedef struct YYLTYPE
{
	int first_line;
	int first_column;
	int last_line;
	int last_column;
} YYLTYPE;
#endif




namespace LIL {
	class Node;
	class NVariableDeclaration;
	typedef std::vector<Node*> ExpressionList;
	typedef std::vector<NVariableDeclaration*> VariableList;
	typedef LValue* LValpT;

	class Node {
	public:
		int firstLine = -1;
		int firstColumn = -1;
		int lastLine = -1;
		int lastColumn = -1;
		Node* type;
		virtual ~Node() {}
		virtual LValpT codeGen(CodeGenC& context) { return nullptr; }
	};


}