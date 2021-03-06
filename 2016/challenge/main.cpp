
#include "utils.hpp"
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
// #include <opencv2/highgui/highgui.hpp>

//#include <boost/multi_array.hpp>

using namespace cv;
//using namespace boost;

#ifndef GDEBUG // debug or not
#define imshow(a,b)
#define waitKey(a)
#endif


struct Candidate
{
	string command;
	Rect   rect;
	double counterScore = 0;
};

struct gPoint
{
	gPoint(){}
	gPoint(const Point _p) : c(_p.x), r(_p.y) {}
	int c = 0;
	int r = 0;
	bool operator == (const gPoint& p){return r == p.r && c == p.c;}
	inline friend istream& operator>> (istream& is, gPoint& p) {is >> p.r; is >> p.c;}
	inline friend ostream& operator<< (ostream& os, const gPoint& p) {os << p.r << " " << p.c;}
	Point toPoint(){
		return Point(c, r);
	}
};

struct gLine
{
	gLine(){}
	gLine(const gPoint& _p1, const gPoint _p2) : p1(_p1), p2(_p2){}
	gLine(const Rect& _r) : p1(_r.tl()), p2(_r.br() - Point(1,1)){}
	gPoint p1;
	gPoint p2;
	Rect toRect(){
		return Rect(p1.toPoint(), p2.toPoint() + Point(1,1));
	}
	bool operator == (const gLine& li){return p1 == li.p1 && p2 == li.p2;}
	inline friend istream& operator>> (istream& is, gLine& li) {is >> li.p1; is >> li.p2;}
	inline friend ostream& operator<< (ostream& os, const gLine& li) {os << li.p1.r << " " << li.p1.c << " " << li.p2.r << " " << li.p2.c;}
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
	inline friend istream& operator>> (istream& is, gSquare& sq) {is >> sq.r; is >> sq.c; is >> sq.s;}
	inline friend ostream& operator<< (ostream& os, const gSquare& sq) {os << sq << " " << sq << " " << sq.s;}
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
		Mat counter; // counts how many candidate for each cell
		vector<string> result;

		void SelectCandidates(const Rect& bounds, const Mat& image, list<Candidate>& candidates) const;
		bool PaintMaxCandidate();
		void Erase();
		static inline int  Rate1(Candidate& can, const Mat& wall, const Mat& painted) {return countNonZero(wall(can.rect) & (painted(can.rect) == 0));}

		// Our heuristic
		static inline int  Rate(Candidate& can, const Mat& wall, const Mat& painted, int K) {
			int val = Rate1(can, wall, painted); 
			if(val == 0) 
				return INT_MIN; 
			// else return 1000 * static_cast<double>(val) / (K + countNonZero(wall(can.rect) == 0) * val);
			else return val - K * countNonZero(wall(can.rect) == 0); //  + can.counterScore;
		}

		// Internal use
		const int test_num;

		// Params for optimization
		const int K;

	public:
		TestCase(int _test_num, int _k) : test_num(_test_num), K(_k) {}
		~TestCase(){};
		void doWork();
		// Used to name the result file
		string OutputFileName(){
			stringstream ss;
			ss << setfill('0') << setw(5) << result.size() << ".K=" << K;
			return ss.str();
		}
		void WriteToFile(const string& inputFile, const string& directory)
		{
			ofstream fout;
			stringstream ss;
			ss << directory << inputFile << "." << OutputFileName() << ".out";
			fout.open ( ss.str().c_str());
#ifdef MULTI_INPUT
			fout<< "Case #"<<(test_num + 1)<<": "<<*this<<endl;
#else
			fout<< *this << endl;
#endif
		}
		friend ostream & operator << (ostream & os,TestCase &t);
		friend istream & operator >> (istream & is,TestCase &t);

	private:
		bool started = false;
		list<Candidate> candidates;
};

void TestCase::SelectCandidates(const Rect& bounds, const Mat& image, list<Candidate>& candidates) const
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

				if(Rate(can, image, painted, K) > 0)
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
		vector<Point2i> start;
		vector<Point2i> finish;
		// cout << "r" << r << endl;

		for (int c = bounds.x; c < bounds.x + bounds.width; c++)
		{
			// if((c == 0 || *(pwall-1) == 0 ) && *pwall== 255)
			if((c == bounds.x || image.at<uchar>(r,c-1) == 0 ) && image.at<uchar>(r,c)== 255)
				start.emplace_back(c,r);
			if((c == bounds.x + bounds.width - 1 || image.at<uchar>(r,c+1) == 0 ) && image.at<uchar>(r,c)== 255)
				finish.emplace_back(c + 1,r + 1);
		}
		// cout << "sizes " << finish.size() << "  " << start.size() << endl;

		for(const auto& estart : start)
		{
			for(const auto& efinish : finish)
			{
				if(efinish.x <= estart.x)
					continue;

				Candidate can;
				can.rect = Rect(estart, efinish);

				if(Rate(can, image, painted, K) > 0)
				{
					gLine li(can.rect);
					assert(li.toRect() == can.rect);
					stringstream ss;
					ss << "PAINT_LINE " << li;
					can.command = ss.str();
					// assert((can.rect & bounds) == can.rect);
					candidates.push_back(can);
				}
			}
		}
	}

	cout << "Candidates sq+li " << candidates.size() << endl;

	for (int c = bounds.x; c < bounds.x + bounds.width; c++)
	{
		// cout << "r" << r << endl;
		vector<Point2i> start;
		vector<Point2i> finish;
		// cout << "r" << r << endl;
		for (int r = bounds.y; r < bounds.y + bounds.height; r++)
		{
			if((r == bounds.y || image.at<uchar>(r-1,c) == 0 ) && image.at<uchar>(r,c)== 255)
				start.emplace_back(c,r);
			if((r == bounds.y + bounds.height - 1 || image.at<uchar>(r+1,c) == 0 ) && image.at<uchar>(r,c)== 255)
				finish.emplace_back(c + 1,r + 1);
		}

		for(const auto& estart : start)
		{
			for(const auto& efinish : finish)
			{
				if(efinish.y <= estart.y)
					continue;
				Candidate can;
				can.rect = Rect(estart, efinish);

				if(Rate(can, image, painted, K) > 0)
				{
					gLine li(can.rect);
					stringstream ss;
					ss << "PAINT_LINE " << li;
					can.command = ss.str();
					// assert((can.rect & bounds) == can.rect);
					candidates.push_back(can);
				}
			}
		}
	}

	cout << "Candidates sq+li " << candidates.size() << endl;
}

bool TestCase::PaintMaxCandidate()
{
	list<Candidate>::iterator itmax;
	list<Candidate>::iterator it = candidates.begin();

	for(auto& elem : candidates)
	{
		elem.counterScore = sum(counter(elem.rect))[0];
	}

	int max = INT_MIN;
	int maxCounterScore = INT_MIN;
	while(it != candidates.end())
	{
		int score = Rate(*it, wall, painted, K);
		if(score == INT_MIN)
		{
			it = candidates.erase(it);
			continue;
		}
		else if(score > max)
		{
			max = score;
			maxCounterScore = it->counterScore;
			itmax = it;
		}
		else if(score == max)
		{
			max = score;
			// cout << "compare" << it->counterScore << " " << maxCounterScore << endl;
			if(it->counterScore < maxCounterScore)
			{
				itmax = it;
				maxCounterScore = it->counterScore;
			}
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
		// waitKey(0);
	}
	return countNonZero(paint != image) == 0;
}

/// Implement here the main work of the test case
void TestCase::doWork()
{
	painted = Mat(wall.size(), wall.type());
	painted.setTo(0);
	counter = Mat(wall.size(), CV_16UC1);

	/*
	for(int s = (min(painted.rows, painted.cols) - 1) / 2 ; s >= 0 ; s--)
	{
		paintSquares(wall, painted, s, 0, result);
	}

	*/
	// namedWindow( "Wall", WINDOW_AUTOSIZE );

	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;

	/// Detect edges using Threshold
	// threshold(m_input, threshold_output, thresh, 255, THRESH_BINARY);
	/// Find contours
	Mat copy(Size(wall.cols + 2, wall.rows + 2), wall.type());
	// dirty trick
	copy.setTo(0);
	Rect roi(1,1,wall.cols,wall.rows);
	wall.copyTo(copy(roi));
	findContours(copy, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, Point(-1,-1));

	for(unsigned int i = 0; i< contours.size(); i++)
	{
		// For each contour
		Rect rect(boundingRect(contours[i]));
		cout << "box " << rect << endl;
		// mask image to avoid drawing cut shapes
		Mat mask(wall.size(), wall.type());
		mask.setTo(0);
		drawContours(mask, contours, i, CV_RGB(255,255,255), CV_FILLED, 4);
		// drawContours(mask, contours, i, CV_RGB(255,255,255),         1, 4, vector<Vec4i>(), 0, Point());
		imshow("Mask", mask);
		// waitKey(0);
		mask &= wall;
		mask &= (255 - painted);
		imshow("Mask", mask);
		waitKey(0);
		SelectCandidates(rect, mask, candidates);

		counter.setTo(0);
		for(auto& elem : candidates)
		{
			counter(elem.rect) += 1;
		}
		swap(wall, mask); // trick
		bool ret = true;
		while(ret)
		{
			ret = PaintMaxCandidate();
			//imshow("Painted", painted);
			//waitKey(0);
		}
		swap(wall, mask); // trick
	}
	Erase();

	if(countNonZero(painted != wall) != 0)
	{
		cout << "ERROR: Non identical " << countNonZero(painted != wall) << endl;
		imshow("Difference", painted != wall);
		waitKey(0);
		// cout << (painted != wall) << endl;
		result.clear();
	}
	// imshow("Painted", painted);
	// waitKey(0);

	if(!check(wall, result))
	{
		cout << "ERROR" << endl;
		// imshow("Wall", wall);
		// waitKey(0);
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
	vector<int> ks({25, 36, 49, 64, 81, 100, 121, 144});
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
		tptr = new TestCase(i, ks.at(i));
		fin >> *tptr;
		tarr.push_back(tptr);
	}

	int max_threads = thread::hardware_concurrency();
	if(argc > 2 && atoi(argv[2]) > 0)
		max_threads =  atoi(argv[2]);
#ifdef GDEBUG
	max_threads = 1;
#endif

	if(max_threads > 1)
	{
		cout << "Create a pool of " << max_threads << " working threads" << endl;
		vector< future<string> > results;
		ThreadPool pool(max_threads);

		for(auto& elem : tarr)
		{
			results.emplace_back(
				pool.enqueue([&elem,&filename,&dirname]{
					cout << "starting job" << endl;
					elem->doWork();
#ifndef MULTI_INPUT
					elem->WriteToFile(filename, dirname);
#endif
					stringstream ss;
					ss << endl << "Found solution " << *elem <<endl;
					return ss.str();
				})
			);
		}

		for(auto && result: results)
			cout << result.get() << ' ';
	}

	// Write down results
	for(auto& elem : tarr)
	{
#ifndef MULTI_INPUT
		if(max_threads <= 1)
		{
			elem->doWork();
			elem->WriteToFile(filename, dirname);
		}
#else
		elem->WriteToFile(filename, dirname);
#endif
	}
}



