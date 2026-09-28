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
}  // namespace
}  // namespace mysong::tui
