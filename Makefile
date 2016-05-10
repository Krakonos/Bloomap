CXXFLAGS=-std=c++11 -g -Wall -DDEBUG_STATS
CC=g++

OBJECTS=bloomfilter.o bloomapfamily.o bloomap.o murmur.o

all: tests

%.html : %.md
	asciidoc -o $@ $<

doc: README.html

tests: test test-intersection

test: $(OBJECTS) test.o
	$(CC) $(CXXLAGS) -o test $^

test-intersection: $(OBJECTS) test-intersection.o
	$(CC) $(CXXLAGS) -o test-intersection $^

check: test test-intersection
	./test 1000 10000 0.01
	./test-intersection  1000 1000 0.001 2
	./test-intersection  1000 10000 0.001 2

B_ITER=1000
B_PR=0.01
B_ORDER=0

benchmark: test-intersection
	./test-intersection  1${B_ORDER} 10${B_ORDER} ${B_PR} ${B_ITER} 2>&1 | grep ::
	./test-intersection  2${B_ORDER} 10${B_ORDER} ${B_PR} ${B_ITER} 2>&1 | grep ::
	./test-intersection  3${B_ORDER} 10${B_ORDER} ${B_PR} ${B_ITER} 2>&1 | grep ::
	./test-intersection  4${B_ORDER} 10${B_ORDER} ${B_PR} ${B_ITER} 2>&1 | grep ::
	./test-intersection  5${B_ORDER} 10${B_ORDER} ${B_PR} ${B_ITER} 2>&1 | grep ::
	./test-intersection  6${B_ORDER} 10${B_ORDER} ${B_PR} ${B_ITER} 2>&1 | grep ::
	./test-intersection  7${B_ORDER} 10${B_ORDER} ${B_PR} ${B_ITER} 2>&1 | grep ::
	./test-intersection  8${B_ORDER} 10${B_ORDER} ${B_PR} ${B_ITER} 2>&1 | grep ::
	./test-intersection  9${B_ORDER} 10${B_ORDER} ${B_PR} ${B_ITER} 2>&1 | grep ::
	./test-intersection 10${B_ORDER} 10${B_ORDER} ${B_PR} ${B_ITER} 2>&1 | grep ::

clean:
	rm -f test test-intersection *.o *.html


.PHONY: clean check