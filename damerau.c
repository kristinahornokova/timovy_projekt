#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>
#include "curl/curl.h"

// buffer - docasne ulozisko s velkostou 1MB
#define BUFFER_SIZE 1000000
#define URL_FORMAT "https://en.wiktionary.org/wiki/Appendix:%s_Swadesh_list"

// Deklarácie funkcií
size_t write_data(void *ptr, size_t size, size_t nmemb, char *data);
int minimum(int a, int b);
int minimum3(int a, int b, int c);
void writeMatrixToFile(FILE *file, int **matrix, wchar_t *a, wchar_t *b, int rows, int cols, const wchar_t *word1, const wchar_t *word2);
int damerauLevenshteinDistanceWithOutput(wchar_t *a, wchar_t *b, const wchar_t *word1, const wchar_t *word2, FILE *table_file);
void extract_words_from_html(const char *html, const char *output_file);
void distancesWithOutput(const char *input_file, const char *output_file, const char *table_file);

// Funkcia na získanie minima z dvoch čísel
int minimum(int a, int b) {
    return (a < b) ? a : b;
}

// Funkcia na získanie minima z troch čísel
int minimum3(int a, int b, int c) {
    return minimum(a, minimum(b, c));
}

// Funkcia na zápis matice do súboru
void writeMatrixToFile(FILE *file, int **matrix, wchar_t *a, wchar_t *b, int rows, int cols, const wchar_t *word1, const wchar_t *word2) {
    fwprintf(file, L"\nTabuľka vzdialenosti pre '%ls' a '%ls':\n", word1, word2);
    fwprintf(file, L"    ");
    for (int j = 0; j < cols - 1; j++) {
        fwprintf(file, L" %lc ", b[j]);
    }
    fwprintf(file, L"\n");

    for (int i = 0; i < rows; i++) {
        if (i == 0) {
            fwprintf(file, L"  ");
        } else {
            fwprintf(file, L" %lc", a[i - 1]);
        }

        for (int j = 0; j < cols; j++) {
            fwprintf(file, L" %2d ", matrix[i][j]);
        }
        fwprintf(file, L"\n");
    }
}

// Funkcia na výpočet Damerau-Levenshtein vzdialenosti s výstupom do súboru
int damerauLevenshteinDistanceWithOutput(wchar_t *a, wchar_t *b, const wchar_t *word1, const wchar_t *word2, FILE *table_file) {
    int lenA = wcslen(a);
    int lenB = wcslen(b);

    int **d = (int **)malloc((lenA + 1) * sizeof(int *));
    for (int i = 0; i <= lenA; i++) {
        d[i] = (int *)malloc((lenB + 1) * sizeof(int));
    }

    for (int i = 0; i <= lenA; i++) d[i][0] = i;
    for (int j = 0; j <= lenB; j++) d[0][j] = j;

    for (int i = 1; i <= lenA; i++) {
        for (int j = 1; j <= lenB; j++) {
            int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            d[i][j] = minimum3(
                d[i - 1][j] + 1,       // vymazanie
                d[i][j - 1] + 1,       // vloženie
                d[i - 1][j - 1] + cost // substitúcia
            );

            if (i > 1 && j > 1 && a[i - 1] == b[j - 2] && a[i - 2] == b[j - 1]) {
                d[i][j] = minimum(d[i][j], d[i - 2][j - 2] + 1); // transpozícia
            }
        }
    }

    // Zapísať maticu do súboru
    writeMatrixToFile(table_file, d, a, b, lenA + 1, lenB + 1, word1, word2);

    int result = d[lenA][lenB];

    for (int i = 0; i <= lenA; i++) {
        free(d[i]);
    }
    free(d);

    return result;
}

// Callback funkcia na ukladanie dat do pamate
size_t write_data(void *ptr, size_t size, size_t nmemb, char *data) {
    size_t realsize = size * nmemb;
    if (strlen(data) + realsize >= BUFFER_SIZE - 1) {
        fprintf(stderr, "Data neboli zapisane.\n");
        return 0;
    }
    strncat(data, ptr, realsize);
    return realsize;
}

// Funkcia na extrakciu slov z HTML
void extract_words_from_html(const char *html, const char *output_file) {
    FILE *fp = fopen(output_file, "w");
    if (fp == NULL) {
        perror("Chyba pri otvarani suboru.");
        return;
    }

    const char *start = strstr(html, "<table");
    if (start == NULL) {
        fprintf(stderr, "Table not found in the HTML content.\n");
        fclose(fp);
        return;
    }

    const char *table_end = strstr(start, "</table>");
    if (table_end == NULL) {
        fprintf(stderr, "End of table not found in the HTML content.\n");
        fclose(fp);
        return;
    }

    while ((start = strstr(start, "<td>")) != NULL && start < table_end) {
        start += 4;
        const char *end = strstr(start, "</td>");
        if (end == NULL || end > table_end) {
            break;
        }

        const char *word_start = strstr(start, "<a href=");
        if (word_start != NULL && word_start < end) {
            word_start = strstr(word_start, ">");
            if (word_start != NULL && word_start < end) {
                word_start++;
                const char *word_end = strstr(word_start, "</a>");
                if (word_end != NULL && word_end < end) {
                    char english_word[100];
                    snprintf(english_word, word_end - word_start + 1, "%s", word_start);

                    start = end + 5;
                    end = strstr(start, "</td>");
                    if (end == NULL || end > table_end) {
                        break;
                    }

                    const char *trans_start = strstr(start, "<a href=");
                    if (trans_start != NULL && trans_start < end) {
                        trans_start = strstr(trans_start, ">");
                        if (trans_start != NULL && trans_start < end) {
                            trans_start++;
                            const char *trans_end = strstr(trans_start, "</a>");
                            if (trans_end != NULL && trans_end < end) {
                                fprintf(fp, "%s\t%.*s\n", english_word, (int)(trans_end - trans_start), trans_start);
                            }
                        }
                    }
                }
            }
        }
        start = end + 5;
    }

    fclose(fp);
}

// Funkcia na výpočet vzdialeností
void distancesWithOutput(const char *input_file, const char *output_file, const char *table_file) {
    FILE *fp_in = fopen(input_file, "r");
    FILE *fp_out = fopen(output_file, "w");
    FILE *fp_table = fopen(table_file, "w");

    if (fp_in == NULL || fp_out == NULL || fp_table == NULL) {
        perror("Chyba pri otváraní súborov.");
        return;
    }

    wchar_t english_word[100];
    wchar_t translation[100];

    while (fwscanf(fp_in, L"%ls\t%ls", english_word, translation) == 2) {
        int distance = damerauLevenshteinDistanceWithOutput(english_word, translation, english_word, translation, fp_table);
        fwprintf(fp_out, L"%ls\t%ls\t%d\n", english_word, translation, distance);
    }

    fclose(fp_in);
    fclose(fp_out);
    fclose(fp_table);
}

int main() {
    setlocale(LC_ALL, ""); // Nastavenie pre široké znaky
    CURL *curl;
    CURLcode res;
    char data[BUFFER_SIZE] = {0};
    char jazyk[100];
    char url[200];

    printf("Zadajte jazyk pre Swadesh list: ");
    scanf("%99s", jazyk);

    // Vytvorenie URL na základe jazyka
    snprintf(url, sizeof(url), URL_FORMAT, jazyk);

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, data);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            // Extrahovanie slov a výpočet vzdialeností
            extract_words_from_html(data, "words.txt");
            printf("Slova a preklady sú uložené v words.txt\n");
            distancesWithOutput("words.txt", "vzdialenost.txt", "tabulka.txt");
            printf("Vzdialenosti sú zapísané do vzdialenost.txt a tabuľky sú v tabulka.txt\n");
        }

        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    return 0;
}