// ══════════════════════════════════════════════════════════════════════════
//   PROVA DO LETREIRO — testes/prova_letreiro.cpp
// ══════════════════════════════════════════════════════════════════════════
// A linha do pango-view, a margem que casa a proporção, a chave do cache, a
// decisão de haver letreiro, e a ordem das tres chapas sobre um cabeçalho de
// PAPEL. Programa algum se corre, e X11 algum se abre: as cinco cousas são
// funcções puras, e é para isto que ellas o são.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <string>
#include <vector>

#include "nucleo/letreiro.hpp"
#include "tui/cabecalho.hpp"

namespace nu = mysong::nucleo;
namespace tui = mysong::tui;

namespace {

// O pedido da aba corrente, que é o que o cabeçalho manda rasterizar: a
// palavra, o par de côres do bloco solido, e as sete cellas d'ella.
nu::PedidoDaChapa da_corrente() {
  nu::PedidoDaChapa pedido;
  pedido.texto = "MY SONG";
  pedido.tinta = "#f4effe";
  pedido.fundo = "#7c3aed";
  pedido.cellulas = 7;
  return pedido;
}

}  // namespace

TEST_CASE("a linha do pango-view sahe argumento a argumento") {
  const std::vector<std::string> linha =
      nu::argumentos_do_letreiro(da_corrente(), 13, "/tmp/chapa.png");
  CHECK(linha == std::vector<std::string>{
                     "pango-view", "--font=Xirod 22", "--foreground=#f4effe",
                     "--background=#7c3aed", "--margin=13 0", "-q", "-o",
                     "/tmp/chapa.png", "-t", "MY SONG"});
}
