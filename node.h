#pragma once
#include <vector>
#include <llvm/IR/Value.h>

class CodeGenContext;
class NStatement;
class NExpression;
class NVariableDefinition;

typedef std::vector<NStatement*> StatementList;
typedef std::vector<NExpression*> ExpressionList;
typedef std::vector<NVariableDefinition*> VariableList;

using namespace llvm;

class Node
{
public:
	virtual ~Node() {}
	virtual Value* codeGen(CodeGenContext& context) { return nullptr; }
};

class NExpression : public Node
{};

class NStatement : public Node
{};

class NValue : public NExpression
{
protected:
	NValue() {}
public:
	Value* codeGen(CodeGenContext& context) override = 0;
	virtual int64_t getValue() = 0;
};

class NBool : public NValue
{
public:
	bool value;
	explicit NBool(bool value) : value(value) { }
	Value* codeGen(CodeGenContext& context) override;
	int64_t getValue() override;
};

class NInteger : public NValue
{
public:
	int64_t value;
	explicit NInteger(int64_t value) : value(value) { }
	Value* codeGen(CodeGenContext& context) override;
	int64_t getValue() override;
};

class NDouble : public NExpression
{
public:
	double value;
	explicit NDouble(double value) : value(value) { }
	Value* codeGen(CodeGenContext& context) override;
};

class NIdentifier : public NExpression
{
public:
	std::string name;
	explicit NIdentifier(const std::string& name) : name(name) { }
	Value* codeGen(CodeGenContext& context) override;
};

class NMethodCall : public NExpression
{
public:
	const NIdentifier& id;
	ExpressionList arguments;

	NMethodCall(const NIdentifier& id, ExpressionList& arguments) :
		id(id), arguments(arguments) { }

	explicit NMethodCall(const NIdentifier& id) : id(id) { }
	Value* codeGen(CodeGenContext& context) override;
};

class NBinaryOperator : public NExpression
{
public:
	int op;
	NExpression& lhs;
	NExpression& rhs;

	NBinaryOperator(NExpression& lhs, int op, NExpression& rhs) :
		op(op), lhs(lhs), rhs(rhs) { }

	Value* codeGen(CodeGenContext& context) override;
};

class NAssignment : public NExpression
{
public:
	NIdentifier& lhs;
	NExpression& rhs;

	NAssignment(NIdentifier& lhs, NExpression& rhs) :
		lhs(lhs), rhs(rhs) { }

	Value* codeGen(CodeGenContext& context) override;
};

class NBlock : public NExpression
{
public:
	StatementList statements;
	NBlock() { }
	Value* codeGen(CodeGenContext& context) override;
};

class NExpressionStatement : public NStatement
{
public:
	NExpression& expression;

	explicit NExpressionStatement(NExpression& expression) :
		expression(expression) { }

	Value* codeGen(CodeGenContext& context) override;
};

class NReturnStatement : public NStatement
{
public:
	NExpression& expression;

	NReturnStatement(NExpression& expression) :
		expression(expression) { }

	Value* codeGen(CodeGenContext& context) override;
};

class NIfBlock : public NExpression
{
public:
	NExpression& cond;
	NExpression& thenblock;
	NExpression& elseblock;

	NIfBlock(NExpression& cond, NExpression& thenblock, NExpression& elseblock) :
		cond(cond), thenblock(thenblock), elseblock(elseblock) { };
	Value* codeGen(CodeGenContext& context) override;
};

class NWhileBlock : public NStatement
{
public:
	NExpression& cond;
	NBlock& doblock;

	NWhileBlock(NExpression& cond, NBlock& doblock) :
		cond(cond), doblock(doblock) { };
	Value* codeGen(CodeGenContext& context) override;
};

class NVariableDefinition : public NStatement
{
public:
	const NIdentifier& type;
	NIdentifier& id;
	NExpression* assignmentExpr;

	NVariableDefinition(const NIdentifier& type, NIdentifier& id) :
		type(type), id(id) { assignmentExpr = nullptr; }

	NVariableDefinition(const NIdentifier& type, NIdentifier& id, NExpression* assignmentExpr) :
		type(type), id(id), assignmentExpr(assignmentExpr) { }

	Value* codeGen(CodeGenContext& context) override;
};

class NVariableDeclaration : public NStatement
{
public:
	const NIdentifier& type;
	NIdentifier& id;

	NVariableDeclaration(const NIdentifier& type, NIdentifier& id) :
		type(type), id(id) { }
};

class NExternDeclaration : public NStatement
{
public:
	const NIdentifier& type;
	const NIdentifier& id;
	VariableList arguments;

	NExternDeclaration(const NIdentifier& type, const NIdentifier& id,
	                   const VariableList& arguments) :
		type(type), id(id), arguments(arguments) {}

	Value* codeGen(CodeGenContext& context) override;
};

class NFunctionDeclaration : public NStatement
{
public:
	const bool local;
	const NIdentifier& type;
	const NIdentifier& id;
	VariableList arguments;
	NBlock& block;

	NFunctionDeclaration(const NIdentifier& type, const NIdentifier& id,
	                     const VariableList& arguments, NBlock& block, bool local) :
		local(local), type(type), id(id), arguments(arguments), block(block) { }

	Value* codeGen(CodeGenContext& context) override;
};
