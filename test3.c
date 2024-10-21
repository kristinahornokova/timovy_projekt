#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#define BUFFER_SIZE 1000000 // velkost bufferu je teraz 1MB
#define URL_FORMAT "https://en.wiktionary.org/wiki/Appendix:%s_Swadesh_list" // URL cez %s kde sa zadáva jazyk

// Callback funkcia na ukladanie dat do pamate
size_t write_data(void *ptr, size_t size, size_t nmemb, char *data) {
    size_t realsize = size * nmemb; // vypocita realu velkost ziskanych dat
    strncat(data, ptr, realsize);
    return realsize;
}

// funkcia na extrakciu slov z tabuľly
void extract_words_from_html(const char *html, const char *output_file) {
    FILE *fp = fopen(output_file, "w");
    if (fp == NULL) {
        perror("Error opening output file");
        return;
    }

    const char *start = html; // nastavi zaciatok na zaciatok HTML obsahu
    while ((start = strstr(start, "<td>")) != NULL) {
        start += 4; // presun ukazovatel za <td>
        const char *end = strstr(start, "</td>"); // presun za <td>
        if (end == NULL) {
            break;
        }

        // Vypise slovo medzi <td> a </td> do suboru
        fprintf(fp, "%.*s\n", (int)(end - start), start);
        start = end + 5; // presuň za </td>
    }

    fclose(fp);
}

int main() {
    CURL *curl;
    CURLcode res;
    char data[BUFFER_SIZE] = {0}; // Buffer na uloeenie stiahnuteho HTML obsahu
    char jazyk[100];              // Buffer pre jazyk od uzivatela
    char url[200];                // buffer pre URL

    // ziskanie jazyku zo vstupu
    printf("Zadajte jazyk pre Swadesh list: ");
    scanf("%s", jazyk);

    // Vytvorenie URL pre dany jazyk
    snprintf(url, sizeof(url), URL_FORMAT, jazyk);

    // Inicializácia cURL
    curl = curl_easy_init();
    if (curl) {
        // Nastavenie URL a callback funkcie
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, data);

        // Spusti cURL poziadavku
        res = curl_easy_perform(curl);

        // kontrola chyb
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else {
            // Extrahovanie slov z tabulky a zapis do súboru
            extract_words_from_html(data, "words15.txt");
            printf("Slova su extrahovane do suboru words.txt\n");
        }

        // Uvolni zdroje
        curl_easy_cleanup(curl);
    }

    return 0;
}
