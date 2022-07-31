EXECUTABLE=testinterpreter
CC=g++
OBJ_PATH=obj
ifndef OS
OS=$(shell uname)
OSMINOR=$(shell uname -m)
endif

ifeq ($(OS),Darwin)		
  LOCAL_PATH=..
  GAME_PATH=.
  SEP=/
  CN=rm -rf 
  CFLAGS=-c -Wall -Wno-address-of-packed-member -F/Library/Frameworks
  LDFLAGS=-F/Library/Frameworks 
else ifeq ($(OS),Linux)
  LOCAL_PATH=..
  GAME_PATH=.
  LDFLAGS=
  CFLAGS=-c -Wall -Wno-unused-result -O3 
  SEP=/
  CN=rm -rf
else ifeq ($(OS),Windows_NT)
  LOCAL_PATH=..
  GAME_PATH=.
  SEP=\\
  CN=del /q
  CFLAGS=-c -Wall -O3
  LDFLAGS=
endif

INCFLAGS=-I$(LOCAL_PATH) -I$(GAME_PATH) 

SOURCES=$(GAME_PATH)$(SEP)pythonmain.cpp 
SOURCES+=$(LOCAL_PATH)$(SEP)interpreter.cpp $(LOCAL_PATH)$(SEP)binaryparser.cpp $(LOCAL_PATH)$(SEP)pythonparser.cpp  

OBJECTS=$(SOURCES:.cpp=.o)
OBJECTS+=$(CSOURCES:.c=.o)

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(addprefix $(OBJ_PATH)$(SEP),$(notdir $(OBJECTS))) -o $@ $(LDFLAGS)

.cpp.o:
	$(CC) $(INCFLAGS) $(CFLAGS) $< -o $(addprefix $(OBJ_PATH)$(SEP),$(notdir $@))
.c.o:
	$(CC) $(INCFLAGS) $(CFLAGS) $< -o $(addprefix $(OBJ_PATH)$(SEP),$(notdir $@))
	
clean:
	$(CN) $(OBJ_PATH)$(SEP)*.o
