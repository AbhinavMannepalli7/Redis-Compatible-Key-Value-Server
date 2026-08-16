CXX = g++
CXXFLAGS = -std=c++23 -Wall -Wextra -g -fsanitize=address,undefined

TARGET = server

SRCS = main.cpp socket.cpp connection.cpp command.cpp dispatcher.cpp
OBJS = $(SRCS:.cpp=.o)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	./$(TARGET)