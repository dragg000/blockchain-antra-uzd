# Steps, in order

This is the to-do list for assignment 2, split between two people. It says **what** to build and how to know it is done. It has no solution code and leaves the design choices to you. Section numbers (§) refer to the assignment PDF.

| Version | Due | Contains |
|---|---|---|
| `v0.1` | 2026-10-29 | Working single-process chain, your hash, PoW, console output, README |
| `v0.2` | 2026-11-05 | Merkle tree, extended validation, 5 candidates, correctness tests |
| `v0.3` | 2026-11-05 | AI-assisted improvement, verified comparison against frozen `v0.2` |

## Who does what

Fill in your names:

| | Name | Area |
|---|---|---|
| **A** | Jonas Salys | Transactions, balances, pool, validation |
| **B** | zukasdovis | Hash, blocks, Merkle tree, mining |

Each step is marked **[A]**, **[B]** or **[Both]**. "(after N)" means that step has to wait for step N. Steps with no waits can run at the same time.

## Working rules

- Commit each step when it works, under your own name. Both people's work must be visible in the commit history and the README (§6), and commit history is graded ("nuosekli ir laiku užfiksuota projekto raida"). Don't backdate anything.
- Work on the `v0.x-dev` branch, then merge to `main` and tag. Don't name a branch `v0.1`, because a branch and tag with the same name make git commands ambiguous.
- Run `git pull` before you start and `git push` when you finish a step. Keep to your own files where you can, so you don't overwrite each other.
- Read what the other person pushed. You both have to be able to explain the whole flow, not only your half (§6).
- Write down every AI use as it happens, including for `v0.1` and `v0.2` (§5). This step list and its split were written with Claude, so log that too.
- An earlier AI-written attempt is in `../blockchain`. If you copy from it, disclose that as well.

---

## Setup

- [x] **1. [Both] Get access and get on the branch.** The repo owner invites the other person as a collaborator (repo Settings → Collaborators). Each person clones the repo, checks out `v0.1-dev`, and sets `git config user.name` and `user.email` to their own details.
- [x] **2. [A] Create the project skeleton.** Add `src/`, `tests/`, a Makefile and a `.gitignore`. Build and run a program that prints one line.
  - If linking fails with `tapi error: malformed file`, run `export SDKROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX26.5.sdk` first.
- [x] **3. [Both, then B] Choose the hash and bring it in.** Together, decide whose assignment 1 hash to use, and write down why in the README, as §1 requires for pairs. Then B adds it and notes the repo and commit in the README.
  - One option is `dragg000/blockchain-pirma-uzd`, branch `v0.1`, commit `05247fa`. That version's `main()` ignores its arguments and always hashes "hello", so call the hash function directly instead of running the program.
  - **Done when:** the same input always gives the same output, the output length is fixed, and leading zeros are kept.

---

## v0.1: basic chain (§2, §3)

**Data**

- [x] **4. [Both] Define the data types.** Do this one together, because all your later code depends on it.
  - User: name, unique `public_key`, balance.
  - Transaction: `sender`, `receiver`, `amount`, `sequence`, `transaction_id`.
  - Block header: `prev_hash`, `timestamp`, `version`, `tx_root`, `nonce`, `difficulty`.
  - Block: header plus an ordered transaction list.
  - Amounts must be integers, never floating point.
- [x] **5. [A] Write the serialization (after 3).** Use one function for both creating and checking: `sender|receiver|amount|sequence`, in UTF-8, with plain decimal numbers and no trailing newline.
  - The transaction ID is the hash of that text and never includes the ID itself.
  - **Done when:** `a12f|b34e|100|5` comes out exactly, and amount 1 + sequence 23 serializes differently from amount 12 + sequence 3.
- [x] **6. [A] Generate users from a seed.** About 1,000 users with starting balances from 100 to 1,000,000. Save the starting balances before applying any transaction.
  - **Done when:** the same seed gives identical users.
- [x] **7. [A] Generate transactions from a seed (after 5).** About 10,000, each with a positive amount and a unique `sequence`. Sender and receiver must exist and differ.
  - **Done when:** the same seed gives the same transaction IDs.

**State and blocks**

- [x] **8. [A] Build the balance state.** Map each user to a balance, and apply a transaction to any copy of that state. Add a function that sums all funds.
  - **Done when:** balances never go negative and the total never changes.
- [x] **9. [B] Define the genesis block.** Fix its `prev_hash` (e.g. all zeros) and its empty `tx_root` (e.g. the hash of empty bytes). Decide whether genesis is exempt from PoW, and document it.
- [x] **10. [B] Compute `tx_root` for v0.1.** It is the hash of the transaction IDs joined in block order. The Merkle tree replaces this in v0.2.
- [x] **11. [B] Hash the header.** Hash the six header fields through a defined serialization, not an in-memory object.
  - **Done when:** a fixed header always gives the same hash.
- [x] **12. [B] Add Proof-of-Work (after 11).** With `difficulty = 3`, the hash must start with `000`. Increase `nonce` while the other fields stay fixed. Count attempts and time.
  - The PoW check must recompute the hash, not trust the stored one.
  - **Done when:** a block with a changed field fails the check.

**The main loop**

- [x] **13. [A] Build the transaction pool and candidate selection (after 7, 8).** Pick up to 100 at random without repeats, and check them in order against a temporary copy of the balances. Real balances must not change here.
- [x] **14. [Both, then A] Decide what happens to invalid transactions.** Structurally bad ones are rejected with a reason. Together, choose either defer or reject for insufficient balance. A then writes down the rule and implements it.
- [x] **15. [B] Write the mining loop (after 12, 13, 14).** Select → compute `tx_root` → mine → re-check the mined block → only then append it, apply balances and remove its transactions from the pool. Repeat until the pool is empty or no transaction can be included.
  - **Done when:** it can't loop forever, even if nothing can be included.
- [x] **16. [B] Add the console output (after 15).**
  - For each block: height, transaction count, `prev_hash`, `tx_root`, block hash, difficulty, nonce, attempts, mining time.
  - At the end: users, transactions, accepted blocks, included/rejected/remaining transactions, starting and final funds, total time.
  - You must be able to explain every rejected or remaining transaction.

**Finish v0.1**

- [x] **17. [A] Run the v0.1 checks (end of §3, after 16).**
  - A small case with a few users that you verify by hand
  - Same input gives the same hashes
  - A user can't overspend, including across several payments in one block
  - The last, partly filled block works
  - No duplicate transactions and no negative balances
- [x] **18. [Both] Write the README.**
  - A: how to run it, the model, the seed.
  - B: the hash version and why you chose it, one block's output, the final summary.
- [x] **19. [B] Release v0.1.** Once you've both read the README, merge to `main`, tag `v0.1`, create a GitHub release and push the tag.

**Can you both explain it?** Why does `tx_root` belong in the header? Why does changing an old transaction break every later block? Why does selection use a temporary copy of the balances?

---

## v0.2: Merkle tree and validation (§4)

- [x] **20. [A] Create the branch.** Make `v0.2-dev` from `main`.
- [x] **21. [B] Build the Merkle tree.** Leaves are transaction IDs in block order. Decode hex to bytes before joining, then compute H(left bytes ‖ right bytes). Pair an odd node with its own copy. A single leaf is the root. Define the empty case separately.
  - Use it as `tx_root`, and think about whether the block `version` should change.
  - **Done when:** 1, 2, 3 and 5 transactions all work, and changing a transaction or swapping the order changes the root.
- [x] **22. [A] Validate before entering the pool (§4, stage 1).** Check field format, that participants exist, that the amount is a positive integer, and the recomputed ID. Refuse an ID already in the pool or on the chain.
- [x] **23. [A] Validate during candidate selection (stage 2).** Check transactions in order against a temporary copy of confirmed balances, including integer overflow.
  - **Done when:** the PDF's example works: A has 100, T1 pays 70 and T2 pays 60, so only the first one checked gets in.
- [x] **24. [B] Validate before accepting a block (stage 3, after 21, 23).** Re-check `prev_hash`, format version, `tx_root`, transaction count, and PoW against the difficulty *the chain requires*, so a candidate can't lower it. Replay the transactions in order.
- [x] **25. [A] Make acceptance all-or-nothing (after 24).** Update chain, balances and pool as one step. A rejected block changes nothing.
- [x] **26. [A] Add five candidates (after 23).** All five build on the same `prev_hash` and the same confirmed balances, and each has its own temporary copy. They may overlap with each other, but no candidate repeats a transaction.
- [x] **27. [B] Add the mining budget (after 26).** Give each candidate a limited number of attempts or limited time, and document the order. If none succeeds, raise the budget and continue from untried nonces. Never lower the difficulty. Add an overall limit so it always stops.
- [x] **28. [B] Pick a winner (after 25, 27).** Choose one by a rule you document and re-check it. Only the winner's transactions count, and losers have no effect. Rebuild candidates for the next block.
- [x] **29. [B] Print a candidate summary (after 28).** Show attempts and time per candidate.
- [x] **30. [Both] Write the correctness tests (table in §5, after 28).** You'll rerun the same tests on v0.3, so make them repeatable. Each person tests the other's code, which is also how you learn it.
  - **B tests A's code:**
    - Valid payment; final block with under 100 transactions
    - Unknown receiver; amount of 0 or negative; altered ID
    - Two payments from one sender that together exceed the balance
    - Duplicate ID; replay of a confirmed payment
  - **A tests B's code:**
    - Altered block contents, wrong `prev_hash`, invalid PoW
    - Merkle cases 1/2/3/5; losing candidates don't mix state
- [x] **31. [Both] Update the README.**
  - A: the test table.
  - B: the candidate rules.
  - Together: why this simulation still isn't P2P consensus.
- [ ] **32. Optional extras.** Skip these until the main work is done.
  - **[B]** Parallel mining (+0.5): threads or processes, safe single winner, stopping the others, compared with sequential mining under the same conditions.
  - **[A]** UTXO model (+0.5): replaces the account model, so it's a big change this late.
- [x] **33. [A] Freeze v0.2.** Save your test results and a full run's output, merge to `main`, tag `v0.2` and push. Never move this tag afterwards, because v0.3 is compared against it.

**Can you both explain it?** Why can a transaction pass at the pool but fail in a block? How does Merkle odd-node duplication work? Why is five candidates on one machine not a decentralized network?

---

## v0.3: improvement with AI tools (§5)

- [ ] **34. [B] Create the branch.** Make `v0.3-dev` from `main`.
- [ ] **35. [Both] Choose at least two directions to improve.** Each person takes one. For example, A could take validation errors or test coverage, and B mining or Merkle efficiency. Other options are code structure and memory use.
  - **Planned: multithreaded candidate mining.** Replace step 27's taking turns with a budget by mining the five candidates on separate threads at the same time. The first thread to find a valid nonce wins and the others stop. Compare it with v0.2's sequential mining under the same conditions (step 39). The winner can then change between runs, so decide how the "same input, same chain" test handles that.
- [ ] **36. [Both] Run the AI review and log it.** Each person does this for their own direction. Give the tool the problem, the requirements and the relevant code. Save the tool or model name, the date and the significant prompts.
- [ ] **37. [Both] Evaluate the suggestions.** Treat each one as a hypothesis, because an AI suggestion doesn't prove anything is correct. Put each accepted change in its own commit, and have the other person read it. Describe how you verified at least one, and explain any you rejected.
- [ ] **38. [A] Run the same tests on v0.2 and v0.3 (after 37).** Use the same starting state, data and difficulty. Compare with the frozen tag, and keep the v0.2 baseline available next to v0.3.
- [ ] **39. [B] If you claim a speedup, measure it properly (after 37).**
  - At least 5 runs, with mean and spread
  - Timing kept separate from console output
  - Same machine, compiler flags and hash version, with fixed candidate headers
  - A fixed number of attempts, not time to a lucky solution
  - Nothing else heavy running on the machine
- [ ] **40. [Both] Write the conclusion (after 38, 39).** Say what improved, what didn't change or got worse, and what limits remain. If a suggestion gave no benefit, show the test instead of inventing an improvement.

---

## Submit (§6)

- [ ] **41. [Both] Finish the README.**
  - **[A] Running:** language, dependencies, build, small and full runs, test command
  - **[A] Model:** fields, serialization, hash version, genesis, balance and invalid-transaction rules
  - **[A] Reproducibility:** seed, generator, starting balances, difficulty, budgets, measurement conditions
  - **[B] Evidence:** a block example, final summary, test table, v0.2/v0.3 comparison
  - **[B] AI use:** tools and models, prompt log, accepted and rejected suggestions and how you checked them, all stages included
  - **[Both] Sources and contributions:** other people's code and sources you used, plus who did what
- [ ] **42. [B] Release v0.3 (after 41).** Merge to `main`, tag `v0.3` and push.
- [ ] **43. [A] Check the repo before submitting (after 42).**
  - The repo is public
  - Tags `v0.1`, `v0.2` and `v0.3` are all on GitHub
  - A fresh clone of each tag builds and passes its tests
  - No secrets or API keys are committed
- [ ] **44. [A] Submit the repo link** the way the course requires. Check whether you each need to submit it.
- [ ] **45. [Both] Practise the demo.** Each person explains the *other* person's half out loud, without help. You must both be able to explain the whole flow and your accepted AI changes.
