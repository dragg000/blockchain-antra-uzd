# Supaprastinta blokų grandinė

Blokų grandinių technologijos, 2 užduotis (Vilniaus universitetas, 2026). Autoriai: Jonas Salys ir zukasdovis.

Tai v0.1 versija: viename procese veikianti grandinė su sąskaitų modeliu ir Proof-of-Work. Darbo žingsniai aprašyti faile [STEPS.md](STEPS.md).

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

- `./blockchain` paleidžia visą rinkinį: 1000 vartotojų ir 10 000 transakcijų.
- `./blockchain 250` paleidžia mažą pavyzdį su 250 transakcijų.
- `make test` paleidžia testus.

## Modelis

**Vartotojas:** `name`, `public_key`, `balance`. `public_key` yra vardo (`user0`, `user1`, ...) maiša, tai imituojamas viešas raktas. Pradiniai balansai yra nuo 100 iki 1 000 000 mokomųjų vienetų ir lieka išsaugoti `User` objektuose. Einamieji balansai laikomi atskirai, `BalanceState` klasėje.

**Transakcija:** `sender`, `receiver`, `amount`, `sequence`, `transaction_id`. Sumos yra sveikieji skaičiai (`uint64_t`). `sequence` yra bendras unikalus generatoriaus numeris. `transaction_id` yra šios eilutės maiša:

```text
sender|receiver|amount|sequence
```

Pavyzdžiui, `a12f|b34e|100|5`.

**Bloko antraštė:** `prev_hash`, `tx_root`, `timestamp`, `version`, `nonce`, `difficulty`. Bloko maiša skaičiuojama iš šių laukų, sujungtų `|` ženklu ta pačia tvarka. Blokas taip pat turi sutvarkytą transakcijų sąrašą.

**`tx_root`:** v0.1 versijoje tai transakcijų ID, sujungtų bloko tvarka, maiša.

**`timestamp`:** fiksuota reikšmė 1000 plius bloko aukštis, o ne tikras laikrodis. Dėl to tie patys įvesties duomenys kiekvieną kartą duoda tas pačias maišas.

**Proof-of-Work:** bloko maiša turi prasidėti `difficulty` nuliais. Šiame paleidime `difficulty = 3`. Ieškant keičiamas tik `nonce`.

**Genesis:** `prev_hash` yra 64 nuliai, `tx_root` yra tuščios eilutės maiša, transakcijų nėra. Genesis yra PoW išimtis, jis nekasamas.

## Maišos funkcija

Naudojama pirmoje užduotyje sukurta funkcija: [dragg000/blockchain-pirma-uzd](https://github.com/dragg000/blockchain-pirma-uzd), commit `1f4df0a`, failas `src/main.cpp`. Ji grąžina 64 šešioliktainių simbolių eilutę. Į `hash.cpp` perkeltos tik maišos funkcijos, algoritmas nepakeistas.

## Vienas ciklas

1. Iš fondo atsitiktinai ir be pasikartojimų traukiamos transakcijos. Kiekviena tikrinama laikinoje balansų kopijoje, kol bloke yra 100 tinkamų transakcijų arba nebėra ką traukti.
2. Iš tinkamų transakcijų sudaromas blokas ir kasamas.
3. Iškastas blokas dar kartą patikrinamas naujoje patvirtintų balansų kopijoje, tada `add_block` patikrina nuorodą į ankstesnį bloką ir PoW.
4. Tik tada kartu atnaujinami grandinė, balansai ir fondas. Jei patikra nepavyksta, programa sustoja su klaida ir nieko nepakeičia.
5. Ciklas kartojamas, kol fondas tuščias. Kiekvienas ciklas pašalina bent vieną transakciją, todėl programa visada baigiasi.

## Netinkamos transakcijos

Pasirinkome **atmetimą**, o ne atidėjimą. Transakcija, kuriai siuntėjas tuo metu neturi pakankamai lėšų arba kurios dalyvis nežinomas, nebetikrinama iš naujo: priėmus bloką ji pašalinama iš fondo ir išvedama galutinėje suvestinėje su priežastimi. Atmetimas priklauso nuo atsitiktinės traukimo tvarkos, todėl vėliau gavęs lėšų siuntėjas šios transakcijos nebeįvykdys.

## Atkuriamumas

| Parametras | Reikšmė |
|---|---|
| Vartotojų seed | 1 |
| Transakcijų seed | 2 |
| Atrankos seed | 3 |
| Generatorius | `std::mt19937_64`, reikšmės gaunamos su `rng() %` |
| Sudėtingumas | 3 |
| Tikrinta su | Apple clang 17, arm64, macOS |

Sąmoningai nenaudojami `std::uniform_int_distribution` ir `std::shuffle`, nes jų rezultatai skirtingose standartinėse bibliotekose gali skirtis. Du paleidimai duoda tuos pačius blokus ir maišas, skiriasi tik išmatuotas laikas.

## Rezultatai

Vieno bloko išvestis:

```text
Blokas #1
  transakcijos:  100
  prev_hash:     cacafac38a4a9f9c65a439820c164a991f83421db51dc7b7c0a4649faf0674c5
  tx_root:       d976a89b71c6d9aff64e5f83f278d10bf38762f238c10db549828b524e5d3826
  hash:          000ef67d733a27228384b1dac8d7952c214d5fcab1e9d8c371e74ed6daac3abf
  difficulty:    3
  nonce:         5345
  bandymai:      5346
  kasimo laikas: 0.011116 s
```

Galutinė suvestinė:

```text
Suvestine
  vartotojai:             1000
  transakcijos:           10000
  priimti blokai:         100
  itrauktos transakcijos: 9999
  atmestos transakcijos:  1
  likusios transakcijos:  0
  pradine lesu suma:      498753171
  galutine lesu suma:     498753171
  bendras laikas:         0.829038 s
Atmesta cee7fa26e4e5e75864775263056440159176d97a3c45dc4fcadfc385286f65f7 (suma 76336): siuntejui nepakako lesu
```

Paskutiniame bloke yra 99 transakcijos, nes viena iš 10 000 buvo atmesta.

## Testai

`make test` tikrina (54 patikros, visos praeina):

| Atvejis | Laukiamas rezultatas |
|---|---|
| Serializacija | Gaunama tiksliai `a12f\|b34e\|100\|5`, o suma 1 + sequence 23 skiriasi nuo suma 12 + sequence 3 |
| Tas pats seed | Tos pačios transakcijų maišos ir tas pats paskutinis blokas |
| Fiksuota antraštė | Tas pats bloko hash, o kitas `nonce` jį pakeičia |
| Mažas atvejis (rankomis) | A 100, B 50, C 0, mokėjimai A→B 30 ir B→C 20: lieka A 70, B 60, C 20, suma 150 |
| Per didelis mokėjimas | Atmetamas ir nieko nekeičia, o visas balansas išleidžiamas ir lieka 0 |
| Du mokėjimai viename bloke | A turi 100, T1 moka 70, T2 moka 60: 50 seed bandymų, kiekvienąkart įtraukiama tik viena |
| Paskutinis nepilnas blokas | 250 transakcijų sudaro blokus po 100, 100 ir 50 |
| Nieko negalima įtraukti | Ciklas baigiasi, blokų nėra, lėšų suma nesikeičia |
| 2000 transakcijų paleidimas | Nėra dublikatų ir neigiamų balansų, visos transakcijos suskaičiuotos, blokai susieti, PoW tinka |
| Netinkami blokai | Pakeistas turinys, neteisingas `prev_hash` ar neiškastas blokas atmetami, grandinė nesikeičia |

## Žinomi v0.1 apribojimai

- `add_block` tikrina PoW pagal paties bloko `difficulty`, o ne pagal grandinės reikalaujamą. Tai bus pataisyta v0.2 versijoje.
- `add_block` neperskaičiuoja transakcijų ir balansų pats. Tai daro kasimo ciklas prieš jį iškviesdamas.
- Prieš patenkant į fondą transakcijos dar netikrinamos. Tai numatyta v0.2 versijoje.
