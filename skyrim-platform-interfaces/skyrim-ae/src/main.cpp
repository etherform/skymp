extern "C" {
DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse,
                                        SKSE::PluginInfo* a_info)
{
  auto path = logger::log_directory();
  if (!path) {
    return false;
  }

  *path /= "sp-interface-ae.log"sv;
  auto sink =
    std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);

  auto log = std::make_shared<spdlog::logger>("global log", std::move(sink));

  log->set_level(spdlog::level::info);
  log->flush_on(spdlog::level::info);

  spdlog::set_default_logger(std::move(log));
  spdlog::set_pattern("%g(%#): [%^%l%$] %v", spdlog::pattern_time_type::local);

  logger::info(FMT_STRING("{} v{}"), Version::PROJECT, Version::NAME);

  a_info->infoVersion = SKSE::PluginInfo::kVersion;
  a_info->name = "Skyrim Platform AE Interface";
  a_info->version = Version::MAJOR;

  if (a_skse->IsEditor()) {
    logger::critical("Loaded in editor, marking as incompatible"sv);
    return false;
  }

  return true;
}

DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
  logger::info("plugin loaded");

  SKSE::Init(a_skse);
  /* SKSE::AllocTrampoline(); do we need this? */

  /* Init Platform Impl here */

  /* handle SKSE Interfaces */
  const auto papyrusInterface = SKSE::GetPapyrusInterface();
  if (!papyrusInterface) {
    logger::critical("QueryInterface failed for PapyrusInterface");
    return false;
  }

  const auto taskInterface = SKSE::GetTaskInterface();
  if (!taskInterface) {
    logger::critical("QueryInterface failed for TaskInterface");
    return false;
  }

  const auto messagingInterface = SKSE::GetMessagingInterface();
  if (!messagingInterface) {
    logger::critical("QueryInterface failed for MessagingInterface");
    return false;
  }

  const auto serializationInterface = SKSE::GetSerializationInterface();
  if (!serializationInterface) {
    logger::critical("QueryInterface failed for SerializationInterface");
    return false;
  }

  return true;
}
}
