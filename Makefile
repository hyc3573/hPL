bin ?= hPL
test ?= test/test

CXX ?= CXX
CXXFLAGS ?= -Wall -Wpedantic -g -ggdb3 -std=c++17
LDFLAGS ?= 

objs ?= main.o lex.o utils.o tokens.o node.o parse.o

export CXX

.Phony : run clean
all: $(bin) $(test)

deps := $(patsubst %.o,%.d,$(objs))
-include $(deps)
DEPFLAGS = -MMD -MF $(@:.o=.d)

$(bin): $(objs)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

$(test): $(objs)
	make -C test

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< $(DEPFLAGS)

clean:
	rm -f $(objs) $(bin)

run: $(test)
	./$(test)
