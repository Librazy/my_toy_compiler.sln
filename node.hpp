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

	typedef LValue* LValpT;
	typedef std::vector<Node*> LNodepvT;

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

	class NIdentifier :public Node
	{
	public:
		explicit NIdentifier(std::string ident);
		std::string ident;
	};
	class NIdentifierList :public Node
	{
	public:
		explicit NIdentifierList(std::vector<std::string> idents);
		explicit NIdentifierList(std::string ident);
		std::vector<std::string> idents;
	};
	class NUsing :public Node
	{
	public:
		explicit NUsing(std::vector<std::string> idents);
		explicit NUsing(NIdentifierList idents);
		std::vector<std::string> idents;
	};
	class NBlock : public Node {
	public:
		LNodepvT exprs;
		NBlock(Node* expr);
		NBlock();
	};
}