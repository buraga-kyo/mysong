// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DAS PROVAS DO RIO — testes/prova_letra_viva.cpp
// ══════════════════════════════════════════════════════════════════════════
// Prova o rio da letra da issue #109. Caso algum abre terminal, som ou relogio:
// o rio resolve-se em QUADRO por POSIÇÃO, e é pelo quadro que se prova. Sendo a
// funcção pura, a posição escreve-se á mão e o instante da prova é o instante
// que se quer, sem esperar segundo algum.
//
// A LIÇÃO D'ESTA CASA, que aqui se obedece: o alvo esperado vem escripto Á MÃO,
// ou recalculado por conta INDEPENDENTE. Jamais se compara a sahida de
// quadro_da_letra() com outra sahida d'ella, salvo quando a egualdade das duas É
// o que se afirma, que é o caso da pureza. E nome de caso sem ponto e vírgula,
// que o ponto e vírgula parte a lista do CMake e zero casos corridos dá status
// de successo.
//
// DOMÍNIO ......... versos armados aqui mesmo, com os instantes escolhidos no
//                   proprio caso, e um quadro de espectro de barras cheias.
// CONTRA-DOMÍNIO .. o veredicto do doctest, e por elle o do ctest.
// INVARIANTE ...... o que se afirma é a REGRA do rio, e não a apparencia: a
//                   linha da tela por INDICE, a fracção resolvida, a tinta por
//                   token, e a composição por célulla.
// Q.E.D. .......... provada a linha da tela por indice, o rio de cabeça para
//                   baixo deixa de passar calado, que a contagem de célullas não
//                   muda com o sentido.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <cstddef>
#include <string>
#include <vector>

#include <ftxui/screen/screen.hpp>

#include "nucleo/letra.hpp"
#include "tui/espectro.hpp"
#include "tui/letra_viva.hpp"
#include "tui/tokens.hpp"

namespace tui = mysong::tui;
namespace nu = mysong::nucleo;
namespace tk = mysong::tui::tokens;

namespace {

// A LETRA de prova: dous versos, um aos dez segundos e outro aos catorze. Cinco
// glyphos cada, que é o que faz as contas da fracção resolvida sahirem redondas.
const std::vector<nu::LinhaDaLetra> kVersos = {{10.0, "abcde"}, {14.0, "fghij"}};

// O painel: vinte por doze. D'ahi a linha de leitura em quatro (doze a dividir
// por tres) e a base em onze, e o vão da subida de sete. Escrevem-se Á MÃO, que
// recalculá-los aqui seria perguntar ao oraculo sob exame.
constexpr std::size_t kLargura = 20;
constexpr std::size_t kAltura = 12;
constexpr std::size_t kLeitura = 4;
constexpr std::size_t kBase = 11;

// d_ella — a linha viva de tal verso, ou nada. Busca-se pelo `qual`, e não pela
// ordem no vector: quem morre no alto sae do quadro, e o indice deslocava-se.
const tui::LinhaViva* d_ella(const tui::QuadroDaLetra& quadro,
                             std::size_t qual) {
  for (const tui::LinhaViva& viva : quadro.linhas)
    if (viva.qual == qual) return &viva;
  return nullptr;
}

}  // namespace

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
