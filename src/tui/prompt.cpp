// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO PROMPT — src/tui/prompt.cpp
// ══════════════════════════════════════════════════════════════════════════
// A taboada dos modos, e a pintura do topo. O contracto está em prompt.hpp.
// ══════════════════════════════════════════════════════════════════════════
#include "tui/prompt.hpp"

namespace mysong::tui {

bool aceita_letra(Modo modo) noexcept {
  return modo != Modo::Nada && modo != Modo::Confirma;
}

std::size_t linhas_do_topo(Modo modo) noexcept {
  return modo == Modo::Nada ? 1u : 2u;
}

// Os ROTULOS. Ficam as palavras que a janella já dizia, e sómente o filtro
// ganha palavra sua: uma barra sozinha n'uma linha nova lê-se como sujeira da
// tela, e não como campo em que se digita.
//
// O switch não tem `default`, de proposito: modo novo que se acrescente ao enum
// deixa de compilar aqui, em vez de sahir mudo na tela.
std::string rotulo_do_prompt(Modo modo, std::string_view contexto) {
  switch (modo) {
    case Modo::Busca: return "FILTRO:";
    case Modo::Url: return "URL:";
    case Modo::Procura: return "BUSCA NA REDE (" + std::string(contexto) + "):";
    case Modo::Lista: return "PLAYLIST DO SPOTIFY:";
    case Modo::NomeNovo: return "LISTA NOVA:";
    case Modo::NomeOutro: return "NOME:";
    case Modo::Confirma: return "apagar «" + std::string(contexto) + "»? s/n";
    case Modo::Nada: break;
  }
  return {};
}

}  // namespace mysong::tui
//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
