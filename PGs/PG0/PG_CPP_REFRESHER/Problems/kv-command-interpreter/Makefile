# run with 'make -i'
# Note: the large spaces are tabs, this is important
# Run GCOV, requires that we build the code
# All of these runs of ./prog are a test case
# We want to cover 100% of prog through these tests
run-tests-coverage: build
	./prog
	./prog missing-input.txt missing-output.txt
	./prog invalid-command.txt invalid-command-output.txt
	./prog malformed-input.txt malformed-output.txt
	./prog coverage-input.txt .
	./prog coverage-input.txt coverage-output.txt
	./prog example-input.txt example-output.txt
	gcov --relative-only prog.cpp
# Run Valgrind to find memory leaks, requires that we build the code
run-tests-leak: build
	valgrind --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-fds=yes ./prog
	valgrind --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-fds=yes ./prog missing-input.txt missing-output.txt
	valgrind --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-fds=yes ./prog invalid-command.txt invalid-command-output.txt
	valgrind --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-fds=yes ./prog malformed-input.txt malformed-output.txt
	valgrind --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-fds=yes ./prog coverage-input.txt .
	valgrind --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-fds=yes ./prog coverage-input.txt leak-output.txt
	valgrind --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-fds=yes ./prog example-input.txt example-leak-output.txt

# Compile the code, requires that we clean up some files first.
# Add –lm if your code needs the math library.
build: clean
	g++ -fprofile-arcs -ftest-coverage -g prog.cpp -o prog
clean:
	rm -f *~
	rm -f prog
	rm -f *.gcda *.gcno *.gcov
	rm -f coverage-output.txt leak-output.txt missing-output.txt
	rm -f example-output.txt example-leak-output.txt
	rm -f invalid-command-output.txt malformed-output.txt
