bin := test

c++ := c++
c++flags := -Wall -Wpedantic -g -ggdb3
ldflags := -lboost_unit_test_framework

objs := test.o
testobjs := ../lex.o ../utils.o ../tokens.o ../node.o ../parse.o

.Phony : run clean
all: $(bin)

deps := $(patsubst %.o,%.d,$(objs))
-include $(deps)
DEPFLAGS = -MMD -MF $(@:.o=.d)

$(bin): $(objs) $(testobjs)
	$(c++) $(c++flags) $(ldflags) -o $@ $^

%.o: %.cpp
	$(c++) $(c++flags) -c $< $(DEPFLAGS)

$(testobjs):
	make -C

clean:
	rm -f $(objs) $(bin)

run:
	./$(bin)
