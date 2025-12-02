all: Jot.exe

Jot.exe: main.cpp fileio.cpp undo.cpp util.cpp display.cpp input.cpp editor.cpp
	g++ -std=c++17 -O2 -o Jot.exe main.cpp fileio.cpp undo.cpp util.cpp display.cpp input.cpp editor.cpp

clean:
	del /f Jot.exe 2>nul || (if exist Jot.exe del /f Jot.exe)