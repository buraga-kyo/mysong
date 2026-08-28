// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA PROVA DA FILA — testes/prova_fila.cpp
// ══════════════════════════════════════════════════════════════════════════
// Prova a fila SEM mpv, sem placa de som e sem arquivo em disco: os caminhos
// que se lhe entregam são cadeias que ella nunca abre. É metade do aceite que
// se pode provar em machina surda, e é por isso que a fila vive em tractado
// apartado do motor.
//
// DOMÍNIO ......... uma Fila armada aqui mesmo, com zero, uma ou tres faixas.
// CONTRA-DOMÍNIO .. veredicto do doctest, e por elle o status do ctest.
// INVARIANTE ...... nenhum caso d'esta prova toca no systema de arquivos nem
//                   na saída de áudio; roda igual em machina sem som algum.
// Q.E.D. .......... a ordem lida de volta é a ordem que o cliente definiu, e
//                   os passos que a issue pede respondem por booleano em vez
//                   de excepção, de sorte que a borda se assere e não se
//                   apanha.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <algorithm>

#include "nucleo/fila.hpp"

namespace {

// Tres faixas, na ordem em que o cliente as definiu.
mysong::nucleo::Fila com_tres() {
  mysong::nucleo::Fila fila;
  fila.junta("primeira.wav");
  fila.junta("segunda.wav");
  fila.junta("terceira.wav");
  return fila;
}

// Cinco faixas, que é o numero com que o aceite da issue #62 está escripto.
mysong::nucleo::Fila com_cinco() {
  mysong::nucleo::Fila fila;
  for (const char* nome : {"a.wav", "b.wav", "c.wav", "d.wav", "e.wav"})
    fila.junta(nome);
  return fila;
}

// O passeio inteiro para deante, colhendo o assento por onde se passa. Colhe-se
// o de partida tambem: elle é uma das faixas por que se passou.
std::vector<std::size_t> passeio(mysong::nucleo::Fila& fila) {
  std::vector<std::size_t> visitados{fila.indice()};
  while (fila.proxima()) visitados.push_back(fila.indice());
  return visitados;
}

}  // namespace

TEST_CASE("embaralhada, a fila passa por todas as faixas sem repetir nenhuma") {
  auto fila = com_cinco();
  fila.embaralhar(true);
  std::vector<std::size_t> visitados = passeio(fila);
  REQUIRE(visitados.size() == 5);      // cinco passos, e não quatro nem seis
  CHECK(visitados.front() == 0);       // a corrente vae ao principio
  std::sort(visitados.begin(), visitados.end());
  const std::vector<std::size_t> todos = {0, 1, 2, 3, 4};
  CHECK(visitados == todos);           // as cinco, e nenhuma duas vezes
}

TEST_CASE("desligar o embaralhar restitue a ordem e conserva a faixa") {
  auto fila = com_cinco();
  CHECK(fila.ir_para(2));
  fila.embaralhar(true);
  CHECK(fila.corrente() == "c.wav");  // ligar não troca a faixa
  REQUIRE(fila.proxima());
  REQUIRE(fila.proxima());
  const std::string tocando(fila.corrente());
  fila.embaralhar(false);
  CHECK(fila.corrente() == tocando);  // desligar tambem não
  const std::vector<std::string> chegada = {"a.wav", "b.wav", "c.wav", "d.wav",
                                            "e.wav"};
  CHECK(fila.todas() == chegada);
  CHECK(fila.ordem().empty());
}

TEST_CASE("a fila guarda a ordem que o cliente definiu") {
  auto fila = com_tres();
  CHECK_FALSE(fila.vazia());
  CHECK(fila.tamanho() == 3);
  CHECK(fila.indice() == 0);
  CHECK(fila.corrente() == "primeira.wav");
}

TEST_CASE("a fila anda nos dous sentidos e volta ao ponto de partida") {
  auto fila = com_tres();
  CHECK(fila.proxima());
  CHECK(fila.corrente() == "segunda.wav");
  CHECK(fila.proxima());
  CHECK(fila.corrente() == "terceira.wav");
  CHECK(fila.anterior());
  CHECK(fila.anterior());
  CHECK(fila.corrente() == "primeira.wav");
  CHECK(fila.indice() == 0);
}

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
TEST_CASE("fila vazia responde, e não erra") {
  mysong::nucleo::Fila fila;
  CHECK(fila.vazia());
  CHECK(fila.tamanho() == 0);
  CHECK(fila.corrente().empty());
  CHECK_FALSE(fila.proxima());
  CHECK_FALSE(fila.anterior());
  CHECK_FALSE(fila.ir_para(0));
}

TEST_CASE("a fila não envolve do ultimo ao primeiro") {
  auto fila = com_tres();
  CHECK(fila.ir_para(2));
  CHECK_FALSE(fila.proxima());
  CHECK(fila.indice() == 2);
  CHECK(fila.corrente() == "terceira.wav");
}

TEST_CASE("a fila não recua além do primeiro") {
  auto fila = com_tres();
  CHECK_FALSE(fila.anterior());
  CHECK(fila.indice() == 0);
  CHECK_FALSE(fila.ir_para(3));
  CHECK(fila.indice() == 0);
}

TEST_CASE("esvaziada, a fila torna ao estado de vazia") {
  auto fila = com_tres();
  CHECK(fila.proxima());
  fila.esvazia();
  CHECK(fila.vazia());
  CHECK(fila.indice() == 0);
}

TEST_CASE("todas dá a vista inteira, na ordem que o cliente definiu") {
  auto fila = com_tres();
  const std::vector<std::string> esperado = {"primeira.wav", "segunda.wav",
                                             "terceira.wav"};
  CHECK(fila.todas() == esperado);
  CHECK(mysong::nucleo::Fila().todas().empty());
}

// ══════════════════════════════════════════════════════════════════════════
