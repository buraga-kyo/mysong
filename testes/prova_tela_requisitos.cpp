// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DAS PROVAS DA TELA — testes/prova_tela_requisitos.cpp
// ══════════════════════════════════════════════════════════════════════════
// Prova a tela dos requisitos SEM abrir terminal algum: pinta-se o quadro num
// écran de papel, de largura escolhida, e afere-se o que elle diz.
//
// É d'isto que a tela de erro não apodrece. Tela julgada pelo olho de quem a
// abriu prova-se uma vez, no dia em que se escreveu, e nunca mais; pintada em
// écran de papel, a asserção corre em cada ctest.
//
// DOMÍNIO ......... relatorios de dublê, e larguras de écran escolhidas aqui.
// CONTRA-DOMÍNIO .. o veredicto do doctest, e por elle o do ctest.
// INVARIANTE ...... nenhum caso abre terminal, nem consulta o systema, nem lê
//                   ambiente. A largura é parametro, e não a do terminal.
// Q.E.D. .......... afere-se em quarenta collunas a mesma tela que se afere em
//                   cem; donde a tela estreita deixa de ser esperança.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <cstddef>
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>

#include "nucleo/sonda.hpp"
#include "tui/tela_requisitos.hpp"
#include "tui/tokens.hpp"

namespace nu = mysong::nucleo;
namespace tl = mysong::tui;
namespace tk = mysong::tui::tokens;

namespace {

// Dublê identico ao da prova da sonda, e de proposito repetido: prova que se
// apoia em auxiliar de outra prova quebra por motivo alheio ao que afirma.
nu::Inquerito faltando(std::initializer_list<std::string_view> chaves) {
  const std::vector<std::string_view> ausentes(chaves.begin(), chaves.end());
  const auto responde = [ausentes](nu::Especie especie, std::string_view alvo) {
    for (const nu::Requisito& requisito : nu::requisitos())
      if (requisito.especie == especie && requisito.alvo == alvo)
        for (const std::string_view chave : ausentes)
          if (chave == requisito.chave) return false;
    return true;
  };
  nu::Inquerito inquerito;
  inquerito.familia_de_fonte = [responde](std::string_view alvo) {
    return responde(nu::Especie::FamiliaDeFonte, alvo);
  };
  inquerito.bibliotheca = [responde](std::string_view alvo) {
    return responde(nu::Especie::Bibliotheca, alvo);
  };
  inquerito.executavel = [responde](std::string_view alvo) {
    return responde(nu::Especie::Executavel, alvo);
  };
  return inquerito;
}

}  // namespace
