CC       = gcc
CFLAGS   = -Wall -Wextra -std=c11 -Isrc
DBGFLAGS = -g -DDEBUG

SRCS = src/graphe.c src/liste_chainee.c src/dijkstra.c \
       src/securite.c src/utils.c src/api.c src/main.c

TARGET = netflow

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) -lm
	@echo "[OK] $(TARGET) compilé."

debug: $(SRCS)
	$(CC) $(CFLAGS) $(DBGFLAGS) -o $(TARGET)_debug $(SRCS) -lm

test:
ifeq ($(OS),Windows_NT)
	$(CC) $(CFLAGS) -o tests/run_tests.exe \
	    src/graphe.c src/liste_chainee.c src/dijkstra.c \
	    src/securite.c src/utils.c tests/tests_unitaires.c -lm
	tests\run_tests.exe
else
	$(CC) $(CFLAGS) -o tests/run_tests \
	    src/graphe.c src/liste_chainee.c src/dijkstra.c \
	    src/securite.c src/utils.c tests/tests_unitaires.c -lm
	./tests/run_tests
endif

clean:
ifeq ($(OS),Windows_NT)
	-del /f /q $(TARGET).exe $(TARGET)_debug.exe tests\run_tests.exe 2>nul
	-del /f /q $(TARGET) $(TARGET)_debug 2>nul
else
	rm -f $(TARGET) $(TARGET).exe $(TARGET)_debug tests/run_tests
endif

install:
	pip install flask flask-cors

web: $(TARGET)
	python server.py

test-api: $(TARGET)
ifeq ($(OS),Windows_NT)
	@echo {"cmd":"dijkstra","src":0,"dst":7,"metric":"latence"} | $(TARGET).exe --api
	@echo {"cmd":"cycles"} | $(TARGET).exe --api
else
	@echo '{"cmd":"dijkstra","src":0,"dst":7,"metric":"latence"}' | ./$(TARGET) --api
	@echo '{"cmd":"cycles"}' | ./$(TARGET) --api
endif

.PHONY: all debug test clean install web test-api