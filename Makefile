all: Jot.exe

Jot.exe: Jot.cpp
	g++ -std=c++17 -O2 -o Jot.exe Jot.cpp

clean:
	del /f Jot.exe 2>nul || (if exist Jot.exe del /f Jot.exe)