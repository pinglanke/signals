/*
 * Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 * SPDX-License-Identifier: MIT
 * Copyright (c) 2020 Ousixin(oyshixin@sina.com).
 * Permission is hereby  granted, free of charge, to any  person obtaining a copy
 * of this software and associated  documentation files (the "Software"), to deal
 * in the Software  without restriction, including without  limitation the rights
 * to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
 * copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
 * furnished to do so, subject to the following conditions:
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
 * IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
 * FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
 * AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
 * LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

#pragma once

#include <functional>
#include <mutex>
#include <list>
#include <unordered_map>
#include <cstdint>
#include <memory>
#include <atomic>

#define DEBUG_SIGNAL_SLOT

#if defined(DEBUG_SIGNAL_SLOT)
#include <iostream>
#endif

namespace plk {

template <typename> class signal;

template <typename R, typename ...Args>
class signal<R(Args...)>
{
	class destroy_guard final {
	public:
		void lock() { m_mutex.lock(); }
		void unlock() { m_mutex.unlock(); }
		bool is_alive() { return m_alive; }
	private:
		friend signal;
		destroy_guard() = default;
		destroy_guard(const destroy_guard& other) = delete;
		destroy_guard& operator=(const destroy_guard& other) = delete;
		void set_alive(bool alive) { std::unique_lock<std::mutex> l(m_mutex); m_alive = alive; }
	private:
		std::mutex m_mutex;
		std::atomic_bool m_alive { true };
	};
public:
	using slot_type = R(Args...);

	class connection final {
	public:
		connection() = default;
		connection(const connection& other) : m_id(other.m_id), m_alive(other.m_alive), m_signal(other.m_signal) {}
		bool is_valid()  { return m_signal ? true : false; }
		void detach()
		{
			if(!m_signal) {
#if defined(DEBUG_SIGNAL_SLOT)
				std::cerr << "The signal is null." << std::endl;
#endif
				return;
			}
			auto life_guard = m_alive.lock();
			if(!life_guard) {
#if defined(DEBUG_SIGNAL_SLOT)
				std::cerr << "The signal is already destroied. Case 1." << std::endl;
#endif
				return;
			}
			life_guard->lock();
			if(life_guard->is_alive()) m_signal->detach(*this);
#if defined(DEBUG_SIGNAL_SLOT)
			else std::cerr << "The signal is already destroied. Case 2." << std::endl;
#endif
			life_guard->unlock();
		}
		connection& operator=(const connection& other) { if(this == &other) return *this; m_id = other.m_id; m_alive = other.m_alive; m_signal = other.m_signal; return *this;}
	private:
		friend signal;
		connection(uint64_t id, std::shared_ptr<destroy_guard> life_guard, signal* sig) : m_id(id), m_alive(life_guard), m_signal(sig) {}
	private:
		uint64_t m_id = 0;
		std::weak_ptr<destroy_guard> m_alive;
		signal* m_signal = nullptr;
	};

private:
	using slot_iterator = typename std::list<std::function<slot_type>>::iterator;

public:
	signal() : m_alive(new destroy_guard()) {}
	signal(const signal& other) = delete;
	void operator=(const signal& other) = delete;
	~signal()
	{
		std::shared_ptr<destroy_guard> life_guard = m_alive;
		life_guard->set_alive(false);
		m_alive.reset();

		std::unique_lock<std::mutex> lock(m_mutex);
		m_slots.clear();
	}

	connection attach(slot_type slot)
	{
		slot_iterator sit;
		{
		std::unique_lock<std::mutex> lock(m_mutex);
		sit = m_slots.emplace(m_slots.end(), slot);
		}


		{
		std::unique_lock<std::mutex> lock(m_connect_mutex);
		connection conn(m_currId, m_alive, this);
		m_slotIterators[m_currId] = sit;
		m_currId ++;
		return conn;
		}

	}

	void detach(const connection& conn)
	{
		slot_iterator sit;

		if(conn.m_signal != this) return;

		{
		std::unique_lock<std::mutex> lock(m_connect_mutex);
		auto it = m_slotIterators.find(conn.m_id);
		if(it == m_slotIterators.end()) {
			return;
		}
		sit = it->second;
		m_slotIterators.erase(it);
		}

		{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_slots.erase(sit);
		}
	}

	void emit(Args... args) const
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		for(auto & slot : m_slots) {
			slot(args...);
		}

	}

	void operator () (Args... args) const
	{
		emit(args...);
	}

private:
	mutable std::mutex m_mutex;
	std::list<std::function<slot_type>> m_slots;
	std::shared_ptr<destroy_guard> m_alive;
	mutable std::mutex m_connect_mutex;
	std::unordered_map<uint64_t, slot_iterator> m_slotIterators;
	uint64_t m_currId = 0;
};

}

