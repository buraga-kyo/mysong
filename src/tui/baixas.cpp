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
// Só os botões habilitados recebem caixa, impedindo cliques em acções antigas.
ftxui::Element controles_da_baixa(const RegistroDaBaixa& registro,
    std::size_t pagina, std::size_t total, CaixasDasBaixas& caixas) {
  using namespace ftxui;
  caixas.id = registro.id;
  caixas.recomecar = registro.estado == EstadoDaBaixa::Parado ||
                      registro.estado == EstadoDaBaixa::Falhou;
  const bool parar = !nucleo::baixa_terminada(registro.estado) &&
                      registro.estado != EstadoDaBaixa::Parando;
  Element acao = text(caixas.recomecar ? " [R] Recomeçar " : parar ? " [P] Parar "
      : registro.estado == EstadoDaBaixa::Parando ? " Parando... " : " Salvo ");
  if (parar || caixas.recomecar)
    acao = acao | bold | inverted | reflect(caixas.acao);
  else acao = acao | dim;
  return hbox({text(" < ") | reflect(caixas.anterior),
      text(std::to_string(pagina + 1) + "/" + std::to_string(total)),
      text(" V > ") | reflect(caixas.seguinte), filler(), std::move(acao)});
}
// O conselho dá ao operador um passo concreto quando o progresso não basta.
std::string conselho_da_baixa(const RegistroDaBaixa& registro) {
  if (registro.estado == EstadoDaBaixa::Falhou)
    return "Não foi possível concluir. Confira a conexão e tente novamente. " + registro.detalhe;
  if (registro.estado == EstadoDaBaixa::Parado)
    return "Download interrompido. Recomeçar inicia uma nova tentativa.";
  if (registro.estado == EstadoDaBaixa::Parando) return "Encerrando a tentativa atual...";
  if (registro.estado == EstadoDaBaixa::Aguardando) return "Sua música está na fila.";
  if (!registro.detalhe.empty()) return registro.detalhe;
  return registro.estado == EstadoDaBaixa::Concluido
      ? "Tudo pronto. A música está na biblioteca." : "Pode continuar ouvindo enquanto baixamos.";
}
}  // namespace
// Retracto puro: caixas só valem para o registro efectivamente desenhado.
ftxui::Element painel_das_baixas(const nucleo::Andamento& andamento,
    std::size_t pagina, std::size_t altura, CaixasDasBaixas& caixas) {
  using namespace ftxui;
  caixas = {};
  if (altura == 0) return emptyElement();
  Elements linhas;
  linhas.push_back(text(" ↓ TRANSFERÊNCIAS") | bold);
  if (andamento.registros.empty()) {
    linhas.push_back(paragraph("Busque uma música com s ou cole um link com u."));
    linhas.push_back(text("Seus downloads aparecerão aqui.") | dim);
  } else {
    pagina %= andamento.registros.size();
    const auto& registro = andamento.registros[pagina];
    if (altura >= 7) linhas.push_back(text(" " + std::string(nucleo::nome_da_fonte(registro.fonte)) +
        " · " + nome_do_estado(registro)) | color(tinta_da_baixa(registro.estado)));
    linhas.push_back(text(" " + registro.titulo) | bold);
    linhas.push_back(progresso_da_baixa(registro));
    if (altura >= 11) linhas.push_back(paragraph(conselho_da_baixa(registro)) |
        size(HEIGHT, EQUAL, 2));
    linhas.push_back(controles_da_baixa(registro, pagina, andamento.registros.size(), caixas));
  }
  Element painel = vbox(std::move(linhas));
  if (altura >= 7) painel = painel | borderRounded;
  return painel | size(HEIGHT, EQUAL, static_cast<int>(altura));
}
}  // namespace mysong::tui
