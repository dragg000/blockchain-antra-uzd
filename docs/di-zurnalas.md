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

| Etapas | Užklausų pobūdis | Ką darė įrankis |
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

Sprendimai, patikra ir matavimai surašyti toliau, kai pakeitimai atlikti.
