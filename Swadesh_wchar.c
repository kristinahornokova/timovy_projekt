#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <wchar.h>
#include <locale.h>
#include "curl/curl.h"

// buffer - docasne ulozisko s velkostou 1MB
#define BUFFER_SIZE 1000000
#define URL_FORMAT "https://en.wiktionary.org/wiki/Appendix:%s_Swadesh_list"

// deklaracie vsetkych funkcii ktore mam v kode (koli kompilacii mi je to takto lepsie)
size_t write_data(void *ptr, size_t size, size_t nmemb, char *data);
int levenshtein_wchar(const wchar_t *a, const wchar_t *b, FILE *table_fp);
void extract_words_from_html(const char *html, const char *output_file);
void distances(const char *input_file, const char *output_file);

// Callback funkcia na ukladanie dat do pamate (bez zmeny)
size_t write_data(void *ptr, size_t size, size_t nmemb, char *data) {
    size_t realsize = size * nmemb;
    if (strlen(data) + realsize >= BUFFER_SIZE - 1) {
        fprintf(stderr, "Data neboli zapisane.\n");
        return 0;
    }
    strncat(data, ptr, realsize);
    return realsize;
}

// Funkcia na výpočet Levenshteinovej vzdialenosti pre široké znaky
// ukladam vsetky medzi vypocty aby sa hodnoty nemuseli pocitat opakovane
int levenshtein_wchar(const wchar_t *a, const wchar_t *b, FILE *table_fp) {
    int len_a = wcslen(a);
    int len_b = wcslen(b);

    int **table = (int **)malloc((len_a + 1) * sizeof(int *));
    for (int i = 0; i <= len_a; i++) {
        table[i] = (int *)malloc((len_b + 1) * sizeof(int));
    }

    for (int i = 0; i <= len_a; i++) table[i][0] = i;
    for (int j = 0; j <= len_b; j++) table[0][j] = j;

    for (int i = 1; i <= len_a; i++) {
        for (int j = 1; j <= len_b; j++) {
            if (a[i - 1] == b[j - 1]) {
                table[i][j] = table[i - 1][j - 1];
            } else {
                table[i][j] = 1 + (table[i - 1][j] < table[i][j - 1]
                                    ? (table[i - 1][j] < table[i - 1][j - 1] ? table[i - 1][j] : table[i - 1][j - 1])
                                    : (table[i][j - 1] < table[i - 1][j - 1] ? table[i][j - 1] : table[i - 1][j - 1]));
            }
        }
    }

    if (table_fp != NULL) {
        fprintf(table_fp, "\nTabulka Levenshteinovej vzdialenosti pre '%ls' a '%ls':\n   ", a, b);
        for (int j = 0; j < len_b; j++) {
            fwprintf(table_fp, L" %lc ", b[j]);
        }
        fprintf(table_fp, "\n");
        for (int i = 0; i <= len_a; i++) {
            if (i > 0) {
                fwprintf(table_fp, L"%lc ", a[i - 1]);
            } else {
                fprintf(table_fp, "  ");
            }
            for (int j = 0; j <= len_b; j++) {
                fprintf(table_fp, "%2d ", table[i][j]);
            }
            fprintf(table_fp, "\n");
        }
        fprintf(table_fp, "\n");
    }

    int result = table[len_a][len_b];
    for (int i = 0; i <= len_a; i++) free(table[i]);
    free(table);

    return result;
}

// Funkcia na extrakciu slov
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

// funkcia na vypocet vzdialenosti
// zoberie slova zo vstupneho suboru a pre kazdu funkciu zavola levenshtein_wchar
void distances(const char *input_file, const char *output_file) {
    FILE *fp_in = fopen(input_file, "r");
    FILE *fp_out = fopen(output_file, "w");
    FILE *table_fp = fopen("tabulka.txt", "w");

    if (fp_in == NULL || fp_out == NULL || table_fp == NULL) {
        perror("Chyba pri otvarani suboru.");
        return;
    }

    wchar_t english_word[100];
    wchar_t translation[100];

    while (fwscanf(fp_in, L"%ls\t%ls", english_word, translation) == 2) {
        int distance = levenshtein_wchar(english_word, translation, table_fp);
        fwprintf(fp_out, L"%ls\t%ls\t%d\n", english_word, translation, distance);
    }

    fclose(fp_in);
    fclose(fp_out);
    fclose(table_fp);
}

// Hlavna funkcia
int main() {
    setlocale(LC_ALL, ""); // locane pre "siroke" znaky 
    CURL *curl;
    CURLcode res;
    char data[BUFFER_SIZE] = {0};
    char jazyk[100];
    char url[200];

    printf("Zadajte jazyk pre Swadesh list: ");
    scanf("%99s", jazyk);

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
            extract_words_from_html(data, "words.txt");
            printf("Slova a preklady su extrahovane do suboru words.txt\n");
            distances("words.txt", "vzdialenost.txt");
            printf("Vzdialenosti su zapisane do suboru vzdialenost.txt\n");
        }

        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    return 0;
}


