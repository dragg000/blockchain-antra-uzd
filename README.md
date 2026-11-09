# Supaprastinta blokų grandinė

Blokų grandinių technologijos, 2 užduotis (Vilniaus universitetas, 2026). Autoriai: Jonas Salys ([dragg000](https://github.com/dragg000)) ir [zukasdovis](https://github.com/zukasdovis).

Tai **v0.2** versija: viename procese veikianti grandinė su sąskaitų modeliu, Proof-of-Work, Merkle medžiu, trijų etapų tikrinimu ir penkiais kandidatiniais blokais kiekvienam grandinės aukščiui. Darbo žingsniai surašyti faile [STEPS.md](STEPS.md).

| Versija | Kas joje yra |
|---|---|
| `v0.1` | Bazinė grandinė: vartotojai, transakcijos, blokai, PoW, konsolės išvestis |
| `v0.2` | Merkle medis, tikrinimas prieš fondą, atrenkant ir priimant bloką, penki kandidatai su bandymų biudžetu, teisingumo testai |

## Paleidimas

Kalba: C++17. Papildomų bibliotekų nereikia, užtenka C++ kompiliatoriaus ir `make`.

```bash
make
```

```bash
./blockchain
```

```bash
./blockchain 250
```

```bash
make test
```

```bash
make bench
```

- `./blockchain` paleidžia visą rinkinį: 1000 vartotojų ir 10 000 transakcijų.
- `./blockchain 250` paleidžia mažą pavyzdį su 250 transakcijų (3 blokai).
- `make test` paleidžia teisingumo testus.
- `make bench` išmatuoja kasimo, Merkle, tikrinimo ir viso paleidimo laiką (7 kartojimai, vidurkis ir nuokrypis).

## Failai

| Failas | Kas jame |
|---|---|
| `user.hpp`, `transaction.hpp`, `setup.cpp` | Vartotojai, transakcijos, serializacija ir jų generavimas iš seed |
| `hash.hpp`, `hash.cpp` | Maišos funkcija iš 1 užduoties |
| `balance_state.hpp` | Balansų būsena ir vienos transakcijos pritaikymas |
| `block_h.hpp`, `block_h.cpp` | Bloko antraštė, blokas, bloko maiša, PoW, Merkle šaknis, kasimas |
| `blockchain.hpp`, `blockchain.cpp` | Grandinė, genesis, bloko tikrinimas ir priėmimas |
| `validation.hpp`, `validation.cpp` | Transakcijos tikrinimas ir priėmimas į fondą |
| `mining.hpp`, `mining.cpp` | Atranka į kandidatą, penki kandidatai, kasimas paeiliui, kasimo ciklas |
| `main.cpp` | Programos eiga ir suvestinė |
| `tests/tests.cpp` | Teisingumo testai |
| `bench.cpp` | Spartos matavimas |
| `results/v0.2/` | Išsaugota šios versijos išvestis: visas ir mažas paleidimas, testai, matavimai |

## Modelis

**Vartotojas:** `name`, `public_key`, `balance`. `public_key` yra vardo (`user0`, `user1`, ...) maiša, tai tik imituojamas viešas identifikatorius: jis neįrodo rakto nuosavybės. Pradiniai balansai yra nuo 100 iki 1 000 000 mokomųjų vienetų ir lieka išsaugoti `User` objektuose. Einamieji balansai laikomi atskirai, `BalanceState` klasėje, todėl pradinę būseną visada galima atkurti.

**Transakcija:** `sender`, `receiver`, `amount`, `sequence`, `transaction_id`. Sumos yra sveikieji skaičiai (`uint64_t`). `sequence` yra bendras unikalus generatoriaus numeris. `transaction_id` yra šios eilutės UTF-8 baitų maiša:

```text
sender|receiver|amount|sequence
```

Pavyzdžiui, `a12f|b34e|100|5`. Skaičiai rašomi dešimtaine forma be tarpų ir pradinių nulių, eilutės pabaigos nėra. Pats ID į maišos įvestį neįeina. Kūrimui ir tikrinimui naudojama ta pati funkcija `Transaction::serialize()`.

**Bloko antraštė:** `prev_hash`, `tx_root`, `timestamp`, `version`, `nonce`, `difficulty`. Bloko maiša skaičiuojama iš šių šešių laukų, sujungtų `|` ženklu šia tvarka. Blokas taip pat turi tikslia tvarka išsaugotą transakcijų sąrašą, kurį su antrašte susieja `tx_root`.

**`version`:** nuo v0.2 lygi 2, nes pasikeitė `tx_root` skaičiavimo taisyklė (v0.1 buvo 1). Blokas su kita versija atmetamas.

**`timestamp`:** fiksuota reikšmė 1000 plius bloko aukštis, o ne tikras laikrodis. Dėl to tie patys įvesties duomenys kiekvieną kartą duoda tas pačias maišas.

**Proof-of-Work:** bloko maišos hex užrašas turi prasidėti `difficulty` nulių. Šiame paleidime `difficulty = 3`. Ieškant keičiamas tik `nonce`, jis didinamas po vieną nuo 0. Tikrinant maiša visada perskaičiuojama iš antraštės laukų, išsaugota reikšme nepasitikima.

**Genesis:** `prev_hash` yra 64 nuliai, `tx_root` yra tuščios baitų sekos maiša, `timestamp = 0`, transakcijų nėra. Genesis yra PoW išimtis: jis nekasamas ir netikrinamas.

## Maišos funkcija

Naudojama pirmoje užduotyje sukurta funkcija: [dragg000/blockchain-pirma-uzd](https://github.com/dragg000/blockchain-pirma-uzd), commit `1f4df0a`, failas `src/main.cpp`. Į `hash.cpp` perkeltos tik maišos funkcijos, algoritmas nepakeistas.

Poroje pasirinkome Jono algoritmą dėl šių priežasčių:

- išvestis yra fiksuoto ilgio, 256 bitų, užrašoma 64 mažosiomis hex raidėmis ir išlaiko pradinius nulius, todėl tinka ir ID, ir PoW sąlygai;
- ji deterministinė ir priima bet kokio ilgio baitų seką, įskaitant tuščią (to reikia genesis `tx_root`);
- 1 užduoties bandymuose tarp 4 × 100 000 atsitiktinių porų kolizijų nerasta, o pakeitus vieną simbolį vidutiniškai pasikeičia apie 50 % bitų;
- ji jau parašyta C++17 be išorinių bibliotekų, todėl perkelta nekeičiant kodo.

Ši maiša yra mokomoji. Sėkmingi testai neįrodo kriptografinio saugumo.

## Merkle medis

- Lapai yra transakcijų ID jų tvarka bloke.
- Vidinis mazgas yra `H(kairiojo vaiko baitai || dešiniojo vaiko baitai)`. Hex ID prieš jungiant dekoduojami į 32 baitus.
- Jei lygyje lieka neporinis mazgas, jis poruojamas su savo kopija.
- Vienas lapas pats yra šaknis.
- Tuščio bloko (genesis) šaknis apibrėžta atskirai: tuščios baitų sekos maiša.

Testuose šaknis 1, 2, 3 ir 5 transakcijoms perskaičiuojama atskiru kodu ir palyginama. Sukeitus dviejų transakcijų tvarką arba pakeitus transakcijos turinį ir perskaičiavus jos ID, šaknis pasikeičia.

**Pastebėta savybė.** Dėl neporinio mazgo dubliavimo sąrašai `[a, b, c]` ir `[a, b, c, c]` duoda tą pačią šaknį. Tai ne maišos kolizija, o medžio taisyklės pasekmė. Grandinė antrojo bloko vis tiek nepriima, nes jame kartojasi transakcijos ID. Šis atvejis įtrauktas į testus.

## Trys tikrinimo etapai

Vien patekimas į fondą negarantuoja, kad transakciją bus galima įvykdyti bloke, todėl tikrinama tris kartus.

**1. Prieš priimant į fondą** (`add_transaction_to_pool`). Tikrinama šia tvarka, pirma rasta priežastis grąžinama:

| Priežastis | Kada |
|---|---|
| `netinkamas formatas` | `sender`, `receiver` arba ID nėra 64 mažųjų hex simbolių eilutė |
| `siuntejas ir gavejas sutampa` | Mokėjimas sau |
| `nezinomas dalyvis` | Siuntėjo arba gavėjo nėra būsenoje |
| `netinkama suma` | Suma lygi 0 arba didesnė už visą lėšų sumą |
| `neteisingas ID` | Perskaičiuota maiša nesutampa su saugomu ID |
| `pasikartojantis ID` | Toks ID jau yra fonde arba patvirtintoje grandinėje |

Suma laikoma `uint64_t`, todėl neigiamos reikšmės tipas neleidžia: `-1` virsta labai dideliu skaičiumi, kurį atmeta sumos riba. Balansas šiame etape netikrinamas, nes patekimas į fondą lėšų nerezervuoja.

**2. Atrenkant į kandidatą** (`select_transaction`). Patvirtinti balansai nukopijuojami į laikiną būseną. Transakcijos traukiamos atsitiktinai, be pasikartojimų, tikrinamos ir iškart pritaikomos tai kopijai jų tvarka bloke. Čia tikrinamas ir balanso pakankamumas, ir gavėjo balanso perpildymas. Pavyzdys iš užduoties: A turi 100, T1 moka 70, T2 moka 60. Įtraukiama tik ta, kuri patikrinta pirma. Tikrieji balansai atrankos metu nesikeičia.

**3. Prieš priimant iškastą bloką** (`Blockchain::add_block`). Iš naujo tikrinama:

1. `prev_hash` lygus dabartinio paskutinio bloko maišai;
2. `version` yra 2;
3. transakcijų yra nuo 1 iki 100;
4. perskaičiuota Merkle šaknis sutampa su `tx_root`;
5. bloko `difficulty` lygus grandinės reikalaujamam (kandidatas negali jo susimažinti);
6. perskaičiuota bloko maiša tenkina PoW;
7. visos transakcijos iš eilės tinka patvirtintų balansų kopijoje ir jų ID nesikartoja nei bloke, nei grandinėje.

Tik praėjus visoms patikroms vienu veiksmu pridedamas blokas, pakeičiami balansai ir papildoma patvirtintų ID aibė. Jei bent viena patikra nepavyksta, nepritaikoma niekas.

## Penki kandidatai kitam blokui

Kiekvienam grandinės aukščiui:

1. **Sudaromi 5 kandidatai.** Visi remiasi tuo pačiu `prev_hash`, tuo pačiu `timestamp` ir tais pačiais patvirtintais balansais. Kiekvienas turi savo laikiną balansų kopiją ir iš viso fondo atsirenka iki 100 transakcijų. Atsitiktinių skaičių generatorius tarp kandidatų tęsiamas, todėl kandidatai skiriasi. Jų transakcijos gali persidengti, bet viename kandidate nesikartoja.
2. **Biudžetas.** Kiekvienas kandidatas per vieną eilę gauna **2000 bandymų**. Biudžetas skaičiuojamas bandymais, o ne laiku, kad tie patys duomenys visada duotų tą pačią grandinę.
3. **Tvarka.** Kandidatai kasa paeiliui: 1, 2, 3, 4, 5. Tai imituoja vienu metu dirbančius kasėjus viename procese: daugiausia bandymų reikalaujantis kandidatas nelaimi vien todėl, kad buvo pirmas.
4. **Jei sprendimo nėra.** Jei per visą eilę niekas nerado tinkamo `nonce`, biudžetas padvigubinamas (4000, 8000, ...) ir kiekvienas kandidatas tęsia nuo savo dar netikrintų `nonce`. Sudėtingumas nemažinamas.
5. **Bendra riba.** Daugiausia 10 eilių, tai yra iki 2 046 000 bandymų vienam kandidatui. Jei ir tada sprendimo nėra, programa sustoja su aiškiu pranešimu. Jau priimti blokai lieka galioti, o nebaigtas blokas nieko nepakeičia.
6. **Laimėtojo taisyklė.** Laimi pirmas kandidatas (eilės tvarka), radęs tinkamą `nonce`. Po jo einantys kandidatai toje eilėje nebekasa.
7. **Pakartotinė patikra.** Laimėtojo blokas perduodamas `add_block`, kuris viską patikrina iš naujo (3 etapas).
8. **Nelaimėję kandidatai.** Jie dirbo tik su savo kopijomis, todėl išmetami be jokio poveikio. Iš fondo pašalinamos tik laimėtojo įtrauktos ir atmestos transakcijos. Kitam blokui kandidatai sudaromi iš naujo.

Kiekvienam blokui spausdinama visų penkių kandidatų suvestinė, o pabaigoje bendri skaičiai.

### Kodėl tai dar nėra P2P konsensusas

- Viskas vyksta viename procese ir viename kompiuteryje. Yra tik viena grandinės ir balansų kopija, nėra nepriklausomų mazgų, kurie kiekvienas tikrintų bloką savarankiškai.
- Nėra tinklo: transakcijos ir blokai niekur nesiunčiami, nėra vėlavimų, prarastų pranešimų ar skirtingų fondų skirtinguose mazguose.
- Kandidatai nekonkuruoja tikru laiku. Laimėtoją lemia fiksuota eilės tvarka ir biudžetas, o ne tai, kas iš tikrųjų pirmas rado sprendimą.
- Nėra šakų: du blokai tame pačiame aukštyje niekada nepriimami, todėl nereikia nei ilgiausios grandinės taisyklės, nei persitvarkymų.
- Visi kandidatai yra tos pačios programos dalys ir vienas kitu pasitiki. Nėra skaitmeninių parašų, atlygio už bloką ar sudėtingumo perskaičiavimo.

Penki kandidatai parodo tik tai, kad keli skirtingi blokai gali varžytis dėl to paties aukščio ir kad priimamas vienas, o kiti neturi poveikio.

## Netinkamos transakcijos

Struktūriškai netinkamos transakcijos atmetamos prieš fondą su priežastimi iš lentelės aukščiau.

Laikinai netinkamoms (siuntėjui tuo metu nepakanka lėšų) pasirinkome **atmetimą**, o ne atidėjimą. Kandidate atmesta transakcija iš fondo pašalinama tada, kai tas kandidatas laimi, ir išvedama galutinėje suvestinėje su priežastimi. Atmetimas priklauso nuo atsitiktinės traukimo tvarkos, todėl vėliau gavęs lėšų siuntėjas šios transakcijos nebeįvykdys. Nelaimėjusių kandidatų atmestos transakcijos lieka fonde.

Kiekvienas ratas pašalina iš fondo bent vieną transakciją arba sustoja, todėl ciklas visada baigiasi.

## Atkuriamumas

| Parametras | Reikšmė |
|---|---|
| Vartotojų seed | 1 |
| Transakcijų seed | 2 |
| Atrankos seed | 3 |
| Generatorius | `std::mt19937_64`, reikšmės gaunamos su `rng() %` |
| Vartotojai | 1000, balansai nuo 100 iki 1 000 000 |
| Transakcijos | 10 000 (arba pirmasis programos argumentas) |
| Sudėtingumas | 3 |
| Kandidatų vienam blokui | 5 |
| Pradinis biudžetas | 2000 bandymų kandidatui per eilę |
| Biudžeto didinimas | Dvigubinamas po kiekvienos nesėkmingos eilės, daugiausia 10 eilių |
| `timestamp` | 1000 + bloko aukštis |
| Tikrinta su | Apple clang 17, arm64 (Apple M4), macOS, `-std=c++17 -O2` |

Sąmoningai nenaudojami `std::uniform_int_distribution` ir `std::shuffle`, nes jų rezultatai skirtingose standartinėse bibliotekose gali skirtis. Du paleidimai duoda tuos pačius blokus, maišas ir laimėtojus, skiriasi tik išmatuotas laikas.

## Rezultatai

Visa išvestis išsaugota aplanke [results/v0.2](results/v0.2).

Vieno bloko išvestis (antrasis blokas: pirmas kandidatas išnaudojo biudžetą, laimėjo antras):

```text
Blokas #2
  transakcijos:  100
  prev_hash:     0009d2556abc1aaa7270a3dd2e5c27100f1aa77bcd5874b4e5c8a36427621092
  tx_root:       b59fca11963efa994f32eafa45c8ef252ffcd8d58427f47873dabb85ea09862d
  hash:          0000c744ea4fb15f2beb7b01ca7e981e70ef60cc6af4db88b47090afced0f21d
  difficulty:    3
  nonce:         132
  bandymai:      133
  kasimo laikas: 0.000204125 s
  kandidatai:    nr  transakcijos  eiles  bandymai  laikas, s  rezultatas
                  1           100      1      2000   0.003078  nerado
                  2           100      1       133   0.000204  laimejo
                  3           100      0         0   0.000000  nekase
                  4           100      0         0   0.000000  nekase
                  5           100      0         0   0.000000  nekase
```

Galutinė suvestinė:

```text
Suvestine
  vartotojai:             1000
  transakcijos:           10000
  priimti blokai:         100
  itrauktos transakcijos: 9999
  atmestos pries fonda:   0
  atmestos atrenkant:     1
  likusios transakcijos:  0
  pradine lesu suma:      498753171
  galutine lesu suma:     498753171
  bendras laikas:         1.87696 s

Kandidatai
  kandidatu vienam blokui: 5
  pradinis biudzetas:      2000 bandymu per eile
  visu kandidatu bandymai: 372639
  visu kandidatu laikas:   0.596182 s
  biudzetas dvigubintas:   7 k.
  laimeti blokai:          nr 1: 44, nr 2: 30, nr 3: 14, nr 4: 3, nr 5: 9

Atmesta 5dc4e0c85d3d449609dfcbd08e7eec2f5e6e9dc589b02aa8bad1ae81d2520614 (suma 92632): siuntejui nepakako lesu
```

Paskutiniame bloke yra 99 transakcijos, nes viena iš 10 000 buvo atmesta.

## Testai

`make test` atlieka 187 patikras, visos praeina. Lentelė atitinka bendrą užduoties testų rinkinį.

| Scenarijus | Kaip tikrinama | Laukiama savybė | Rezultatas |
|---|---|---|---|
| Galiojantis mokėjimas | A 100, B 50, C 0, mokėjimai A→B 30 ir B→C 20 | Lieka A 70, B 60, C 20, suma 150, mokėjimas įtrauktas vieną kartą | Praeina |
| Galutinis blokas su <100 įrašų | 250 transakcijų | Blokai po 100, 100 ir 50 | Praeina |
| Nežinomas gavėjas | Gavėjo nėra būsenoje | Atmetama, priežastis `nezinomas dalyvis` | Praeina |
| Suma 0 arba neigiama | Suma 0, suma `-1` kaip `uint64_t`, suma virš visų lėšų | Atmetama, priežastis `netinkama suma` | Praeina |
| Pakeistas ID | Pakeista suma arba pats ID | Atmetama, priežastis `neteisingas ID` | Praeina |
| Du mokėjimai viršija balansą | A turi 100, T1 moka 70, T2 moka 60, 50 skirtingų seed | Kiekvieną kartą įtraukiama tik viena, balansai nesikeičia | Praeina |
| Dubliuotas ID | Ta pati transakcija į fondą du kartus, tas pats ID du kartus bloke | Antroji atmetama, priežastis `pasikartojantis ID`, blokas nepriimamas | Praeina |
| Patvirtinto mokėjimo pakartojimas | Iškasta transakcija pateikiama dar kartą į fondą ir naujame bloke | Atmetama, balansai nesikeičia | Praeina |
| Pakeistas bloko turinys | Ta pati antraštė, kita transakcija | PoW nebetinka, blokas atmetamas | Praeina |
| Neteisingas `prev_hash` | Iškastas blokas su kita nuoroda | Blokas atmetamas | Praeina |
| Netinkamas PoW | Neiškastas blokas, blokas su mažesniu `difficulty`, sena `version` | Blokas atmetamas, grandinė ir balansai nesikeičia | Praeina |
| Merkle 1 / 2 / 3 / 5 | Šaknis perskaičiuojama atskiru kodu | Sutampa, keičiasi sukeitus tvarką ar pakeitus turinį | Praeina |
| Nelaimėję kandidatai | Vienas ratas su 5 kandidatais | Balansai lygūs pradiniams plius tik laimėtojo transakcijos, kitų transakcijos lieka fonde | Praeina |

Papildomai tikrinama: serializacija (`a12f|b34e|100|5`, o suma 1 + `sequence` 23 skiriasi nuo sumos 12 + `sequence` 3), tas pats seed duoda tas pačias maišas ir tą patį paskutinį bloką, gavėjo balanso perpildymas, ciklas sustoja, kai nieko negalima įtraukti, biudžetas baigiasi ir kasimas tęsiamas nuo išsaugoto `nonce`, kasimas sustoja po leistinų eilių, 2000 transakcijų paleidime nėra dublikatų ir neigiamų balansų.

## Spartos atskaita

`make bench` rezultatai šiai versijai (7 kartojimai, Apple M4, `-O2`, laikas be konsolės išvedimo). Tai atskaita v0.3 palyginimui.

| Matavimas | Vidurkis | Nuokrypis |
|---|---|---|
| Kasimas (fiksuotas bandymu skaicius): 200000 bandymu per karta, viena antraste, sudetingumas 16 | 0.322235 s | 0.002472 s |
| Merkle saknis: 500 skaiciavimu per karta, 100 transakciju | 0.288939 s | 0.001260 s |
| Tikrinimas pries fonda: 10000 transakciju per karta, priimta 10000 | 0.022699 s | 0.000294 s |
| Kandidatu sudarymas: 20 kartu po 5 kandidatus is 10000 transakciju fondo, kandidate 100 transakciju | 0.199395 s | 0.000797 s |
| Visas paleidimas (be isvedimo): 100 bloku, 9999 transakciju, lesu suma 498753171 | 1.851235 s | 0.007030 s |

## DI naudojimas

Šiame darbe nuo pat pradžių naudotas **Claude Code** (Anthropic), modeliai Claude Opus 5.5 ir `claude-fable-5-2`. Kiekvienas commit'as, prie kurio prisidėjo įrankis, turi eilutę `Co-Authored-By: Claude`, todėl tai matyti ir istorijoje (`git log`).

| Etapas | Kam naudota |
|---|---|
| Planavimas | Žingsnių sąrašas `STEPS.md` ir jo padalijimas dviem žmonėms |
| v0.1 | Sąvokų ir žingsnių aiškinimas, Jono rašyto kodo peržiūra, kompiliavimo klaidų taisymas, kasimo ciklo sutvarkymas, konsolės išvestis, testai, `Makefile`, README |
| v0.2 | Tikrinimo prieš fondą užbaigimas ir pagreitinimas, porininko commit'ų kompiliavimo klaidų taisymas, kasimas paeiliui (`mine_candidates`), laimėtojo parinkimas, kandidatų suvestinė, Merkle ir kandidatų testai, spartos matavimas, šis README |

Išsamus užklausų žurnalas, priimti ir atmesti pasiūlymai bei jų patikra bus pateikti kartu su v0.3.

## Indėliai

| Autorius | Darbai |
|---|---|
| Jonas Salys | Vartotojai ir transakcijos, serializacija, generavimas iš seed, balansų būsena, atranka į kandidatą, atmetimo taisyklė, tikrinimas prieš fondą, penkių kandidatų sudarymas, kasimas su biudžetu, neiškasto bloko paruošimas |
| zukasdovis | Bloko antraštė ir blokas, grandinės klasė ir genesis, `tx_root`, bloko maiša, PoW, pirmasis kasimo ciklo variantas, Merkle medis, tikrinimas atrenkant, bloko tikrinimas ir priėmimas |

Plane 27–29 žingsniai (biudžetas, laimėtojas, kandidatų suvestinė) buvo skirti porininkui. Jam susirgus, juos atliko Jonas su Claude Code.

## Žinomi v0.2 apribojimai

- Kai kandidato eilės biudžetas baigiasi, papildomai patikrinamas dar vienas `nonce`, kuris į bandymų skaičių neįtraukiamas. Tas pats `nonce` kitoje eilėje tikrinamas iš naujo. Grandinės teisingumui tai įtakos neturi, tik kiekvienos nesėkmingos eilės bandymų skaičius yra vienu mažesnis už tikrąjį.
- Kiekvienas kandidatas kopijuoja visą fondą ir visą patvirtintų ID aibę, todėl kandidatų sudarymas užima daugiau laiko nei pats kasimas.
- Bloko atmetimo priežastis negrąžinama, `add_block` grąžina tik taip arba ne.
- Kandidate atmestos transakcijos priežastis suvestinėje nustatoma iš naujo, o ne išsaugoma atmetimo metu.
- Kandidatai kasa nuosekliai, viena gija.
