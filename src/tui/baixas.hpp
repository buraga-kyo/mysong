#pragma once
#include "nucleo/estaleiro.hpp"
#include "tui/rato.hpp"
#include <ftxui/dom/elements.hpp>

namespace mysong::tui {
struct CaixasDasBaixas {
  ftxui::Box acao = caixa_por_pintar();
  ftxui::Box anterior = caixa_por_pintar();
  ftxui::Box seguinte = caixa_por_pintar();
  std::size_t id = 0;
  bool recomecar = false;
};
// A altura preserva ao menos quatro linhas para a busca e seus resultados.
inline std::size_t altura_das_baixas(std::size_t disponivel) {
  return disponivel >= 16 ? 11 : disponivel >= 11 ? 7 : disponivel >= 8 ? 4 : 0;
}
ftxui::Element painel_das_baixas(const nucleo::Andamento& andamento,
    std::size_t pagina, std::size_t altura, CaixasDasBaixas& caixas);
}  // namespace mysong::tui
