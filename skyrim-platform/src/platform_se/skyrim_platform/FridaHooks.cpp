#include "FridaHooks.h"
#include "EventsApi.h"
#include "FridaHookHandler.h"
#include "FridaHooksUtils.h"
#include "PapyrusTESModPlatform.h"
#include "StringHolder.h"

#include <hooks/DInputHook.hpp>
#include <ui/DX11RenderHandler.h>

/**
 * ConsoleVPrint hook
 */
void OnConsoleVPrintEnter(GumInvocationContext* ic)
{
  auto refr = ic->cpu_context->rdx ? (char*)ic->cpu_context->rdx : nullptr;

  if (refr)
    EventsApi::SendConsoleMsgEvent(refr);
}

void InstallConsoleVPrintHook()
{
  auto hook = new Frida::Hook(OnConsoleVPrintEnter, nullptr);
  REL::Relocation<std::uintptr_t> target{ REL::ID(51110) };
  Frida::HookHandler::GetSingleton()->Install(Frida::HookID::CONSOLE_VPRINT,
                                              target.address(), hook);
}

/**
 * Send Event hook
 */

// (VMHandle handle, const BSFixedString& eventName, IFunctionArguments* args)
void OnSendEventEnter(GumInvocationContext* ic)
{
  auto handle = (RE::VMHandle)gum_invocation_context_get_nth_argument(ic, 1);
  auto eventName = (char**)gum_invocation_context_get_nth_argument(ic, 2);

  auto vm = VM::GetSingleton();

  uint32_t selfId = 0;

  auto policy = vm->GetObjectHandlePolicy();
  if (policy) {
    if (auto actor =
          policy->GetObjectForHandle(RE::FormType::ActorCharacter, handle)) {
      selfId = actor->GetFormID();
    }
    if (auto refr =
          policy->GetObjectForHandle(RE::FormType::Reference, handle)) {
      selfId = refr->GetFormID();
    }
  }

  auto eventNameStr = std::string(*eventName);
  EventsApi::SendPapyrusEventEnter(selfId, eventNameStr);

  auto blockEvents = TESModPlatform::GetPapyrusEventsBlocked();
  if (blockEvents && strcmp(*eventName, "OnUpdate") != 0 && vm) {
    vm->attachedScriptsLock.Lock();
    auto it = vm->attachedScripts.find(handle);

    if (it != vm->attachedScripts.end()) {
      auto& scripts = it->second;

      for (size_t i = 0; i < scripts.size(); i++) {
        auto script = scripts[i].get();
        auto info = script->GetTypeInfo();
        auto name = info->GetName();

        const char* skyui_name = "SKI_"; // start skyui object name
        if (strlen(name) >= 4 && name[0] == skyui_name[0] &&
            name[1] == skyui_name[1] && name[2] == skyui_name[2] &&
            name[3] == skyui_name[3]) {
          blockEvents = false;
          break;
        }
      }
    }

    vm->attachedScriptsLock.Unlock();
  }

  if (blockEvents) {
    static const auto fsEmpty = new FixedString("");
    gum_invocation_context_replace_nth_argument(ic, 2, fsEmpty);
  }
}

void OnSendEventLeave(GumInvocationContext* ic)
{
  EventsApi::SendPapyrusEventLeave();
}

void InstallSendEventHook()
{
  auto hook = new Frida::Hook(OnSendEventEnter, OnSendEventLeave);
  REL::Relocation<std::uintptr_t> target{ REL::ID(104800) };
  Frida::HookHandler::GetSingleton()->Install(Frida::HookID::SEND_EVENT,
                                              target.address(), hook);
}

/**
 *  Draw Sheathe Weapon PC hook
 */
void OnDrawSheatheWeaponPcEnter(GumInvocationContext* ic)
{
  auto refr =
    ic->cpu_context->rcx ? (RE::Actor*)(ic->cpu_context->rcx) : nullptr;
  uint32_t formId = refr ? refr->formID : 0;

  union
  {
    size_t draw;
    uint8_t byte[8];
  };

  draw = (size_t)gum_invocation_context_get_nth_argument(ic, 1);

  auto falseValue = gpointer(*byte ? draw - 1 : draw);
  auto trueValue = gpointer(*byte ? draw : draw + 1);

  auto mode = TESModPlatform::GetWeapDrawnMode(formId);
  if (mode == TESModPlatform::WEAP_DRAWN_MODE_ALWAYS_TRUE) {
    gum_invocation_context_replace_nth_argument(ic, 1, trueValue);
  } else if (mode == TESModPlatform::WEAP_DRAWN_MODE_ALWAYS_FALSE) {
    gum_invocation_context_replace_nth_argument(ic, 1, falseValue);
  }
}

void InstallDrawSheatheWeaponPcHook()
{
  auto hook = new Frida::Hook(OnDrawSheatheWeaponPcEnter, nullptr);
  REL::Relocation<std::uintptr_t> target{ REL::ID(41235) };
  Frida::HookHandler::GetSingleton()->Install(
    Frida::HookID::DRAW_SHEATHE_WEAPON_PC, target.address(), hook);
}

/**
 * Draw Sheathe Weapon Actor hook
 */
void OnDrawSheatheWeaponActorEnter(GumInvocationContext* ic)
{
  auto refr =
    ic->cpu_context->rcx ? (RE::Actor*)(ic->cpu_context->rcx) : nullptr;
  uint32_t formId = refr ? refr->formID : 0;

  auto draw = (uint32_t*)gum_invocation_context_get_nth_argument(ic, 1);

  auto mode = TESModPlatform::GetWeapDrawnMode(formId);
  if (mode == TESModPlatform::WEAP_DRAWN_MODE_ALWAYS_TRUE) {
    gum_invocation_context_replace_nth_argument(ic, 1, gpointer(1));
  } else if (mode == TESModPlatform::WEAP_DRAWN_MODE_ALWAYS_FALSE) {
    gum_invocation_context_replace_nth_argument(ic, 1, gpointer(0));
  }
}

void InstallDrawSheatheWeaponActorHook()
{
  auto hook = new Frida::Hook(OnDrawSheatheWeaponActorEnter, nullptr);
  REL::Relocation<std::uintptr_t> target{ REL::ID(37279) };
  Frida::HookHandler::GetSingleton()->Install(
    Frida::HookID::DRAW_SHEATHE_WEAPON_ACTOR, target.address(), hook);
}

/**
 * Send Animation Event hook
 */
void OnSendAnimationEventEnter(GumInvocationContext* ic)
{
  auto refr = ic->cpu_context->rcx
    ? (RE::TESObjectREFR*)(ic->cpu_context->rcx - 0x38)
    : nullptr;
  uint32_t formId = refr ? refr->formID : 0;

  constexpr int argIdx = 1;
  auto animEventName =
    (char**)gum_invocation_context_get_nth_argument(ic, argIdx);

  if (!refr || !animEventName)
    return;

  std::string str = *animEventName;
  EventsApi::SendAnimationEventEnter(formId, str);
  if (str != *animEventName) {
    auto fs =
      const_cast<RE::BSFixedString*>(&StringHolder::ThreadSingleton()[str]);
    auto newAnimEventName = reinterpret_cast<char**>(fs);
    gum_invocation_context_replace_nth_argument(ic, argIdx, newAnimEventName);
  }
}

void OnSendAnimationEventLeave(GumInvocationContext* ic)
{
  bool res = !!gum_invocation_context_get_return_value(ic);
  EventsApi::SendAnimationEventLeave(res);
}

void InstallSendAnimationEventHook()
{
  auto hook =
    new Frida::Hook(OnSendAnimationEventEnter, OnSendAnimationEventLeave);
  REL::Relocation<std::uintptr_t> target{ REL::ID(38048) };
  Frida::HookHandler::GetSingleton()->Install(
    Frida::HookID::HOOK_SEND_ANIMATION_EVENT, target.address(), hook);
}

/**
 * Queue Ninode Update hook
 */
thread_local uint32_t g_queueNiNodeActorId = 0;

void OnQueueNinodeUpdateEnter(GumInvocationContext* ic)
{
  auto refr = ic->cpu_context->rcx ? (RE::TESObjectREFR*)(ic->cpu_context->rcx)
                                   : nullptr;

  uint32_t id = refr ? refr->formID : 0;

  g_queueNiNodeActorId = id;
}

void InstallQueueNinodeUpdateHook()
{
  auto hook = new Frida::Hook(OnQueueNinodeUpdateEnter, nullptr);
  REL::Relocation<std::uintptr_t> target{ REL::ID(40255) };
  Frida::HookHandler::GetSingleton()->Install(
    Frida::HookID::QUEUE_NINODE_UPDATE, target.address(), hook);
}

/**
 * Apply Masks To Render Targets hook
 */
void OnApplyMasksToRenderTargetsEnter(GumInvocationContext* ic)
{
  if (g_queueNiNodeActorId > 0) {
    auto tints = TESModPlatform::GetTintsFor(g_queueNiNodeActorId);
    if (tints) {
      gum_invocation_context_replace_nth_argument(ic, 0, tints.get());
    }
  }

  g_queueNiNodeActorId = 0;
}

void InstallApplyMasksToRenderTargetsHook()
{
  auto hook = new Frida::Hook(OnApplyMasksToRenderTargetsEnter, nullptr);
  REL::Relocation<std::uintptr_t> target{ REL::ID(27040) };
  Frida::HookHandler::GetSingleton()->Install(
    Frida::HookID::APPLY_MASKS_TO_RENDER_TARGET, target.address(), hook);
}

/**
 * Render Cursor Menu hook
 */
bool g_allowHideCursorMenu = true;
bool g_transparentCursor = false;

void OnRenderCursorMenuEnter(GumInvocationContext* ic)
{
  auto menu = FridaHooksUtils::GetMenuByName(RE::CursorMenu::MENU_NAME);
  auto this_ = (int64_t*)ic->cpu_context->rcx;
  if (!this_ || !g_allowHideCursorMenu || this_ != menu)
    return;

  auto& visibleFlag = CEFUtils::DX11RenderHandler::Visible();
  auto& focusFlag = CEFUtils::DInputHook::ChromeFocus();
  if (visibleFlag && focusFlag) {
    if (!g_transparentCursor) {
      if (FridaHooksUtils::SetMenuNumberVariable(
            RE::CursorMenu::MENU_NAME, "_root.mc_Cursor._alpha", 0)) {
        g_transparentCursor = true;
      }
    }
  } else {
    if (g_transparentCursor) {
      if (FridaHooksUtils::SetMenuNumberVariable(
            RE::CursorMenu::MENU_NAME, "_root.mc_Cursor._alpha", 100)) {
        g_transparentCursor = false;
      }
    }
  }
}

void InstallRenderCursorMenuHook()
{
  auto hook = new Frida::Hook(OnRenderCursorMenuEnter, nullptr);
  REL::Relocation<std::uintptr_t> target{ REL::ID(33632) };
  Frida::HookHandler::GetSingleton()->Install(
    Frida::HookID::RENDER_CURSOR_MENU, target.address(), hook);
}

void Frida::InstallHooks()
{
  InstallConsoleVPrintHook();
  InstallSendEventHook();
  InstallDrawSheatheWeaponPcHook();
  InstallDrawSheatheWeaponActorHook();
  InstallSendAnimationEventHook();
  InstallQueueNinodeUpdateHook();
  InstallRenderCursorMenuHook();

  logger::info("Frida hooks installed.");
}
