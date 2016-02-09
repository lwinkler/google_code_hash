#include <iostream>
#include <fstream>
#include <vector>
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

#include "boost/multi_array.hpp"


using namespace std;
using namespace boost;

/// Class to create a pool of threads: do not change
class ThreadPool {
	public:
		ThreadPool(size_t);
		template<class F, class... Args> auto enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>;
		~ThreadPool();
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


struct Point
{
	int x = 0;
	int y = 0;
};


/// Class representing our main test case
class TestCase{

	public:
		// Data of the problem
		int rows;
		int cols;
		int altitudes;
		int n_targets;
		int radius;
		int n_ballons;
		int n_turns;

		vector<Point> targets;
		multi_array<char, 3> wind;



	public:
		TestCase(): wind(boost::extents[8][70][300]){}
		~TestCase(){};
		void doWork();
		friend ostream & operator << (ostream & os,TestCase &t);
		friend istream & operator >> (istream & is,TestCase &t);

	private:
		bool started = false;
};


/// Implement here the main work of the test case
void TestCase::doWork()
{
}

/// Write the result to file
ostream & operator << (ostream & os,TestCase &t){
	// correspondances

	return os;
}

istream & operator >> (istream & is,TestCase &t){
	// Read input
	is >>t.rows;
	is >>t.cols;
	is >>t.altitudes;
	is >>t.n_targets;
	is >>t.radius;
	is >>t.n_ballons;
	is >>t.n_turns;

	for(int i = 0; i < t.n_targets ; i++)
	{
		Point pt;
		is >> pt.x;
		is >> pt.y;
		t.targets.push_back(pt);
	}
	assert(t.altitudes==8);
	assert(t.rows == 70);
	assert(t.cols == 300);

	for(int i = 0 ; i < t.altitudes ; i++)
	for(int j = 0 ; j < t.rows ; j++)
	for(int k = 0 ; k < t.cols ; k++)
	{
		is >> t.wind[i][j][k];
	}

	return is;
}


/*
void *doWork(void *threadArg){
	TestCase *tmp;
	tmp = reinterpret_cast<TestCase *>(threadArg);
	tmp->doWork();
}
*/

int main(int argc, char **argv){
	vector<TestCase *> tarr;
	
	assert(argc > 1);
	string filename(argv[1]);

	// open in and out files
	ifstream fin;
	ofstream fout;
	fin.open (filename.c_str());
	filename += ".out";
	fout.open (filename.c_str());

	int gN = 1;
	
	// read params
	// fin>>gN;


	// HACK for getline
	// string a;
	// getline(fin,a);

	TestCase *tptr;
	for(int i=0;i<gN;i++){
		tptr = new TestCase();
		fin >> *tptr;
		tarr.push_back(tptr);
	}

#ifdef OLD
	vector<pthread_t> mythreads(gN);
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_JOINABLE);
	void *status;
	int rc;

	int simthreads = argc > 2 ? atoi(argv[2]) : 1;

	cout << "Threads: " <<simthreads << endl;

	if(simthreads == 0 || gN % simthreads != 0)
	{
		cerr << "simthreads must be a divisor of " << gN << endl;
		exit(1);
	}
	
	for(int i=0;i<gN;i+=simthreads)
	{
		for(int j=i;j<i+simthreads;j++)
		{
			rc = pthread_create(&mythreads[j],&attr,doWork, (void *) tarr[j]);
			//forAllTestCases(gN,tarr,&try1);

			//print the output
		}
		for(int j=i;j<i+simthreads;j++)
		{
			rc = pthread_join(mythreads[j],&status);
			fout<< "Case #"<<(j+1)<<": "<<*tarr[j]<<endl;
		}
	}



	pthread_attr_destroy(&attr);
#else
	int max_threads = std::thread::hardware_concurrency();
	if(argc > 2 && atoi(argv[2]) > 0)
		max_threads =  atoi(argv[2]);
	cout << "Create a pool of " << max_threads << " working threads" << endl;
	ThreadPool pool(max_threads);
	std::vector< std::future<string> > results;

	for(auto& elem : tarr)
	{
		results.emplace_back(
			pool.enqueue([&elem]{
				cout << "starting job" << endl;
				elem->doWork();
				stringstream ss;
				ss<< "Case --> " << *elem <<endl;
				return ss.str();
			})
		);
	}

	for(auto && result: results)
		std::cout << result.get() << ' ';
	std::cout << std::endl;

	// Write down results
	int j = 0;
	for(auto& elem : tarr)
	{
		fout<< "Case #"<<(j+1)<<": "<<*elem<<endl;
		j++;
	}
#endif

}

// the constructor just launches some amount of workers
inline ThreadPool::ThreadPool(size_t threads):stop(false)
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

// add new work item to the pool
	template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>
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
inline ThreadPool::~ThreadPool()
{
	{
		std::unique_lock<std::mutex> lock(queue_mutex);
		stop = true;
	}
	condition.notify_all();
	for(std::thread &worker: workers)
		worker.join();
}
