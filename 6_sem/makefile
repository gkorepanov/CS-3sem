.PHONY: clean server client

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

server:
	$(TARGET) test.txt

client:
	$(TARGET)

debug: FLAGS += -DDEBUG
debug: $(TARGET)
