#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#define BUFFER_SIZE 1000000 // velkost bufferu je teraz 1MB
#define URL_FORMAT "https://en.wiktionary.org/wiki/Appendix:%s_Swadesh_list" // URL cez %s kde sa zadáva jazyk


// Callback funkcia na ukladanie dat do pamate
size_t write_data(void *ptr, size_t size, size_t nmemb, char *data) {
    size_t realsize = size * nmemb; // vypocita realu velkost ziskanych dat
    if (strlen(data) + realsize >= BUFFER_SIZE - 1) {
        fprintf(stderr, "Data neboli zapisane.\n");
        return 0;
    }
    strncat(data, ptr, realsize);
    return realsize;
}

// funkcia na extrakciu slov z tabulky
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

    // Find the end of the table
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

        // Vypise slovo medzi <td> a </td> do suboru
        const char *word_start = start;
        while ((word_start = strstr(word_start, "<a href=")) != NULL && word_start < end) {
            // posun pointera na slovo po href a title
            word_start = strstr(word_start, ">");
            if (word_start == NULL || word_start >= end) {
                break;
            }
            word_start++; // posunutie za >

            const char *word_end = strstr(word_start, "</a>");
            if (word_end == NULL || word_end > end) {
                break;
            }

            // yapise slovo do suboru
            fprintf(fp, "%.*s", (int)(word_end - word_start), word_start);

            // Posun sa na dalsie slovo alebo ciarku, ak su v rovnakom riadku dalsie slova
            word_start = word_end + 4;

            // kontrola ci su dalsie slova
            if (strstr(word_start, "<a href=") != NULL && word_start < end) {
                fprintf(fp, ", ");
            }
        }

        // napise novy riadok ak su extrahovane vsetkz slova z riadku
        fprintf(fp, "\n");

        start = end + 5; // presuň za </td>
    }

    fclose(fp);
}

int main() {
    CURL *curl;
    CURLcode res;
    char data[BUFFER_SIZE] = {0}; // Buffer na uloeenie stiahnuteho HTML obsahu
    char jazyk[100];              // buffer pre jazyk od uzivatela
    char url[200];                // Buffer pre URL

    // Vytvorenie URL pre dany jazyk
    printf("Zadajte jazyk pre Swadesh list: ");
    scanf("%99s", jazyk);

    // Create the URL for the given language
    snprintf(url, sizeof(url), URL_FORMAT, jazyk);

    // inicializacia cURL
    curl = curl_easy_init();
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
            // debugging pre vypis
            //printf("Downloaded HTML (first 500 characters):\n%.*s\n", 500, data);

            // Extrahovanie slov z tabulky a zapis do suboru
            extract_words_from_html(data, "words17.txt");
            printf("Slova su extrahovane do suboru words.txt\n");
        }

        // Cleanup
        curl_easy_cleanup(curl);
    }

    return 0;
}

/*
tu mame problem pretoze nam vypisuje aj slova resp. jazyky, ktore su na konci WIkipedie
ale aspon nam uz vypisuje vsetky slova tak ako ma 
su zapisane pod sebou a nie vedla seba ale to by sme este vedeli zmenit 


--------------------------------------

uz to ide krasne !!!!!!
nevypisuje nic len slova z tabulky a vsetky 


napr pri Danish nam nezoberie vsetky slova lebo nie su vo forme <a herf= ..... a>
*/