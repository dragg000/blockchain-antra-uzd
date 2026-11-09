#pragma once

#include "hash.hpp"
#include "user.hpp"
#include <vector>

// Transakcijos ID (lygiai 64 mazieji hex simboliai) -> 32 baitai.
// Jei ID kitokio ilgio arba jame yra kitoks simbolis, meta std::invalid_argument.
HashStream::Digest decode_id(const Hash &id);

// Merkle saknis transakciju ID sarasui, ju tvarka bloke.
//   - lapai yra ID, dekoduoti i baitus;
//   - vidinis mazgas yra H(kairiojo vaiko baitai || desiniojo vaiko baitai);
//   - neporinis mazgas poruojamas su savo kopija;
//   - vienas lapas pats yra saknis;
//   - tuscio saraso saknis yra tuscios baitu sekos maisa.
// Meta std::invalid_argument, jei kuris nors ID netinkamo formato.
Hash merkle_root(const std::vector<Hash> &ids);
