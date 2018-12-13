PROJECT_NAME = httpDynServerWs
CC=gcc
PROJECT_SRC = .
vpath %.c $(PROJECT_SRC)

########################################################################
SRCS = httpDynServerWs_sensor.c
SRCS += httpServerLib.c
SRCS += libWs.c
SRCS += senseHat.c

########################################################################
INC_DIRS = .
INCLUDE = $(addprefix -I,$(INC_DIRS))
########################################################################

CFLAGS=-std=c99 -g -U__STRICT_ANSI__ \
       -W -Wall -pedantic -O3 \
	-lsqlite3 \
       -pthread \
       -D_REENTRANT # `pkg-config openssl --cflags`

LDFLAGS=-lpthread -lm # `pkg-config openssl --libs`

########################################################################

.PHONY: all
all: $(PROJECT_NAME)

.PHONY: $(PROJECT_NAME)
$(PROJECT_NAME): exec_$(PROJECT_NAME)

exec_$(PROJECT_NAME): $(SRCS)
	$(CC) $^ $(INCLUDE) $(DEFS) $(CFLAGS) $(WFLAGS) $(LDFLAGS)  -o $@

%.o: %.c
	$(CC) -c -o $@ $^ $(INCLUDE) $(DEFS) $(CFLAGS) $(LDFLAGS)

clean:
	rm -f *.o exec_$(PROJECT_NAME)

########################################################################
