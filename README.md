# Konstrukcija kompilatora

## Zamena switch instrukcija grananjem (lowerswitch)
### mi19108, mi19255

Pocinjemo sa test fajlom (test.cpp):
![s1](https://github.com/adjordje/llvm_switch/assets/126694198/57afbd28-509c-41e8-8eb2-aec23f7396b9)



U sledecem koraku cemo gledati LLVM IR tj. medjureprezentaciju naseg c++ koda.
Pre toga smo duzni da ga prebacimo u ljudski citljivu IR reprezentaciju (.ll) ili bitkod reprezentaciju (.bc)
Prebacicemo je u .ll:
clang++ -emit-llvm test.cpp -O0 -S -Xclang -disable-O0-optnone
> (clang++) komanda da pokrene Clang C++ kompajler
> 
> (-emit-llvm) instrukcija kojom LLVM pravi IR kod umesto masinskog koda
> 
> (test.cpp) input fajl koji zelimo da kompajliramo
> 
> (-O0) podesavanje optimizacija na nivo nula (bez optimizacija)
> 
> (-S) generisanje .ll umesto .bc
> 
> (-Xclang -disable-O0-optnone) ovaj argument je specifican za clang jer onemogucava optimizacijski pass cak iako je nivo vec namesten na nulu. Jer clang zapravo zadrzava pojedine optimizacijske pass-ove na nivou nula za svrhe debagovanja

U sustini, ova komanda kompajlira test.cpp koristeci Clang C++ kompajler, generise LLVM IR kod bez bilo kakvih optimizacija, i smesti ih u test.ll

Pogledajmo LLVM IR (medjureprezentaciju):

![s2](https://github.com/adjordje/llvm_switch/assets/126694198/599c9e21-d8f8-4fa3-926d-a3740c73852c)

Pogledajmo switch instrukciju:

![d3](https://github.com/adjordje/llvm_switch/assets/126694198/7702eefc-35c8-412a-8823-01ca92e524d9)

Tumacimo je na sledeci nacin:

Koristi treci registar (ovo nisu pravi registri, vec virtuelni LLVM registri):
> poredi case-ove sa trecim virtuelnim registrom
> 
> ukoliko se ne izvrsi ni jedan skok, skoci na labelu 16
>
> ukoliko je vrednost 1000, skoci na labelu 4
>
> ukoliko je vrednost 2000, skoci na labelu 7
>
> ukoliko je vrednost 3000, skoci na labelu 10
>
> ukoliko je vrednost 4000, skoci na labelu 13

### skokovi na %4, %7 i %10 bi svi skocili nazad na labelu %19 (koristeci br instrukciju)

![d4](https://github.com/adjordje/llvm_switch/assets/126694198/cd5d18e6-8c3a-4519-a899-b8f4d44a7cc9)

# br instrukcija

### br instrukcija u LLVM IR-u se koristi za grananje/skakanje u delove koda koji su zasnovani na uslovu. Ona je instrukcijski ekvivalent if-else naredbama koje imaju programski jezici viseg nivoa.
***
### sintaksa:

**br `[condition]`, label `[label_if_true]`, label `[label_if_false]`**

![d5](https://github.com/adjordje/llvm_switch/assets/126694198/d5504eb6-99f0-4342-b4e5-8ad9ea433f4c)

> Ovde mozemo videti da u virtuelni registar "condition" poredimo vrednost virtuelnog registra %value sa konstantom 0. Umesto dosadasnjeg i32 (int32) koristimo i1 (int1) za boolean tip, kako nam je jedan bit informacija dovoljan za true/false.


## Prepravljanje  `switch` instrukcije koristeci `br` instrukcije za grananje


## A) Registrovanje pass-a
> Da bismo mogli da koristimo modularni optimizator / analizator `opt`, prvo moramo da registrujemo pass pod imenom "switchtoifelse"

![d6](https://github.com/adjordje/llvm_switch/assets/126694198/f46a8e73-0cb7-4ab0-9270-966eb5b94178)


## B) Kompilacija
> Kompajliracemo nas fajl na sledeci nacin

clang++ -fPIC -g -O3 -Wall -Wextra -shared -o kk_pass.so pass.cpp \`llvm-config --cppflags --ldflags --libs\`

> Kao rezultat cemo dobiti kk_pass.so fajl.
> Posle toga cemo kompajlirati nas `test.cpp` fajl koji zelimo da optimizujemo na IR nivou

clang++ -emit-llvm test.cpp -O0 -S -Xclang -disable-O0-optnone

> Kao rezultat cemo dobiti `test.ll` fajl
> Posle ovoga koristimo `opt` kako bismo iskoristili nas novo-registrovani pass

opt -S -enable-new-pm=0  -load ./kk_pass.so --switchtoifelse test.ll -o output.ll

## C) Pristup
> 1) Identifikovati `switch` instrukciju
> 2) Analizirati case-ove
> 3) Kreirati nova poredjenja i preusmeriti na iste blokove
> 4) Zameniti switch instrukciju

***
### 1 - Identifikacija
Mozemo identifikovati switch naredbe time sto pokusamo da izvrsimo dinamicki cast na sve instrukcije, ili koristeci 
```c++
void visitSwitchInst(SwitchInst	 &SI) {}
```
Koja se pokrene za svaku switch instrukciju kroz koju prodje visit() funkcija.

***
### 2 - Analiza
Svaki `case` u switch instrukciji se sastoji od konstantne vrednosti i bloka koji preuzima kontrolu toka ukoliko je vrednost koja se ispituje ista kao i vrednost za taj slucaj. Default case takodje postoji, i u slucaju da se ne izvrsi ni jedan skok, skace se na blok iz default grane.
***



### 3 - Preusmeravanje
Na IR nivou, nasa transformacija bi trebalo da daje ovakav rezultat.

![d8](https://github.com/adjordje/llvm_switch/assets/126694198/fbc9be24-7dbe-4e42-a56e-b036900c9d18)
Potrebne promenljive/informacije su:
>  Nasa incijalna vrednost koju poredimo (input)
> 
> Destinacioni blok default grane

+ Za svaki `case`:
> Vrednost slucaja sa kojim poredimo inicijalnu vrednost
> 
> Destinacioni blok case grane
## D) Implementacija

### Koraci za implementaciju:
- prvi prolaz: U prvom prolazu cemo proci kroz celoukupni Modul, i pokupiti sva pojavljivanja SwitchInst (switch instrukcija)
- drugi prolaz: U drugom prolazu cemo generisati kod, `icmp` (Integer Comparison) instrukciju za poredjenje vrednosti, `br` (Branch) instrukciju kako bismo preusmerili kontrolu toka na odgovarajuce blokove, ove instrukcije cemo umetnuti na pocetku bloka nasih `switch` instrukcija
- treci prolaz: U trecem prolazu cemo obrisati nase `switch` instrukcije

***
## rezultat

> Sa leve strane mozemo uociti da medjureprezentacija bez optimizacijskog pass-a [-switchifelse] -> test.ll 
> 
> i IR kod generisan sa optimizacijskim passom [-switchifelse] -> switchless.ll
>
> pravi razliku u tome sto ne koristi switch instrukcije (SwitchInst)

![image](https://github.com/adjordje/llvm_switch/assets/126694198/daef1538-2e3f-4db4-81fa-f95b6dbfcb8c)




Repozitorijum projekta:
https://github.com/adjordje/llvm_switch
