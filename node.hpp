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

	class NBool : public Node {
	public:
		bool value;
		NBool(bool value) : value(value) { }
		virtual LValpT codeGen(CodeGenC& context) override;
	};

	class NInteger : public Node {
	public:
		long long value;
		NInteger(long long value) : value(value) { }
		virtual LValpT codeGen(CodeGenC& context) override;
	};

	class NDouble : public Node {
	public:
		double value;
		NDouble(double value) : value(value) { }
		virtual LValpT codeGen(CodeGenC& context) override;
	};

	class NIdentifier : public Node {
	public:
		std::string name;
		NIdentifier(const std::string& name) : name(name) { }
		virtual LValpT codeGen(CodeGenC& context) override;
	};

	class NMethodCall : public Node {
	public:
		const NIdentifier& id;
		ExpressionList arguments;
		NMethodCall(const NIdentifier& id, ExpressionList& arguments) :
			id(id), arguments(arguments) { }
		NMethodCall(const NIdentifier& id) : id(id) { }
		virtual LValpT codeGen(CodeGenC& context) override;
	};

	class NBinaryOperator : public Node {
	public:
		int op;
		Node& lhs;
		Node& rhs;
		NBinaryOperator(Node& lhs, int op, Node& rhs) :
			op(op), lhs(lhs), rhs(rhs) { }
		virtual LValpT codeGen(CodeGenC& context) override;
	};

	class NAssignment : public Node {
	public:
		NIdentifier& lhs;
		Node& rhs;
		NAssignment(NIdentifier& lhs, Node& rhs) :
			lhs(lhs), rhs(rhs) { }
		virtual LValpT codeGen(CodeGenC& context) override;
	};

	class NBlock : public Node {
	public:
		ExpressionList expressions;
		NBlock() { }
		virtual LValpT codeGen(CodeGenC& context) override;
	};

	class NReturn : public Node {
	public:
		Node& expression;
		NReturn(Node& expression) :
			expression(expression) { }
		virtual LValpT codeGen(CodeGenC& context) override;
	};

	class NIfBlock : public Node
	{
	public:
		Node& cond;
		NBlock& thenblock;
		NBlock& elseblock;
		NIfBlock(Node& cond, NBlock& thenblock, NBlock& elseblock) :
			cond(cond), thenblock(thenblock), elseblock(elseblock) { };
		virtual LValpT codeGen(CodeGenC& context) override;
	};

	class NWhileBlock : public Node
	{
	public:
		Node& cond;
		NBlock& doblock;
		NWhileBlock(Node& cond, NBlock& doblock) :
			cond(cond), doblock(doblock) { };
		virtual LValpT codeGen(CodeGenC& context) override;
	};

	class NVariableDeclaration : public Node {
	public:
		const NIdentifier& type;
		NIdentifier& id;
		NVariableDeclaration(const NIdentifier& type, NIdentifier& id) :
			type(type), id(id) {
		}
	};

	class NVariableDefinition : public Node {
	public:
		const NIdentifier& type;
		NIdentifier& id;
		Node *assignmentExpr;
		NVariableDefinition(const NVariableDeclaration& decl, Node *assignmentExpr = nullptr) :
			type(decl.type), id(decl.id), assignmentExpr(assignmentExpr) { }
		virtual LValpT codeGen(CodeGenC& context) override;
	};

	class NExternDeclaration : public Node {
	public:
		const NIdentifier& type;
		const NIdentifier& id;
		VariableList arguments;
		NExternDeclaration(const NIdentifier& type, const NIdentifier& id,
			const VariableList& arguments) :
			type(type), id(id), arguments(arguments) {}
		virtual LValpT codeGen(CodeGenC& context) override;
	};

	class NFunctionDeclaration : public Node {
	public:
		const NIdentifier& type;
		const NIdentifier& id;
		VariableList arguments;
		NBlock& block;
		NFunctionDeclaration(const NIdentifier& type, const NIdentifier& id,
			const VariableList& arguments, NBlock& block) :
			type(type), id(id), arguments(arguments), block(block) { }
		virtual LValpT codeGen(CodeGenC& context) override;
	};
}