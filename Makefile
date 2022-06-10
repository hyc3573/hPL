bin := hPL

c++ := c++
c++flags := -Wall -Wpedantic -g -ggdb3
ldflags := 

objs := main.o lex.o utils.o tokens.o node.o parse.o

.Phony : run clean
all: $(bin)

deps := $(patsubst %.o,%.d,$(objs))
-include $(deps)
DEPFLAGS = -MMD -MF $(@:.o=.d)

$(bin): $(objs)
	$(c++) $(c++flags) $(ldflags) -o $@ $^

%.o: %.cpp
	$(c++) $(c++flags) -c $< $(DEPFLAGS)

clean:
	rm -f $(objs) $(bin)

run:
	./$(bin)
