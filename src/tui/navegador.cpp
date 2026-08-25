// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO NAVEGADOR — src/tui/navegador.cpp
// ══════════════════════════════════════════════════════════════════════════
// A implementação. Uma regra governa tudo: TODA mudança de vista passa por
// refaz_vista(), e é ella que apara o eleito. Donde não ha caminho por onde o
// eleito saia da lista, e o invariante é estructural e não vigilancia.
//
// DOMÍNIO ......... a bibliotheca, a secção, a trilha e o termo.
// CONTRA-DOMÍNIO .. a vista e o eleito.
// INVARIANTE ...... o eleito está dentro da vista, ou a vista é vazia e elle é
//                   zero. Uma funcção só o garante.
// Q.E.D. .......... havendo uma porta única para a vista, acrescentar secção é
//                   acrescentar um ramo, e não rever a aparadura em N logares.
// ══════════════════════════════════════════════════════════════════════════
#include "tui/navegador.hpp"

#include <algorithm>
#include <cctype>
#include <utility>

namespace mysong::tui {

namespace {

// contem_sem_caixa — a comparação do filtro. Sem caixa, que o operador que busca
// «bach» não ha de escrever «Bach» para achar o que já vê na tela. Sem dobra de
// acento, de proposito: dobrar acento em UTF-8 pede taboa que esta Casa não tem,
// e prometter menos é melhor que prometter e falhar no «á» contra o «a».
bool contem_sem_caixa(const std::string& palheiro, const std::string& agulha) {
  if (agulha.empty()) return true;
  const auto baixa = [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  };
  std::string p, a;
  p.reserve(palheiro.size());
  a.reserve(agulha.size());
  for (const unsigned char c : palheiro) p += baixa(c);
  for (const unsigned char c : agulha) a += baixa(c);
  return p.find(a) != std::string::npos;
}

}  // namespace

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
