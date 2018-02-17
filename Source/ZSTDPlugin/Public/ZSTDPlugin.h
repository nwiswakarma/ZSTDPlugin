/*
 
 -----
 
*/

#pragma once

#include "ModuleManager.h"

class FZSTDPlugin : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

