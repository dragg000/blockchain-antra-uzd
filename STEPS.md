# Steps, in order

This is the to-do list for assignment 2. It says **what** to build and how to know it is done. It has no solution code and leaves the design choices to you. Section numbers (§) refer to the assignment PDF.

| Version | Due | Contains |
|---|---|---|
| `v0.1` | 2026-10-29 | Working single-process chain, your hash, PoW, console output, README |
| `v0.2` | 2026-11-05 | Merkle tree, extended validation, 5 candidates, correctness tests |
| `v0.3` | 2026-11-05 | AI-assisted improvement, verified comparison against frozen `v0.2` |

**Working rules**

- Commit each step when it works. Commit history is graded ("nuosekli ir laiku užfiksuota projekto raida"), so don't backdate anything.
- Work on the `v0.x-dev` branch, then merge to `main` and tag. Don't name a branch `v0.1`, because a branch and tag with the same name make git commands ambiguous.
- Write down every AI use as it happens, including for `v0.1` and `v0.2` (§5). This step list was itself written with Claude, so log that too.
- An earlier AI-written attempt is in `../blockchain`. If you copy from it, disclose that as well.

---

## Setup

- [ ] **1. Get on the branch.** Clone this repo and check out `v0.1-dev`.
- [ ] **2. Create the project skeleton.** Add `src/`, `tests/`, a Makefile and a `.gitignore`. Build and run a program that prints one line.
  - If linking fails with `tapi error: malformed file`, run `export SDKROOT=/Library/Developer/CommandLineTools/SDKs/MacOSX26.5.sdk` first.
- [ ] **3. Bring in your hash from assignment 1.** Use `dragg000/blockchain-pirma-uzd`, branch `v0.1`, commit `05247fa`. Note the repo and commit in the README.
  - That version's `main()` ignores its arguments and always hashes "hello", so call the hash function directly instead of running the program.
  - **Done when:** the same input always gives the same output, the output length is fixed, and leading zeros are kept.

---

## v0.1: basic chain (§2, §3)

**Data**

- [ ] **4. Define the data types.**
  - User: name, unique `public_key`, balance.
  - Transaction: `sender`, `receiver`, `amount`, `sequence`, `transaction_id`.
  - Block header: `prev_hash`, `timestamp`, `version`, `tx_root`, `nonce`, `difficulty`.
  - Block: header plus an ordered transaction list.
  - Amounts must be integers, never floating point.
- [ ] **5. Write the serialization.** Use one function for both creating and checking: `sender|receiver|amount|sequence`, in UTF-8, with plain decimal numbers and no trailing newline.
  - The transaction ID is the hash of that text and never includes the ID itself.
  - **Done when:** `a12f|b34e|100|5` comes out exactly, and amount 1 + sequence 23 serializes differently from amount 12 + sequence 3.
- [ ] **6. Generate users from a seed.** About 1,000 users with starting balances from 100 to 1,000,000. Save the starting balances before applying any transaction.
  - **Done when:** the same seed gives identical users.
- [ ] **7. Generate transactions from a seed.** About 10,000, each with a positive amount and a unique `sequence`. Sender and receiver must exist and differ.
  - **Done when:** the same seed gives the same transaction IDs.

**State and blocks**

- [ ] **8. Build the balance state.** Map each user to a balance, and apply a transaction to any copy of that state. Add a function that sums all funds.
  - **Done when:** balances never go negative and the total never changes.
- [ ] **9. Define the genesis block.** Fix its `prev_hash` (e.g. all zeros) and its empty `tx_root` (e.g. the hash of empty bytes). Decide whether genesis is exempt from PoW, and document it.
- [ ] **10. Compute `tx_root` for v0.1.** It is the hash of the transaction IDs joined in block order. The Merkle tree replaces this in v0.2.
- [ ] **11. Hash the header.** Hash the six header fields through a defined serialization, not an in-memory object.
  - **Done when:** a fixed header always gives the same hash.
- [ ] **12. Add Proof-of-Work.** With `difficulty = 3`, the hash must start with `000`. Increase `nonce` while the other fields stay fixed. Count attempts and time.
  - The PoW check must recompute the hash, not trust the stored one.
  - **Done when:** a block with a changed field fails the check.

**The main loop**

- [ ] **13. Build the transaction pool and candidate selection.** Pick up to 100 at random without repeats, and check them in order against a temporary copy of the balances. Real balances must not change here.
- [ ] **14. Decide what happens to invalid transactions.** Structurally bad ones are rejected with a reason. For insufficient balance, choose either defer or reject, and write down the rule.
- [ ] **15. Write the mining loop.** Select → compute `tx_root` → mine → re-check the mined block → only then append it, apply balances and remove its transactions from the pool. Repeat until the pool is empty or no transaction can be included.
  - **Done when:** it can't loop forever, even if nothing can be included.
- [ ] **16. Add the console output.**
  - For each block: height, transaction count, `prev_hash`, `tx_root`, block hash, difficulty, nonce, attempts, mining time.
  - At the end: users, transactions, accepted blocks, included/rejected/remaining transactions, starting and final funds, total time.
  - You must be able to explain every rejected or remaining transaction.

**Finish v0.1**

- [ ] **17. Run the v0.1 checks (end of §3).**
  - A small case with a few users that you verify by hand
  - Same input gives the same hashes
  - A user can't overspend, including across several payments in one block
  - The last, partly filled block works
  - No duplicate transactions and no negative balances
- [ ] **18. Write the README.** Include how to run it, the model, the hash version, the seed, one block's output and the final summary.
- [ ] **19. Release v0.1.** Merge to `main`, tag `v0.1`, create a GitHub release and push the tag.

**Can you explain it?** Why does `tx_root` belong in the header? Why does changing an old transaction break every later block? Why does selection use a temporary copy of the balances?

---

## v0.2: Merkle tree and validation (§4)

- [ ] **20. Create the branch.** Make `v0.2-dev` from `main`.
- [ ] **21. Build the Merkle tree.** Leaves are transaction IDs in block order. Decode hex to bytes before joining, then compute H(left bytes ‖ right bytes). Pair an odd node with its own copy. A single leaf is the root. Define the empty case separately.
  - Use it as `tx_root`, and think about whether the block `version` should change.
  - **Done when:** 1, 2, 3 and 5 transactions all work, and changing a transaction or swapping the order changes the root.
- [ ] **22. Validate before entering the pool (§4, stage 1).** Check field format, that participants exist, that the amount is a positive integer, and the recomputed ID. Refuse an ID already in the pool or on the chain.
- [ ] **23. Validate during candidate selection (stage 2).** Check transactions in order against a temporary copy of confirmed balances, including integer overflow.
  - **Done when:** the PDF's example works: A has 100, T1 pays 70 and T2 pays 60, so only the first one checked gets in.
- [ ] **24. Validate before accepting a block (stage 3).** Re-check `prev_hash`, format version, `tx_root`, transaction count, and PoW against the difficulty *the chain requires*, so a candidate can't lower it. Replay the transactions in order.
- [ ] **25. Make acceptance all-or-nothing.** Update chain, balances and pool as one step. A rejected block changes nothing.
- [ ] **26. Add five candidates.** All five build on the same `prev_hash` and the same confirmed balances, and each has its own temporary copy. They may overlap with each other, but no candidate repeats a transaction.
- [ ] **27. Add the mining budget.** Give each candidate a limited number of attempts or limited time, and document the order. If none succeeds, raise the budget and continue from untried nonces. Never lower the difficulty. Add an overall limit so it always stops.
- [ ] **28. Pick a winner.** Choose one by a rule you document and re-check it. Only the winner's transactions count, and losers have no effect. Rebuild candidates for the next block.
- [ ] **29. Print a candidate summary.** Show attempts and time per candidate.
- [ ] **30. Write the correctness tests (table in §5).** You'll rerun the same tests on v0.3, so make them repeatable.
  - Valid payment; final block with under 100 transactions
  - Unknown receiver; amount of 0 or negative; altered ID
  - Two payments from one sender that together exceed the balance
  - Duplicate ID; replay of a confirmed payment
  - Altered block contents, wrong `prev_hash`, invalid PoW
  - Merkle cases 1/2/3/5; losing candidates don't mix state
- [ ] **31. Update the README.** Cover the candidate rules, the test table, and why this simulation still isn't P2P consensus.
- [ ] **32. Optional extras.** Skip these until the main work is done.
  - Parallel mining (+0.5): threads or processes, safe single winner, stopping the others, compared with sequential mining under the same conditions.
  - UTXO model (+0.5): replaces the account model, so it's a big change this late.
- [ ] **33. Freeze v0.2.** Save your test results and a full run's output, merge to `main`, tag `v0.2` and push. Never move this tag afterwards, because v0.3 is compared against it.

**Can you explain it?** Why can a transaction pass at the pool but fail in a block? How does Merkle odd-node duplication work? Why is five candidates on one machine not a decentralized network?

---

## v0.3: improvement with AI tools (§5)

- [ ] **34. Create the branch.** Make `v0.3-dev` from `main`.
- [ ] **35. Choose at least two directions to improve.** For example validation errors, test coverage, code structure, memory use, or mining/Merkle efficiency.
- [ ] **36. Run the AI review and log it.** Give the tool the problem, the requirements and the relevant code. Save the tool or model name, the date and the significant prompts.
- [ ] **37. Evaluate the suggestions.** Treat each one as a hypothesis, because an AI suggestion doesn't prove anything is correct. Put each accepted change in its own commit. Describe how you verified at least one, and explain any you rejected.
- [ ] **38. Run the same tests on v0.2 and v0.3.** Use the same starting state, data and difficulty. Compare with the frozen tag, and keep the v0.2 baseline available next to v0.3.
- [ ] **39. If you claim a speedup, measure it properly.**
  - At least 5 runs, with mean and spread
  - Timing kept separate from console output
  - Same machine, compiler flags and hash version, with fixed candidate headers
  - A fixed number of attempts, not time to a lucky solution
  - Nothing else heavy running on the machine
- [ ] **40. Write the conclusion.** Say what improved, what didn't change or got worse, and what limits remain. If a suggestion gave no benefit, show the test instead of inventing an improvement.

---

## Submit (§6)

- [ ] **41. Finish the README.**
  - **Running:** language, dependencies, build, small and full runs, test command
  - **Model:** fields, serialization, hash version, genesis, balance and invalid-transaction rules
  - **Reproducibility:** seed, generator, starting balances, difficulty, budgets, measurement conditions
  - **Evidence:** a block example, final summary, test table, v0.2/v0.3 comparison
  - **AI use:** tools and models, prompt log, accepted and rejected suggestions and how you checked them, all stages included
  - **Sources and contributions:** other people's code and sources you used, plus your own contribution
- [ ] **42. Release v0.3.** Merge to `main`, tag `v0.3` and push.
- [ ] **43. Check the repo before submitting.**
  - The repo is public
  - Tags `v0.1`, `v0.2` and `v0.3` are all on GitHub
  - A fresh clone of each tag builds and passes its tests
  - No secrets or API keys are committed
- [ ] **44. Submit the repo link** the way the course requires.
- [ ] **45. Practise the demo.** You must be able to explain the whole flow and every accepted AI change without help.
