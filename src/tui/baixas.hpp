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
// Novos pedidos ganham o acompanhamento. Quando o pedido termina, segue outro
// ainda ativo; consultar manualmente um pedido antigo continua permitido.
class AcompanhamentoDasBaixas {
 public:
  std::size_t atualiza(const nucleo::Andamento& andamento, std::size_t pagina);

 private:
  std::size_t maior_id_ = 0;
  std::size_t anterior_id_ = 0;
  bool anterior_terminou_ = true;
};

// A altura preserva ao menos quatro linhas para a busca e seus resultados.
inline std::size_t altura_das_baixas(std::size_t disponivel) {
  return disponivel >= 22 ? 17 : disponivel >= 16 ? 11 : disponivel >= 11 ? 7 : disponivel >= 8 ? 4 : 0;
}
ftxui::Element painel_das_baixas(const nucleo::Andamento& andamento,
    std::size_t pagina, std::size_t altura, CaixasDasBaixas& caixas,
    std::chrono::steady_clock::time_point agora = std::chrono::steady_clock::now());
}  // namespace mysong::tui
