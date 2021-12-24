#include "Hooks.h"

/**
 * On Frame Update hook
 */
uint64_t iterator = 0;

/**
 * This should hook into the child of WinMain
 * which should trigger per frame
 */
struct OnFrameUpdate
{
  static void thunk()
  {
    RE::ConsoleLog::GetSingleton()->Print("OnFrameUpdateHook iterator: %d",
                                          iterator);
    iterator++;
    func();
  };
  static inline REL::Relocation<decltype(&thunk)> func;
};

void InstallOnFrameUpdateHook()
{
  REL::Relocation<std::uintptr_t> target{ REL::ID(36564) };
  Hooks::write_thunk_call<OnFrameUpdate>(target.address() + 0x73);
}

void Hooks::Install()
{
  InstallOnFrameUpdateHook();
  logger::info("CommonLib hooks installed.");
}
