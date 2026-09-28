#include "tui/baixas.hpp"
#include "tui/tokens.hpp"
#include <algorithm>

namespace mysong::tui {
namespace {
using nucleo::EstadoDaBaixa;
using nucleo::RegistroDaBaixa;
// Cada estado tem palavra propria; cem por cento ainda pode estar finalizando.
std::string nome_do_estado(const RegistroDaBaixa& registro) {
  switch (registro.estado) {
    case EstadoDaBaixa::Aguardando: return "NA FILA";
    case EstadoDaBaixa::Preparando: return "PREPARANDO";
    case EstadoDaBaixa::Baixando:
      return registro.porcentagem && *registro.porcentagem == 100
          ? "FINALIZANDO" : "BAIXANDO";
    case EstadoDaBaixa::Concluido: return "CONCLUÍDO";
    case EstadoDaBaixa::Falhou: return "FALHOU";
    case EstadoDaBaixa::Parando: return "PARANDO";
    case EstadoDaBaixa::Parado: return "PARADO";
  }
  return {};
}
// A mesma paleta da sala conserva contraste entre aviso, falha e conclusão.
ftxui::Color tinta_da_baixa(EstadoDaBaixa estado) {
  const auto tom = tokens::rgb(estado == EstadoDaBaixa::Falhou ? tokens::crit
      : estado == EstadoDaBaixa::Concluido ? tokens::ok
      : estado == EstadoDaBaixa::Parado || estado == EstadoDaBaixa::Parando
          ? tokens::warn : tokens::v400);
  return ftxui::Color::RGB(tom.r, tom.g, tom.b);
}
// Porcentagem ausente conserva texto; não se inventa medida da rede.
ftxui::Element progresso_da_baixa(const RegistroDaBaixa& registro) {
  using namespace ftxui;
  const bool concluido = registro.estado == EstadoDaBaixa::Concluido;
  if (!concluido && !registro.porcentagem)
    return text("  " + nome_do_estado(registro) + " · aguardando informação") | dim;
  const int valor = concluido ? 100 : std::clamp(*registro.porcentagem, 0, 100);
  return hbox({text("  "), gauge(valor / 100.0f) | flex,
      text("  " + std::to_string(valor) + "%  ")}) |
      color(tinta_da_baixa(registro.estado));
}
}  // namespace
}  // namespace mysong::tui
