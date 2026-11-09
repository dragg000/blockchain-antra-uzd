#!/usr/bin/env python3
"""v0.2 ir v0.3 palyginimas.

Paleidimas is repozitorijos saknies:

    python3 scripts/compare.py [kartojimu skaicius, numatyta 7]

Ka daro:
  1. Istraukia uzfiksuota zyma v0.2 i laikina aplanka ir sukompiliuoja ja su jos paties Makefile.
  2. Sukompiliuoja dabartini koda (v0.3).
  3. Teisingumas: v0.2 zymos testu faila tests/tests.cpp sukompiliuoja su abiem versijomis ir paleidzia.
  4. Grandine: paleidzia abi programas su tais paciais duomenimis ir palygina kiekvieno bloko
     auksti, transakciju skaiciu, prev_hash, tx_root, hash, difficulty ir nonce bei galutine suvestine.
  5. Sparta: ta pati bench.cpp programa paleidziama pakaitomis (v0.2, v0.3, v0.2, v0.3, ...),
     kad abi versijos butu matuojamos tomis paciomis salygomis. Pateikiamas vidurkis ir nuokrypis.
  6. Atmintis: didziausias ./blockchain atminties naudojimas pagal /usr/bin/time -l.

Reikia: git, make, C++17 kompiliatoriaus, python3.
"""

import hashlib
import os
import re
import shutil
import statistics
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BASELINE_TAG = "v0.2"
MEASUREMENTS = [
    "Kasimas, 200 000 bandymu",
    "Merkle saknis x 500",
    "Tikrinimas pries fonda, 10 000 tx",
    "5 kandidatu sudarymas x 20",
    "Visas paleidimas be isvedimo",
]


def run(command, cwd, check=True):
    return subprocess.run(command, cwd=cwd, check=check, capture_output=True, text=True)


def sha256(path):
    with open(path, "rb") as handle:
        return hashlib.sha256(handle.read()).hexdigest()


def build(directory, targets):
    run(["make"] + targets, directory)


def chain_facts(output):
    """Is programos isvesties palieka tik tai, kas nepriklauso nuo laiko ir kasimo budo."""
    kept = []
    for line in output.splitlines():
        stripped = line.strip()
        if stripped.startswith(("Blokas #", "transakcijos:", "prev_hash:", "tx_root:", "hash:", "difficulty:", "nonce:")):
            kept.append(stripped)
        elif stripped.startswith(("vartotojai:", "priimti blokai:", "itrauktos transakcijos:", "atmestos pries fonda:",
                                  "atmestos atrenkant:", "likusios transakcijos:", "pradine lesu suma:",
                                  "galutine lesu suma:", "laimeti blokai:", "Atmesta ", "Neileista ")):
            kept.append(stripped)
    return kept


def bench_once(directory):
    output = run(["./run_bench", "1"], directory).stdout
    times = [float(line.split()[1]) for line in output.splitlines() if "vidurkis" in line]
    tip = [line.split()[-1] for line in output.splitlines() if "paskutinio bloko hash" in line][0]
    return times, tip


def peak_memory(directory, arguments):
    result = subprocess.run(["/usr/bin/time", "-l", "./blockchain"] + arguments, cwd=directory,
                            capture_output=True, text=True)
    match = re.search(r"(\d+)\s+maximum resident set size", result.stderr)
    return int(match.group(1)) if match else None


def main():
    rounds = int(sys.argv[1]) if len(sys.argv) > 1 else 7
    work = tempfile.mkdtemp(prefix="compare-v02-")
    try:
        baseline = os.path.join(work, "v0.2")
        os.makedirs(baseline)
        archive = subprocess.run(["git", "archive", BASELINE_TAG], cwd=ROOT, check=True, capture_output=True).stdout
        subprocess.run(["tar", "-x", "-C", baseline], input=archive, check=True)
        commit = run(["git", "rev-list", "-n", "1", BASELINE_TAG], ROOT).stdout.strip()
        head = run(["git", "rev-parse", "HEAD"], ROOT).stdout.strip()
        dirty = run(["git", "status", "--porcelain"], ROOT).stdout.strip() != ""
        compiler = run(["c++", "--version"], ROOT).stdout.splitlines()[0]

        print("v0.2 ir v0.3 palyginimas")
        print("========================")
        print("v0.2 atskaita:   zyma %s, commit %s" % (BASELINE_TAG, commit[:7]))
        print("v0.3:            commit %s%s" % (head[:7], " + neissaugoti pakeitimai" if dirty else ""))
        print("Kompiliatorius:  %s" % compiler)
        print("Parinktys:       v0.2 %s" % re.search(r"CXXFLAGS = (.*)", open(os.path.join(baseline, "Makefile")).read()).group(1))
        print("                 v0.3 %s" % re.search(r"CXXFLAGS = (.*)", open(os.path.join(ROOT, "Makefile")).read()).group(1))
        old_hash = open(os.path.join(baseline, "hash.cpp")).read()
        new_hash = open(os.path.join(ROOT, "hash.cpp")).read().replace('#include "hash.hpp"\n\n', "", 1)
        if new_hash.startswith(old_hash):
            added = new_hash[len(old_hash):].count("\n")
            print("Maisos funkcija: ta pati. Visas v0.2 hash.cpp kodas islikes nepakeistas, po juo prideta %d eiluciu" % added)
        else:
            print("Maisos funkcija: DEMESIO, v0.2 hash.cpp kodas pakeistas")
        print("Duomenys:        seed 1/2/3, 1000 vartotoju, sudetingumas 3")
        print()

        build(baseline, ["blockchain", "run_tests", "run_bench"])
        build(ROOT, ["blockchain", "run_tests", "run_tests_v03", "run_bench"])

        # ---- 1. Tie patys teisingumo testai
        print("1. Tie patys teisingumo testai")
        print("------------------------------")
        tests_baseline = os.path.join(baseline, "tests", "tests.cpp")
        tests_current = os.path.join(ROOT, "tests", "tests.cpp")
        same_file = sha256(tests_baseline) == sha256(tests_current)
        print("tests/tests.cpp v0.3 versijoje %s (SHA-256 %s)" %
              ("nepakeistas" if same_file else "PAKEISTAS", sha256(tests_baseline)[:16]))

        # v0.2 zymos testu failas kompiliuojamas su v0.3 kodu, kad tikrai butu paleisti tie patys testai.
        sources = re.search(r"SOURCES = (.*)", open(os.path.join(ROOT, "Makefile")).read()).group(1).split()
        flags = re.search(r"CXXFLAGS = (.*)", open(os.path.join(ROOT, "Makefile")).read()).group(1).split()
        frozen_on_new = os.path.join(work, "v02_tests_on_v03")
        run(["c++"] + flags + [tests_baseline] + sources + ["-o", frozen_on_new], ROOT)

        old_tests = run(["./run_tests"], baseline, check=False)
        new_tests = run([frozen_on_new], ROOT, check=False)
        extra_tests = run(["./run_tests_v03"], ROOT, check=False)
        print("v0.2 kodas:  %s" % old_tests.stdout.strip().splitlines()[-1])
        print("v0.3 kodas:  %s" % new_tests.stdout.strip().splitlines()[-1])
        print("Testu isvestis %s" % ("sutampa" if old_tests.stdout == new_tests.stdout else "SKIRIASI"))
        print("Papildomi v0.3 testai (tests/tests_v03.cpp): %s" % extra_tests.stdout.strip().splitlines()[-1])
        print()

        # ---- 2. Ta pati grandine
        print("2. Ta pati grandine is tu paciu duomenu")
        print("---------------------------------------")
        all_same = True
        for count in ["250", "2000", "10000"]:
            old = chain_facts(run(["./blockchain", count], baseline).stdout)
            sequential = chain_facts(run(["./blockchain", count, "--nuosekliai"], ROOT).stdout)
            parallel = chain_facts(run(["./blockchain", count, "--lygiagreciai"], ROOT).stdout)
            blocks = sum(1 for line in old if line.startswith("Blokas #"))
            last_hash = [line for line in old if line.startswith("hash:")][-1].split()[-1]
            same = old == sequential == parallel
            all_same = all_same and same
            print("%6s transakciju: %3d bloku, %4d palygintos eilutes, v0.2 = v0.3 nuosekliai = v0.3 lygiagreciai: %s" %
                  (count, blocks, len(old), "TAIP" if same else "NE"))
            print("                     paskutinio bloko hash %s" % last_hash)
        print("Lyginama: kiekvieno bloko aukstis, transakciju skaicius, prev_hash, tx_root, hash, difficulty, nonce,")
        print("          suvestines skaiciai, lesu sumos, laimeti blokai pagal kandidata ir atmestos transakcijos.")
        print()

        # ---- 3. Sparta
        print("3. Sparta (bench.cpp, tas pats failas abiejose versijose)")
        print("---------------------------------------------------------")
        same_bench = sha256(os.path.join(baseline, "bench.cpp")) == sha256(os.path.join(ROOT, "bench.cpp"))
        print("bench.cpp %s. Kartojimu: %d, versijos matuojamos pakaitomis." %
              ("nepakeistas" if same_bench else "PAKEISTAS", rounds))
        old_times, new_times = [], []
        old_tip = new_tip = ""
        for _ in range(rounds):
            times, old_tip = bench_once(baseline)
            old_times.append(times)
            times, new_tip = bench_once(ROOT)
            new_times.append(times)

        print()
        print("%-36s %-22s %-22s %s" % ("Matavimas", "v0.2 vidurkis (nuokr.)", "v0.3 vidurkis (nuokr.)", "Santykis"))
        for index, name in enumerate(MEASUREMENTS):
            old = [row[index] for row in old_times]
            new = [row[index] for row in new_times]
            old_dev = statistics.stdev(old) if len(old) > 1 else 0.0
            new_dev = statistics.stdev(new) if len(new) > 1 else 0.0
            print("%-36s %8.4f s (%.4f)    %8.4f s (%.4f)    %5.2f karto" %
                  (name, statistics.mean(old), old_dev, statistics.mean(new), new_dev,
                   statistics.mean(old) / statistics.mean(new)))
        print()
        print("Viso paleidimo paskutinio bloko hash: v0.2 %s" % old_tip)
        print("                                      v0.3 %s (%s)" % (new_tip, "sutampa" if old_tip == new_tip else "NESUTAMPA"))
        print("Kasimo matavime bandymu skaicius fiksuotas (200 000, sudetingumas 16), o ne laikas iki sprendimo.")
        print("v0.3 visas paleidimas kasa lygiagreciai (numatytasis budas).")
        print()

        # ---- 4. Atmintis
        print("4. Didziausias atminties naudojimas (./blockchain, 10 000 transakciju)")
        print("----------------------------------------------------------------------")
        old_memory = sorted(peak_memory(baseline, []) for _ in range(3))[1]
        new_memory = sorted(peak_memory(ROOT, []) for _ in range(3))[1]
        if old_memory and new_memory:
            print("v0.2: %.1f MB    v0.3: %.1f MB    (3 paleidimu mediana, /usr/bin/time -l)" %
                  (old_memory / 1e6, new_memory / 1e6))
        else:
            print("Nepavyko ismatuoti (reikia /usr/bin/time -l).")

        return 0 if (all_same and same_file and old_tests.returncode == 0 and new_tests.returncode == 0
                     and extra_tests.returncode == 0) else 1
    finally:
        shutil.rmtree(work, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
