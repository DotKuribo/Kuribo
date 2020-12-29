#include "Project.hxx"
#include "io/io.hxx"
#include "modules/SymbolManager.hxx"

namespace kuribo {

ModuleInstance::ModuleInstance(mem::shared_ptr<IModule> m)
    : pModule(eastl::move(m)) {
  configure();

  {
    CanaryObject<__kuribo_simple_meta_v0> meta;
    mInterop.udata.fillin_meta = &meta.get();

    if (moduleCall(KURIBO_REASON_INQUIRE_META_DESC, &mInterop) &&
        meta.valid()) {
      KURIBO_PRINTF("~~~~~~~~~~~~~~~~~~~~~~~\n");
      KURIBO_PRINTF("[KURIBO] Loading module\n");
      KURIBO_PRINTF("         Name:     \t%s\n", meta->module_name);
      KURIBO_PRINTF("         Author:   \t%s\n", meta->module_author);
      KURIBO_PRINTF("         Version:  \t%s\n", meta->module_version);
      KURIBO_PRINTF("                     \n");
      KURIBO_PRINTF("         Built:    \t%s\n", meta->build_date);
      KURIBO_PRINTF("         Compiler: \t%s\n", meta->compiler_name);
      KURIBO_PRINTF("~~~~~~~~~~~~~~~~~~~~~~~\n");
    } else if (meta.valid()) {
      KURIBO_PRINTF("..Failed to inquire module info.\n");
    }
  }
  
  mInterop.udata.fillin_meta = nullptr;

  attach();
}
ModuleInstance::~ModuleInstance() {
  KURIBO_SCOPED_LOG("Module: Destruction");
  detach();
}
bool ModuleInstance::configured() const {
  return mInterop.register_procedure != nullptr;
}
bool ModuleInstance::configure() {
  // Check if already configured
  if (configured())
    return false;

  mInterop.core_version = KURIBO_CORE_VERSION;
  mInterop.register_procedure = kxRegisterProcedure;
  mInterop.get_procedure = kxGetProcedure;
  return true;
}

bool ModuleInstance::attach() {
  KURIBO_SCOPED_LOG("Module: Attaching");
  return moduleCall(KURIBO_REASON_LOAD);
}
bool ModuleInstance::detach() {
  KURIBO_SCOPED_LOG("Module: Detatching");
  return moduleCall(KURIBO_REASON_UNLOAD);
}
bool ModuleInstance::reload() {
  KURIBO_SCOPED_LOG("Module: Reloading");
  return detach() && attach();
}
// When loading a new version
bool ModuleInstance::transitionTo(mem::shared_ptr<IModule> pOther) {
  if (!configured())
    return false;

  if (!detach())
    return false;
  KURIBO_ASSERT(pModule.get());
  pModule.reset();

  pModule = std::move(pOther);
  return attach();
}
bool ModuleInstance::moduleCall(__KReason t) {
  if (!configured())
    return false;

  KURIBO_SCOPED_LOG("Calling module prologue");
  pModule->prologue(t, &mInterop.get());
  mInterop.check();
  return true;
}
} // namespace kuribo