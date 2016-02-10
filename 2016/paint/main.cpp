
#include "utils.hpp"
#include <opencv2/opencv.hpp>
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

struct gPoint
{
	gPoint(){}
	gPoint(const Point _p) : c(_p.x), r(_p.y) {}
	int c = 0;
	int r = 0;
	bool operator == (const gPoint& p){return r == p.r && c == p.c;}
	inline friend std::istream& operator>> (std::istream& is, gPoint& p) {is >> p.r; is >> p.c;}
	inline friend std::ostream& operator<< (std::ostream& os, const gPoint& p) {os << p.r << " " << p.c;}
	Point toPoint(){
		return Point(c, r);
	}
};

struct gLine
{
	gLine(){}
	gLine(const gPoint& _p1, const gPoint _p2) : p1(_p1), p2(_p2){}
	gLine(const Rect& _r) : p1(_r.tl()), p2(_r.br()){}
	gPoint p1;
	gPoint p2;
	Rect toRect(){
		return Rect(p1.toPoint(), p2.toPoint());
	}
	bool operator == (const gLine& li){return p1 == li.p1 && p2 == li.p2;}
	inline friend std::istream& operator>> (std::istream& is, gLine& li) {is >> li.p1; is >> li.p2;}
	inline friend std::ostream& operator<< (std::ostream& os, const gLine& li) {os << li.p1.r << " " << li.p1.c << " " << li.p2.r << " " << li.p2.c;}
};

struct gSquare
{
	gSquare(){}
	gSquare(const Rect& _r){
		assert(_r.width == _r.height);
		s = (_r.width - 1) / 2;
		c = _r.x + s; 
		r = _r.y + s;
	}
	int r = 0;
	int c = 0;
	int s = 0;
	Rect toRect(){
		return Rect(c - s, r - s, s * 2 + 1, s * 2 + 1);
	}
	bool operator == (const gSquare& sq){return r == sq.r && c == sq.c && s == sq.s;}
	inline friend std::istream& operator>> (std::istream& is, gSquare& sq) {is >> sq.r; is >> sq.c; is >> sq.s;}
	inline friend std::ostream& operator<< (std::ostream& os, const gSquare& sq) {os << sq << " " << sq << " " << sq.s;}
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

		void SelectCandidates(const Rect& bounds, list<Candidate>& candidates) const;
		bool PaintMaxCandidate();
		void Erase();
		inline int  Rate1(Candidate& can) const {return countNonZero(wall(can.rect) & (painted(can.rect) == 0));}
		// inline int  Rate2(Candidate& can){return countNonZero(wall(can.rect) == 0);}

		// Our heuristic
		inline int  Rate(Candidate& can) const {
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

void TestCase::SelectCandidates(const Rect& bounds, list<Candidate>& candidates) const
{
	candidates.clear();
	for(int s = (min(bounds.width, bounds.height) - 1) / 2 ; s >= 0 ; s--)
	{
		// cout << "s" << s << endl;
		for(int i = bounds.y + s ; i < bounds.y + bounds.height - s; i++)
		{
			for(int j = bounds.x + s ; j < bounds.x + bounds.width - s; j++)
			{
				Candidate can;
				can.rect = Rect(Point2i(j-s,i-s), Point2i(j+s + 1, i+s + 1));

				if(Rate(can) > 0)
				{
					stringstream ss;
					gSquare sq(can.rect);
					// cout << can.rect << endl;
					// cout << sq.r << sq.c << sq.s << endl;
					// cout << sq.toRect() << endl;
					assert(can.rect == sq.toRect());
					ss << "PAINT_SQUARE " << sq.r << " " << sq.c << " " << sq.s;
					can.command = ss.str();
					// assert((can.rect & bounds) == can.rect);
					candidates.push_back(can);
				}
			}
		}
	}

	cout << "Candidates sq " << candidates.size() << endl;

	// horiz
	for (int r = bounds.y ; r < bounds.y + bounds.height; r++)
	{
		// cout << "r" << r << endl;
		for (int c = bounds.x; c < bounds.x + bounds.width; c++)
		{
			Rect rect = Rect(Point2i(c,r), Point2i(c+1,r + 1));
			for (int l = 2; l < bounds.width - c; l++)
			{
				Candidate can;
				can.rect = rect;

				if(Rate(can) > 0)
				{
					gLine li(rect);
					assert(li.toRect() == rect);
					assert(c+l < bounds.x + bounds.width);
					stringstream ss;
					ss << "PAINT_LINE " << li;
					can.command = ss.str();
					// assert((can.rect & bounds) == can.rect);
					candidates.push_back(can);
				}
				rect.width++;
			}
		}
	}

	cout << "Candidates sq+li " << candidates.size() << endl;

	for (int r = bounds.y; r < bounds.y + bounds.height; r++)
	{
		// cout << "r" << r << endl;
		for (int c = bounds.x; c < bounds.x + bounds.width; c++)
		{
			Rect rect = Rect(Point2i(c,r), Point2i(c+1,r + 1));
			for (int l = 2; l < bounds.height - r; l++)
			{
				Candidate can;
				can.rect = rect;

				if(Rate(can) > 0)
				{
					stringstream ss;
					gLine li(rect);
					ss << "PAINT_LINE " << li;
					can.command = ss.str();
					// assert((can.rect & bounds) == can.rect);
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
			/*
			else if(*pwall > 0 && *ppaint == 0)
			{
				assert(false);
				cout << "ERROR " << r << " " << c << " : " << *pwall * 1.0 << "!=" << *ppaint * 1.0 << endl;
			}
			assert(*pwall == *ppaint);
			*/
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

bool check(Mat& image, const vector<string>& result)
{
	Mat paint(image.size(), image.type());
	paint.setTo(0);
	for(auto& elem : result)
	{
		istringstream ss(elem);
		string cmd;
		ss >> cmd;
		cout << elem << endl;
		Point p1, p2;
		if(cmd == "PAINT_SQUARE")
		{
			gSquare sq;
			ss >> sq;
			paint(sq.toRect()).setTo(255);
		}
		else if(cmd == "PAINT_LINE")
		{
			gLine li;
			ss >> li;
			paint(li.toRect()).setTo(255);
		}
		else if(cmd == "ERASE_CELL")
		{
			gPoint p;
			ss >> p;
			paint.at<uchar>(p.r, p.c) = 0;
		}
		else 
		{
			cout << "Unexpected cmd " << cmd << endl;
			exit(1);
		}
	}
	// imshow("result", paint);
	// cout << countNonZero(paint != image) << endl;
	// imshow("diff", paint - image);
	if(countNonZero(paint != image))
	{
		Mat diff;
		absdiff(paint, image, diff);
		cout << diff << endl;
		waitKey(0);
	}
	return countNonZero(paint != image) == 0;
}

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

	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;

	/// Detect edges using Threshold
	// threshold(m_input, threshold_output, thresh, 255, THRESH_BINARY);
	/// Find contours
	Mat copy;
	wall.copyTo(copy);
	// findContours(copy, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(0, 0));

	// for(unsigned int i = 0; i< contours.size(); i++)
	{
		// For each contour
		// Rect rect(boundingRect(contours[i]);
		Rect rect(0,0,wall.cols,wall.rows);
		SelectCandidates(rect, candidates);
		bool ret = true;
		while(ret)
		{
			ret = PaintMaxCandidate();
		}
	}
	imshow("Wall", painted);
	// waitKey(0);
	Erase();

	if(countNonZero(painted != wall) != 0)
	{
		cout << "ERROR: Non identical " << countNonZero(painted != wall) << endl;
		imshow("Wall", painted != wall);
		// waitKey(0);
		// cout << (painted != wall) << endl;
		result.clear();
	}
	imshow("Painted", painted);
	// waitKey(0);

	if(!check(wall, result))
	{
		cout << "ERROR" << endl;
		imshow("Wall", wall);
		waitKey(0);
		exit(1);
	}

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
	cmd << "mkdir -p " << dirname << " && rm -f out_latest && ln -s " << dirname << " out_latest";
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



