#pragma once

extern rwd::App* rwd::CreateApp();

int main() {
	rwd::App* app = rwd::CreateApp();
	app->Run();
	delete app;
}
