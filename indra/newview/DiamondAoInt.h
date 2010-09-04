#ifndef DIAMONDAOINT_H
#define DIAMONDAOINT_H

#include "llviewerprecompiledheaders.h"

class DiamondAoInt
{
public:
	DiamondAoInt();
	~DiamondAoInt();
	static bool AOCommand(std::string message);
	static void AOStatusUpdate(bool status);
private:
	static S32 regchan;
};

#endif
