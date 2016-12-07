.PHONY: clean run

TARGET = ./rw
FLAGS = -Wall -g
OBJS = rw.o
DEPS = $(OBJS:.o=.d)


$(TARGET): $(OBJS)
	gcc -o $@ $^

-include $(DEPS)

%.o: %.c
	gcc $(FLAGS) -c -o $@ $<
	gcc $(FLAGS) -MM -o $(patsubst %.o, %.d, $@) $<


clean:
	-rm $(OBJS) $(TARGET)
	-find . -name "*.d" | xargs rm

run:
	$(TARGET) test.txt

debug: FLAGS += -DDEBUG
debug: $(TARGET)
