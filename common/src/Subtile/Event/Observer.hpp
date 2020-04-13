#pragma once

#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <optional>
#include "Listener.hpp"

namespace Subtile {

class Instance;

namespace Event {

class Observer
{
public:
	class Cluster;
	template <typename... Types>
	class Group;

	virtual ~Observer(void) = default;

private:
	friend Instance;
	virtual void update(void) = 0;
};

template <typename ObserverType, typename ...ArgsType>
struct Descriptor
{
	Descriptor(ObserverType &observer, ArgsType &&...args) :
		observer(observer),
		args(std::forward_as_tuple<ArgsType>(args)...)
	{
	}
	~Descriptor(void) = default;

	ObserverType &observer;
	const std::tuple<ArgsType...> args;
};

template <typename ObserverType>
class DescGen
{
public:
	DescGen(void) = default;
	~DescGen(void) = default;

	template <typename ...ArgsTypes>
	Descriptor<ObserverType, ArgsTypes...> operator()(ArgsTypes &&...args)
	{
		return Descriptor<ObserverType, ArgsTypes...>(getObserver(), std::forward<ArgsTypes...>(args)...);
	}

private:
	ObserverType& getObserver(void)
	{
		return static_cast<ObserverType&>(*this);
	}
};

class Socket;

template <typename ObserverType, template <typename...> class GroupingType, typename... RequestTypes, typename... StoreTypes, typename... ReturnTypes>
class Observer::Group<ObserverType, GroupingType<RequestTypes...>, GroupingType<StoreTypes...>, GroupingType<ReturnTypes...>> : public Observer
{
public:
	using ConverterType = std::function<std::tuple<StoreTypes...> (const RequestTypes &...)>;
	using UpdaterType = std::function<std::optional<std::tuple<ReturnTypes...>> (const StoreTypes &...)>;
	using CallbackType = std::function<void (const ReturnTypes &...)>;

	Group(const ConverterType &converter, const UpdaterType &updater) :
		m_converter(converter),
		m_updater(updater)
	{
		getObserver().add(*this);
	}
	~Group(void) override
	{
	}

private:
	class Listener : Event::Listener
	{
	public:
		Listener(Group &group, const std::tuple<StoreTypes...> &request, const CallbackType &callback) :
			m_group(group),
			m_request(request),
			m_callback(callback)
		{
		}
		~Listener(void) override
		{
			m_group.unlisten(*this);
		}

	private:
		friend Group;
		Group &m_group;
		const std::tuple<StoreTypes...> m_request;
		const CallbackType m_callback;
	};

	friend Listener;
	const ConverterType m_converter;
	const UpdaterType m_updater;
	std::map<std::tuple<StoreTypes...>, std::map<Listener*, Listener&>> m_listeners;

	friend Event::Socket;
	std::unique_ptr<Event::Listener> listen(const std::tuple<RequestTypes...> &request, const CallbackType &callback)
	{
		auto res = new Listener(*this, m_converter ? m_converter(std::get<RequestTypes>(request)...) : request, callback);

		m_listeners[res->m_request].emplace(res, *res);
		return std::unique_ptr<Event::Listener>(res);
	}

	void unlisten(Listener &listener)
	{
		auto got = m_listeners.find(listener.m_request);
		if (got != m_listeners.end()) {
			auto &got_map = got->second;
			auto l = got_map.find(&listener);
			if (l != got_map.end()) {
				got_map.erase(l);
				if (got_map.size() == 0)
					m_listeners.erase(got);
			} else
				throw std::runtime_error("Can't unlisten");
		} else
			throw std::runtime_error("Can't unlisten");
	}

	void update(void) override
	{
		for (auto &p : m_listeners) {
			auto res = m_updater(std::get<RequestTypes>(p.first)...);
			if (res)
				for (auto &l : p.second)
					l.second.m_callback(std::get<ReturnTypes>(*res)...);
		}
	}

	ObserverType& getObserver(void)
	{
		return static_cast<ObserverType&>(*this);
	}
};

template <typename ObserverType, template <typename...> class GroupingType, typename... RequestTypes, typename... ReturnTypes>
class Observer::Group<ObserverType, GroupingType<RequestTypes...>, GroupingType<ReturnTypes...>> : public Observer::Group<ObserverType, GroupingType<RequestTypes...>, GroupingType<RequestTypes...>, GroupingType<ReturnTypes...>>
{
public:
	using UpdaterType = std::function<std::optional<std::tuple<ReturnTypes...>> (const RequestTypes &...)>;

	Group(const UpdaterType &updater) :
		Group<ObserverType, GroupingType<RequestTypes...>, GroupingType<RequestTypes...>, GroupingType<ReturnTypes...>>(nullptr, updater)
	{
	}

	~Group(void) override
	{
	}
};

class Observer::Cluster : public Observer
{
public:
	virtual ~Cluster(void) = default;

protected:
	void add(Observer &observer);

private:
	std::vector<std::reference_wrapper<Observer>> m_observers;

	template <typename... GroupingType>
	friend class Group;
	void update(void) override;
};

}
}

#include "Socket.hpp"