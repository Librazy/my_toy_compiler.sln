#include "node.hpp"
namespace LIL {
	NIdentifier::NIdentifier(std::string ident):ident(ident)
	{}

	NIdentifierList::NIdentifierList(std::vector<std::string> idents):idents(idents)
	{}

	NIdentifierList::NIdentifierList(std::string ident) : idents({ ident })
	{}

	NUsing::NUsing(std::vector<std::string> idents):idents(idents)
	{}

	NUsing::NUsing(NIdentifierList idents) : idents(idents.idents)
	{}

	NBlock::NBlock(Node* expr)
	{
		exprs.push_back(expr);
	}

	NBlock::NBlock()
	{ }
}
