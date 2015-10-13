#ifndef SRC_THREADING_CYCLECONTROL_HPP_
#define SRC_THREADING_CYCLECONTROL_HPP_

#include "parallelscheduler.hpp"
#include <clock/clock.hpp>

#include <vector>

namespace fc
{
namespace thread
{

/**
 * \brief class representing a periodic task with cycle rate
 */
struct periodic_task
{
	/**
	 * \brief Constructor taking a job and the cycle rate.
	 * \param job task which is to be executed every cycle
	 * \param rate cycle rate of the task in virtual time
	 */
	periodic_task(std::function<void(void)> job,
			chrono::virtual_clock::duration rate) :
				work_to_do(false),
				cycle_rate(rate),
				work(job)
	{
	}

	bool done() const { return !work_to_do; }
	void set_work_to_do(bool todo) { work_to_do = todo; }
	void operator()()
	{
		work_to_do = false;
		work();
	}
private:
	/// flag to check if work has already been executed this cycle.
	bool work_to_do;
	chrono::virtual_clock::duration cycle_rate;
	/// work to be done every cycle
	std::function<void(void)> work;
};

/**
 * \brief Controls timing and the execution of cyclic tasks in the scheduler.
 * Todo: allow to set virtual clock as control clock for replay as template parameter
 * todo: allow to set min_tick_length
 * todo: add functionality to cleanly stop the main loop
 */
class cycle_control
{
public:
	static constexpr chrono::wall_clock::steady::duration min_tick_length =
			chrono::wall_clock::steady::duration::min(); //todo specify correct time

	cycle_control() = default;

	/// starts the main loop
	void start();
	/// stops the main loop in all threads
	void stop();

	/// advances the clock by a single tick and executes all tasks for a single cycle.
	void work();

	/**
	 * \brief adds a new cyclic task.
	 * \post list of tasks is not empty
	 * todo: allow client code to specify cycle rate of task
	 */
	void add_task(periodic_task task);
private:
	/// contains the main loop, which is running as as long as it is not stopped
	void main_loop();
	void run_periodic_tasks();
	std::vector<periodic_task> tasks;
	parallel_scheduler scheduler;
	bool keep_working = false;
	std::condition_variable main_loop_control;
	std::mutex main_loop_mutex;
};

} /* namespace thread */
} /* namespace fc */

#endif /* SRC_THREADING_CYCLECONTROL_HPP_ */
