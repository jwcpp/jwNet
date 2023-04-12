#pragma once

#include <tuple>

struct PostBase
{
	virtual void call() = 0;
};

template<typename FUNC>
struct PostNone : public PostBase
{
	PostNone(FUNC&& func) :
		m_func(std::forward<FUNC>(func)) {
	}
	virtual void call() {
		(m_func)();
	}

	FUNC m_func;
};

template<typename FUNC, typename... ARGS>
struct PostArge : public PostBase
{
public:
	PostArge(FUNC&& func, ARGS &&...args) :
		m_func(std::forward<FUNC>(func)),
		m_tuple(std::forward<ARGS>(args)...) {
	}
	virtual void call() {
		std::apply(std::move(m_func), std::move(m_tuple));
	}

private:
	FUNC m_func;
	std::tuple<ARGS...> m_tuple;
};

#if 0
template<typename FUNC, typename... ARGS>
void post(FUNC&& func, ARGS &&...args)
{
	auto ptr = new PostArge<FUNC, ARGS...>(std::forward<FUNC>(func), std::forward<ARGS>(args)...);
	ptr->call();

	delete ptr;
}

template<typename FUNC>
void post(FUNC&& func)
{
	auto ptr = new PostNone<FUNC>(std::forward<FUNC>(func));
	ptr->call();

	delete ptr;
}

#endif