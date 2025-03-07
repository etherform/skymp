#include "ConsoleApi.h"
#include "InGameConsolePrinter.h"
#include "InvalidArgumentException.h"
#include "NullPointerException.h"
#include "SkyrimPlatform.h"
#include "WindowsConsolePrinter.h"

namespace {
// TODO: Add printers switching
static std::shared_ptr<IConsolePrinter> g_printer(new InGameConsolePrinter);
}

namespace {
struct ConsoleComand
{
  std::string longName;
  std::string shortName;
  uint16_t numArgs = 0;
  RE::SCRIPT_FUNCTION::Execute_t* execute;
  JsValue jsExecute;
  RE::SCRIPT_FUNCTION* myIter;
  RE::SCRIPT_FUNCTION myOriginalData;
};
static std::map<std::string, ConsoleComand> replacedConsoleCmd;
static bool printConsolePrefixesEnabled = true;

bool IsNameEqual(const std::string& first, const std::string& second)
{
  return first.size() > 0 && second.size() > 0
    ? stricmp(first.data(), second.data()) == 0
    : false;
}
}

JsValue ConsoleApi::PrintConsole(const JsFunctionArguments& args)
{
  g_printer->Print(args);
  return JsValue::Undefined();
}

void ConsoleApi::Clear()
{
  for (auto& item : replacedConsoleCmd) {
    REL::safe_write((uintptr_t)item.second.myIter,
                    &(item.second.myOriginalData),
                    sizeof(item.second.myOriginalData));
  }

  replacedConsoleCmd.clear();
}

const char* ConsoleApi::GetScriptPrefix()
{
  return printConsolePrefixesEnabled ? "[Script] " : "";
}

const char* ConsoleApi::GetExceptionPrefix()
{
  return printConsolePrefixesEnabled ? "[Exception] " : "";
}

namespace {
ConsoleComand FillCmdInfo(RE::SCRIPT_FUNCTION* cmd)
{
  ConsoleComand cmdInfo;

  cmdInfo.longName = cmd->functionName;
  cmdInfo.shortName = cmd->shortName;
  cmdInfo.numArgs = cmd->numParams;
  cmdInfo.execute = cmd->executeFunction;
  cmdInfo.myIter = cmd;
  cmdInfo.myOriginalData = *cmd;
  cmdInfo.jsExecute = JsValue::Function(
    [](const JsFunctionArguments& args) { return JsValue::Bool(true); });

  return cmdInfo;
}

void CreateLongNameProperty(JsValue& obj, ConsoleComand* replaced)
{
  obj.SetProperty(
    "longName",
    [=](const JsFunctionArguments& args) {
      return JsValue::String(replaced->myIter->functionName);
    },
    [=](const JsFunctionArguments& args) {
      replaced->longName = args[1].ToString();

      RE::SCRIPT_FUNCTION cmd = *replaced->myIter;
      cmd.functionName = replaced->longName.c_str();

      REL::safe_write((uintptr_t)replaced->myIter, &cmd, sizeof(cmd));
      return JsValue::Undefined();
    });
}

void CreateShortNameProperty(JsValue& obj, ConsoleComand* replaced)
{
  obj.SetProperty(
    "shortName",
    [=](const JsFunctionArguments& args) {
      return JsValue::String(replaced->myIter->shortName);
    },
    [=](const JsFunctionArguments& args) {
      replaced->shortName = args[1].ToString();

      RE::SCRIPT_FUNCTION cmd = *replaced->myIter;
      cmd.shortName = replaced->shortName.c_str();

      REL::safe_write((uintptr_t)replaced->myIter, &cmd, sizeof(cmd));
      return JsValue::Undefined();
    });
}

void CreateNumArgsProperty(JsValue& obj, ConsoleComand* replaced)
{
  obj.SetProperty(
    "numArgs",
    [=](const JsFunctionArguments& args) {
      return JsValue::Double(replaced->myIter->numParams);
    },
    [=](const JsFunctionArguments& args) {
      replaced->numArgs = (double)args[1];

      RE::SCRIPT_FUNCTION cmd = *replaced->myIter;
      cmd.numParams = replaced->numArgs;

      REL::safe_write((uintptr_t)replaced->myIter, &cmd, sizeof(cmd));
      return JsValue::Undefined();
    });
}

void CreateExecuteProperty(JsValue& obj, ConsoleComand* replaced)
{
  obj.SetProperty("execute", nullptr, [=](const JsFunctionArguments& args) {
    replaced->jsExecute = args[1];
    return JsValue::Undefined();
  });
}
}

namespace {
struct ParseCommandResult
{
  std::string commandName;
  std::vector<std::string> params;
};

ParseCommandResult ParseCommand(std::string comand)
{
  ParseCommandResult res;
  static const std::string delimiterComa = ".";
  static const std::string delimiterSpase = " ";
  std::string token;

  size_t pos = comand.find(delimiterComa);
  if (pos != std::string::npos) {
    comand.erase(0, pos + delimiterComa.length());
  }

  while ((pos = comand.find(delimiterSpase)) != std::string::npos) {

    token = comand.substr(0, pos);
    if (res.commandName.empty()) {
      res.commandName = token;
    } else {
      res.params.push_back(token);
    }
    comand.erase(0, pos + delimiterSpase.length());
  }

  if (comand.size() >= 1)
    res.params.push_back(comand);

  return res;
}

JsValue GetObject(const std::string& param)
{
  if (auto formByEditorId = RE::TESForm::LookupByEditorID(param))
    return JsValue::Double(formByEditorId->formID);

  auto id = strtoul(param.c_str(), nullptr, 16);

  if (auto formById = RE::TESForm::LookupByID(id))
    return JsValue::Double(formById->formID);

  auto err = "For param: " + param + " formId and editorId was not found";
  throw std::runtime_error(err.data());
}

JsValue GetTypedArg(RE::SCRIPT_PARAM_TYPE type, std::string param)
{
  switch (type) {
    case RE::SCRIPT_PARAM_TYPE::kStage:
    case RE::SCRIPT_PARAM_TYPE::kInt:
      return JsValue::Double((double)strtoll(param.c_str(), nullptr, 10));

    case RE::SCRIPT_PARAM_TYPE::kFloat:
      return JsValue::Double((double)strtod(param.c_str(), nullptr));

    case RE::SCRIPT_PARAM_TYPE::kCoontainerRef:
    case RE::SCRIPT_PARAM_TYPE::kInvObjectOrFormList:
    case RE::SCRIPT_PARAM_TYPE::kSpellItem:
    case RE::SCRIPT_PARAM_TYPE::kInventoryObject:
    case RE::SCRIPT_PARAM_TYPE::kPerk:
    case RE::SCRIPT_PARAM_TYPE::kActorBase:
    case RE::SCRIPT_PARAM_TYPE::kObjectRef:
      return JsValue::Double((double)strtoul(param.c_str(), nullptr, 16));

    case RE::SCRIPT_PARAM_TYPE::kAxis:
    case RE::SCRIPT_PARAM_TYPE::kActorValue:
    case RE::SCRIPT_PARAM_TYPE::kChar:
      return JsValue::String(param);

    default:
      return GetObject(param);
  }
}

bool ConsoleComand_Execute(const RE::SCRIPT_PARAMETER* paramInfo,
                           RE::SCRIPT_FUNCTION::ScriptData* scriptData,
                           RE::TESObjectREFR* thisObj,
                           RE::TESObjectREFR* containingObj,
                           RE::Script* scriptObj, RE::ScriptLocals* locals,
                           double& result, std::uint32_t& opcodeOffsetPtr)
{
  std::pair<const std::string, ConsoleComand>* iterator = nullptr;

  auto func = [&](int) {
    try {
      if (!scriptObj)
        throw NullPointerException("scriptObj");

      std::string comand = scriptObj->GetCommand();
      auto parseCommandResult = ParseCommand(comand);

      for (auto& item : replacedConsoleCmd) {
        if (IsNameEqual(item.second.longName,
                        parseCommandResult.commandName) ||
            IsNameEqual(item.second.shortName,
                        parseCommandResult.commandName)) {

          std::vector<JsValue> args;
          args.push_back(JsValue::Undefined());
          auto refr = reinterpret_cast<RE::TESObjectREFR*>(thisObj);

          refr ? args.push_back(JsValue::Double((double)refr->formID))
               : args.push_back(JsValue::Double(0));

          for (size_t i = 0; i < parseCommandResult.params.size(); ++i) {
            if (!paramInfo)
              break;

            JsValue arg = GetTypedArg(paramInfo[i].paramType.get(),
                                      parseCommandResult.params[i]);

            if (arg.GetType() == JsValue::Type::Undefined) {
              auto err = " typeId " +
                std::to_string((uint32_t)paramInfo[i].paramType.get()) +
                " not yet supported";

              throw std::runtime_error(err.data());
            }
            args.push_back(arg);
          }

          if (item.second.jsExecute.Call(args))
            iterator = &item;
          break;
        }
      }
    } catch (std::exception& e) {
      std::string what = e.what();
      SkyrimPlatform::GetSingleton().AddUpdateTask([what] {
        throw std::runtime_error(what + " (in ConsoleComand_Execute)");
      });
    }
  };

  SkyrimPlatform::GetSingleton().PushAndWait(func);
  if (iterator)
    iterator->second.execute(paramInfo, scriptData, thisObj, containingObj,
                             scriptObj, locals, result, opcodeOffsetPtr);
  return true;
}

JsValue FindComand(const std::string& comandName)
{
  auto commands = RE::SCRIPT_FUNCTION::GetFirstConsoleCommand();
  for (std::uint16_t i = 0;
       i < RE::SCRIPT_FUNCTION::Commands::kConsoleCommandsEnd; ++i) {
    RE::SCRIPT_FUNCTION* _iter = &commands[i];

    if (IsNameEqual(_iter->functionName, comandName) ||
        IsNameEqual(_iter->shortName, comandName)) {
      JsValue obj = JsValue::Object();

      auto& replaced = replacedConsoleCmd[comandName];
      replaced = FillCmdInfo(_iter);

      CreateLongNameProperty(obj, &replaced);
      CreateShortNameProperty(obj, &replaced);
      CreateNumArgsProperty(obj, &replaced);
      CreateExecuteProperty(obj, &replaced);

      RE::SCRIPT_FUNCTION cmd = *_iter;
      cmd.executeFunction = ConsoleComand_Execute;
      REL::safe_write((uintptr_t)_iter, &cmd, sizeof(cmd));
      return obj;
    }
  }
  return JsValue::Null();
}
}

JsValue ConsoleApi::FindConsoleComand(const JsFunctionArguments& args)
{
  auto comandName = args[1].ToString();

  JsValue res = FindComand(comandName);

  if (res.GetType() == JsValue::Type::Null)
    res = FindComand(comandName);

  return res;
}

JsValue ConsoleApi::WriteLogs(const JsFunctionArguments& args)
{
  auto pluginName = args[1].ToString();

  auto slashCount = std::count(pluginName.begin(), pluginName.end(), '/') +
    std::count(pluginName.begin(), pluginName.end(), '\\');
  if (slashCount > 0) {
    throw InvalidArgumentException("pluginName", pluginName);
  }

  static std::map<std::string, std::unique_ptr<std::ofstream>> g_m;

  if (!g_m[pluginName]) {
    g_m[pluginName] = std::make_unique<std::ofstream>(
      "Data\\Platform\\Logs\\" + pluginName + "-logs.txt");
  }

  std::string s;

  for (size_t i = 2; i < args.GetSize(); ++i) {
    JsValue str = args[i];
    if (args[i].GetType() == JsValue::Type::Object &&
        !args[i].GetExternalData()) {

      JsValue json = JsValue::GlobalObject().GetProperty("JSON");
      str = json.GetProperty("stringify").Call({ json, args[i] });
    }
    s += str.ToString() + ' ';
  }

  (*g_m[pluginName]) << s << std::endl;
  return JsValue::Undefined();
}

JsValue ConsoleApi::SetPrintConsolePrefixesEnabled(
  const JsFunctionArguments& args)
{
  printConsolePrefixesEnabled = static_cast<bool>(args[1]);
  return JsValue::Undefined();
}
