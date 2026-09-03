// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO EXEMPLO DA FITA DA LETRA — exemplos/fita_letra.cpp
// ══════════════════════════════════════════════════════════════════════════
// Pinta o RIO da letra por cima do espectro, para se OLHAR e para se
// INSPECCIONAR. Não é prova: é a peça que o olho do operador ha de julgar n'um
// terminal com truecolor, e é tambem a sahida cujos bytes se conferem.
//
// Não colhe som nem relogio: a posição vem do argumento, donde a mesma linha de
// commando dá sempre os mesmos bytes. Corrida em tres posições, vê-se a mesma
// linha a nascer, a subir e a chegar, sem se abrir o tocador.
//
// DOMÍNIO ......... largura e altura em célullas, a posição em segundos, e o
//                   caminho de um `.lrc`. Sem elle, arma-se uma letra propria.
// CONTRA-DOMÍNIO .. `altura` linhas de espectro com o rio por cima, na sahida
//                   padrão, mais a linha que diz qual verso se canta; status 0.
// INVARIANTE ...... a fita sahe do MESMO tapete que a janella compõe. Regra de
//                   desenho alguma se escreve aqui: o que divergir do tapete é
//                   defeito, e não decisão d'este exemplo.
// Q.E.D. .......... redigida a sahida a um arquivo, os bytes dizem que glyphos
//                   sahiram, em que linha e com que tinta e que cama.
// ══════════════════════════════════════════════════════════════════════════
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <ftxui/dom/node.hpp>
#include <ftxui/screen/screen.hpp>

#include "nucleo/analisador.hpp"
#include "nucleo/letra.hpp"
#include "tui/espectro.hpp"
#include "tui/letra_viva.hpp"
#include "tui/tokens.hpp"

namespace es = mysong::tui;
namespace nu = mysong::nucleo;
namespace tk = mysong::tui::tokens;


namespace {

// A LETRA de dentro, quando não se dá `.lrc`. Quatro versos de quatro em quatro
// segundos, que é o passo em que o nascimento e a subida se vêem inteiros.
std::vector<nu::LinhaDaLetra> letra_de_dentro() {
  return {{4.0, "primeiro verso d'esta prova"},
          {8.0, "segundo verso d'esta prova"},
          {12.0, "terceiro verso, que é o que canta"},
          {16.0, "quarto verso d'esta prova"},
          {20.0, "quinto verso d'esta prova"}};
}

// bandas_da_posição — a fita do espectro por baixo, armada da POSIÇÃO e não do
// som: o exemplo não abre PipeWire, e ainda assim as barras hão de mexer entre
// uma corrida e a seguinte, que é o que o olho precisa de ver por baixo do rio.
std::vector<float> bandas_da_posicao(double posicao) {
  std::vector<float> bandas;
  for (std::size_t b = 0; b < nu::QUANTAS_BANDAS; ++b) {
    const double fase = posicao * 1.7 + static_cast<double>(b) * 0.6;
    // Serra em vez de seno: sem bibliotheca de trigonometria a mais, e o dente
    // dá altura differente em cada banda, que é o que a fita quer mostrar.
    const double dente = fase - static_cast<double>(static_cast<long>(fase));
    bandas.push_back(static_cast<float>(0.25 + 0.7 * dente));
  }
  return bandas;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 4) {
    std::fprintf(stderr,
                 "uso: fita_letra <largura> <altura> <posição> [arquivo.lrc]\n"
                 "  sem o `.lrc`, arma-se uma letra propria de quatro versos.\n");
    return 2;
  }
  const int largura = std::atoi(argv[1]);
  const int altura = std::atoi(argv[2]);
  const double posicao = std::atof(argv[3]);
  if (largura < 0 || altura < 0) return 2;

  std::vector<nu::LinhaDaLetra> linhas;
  if (argc > 4) {
    std::ifstream arquivo(argv[4]);
    if (!arquivo) {
      std::fprintf(stderr, "fita_letra: não se abriu «%s»\n", argv[4]);
      return 1;
    }
    std::ostringstream corpo;
    corpo << arquivo.rdbuf();
    linhas = nu::analysa_lrc(corpo.str());
  } else {
    linhas = letra_de_dentro();
  }

  const es::Quadro espectro =
      es::compor(bandas_da_posicao(posicao), static_cast<std::size_t>(largura),
                 static_cast<std::size_t>(altura), false,
                 es::centros_da_escala(nu::QUANTAS_BANDAS));
  // O BLOCO PARADO (issue #157) por cima, e o espectro por baixo: é a ordem em
  // que a janella os compõe, e o exemplo não inventa outra.
  const int corrente = nu::linha_corrente(linhas, posicao);
  ftxui::Screen ecran = ftxui::Screen::Create(
      ftxui::Dimension::Fixed(largura),
      ftxui::Dimension::Fixed(altura + static_cast<int>(es::FILEIRAS_DA_LETRA)));
  ftxui::Render(
      ecran,
      ftxui::vbox({es::elemento_da_letra_parada(
                       linhas, corrente, static_cast<std::size_t>(largura),
                       es::FILEIRAS_DA_LETRA),
                   es::elemento_do_espectro(espectro)}));
  std::string tela = ecran.ToString();
  tela += '\n';
  // A LINHA DA CONTA, por baixo, para se conferir o que o olho vê com o que a
  // funcção diz. Sem ella, quem lê o dump não sabe se a linha que assentou no
  // terço do alto é de facto a que o quadro tem por corrente.
  tela += "posição " + std::string(argv[3]) + ", " +
          std::to_string(linhas.size()) + " versos, canta: " +
          (corrente < 0 ? std::string("nenhum")
                        : "«" + es::verso_do_bloco(
                                    linhas, corrente,
                                    static_cast<std::size_t>(largura)) +
                              "»") +
          '\n';
  std::fputs(tela.c_str(), stdout);
  return 0;
}

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
