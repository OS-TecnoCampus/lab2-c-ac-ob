PROGRAM_SERVER = server1 
PROGRAM_CLIENT = client1
PROGRAM_OBJS = server1.o client1.o library.h
LDFLAGS = -pthread

all: $(PROGRAM_SERVER)
    @echo "Finished"
    
all: $(PROGRAM_CLIENT)
    @echo "Finished"

$(PROGRAM_SERVER): $(PROGRAM_OBJS)
    gcc $(LDFLAGS) -o $@ $<

$(PROGRAM_CLIENT): $(PROGRAM_OBJS)
    gcc -o $@ $<

%.o: %.c
    gcc -c $<

clean: $(RM)(PROGRAM_NAME)
    @echo "Clean done"
