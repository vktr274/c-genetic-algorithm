//#define _CRTDBG_MAP_ALLOC

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

//#include <crtdbg.h>

/*
 * Makro funkcia na overenie alokovania bloku pamate.
 */
#define CHECK_ALLOC_ERR(ptr)		\
	if (ptr == NULL) {				\
		exit(SIGSEGV);				\
	}								\

/*
 * Velkost bufferu na nacitavanie zo suboru
 */
#define BUF_SIZE			50

/* 
 * -1 je kamen na mape zahrady
 */
#define OBSTACLE			-1

/*
 * Smery pohybu mnicha na mape
 */
#define RIGHT				'r'
#define LEFT				'l'
#define UP					'u'
#define DOWN				'd'

/* 
 * Hodnoty boolean
 */
#define TRUE				'1'
#define FALSE				'0'

/* 
 * # oznacuje komentar v input.txt
 */
#define COMMENT				'#'

/*
 * Definovanie vlastnych typov.
 */
typedef unsigned short	u_short;
typedef char			boolean;

/*
 * Struktura pre uchovanie informacii o zen zahrade.
 */
typedef struct zen {
	short** map;
	u_short width;
	u_short height;
	u_short obstacle_count;
} Zen;

/*
 * Struktura pre uchovanie informacii o genoch.
 */
typedef struct gene {
	u_short index;
	char	direction_on_map;
	char	priority_move;
} Gene;

/*
 * Struktura pre uchovanie informacii o chromozome.
 */
typedef struct chromosome {
	Gene**	genes;
	u_short gene_count;
} Chromosome;

/*
 * Struktura pre populaciu.
 */
typedef struct population {
	Chromosome** chromosomes;
	u_short		 chromosome_count;
} Population;

/* 
 * Flag na oznacenie uviaznutia mnicha.
 */
boolean	flag = FALSE;

/*
 * Premenna na ulozenie poradia hrabania.
 */
short	order = 1;

/*
 * Funkcia na vypocet poctu genov v jednom chromozome.
 * Pocet genov je pseudonahodny v intervale <max/2, max>.
 * max = maximalny pocet genov podla mapy.
 */
u_short calc_gene_count(Zen* zen) {
	u_short max_count,
			min_count,
			between_min_max;

	max_count		=	zen->width + zen->height + zen->obstacle_count;
	min_count		=	max_count / 2;
	between_min_max =	(rand() % (max_count - min_count + 1)) + min_count;

	return between_min_max;
}

/*
 * Funkcia na alokovanie pamate pre mapu zahrady.
 */
short** map_alloc(u_short width, u_short height) {
	u_short i;

	short** new_map;

	new_map = (short**)malloc(height * sizeof(short*));
	CHECK_ALLOC_ERR(new_map);

	for (i = 0; i < height; i++) {
		new_map[i] = (short*)malloc(width * sizeof(short));
		CHECK_ALLOC_ERR(new_map[i]);
	}
	return new_map;
}

/*
 * Funkcia na alokovanie struktury pre zahradu
 * a na inicializaciu jej hodnot.
 */
Zen* zen_alloc(u_short width, u_short height) {
	Zen*	new_zen = NULL;

	short** new_map = NULL;

	new_zen = (Zen*)malloc(sizeof(Zen));
	CHECK_ALLOC_ERR(new_zen);

	new_map = map_alloc(width, height);

	new_zen->map			=	new_map;
	new_zen->width			=	width;
	new_zen->height			=	height;
	new_zen->obstacle_count =	0;

	return new_zen;
}

/*
 * Funkcia na uvolnenie mapy.
 */
void map_free(short** map, Zen* zen) {
	u_short i;

	for (i = 0; i < zen->height; i++) {
		free(map[i]);
	}
	free(map);
}

/*
 * Funkcia pre inicializaciu kazdeho policka zahrady na 0.
 */
short** map_init(short** map, u_short width, u_short height) {
	u_short i,
			j;

	for (i = 0; i < height; i++) {
		for (j = 0; j < width; j++) {
			map[i][j] = 0;
		}
	}

	return map;
}

/*
 * Funkcia na vytvorenie zahrady podla udajov v subore input.txt.
 */
Zen* fill_map(Zen* zen) {
	u_short line,
			column,
			width,
			height;

	FILE*	input;

	char	buf[BUF_SIZE];

	int		c;

	input = fopen("input.txt", "r");
	if (input == NULL) {
		return NULL;
	}
	
	while ((c = fgetc(input)) != EOF) {
		// Riadky s komentarmi sa preskakuju
		if (c == COMMENT) {
			fgets(buf, BUF_SIZE, input);
		}

		// Nacitanie rozmerov mapy zahrady
		if (c == '(') {
			fscanf(input, "%hu", &width);
			fgetc(input);
			fscanf(input, "%hu", &height);

			zen = zen_alloc(width, height);
			zen->map = map_init(zen->map, width, height);
		}

		// Nacitavanie suradnic prekazok (kamenov)
		if (c == '{') {
			while ((c = fgetc(input)) != '}') {
				if (c == '[') {
					fscanf(input, "%hu", &line);
					fgetc(input);
					fscanf(input, "%hu", &column);

					zen->map[line][column] = OBSTACLE;
					zen->obstacle_count++;
				}
			}
		}
	}

	if (fclose(input) == EOF) {
		return NULL;
	}

	return zen;
}

/*
 * Funkcia na formatovany vypis mapy zahrady.
 */
void print_map(Zen* zen) {
	u_short i,
			j;

	for (i = 0; i < zen->height; i++) {
		for (j = 0; j < zen->width; j++) {
			if (zen->map[i][j] == OBSTACLE) {
				printf("  K");
			}
			else {
				printf("%3hu", zen->map[i][j]);
			}
		}
		putchar('\n');
	}
}

/*
 * Funkcia na overenie ci gen s danym indexom v chromozome
 * uz existuje (TRUE) alebo neexistuje (FALSE).
 */
boolean is_gene(Chromosome* chromosome, Gene* new_gene, u_short gene_count) {
	u_short i;

	for (i = 0; i < gene_count; i++) {
		if (chromosome->genes[i]->index == new_gene->index) {
			return TRUE;
		}
	}

	return FALSE;
}

/*
 * Funkcia na alokovanie pamate pre chromozom a jeho pole genov.
 */
Chromosome* chromosome_alloc(u_short gene_count) {
	Chromosome* new_chromosome = NULL;

	new_chromosome = (Chromosome*)malloc(sizeof(Chromosome));
	CHECK_ALLOC_ERR(new_chromosome);

	new_chromosome->genes = (Gene**)malloc(gene_count * sizeof(Gene*));
	CHECK_ALLOC_ERR(new_chromosome->genes);

	new_chromosome->gene_count = gene_count;

	return new_chromosome;
}

/*
 * Funkcia na alokovanie pamate pre gen.
 */
Gene* gene_alloc(u_short index, char priority_move) {
	Gene* new_gene = NULL;

	new_gene = (Gene*)malloc(sizeof(Gene));
	CHECK_ALLOC_ERR(new_gene);

	new_gene->index = index;
	new_gene->priority_move = priority_move;

	return new_gene;
}

/*
 * Funkcia na vytvorenie chromozomu s nahodnymi genmi v protnej populacii.
 * Ak sa nahodou vytvori gen s rovnakym indexom ako index uz existujuceho genu,
 * tak sa najde pre dany gen iny index.
 */
Chromosome* initial_chromosome(u_short gene_count, Zen* zen) {
	Chromosome* new_chromosome = NULL;

	Gene*		new_gene = NULL;

	char		movements[] = { LEFT, RIGHT };

	u_short		i,
				width,
				height,
				perimeter;

	new_chromosome = chromosome_alloc(gene_count);

	width	  = zen->width;
	height	  = zen->height;
	perimeter = 2 * width + 2 * height;

	for (i = 0; i < gene_count; i++) {
		// Generovanie indexu a prioritneho smeru noveho genu a alokovanie genu
		new_gene = gene_alloc((rand() % perimeter) + 1, movements[rand() % 2]);

		// Toto zabezpeci, aby nevznikli geny s rovnakym indexom
		while (is_gene(new_chromosome, new_gene, i) == TRUE) {
			new_gene->index = (rand() % perimeter) + 1;
		}

		new_chromosome->genes[i] = new_gene;
	}

	return new_chromosome;
}

/*
 * Funkcia na skopirovanie mapy do inej mapy.
 */
short** map_cpy(short** dest, short** src, Zen* zen) {
	u_short i,
			j;

	for (i = 0; i < zen->height; i++) {
		for (j = 0; j < zen->width; j++) {
			dest[i][j] = src[i][j];
		}
	}

	return dest;
}

/*
 * Funkcia pre pohybovanie mnicha v zen zahrade a zaroven pocitanie
 * fitness hodnot jednotlivych genov.
 */
u_short gene_fitness(Zen* zen, Gene* gene) {
	u_short gene_fitness,
			width,
			height,
			index,
			perimeter,
			coordinate,
			entrance_line,
			entrance_column;

	int		i,
			j;

	short** map;
	
	gene_fitness		=	0;
	width				=	zen->width;
	height				=	zen->height;
	index				=	gene->index;
	perimeter			=	2 * width + 2 * height;
	coordinate			=	0;
	entrance_line		=	0;
	entrance_column		=	0;

	map					=	zen->map;

	// Mnich vchadza zlava
	if (gene->index > perimeter - height) {
		gene->direction_on_map = RIGHT;
	}
	// Mnich vchadza zdola
	else if (gene->index > perimeter - height - width) {
		gene->direction_on_map = UP;
	}
	// Mnich vchadza sprava
	else if (gene->index > perimeter - height - width - height) {
		gene->direction_on_map = LEFT;
	}
	// Mnich vchadza zhora
	else if (gene->index > perimeter - height - width - height - width) {
		gene->direction_on_map = DOWN;
	}

	// Vypocet suradnic na zaklde smeru vchadzania a indexu
	if (gene->direction_on_map == RIGHT) {
		for (i = perimeter; i > index; i--) {
			coordinate++;
		}
		entrance_line	= coordinate;
		entrance_column = 0;
	}
	else if (gene->direction_on_map == UP) {
		for (i = perimeter - height; i > index; i--) {
			coordinate++;
		}
		entrance_line	= height - 1;
		entrance_column = coordinate;
	}
	else if (gene->direction_on_map == LEFT) {
		for (i = perimeter - height - width; i > index; i--) {
			coordinate++;
		}
		coordinate		= (height - 1) - coordinate;
		entrance_line	= coordinate;
		entrance_column = width - 1;
	}
	else if (gene->direction_on_map == DOWN) {
		for (i = perimeter - height - width - height; i > index; i--) {
			coordinate++;
		}
		coordinate		= (width - 1) - coordinate;
		entrance_line	= 0;
		entrance_column	= coordinate;
	}

	// Ak sa nemoze vojst do zahrady, vrati sa fitness genu 0 
	if (map[entrance_line][entrance_column] == OBSTACLE || map[entrance_line][entrance_column] > 0) {
		order--;
		return 0;
	}

	for (i = entrance_line, j = entrance_column; i >= 0 && i < height && j >= 0 && j < width; ) {
		// Ak je smer mnicha na mape vpravo
		if (gene->direction_on_map == RIGHT) {
			// Ak je prekazka alebo upraveny piesok v ceste
			if (j + 1 < width && (map[i][j + 1] == OBSTACLE || map[i][j + 1] > 0)) {
				// Ak je prioritny smer pri prekazke vlavo
				if (gene->priority_move == LEFT) {
					// Ak mnich dosiel k hranici zahrady
					if (i - 1 < 0) {
						map[i][j] = order;
						gene_fitness++;

						return gene_fitness;
					}
					// Ak sa prioritnym smerom neda ist
					if (map[i - 1][j] == OBSTACLE || map[i - 1][j] > 0) {
						// Ak sa da ist smerom opacnym prioritnemu smeru
						if (i + 1 < height && map[i + 1][j] != OBSTACLE && map[i + 1][j] == 0) {
							gene->direction_on_map = DOWN;
						}
						// Neda sa vybocit prekazku alebo upraveny piesok, tak dalej chromozom nevysetrujeme
						else if (i + 1 < height && (map[i + 1][j] == OBSTACLE || map[i + 1][j] > 0)) {
							map[i][j] = order;
							gene_fitness++;

							flag = TRUE;
							return gene_fitness;
						}
						else {
							map[i][j] = order;
							gene_fitness++;

							return gene_fitness;
						}
					}
					// Ak sa prioritnym smerom da ist
					else {
						gene->direction_on_map = UP;
					}
				}
				// Ak je prioritny smer pri prekazke vpravo
				else if (gene->priority_move == RIGHT) {
					// Ak mnich dosiel k hranici zahrady
					if (i + 1 >= height) {
						map[i][j] = order;
						gene_fitness++;

						return gene_fitness;
					}
					// Ak sa prioritnym smerom neda ist
					if (map[i + 1][j] == OBSTACLE || map[i + 1][j] > 0) {
						// Ak sa da ist smerom opacnym prioritnemu smeru
						if (i - 1 >= 0 && map[i - 1][j] != OBSTACLE && map[i - 1][j] == 0) {
							gene->direction_on_map = UP;
						}
						// Neda sa vybocit prekazku alebo upraveny piesok, tak dalej chromozom nevysetrujeme
						else if (i - 1 >= 0 && (map[i - 1][j] == OBSTACLE || map[i - 1][j] > 0)) {
							map[i][j] = order;
							gene_fitness++;

							flag = TRUE;
							return gene_fitness;
						}
						else {
							map[i][j] = order;
							gene_fitness++;

							return gene_fitness;
						}
					}
					// Ak sa prioritnym smerom da ist
					else {
						gene->direction_on_map = DOWN;
					}
				}
			}
			// Ak sa v ceste prekazka alebo upraveny piesok nenachadza
			else {
				map[i][j] = order;
				gene_fitness++;
				j++;
			}
		}
		// Ak je smer mnicha na mape vlavo
		else if (gene->direction_on_map == LEFT) {
			// Ak je prekazka alebo upraveny piesok v ceste
			if (j - 1 >= 0 && (map[i][j - 1] == OBSTACLE || map[i][j - 1] > 0)) {
				// Ak je prioritny smer pri prekazke vpravo
				if (gene->priority_move == RIGHT) {
					// Ak mnich dosiel k hranici zahrady
					if (i - 1 < 0) {
						map[i][j] = order;
						gene_fitness++;

						return gene_fitness;
					}
					// Ak sa prioritnym smerom neda ist
					if (map[i - 1][j] == OBSTACLE || map[i - 1][j] > 0) {
						// Ak sa da ist smerom opacnym prioritnemu smeru
						if (i + 1 < height && map[i + 1][j] != OBSTACLE && map[i + 1][j] == 0) {
							gene->direction_on_map = DOWN;
						}
						// Neda sa vybocit prekazku alebo upraveny piesok, tak dalej chromozom nevysetrujeme
						else if (i + 1 < height && (map[i + 1][j] == OBSTACLE || map[i + 1][j] > 0)) {
							map[i][j] = order;
							gene_fitness++;

							flag = TRUE;
							return gene_fitness;
						}
						else {
							map[i][j] = order;
							gene_fitness++;

							return gene_fitness;
						}
					}
					// Ak sa prioritnym smerom da ist
					else {
						gene->direction_on_map = UP;
					}
				}
				// Ak je prioritny smer pri prekazke vlavo
				else if (gene->priority_move == LEFT) {
					// Ak mnich dosiel k hranici zahrady
					if (i + 1 >= height) {
						map[i][j] = order;
						gene_fitness++;

						return gene_fitness;
					}
					// Ak sa prioritnym smerom neda ist
					if (map[i + 1][j] == OBSTACLE || map[i + 1][j] > 0) {
						// Ak sa da ist smerom opacnym prioritnemu smeru
						if (i - 1 >= 0 && map[i - 1][j] != OBSTACLE && map[i - 1][j] == 0) {
							gene->direction_on_map = UP;
						}
						// Neda sa vybocit prekazku alebo upraveny piesok, tak dalej chromozom nevysetrujeme
						else if (i - 1 >= 0 && (map[i - 1][j] == OBSTACLE || map[i - 1][j] > 0)) {
							map[i][j] = order;
							gene_fitness++;

							flag = TRUE;
							return gene_fitness;
						}
						else {
							map[i][j] = order;
							gene_fitness++;

							return gene_fitness;
						}
					}
					// Ak sa prioritnym smerom da ist
					else {
						gene->direction_on_map = DOWN;
					}
				}
			}
			// Ak sa v ceste prekazka alebo upraveny piesok nenachadza
			else {
				map[i][j] = order;
				gene_fitness++;
				j--;
			}
		}
		// Ak je smer mnicha na mape dole
		else if (gene->direction_on_map == DOWN) {
			// Ak je prekazka alebo upraveny piesok v ceste
			if (i + 1 < height && (map[i + 1][j] == OBSTACLE || map[i + 1][j] > 0)) {
				// Ak je prioritny smer pri prekazke vlavo
				if (gene->priority_move == LEFT) {
					// Ak mnich dosiel k hranici zahrady
					if (j + 1 >= width) {
						map[i][j] = order;
						gene_fitness++;

						return gene_fitness;
					}
					// Ak sa prioritnym smerom neda ist
					if (map[i][j + 1] == OBSTACLE || map[i][j + 1] > 0) {
						// Ak sa da ist smerom opacnym prioritnemu smeru
						if (j - 1 >= 0 && map[i][j - 1] != OBSTACLE && map[i][j - 1] == 0) {
							gene->direction_on_map = LEFT;
						}
						// Neda sa vybocit prekazku alebo upraveny piesok, tak dalej chromozom nevysetrujeme
						else if (j - 1 >= 0 && (map[i][j - 1] == OBSTACLE || map[i][j - 1] > 0)) {
							map[i][j] = order;
							gene_fitness++;

							flag = TRUE;
							return gene_fitness;
						}
						else {
							map[i][j] = order;
							gene_fitness++;

							return gene_fitness;
						}
					}
					// Ak sa prioritnym smerom da ist
					else {
						gene->direction_on_map = RIGHT;
					}
					
				}
				// Ak je prioritny smer pri prekazke vpravo
				else if (gene->priority_move == RIGHT) {
					// Ak mnich dosiel k hranici zahrady
					if (j - 1 < 0) {
						map[i][j] = order;
						gene_fitness++;

						return gene_fitness;
					}
					// Ak sa prioritnym smerom neda ist
					if (map[i][j - 1] == OBSTACLE || map[i][j - 1] > 0) {
						// Ak sa da ist smerom opacnym prioritnemu smeru
						if (j + 1 < width && map[i][j + 1] != OBSTACLE && map[i][j + 1] == 0) {
							gene->direction_on_map = RIGHT;
						}
						// Neda sa vybocit prekazku alebo upraveny piesok, tak dalej chromozom nevysetrujeme
						else if (j + 1 < width && (map[i][j + 1] == OBSTACLE || map[i][j + 1] > 0)) {
							map[i][j] = order;
							gene_fitness++;

							flag = TRUE;
							return gene_fitness;
						}
						else {
							map[i][j] = order;
							gene_fitness++;

							return gene_fitness;
						}
					}
					// Ak sa prioritnym smerom da ist
					else {
						gene->direction_on_map = LEFT;
					}
				}
			}
			// Ak sa v ceste prekazka alebo upraveny piesok nenachadza
			else {
				map[i][j] = order;
				gene_fitness++;
				i++;
			}
		}
		// Ak je smer mnicha na mape hore
		else if (gene->direction_on_map == UP) {
			// Ak je prekazka alebo upraveny piesok v ceste
			if (i - 1 >= 0 && (map[i - 1][j] == OBSTACLE || map[i - 1][j] > 0)) {
				// Ak je prioritny smer pri prekazke vpravo
				if (gene->priority_move == RIGHT) {
					// Ak mnich dosiel k hranici zahrady
					if (j + 1 >= width) {
						map[i][j] = order;
						gene_fitness++;

						return gene_fitness;
					}
					// Ak sa prioritnym smerom neda ist
					if (map[i][j + 1] == OBSTACLE || map[i][j + 1] > 0) {
						// Ak sa da ist smerom opacnym prioritnemu smeru
						if (j - 1 >= 0 && map[i][j - 1] != OBSTACLE && map[i][j - 1] == 0) {
							gene->direction_on_map = LEFT;
						}
						// Neda sa vybocit prekazku alebo upraveny piesok, tak dalej chromozom nevysetrujeme
						else if (j - 1 >= 0 && (map[i][j - 1] == OBSTACLE || map[i][j - 1] > 0)) {
							map[i][j] = order;
							gene_fitness++;

							flag = TRUE;
							return gene_fitness;
						}
						else {
							map[i][j] = order;
							gene_fitness++;

							return gene_fitness;
						}
					}
					// Ak sa prioritnym smerom da ist
					else {
						gene->direction_on_map = RIGHT;
					}
				}
				// Ak je prioritny smer pri prekazke vlavo
				else if (gene->priority_move == LEFT) {
					// Ak mnich dosiel k hranici zahrady
					if (j - 1 < 0) {
						map[i][j] = order;
						gene_fitness++;

						return gene_fitness;
					}
					// Ak sa prioritnym smerom neda ist
					if (map[i][j - 1] == OBSTACLE || map[i][j - 1] > 0) {
						// Ak sa da ist smerom opacnym prioritnemu smeru
						if (j + 1 < width && map[i][j + 1] != OBSTACLE && map[i][j + 1] == 0) {
							gene->direction_on_map = RIGHT;
						}
						// Neda sa vybocit prekazku alebo upraveny piesok, tak dalej chromozom nevysetrujeme
						else if (j + 1 < width && (map[i][j + 1] == OBSTACLE || map[i][j + 1] > 0)) {
							map[i][j] = order;
							gene_fitness++;

							flag = TRUE;
							return gene_fitness;
						}
						else {
							map[i][j] = order;
							gene_fitness++;

							return gene_fitness;
						}
					}
					// Ak sa prioritnym smerom da ist
					else {
						gene->direction_on_map = LEFT;
					}
				}
			}
			// Ak sa v ceste prekazka alebo upraveny piesok nenachadza
			else {
				map[i][j] = order;
				gene_fitness++;
				i--;
			}
		}
	}

	return gene_fitness;
}

/*
 * Funkcia na nahodny vyber TRUE alebo FALSE.
 */
boolean bool_rand() {
	boolean values[] = { TRUE, FALSE };
	return values[rand() % 2];
}

/*
 * Funkcia pre krizenie chromozomov.
 * Podla vygenerovaneho pola uniform (obsahuje hodnoty TRUE alebo FALSE)
 * potomok ziskava geny otca (ak je hodnota TRUE) a geny matky (ak je hodnota FALSE).
 * Hodnota direction_on_map sa nededi - ta sa nastavi pri zistovani fitness podla indexu.
 */
Chromosome* crossover(boolean* uniform, Chromosome** parents) {
	Chromosome* father		=	NULL,
			  * mother		=	NULL,
			  * offspring	=	NULL;

	Gene*		new_gene	=	NULL;

	u_short		i;

	father		=	parents[0];
	mother		=	parents[1];
	offspring	=	chromosome_alloc(father->gene_count);

	for (i = 0; i < mother->gene_count; i++) {
		if (uniform[i] == TRUE) {
			new_gene = gene_alloc(father->genes[i]->index, father->genes[i]->priority_move);
		} 
		else {
			new_gene = gene_alloc(mother->genes[i]->index, mother->genes[i]->priority_move);
		}

		offspring->genes[i] = new_gene;
	}

	return offspring;
}

/*
 * Funkcia pre vytvorenie jedneho potomka z dvoch rodicov a pre vykonanie krizenia.
 */
Chromosome* create_offspring(Chromosome** parents) {
	Chromosome* offspring	=	NULL,
			  * father		=	NULL,
		      * mother		=	NULL;

	boolean	  *	uniform		=	NULL;

	u_short		i;

	father	= parents[0];
	mother	= parents[1];

	uniform = (boolean*)malloc(father->gene_count * sizeof(boolean));
	CHECK_ALLOC_ERR(uniform);

	for (i = 0; i < mother->gene_count; i++) {
		uniform[i] = bool_rand();
	}

	offspring = crossover(uniform, parents);

	free(uniform);
	return offspring;
}

/*
 * Funkcia na vymazanie populacie.
 */
void free_population(Population* prev_population) {
	u_short i,
			j;

	for (i = 0; i < prev_population->chromosome_count; i++) {
		for (j = 0; j < prev_population->chromosomes[i]->gene_count; j++) {
			free(prev_population->chromosomes[i]->genes[j]);
		}
		free(prev_population->chromosomes[i]->genes);
		free(prev_population->chromosomes[i]);
	}
	free(prev_population->chromosomes);
	free(prev_population);
}

/*
 * Funkcia sluziaca na rozhodovanie ci ma dojst k mutacii alebo nie.
 * Sanca na mutaciu kazdeho noveho chromozomu je pseudonahodne 5 az 25%.
 */
boolean do_mutation() {
	u_short percentage;

	percentage = rand() % 101;
	if (percentage >= 1 && percentage <= ( ( rand() % 21 ) + 5 ) ) {
		return TRUE;
	}

	return FALSE;
}

/*
 * Funkcia na zaistenie toho, aby nemutoval gen viackrat ako raz
 * alebo aby sa pri turnaji nevybralo K tych istych chromozomov.
 */
boolean is_index(u_short* indexes, u_short count, u_short index) {
	u_short i;

	for (i = 0; i < count; i++) {
		if (indexes[i] == index) {
			return TRUE;
		}
	}

	return FALSE;
}

/*
 * Funkcia na zistenie ci mutacia pomaha alebo nie.
 * Pomaha vtedy, ked sa vygeneruje pre gen index iny
 * ako index akehokolvek ineho genu v chromozome.
 */
boolean is_helping(Chromosome* chromosome, u_short entrance_index) {
	u_short i;

	for (i = 0; i < chromosome->gene_count; i++) {
		if (chromosome->genes[i]->index == entrance_index) {
			return FALSE;
		}
	}

	return TRUE;
}

/*
 * Funkcia pre mutaciu chromozomu v pripade, ak sa rozhodlo, ze sa ma mutovat.
 */
Chromosome* mutation(Chromosome* chromosome, Zen* zen) {
	u_short		mutation_genes_count,
				perimeter,
				mutation_index,
				entrance_index,
				i;

	u_short*	mutation_indexes = NULL;

	// Mutuje jedna patina genov v chromozome
	mutation_genes_count =	chromosome->gene_count / 5;
	mutation_indexes	 =	(u_short*)malloc(mutation_genes_count * sizeof(u_short));
	perimeter			 =	2 * (zen->height + zen->width);

	CHECK_ALLOC_ERR(mutation_indexes);
	memset(mutation_indexes, USHRT_MAX, mutation_genes_count);

	for (i = 0; i < mutation_genes_count; i++) {
		mutation_index = rand() % chromosome->gene_count;

		// Tymto sa zabezpeci, aby nemutoval viackrat ten isty chromozom.
		while (is_index(mutation_indexes, mutation_genes_count, mutation_index) == TRUE) {
			mutation_index = rand() % chromosome->gene_count;
		}
		mutation_indexes[i] = mutation_index;

		entrance_index = (rand() % perimeter) + 1;

		// Zistovanie ci mutacia pomaha. Ak nie, vygeneruje sa novy index.
		while (is_helping(chromosome, entrance_index) == FALSE) {
			entrance_index = (rand() % perimeter) + 1;
		}
		chromosome->genes[mutation_index]->index = entrance_index;
	}

	free(mutation_indexes);
	return chromosome;
}

/*
 * Funkcia pre alokovanie populacie a jej pola chromozomov.
 */
Population* alloc_population(u_short chromosome_count) {
	Population* new_population = NULL;
	
	new_population = (Population*)malloc(sizeof(Population));
	CHECK_ALLOC_ERR(new_population);

	new_population->chromosomes = (Chromosome**)malloc(chromosome_count * sizeof(Chromosome*));
	CHECK_ALLOC_ERR(new_population->chromosomes);

	new_population->chromosome_count = chromosome_count;

	return new_population;
}

/*
 * Funkcia pre vyber rodicov ruletou.
 * Vyberaju sa dvaja rodicia pre jedneho potomka.
 */
Population* roulette(u_short* fitness_arr, Population* prev_population, Zen* zen) {
	Population*		new_population		=	NULL;

	Chromosome**	parents				=	NULL;

	u_short			random_num_tmp,
					random_num,
					fitness_sum,
					sum,
					i,
					j,
					k;
			
	fitness_sum		 =	0;
	sum				 =	0;
	random_num		 =	0;
	random_num_tmp	 =	0;

	new_population	 =	alloc_population(prev_population->chromosome_count);
	parents			 =	(Chromosome**)malloc(2 * sizeof(Chromosome*));
	CHECK_ALLOC_ERR(parents);

	for (i = 0; i < prev_population->chromosome_count; i++) {
		fitness_sum += fitness_arr[i];
	}

	random_num = rand() % fitness_sum;

	// Opakovanie kym sa nenaplni cela nova populacia
	for (i = 0; i < new_population->chromosome_count; i++) {
		// Vyberaju sa dvaja rodicia pre jedneho potomka
		for (j = 0; j < 2; j++) {
			sum = 0;
			// Vyberanie rodica z populacie
			for (k = 0; k < prev_population->chromosome_count; k++) {
				sum += fitness_arr[k];
				if (sum > random_num) {
					parents[j] = prev_population->chromosomes[k];
					break;
				}
			}
			random_num_tmp = rand() % fitness_sum;

			// Tymto sa zabezpeci, ze sa nevyberie dvakrat ten isty chromozom
			while (random_num_tmp == random_num) {
				random_num_tmp = rand() % fitness_sum;
			}
			random_num = random_num_tmp;
		}
		// Vytvorenie potomka a jeho pripadna mutacia
		new_population->chromosomes[i] = create_offspring(parents);
		if (do_mutation() == TRUE) {
			new_population->chromosomes[i] = mutation(new_population->chromosomes[i], zen);
		}
	}

	free_population(prev_population);
	free(parents);

	return new_population;
}

/*
 * Funkcia pre vyber rodicov turnajom. Nahodne sa vyberie K chromozomov
 * populacie a z vybranych K chromozomov sa vyberie chromozom s najlepsou
 * fitness hodnotou, ktory bude jeden z rodicov.
 * Vyberaju sa dvaja rodicia pre jedneho potomka.
 */
Population* k_way_tournament(u_short* fitness_arr, Population* prev_population, Zen* zen) {
	u_short random_index,
			best_index,
			max,
			i,
			j, 
			ij,
			K;

	Population*	 new_population		=	NULL;

	Chromosome** parents			=	NULL;

	u_short*	 chosen_indexes		=	NULL;

	max = 0;
	K	= 3;

	new_population	= alloc_population(prev_population->chromosome_count);
	parents			= (Chromosome**)malloc(2 * sizeof(Chromosome*));
	chosen_indexes	= (u_short*)malloc(K * sizeof(u_short));

	CHECK_ALLOC_ERR(parents);
	CHECK_ALLOC_ERR(chosen_indexes);

	memset(chosen_indexes, USHRT_MAX, K);	

	// Opakovanie kym sa nenaplni cela nova populacia
	for (i = 0; i < new_population->chromosome_count; i++) {
		// Vyberaju sa dvaja rodicia pre jedneho potomka
		for (j = 0; j < 2; j++) {
			// Vyberanie K chromozomov
			for (ij = 0; ij < K; ij++) {
				random_index = rand() % prev_population->chromosome_count;

				// Toto zabezpeci, aby sa nevybralo 2 az K tych istych chromozomov
				while (is_index(chosen_indexes, K, random_index) == TRUE) {
					random_index = rand() % prev_population->chromosome_count;
				}
				chosen_indexes[ij] = random_index;
			}

			// Vyberanie najlepsieho chromozomu (jedneho rodica)
			// z K chromozomov populacie podla jeho fitness hodnoty
			for (ij = 0; ij < K; ij++) {
				if (fitness_arr[chosen_indexes[ij]] > max) {
					max = fitness_arr[chosen_indexes[ij]];
					best_index = chosen_indexes[ij];
				}
			}
			parents[j] = prev_population->chromosomes[best_index];
		}
		// Vytvorenie potomka a jeho pripadna mutacia
		new_population->chromosomes[i] = create_offspring(parents);
		if (do_mutation() == TRUE) {
			new_population->chromosomes[i] = mutation(new_population->chromosomes[i], zen);
		}
	}

	free_population(prev_population);
	free(parents);
	free(chosen_indexes);

	return new_population;
}

/*
 * Funkcia na vypocet fitness hodnoty chromozomov ako sucet fitness
 * jednotlivych genov v chromozomoch podla gene_fitness.
 */
u_short* fitness(u_short* fitness_arr, Zen* zen, Population* population, u_short** initial_population_map) {
	u_short i,
			j;
	
	for (i = 0; i < population->chromosome_count; i++) {
		fitness_arr[i] = 0;
	}
	
	for (i = 0; i < population->chromosome_count; i++) {
		order = 1;
		for (j = 0; j < population->chromosomes[i]->gene_count; j++) {
			// K fitness hodnote chromozomu sa pripocitavaju fitness hodnoty jeho genov
			fitness_arr[i] += gene_fitness(zen, population->chromosomes[i]->genes[j]);
			if (flag == TRUE) {
				flag = FALSE;
				break;
			}
			order++;
		}
		zen->map = map_cpy(zen->map, initial_population_map, zen);
	}

	return fitness_arr;
}

/*
 * Funkcia na vypocet maximalnej moznej fitness hodnoty v danej mape zahrady.
 */
u_short calc_max_fitness(Zen* zen) {
	return (zen->width * zen->height) - zen->obstacle_count;
}

/*
 * Funkcia na overovanie ci sa naslo riesenie s maximalnou moznou 
 * fitness hodnotou pre danu mapu.
 */
boolean is_max_fitness(Population* population, u_short* fitness_arr, Zen* zen, u_short* chromosome_num) {
	u_short max_fitness,
			i;

	max_fitness = calc_max_fitness(zen);

	for (i = 0; i < population->chromosome_count; i++) {
		if (fitness_arr[i] == max_fitness) {
			*chromosome_num = i;
			return TRUE;
		}
	}

	return FALSE;
}

/*
 * Funkcia pre inicializaciu prvej populacie chromozomami.
 */
Population* init_population(Population* population, u_short gene_count, Zen* zen) {
	u_short i;

	for (i = 0; i < population->chromosome_count; i++) {
		population->chromosomes[i] = initial_chromosome(gene_count, zen);
	}

	return population;
}

/*
 *	Funkcia na spustenie vykonanavania pohybov mnicha s najlepsim najdenym chromozomom.
 */
void check_last(Chromosome* chromosome, Zen* zen) {
	u_short fitness,
			i;
			
	fitness = 0;
	order	= 1;

	for (i = 0; i < chromosome->gene_count; i++) {
		fitness += gene_fitness(zen, chromosome->genes[i]);
		if (flag == TRUE) {
			flag = FALSE;
			break;
		}
		order++;
	}
	print_map(zen);
}

/*
 * Funkcia na vyhladanie najlepsieho chromozomu populacie v pripade, ak sa
 * nenaslo riesenie s maximalnou moznou fitness hodnotou.
 */
u_short find_best_fitness(Population* population, u_short* fitness_arr) {
	u_short max_fitness_index,
			max_fitness,
			i;

	max_fitness_index	=	0;
	max_fitness			=	0;

	for (i = 0; i < population->chromosome_count; i++) {
		if (fitness_arr[i] > max_fitness) {
			max_fitness			=	fitness_arr[i];
			max_fitness_index	=	i;
		}
	}
	return max_fitness_index;
}

/*
 * Prototyp funkcie odosielanej cez parameter funkcie solve.
 */
Population* (*selection)(void* fitness_arr, void* population, void* zen);

/*
 * Funkcia na vypis fitness hodnoty celej populacie.
 */
void print_population_fitness(u_short* fitness_arr, Population* population, u_short population_num) {
	u_short i, 
			population_fitness;

	population_fitness = 0;

	for (i = 0; i < population->chromosome_count; i++) {
		population_fitness += fitness_arr[i];
	}

	printf("%hu. Population fitness: %hu\n", population_num, population_fitness);
}

/*
 * Funkcia na spustenie hladania riesenia.
 */
void solve(Population* population, Zen* zen, Population* (*selection)(u_short*, Population*, Zen*)) {
	u_short		max_population,
				population_num,
				chromosome_num,
				best_not_max;

	short**		initial_population_map	=	NULL;

	u_short*	fitness_arr				=	NULL;

	population_num = 1;
	chromosome_num = 0;

	initial_population_map	=	map_alloc(zen->width, zen->height);
	initial_population_map	=	map_cpy(initial_population_map, zen->map, zen);
	fitness_arr				=	(u_short*)malloc(population->chromosome_count * sizeof(u_short));
	CHECK_ALLOC_ERR(fitness_arr);

	printf("Enter maximum number of populations: ");
	scanf("%hu", &max_population);
	putchar('\n');

	while (1) {
		// Vypocet fitness hodnot chromozomov populacie
		fitness_arr = fitness(fitness_arr, zen, population, initial_population_map);
		
		/*
		// *Optional*
		// Treba zrusit komentar na vypis fitness kazdej generacie
		print_population_fitness(fitness_arr, population, population_num);
		*/

		// Skusanie ci sa naslo uplne riesenie
		if (is_max_fitness(population, fitness_arr, zen, &chromosome_num) == TRUE) {
			printf("DONE in population #%hu\n", population_num);
			printf("CHROMOSOME #%hu\n", chromosome_num + 1);
			printf("Solution with fitness: %hu (max is %hu)\n", fitness_arr[chromosome_num], calc_max_fitness(zen));
			check_last(population->chromosomes[chromosome_num], zen);
			break;
		}

		// Minul sa maximalny pocet populacii vratane prvej
		if (max_population == 1) {
			printf("DONE in last population #%hu\n", population_num);
			best_not_max = find_best_fitness(population, fitness_arr);
			printf("CHROMOSOME #%hu\n", best_not_max + 1);
			printf("Solution with fitness: %hu (max is %hu)\n", fitness_arr[best_not_max], calc_max_fitness(zen));
			check_last(population->chromosomes[best_not_max], zen);
			break;
		}

		// Vytvaranie novej populacie, kym sa nenajde riesenie
		population = selection(fitness_arr, population, zen);
		max_population--;
		population_num++;
	}
	free_population(population);
	free(fitness_arr);
	map_free(initial_population_map, zen);
	map_free(zen->map, zen);
	free(zen);
}

/*
 * Funkcia na nastavenie seedu pseudonahodnych cisel pouzivatelom.
 */
unsigned int set_seed() {
	unsigned int seed;

	printf("\nEnter seed: ");
	scanf("%u", &seed);

	return seed;
}

/*
 * Funkcia na vyber algoritmu vyberu rodicov pouzivatelom.
 */
u_short select_selection() {
	u_short	this_selection;

	printf("\nChoose selection method:\n");

	printf("1. Roulette\n");
	printf("2. Tournament\n");
	printf("Enter your choice: ");
	scanf("%hu", &this_selection);

	return this_selection;
}

/*
 * Funkcia na nastavenie poctu chromozomov pouzivatelom. 
 */
u_short set_chromosome_count() {
	u_short chromosome_count;

	printf("Enter chromosome count: ");
	scanf("%hu", &chromosome_count);

	return chromosome_count;
}

/*
 * Hlavna funkcia programu.
 */
int main() {
	Population*		population	=	NULL;

	Zen*			zen			=	NULL;

	u_short			this_selection,
					gene_count,
					chromosome_count;

	zen	= fill_map(zen);
	print_map(zen);

	srand(set_seed());

	chromosome_count =	set_chromosome_count();
	gene_count		 =	calc_gene_count(zen);
	printf("\nNumber of genes: %hu\n", gene_count);

	population		 =	alloc_population(chromosome_count);
	population		 =	init_population(population, gene_count, zen);
	this_selection	 =	select_selection();

	while (1) {
		if (this_selection == 1) {
			selection = &roulette;
		}
		else if (this_selection == 2) {
			selection = &k_way_tournament;
		}
		else {
			printf("Unknown command\n");
			this_selection = select_selection();
			continue;
		}

		solve(population, zen, (*selection));
		break;
	}

	//_CrtDumpMemoryLeaks();

	return 0;
}