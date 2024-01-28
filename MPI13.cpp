/*
 * File: MPI13.cpp
 * Project: Lab_MPI
 * File Created: Wednesday, 17th January 2024 17:02:21
 * Author: Maciej Żyrek (macizyr350@student.polsl.pl), Maciej Węcki (maciwec227@student.polsl.pl)
 * -----
 * Last Modified: Sunday, 28th January 2024 00:27:55
 * Modified By: Maciej Żyrek (macizyr350@student.polsl.pl>)
 * -----
 */


#include <mpi.h>
#include <stdio.h>
#include <mpi.h>
#include <stdio.h>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <random>

#define SIEC 0
#define LICZ 1
#define PROC 2

#define DEFAULT_TAG 36
#define OUT_OF_VOTING -1
#define EXIT_VAR -2

using namespace std;

int typeOf(int rank) {
	/* 
		assign to class:
			SIEC noe 0 "Master"
			LICZ noe 1,3,5 ...
			PROC noe 2,4,6 ...
	*/
	if (rank == SIEC) {
		return SIEC;
	}
	else if ((rank - 1) % 2 == 0) {
		return LICZ;
	}
	else {
		return PROC;
	}
}

int vote(int size_array, int* pt_array) {
	// Check no. candidates
	int check = 0;
	for (int i = 0; i < size_array; i++) {
		if (pt_array[i] > OUT_OF_VOTING) check++; // count elements greater than -1
	}	if (check <= 1) return EXIT_VAR; // if there is less than two candidates return -1
	// Setup random number generator
	srand(time(NULL));
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<int> dist(1, size_array);
	// Vote for candidate
	int voted_id = 0;
	while ((typeOf(voted_id) != PROC) || (pt_array[voted_id] < 0)) voted_id = dist(gen);
	return voted_id;
}

int main(int argc, char* argv[]) {

	int rank, size;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	// Secure abd input parameters
	if (size <= 5 || size % 2 == 0) {
		printf("Invalid input. Please enter a number greater than 5 and odd.\n");
		MPI_Abort(MPI_COMM_WORLD, 1);
		MPI_Finalize();
		return 0;
	}
	
	const int c_size = size;
	int* voting_array = 0;
	bool loop_breaker = false;
	int max = 0, min_vote_lvl;

	char output[500] = "";

	// INIT TASK
	switch (typeOf(rank)) {
	case SIEC: // Send to all LICZ process first 
		break;
	case LICZ: // Setup members rights to voting (0 and above can, below can't voting)
		voting_array = new int[size] {0};
		for (int i = 0; i < size; i++) if (typeOf(i) != PROC) voting_array[i] = OUT_OF_VOTING;
			MPI_Send(voting_array, size, MPI_INT, rank+1, DEFAULT_TAG, MPI_COMM_WORLD);
		break;
	case PROC:
		voting_array = new int[size] {0};
		break;
	default:
		break;
	}
	// MAIN LOOP
	for (int xd = 0; xd < 30 && !loop_breaker; xd ++)
	{

		switch (typeOf(rank)) {
		case SIEC:
			int siec_collector;
			for (int i = 0; i < size; i++) if (typeOf(i) == PROC) { // collect vote from all participants
				MPI_Recv(&siec_collector, 1, MPI_INT, i, i, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				for (int j = 0; j < size; j++) if (typeOf(j) == LICZ) { // send to all participants information about vote
					MPI_Send(&siec_collector, 1, MPI_INT, j, DEFAULT_TAG, MPI_COMM_WORLD);
				}
				if (siec_collector == EXIT_VAR) loop_breaker = true;
				sprintf_s(output + strlen(output), sizeof(output) - strlen(output), "K%d:%d, ", i, siec_collector);
			}

			// cout << output << "\t glosowanie: " << max++ << endl;
			memset(output, 0, sizeof(output));
			MPI_Barrier(MPI_COMM_WORLD);
			break;
		case LICZ:
			int licz_collector;
			for (int j = 0; j < size; j++) if (typeOf(j) == LICZ) { // Collect votes 
				MPI_Recv(&licz_collector, 1, MPI_INT, SIEC, DEFAULT_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				if (licz_collector > OUT_OF_VOTING) // if is grater than -1
					voting_array[licz_collector]++; // Store in array
				if (licz_collector == EXIT_VAR) { loop_breaker = true; }
			}

			min_vote_lvl = size;	// Find task minimum
			for (int i = 1; i < size; i++) {
				if (min_vote_lvl > voting_array[i] && voting_array[i] > 0 ) {
					min_vote_lvl = voting_array[i];
				}
			}
			//if (rank == 1) { // printout information about voting
			//	
			//	for (int i = 0; i < size; i++) {
			//		//if (voting_array[i] != -1)
			//			sprintf_s(output + strlen(output), sizeof(output) - strlen(output), "K%d: %s, %d,\t", i, (voting_array[i] <= min_vote_lvl) ? "F" : "A", voting_array[i]);
			//	}
			//	cout << "L" << rank << " " << output << "glosowannie: " << max++ << endl;
			//	memset(output, 0, sizeof(output));
			//}

			for (int i = 1; i < size; i++) {
				if (voting_array[i] > min_vote_lvl) {
					voting_array[i] = 0;
				}
				else voting_array[i] = OUT_OF_VOTING;
			}

			MPI_Barrier(MPI_COMM_WORLD);
			MPI_Send(voting_array, size, MPI_INT, rank + 1, DEFAULT_TAG, MPI_COMM_WORLD);
			break;
		case PROC:
			int my_candidate;
			MPI_Recv(voting_array, size, MPI_INT, rank-1, DEFAULT_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			my_candidate = vote(size, voting_array);
			// cout << my_candidate << endl;
			MPI_Send(&my_candidate, 1, MPI_INT, SIEC, rank, MPI_COMM_WORLD);
			// cout << "PROC MPI before" << endl;
			if (rank == 2) { // printout information about voting

				for (int i = 0; i < size; i++) {
					if (voting_array[i] != OUT_OF_VOTING)
						sprintf_s(output + strlen(output), sizeof(output) - strlen(output), " K%d: %s, %d, ", i, (voting_array[i] < 0) ? "F" : "A", voting_array[i]);
				}
				cout << output << ", voted:" << my_candidate << "at " << max++ << endl;
				memset(output, 0, sizeof(output));
			}
			MPI_Barrier(MPI_COMM_WORLD);
			if (my_candidate == EXIT_VAR) { loop_breaker = true; }
			break;
		default:
			break;
		}
		MPI_Barrier(MPI_COMM_WORLD);
	}

	MPI_Finalize();
	return 0;
}