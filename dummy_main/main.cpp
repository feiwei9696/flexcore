
#include <iostream>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <unistd.h>

#include <threading/cyclecontrol.hpp>
#include <threading/parallelregion.hpp>
#include <ports/region_aware.hpp>

int main()
{

	std::cout << "Starting Dummy Solution\n";

	std::cout << "build up infrastructure \n";
	fc::thread::cycle_control thread_manager;
	auto first_region = std::make_shared<fc::parallel_region>("region one");

	auto tick_cycle = fc::thread::periodic_task(
			[&first_region]()
			{
				first_region->ticks.in_work()();
			},
			fc::thread::cycle_control::medium_tick);

	tick_cycle.out_switch_tick() >> first_region->ticks.in_switch_buffers();
	thread_manager.add_task(tick_cycle);

	std::cout << "start building connections\n";

	using time_point = fc::wall_clock::system::time_point;
	fc::event_out_port<time_point> source;
	first_region->ticks.work_tick()
			>> [&source](){ source.fire(fc::wall_clock::system::now()); };

	source >> [](time_point t){ return fc::wall_clock::system::to_time_t(t); }
		   >> [](time_t t) { std::cout << std::localtime(&t)->tm_sec << "\n"; };

	unsigned int count = 0;

	first_region->ticks.work_tick()
			>> [&count](){return count++;}
			>> [](int i) { std::cout << "counted ticks: " << i << "\n"; };


	//create a connection with region transition
	auto second_region = std::make_shared<fc::parallel_region>("region two");

	auto second_tick_cycle = fc::thread::periodic_task(
			[&second_region]()
			{
				second_region->ticks.in_work()();
			},
			fc::thread::cycle_control::slow_tick);

	second_tick_cycle.out_switch_tick() >> second_region->ticks.in_switch_buffers();
	thread_manager.add_task(second_tick_cycle);

	second_region->ticks.work_tick() >> [](){std::cout << "Zonk!\n";;};

	fc::region_aware<fc::event_out_port<std::string>> string_source(first_region);
	fc::region_aware<fc::event_in_port<std::string>> string_sink(second_region,
			[second_region](std::string in){std::cout << second_region->get_id().key << " received: " << in << "\n";});

	string_source >> string_sink;
	first_region->ticks.work_tick() >> [string_source, first_region]() mutable {string_source.fire("a magic string from " + first_region->get_id().key); };

	thread_manager.start();

	while (true)
		sleep(1);

	return 0;
}