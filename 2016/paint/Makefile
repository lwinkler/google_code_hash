all: main simpleline

main: main.cpp
	g++ -g -O3 -Wextra -o main main.cpp -pthread --std=c++11 `pkg-config --libs opencv`

simpleline: simpleline.cpp
	g++ -g -O3 -Wextra -o simpleline simpleline.cpp -pthread --std=c++11 `pkg-config --libs opencv`

clean:
	rm -rf out_*
