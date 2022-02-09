#pragma once

/**
 * concepts to distinguish what approach we should take to acquire event source
 */
template <class T, class E = T::Event>
concept HasEvent = requires
{
  {
    T::GetEventSource()
    } -> std::same_as<RE::BSTEventSource<E>*>;
};

template <class T, class E>
concept SingletonSource = requires
{
  {
    T::GetSingleton()
    } -> std::convertible_to<RE::BSTEventSource<E>*>;
};

template <class E>
RE::BSTEventSource<E>* GetEventSource()
{
  return RE::ScriptEventSourceHolder::GetSingleton()->GetEventSource<E>();
}

template <class T, class E = T::Event>
requires HasEvent<T> RE::BSTEventSource<E>
*GetEventSource()
{
  return T::GetEventSource();
}

template <class T, class E>
requires SingletonSource<T, E> RE::BSTEventSource<E>
*GetEventSource()
{
  return T::GetSingleton();
}
