#ifndef __SOME_UTILS_HPP__
#define __SOME_UTILS_HPP__

#include <iostream>
#include <fstream>
#include <vector>
#include <list>
#include <map>
#include <pthread.h>
#include <assert.h>
#include <climits>
#include <cmath>
#include <iomanip>
#include <cstdio>
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <queue>

#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
#include <sstream>

using namespace std;

// Utility
const string timeStamp()
{
	time_t rawtime;
	time(&rawtime);
	const tm* timeinfo = localtime (&rawtime);

	char dd[20];
	strftime(dd, sizeof(dd), "%Y%m%d_%H%M%S", timeinfo);

	return string(dd);
}

/// Class to create a pool of threads: do not change
class ThreadPool {
	public:
		// the constructor just launches some amount of workers
		inline ThreadPool(size_t threads):stop(false)
		{
			for(size_t i = 0;i<threads;++i)
				workers.emplace_back(
						[this]
						{
						for(;;)
						{
						std::function<void()> task;

						{
						std::unique_lock<std::mutex> lock(this->queue_mutex);
						this->condition.wait(lock,
							[this]{ return this->stop || !this->tasks.empty(); });
						if(this->stop && this->tasks.empty())
						return;
						task = std::move(this->tasks.front());
						this->tasks.pop();
						}

						task();
						}
						}
						);
		}
		template<class F, class... Args> auto enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>
		{
			using return_type = typename std::result_of<F(Args...)>::type;

			auto task = std::make_shared< std::packaged_task<return_type()> >(
					std::bind(std::forward<F>(f), std::forward<Args>(args)...)
					);

			std::future<return_type> res = task->get_future();
			{
				std::unique_lock<std::mutex> lock(queue_mutex);

				// don't allow enqueueing after stopping the pool
				if(stop)
					throw std::runtime_error("enqueue on stopped ThreadPool");

				tasks.emplace([task](){ (*task)(); });
			}
			condition.notify_one();
			return res;
		}
		// the destructor joins all threads
		inline ~ThreadPool()
		{
			{
				std::unique_lock<std::mutex> lock(queue_mutex);
				stop = true;
			}
			condition.notify_all();
			for(std::thread &worker: workers)
				worker.join();
		}
	private:
		// need to keep track of threads so we can join them
		std::vector< std::thread > workers;
		// the task queue
		std::queue< std::function<void()> > tasks;

		// synchronization
		std::mutex queue_mutex;
		std::condition_variable condition;
		bool stop;
};


#endif
