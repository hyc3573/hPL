bin ?= test

CXX ?= CXX
CXXFLAGS ?= -Wall -Wpedantic -g -ggdb3 -std=c++17
LDFLAGS ?= -lboost_unit_test_framework

objs ?= test.o
testobjs ?= ../lex.o ../utils.o ../tokens.o ../node.o ../parse.o

.Phony : run clean
all: $(bin)

deps := $(patsubst %.o,%.d,$(objs))
-include $(deps)
DEPFLAGS = -MMD -MF $(@:.o=.d)

$(bin): $(objs) $(testobjs)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

%.o: %.cpp $(testobjs)
	$(CXX) $(CXXFLAGS) -c $< $(DEPFLAGS)

../%.o: ../%.cpp
	make -C ..

clean:
	rm -f $(objs) $(bin)
	make -C .. clean

run:
	./$(bin)
