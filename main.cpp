#include "codegen.h"
#include "node.h"
#include <llvm/Support/TargetSelect.h>

extern int yyparse();
extern NBlock* programBlock;

void createCoreFunctions(CodeGenContext& context);

int main(int argc, char *argv[])
{
	auto compileOnly = false;
	auto logLevel = 4;
	for(auto i = 0; i < argc; ++i) {
		if(std::string(argv[i]).compare("-c") == 0 || std::string(argv[i]).compare("--compile") == 0) {
			compileOnly = true;
		}
		if(std::string(argv[i]).find("--log") == 0) {
			if(argv[i][5] < '0' || argv[i][5] > '4') {
				std::cerr << "Bad level" << std::endl;
				exit(2);
			}
			logLevel = argv[i][5] - '0';
		}
	}
	yyparse();
	InitializeNativeTarget();
	InitializeNativeTargetAsmPrinter();
	InitializeNativeTargetAsmParser();
	CodeGenContext context;
	context.dclog.max_level = debug_stream::level(logLevel);
	createCoreFunctions(context);
	context.generateCode(*programBlock);
	if (!compileOnly) {
		auto val = context.runCode();
		(context.llclog << val.IntVal.getSExtValue() << "\n").flush();
	}
	return 0;
}
