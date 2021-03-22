#include "mpi.h"
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <iterator> 
#include <map> 
#include <atomic>
using namespace std;


#define MASTER 0
#define NUM_THREADS_MASTER 4
#define INITIAL_SIZE 1000

char fin_name[20];
char fout_name[20]; 
std::atomic<int> current(1);

// Prelucrare paragraf HORROR
char* double_consonants(char* str)
{
	char* result_str = (char*) calloc(2 * strlen(str), sizeof(char));

	for(int i = 0, j = 0; i < strlen(str); i++, j++) {
		char c = str[i];
		result_str[j] = c;

		/*  Daca intalnim o consoana minuscula, aceasta se dubleaza */
		if(c != 'a' && c != 'e' && c != 'i' && c != 'o' && c != 'u' && (c>= 'a' && c <= 'z')) {
			result_str[j + 1] = c;
			j++;
		}
		/*  Daca intalnim o consoana majuscula, aceasta se dubleaza in minuscula */
		else if (c != 'A' && c != 'E' && c != 'I' && c != 'O' && c != 'U' && (c>= 'A' && c <= 'Z')) {
			result_str[j + 1] = c + 32;
			j++;
		}
	}


	return result_str;
}

// Prelucrare paragraf COMEDY
char* uppercase_even_pos(char* str)
{
	int count = 0;

    for (int i = 0; i < strlen(str); i++) {
    	/* Cand intalnim spatiu sau \n se reseteaza cursorul */
    	if (str[i] == ' ' || str[i] == '\n') {
    		count = 0;
    	}

    	/* Se transforma in majuscule literele de pe pozitii pare in cuvant */
    	else {
    		count++;
    		if(count % 2 == 0 && str[i] >= 'a' && str[i] <= 'z') 
    			str[i] = toupper(str[i]);
    	}
    }
    
    return str;
}


// Prelucrare paragraf FANTASY
char* uppercase_first_letter(char* str)
{
	/* Cream o copie a string-ului initial*/
	char* result_str = (char*) calloc(strlen(str) + 1, sizeof(char));
	memcpy(result_str, str, strlen(str));


    /* Impartim string-ul in cuvinte */
    char *token = strtok(str, " ,.?\n");
    while(token != NULL) 
    {
    	char* pointer = strstr(result_str, token);

    	/* Transformam prima litera din cuvant in majuscula */
    	if(token[0] >= 'a' && token[0] <= 'z')
    		token[0] = toupper(token[0]);

    	/* Inlocuim cuvantul in string-ul initial*/
    	if(pointer != NULL)
    		strncpy(pointer, token, strlen(token));

        token = strtok(NULL, " ,.?\n");
    }

    return result_str;
}


// Prelucrare paragraf SCIENCE-FICTION
char* reverse_7th_word(char* str) 
{
	char word[30];
	int count = 0;

	for (int i = 0, j = 0; i <= strlen(str); i++) {

		if (str[i] == ' ' || str[i] == '\n' || str[i] == '\0') {
			count++;
			/* Se inverseaza al 7-lea cuvant */
			if (count % 7 == 0) {
				int len = strlen(word);
				for(int k = 0; k < len; k++)
        			str[i - k - 1] = word[k];
			}

			/* Se reseteaza cuvantul curent */
			else {
				memset(word, 0, strlen(word));
				j = 0;
			}
			if (str[i] == '\n') {
				memset(word, 0, strlen(word));
				j = 0;
				count = 0;
			}
		}

		/* Se formeaza cuvantul curent */
		else {
			word[j] = str[i];
			j++;
		}
	}

	return str;
}

// Se primeste paragraful procesat de la worker si se salveaza in map
void recv_from_worker(int worker, int num, std::map<int, char*> &paragraphs) {
	MPI_Status status;

	int len = 0;
	MPI_Recv(&len, 1, MPI_INT, worker, 1, MPI_COMM_WORLD, &status);

	char* processed = (char*) calloc(len + 1, sizeof(char));
	memset(processed, 0, len + 1);
	MPI_Recv(processed, len, MPI_CHAR, worker, 1, MPI_COMM_WORLD, &status);

	paragraphs.insert(std::pair<int, char*>(num, processed));
}



void *thread_function(void *arg) 
{
	// Se obtine id-ul thread-ului curent
	int thread_id = *(int *)arg;

    int read, len, worker, num_paragraphs;
    size_t size;
    
    char* line;
    char* paragraph;
    paragraph = (char*) calloc(INITIAL_SIZE, sizeof(char));
    std::map<int, char*> paragraphs;


	FILE *in_file = fopen(fin_name, "r");
	if (in_file == NULL) {
    	fprintf(stderr, "ERROR: Can't open input file in MASTER");
		exit(-1);
	}


	worker = -1;
    
    // Se citeste linie cu linie din fisier si se formeaza paragrafele
	while ((read = getline(&line, &size, in_file)) != -1) {
		// Se numara cate paragrafe sunt in fisier
		if (strcmp(line, "science-fiction\n") == 0 || strcmp(line, "fantasy\n") == 0 ||
			strcmp(line, "horror\n") == 0 || strcmp(line, "comedy\n") == 0)
			num_paragraphs++;

		// Se afla workerul care se va ocupa de paragraf
		if(strcmp(line, "horror\n") == 0 && thread_id == 0) 
			worker = 1;
		else if(strcmp(line, "comedy\n") == 0 && thread_id == 1) 
			worker = 2;
		else if(strcmp(line, "fantasy\n") == 0 && thread_id == 2) 
			worker = 3;
		else if(strcmp(line, "science-fiction\n") == 0 && thread_id == 3)
			worker = 4;

		// S-a format paragraful si se trimite la worker
		else if(strcmp(line, "\n") == 0 && thread_id == worker - 1) {
			// Se trimite dimensiunea si paragraful la worker
			len = strlen(paragraph);
			MPI_Send(&len, 1, MPI_INT, worker, 1, MPI_COMM_WORLD);
			MPI_Send(paragraph, len, MPI_CHAR, worker, 1, MPI_COMM_WORLD);

			/* Se primeste paragraful procesat de la worker si se adauga
			   in map-ul paragraphs cu cheia "num_paragraphs" */
			recv_from_worker(worker, num_paragraphs, paragraphs);

			// Se reseteaza paragraful pentru citirea urmatoare
			free(paragraph);
			paragraph = (char*) calloc(INITIAL_SIZE, sizeof(char));
			worker = -1;
		}

		// Se concateneaza linia la paragraful curent
		else if(thread_id == worker-1) {
			int size = strlen(paragraph) + strlen(line) + 10;
			if(size > INITIAL_SIZE)
				paragraph = (char*) realloc(paragraph, size * sizeof(char));
			strcat(paragraph, line);
		}

	}
	
	// Daca fisierul nu se termina in \n, procesam si ultimul paragraf 
	if(read == -1 && thread_id == worker-1) {
		len = strlen(paragraph);
		MPI_Send(&len, 1, MPI_INT, worker, 1, MPI_COMM_WORLD);
		MPI_Send(paragraph, len, MPI_CHAR, worker, 1, MPI_COMM_WORLD);

		recv_from_worker(worker, num_paragraphs, paragraphs);
	}

	free(paragraph);
	fclose(in_file);


	worker = thread_id + 1;
	len = -1;
	// Se trimite -1 pentru a anunta workerul ca s-a citit tot din fisier
	MPI_Send(&len, 1, MPI_INT, worker, 1, MPI_COMM_WORLD);




	/* Variabila current retine numarul paragrafului curent si asigura
	   scrierea in ordine in fisierul de iesite */
	//std::map<char,int>::iterator it;
	while (current <= num_paragraphs) {
		auto it = paragraphs.find(current);
		if (it != paragraphs.end()) {
			// Se scrie pe rand fiecare paragraf in fisierul de iesire
			FILE *out_file = fopen(fout_name, "a");
			if (out_file == NULL) {
		    	fprintf(stderr, "ERROR: Can't open output file in MASTER");
				exit(-1);
			}

			// Se scrie tipul paragrafului
			if(worker == 1) 
				fprintf(out_file, "horror\n");
			else if(worker == 2) 
				fprintf(out_file, "comedy\n");
			else if(worker == 3) 
				fprintf(out_file, "fantasy\n");
			else if(worker == 4) 
				fprintf(out_file, "science-fiction\n");

			// Se scrie paragraful
			fprintf(out_file,"%s\n", paragraphs.at(current));

			fclose(out_file);
			paragraphs.erase(it);
			current++;
		}
	}


	// Se elibereaza spatiul alocat
	map<int, char*>::iterator itr; 
	for (itr = paragraphs.begin(); itr != paragraphs.end(); ++itr)
		free(itr->second);
	paragraphs.clear();

	pthread_exit(NULL);
}


int main (int argc, char *argv[])
{
    int  numtasks, rank, len, provided;
    MPI_Status status;

    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);


    // Se obtine numele fisierului de intrare
    memcpy(fin_name, argv[1], strlen(argv[1]));

    // Se prelucreaza numele fisierului de iesire
    memcpy(fout_name, fin_name, strlen(fin_name) - 4);
    strcat(fout_name, ".out");

    
    if (rank == MASTER) {
    	pthread_t threads[NUM_THREADS_MASTER];
		int thread_id[NUM_THREADS_MASTER ];

    	/* Se pornesc thread-urile din Master care citesc paragrafele si le trimit
    	   la workeri, dupa care le primesc inapoi prelucrare si le scriu in fisier */
		for (int i = 0; i < NUM_THREADS_MASTER ; i++) {
			thread_id[i] = i;
			pthread_create(&threads[i], NULL, thread_function, &thread_id[i]);
		}

		for (int i = 0; i < NUM_THREADS_MASTER ; i++) {
			pthread_join(threads[i], NULL);
		}
		
    }

    else {
		int len;

		while (1) {
			// Se primeste dimensiunea paragrafului
			MPI_Recv(&len, 1, MPI_INT, MASTER, 1, MPI_COMM_WORLD, &status);
			// Daca se primeste -1 citirea s-a terminat
			if(len == -1) {
				break;
			}

			// Se primeste paragraful de prelucrat
			char* paragraph = (char*) calloc(len + 1, sizeof(char));
			memset(paragraph, 0, len + 1);
			MPI_Recv(paragraph, len, MPI_CHAR, MASTER, 1, MPI_COMM_WORLD, &status);


			// Worker-ul proceseaza paragraful in mod corespunzator
			char* processed;
			if(rank == 1)		// paragraf horror
				processed = double_consonants(paragraph);
			else if(rank == 2)	// paragraf comedy
				processed = uppercase_even_pos(paragraph);
			else if(rank == 3)	// paragraf fantesy
				processed = uppercase_first_letter(paragraph);
			else if(rank == 4)	// paragraf sf
				processed = reverse_7th_word(paragraph);
			

			// Se trimite inapoi la Master dimensiunea si paragraful procesat
			len = strlen(processed);
			MPI_Send(&len, 1, MPI_INT, MASTER, 1, MPI_COMM_WORLD);
			MPI_Send(processed, len, MPI_CHAR, MASTER, 1, MPI_COMM_WORLD);

			free(paragraph);
		}
    }
    

    MPI_Finalize();
  }