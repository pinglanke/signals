//g++ -std=c++11 test.cpp -o test -pthread
#include "signal.hpp"
#include <chrono>
#include <thread>

void do_nothing() {}

void hello_world_1(int id, const char* tag)
{
	std::cerr << "[" << id << "]" << " hellow world 1: [" << tag << "] good luck." << std::endl;
}

void hello_world_2(int id, const char* tag)
{
	std::cerr << "[" << id << "]" << " hellow world 2: [" << tag << "] good luck." << std::endl;
}

void hello_world_3(int id, const char* tag)
{
	std::cerr << "[" << id << "]" << " hellow world 3: [" << tag << "] good luck." << std::endl;
}

std::atomic_bool exit_flag1 {false};
std::atomic_bool exit_flag2 {false};

int main()
{
	plk::signal<void(int, const char*)> mysignal;
	std::thread t1 ([&mysignal] {
		while(!exit_flag1) {
			mysignal(1, "osx");
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	});
	std::thread t2 ([&mysignal] {
		while(!exit_flag2) {
			auto conn = mysignal.attach(hello_world_3);
			std::cerr << "hellow_world_3 attached." << std::endl;
			std::this_thread::sleep_for(std::chrono::seconds(2));
			conn.detach();
			std::cerr << "hellow_world_3 detached." << std::endl;
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	});

	mysignal.attach(hello_world_1);
	auto conn = mysignal.attach(hello_world_2);
	mysignal(0, "plk");
	conn.detach();
	std::cerr << "hellow_world_2 detached." << std::endl;
	mysignal(0, "plk");

	std::this_thread::sleep_for(std::chrono::seconds(30));
	exit_flag1 = true;
	std::this_thread::sleep_for(std::chrono::seconds(1));
	exit_flag2 = true;

	t1.join();
	t2.join();

	//test performance.
	void (*pf)(void) = &do_nothing;
	std::function<void (void)> f = do_nothing;
	plk::signal<void (void)> s;
	s.attach(&do_nothing);

	using seconds = std::chrono::duration<double>;
	seconds diff;

	auto begin = std::chrono::steady_clock::now();
	for(int i = 0; i < 10000000; ++i)
		pf();
	auto now = std::chrono::steady_clock::now();
	diff = now - begin;
	std::cout << "C function pointer call 1000w times:" << diff.count() << std::endl;

	begin = std::chrono::steady_clock::now();
	for(int i = 0; i < 10000000; ++i)
		f();
	now = std::chrono::steady_clock::now();
	diff = now - begin;
	std::cout << "std::function call 1000w times:" << diff.count() << std::endl;

	begin = std::chrono::steady_clock::now();
	for(int i = 0; i < 10000000; ++i)
		s();
	now = std::chrono::steady_clock::now();
	diff = now - begin;
	std::cout << "plk::signal call 1000w times:" << diff.count() << std::endl;

	return 0;
}

