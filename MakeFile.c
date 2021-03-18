PROGRAM_NAME = server client
PROGRAM_OBJS = server.o client.o

all: $(PROGRAM_NAME)
    @echo "Finished"

$(PROGRAM_NAME): $(PROGRAM_OBJS)
    gcc -o $@ $<

%.o: %.c
    gcc -c $<

clean: $(RM)(PROGRAM_NAME)
    @echo "Clean done"
