// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO EXEMPLO DA FITA DA LOUSA, exemplos/fita_lousa.cpp
// ══════════════════════════════════════════════════════════════════════════
// Põe uma imagem no rectangulo pedido, espera, e tira-a. É a prova do OLHO da
// lousa sem se abrir o tocador: o que se vê aqui é o que o painel ha de
// mostrar. Escreve uma regua de collunhas e linhas por baixo, para que a
// posição se confira contando, e não por impressão.
//
// DOMÍNIO ......... um arquivo, o canto em `COLLUNHAxLINHA`, o rectangulo em
//                   `LARGURAxALTURA` de célullas, e os segundos de espera.
// CONTRA-DOMÍNIO .. a imagem na tela por esse tempo, e o status zero; ou nada
//                   na tela, queixa no erro, e o status dous.
// INVARIANTE ...... a lousa desfaz-se ao sahir, por qualquer das portas, e
//                   fantasma algum fica na tela.
// Q.E.D. .......... havendo regua debaixo da imagem, a prova do olho conta
//                   célullas em vez de as estimar.
// ══════════════════════════════════════════════════════════════════════════
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>
#include <thread>

#include "nucleo/lousa.hpp"

namespace nu = mysong::nucleo;

namespace {

// par_de, o `AxB`, e sómente elle. Zero admitte-se, ao contrario do que faz o
// fita_capa: o canto do terminal É a collunha zero, e recusál-o tiraria da
// prova justamente a posição mais fácil de conferir.
bool par_de(std::string_view texto, long* um, long* outro) {
  const std::size_t cruz = texto.find('x');
  if (cruz == std::string_view::npos) return false;
  const std::string esquerda(texto.substr(0, cruz));
  const std::string direita(texto.substr(cruz + 1));
  char* resto = nullptr;
  *um = std::strtol(esquerda.c_str(), &resto, 10);
  if (resto == nullptr || *resto != '\0' || *um < 0) return false;
  *outro = std::strtol(direita.c_str(), &resto, 10);
  return resto != nullptr && *resto == '\0' && *outro >= 0;
}

}  // namespace

int main(int argc, char** argv) {
  long collunha = 0, linha = 0, largura = 0, altura = 0;
  if (argc < 5 || !par_de(argv[2], &collunha, &linha) ||
      !par_de(argv[3], &largura, &altura)) {
    std::fprintf(stderr,
                 "uso: fita_lousa <imagem> <COLLUNHAxLINHA> <LARGURAxALTURA>"
                 " <segundos>\n");
    return 2;
  }
  char* resto = nullptr;
  const double espera = std::strtod(argv[4], &resto);
  // Afere-se como o par: «oito» dá zero e «-8» dá negativo, e nos dous casos a
  // imagem piscava e sumia, e quem olhasse concluiria que a lousa está quebrada.
  if (resto == nullptr || *resto != '\0' || !(espera > 0.0)) {
    std::fprintf(stderr, "fita_lousa: «%s» não é numero de segundos\n", argv[4]);
    return 2;
  }
  // A tela LIMPA-SE, e o cursor vae ao canto: a regua conta-se do cursor para
  // baixo e o Überzug++ conta do canto ABSOLUTO. Havendo prompt por cima, a
  // linha rotulada zero não era a linha zero, e a prova do olho confirmava
  // posição errada com ar de certeza.
  std::fputs("\x1b[2J\x1b[H", stdout);
  // A REGUA, para que a prova do olho conte em vez de estimar: uma linha por
  // fileira, numerada de zero, com marca de dez em dez collunhas.
  for (int i = 0; i < static_cast<int>(linha + altura + 2); ++i) {
    std::printf("%02d", i);
    for (int c = 2; c < 78; ++c) std::putchar(c % 10 == 0 ? '0' + c / 10 : '.');
    std::putchar('\n');
  }
  nu::Lousa lousa(nu::ModoDaLousa::Auto);
  if (!lousa.disponivel()) {
    std::fprintf(stderr, "fita_lousa: %s\n", lousa.parecer().razao.c_str());
    return 2;
  }
  if (!lousa.poe("fita", argv[1], static_cast<int>(collunha),
                 static_cast<int>(linha), static_cast<std::size_t>(largura),
                 static_cast<std::size_t>(altura))) {
    std::fprintf(stderr, "fita_lousa: a ordem não passou pelo cano\n");
    return 2;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(
      static_cast<long long>(espera * 1000.0)));
  // O `tira` explicito, e não sómente o destructor: a prova ha de ver a imagem
  // sahir com o programa ainda vivo, que é o caso do foco que se perde.
  lousa.tira("fita");
  return 0;
}

//   Da lavra do eminente Doutor BURAGA KYO., buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
