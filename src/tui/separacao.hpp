#pragma once
#include <ftxui/component/event.hpp>
#include <ftxui/dom/elements.hpp>
#include "nucleo/tocador.hpp"

namespace mysong::tui {
struct PainelDeSeparacao {
  bool aberto = false;
  std::string alvo;
  std::string aviso;
  int selecionada = 0;
  nucleo::VolumesDasPartes volumes{100, 100, 100, 100};
  bool tecla(const ftxui::Event&, nucleo::SeparadorDeAudio&, nucleo::Tocador&);
};
std::string situacao_da_separacao(const nucleo::RetratoDaSeparacao&,
    std::chrono::steady_clock::time_point agora = std::chrono::steady_clock::now());
ftxui::Element painel_de_separacao(const PainelDeSeparacao&, const nucleo::RetratoDaSeparacao&,
                                  const nucleo::Retracto&, int largura, int altura);
}
