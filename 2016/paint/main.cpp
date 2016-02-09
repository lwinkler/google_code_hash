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

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <boost/multi_array.hpp>


using namespace std;
using namespace cv;
using namespace boost;

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


/// Class representing our main test case
class TestCase{

	public:
		// Data of the problem
		int rows;
		int cols;

		// multi_array<char, 2>* pwall = nullptr;
		Mat wall;
		vector<string> result;



	public:
		TestCase() {}
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
	Mat painted(wall.size(), wall.type());
	painted.setTo(0);
	Rect rect_mat(0, 0, wall.cols, wall.rows);

	for(int s = (min(painted.rows, painted.cols) - 1) / 2 ; s >= 0 ; s--)
	{
		int area = (2*s+1) * (2*s+1);
		for(int i = 0 + s ; i < painted.rows - s; i++)
		{
			for(int j = 0 + s ; j < painted.cols - s; j++)
			{
				Rect rect(Point2i(j-s,i-s), Point2i(j+s + 1, i+s + 1));
				// cout << rect << endl;
				// cout << rect_mat << endl;
				// cout << i << " " << j << " " << s << endl;

				assert((rect & rect_mat) == rect);

				if(countNonZero(wall(rect)) == area && countNonZero(painted(rect)) != area)
				{
					painted(rect).setTo(255);
					stringstream ss;
					ss << "PAINT_SQUARE " << i - 1 << " " << j - 1 << " " << s;
					result.push_back(ss.str());
				}
			}
		}
	}

	namedWindow( "Wall", WINDOW_AUTOSIZE );
	imshow("Wall", painted);
	waitKey(0);
}

/// Write the result to file
ostream & operator << (ostream & os,TestCase &t){
	// correspondances
	os << t.result.size() << endl;

	for(auto & elem : t.result)
		os << elem << endl;

	return os;
}

istream & operator >> (istream & is,TestCase &t){
	// Read input
	is >>t.rows;
	is >>t.cols;

	char c = 0;
	t.wall = Mat(t.rows, t.cols, CV_8UC1);

	for(int j = 0 ; j < t.rows ; j++)
	{
		uchar* pwall = (uchar*)t.wall.data + t.cols * j;
		for(int k = 0 ; k < t.cols ; k++)
		{
			is >> c;
			*pwall = c == '#' ? 255 : 0;
			pwall++;
		}
	}

	return is;
}


int main(int argc, char **argv){
	vector<TestCase *> tarr;
	
	assert(argc > 1);
	string filename(argv[1]);

	// open in and out files
	ifstream fin;
	fin.open (filename.c_str());

	string dirname = "out_" + timeStamp() + '/';
	stringstream cmd;
	cmd << "mkdir -p " << dirname << " && ln -s " << dirname << " out_latest";
	if(system(cmd.str().c_str()) != 0)
	{
		cout << "error while making " << dirname << endl;
		exit(1);
	}

	

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
				ss<< /*"Case --> " <<*/ *elem <<endl;
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
		ofstream fout;
		stringstream ss;
		ss << dirname << filename << ".out";
		fout.open ( ss.str().c_str());
		fout<< /*"Case #"<<(j+1)<<": "<<*/*elem<<endl;
		j++;
	}

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
