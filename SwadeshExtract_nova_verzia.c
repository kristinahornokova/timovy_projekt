#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "curl/curl.h"

#define BUFFER_SIZE 1000000 // velkost bufferu je teraz 1MB
#define URL_FORMAT "https://en.wiktionary.org/wiki/Appendix:%s_Swadesh_list" // URL cez %s kde sa zadáva jazyk

// Callback funkcia na ukladanie dat do pamate
size_t write_data(void *ptr, size_t size, size_t nmemb, char *data) {
    size_t realsize = size * nmemb; // vypocita realnu velkost ziskanych dat
    if (strlen(data) + realsize >= BUFFER_SIZE - 1) {
        fprintf(stderr, "Data neboli zapisane.\n");
        return 0;
    }
    strncat(data, ptr, realsize);
    return realsize;
}

// Funkcia na extrakciu slov a jedneho prekladu z tabulky
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

    // Najde koniec tabulky
    const char *table_end = strstr(start, "</table>");
    if (table_end == NULL) {
        fprintf(stderr, "End of table not found in the HTML content.\n");
        fclose(fp);
        return;
    }

    while ((start = strstr(start, "<td>")) != NULL && start < table_end) {
        start += 4; // presun pointer za <td>
        const char *end = strstr(start, "</td>");
        if (end == NULL || end > table_end) {
            break;
        }

        // Najde slovo v prvom stlpci
        const char *word_start = strstr(start, "<a href=");
        if (word_start != NULL && word_start < end) {
            word_start = strstr(word_start, ">");
            if (word_start != NULL && word_start < end) {
                word_start++; // posun za >
                const char *word_end = strstr(word_start, "</a>");
                if (word_end != NULL && word_end < end) {
                    // Vyextrahuje anglické slovo
                    char english_word[100];
                    snprintf(english_word, word_end - word_start + 1, "%s", word_start);

                    // Presun do dalsieho <td>, kde je preklad
                    start = end + 5;
                    end = strstr(start, "</td>");
                    if (end == NULL || end > table_end) {
                        break;
                    }

                    // Najde len prvy preklad v druhom stlpci
                    const char *trans_start = strstr(start, "<a href=");
                    if (trans_start != NULL && trans_start < end) {
                        trans_start = strstr(trans_start, ">");
                        if (trans_start != NULL && trans_start < end) {
                            trans_start++;
                            const char *trans_end = strstr(trans_start, "</a>");
                            if (trans_end != NULL && trans_end < end) {
                                // Vyextrahuje a zapíše iba prvý preklad spolu s anglickým slovom do súboru
                                fprintf(fp, "%s\t%.*s\n", english_word, (int)(trans_end - trans_start), trans_start);
                            }
                        }
                    }
                }
            }
        }
        start = end + 5; // presuň za </td>
    }

    fclose(fp);
}

int main() {
    CURL *curl;
    CURLcode res;
    char data[BUFFER_SIZE] = {0}; // Buffer na ulozenie stiahnuteho HTML obsahu
    char jazyk[100];              // buffer pre jazyk od uzivatela
    char url[200];                // Buffer pre URL

    // Vytvorenie URL pre dany jazyk
    printf("Zadajte jazyk pre Swadesh list: ");
    scanf("%99s", jazyk);

    // Vytvorenie URL pre dany jazyk
    snprintf(url, sizeof(url), URL_FORMAT, jazyk);

    // inicializacia cURL
    int x;
    x = curl_global_init(CURL_GLOBAL_ALL);
    printf("Global OK.\n");
    if (x != 0) printf("Global Init Error: %i\n\n", x);
    
    curl = curl_easy_init();
    printf("Easy OK.\n");
    if (curl) {

        // nastavenie URL a callback funkcie
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, data);

        // spusti cURL poziadavku
        res = curl_easy_perform(curl);

        // kontrola chyb
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            // Extrahovanie slov a prekladov z tabulky a zapis do suboru
            extract_words_from_html(data, "words.txt");
            printf("Slova a preklady su extrahovane do suboru words17.txt\n");
        }

        // Cleanup
        curl_easy_cleanup(curl);
    }

    return 0;
}
