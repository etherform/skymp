#include "EventManager.h"
#include "EventsApi.h"

/**
 * @brief If we create our own custom events
 * that don't have sinks and fire from within the code
 * we register them with nullptr SinkObj here.
 */
void EventManager::InitCustom()
{
  auto manager = GetSingleton();

  manager->EmplaceEvent("update");
  manager->EmplaceEvent("tick");
  manager->EmplaceEvent("browserMessage");
  manager->EmplaceEvent("consoleMessage");
  manager->EmplaceEvent("ipcMessage");

  // skse messages
  manager->EmplaceEvent("dataLoaded");
  manager->EmplaceEvent("inputLoaded");
  manager->EmplaceEvent("postLoad");
  manager->EmplaceEvent("postPostLoad");
  manager->EmplaceEvent("newGame");
  manager->EmplaceEvent("preLoadGame");
  manager->EmplaceEvent("postLoadGame");
  manager->EmplaceEvent("saveGame");
  manager->EmplaceEvent("deleteGame");

  logger::debug("Custom events initialized.");
}

/**
 * @brief Fetches collection of sinks, populates global event map with event
 * names, corresponding Sink class instance and empty callback collection,
 * which is being filled when JS plugin subscribes to an event.
 */
void EventManager::Init()
{
  if (const auto sinks = EventHandler::GetSingleton()->GetSinks()) {
    if (sinks->empty()) {
      return;
    }

    auto manager = GetSingleton();

    for (const auto& sink : *sinks) {
      for (const auto& event : sink->events) {
        std::vector<std::string_view> linkedEvents;

        std::copy_if(sink->events.begin(), sink->events.end(),
                     std::back_inserter(linkedEvents),
                     [&](const char* const s) { return s != event; });

        auto sinkObj = new SinkObject(sink, linkedEvents);
        manager->EmplaceEvent(event, new EventState(sinkObj));
      }
    }
  }

  logger::debug("Game events initialized.");
}

std::unique_ptr<EventHandle> EventManager::Subscribe(std::string eventName,
                                                     JsValue callback,
                                                     bool runOnce)
{
  // check if event is supported
  auto event = events[eventName];
  if (!event) {
    logger::critical("Subscription to event failed. "
                     "{} is not a valid argument for eventName",
                     eventName);
    throw InvalidArgumentException("eventName", eventName);
    return std::make_unique<EventHandle>(0, "");
  }

  // if sink for that event is not active activate it, duh
  if (event->sinkObj) {
    auto sink = event->sinkObj->sink;

    if (!sink->IsActive(sink)) {
      sink->Activate(sink);
      logger::debug("Activated sink for event {}", eventName);
    }
  }

  auto cb = new CallbackObject(callback, runOnce);
  auto uid = reinterpret_cast<uintptr_t>(cb);

  event->callbacks.emplace(uid, cb);

  logger::debug("Subscribed to event {}, callback uid {}", eventName, uid);

  return std::make_unique<EventHandle>(uid, eventName);
}

void EventManager::Unsubscribe(uintptr_t uid, std::string_view eventName)
{
  if (uid == 0) {
    logger::info("Unsubscribe attempt failed, invalid event handle");
    return;
  }

  // check for correct event
  auto event = events[eventName];
  if (!event) {
    logger::info("Unsubscribe attempt failed, event {} not found", eventName);
    return;
  }

  // check if callback with provided uid exists
  if (!event->callbacks.contains(uid)) {
    logger::info("Unsubscribe attempt failed, callback with uid {} for event "
                 "{} not found",
                 uid, eventName);
    return;
  }

  event->callbacks.erase(uid);

  logger::debug("Unsubscribed from event {}, callback uid {}", eventName, uid);

  // now we need to see if we can deactivate event sink

  // ignore this for custom events
  if (event->sinkObj) {
    // check if there are any other callbacks for this event
    if (event->callbacks.empty()) {
      logger::trace("No other callbacks found for event {}", eventName);
      auto sink = event->sinkObj->sink;
      // check if there are any linked events for this sink
      if (event->sinkObj->linkedEvents.empty()) {
        logger::trace("No linked events found for event {}", eventName);
        sink->Deactivate(sink);
        logger::debug("Deactivated sink for event {}", eventName);
      } else {
        logger::trace("Linked events found for event {}", eventName);
        // check if there are any callbacks for linked events
        auto sinkIsBusy = false;
        for (const auto& linkedEventName : event->sinkObj->linkedEvents) {
          logger::trace("Checking callbacks for linked event {}",
                        linkedEventName);
          if (!events[linkedEventName]->callbacks.empty()) {
            logger::trace("Found callbacks for linked event {}. Aborting sink "
                          "deactivation.",
                          linkedEventName);
            sinkIsBusy = true;
            break;
          }
        }

        if (!sinkIsBusy) {
          sink->Deactivate(sink);
          logger::debug("Deactivated sink for event {}", eventName);
        }
      }
    }
  }
}
