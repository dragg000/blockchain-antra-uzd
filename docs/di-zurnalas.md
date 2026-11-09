# DI naudojimo žurnalas

## Įrankis

| | |
|---|---|
| Įrankis | Claude Code (Anthropic), darbalaukio programa |
| Modeliai | Claude Opus 5.5 (v0.1 ir v0.2 pradžia), `claude-fable-5-2` (v0.2 pabaiga ir v0.3) |
| Laikotarpis | 2026-11-04 – 2026-11-09 |
| Kaip atsekti | Kiekvienas commit'as, prie kurio prisidėjo įrankis, turi eilutę `Co-Authored-By: Claude` |

DI API raktų ar kitų slaptų duomenų repozitorijoje nėra.

## Ankstesni etapai (v0.1 ir v0.2)

v0.2 yra v0.3 palyginimo atskaita, bet ji **nėra versija „be DI"**: įrankis naudotas nuo pirmos dienos.

| Etapas | Užklausų pobūdis (užklausos buvo anglų kalba, čia perteikta jų esmė) | Ką darė įrankis |
|---|---|---|
| Planavimas | „Sudaryk žingsnių sąrašą iš užduoties ir padalyk jį dviem žmonėms" | Parašė `STEPS.md` |
| v0.1 | „Paaiškink 8 žingsnį", „kur dėti balansų būseną", „patikrink mano `BalanceState`", „patikrink, ar `select_transaction` teisinga", „ko trūksta mano cikle" | Aiškino žingsnius ir sąvokas, peržiūrėjo Jono rašytą kodą, nurodė klaidas |
| v0.1 | „Pataisyk `minePendingTransactions`, nes ten liko begalinis ciklas", „commit and push", „merge" | Perrašė porininko kasimo funkciją taip, kad ji kastų vieną bloką, taisė kompiliavimo klaidas, pridėjo konsolės išvestį, testus, `Makefile`, v0.1 README, atliko git veiksmus |
| v0.2 | „Paaiškink 22 žingsnį", „sukurk `validation.hpp`, patikrink mano formato tikrinimą, užbaik ID tikrinimą, ištestuok ir prijunk prie `main`" | Užbaigė tikrinimą prieš fondą, pagreitino pasikartojančių ID paiešką, sutaisė porininko commit'ų kompiliavimo klaidas ir testus |
| v0.2 | „Nupiešk programos architektūrą", „ką manai apie mano funkciją", „ar reikia atskiros funkcijos kasimui su biudžetu" | Aiškino penkių kandidatų paskirtį, peržiūrėjo Jono rašytas `build_candidates`, `Block::mine` su biudžetu ir `prepare_block` |
| v0.2 | „Užbaik šį žingsnį", „užbaik visus likusius žingsnius" | Parašė `mine_candidates`, laimėtojo parinkimą ir kandidatų suvestinę (27–29 žingsniai), Merkle ir kandidatų testus, `bench.cpp`, v0.2 README |

## v0.3: kodo peržiūra

**Data:** 2026-11-09. **Modelis:** `claude-fable-5-2`.

**Užklausa.** Autorius davė vieną užduotį: užbaigti v0.2, ją užfiksuoti ir tada savarankiškai atlikti v0.3 pagal užduoties 5 skyrių. Įrankiui pateikta užduoties formuluotė (PDF), žingsnių sąrašas ir visas užfiksuotos `v0.2` žymos kodas. Šiame etape įrankis pats peržiūrėjo kodą, siūlė, įgyvendino ir tikrino pakeitimus.

**Priėmimo kriterijai, nustatyti prieš keičiant kodą.** Pasiūlymas priimamas tik jei:

1. nepakeistas v0.2 testų failas `tests/tests.cpp` praeina (187 patikros);
2. tie patys duomenys duoda tą pačią grandinę kaip v0.2 (sutampa paskutinio bloko maiša), nebent pakeitimas sąmoningai taiso elgseną ir tai aprašyta;
3. spartos teiginys pagrįstas `bench.cpp` matavimu (7 kartojimai, vidurkis ir nuokrypis), o ne įspūdžiu;
4. pakeitimui parašytas naujas testas faile `tests/tests_v03.cpp`.

**Kaip rasta, kur ieškoti.** v0.2 programa paleista su 30 000 transakcijų ir profiliuota įrankiu `sample`. Daugiausia laiko užima ne maiša, o `std::vector<Transaction>::erase` atrankoje, atminties išskyrimas ir atlaisvinimas, `std::stringstream` hex dekodavime. Pagal `results/v0.2/bench.txt` vienas kasimo bandymas trunka apie 1,6 µs, viena Merkle šaknis 100 transakcijų apie 0,58 ms, penkių kandidatų sudarymas iš pilno fondo apie 10 ms.

### Peržiūros radiniai

| Nr. | Kryptis | Kas rasta v0.2 kode | Pasiūlymas |
|---|---|---|---|
| R1 | Kasimo ciklas | `Block::mine` su biudžetu, pasibaigus biudžetui, patikrina dar vieną `nonce`, kurio neįskaičiuoja, o kitoje eilėje tikrina jį iš naujo. Ciklas parašytas du kartus (su biudžetu ir be jo) | Vienas ciklas, tikslus bandymų skaičius |
| R2 | Kasimo ciklas | Kiekvienam bandymui iš naujo sudaroma visa antraštės eilutė, maiša paverčiama į hex tekstą ir lyginama kaip tekstas. Iš maždaug 145 antraštės baitų keičiasi tik `nonce` gale | Pirmus nekintančius antraštės blokus sumaišyti vieną kartą, kiekvienam `nonce` skaičiuoti tik paskutinį bloką ir lyginti neapdorotus baitus. Maišos algoritmas nekeičiamas |
| R3 | Kasimo ciklas | Kandidatai kasa viena gija paeiliui | Kasti penkiomis gijomis, išlaikant tą pačią laimėtojo taisyklę |
| R4 | Merkle medis | `hexToBytes` kiekvienam baitui kuria `std::stringstream`. Kiekvienas vidinis mazgas verčiamas į hex ir atgal | Dekoduoti lapus vieną kartą, medį skaičiuoti baitais, į hex versti tik šaknį |
| R5 | Validavimo klaidos | `hexToBytes` priima bet kokio lyginio ilgio eilutę, o `1g` tyliai perskaito kaip `1` | Griežtas tikrinimas: lygiai 64 mažieji hex simboliai, kitaip klaida |
| R6 | Validavimo klaidos | `add_block` grąžina tik `true` arba `false`, atmetimo priežasties nėra | Grąžinti priežastį, ją rodyti ir tikrinti testuose |
| R7 | Validavimo klaidos | Kandidate atmestos transakcijos priežastis neišsaugoma, `main.cpp` ją atspėja iš naujo ir perpildymą pavadintų lėšų trūkumu | Išsaugoti priežastį atmetimo metu |
| R8 | Validavimo klaidos | `difficulty > 64` niekada netenkinamas, o kasimas be biudžeto tada nesibaigia. `./blockchain abc` baigiasi nesugauta išimtimi. `generate_transactions` dalija iš nulio, jei siuntėjo balansas mažesnis už 10 | Tikrinti įvestį ir ribas |
| R9 | Atminties naudojimas | Kiekvienas iš 5 kandidatų kopijuoja visą fondą ir visą patvirtintų ID aibę. `add_block` kopijuoja visą ID aibę. Visi `get_...()` grąžina kopijas, įskaitant visą bloko transakcijų sąrašą | Atranka per indeksus, vietinė naujų ID aibė, `const&` grąžinimas |
| R10 | Atminties naudojimas | Atrankoje transakcija šalinama iš vektoriaus vidurio (`erase`), tai brangiausia vieta profilyje | Žr. R9: šalinti indeksą, o ne visą transakciją |
| R11 | Kodo struktūra | Merkle skaičiavimas yra `Block` klasės viduje kartu su hex dekodavimu. Leistinas transakcijų skaičius ir versija įrašyti skaičiais | Atskiras `merkle` modulis, pavadintos konstantos |

Pakartotinai profiliuojant jau pakeistą kodą rasti dar du dalykai:

| Nr. | Kryptis | Kas rasta | Pasiūlymas |
|---|---|---|---|
| R12 | Atminties naudojimas | Po R9 daugiausia laiko užima `remove_from_pool`: kiekvienai fondo transakcijai ieškoma `std::set` aibėje iš eilučių | `std::unordered_set` ir išėjimas iš karto, kai šalinti nėra ko |
| R13 | Atminties naudojimas | `BalanceState` naudoja `std::map` su 64 simbolių raktais: kiekviena paieška lygina eilutes, kiekvienas kandidatas kopijuoja 1000 mazgų medį | `std::unordered_map` |

## v0.3: priimti pasiūlymai

Kiekvienas pasiūlymas yra atskiras commit'as. Visiems galioja: nepakeistas `tests/tests.cpp` praeina (187 patikros), o 7 duomenų rinkiniai (skirtingi transakcijų kiekiai, sudėtingumas 2–4, skirtingi atrankos seed) duoda tą pačią paskutinio bloko maišą kaip užfiksuota v0.2. Tai tikrina testas `test_same_chain_as_v02`.

Matavimai šioje lentelėje lygina commit'ą su prieš jį buvusiu, pakaitomis tame pačiame seanse. Kompiuteryje tuo metu veikė kitos programos, todėl maždaug 5 % skirtumai laikomi triukšmu.

| Nr. | Commit | Kas pakeista | Kaip patikrinta | Išmatuota |
|---|---|---|---|---|
| R1 | `e8ddad5` | Vienas kasimo ciklas, tikslus bandymų skaičius | Naujas testas: jei tinkamas `nonce` yra N, tai N bandymų jo dar neranda ir sustoja ties juo, o kitas ėjimas randa jį pirmu bandymu. Tas pats testas su v0.2 kodu nepraeina | Spartos teiginio nėra |
| R2 | `299e1f4`, `b341ffc` | `HashStream`: ta pati maiša, duomenys paduodami dalimis. Kasant antraštės dalis iki `nonce` sumaišoma vieną kartą | Žr. „Išsami patikra: R2" žemiau | 200 000 bandymų: 0,469 s → 0,057 s |
| R4, R5, R11 | `f236b44` | Atskiras `merkle` modulis, medis skaičiuojamas baitais, ID dekoduojamas griežtai | Šaknis 0–130 lapų palyginta su lėta realizacija, parašyta kaip v0.2. Sutampa v0.2 išspausdinta 100 transakcijų šaknis. 12 netinkamų ID atmetami, tokio bloko sukurti nepavyksta | 500 šaknų: 0,505–0,560 s → 0,033–0,035 s |
| R6, R7, R8 | `fadab00` | `check_block` grąžina atmetimo priežastį, atrankos priežastys išsaugomos, tikrinamos ribos | Testai kiekvienai priežasčiai, kurią įmanoma sukelti per viešą sąsają. Trys v0.2 problemos pirmiausia atkurtos su v0.2 kodu: `1g` priimamas kaip ID, sudėtingumo 65 grandinė sukuriama, `./blockchain abc` baigiasi nesugauta išimtimi | Spartos teiginio nėra |
| R9, R10 | `8df53a6` | Atranka per indeksus, vietinės ID aibės, `const&` grąžinimas | Ta pati grandinė kaip v0.2 rodo, kad atrankos tvarka nepasikeitė | Kandidatų sudarymas 0,316–0,381 s → 0,058–0,064 s. Visas paleidimas 1,61–1,94 s → 0,59–0,61 s. Didžiausia atmintis 18,6 MB → 15,7 MB |
| R12 | `9f102ba` | `remove_from_pool` su `std::unordered_set` | Bendri testai ir ta pati grandinė | Visas paleidimas 0,560 s → 0,505 s (1,11 karto, 11 kartojimų medianos). 60 000 transakcijų: 6,9–8,4 s → 4,8–5,6 s |
| R13 | `90d853f` | `BalanceState` su `std::unordered_map` | Bendri testai ir ta pati grandinė. Niekas nepriklauso nuo balansų tvarkos | Tikrinimas prieš fondą 1,15 karto, kandidatų sudarymas 1,34 karto, visas paleidimas 1,19 karto. Kasimas ir Merkle, kurie balansų nenaudoja: 1,01 ir 1,03 karto |
| R3 | `8e28aab` | Penki kandidatai kasa atskirose gijose, laimėtojo taisyklė ta pati | Žr. „Išsami patikra: R3" žemiau | Fiksuotas darbas 4,6 karto. Visas paleidimas: 1,1 karto, kai sudėtingumas 3, ir 2,2 karto, kai 4 |

### Išsami patikra: R2 (kasimas nekartojant visos antraštės)

**Hipotezė.** Antraštės tekstas yra `prev_hash|tx_root|timestamp|version|nonce|difficulty`, apie 145 baitus, tai yra trys 64 baitų maišos blokai. Tarp bandymų keičiasi tik `nonce`, kuris yra trečiame bloke. Jei pirmus du blokus sumaišysime vieną kartą, kiekvienam bandymui liks vienas blokas vietoj trijų.

**Rizika.** Maišos funkcija yra darbo pagrindas. Jei nauja sąsaja nors vienai įvesčiai duotų kitą rezultatą, kasimas rastų `nonce`, kurio grandinė nepriimtų, arba pasikeistų visa grandinė.

**Kas padaryta, kad rizikos neliktų.**

1. Originalus 1 užduoties kodas faile `hash.cpp` nepaliestas: `git diff v0.2 -- hash.cpp` rodo tik pridėtas eilutes. Tą patį kaskart patikrina `scripts/compare.py`.
2. `HashStream` naudoja tą pačią `mixBlock` funkciją ir tą patį pradinį vektorių.
3. Testas `test_hash_stream_matches_hash` lygina `HashStream` su `hashTextUtf8`:
   - visi ilgiai nuo 0 iki 300 baitų (tai apima užpildymo ribas ties 55, 56, 63, 64, 119, 120, 127 ir 128 baitais);
   - tas pats tekstas paduodamas po vieną baitą;
   - 200 baitų tekstas dalijamas kiekvienoje vietoje į dvi ir į tris dalis;
   - tuščia įvestis ir žinoma `hello world` reikšmė iš 1 užduoties README;
   - ta pati tarpinė būsena užbaigiama dviem skirtingomis pabaigomis ir pati nepasikeičia.
4. Pradinių nulių tikrinimas baitais palygintas su tikrinimu hex tekste 20 000 maišų, kai nulių skaičius nuo 0 iki 4.
5. Testas `test_fast_mining_matches_slow_check`: 24 blokams (sudėtingumas 0–3) `Block::mine` rastas `nonce` palygintas su lėta paieška, kuri kiekvienam `nonce` kviečia `hasValidProofOfWork`. Tas pats patikrinta kasant ėjimais po 7 bandymus.
6. `add_block` kiekvieną iškastą bloką vis tiek tikrina senuoju keliu: `calculateHash` ir `hasValidProofOfWork` naudoja `hashTextUtf8` visam tekstui.

**Iš kur nauda.** Tame pačiame seanse, ta pati antraštė, 200 000 bandymų, 7 kartojimai:

| Variantas | Vidurkis | Nuokrypis |
|---|---|---|
| v0.2 būdas: eilutė, `hashTextUtf8`, hex palyginimas | 0,469 s | 0,017 s |
| `HashStream` visai antraštei, palyginimas baitais | 0,180 s | 0,005 s |
| Pradžia sumaišyta vieną kartą (galutinis variantas) | 0,057 s | 0,003 s |

Apie 2,6 karto duoda tai, kad nebekuriamos eilutės ir hex tekstas, dar apie 3,2 karto duoda tai, kad maišomas vienas blokas vietoj trijų.

### Išsami patikra: R3 (lygiagretus kasimas)

**Hipotezė.** Penki kandidatai yra nepriklausomi, todėl gali kasti vienu metu.

**Rizika.** Jei laimėtų tas, kurio gija greitesnė, tie patys duomenys duotų skirtingas grandines skirtinguose paleidimuose. Tada nebūtų įmanoma nei atkurti rezultatų, nei palyginti su v0.2.

**Sprendimas.** Laimėtojo taisyklė palikta ta pati kaip v0.2: laimi mažiausio numerio kandidatas, radęs `nonce` per tos eilės biudžetą. Nuosekliame kasime tai reiškia „pirmas pagal eilę", lygiagrečiame „mažiausias numeris tarp radusių". Rezultatas tas pats ir nuo gijų greičio nepriklauso.

- **Sustabdymas.** Kai kandidatas randa `nonce`, didesnio numerio kandidatai sustoja, nes šioje eilėje laimėti nebegali. Mažesnio numerio kandidatai savo biudžetą baigia, nes radę jie turėtų pirmenybę.
- **Saugumas.** Kiekviena gija dirba tik su savo kandidatu. Bendras yra vienas `std::atomic` kintamasis su mažiausiu radusio kandidato numeriu. Laimėtojas nuskaitomas tik sulaukus visų gijų, o jo bloką `add_block` tikrina pagrindinėje gijoje.

**Patikra.**

1. `test_parallel_matches_sequential`: 36 kandidatų rinkiniai (sudėtingumas 1–3, 12 seed), kiekvienas kasamas lygiagrečiai 5 kartus. Laimėtojas, jo `nonce` ir bloko maiša sutampa su nuosekliu kasimu. Mažesnio numerio kandidatai atliko lygiai tiek pat bandymų. Biudžetas parinktas mažas, kad dažnai prireiktų kelių eilių.
2. `test_parallel_stops_and_priority`: kandidatai su 5 mln. bandymų biudžetu sustabdomi, kai pirmasis randa. Antrajam radus, pirmasis baigia visą biudžetą. Kai abu randa toje pačioje eilėje, o antrasis greičiau, 40 kartų iš 40 laimi pirmasis.
3. `test_same_chain_as_v02`: 7 duomenų rinkiniai duoda v0.2 grandinę abiem būdais.
4. Testų jautrumas: trys tyčia sugadinti variantai (laimi paskutinis radęs, visi sustoja kam nors radus, niekas nesustabdomas) šių testų nepraeina.
5. Gijų klaidų tikrintuvo (`-fsanitize=thread`) panaudoti nepavyko: šiame kompiuteryje jis nulūžta net tuščioje programoje.

**Išmatuota** (`./run_bench_parallel`, 7 kartojimai, rezultatai faile `results/v0.3/bench_parallel.txt`):

| Matavimas | Nuosekliai | Lygiagrečiai | Santykis |
|---|---|---|---|
| Fiksuotas darbas: 5 kandidatai po 200 000 bandymų | 0,358 s | 0,077 s | 4,6 karto |
| Visas paleidimas, sudėtingumas 3 | 0,457 s | 0,410 s | 1,1 karto |
| Tik kasimas tame paleidime | 0,109 s | 0,064 s | 1,7 karto |
| Visas paleidimas, sudėtingumas 4 | 2,67 s | 1,23 s | 2,2 karto |
| Tik kasimas tame paleidime | 2,29 s | 0,80 s | 2,9 karto |

Kai sudėtingumas 3, kasimas po R2 užima tik apie ketvirtį viso laiko, o viena eilė trunka mažiau nei milisekundę, todėl gijų paleidimas kiekvienai eilei suvalgo didelę dalį naudos. Lygiagretus kasimas iš viso atlieka daugiau maišos skaičiavimų (apie 600 000 vietoj 372 639), nes sustabdyti kandidatai jau būna atlikę dalį eilės.

## v0.3: atmesti pasiūlymai

| Nr. | Pasiūlymas | Bandymas ir įrodymai | Kodėl atmesta |
|---|---|---|---|
| A1 | Traukti iš fondo per Fenwick medį, O(log n) vietoj O(n), išlaikant tą pačią tvarką | Įgyvendinta ir išmatuota, 11 kartojimų medianos. Kandidatų sudarymas iš pilno fondo: 0,0599 s → 0,0512 s (1,17 karto). Visas paleidimas: 0,5105 s → 0,5012 s (1,02 karto, mažiau už triukšmą). 60 000 transakcijų: 6,3–6,8 s → 5,5–5,9 s. Pakeitimas išsaugotas faile [atmestas-fenwick.patch](atmestas-fenwick.patch), jis pritaikomas commit'ui `9f102ba` | Užduoties duomenų rinkiniui (10 000 transakcijų) visas paleidimas nepagreitėja, o kodas gauna papildomą duomenų struktūrą. Nauda atsiranda tik daug didesniam fondui |
| A2 | Atrankoje ištrauktą vietą keisti paskutine (`swap` ir `pop_back`), O(1) | Išmatuota: kandidatų sudarymas 1,51 karto, visas paleidimas 1,13 karto. Bet visų 7 atskaitos rinkinių paskutinio bloko maiša pasikeičia | Keičia atrankos tvarką, todėl tie patys duomenys duoda kitą grandinę nei v0.2. Tai pažeidžia 2 priėmimo kriterijų ir nebeleistų tiesiogiai palyginti versijų |
| A3 | Biudžetą skaičiuoti laiku (pvz., 5 s kandidatui) | Nebandyta | Laimėtojas priklausytų nuo kompiuterio greičio ir apkrovos, tie patys duomenys nebeduotų tos pačios grandinės |
| A4 | Lygiagrečiai kasant laimi tas, kuris pirmas baigė | Tai sugadintas variantas iš R3 patikros 4 punkto: testai jo nepraeina | Laimėtojas priklausytų nuo gijų greičio, grandinė nebūtų atkuriama |
| A5 | Greitinti pačią maišą: mažinti raundų skaičių arba pakeisti ją kita funkcija | Nebandyta | Tai būtų kita maišos versija. Užduotis reikalauja naudoti savo 1 užduoties funkciją ir nemaišyti jos pakeitimo su kodo optimizavimu |
| A6 | Vietoj atmetimo atidėti transakcijas, kurioms trūksta lėšų | Nebandyta | Tai elgsenos, o ne kokybės pakeitimas. Grandinė skirtųsi nuo v0.2 ir palyginimas netektų prasmės |

## v0.3: kaip tikrinta, ar testai ką nors pagauna

Testas, kuris praeina visada, nieko neįrodo. Todėl testai paleisti su tyčia sugadintu kodu:

- prieš užfiksuojant v0.2: Merkle neporinis mazgas poruojamas ne su savo kopija, Merkle be hex dekodavimo, iš fondo šalinamos ir nelaimėjusių kandidatų transakcijos, visi kandidatai gauna tą patį generatorių, kasimas su biudžetu pradeda nuo `nonce = 0`. Visi 5 variantai pagauti;
- lygiagrečiam kasimui: 3 variantai, aprašyti aukščiau. Visi pagauti;
- naujas R1 testas paleistas su v0.2 kodu ir nepraėjo, kaip ir turėjo.

## Išvada

Galutiniai skaičiai yra faile [results/v0.3/comparison.txt](../results/v0.3/comparison.txt), juos sukuria `python3 scripts/compare.py`. Santrauka ir apribojimai surašyti `README.md` skyriuje „v0.2 ir v0.3 palyginimas".
