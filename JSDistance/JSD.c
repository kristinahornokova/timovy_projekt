#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define ASCII_SIZE 127
#define BIGRAM_SIZE (ASCII_SIZE * ASCII_SIZE)
#define DIST_SIZE (ASCII_SIZE + BIGRAM_SIZE)
#define BUFFER_SIZE 256
#define OUTPUT_FILE "jsdistance.txt"
#define LOG2_2 1.0  // log2(2) = 1, normalizacia JSD

// Funkcia mi vytvori pravdepodobn. roydelenie pre kazdy par (ang., sloven)
void calculate_distribution(const char *word, double *pravd) {
    int len = strlen(word);
    double total = 0.0;
    
    for (int i = 0; i < DIST_SIZE; i++) {
        pravd[i] = 0.0;
    }
    
    // Vypocet pravdepodobnosti pre znaky a bigramy
    for (int i = 0; i < len; i++) {
        pravd[(unsigned char)word[i]] += 1.0;
        total += 1.0;
        
        if (i < len - 1) {
            int pair_index = ASCII_SIZE + ((unsigned char)word[i] * ASCII_SIZE + (unsigned char)word[i + 1]);
            pravd[pair_index] += 1.0;
            total += 1.0;
        }
    }
    
    // Normalizacia pravdepodobnosti
    for (int i = 0; i < DIST_SIZE; i++) {
        if (total > 0) {
            pravd[i] /= total;
        }
    }
}

// Funkcia na vypocet Jensen-Shannon divergence medzi dvoma distribuciami
double js_divergence(double *dist1, double *dist2) {
    double jsd = 0.0;
    double m[DIST_SIZE];
    
    for (int i = 0; i < DIST_SIZE; i++) {
        m[i] = 0.5 * (dist1[i] + dist2[i]);
    }
    
    // Vypocet Jensen-Shannon divergence
    for (int i = 0; i < DIST_SIZE; i++) {
        if (dist1[i] > 0) jsd += 0.5 * dist1[i] * log2(dist1[i] / m[i]);
        if (dist2[i] > 0) jsd += 0.5 * dist2[i] * log2(dist2[i] / m[i]);
    }
    
    return jsd / LOG2_2;  // Normalizácia do intervalu [0,1]
}

int main(void) {
    FILE *file = fopen("words.txt", "r");
    FILE *output = fopen(OUTPUT_FILE, "w");
    
    if (!file || !output) {
        printf("Chyba pri otváraní súboru!\n");
        return 1;
    }
    
    char eng[BUFFER_SIZE], sk[BUFFER_SIZE];
    double *pravd_eng = (double *)calloc(DIST_SIZE, sizeof(double));
    double *pravd_sk = (double *)calloc(DIST_SIZE, sizeof(double));
    
    if (!pravd_eng || !pravd_sk) {
        printf("Chyba alokácie pamäte!\n");
        return 1;
    }
    
    // Citanie slov a vypocet JSD pre kazdy par slov
    while (fscanf(file, "%s %s", eng, sk) == 2) {
        calculate_distribution(eng, pravd_eng);
        calculate_distribution(sk, pravd_sk);
        double divergence = js_divergence(pravd_eng, pravd_sk);
        double distance = sqrt(divergence);
        fprintf(output, "JSD(%s, %s) = %lf, distance = %lf\n", eng, sk, divergence, distance);
    }
    
    free(pravd_eng);
    free(pravd_sk);
    fclose(file);
    fclose(output);
    printf("Výpočet dokončený, výsledky uložené v %s\n", OUTPUT_FILE);
    return 0;
}

// JSD = 0 znamena, ze distribucie su identicke (slova su uplne rovnake z hladiska distribucie znakov a bigramov).
// JSD = 1 znamena, ze distribucie su uplne rozdielne. To znamena, ze slova maju uplne odlisne charakteristiky, pokial ide o vyskyt znakov a bigramov.
