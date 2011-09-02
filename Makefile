
all: acceptor tester

OBJS += acceptor.o

CXXFLAGS += -Werror -Wall -O0 -g

CFLAGS += -Werror -Wall -O0 -g

LDFLAGS += -lboost_program_options

acceptor: $(OBJS)
	$(CXX) $(LDFLAGS) $^ -o $@

tester: tester.o acceptorclient.o
	$(CXX) $(LDFLAGS) $^ -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	- rm -f *.o acceptor tester
