#

runtest: options.o test_options.o
	cc -o runtest test_options.o options.o

options.o: options.c options.h harness.h
	cc -DUNIT_TEST -c options.c

test_options.o: test_options.c options.h harness.h
	cc -DUNIT_TEST -c test_options.c

clean:
	rm -f runtest *.o


