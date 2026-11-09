# Supaprastinta blokų grandinė

Blokų grandinių technologijos, 2 užduotis (Vilniaus universitetas, 2026). Autoriai: Jonas Salys ([dragg000](https://github.com/dragg000)) ir [zukasdovis](https://github.com/zukasdovis).

Tai **v0.3** versija: viename procese veikianti grandinė su sąskaitų modeliu, Proof-of-Work, Merkle medžiu, trijų etapų tikrinimu ir penkiais kandidatiniais blokais kiekvienam grandinės aukščiui. v0.3 yra v0.2, peržiūrėta ir patobulinta su DI įrankiu: ji duoda tą pačią grandinę, bet visas paleidimas trunka apie 7 kartus trumpiau. Palyginimas yra skyriuje [v0.2 ir v0.3 palyginimas](#v02-ir-v03-palyginimas).

| Versija | Kas joje yra |
|---|---|
| `v0.1` | Bazinė grandinė: vartotojai, transakcijos, blokai, PoW, konsolės išvestis |
| `v0.2` | Merkle medis, tikrinimas prieš fondą, atrenkant ir priimant bloką, penki kandidatai su bandymų biudžetu, teisingumo testai |
| `v0.3` | Su DI įrankiu patobulinta v0.2: greitesnis kasimas ir Merkle medis, lygiagretus kandidatų kasimas, atmetimo priežastys, mažiau kopijavimo, palyginimas su užfiksuota v0.2 |

Darbo žingsniai surašyti faile [STEPS.md](STEPS.md), DI naudojimo žurnalas faile [docs/di-zurnalas.md](docs/di-zurnalas.md).

## Paleidimas

Kalba: C++17. Papildomų bibliotekų nereikia, užtenka C++ kompiliatoriaus su gijų palaikymu ir `make`. Palyginimo scenarijui dar reikia `python3` ir `git`.

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
./blockchain --nuosekliai
```

```bash
make test
```

```bash
make bench
```

```bash
python3 scripts/compare.py
```

- `./blockchain` paleidžia visą rinkinį: 1000 vartotojų ir 10 000 transakcijų. Kandidatai kasami lygiagrečiai.
- `./blockchain 250` paleidžia mažą pavyzdį su 250 transakcijų (3 blokai). Skaičius gali būti nuo 1 iki 1 000 000.
- `./blockchain --nuosekliai` kasa viena gija, kaip v0.2. `--lygiagreciai` yra numatytasis būdas. Abu duoda tą pačią grandinę.
- `make test` paleidžia bendrus v0.2 ir v0.3 testus ir papildomus v0.3 testus.
- `make bench` išmatuoja kasimo, Merkle, tikrinimo ir viso paleidimo laiką, tada palygina nuoseklų ir lygiagretų kasimą.
- `python3 scripts/compare.py` iš žymos `v0.2` atkuria senąją versiją ir palygina ją su dabartine: testai, grandinė, sparta, atmintis.

## Failai

| Failas | Kas jame |
|---|---|
| `user.hpp`, `transaction.hpp`, `setup.cpp` | Vartotojai, transakcijos, serializacija ir jų generavimas iš seed |
| `hash.hpp`, `hash.cpp` | Maišos funkcija iš 1 užduoties ir jos srautinė sąsaja `HashStream` |
| `merkle.hpp`, `merkle.cpp` | Merkle šaknis ir griežtas ID dekodavimas |
| `balance_state.hpp` | Balansų būsena, transakcijos patikra ir pritaikymas |
| `block_h.hpp`, `block_h.cpp` | Bloko antraštė, blokas, bloko maiša, PoW, kasimas su biudžetu |
| `blockchain.hpp`, `blockchain.cpp` | Grandinė, genesis, bloko tikrinimas su priežastimi ir priėmimas |
| `validation.hpp`, `validation.cpp` | Transakcijos tikrinimas ir priėmimas į fondą |
| `mining.hpp`, `mining.cpp` | Atranka į kandidatą, penki kandidatai, nuoseklus ir lygiagretus kasimas, kasimo ciklas |
| `main.cpp` | Programos eiga ir suvestinė |
| `tests/tests.cpp` | Bendras teisingumo testų rinkinys, nepakeistas nuo v0.2 |
| `tests/tests_v03.cpp` | Papildomi v0.3 pakeitimų testai |
| `bench.cpp` | Spartos matavimas, tas pats failas kaip v0.2 |
| `bench_parallel.cpp` | Nuoseklaus ir lygiagretaus kasimo palyginimas |
| `scripts/compare.py` | v0.2 ir v0.3 palyginimas |
| `results/v0.2/`, `results/v0.3/` | Išsaugota abiejų versijų išvestis: paleidimai, testai, matavimai, palyginimas |
| `docs/di-zurnalas.md` | DI naudojimo žurnalas: radiniai, priimti ir atmesti pasiūlymai, patikra |

## Modelis

**Vartotojas:** `name`, `public_key`, `balance`. `public_key` yra vardo (`user0`, `user1`, ...) maiša, tai tik imituojamas viešas identifikatorius: jis neįrodo rakto nuosavybės. Pradiniai balansai yra nuo 100 iki 1 000 000 mokomųjų vienetų ir lieka išsaugoti `User` objektuose. Einamieji balansai laikomi atskirai, `BalanceState` klasėje, todėl pradinę būseną visada galima atkurti.

**Transakcija:** `sender`, `receiver`, `amount`, `sequence`, `transaction_id`. Sumos yra sveikieji skaičiai (`uint64_t`). `sequence` yra bendras unikalus generatoriaus numeris. `transaction_id` yra šios eilutės UTF-8 baitų maiša:

```text
sender|receiver|amount|sequence
```

Pavyzdžiui, `a12f|b34e|100|5`. Skaičiai rašomi dešimtaine forma be tarpų ir pradinių nulių, eilutės pabaigos nėra. Pats ID į maišos įvestį neįeina. Kūrimui ir tikrinimui naudojama ta pati funkcija `Transaction::serialize()`.

**Bloko antraštė:** `prev_hash`, `tx_root`, `timestamp`, `version`, `nonce`, `difficulty`. Bloko maiša skaičiuojama iš šių šešių laukų, sujungtų `|` ženklu šia tvarka. Blokas taip pat turi tikslia tvarka išsaugotą transakcijų sąrašą, kurį su antrašte susieja `tx_root`.

**`version`:** nuo v0.2 lygi 2, nes pasikeitė `tx_root` skaičiavimo taisyklė (v0.1 buvo 1). Blokas su kita versija atmetamas. v0.3 duomenų formato nekeičia, todėl versija lieka 2.

**`timestamp`:** fiksuota reikšmė 1000 plius bloko aukštis, o ne tikras laikrodis. Dėl to tie patys įvesties duomenys kiekvieną kartą duoda tas pačias maišas.

**Proof-of-Work:** bloko maišos hex užrašas turi prasidėti `difficulty` nulių. Šiame paleidime `difficulty = 3`. Ieškant keičiamas tik `nonce`, jis didinamas po vieną nuo 0. Tikrinant maiša visada perskaičiuojama iš antraštės laukų, išsaugota reikšme nepasitikima. Sudėtingumas negali viršyti 64, nes maiša turi 64 hex simbolius.

**Genesis:** `prev_hash` yra 64 nuliai, `tx_root` yra tuščios baitų sekos maiša, `timestamp = 0`, transakcijų nėra. Genesis yra PoW išimtis: jis nekasamas ir netikrinamas.

## Maišos funkcija

Naudojama pirmoje užduotyje sukurta funkcija: [dragg000/blockchain-pirma-uzd](https://github.com/dragg000/blockchain-pirma-uzd), commit `1f4df0a`, failas `src/main.cpp`. Į `hash.cpp` perkeltos tik maišos funkcijos, algoritmas nepakeistas.

Poroje pasirinkome Jono algoritmą dėl šių priežasčių:

- išvestis yra fiksuoto ilgio, 256 bitų, užrašoma 64 mažosiomis hex raidėmis ir išlaiko pradinius nulius, todėl tinka ir ID, ir PoW sąlygai;
- ji deterministinė ir priima bet kokio ilgio baitų seką, įskaitant tuščią (to reikia genesis `tx_root`);
- 1 užduoties bandymuose tarp 4 × 100 000 atsitiktinių porų kolizijų nerasta, o pakeitus vieną simbolį vidutiniškai pasikeičia apie 50 % bitų;
- ji jau parašyta C++17 be išorinių bibliotekų, todėl perkelta nekeičiant kodo.

**v0.3 maišos versija ta pati.** Visas v0.2 `hash.cpp` kodas išlikęs nepakeistas, po juo pridėta srautinė sąsaja `HashStream`. Ji naudoja tą pačią bloko maišymo funkciją, pradinį vektorių ir užpildymą, tik priima duomenis dalimis. Testai lygina ją su `hashTextUtf8` visiems ilgiams nuo 0 iki 300 baitų ir kiekvienai dalijimo vietai. Ji reikalinga kasimui: nekintanti antraštės pradžia sumaišoma vieną kartą.

Ši maiša yra mokomoji. Sėkmingi testai neįrodo kriptografinio saugumo.

## Merkle medis

- Lapai yra transakcijų ID jų tvarka bloke.
- Vidinis mazgas yra `H(kairiojo vaiko baitai || dešiniojo vaiko baitai)`. Hex ID prieš jungiant dekoduojami į 32 baitus.
- Jei lygyje lieka neporinis mazgas, jis poruojamas su savo kopija.
- Vienas lapas pats yra šaknis.
- Tuščio bloko (genesis) šaknis apibrėžta atskirai: tuščios baitų sekos maiša.

Testuose šaknis 1, 2, 3 ir 5 transakcijoms perskaičiuojama atskiru kodu ir palyginama. Sukeitus dviejų transakcijų tvarką arba pakeitus transakcijos turinį ir perskaičiavus jos ID, šaknis pasikeičia. v0.3 papildomai lygina visus dydžius nuo 0 iki 130 lapų su lėta realizacija, parašyta kaip v0.2.

**ID formatas.** Nuo v0.3 Merkle skaičiavimas priima tik lygiai 64 mažuosius hex simbolius. Jei bloko transakcijos ID kitoks, bloko sukurti nepavyksta (`std::invalid_argument`). v0.2 priimdavo bet kokį lyginį ilgį, o `1g` tyliai perskaitydavo kaip `1`.

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

**2. Atrenkant į kandidatą** (`select_transaction`). Patvirtinti balansai nukopijuojami į laikiną būseną. Transakcijos traukiamos atsitiktinai, be pasikartojimų, tikrinamos ir iškart pritaikomos tai kopijai jų tvarka bloke. Prie pirmo etapo priežasčių čia prisideda dvi balanso priežastys:

| Priežastis | Kada |
|---|---|
| `siuntejui nepakako lesu` | Siuntėjo balansas laikinoje būsenoje mažesnis už sumą |
| `gavejo balanso perpildymas` | Gavėjo balansas nebetilptų į `uint64_t` |

Pavyzdys iš užduoties: A turi 100, T1 moka 70, T2 moka 60. Įtraukiama tik ta, kuri patikrinta pirma. Tikrieji balansai atrankos metu nesikeičia. Nuo v0.3 atmetimo priežastis išsaugoma kartu su transakcija ir būtent ji rodoma suvestinėje.

**3. Prieš priimant iškastą bloką** (`Blockchain::add_block`). Iš naujo tikrinama šia tvarka. Nuo v0.3 `check_block` ir `add_block` grąžina pirmą rastą priežastį:

| Patikra | Atmetimo priežastis |
|---|---|
| `prev_hash` lygus dabartinio paskutinio bloko maišai | `neteisingas prev_hash` |
| `version` yra 2 | `nepalaikoma versija` |
| Transakcijų yra nuo 1 iki 100 | `blokas be transakciju`, `per daug transakciju` |
| Perskaičiuota Merkle šaknis sutampa su `tx_root` | `neteisinga tx_root` |
| Bloko `difficulty` lygus grandinės reikalaujamam (kandidatas negali jo pasikeisti) | `neteisingas sudetingumas` |
| Perskaičiuota bloko maiša tenkina PoW | `netinkamas PoW` |
| Kiekviena transakcija iš eilės tinka patvirtintų balansų kopijoje, jos ID nesikartoja nei bloke, nei grandinėje | `transakcija N: ` ir transakcijos priežastis |

Tik praėjus visoms patikroms vienu veiksmu pridedamas blokas, pakeičiami balansai ir papildoma patvirtintų ID aibė. Jei bent viena patikra nepavyksta, nepritaikoma niekas.

## Penki kandidatai kitam blokui

Kiekvienam grandinės aukščiui:

1. **Sudaromi 5 kandidatai.** Visi remiasi tuo pačiu `prev_hash`, tuo pačiu `timestamp` ir tais pačiais patvirtintais balansais. Kiekvienas turi savo laikiną balansų kopiją ir iš viso fondo atsirenka iki 100 transakcijų. Atsitiktinių skaičių generatorius tarp kandidatų tęsiamas, todėl kandidatai skiriasi. Jų transakcijos gali persidengti, bet viename kandidate nesikartoja.
2. **Biudžetas.** Kiekvienas kandidatas per vieną eilę gauna **2000 bandymų**. Biudžetas skaičiuojamas bandymais, o ne laiku, kad tie patys duomenys visada duotų tą pačią grandinę. Nuo v0.3 bandymai skaičiuojami tiksliai: eilė su biudžetu N patikrina lygiai N `nonce` reikšmių.
3. **Tvarka.** Kandidatai sunumeruoti nuo 1 iki 5. Nuosekliai jie kasa paeiliui, lygiagrečiai visi vienu metu (žr. žemiau).
4. **Jei sprendimo nėra.** Jei per visą eilę niekas nerado tinkamo `nonce`, biudžetas padvigubinamas (4000, 8000, ...) ir kiekvienas kandidatas tęsia nuo savo dar netikrintų `nonce`. Sudėtingumas nemažinamas.
5. **Bendra riba.** Daugiausia 10 eilių, tai yra iki 2 046 000 bandymų vienam kandidatui. Jei ir tada sprendimo nėra, programa sustoja su aiškiu pranešimu. Jau priimti blokai lieka galioti, o nebaigtas blokas nieko nepakeičia.
6. **Laimėtojo taisyklė.** Laimi mažiausio numerio kandidatas, radęs tinkamą `nonce` per tos eilės biudžetą.
7. **Pakartotinė patikra.** Laimėtojo blokas perduodamas `add_block`, kuris viską patikrina iš naujo (3 etapas).
8. **Nelaimėję kandidatai.** Jie dirbo tik su savo kopijomis, todėl išmetami be jokio poveikio. Iš fondo pašalinamos tik laimėtojo įtrauktos ir atmestos transakcijos. Kitam blokui kandidatai sudaromi iš naujo.

Kiekvienam blokui spausdinama visų penkių kandidatų suvestinė, o pabaigoje bendri skaičiai.

### Lygiagretus kasimas (v0.3)

Numatytuoju būdu kiekvienos eilės metu kiekvienas kandidatas kasa savo gijoje (`mine_candidates_parallel`).

- **Nepriklausoma paieška.** Kiekviena gija dirba tik su savo kandidato bloku. Kandidatų antraštės skiriasi (`tx_root`), todėl jie ieško skirtingų sprendimų.
- **Ta pati laimėtojo taisyklė.** Laimi mažiausio numerio kandidatas, radęs `nonce` per eilės biudžetą, o ne tas, kurio gija greitesnė. Todėl laimėtojas ir jo `nonce` visada sutampa su nuosekliu kasimu, o tie patys duomenys duoda tą pačią grandinę kiekviename paleidime ir kiekviename kompiuteryje.
- **Kitų sustabdymas.** Kai kandidatas randa `nonce`, didesnio numerio kandidatai sustabdomi, nes šioje eilėje laimėti nebegali. Mažesnio numerio kandidatai savo biudžetą baigia, nes radę jie turėtų pirmenybę. Gijos tikrina sustabdymo sąlygą kas 256 bandymus.
- **Saugus vieno laimėtojo priėmimas.** Vienintelis bendras kintamasis yra `std::atomic` su mažiausiu radusio kandidato numeriu. Laimėtojas nuskaitomas tik sulaukus visų gijų, o jo bloką `add_block` tikrina ir priima pagrindinėje gijoje. Gijos sulaukiamos ir klaidos atveju (RAII).

Išvestyje stulpelis „rezultatas" rodo `laimejo`, `nerado`, `sustabdytas` arba `rado, bet pirmenybe mazesniam numeriui`. Sustabdytų kandidatų bandymų skaičius ir laikas skirtinguose paleidimuose gali skirtis, grandinė nesiskiria.

### Kodėl tai dar nėra P2P konsensusas

- Viskas vyksta viename procese ir viename kompiuteryje. Yra tik viena grandinės ir balansų kopija, nėra nepriklausomų mazgų, kurie kiekvienas tikrintų bloką savarankiškai.
- Nėra tinklo: transakcijos ir blokai niekur nesiunčiami, nėra vėlavimų, prarastų pranešimų ar skirtingų fondų skirtinguose mazguose.
- Kandidatai nekonkuruoja tikru laiku. Laimėtoją lemia fiksuota numerių tvarka ir biudžetas, o ne tai, kas iš tikrųjų pirmas rado sprendimą. Tai galioja ir lygiagrečiam kasimui: gijos viename kompiuteryje nėra decentralizuotas tinklas.
- Nėra šakų: du blokai tame pačiame aukštyje niekada nepriimami, todėl nereikia nei ilgiausios grandinės taisyklės, nei persitvarkymų.
- Visi kandidatai yra tos pačios programos dalys ir vienas kitu pasitiki. Nėra skaitmeninių parašų, atlygio už bloką ar sudėtingumo perskaičiavimo.

Penki kandidatai parodo tik tai, kad keli skirtingi blokai gali varžytis dėl to paties aukščio ir kad priimamas vienas, o kiti neturi poveikio.

## Netinkamos transakcijos

Struktūriškai netinkamos transakcijos atmetamos prieš fondą su priežastimi iš lentelės aukščiau.

Laikinai netinkamoms (siuntėjui tuo metu nepakanka lėšų) pasirinkome **atmetimą**, o ne atidėjimą. Kandidate atmesta transakcija iš fondo pašalinama tada, kai tas kandidatas laimi, ir išvedama galutinėje suvestinėje su atmetimo metu išsaugota priežastimi. Atmetimas priklauso nuo atsitiktinės traukimo tvarkos, todėl vėliau gavęs lėšų siuntėjas šios transakcijos nebeįvykdys. Nelaimėjusių kandidatų atmestos transakcijos lieka fonde.

Kiekvienas ratas pašalina iš fondo bent vieną transakciją arba sustoja, todėl ciklas visada baigiasi.

## Atkuriamumas

| Parametras | Reikšmė |
|---|---|
| Vartotojų seed | 1 |
| Transakcijų seed | 2 |
| Atrankos seed | 3 |
| Generatorius | `std::mt19937_64`, reikšmės gaunamos su `rng() %` |
| Vartotojai | 1000, balansai nuo 100 iki 1 000 000 |
| Transakcijos | 10 000 (arba programos argumentas) |
| Sudėtingumas | 3 |
| Kandidatų vienam blokui | 5 |
| Pradinis biudžetas | 2000 bandymų kandidatui per eilę |
| Biudžeto didinimas | Dvigubinamas po kiekvienos nesėkmingos eilės, daugiausia 10 eilių |
| Kasimo būdas | Lygiagretus, 5 gijos (arba `--nuosekliai`). Grandinė nuo būdo nepriklauso |
| `timestamp` | 1000 + bloko aukštis |
| Tikrinta su | Apple clang 17, arm64 (Apple M4, 10 loginių branduolių), macOS, `-std=c++17 -O2 -pthread` |

Sąmoningai nenaudojami `std::uniform_int_distribution` ir `std::shuffle`, nes jų rezultatai skirtingose standartinėse bibliotekose gali skirtis. Du paleidimai duoda tuos pačius blokus, maišas ir laimėtojus. Skiriasi tik išmatuotas laikas, o lygiagrečiai kasant dar ir sustabdytų kandidatų bandymų skaičius.

Matavimo sąlygos aprašytos skyriuje [v0.2 ir v0.3 palyginimas](#v02-ir-v03-palyginimas).

## Rezultatai

Visa išvestis išsaugota aplankuose [results/v0.2](results/v0.2) ir [results/v0.3](results/v0.3).

Vieno bloko išvestis (antrasis blokas: pirmas kandidatas išnaudojo biudžetą, laimėjo antras, kiti sustabdyti):

```text
Blokas #2
  transakcijos:  100
  prev_hash:     0009d2556abc1aaa7270a3dd2e5c27100f1aa77bcd5874b4e5c8a36427621092
  tx_root:       b59fca11963efa994f32eafa45c8ef252ffcd8d58427f47873dabb85ea09862d
  hash:          0000c744ea4fb15f2beb7b01ca7e981e70ef60cc6af4db88b47090afced0f21d
  difficulty:    3
  nonce:         132
  bandymai:      133
  kasimo laikas: 0.000042 s
  kandidatai:    nr  transakcijos  eiles  bandymai  laikas, s  rezultatas
                  1           100      1      2000   0.000630  nerado
                  2           100      1       133   0.000042  laimejo
                  3           100      1       256   0.000144  sustabdytas
                  4           100      1       256   0.000122  sustabdytas
                  5           100      1       256   0.000081  sustabdytas
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
  bendras laikas:         0.442831 s

Kandidatai
  kasimo budas:            lygiagretus, kiekvienas kandidatas savo gijoje
  kandidatu vienam blokui: 5
  pradinis biudzetas:      2000 bandymu per eile
  visu kandidatu bandymai: 599281
  visu kandidatu laikas:   0.203114 s (giju darbo laiko suma)
  kasimo trukme:           0.065915 s (pagal laikrodi)
  biudzetas dvigubintas:   7 k.
  laimeti blokai:          nr 1: 44, nr 2: 30, nr 3: 14, nr 4: 3, nr 5: 9

Atmesta 5dc4e0c85d3d449609dfcbd08e7eec2f5e6e9dc589b02aa8bad1ae81d2520614 (suma 92632): siuntejui nepakako lesu
```

Paskutiniame bloke yra 99 transakcijos, nes viena iš 10 000 buvo atmesta. Blokų maišos, `nonce`, laimėtojai ir atmesta transakcija yra tie patys kaip v0.2.

## Testai

`make test` paleidžia du rinkinius, visos patikros praeina:

- `tests/tests.cpp`: **187 patikros**. Tai bendras v0.2 ir v0.3 rinkinys, failas nuo v0.2 nepakeistas.
- `tests/tests_v03.cpp`: **186 patikros**. Tikrina tik v0.3 pakeitimus.

Lentelė atitinka bendrą užduoties testų rinkinį (`tests/tests.cpp`).

| Scenarijus | Kaip tikrinama | Laukiama savybė | v0.2 | v0.3 |
|---|---|---|---|---|
| Galiojantis mokėjimas | A 100, B 50, C 0, mokėjimai A→B 30 ir B→C 20 | Lieka A 70, B 60, C 20, suma 150, mokėjimas įtrauktas vieną kartą | Praeina | Praeina |
| Galutinis blokas su <100 įrašų | 250 transakcijų | Blokai po 100, 100 ir 50 | Praeina | Praeina |
| Nežinomas gavėjas | Gavėjo nėra būsenoje | Atmetama, priežastis `nezinomas dalyvis` | Praeina | Praeina |
| Suma 0 arba neigiama | Suma 0, suma `-1` kaip `uint64_t`, suma virš visų lėšų | Atmetama, priežastis `netinkama suma` | Praeina | Praeina |
| Pakeistas ID | Pakeista suma arba pats ID | Atmetama, priežastis `neteisingas ID` | Praeina | Praeina |
| Du mokėjimai viršija balansą | A turi 100, T1 moka 70, T2 moka 60, 50 skirtingų seed | Kiekvieną kartą įtraukiama tik viena, balansai nesikeičia | Praeina | Praeina |
| Dubliuotas ID | Ta pati transakcija į fondą du kartus, tas pats ID du kartus bloke | Antroji atmetama, priežastis `pasikartojantis ID`, blokas nepriimamas | Praeina | Praeina |
| Patvirtinto mokėjimo pakartojimas | Iškasta transakcija pateikiama dar kartą į fondą ir naujame bloke | Atmetama, balansai nesikeičia | Praeina | Praeina |
| Pakeistas bloko turinys | Ta pati antraštė, kita transakcija | PoW nebetinka, blokas atmetamas | Praeina | Praeina |
| Neteisingas `prev_hash` | Iškastas blokas su kita nuoroda | Blokas atmetamas | Praeina | Praeina |
| Netinkamas PoW | Neiškastas blokas, blokas su mažesniu `difficulty`, sena `version` | Blokas atmetamas, grandinė ir balansai nesikeičia | Praeina | Praeina |
| Merkle 1 / 2 / 3 / 5 | Šaknis perskaičiuojama atskiru kodu | Sutampa, keičiasi sukeitus tvarką ar pakeitus turinį | Praeina | Praeina |
| Nelaimėję kandidatai | Vienas ratas su 5 kandidatais | Balansai lygūs pradiniams plius tik laimėtojo transakcijos, kitų transakcijos lieka fonde | Praeina | Praeina |

Bendras rinkinys papildomai tikrina: serializaciją (`a12f|b34e|100|5`, o suma 1 + `sequence` 23 skiriasi nuo sumos 12 + `sequence` 3), tas pats seed duoda tas pačias maišas ir tą patį paskutinį bloką, gavėjo balanso perpildymą, ciklas sustoja, kai nieko negalima įtraukti, biudžetas baigiasi ir kasimas tęsiamas nuo išsaugoto `nonce`, kasimas sustoja po leistinų eilių, 2000 transakcijų paleidime nėra dublikatų ir neigiamų balansų.

Papildomi v0.3 testai (`tests/tests_v03.cpp`):

| Testas | Kas tikrinama |
|---|---|
| Tikslus biudžetas | Jei tinkamas `nonce` yra N, tai N bandymų jo dar neranda, o kitas ėjimas randa pirmu bandymu. Su v0.2 kodu šis testas nepraeina |
| Srautinė maiša | `HashStream` sutampa su `hashTextUtf8`: ilgiai 0–300, po vieną baitą, kiekviena dalijimo vieta, tuščia įvestis, žinoma `hello world` reikšmė |
| Greitas kasimas | 24 blokams rastas `nonce` sutampa su lėta paieška per `hasValidProofOfWork`, taip pat kasant ėjimais po 7 bandymus |
| Merkle modulis | 0–130 lapų sutampa su lėta realizacija, sutampa v0.2 išspausdinta šaknis, 12 netinkamų ID atmetami |
| Bloko atmetimo priežastys | Kiekviena priežastis, kurią įmanoma sukelti per viešą sąsają. `check_block` nieko nekeičia |
| Atrankos priežastys | Priežastis išsaugoma kartu su atmesta transakcija, įskaitant perpildymą |
| Ribos | Sudėtingumo 65 grandinė nesukuriama. Generavimas veikia, kai balansai mažesni už 10 |
| Lygiagretus kasimas | 36 kandidatų rinkiniai po 5 kartus: tas pats laimėtojas, `nonce` ir bandymai kaip nuosekliai. Kiti sustabdomi. Lėtesnis mažesnio numerio kandidatas laimi 40 kartų iš 40 |
| Ta pati grandinė kaip v0.2 | 7 duomenų rinkiniai (250–10 000 transakcijų, sudėtingumas 2–4, skirtingi seed), nuosekliai ir lygiagrečiai, paskutinio bloko maiša lygi v0.2 išspausdintai |

## v0.2 ir v0.3 palyginimas

Atskaita yra užfiksuota žyma `v0.2`. Ji **nėra versija „be DI"**: DI įrankis naudotas visuose etapuose (žr. [DI naudojimas](#di-naudojimas)). Visą palyginimą atkuria viena komanda, rezultatas išsaugotas faile [results/v0.3/comparison.txt](results/v0.3/comparison.txt):

```bash
python3 scripts/compare.py
```

### Tobulinimo kryptys

| Kryptis | Kas padaryta |
|---|---|
| Kasimo ciklo efektyvumas | Tikslus bandymų skaičius, antraštės pradžia sumaišoma vieną kartą, PoW sąlyga tikrinama baitais, kandidatai kasa lygiagrečiai |
| Merkle medžio skaičiavimo efektyvumas | Medis skaičiuojamas baitais, hex dekoduojamas vieną kartą be `std::stringstream` |
| Validavimo klaidos | Bloko ir atrankos atmetimo priežastys, griežtas ID formatas, sudėtingumo, argumento ir generavimo ribos |
| Atminties naudojimas | Nebekopijuojamas fondas, ID aibės ir transakcijų sąrašai, balansai laikomi maišos lentelėje |
| Kodo struktūra ir testų aprėptis | Atskiras `merkle` modulis, pavadintos konstantos, 186 naujos patikros |

Kiekvienas priimtas pasiūlymas yra atskiras commit'as. Jų sąrašas, patikra ir atmesti pasiūlymai yra skyriuje [DI naudojimas](#di-naudojimas).

### Tie patys testai ir ta pati grandinė

- `tests/tests.cpp` v0.3 versijoje nepakeistas (tikrinama SHA-256). Scenarijus sukompiliuoja žymos `v0.2` testų failą su abiejų versijų kodu: abiem atvejais **187 patikros, 0 nepavykusių**, išvestis sutampa.
- Ta pati pradinė būsena, tie patys duomenys ir sudėtingumas. 250, 2000 ir 10 000 transakcijų paleidimuose palyginta kiekvieno bloko aukštis, transakcijų skaičius, `prev_hash`, `tx_root`, `hash`, `difficulty`, `nonce`, suvestinės skaičiai, lėšų sumos, laimėti blokai pagal kandidatą ir atmestos transakcijos. **v0.2, v0.3 nuosekliai ir v0.3 lygiagrečiai sutampa visose eilutėse.**
- Viso rinkinio paskutinio bloko maiša abiejose versijose: `000df5652989778a44f24b2ff303158779079dbdacf501e27764198929b0d64d`.

### Sparta

Matavimo sąlygos:

- tas pats kompiuteris (Apple M4), tas pats kompiliatorius (Apple clang 17) ir `-std=c++17 -O2`; v0.3 papildomai turi `-pthread`;
- ta pati maišos versija;
- ta pati programa `bench.cpp` (failas nepakeistas, tikrinama SHA-256), sukompiliuota su kiekviena versija;
- 7 kartojimai, versijos paleidžiamos pakaitomis (v0.2, v0.3, v0.2, v0.3, ...), kad abi patektų į tas pačias sąlygas;
- laikas matuojamas tik aplink skaičiavimą, konsolės išvedimas į jį neįeina;
- kasimas matuojamas su fiksuota antrašte ir fiksuotu bandymų skaičiumi (200 000, sudėtingumas 16, kurio pasiekti neįmanoma), o ne laiku iki atsitiktinai rasto sprendimo.

| Matavimas | v0.2 vidurkis (nuokrypis) | v0.2 mediana | v0.3 vidurkis (nuokrypis) | v0.3 mediana | Santykis |
|---|---|---|---|---|---|
| Kasimas, 200 000 bandymų | 0,5532 s (0,0368) | 0,5618 s | 0,0648 s (0,0049) | 0,0626 s | **9,0 karto** |
| Merkle šaknis × 500 (100 transakcijų) | 0,4942 s (0,0458) | 0,4854 s | 0,0337 s (0,0024) | 0,0342 s | **14,2 karto** |
| Tikrinimas prieš fondą, 10 000 transakcijų | 0,0407 s (0,0039) | 0,0397 s | 0,0352 s (0,0026) | 0,0337 s | **1,2 karto** |
| 5 kandidatų sudarymas × 20 | 0,3490 s (0,0343) | 0,3390 s | 0,0503 s (0,0031) | 0,0507 s | **6,7 karto** |
| Visas paleidimas be išvedimo | 3,2514 s (0,2355) | 3,2399 s | 0,4530 s (0,0204) | 0,4602 s | **7,0 karto** |

Santykis skaičiuojamas iš medianų.

**Matavimo metu kompiuteris nebuvo laisvas:** veikė žaidimas ir pokalbių programa. Todėl absoliutūs laikai didesni, o nuokrypiai platesni nei ramiame kompiuteryje (plg. `results/v0.2/bench.txt`, kur ta pati v0.2 išmatuota ramesniu metu: kasimas 0,32 s, visas paleidimas 1,85 s). Versijos matuotos pakaitomis, todėl santykiams tai turi mažiau įtakos nei absoliutiems skaičiams. Skaičius galima atkurti komanda `python3 scripts/compare.py`.

### Nuoseklus ir lygiagretus kasimas vienodomis sąlygomis

`./run_bench_parallel` abu būdus matuoja pakaitomis toje pačioje programoje, su tais pačiais duomenimis, antraštėmis ir biudžetu (7 kartojimai, [results/v0.3/bench_parallel.txt](results/v0.3/bench_parallel.txt)):

| Matavimas | Nuosekliai: vidurkis (nuokrypis) | Lygiagrečiai: vidurkis (nuokrypis) | Santykis |
|---|---|---|---|
| Fiksuotas darbas: 5 kandidatai po 200 000 bandymų | 0,358 s (0,004) | 0,077 s (0,002) | **4,6 karto** |
| Visas paleidimas, sudėtingumas 3 | 0,457 s (0,033) | 0,410 s (0,015) | **1,1 karto** |
| Tik kandidatų kasimas tame paleidime | 0,109 s (0,007) | 0,064 s (0,003) | **1,7 karto** |
| Visas paleidimas, sudėtingumas 4 | 2,674 s (0,202) | 1,232 s (0,060) | **2,2 karto** |
| Tik kandidatų kasimas tame paleidime | 2,286 s (0,170) | 0,803 s (0,037) | **2,9 karto** |

Abiem būdais paskutinio bloko maiša sutampa.

### Atmintis

Didžiausias `./blockchain` atminties naudojimas (`/usr/bin/time -l`, 3 paleidimų mediana): **v0.2 18,8 MB, v0.3 15,9 MB**.

### Išvada

**Kas pagerėjo**

- Visas 10 000 transakcijų paleidimas be išvedimo trunka apie 7 kartus trumpiau, o grandinė lieka ta pati.
- Vienas kasimo bandymas apie 9 kartus greitesnis, Merkle šaknis apie 14 kartų, penkių kandidatų sudarymas apie 7 kartus.
- Lygiagretus kasimas tą patį fiksuotą darbą atlieka 4,6 karto greičiau, išlaikydamas tą patį laimėtoją.
- Bandymų skaičius tikslus. Blokai ir atrankoje atmestos transakcijos turi tikrą atmetimo priežastį. Netinkamo formato ID, per didelis sudėtingumas ir netinkamas argumentas nebepraeina tyliai.
- Didžiausias atminties naudojimas sumažėjo maždaug 3 MB.
- Testų padaugėjo nuo 187 iki 373 patikrų.

**Kas nepasikeitė**

- Duomenų modelis, serializacija, maišos funkcija, taisyklės ir pati grandinė: tie patys duomenys duoda tuos pačius blokus.
- Tikrinimas prieš fondą pagreitėjo tik apie 1,2 karto. Ten beveik visas laikas tenka transakcijos ID maišos perskaičiavimui, kurio atsisakyti negalima.
- Užduoties sudėtingumui 3 lygiagretus kasimas visą paleidimą pagreitina tik apie 1,1 karto. Po kitų pakeitimų kasimas užima maždaug ketvirtį laiko, viena eilė trunka mažiau nei milisekundę, o gijos kiekvienai eilei paleidžiamos iš naujo. Nauda aiškesnė, kai sudėtingumas 4 (2,2 karto).

**Kas pablogėjo**

- Lygiagretus kasimas iš viso atlieka apie 60 % daugiau maišos skaičiavimų (apie 600 000 vietoj 372 639), nes sustabdyti kandidatai jau būna atlikę dalį eilės. Laiko sutaupoma, procesoriaus darbo sunaudojama daugiau.
- Lygiagrečiai kasant sustabdytų kandidatų bandymų skaičius ir laikas suvestinėje skirtinguose paleidimuose skiriasi. Grandinė nesiskiria.
- Kodo padaugėjo: programos šaltinis išaugo nuo 1194 iki 1660 eilučių, atsirado gijos ir antras kelias maišai skaičiuoti, kuriuos reikia suprasti ir prižiūrėti.
- Blokas su netinkamo formato transakcijos ID dabar nebesukuriamas (išimtis), o ne atmetamas `add_block`. Tai griežčiau, bet elgsena kitokia nei v0.2.

**Kokie apribojimai liko**

- Maiša yra mokomoji, parašų nėra, viskas vyksta viename procese. Tai ne P2P tinklas.
- Kiekvienas kandidatas vis dar kopijuoja visą balansų būseną, o kiekvienas tikrinimo etapas perskaičiuoja transakcijos ID.
- Gijos kuriamos kiekvienai eilei iš naujo. Nuolatinis gijų rinkinys sumažintų šias išlaidas, bet nebandytas.
- Priežasties `neteisinga tx_root` per viešą sąsają sukelti neįmanoma, nes `Block` konstruktorius šaknį visada perskaičiuoja pats. Patikra palikta kaip apsauga.
- Gijų klaidų tikrintuvo (`-fsanitize=thread`) panaudoti nepavyko: šiame kompiuteryje jis nulūžta net tuščioje programoje. Lygiagretaus kodo teisingumas remiasi testais ir tuo, kad bendras yra tik vienas atominis kintamasis.
- Matuota apkrautame kompiuteryje ir tik vienoje architektūroje.

**Pasiūlymai, kurie naudos nedavė arba buvo atmesti**

- Traukimas iš fondo per Fenwick medį: visas paleidimas 0,5105 s → 0,5012 s (1,02 karto, mažiau už triukšmą). Atmesta, pakeitimas išsaugotas faile [docs/atmestas-fenwick.patch](docs/atmestas-fenwick.patch).
- Atrankos pakeitimas į `swap` ir `pop_back`: 1,13 karto greičiau, bet grandinė pasikeičia visuose 7 atskaitos rinkiniuose. Atmesta.

## DI naudojimas

**Įrankis:** Claude Code (Anthropic). **Modeliai:** Claude Opus 5.5 ir `claude-fable-5-2`. **Laikotarpis:** 2026-11-04 – 2026-11-09. DI API raktų repozitorijoje nėra. Kiekvienas commit'as, prie kurio prisidėjo įrankis, turi eilutę `Co-Authored-By: Claude`, todėl tai matyti ir istorijoje (`git log`).

Išsamus žurnalas su užklausomis, radiniais ir patikra: [docs/di-zurnalas.md](docs/di-zurnalas.md).

| Etapas | Kam naudota |
|---|---|
| Planavimas | Žingsnių sąrašas `STEPS.md` ir jo padalijimas dviem žmonėms |
| v0.1 | Sąvokų ir žingsnių aiškinimas, Jono rašyto kodo peržiūra, kompiliavimo klaidų taisymas, kasimo ciklo sutvarkymas, konsolės išvestis, testai, `Makefile`, README |
| v0.2 | Tikrinimo prieš fondą užbaigimas ir pagreitinimas, porininko commit'ų kompiliavimo klaidų taisymas, kasimas paeiliui (`mine_candidates`), laimėtojo parinkimas, kandidatų suvestinė, Merkle ir kandidatų testai, spartos matavimas, README |
| v0.3 | Visas etapas: užfiksuotos v0.2 kodo peržiūra, pasiūlymai, jų įgyvendinimas, testai, matavimai, palyginimo scenarijus, žurnalas ir šis README. Įrankis čia dirbo savarankiškai pagal vieną autoriaus užduotį |

### v0.3 pasiūlymai

Priėmimo kriterijai nustatyti prieš keičiant kodą: nepakeisti v0.2 testai praeina, grandinė lieka ta pati, spartos teiginys pagrįstas matavimu, pakeitimas turi naują testą.

| Nr. | Pasiūlymas | Sprendimas | Commit | Patikra |
|---|---|---|---|---|
| R1 | Tikslus bandymų skaičius, vienas kasimo ciklas | Priimta | `e8ddad5` | Naujas testas nepraeina su v0.2 kodu ir praeina su v0.3 |
| R2 | Antraštės pradžią maišyti vieną kartą (`HashStream`) | Priimta | `299e1f4`, `b341ffc` | Srautinė maiša palyginta su originalia visiems ilgiams 0–300, `nonce` palygintas su lėta paieška. 200 000 bandymų: 0,469 s → 0,057 s |
| R3 | Kandidatus kasti gijomis | Priimta su ta pačia laimėtojo taisykle | `8e28aab` | Laimėtojas ir `nonce` sutampa su nuosekliu kasimu, 3 tyčia sugadinti variantai testų nepraeina |
| R4, R5, R11 | Merkle medį skaičiuoti baitais, ID dekoduoti griežtai, atskiras modulis | Priimta | `f236b44` | 0–130 lapų palyginta su lėta realizacija. 500 šaknų: apie 0,5 s → 0,034 s |
| R6, R7, R8 | Atmetimo priežastys ir ribų tikrinimas | Priimta | `fadab00` | Testas kiekvienai priežasčiai. v0.2 problemos pirmiausia atkurtos su v0.2 kodu |
| R9, R10 | Nekopijuoti fondo, ID aibių ir transakcijų sąrašų | Priimta | `8df53a6` | Ta pati grandinė. Kandidatų sudarymas apie 5,6 karto greičiau |
| R12 | Maišos aibė fondo valymui | Priimta | `9f102ba` | Visas paleidimas 1,11 karto greičiau |
| R13 | Balansai maišos lentelėje | Priimta | `90d853f` | Kandidatų sudarymas 1,34 karto, visas paleidimas 1,19 karto greičiau |
| A1 | Fenwick medis traukimui iš fondo | Atmesta | – | Išmatuota: visas paleidimas 1,02 karto, mažiau už triukšmą |
| A2 | `swap` ir `pop_back` atrankoje | Atmesta | – | Greičiau, bet grandinė skiriasi nuo v0.2 |
| A3 | Biudžetas laiku, o ne bandymais | Atmesta | – | Grandinė priklausytų nuo kompiuterio greičio |
| A4 | Laimi greičiausia gija | Atmesta | – | Grandinė nebūtų atkuriama, testai tokio varianto nepraeina |
| A5 | Greitinti ar keisti pačią maišą | Atmesta | – | Tai būtų kita maišos versija |
| A6 | Atidėti vietoj atmetimo | Atmesta | – | Elgsenos pakeitimas, grandinė skirtųsi nuo v0.2 |

Išsamiai aprašyta patikra dviem pasiūlymams (R2 ir R3) yra žurnale.

### Šaltiniai ir kitų autorių kodas

- Maišos funkcija: Jono Salio 1 užduotis, [dragg000/blockchain-pirma-uzd](https://github.com/dragg000/blockchain-pirma-uzd), commit `1f4df0a`.
- Užduoties formuluotė: „2-oji praktinė užduotis: supaprastintos blokų grandinės kūrimas" (VU, 2026).
- Kitų autorių kodo ar išorinių bibliotekų nenaudota, tik C++ standartinė biblioteka.
- Kodas, kurį parašė DI įrankis, pažymėtas commit'ų istorijoje, kaip aprašyta aukščiau.

## Indėliai

| Autorius | Darbai |
|---|---|
| Jonas Salys | Vartotojai ir transakcijos, serializacija, generavimas iš seed, balansų būsena, atranka į kandidatą, atmetimo taisyklė, tikrinimas prieš fondą, penkių kandidatų sudarymas, kasimas su biudžetu, neiškasto bloko paruošimas |
| zukasdovis | Bloko antraštė ir blokas, grandinės klasė ir genesis, `tx_root`, bloko maiša, PoW, pirmasis kasimo ciklo variantas, Merkle medis, tikrinimas atrenkant, bloko tikrinimas ir priėmimas |

Plane 27–29 žingsniai (biudžetas, laimėtojas, kandidatų suvestinė) buvo skirti porininkui. Jam susirgus, juos atliko Jonas su Claude Code. v0.3 pakeitimus pagal Jono užduotį atliko Claude Code, kaip aprašyta skyriuje [DI naudojimas](#di-naudojimas).
