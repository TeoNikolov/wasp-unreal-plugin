// Copyright "The Wallenberg AI, Autonomous Systems and Software Program". All Rights Reserved.

#include "WaspRuntime/Public/WaspRuntimeModule.h"

#define LOCTEXT_NAMESPACE "FWaspRuntimeModule"

void FWaspRuntimeModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FWaspRuntimeModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FWaspRuntimeModule, WaspRuntime)