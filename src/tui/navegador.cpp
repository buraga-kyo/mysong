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

// A PORTA UNICA da vista. Toda mudança de secção, de trilha ou de termo passa por
// aqui, e é aqui que o eleito se apara. Não ha segunda aparadura em logar algum:
// havendo duas, uma delas ficaria por corrigir no dia em que a regra mudasse.
void Navegador::refaz_vista() {
  vista_.clear();
  switch (secao_) {
    case Secao::Artistas:
      for (const std::string& nome : livraria_.artistas())
        if (contem_sem_caixa(nome, termo_)) vista_.push_back({nome, nome, 0, 0, {}});
      break;

    case Secao::Albuns:
      for (const std::string& nome : livraria_.albuns(trilha_.front()))
        if (contem_sem_caixa(nome, termo_)) vista_.push_back({nome, nome, 0, 0, {}});
      break;

    case Secao::Faixas:
      for (const nucleo::Faixa& faixa :
           livraria_.faixas_do_album(trilha_.front(), trilha_.back()))
        if (contem_sem_caixa(faixa.titulo, termo_))
          vista_.push_back({faixa.titulo, faixa.caminho, faixa.numero,
                            faixa.duracao, faixa.artista});
      break;

    case Secao::Busca:
      for (const nucleo::Faixa& faixa : livraria_.busca_faixa(termo_))
        vista_.push_back({faixa.titulo, faixa.caminho, faixa.numero,
                          faixa.duracao, faixa.artista});
      break;
  }
  if (vista_.empty()) eleito_ = 0;
  else if (eleito_ >= vista_.size()) eleito_ = vista_.size() - 1;
}

}  // namespace mysong::tui

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
