#pragma once
#include "pch.h"
#include "Core.h"

namespace rwd {

	#define BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)

	class Event { };

	template<typename T>
	class EventHandler {
	public:

		using CallbackFn = std::function<void(T&)>;

		void Subscribe(CallbackFn callback) {
			callbacks.push_back(callback);
		}

		void Dispatch(const Event& event) const {
			for (const auto& callback : callbacks) {
				callback(*(T*)&event);
			}
		}

	private:
		std::vector<CallbackFn> callbacks;
	};

	//-------------------------------------------------------------------------
	//
	// Window Event Classes
	//
	//-------------------------------------------------------------------------

	class WindowResizeEvent : public Event {
	public:
		i32 width, height;
	};

	class WindowCloseEvent : public Event { };

	//-------------------------------------------------------------------------
	//
	// Event Handlers
	//
	//-------------------------------------------------------------------------

	inline EventHandler<WindowResizeEvent> windowResizeEventHandler;
	inline EventHandler<WindowCloseEvent> windowCloseEventHandler;

}
