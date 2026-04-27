// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyUSDTestsModule.h"

#include "MyUSDMemory.h"

class FMyUsdTestsModule : public IMyUsdTestsModule
{
public:
	virtual void StartupModule() override
	{
		LLM_SCOPE_BYTAG(Usd);
	}

	virtual void ShutdownModule() override
	{
	}
};

IMPLEMENT_MODULE_USD(FMyUsdTestsModule, MyUSDTests);
