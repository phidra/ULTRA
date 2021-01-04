CC=clang-9  # ln -s /home/pdrabczuk/Logiciels/clang+llvm-9.0.1-x86_64-linux-gnu-ubuntu-16.04/bin/clang-9 /home/pdrabczuk/.local/bin
FLAGS=-std=c++17 -fopenmp -pipe -stdlib=libc++
LINKFLAGS=-lc++abi -lc++ -lm
OPTIMIZATION=-march=native -O3
DEBUG=-rdynamic -Werror -Wpedantic -pedantic-errors -Wall -Wextra -Wparentheses -Wfatal-errors -D_GLIBCXX_DEBUG -g -fno-omit-frame-pointer
RELEASE=-ffast-math -ftree-vectorize -Wfatal-errors -DNDEBUG

all: BuildBucketCH BuildCoreCH ComputeShortcuts RunCSAQueries RunRAPTORQueries

clean:
	rm -f BuildBucketCH BuildCoreCH ComputeShortcuts RunCSAQueries RunRAPTORQueries 

BuildBucketCH:
	$(CC) $(FLAGS) $(OPTIMIZATION) $(RELEASE) -o BuildBucketCH BuildBucketCH.cpp $(LINKFLAGS)

BuildCoreCH:
	$(CC) $(FLAGS) $(OPTIMIZATION) $(RELEASE) -o BuildCoreCH BuildCoreCH.cpp $(LINKFLAGS)

ComputeShortcuts:
	$(CC) $(FLAGS) $(OPTIMIZATION) $(RELEASE) -o ComputeShortcuts ComputeShortcuts.cpp $(LINKFLAGS)

RunCSAQueries:
	$(CC) $(FLAGS) $(OPTIMIZATION) $(RELEASE) -o RunCSAQueries RunCSAQueries.cpp $(LINKFLAGS)
	
RunRAPTORQueries:
	$(CC) $(FLAGS) $(OPTIMIZATION) $(RELEASE) -o RunRAPTORQueries RunRAPTORQueries.cpp $(LINKFLAGS)
