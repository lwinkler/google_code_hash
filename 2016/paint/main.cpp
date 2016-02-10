
#include "utils.hpp"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <boost/multi_array.hpp>

using namespace cv;
using namespace boost;

struct Candidate
{
	string command;
	Rect   rect;
};


/// Class representing our main test case
class TestCase{

	public:
		// Data of the problem
		int rows;
		int cols;

		// multi_array<char, 2>* pwall = nullptr;
		Mat wall;
		Mat painted;
		vector<string> result;

		void SelectCandidates();
		bool PaintMaxCandidate();
		void Erase();
		inline int  Rate1(Candidate& can){return countNonZero(wall(can.rect) & (painted(can.rect) == 0));}
		// inline int  Rate2(Candidate& can){return countNonZero(wall(can.rect) == 0);}

		// Our heuristic
		inline int  Rate(Candidate& can){
			int val = Rate1(can); 
			if(val == 0) 
				return INT_MIN; 
			else return val - K * countNonZero(wall(can.rect) == 0);
		}

		// Params for optimization
		const int K;

	public:
		TestCase(int k) : K(k) {}
		~TestCase(){};
		void doWork();
		// Used to name the result file
		string OutputFileName(){
			stringstream ss;
			ss << setfill('0') << setw(5) << result.size() << ".K=" << K;
			return ss.str();
		}
		friend ostream & operator << (ostream & os,TestCase &t);
		friend istream & operator >> (istream & is,TestCase &t);

	private:
		bool started = false;
		list<Candidate> candidates;
};

void TestCase::SelectCandidates()
{
	Rect rect_mat(0, 0, wall.cols, wall.rows);
	for(int s = (min(wall.rows, wall.cols) - 1) / 2 ; s >= 0 ; s--)
	{
		cout << "s" << s << endl;
		for(int i = 0 + s ; i < wall.rows - s; i++)
		{
			for(int j = 0 + s ; j < wall.cols - s; j++)
			{
				Candidate can;
				can.rect = Rect(Point2i(j-s,i-s), Point2i(j+s + 1, i+s + 1));

				if(Rate(can) > 0)
				{
					stringstream ss;
					ss << "PAINT_SQUARE " << i - 1 << " " << j - 1 << " " << s;
					can.command = ss.str();
					// assert((can.rect & rect_mat) == can.rect);
					candidates.push_back(can);
				}
			}
		}
	}

	cout << "Candidates sq " << candidates.size() << endl;

	for (int r = 0; r < wall.rows; r++)
	{
		cout << "r" << r << endl;
		for (int c = 0; c < wall.cols; c++)
		{
			Rect rect = Rect(Point2i(c,r), Point2i(c+1,r + 1));
			for (int l = 2; l < wall.cols - c; l++)
			{
				Candidate can;
				can.rect = rect;

				if(Rate(can) > 0)
				{
					stringstream ss;
					ss << "PAINT_LINE " << r << " " << c << " " << r + l << " " << c;
					can.command = ss.str();
					// assert((can.rect & rect_mat) == can.rect);
					candidates.push_back(can);
				}
				rect.width++;
			}
		}
	}

	cout << "Candidates sq+li " << candidates.size() << endl;

	for (int r = 0; r < wall.rows; r++)
	{
		cout << "r" << r << endl;
		for (int c = 0; c < wall.cols; c++)
		{
			Rect rect = Rect(Point2i(c,r), Point2i(c+1,r + 1));
			for (int l = 2; l < wall.rows - r; l++)
			{
				Candidate can;
				can.rect = rect;

				if(Rate(can) > 0)
				{
					stringstream ss;
					ss << "PAINT_LINE " << r << " " << c << " " << r << " " << c + l;
					can.command = ss.str();
					// assert((can.rect & rect_mat) == can.rect);
					candidates.push_back(can);
				}
				rect.height++;
			}
		}
	}

	cout << "Candidates sq+li " << candidates.size() << endl;
}

bool TestCase::PaintMaxCandidate()
{
	list<Candidate>::iterator itmax;
	list<Candidate>::iterator it = candidates.begin();

	int max = INT_MIN;
	while(it != candidates.end())
	{
		int score = Rate(*it);
		if(score == INT_MIN)
		{
			it = candidates.erase(it);
			continue;
		}
		if(score > max)
		{
			max = score;
			itmax = it;
		}
		it++;
	}

	cout << "candidates " << candidates.size() << " max " << max << endl;

	if(max <= 0)
		return false;

	painted(itmax->rect).setTo(255);
	result.push_back(itmax->command);
	candidates.erase(itmax);
	return true;
}

/// Create erase commands
void TestCase::Erase()
{
	assert(wall.size() == painted.size());
	uchar* pwall = wall.data;
	uchar* ppaint = painted.data;
	for (int r = 0; r < wall.rows; r++)
	{
		for (int c = 0; c < wall.cols; c++)
		{
			// cout << *pwall * 1.0 << *ppaint * 1.0 << endl;
			if(*pwall == 0 && *ppaint > 0)
			{
				stringstream ss;
				ss << "ERASE_CELL " << r << " " << c;
				*ppaint = 0;
				result.push_back(ss.str());
			}
			else if(*pwall > 0 && *ppaint == 0)
				cout << "ERROR " << r << " " << c;
			pwall++;
			ppaint++;
		}
	}
	
}

/*
void paintSquares(Mat& wall, Mat& painted, int s, int maxBlack, vector<string>& result, vector<string>& result2)
{
	int area = (2*s+1) * (2*s+1);
	Rect rect_mat(0, 0, wall.cols, wall.rows);
	for(int i = 0 + s ; i < painted.rows - s; i++)
	{
		for(int j = 0 + s ; j < painted.cols - s; j++)
		{
			Rect rect(Point2i(j-s,i-s), Point2i(j+s + 1, i+s + 1));
			assert((rect & rect_mat) == rect);

			if(countNonZero(wall(rect)) >= area - maxBlack && countNonZero(painted(rect)) != area)
			{
				painted(rect).setTo(255);
				stringstream ss;
				ss << "PAINT_SQUARE " << i - 1 << " " << j - 1 << " " << s;
				result.push_back(ss.str());
			}
		}
	}
}
*/

/// Implement here the main work of the test case
void TestCase::doWork()
{
	painted = Mat(wall.size(), wall.type());
	painted.setTo(0);

	/*
	for(int s = (min(painted.rows, painted.cols) - 1) / 2 ; s >= 0 ; s--)
	{
		paintSquares(wall, painted, s, 0, result);
	}

	*/
	namedWindow( "Wall", WINDOW_AUTOSIZE );

	SelectCandidates(); // TODO: Segment to optimize greatly
	bool ret = true;
	while(ret)
	{
		ret = PaintMaxCandidate();
	}
	imshow("Wall", painted);
	// waitKey(0);
	Erase();

	if(countNonZero(painted != wall) != 0)
	{
		cout << "ERROR: Non identical" << endl;
		cout << (painted != wall) << endl;
		result.clear();
	}
	imshow("Wall", painted);
	// waitKey(0);
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

	string dirname = "out_" + timeStamp() + '/';
	stringstream cmd;
	cmd << "mkdir -p " << dirname << " && rm out_latest && ln -s " << dirname << " out_latest";
	if(system(cmd.str().c_str()) != 0)
	{
		cout << "error while making " << dirname << endl;
		exit(1);
	}

	
#ifndef MULTI_INPUT // Multiple problems in single file: e.g. google code jam
	// Some parameter to vary for optimization
	vector<int> ks({0 , 1, 4, 9, 16, 25, 36, 49});
	int gN = ks.size();
#else
	ifstream fin;
	fin.open (filename.c_str());
	// read params
	fin>>gN;

	// HACK for getline
	// string a;
	// getline(fin,a);
#endif

	TestCase *tptr;
	for(int i=0;i<gN;i++){
#ifndef MULTI_INPUT
		ifstream fin;
		fin.open (filename.c_str());
#endif
		tptr = new TestCase(ks.at(i));
		fin >> *tptr;
		tarr.push_back(tptr);
	}

	int max_threads = std::thread::hardware_concurrency();
	if(argc > 2 && atoi(argv[2]) > 0)
		max_threads =  atoi(argv[2]);

	if(max_threads > 1)
	{
		cout << "Create a pool of " << max_threads << " working threads" << endl;
		std::vector< std::future<string> > results;
		ThreadPool pool(max_threads);

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
	}

	std::cout << std::endl;

	// Write down results
	int j = 0;
	for(auto& elem : tarr)
	{
		if(max_threads <= 1)
			elem->doWork();
		ofstream fout;
		stringstream ss;
		ss << dirname << filename << "." << elem->OutputFileName() << ".out";
		fout.open ( ss.str().c_str());
		fout<< /*"Case #"<<(j+1)<<": "<<*/*elem<<endl;
		j++;
	}
}



