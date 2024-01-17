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

using namespace std;

int typeOf(int rank) {
	/* 
		assign to class:
			SIEC noe 1 "Master"
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

int random(int min, int max) //range : [min, max]
{
	static bool first = true;
	if (first)
	{
		srand(time(NULL)); //seeding for the first time only!
		first = false;
	}
	return min + rand() % ((max + 1) - min);
}

int main(int argc, char* argv[]) {
	// helper variables
	srand(time(NULL));
	int ile_osob_w_glosowaniu = 0;
	int rank, size = 0, buf = 0, cnt = 0, glos = 0;
	int talibcaGlosujaca[100], iniUczestnicy = size, iterator = 0, losowaLiczba = 0;
	int ostatni_dodatni_glos = 0;
	int tablica_wynikowa[100], glosowanie=0;
	
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<int> dist(1, size);

	int listaDoGlosowania[100];
	int random_number;

	// INIT TASK
	switch (typeOf(rank)) {
	case SIEC:
		// Inicjalizacja tablicy do wyświetlania głosów
		for (int i = 0; i < 100; i++) {
			tablica_wynikowa[i] = 0; // nikt nie glosuje
		}
		// Wysłanie uczestników głosowania | inicjalizacja
		for (int i = 1; i < size; i++) {
			if (typeOf(i) == LICZ) {
				for (int j = 0; j < size; j++) {
					if (typeOf(j) == PROC) {
						MPI_Send(&j, 1, MPI_INT, i, i, MPI_COMM_WORLD); // kto głosuje?
						// printf("SIEC %d: %d wysyla do %d\n", rank, j, i);
					}
				}
			}
		}
		break;
	case LICZ:
		iniUczestnicy = (size - 1) / 2 - 1;
		for (int i = 1; i < size; i++) {
			talibcaGlosujaca[i] = 0; // nikt nie glosuje
		}
		break;
	case PROC:
		break;
	default:
		break;
	}

	// MAIN LOOP
	do {
		switch (typeOf(rank)) {
		case SIEC:
			for (int i = 0; i < size; i++)
			{
				if (typeOf(i) == PROC)
				{
					MPI_Recv(&buf, 1, MPI_INT, i, i, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
					//if (buf == -1) { // koniec głosowania
					//}
					//else { // zapisz statystykę otrzymanych głosów
					//	tablica_wynikowa[buf] += 1;
					//}
					for (int j = 0; j < size; j++) { // wyślij do wszystkich infromację o oddanym głosie
						if (typeOf(j) == LICZ) {
							MPI_Send(&buf, 1, MPI_INT, j, j, MPI_COMM_WORLD); // glos na
							// printf("SIEC %d: %d wysyla do %d\n", rank, buf, i);
						}
					}
				}
			}
			//printf("G%d:", glosowanie);

			//for (int i = 0; i < size; i++) // wyświetlanie wyników w postaci histogramu
			//{
			//	if (typeOf(i) == PROC) {
			//		printf(" P%d:%d\t",i, tablica_wynikowa[i]);
			//		tablica_wynikowa[i] = 0;
			//	}
			//}
			//glosowanie++;
			//printf("\n");

			break;
		case LICZ:
			do {
				MPI_Recv(&buf, 1, MPI_INT, SIEC, rank, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
				if (rank == 1) {
					printf("LICZ %d: Otrzymalem %d\n", rank, buf);
				}
				if (buf == -1){
					break;
				}
				else {
					//if (talibcaGlosujaca[buf]>=0) {
						talibcaGlosujaca[buf] = talibcaGlosujaca[buf] + 1;
					//}
				}
			} while (iterator++ < iniUczestnicy);
			
			if (rank == 1) {
				printf("G%d:", glosowanie);
				for (int i = 0; i < size; i++) { // wyświetlanie wyników w postaci histogramu
					if (typeOf(i) == PROC) {
						printf(" P%d:%d\t", i, talibcaGlosujaca[i]);
					}
				}
				glosowanie++;
				printf("iterator: %d\n", iterator);
				//printf("LICZ %d: Wysylam do %d\n", rank, rank + 1);
			}
			// TODO: Dopracowanie selekccji glosujacych
			for (int i = 1; i < size; i++) {
				if (talibcaGlosujaca[i] > 0) {
					talibcaGlosujaca[i] = 0;
				}
				else {
					talibcaGlosujaca[i] = -1;
				}
			}
			iterator = 0;
			MPI_Send(talibcaGlosujaca, 100, MPI_INT, rank + 1, rank + 1, MPI_COMM_WORLD);
			break;
		case PROC:
			MPI_Recv(listaDoGlosowania, 100, MPI_INT, rank - 1, rank, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			if (rank == 2) {
				//printf("PROC %d: Otrzymalem liste\n", rank);
			}
			do {
				do
				{
					random_number = dist(gen);
				} while (typeOf(random_number)==PROC);

				//random_number = random(1, size);
			} while (listaDoGlosowania[random_number] >= 0 && random_number != rank);
			printf("Glos %d na %d\n", rank, random_number);

			ile_osob_w_glosowaniu = 0;
			for (int i = 0; i < size; i++) {
				if (listaDoGlosowania[i] >= 0) {
					ile_osob_w_glosowaniu++;
					ostatni_dodatni_glos = i;
				}
			}
			// TODO: Jeżeli głosujący zauważy, że jest za mało osób do głosowania, zwraca -1
			if (ile_osob_w_glosowaniu <= 1) {
				buf = -1;
				random_number = -1;
			}
			else {
				buf = 10;
			}
			MPI_Send(&random_number, 1, MPI_INT, SIEC, rank, MPI_COMM_WORLD);
			if (rank == 2) {
				printf("iloscOsob: %d\n", ile_osob_w_glosowaniu);
				if (ile_osob_w_glosowaniu == 1) {
					printf("Wygrywa: %d\n", ostatni_dodatni_glos);
				}
			}

			//printf("PROC %d: ilosc_osob: %d, wybiera: %d\n", rank, ile_osob_w_glosowaniu, random_number+1);
			if (rank == 2 && random_number == -1) {
				//printf("Wynik glosowania: %d\n", ostatni_dodatni_glos+1);
			}
			break;
		default:
			break;
		}
		
	} while (buf > 0);
	

	// printf("Task: %d Konczy zadanie.\n", rank);
	MPI_Finalize();
	return 0;
}