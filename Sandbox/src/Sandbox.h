#pragma once
#include <Redwood.h>

class Sandbox : public rwd::App {
public:
	Sandbox();
};

rwd::App* rwd::CreateApp() {
	return new Sandbox();
}