import requests # http request
import sys # cli argumenty
import unicodedata # filtrovanie diakritiky
from bs4 import BeautifulSoup # html parsing
from itertools import combinations # parovanie jazykov

def get_zoznam(jazyk):
    # GET stranky
    base_url = "https://en.wiktionary.org/wiki/Appendix:%s_Swadesh_list"
    url = base_url % jazyk
    response = requests.get(url)
    if response.status_code != 200:
        print(f"HTTP Error: url {url}, status code {response.status_code}")
        return
    
    # priprava polievky (krasnej)
    soup = BeautifulSoup(response.text, 'html.parser')
    
    # tabula ma byt iba jedna
    table = soup.find('table')
    if not table:
        print("Error: No table found.")
        return

    # najprv riadky, potom stlpce
    rows = table.find_all('tr')

    data = []
    # prvy riadok je hlavicka, tu ignorujeme
    for row in rows[1:]:
        # samotne slova su v tage <a>
        cols = row.find_all('a')
        lmax = len(cols) - 1
        # zoberieme posledne slovo. prve je vzdy anglictina, pre zvoleny jazyk inak slov moze byt niekolko (napr. tu, vous).
        # nemame specifikovane ktore slovo treba preferovat, volim posledne pre jednoduchost
        data.append(cols[lmax].get_text(strip=True))
    return data

def get_len(zoznam):
    # napr. stranka rustiny ma diakritiku ktoru len() pocita nespravne (len('э́то')==4), preto pocitame iba samostatne znaky
    return len(''.join(c for c in ''.join(zoznam) if unicodedata.combining(c) == 0 and c != ' '))

def get_jazyky(file):
    with open(file, "r", encoding="utf-8") as file:
        lines = [line.strip().capitalize() for line in file]
    return lines

def pis_do_suboru(file, output):
    with open(file, "w", encoding="utf-8") as file:
        for l in output:
            file.write(l + '\n')

def vypocitaj_vzdialenosti(zoznamy):
    outputs = []
    for k1, k2 in combinations(zoznamy.keys(), 2):
        vzdialenost = abs(zoznamy[k1]-zoznamy[k2])
        outputs.append((vzdialenost, f"{k1} {k2} {vzdialenost}"))
    # zaradenie podla vzdialenosti
    outputs.sort(key = lambda line: line[0])
    # vraciame iba retazec, nie (vzdialenost, retazec)
    return [l[1] for l in outputs]

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print('Pouzi ako: "python %s subor_s_jazykmi.txt"' % (sys.argv[0]))
        exit(1)
    jazyky = get_jazyky(sys.argv[1])
    zoznamy = {}
    for jazyk in jazyky:
        zoznamy[jazyk] = get_len(get_zoznam(jazyk))
    pis_do_suboru("output.txt", vypocitaj_vzdialenosti(zoznamy))
    print("Vysledok zapisany do suboru output.txt")
