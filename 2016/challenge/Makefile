# This makefile is provided as is
#
# Author: Laurent Winkler
# Date  : Feb 2016
#
#
# note: include only the min libraries needed, some libs like higui are problematic for multi threading
#
#        -lopencv_calib3d -lopencv_contrib -lopencv_core -lopencv_features2d -lopencv_flann -lopencv_gpu -lopencv_highgui -lopencv_imgproc -lopencv_legacy -lopencv_ml -lopencv_objdetect -lopencv_ocl -lopencv_photo -lopencv_stitching -lopencv_superres -lopencv_ts -lopencv_video -lopencv_videostab  


all: main simpleline main-debug

main: main.cpp utils.hpp
	g++ -g -O3 -Wextra -o main main.cpp -pthread --std=c++11 -lopencv_core -lopencv_imgproc

main-debug: main.cpp utils.hpp
	g++ -g -O3 -Wextra -DGDEBUG -o main-debug main.cpp -pthread --std=c++11 -lopencv_core -lopencv_imgproc -lopencv_highgui


simpleline: simpleline.cpp
	g++ -g -O3 -Wextra -o simpleline simpleline.cpp -pthread --std=c++11 `pkg-config --libs opencv`

clean:
	rm -rf main simpleline main-debug

distclean: clean
	rm -rf out_*

pack:
	tar -cvzf src.tar.gz main.cpp Makefile utils.hpp
