TOOLS	=	minidragonfly 

COMMON	=	globalvars.o log.o minixml.o parser.o rpc.o sql.o utils.o url_parser.o processor.o mongoose.o version.o
DEPS	=	Makefile globalvars.h log.h minixml.h parser.h queue.h rpc.h sql.h types.h utils.h version.h url_parser.h mongoose.h processor.h 

CC		=  gcc -g
LDLIBS  =   -lpcre -lsqlite3 -ljson -lpthread -ldl -lssl -lcrypto
SUFFIX  =	
OBJS	= 	$(COMMON) $(addsuffix .o, $(TOOLS))
CFLAGS	= 	-DPCRE_STATIC -DHAVE_STDINT -DNDEBUG

GIT_VERSION := $(shell git describe --dirty --always)

ifeq ($(OS),Windows_NT)
	SUFFIX = .exe
	LDLIBS += -lws2_32 -lgdi32 -lcrypt32
	CFLAGS += -D_WIN32 -DNO_CGI -DNO_SSL_DL
else
	
endif

all: version $(TOOLS)

version:
	@echo $(GIT_VERSION)
	@git rev-parse HEAD | awk ' BEGIN {print "#include \"version.h\""} {print "const char * build_git_sha = \"" $$0"\";"} END {}' > version.c
	@date | awk 'BEGIN {} {print "const char * build_git_time = \""$$0"\";"} END {} ' >> version.c
	
$(TOOLS): %: %.o $(COMMON) $(DEPS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@$(SUFFIX) $< $(COMMON) $(LDLIBS)	
	
$(OBJS): %.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

test-suite: $(COMMON) $(DEPS)
	$(CC) -o test-suite.exe test/test_suite.c  $(COMMON) $(LDLIBS)

clean:
	-rm -f $(OBJS) $(TOOLS)
	-rm -rf ./cache
	-rm -rf ./library
	-rm minidragonfly$(SUFFIX)
