#include <llvm/IR/Value.h>

class AValue {
public:
	llvm::Value *llvmValue;
	ClassInfo *clazz;
	bool isReadOnly;

	AValue(llvm::Value *llvmValue = NULL, ClassInfo *clazz = NULL, bool isReadOnly =
		false) {
		this->llvmValue = llvmValue;
		this->clazz = clazz;
		this->isReadOnly = isReadOnly;
	}
};
