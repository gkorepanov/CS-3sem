.PHONY: clean run

TARGET = ./proxy
FLAGS = -Wall -g
LIBS = -lm
OBJS = proxy.o tools/tools.o
DEPS = $(OBJS:.o=.d)


$(TARGET): $(OBJS)
	gcc $(FLAGS) -o $@ $^ $(LIBS)

-include $(DEPS)

%.o: %.c
	gcc $(FLAGS) -c -o $@ $<
	gcc $(FLAGS) -MM -o $(patsubst %.o, %.d, $@) $<


clean:
	-rm $(OBJS) $(TARGET)
	-find . -name "*.d" | xargs rm

debug: FLAGS += -DDEBUG
debug: $(TARGET)
