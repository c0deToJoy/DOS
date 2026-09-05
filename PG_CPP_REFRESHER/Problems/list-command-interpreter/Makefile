# run with 'make -i'
# Note: the large spaces are tabs, this is important
# Run GCOV, requires that we build the code
# All of these runs of ./prog are a test case
# We want to cover 100% of prog through these tests
run-tests-coverage: build
	./prog
	./prog missing-input.txt missing-output.txt
	./prog invalid-input.txt invalid-output.txt
	./prog invalid-command.txt invalid-command-output.txt
	./prog malformed-front.txt malformed-front-output.txt
	./prog malformed-back.txt malformed-back-output.txt
	./prog coverage-bounded.txt .
	./prog coverage-bounded.txt bounded-output.txt
	./prog coverage-unbounded.txt unbounded-output.txt
	./prog example-unbounded.txt example-unbounded-output.txt
	./prog example-bounded.txt example-bounded-output.txt
	gcov --relative-only prog.cpp
run-tests-leak: build
	valgrind --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-fds=yes ./prog
	valgrind --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-fds=yes ./prog missing-input.txt missing-output.txt
	valgrind --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-fds=yes ./prog invalid-input.txt invalid-output.txt
	valgrind --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-fds=yes ./prog invalid-command.txt invalid-command-output.txt
	valgrind --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-fds=yes ./prog malformed-front.txt malformed-front-output.txt
	valgrind --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-fds=yes ./prog malformed-back.txt malformed-back-output.txt
	valgrind --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-fds=yes ./prog coverage-bounded.txt .
	-valgrind --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-fds=yes ./prog coverage-bounded.txt bounded-leak-output.txt
	-valgrind --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-fds=yes ./prog coverage-unbounded.txt unbounded-leak-output.txt

# Compile the code, requires that we clean up some files first.
# Add –lm if your code needs the math library.
build: clean
	g++ -fprofile-arcs -ftest-coverage -g prog.cpp -o prog
clean:
	rm -f *~
	rm -f prog
	rm -f *.gcda *.gcno *.gcov
	rm -f bounded-output.txt unbounded-output.txt bounded-leak-output.txt unbounded-leak-output.txt
	rm -f example-unbounded-output.txt example-bounded-output.txt
	rm -f missing-output.txt invalid-output.txt
	rm -f invalid-command-output.txt malformed-front-output.txt malformed-back-output.txt
