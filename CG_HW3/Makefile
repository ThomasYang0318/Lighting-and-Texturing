CXX = clang++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 \
  -I/opt/homebrew/include \
  -I/opt/homebrew/opt/opencv/include/opencv4

OPENCV_CFLAGS := $(shell pkg-config --cflags opencv4)
OPENCV_LIBS   := $(shell pkg-config --libs opencv4)

LDFLAGS = -framework OpenGL -framework GLUT $(OPENCV_LIBS)

SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:.cpp=.o)

CG_HW3: $(OBJS)
	$(CXX) $(CXXFLAGS) $(OPENCV_CFLAGS) $(OBJS) $(LDFLAGS) -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(OPENCV_CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) CG_HW3
