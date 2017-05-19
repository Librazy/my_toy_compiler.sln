#include <iostream>
#include "codegen.h"
#include "node.h"
#include <llvm/Support/TargetSelect.h>

using namespace std;

extern int yyparse();
extern NBlock* programBlock;

void createCoreFunctions(CodeGenContext& context);

int main()
{
	yyparse();
	clog << programBlock << endl;
	InitializeNativeTarget();
	InitializeNativeTargetAsmPrinter();
	InitializeNativeTargetAsmParser();
	CodeGenContext context;
	createCoreFunctions(context);
	context.generateCode(*programBlock);
	auto val = context.runCode();
	clog << val.IntVal.getSExtValue() << endl;
	return 0;
}
