#pragma once
#include <chrono>
#include <cstddef>
#include <ftxui/dom/elements.hpp>
#include "nucleo/escutas.hpp"

namespace mysong::tui {
struct PainelDeEscutas {
  bool aberto = false;
  int dias = 0;
  bool atualizar = true;
  std::chrono::steady_clock::time_point ultima_consulta{};
  nucleo::ResumoDasEscutas resumo;
};
ftxui::Element painel_de_escutas(const nucleo::ResumoDasEscutas& resumo, int dias,
                               std::size_t largura, std::size_t altura);
}  // namespace mysong::tui
