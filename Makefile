build:
	mpic++ tema3.cpp -lm -o main
run:
	mpirun -np 5 --oversubscribe main
clean:
	rm main
