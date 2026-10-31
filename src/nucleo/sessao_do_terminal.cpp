#include "nucleo/sessao_do_terminal.hpp"
#include "nucleo/aquisicao.hpp"
#include <sstream>

namespace mysong::nucleo {
CenaDoTerminal interpreta_cena_do_tmux(std::string_view resposta, const CenaDoTerminal& anterior) {
  std::istringstream campos{std::string(resposta)};
  int janela=0, painel=0;
  std::string geometria, extra;
  if (!(campos >> janela >> painel >> geometria) || (campos >> extra) ||
      (janela != 0 && janela != 1) || (painel != 0 && painel != 1)) return anterior;
  CenaDoTerminal cena{janela == 1, painel == 1 && janela == 1, geometria, anterior.geracao};
  if (cena.visivel != anterior.visivel || cena.ativo != anterior.ativo || cena.geometria != anterior.geometria)
    ++cena.geracao;
  return cena;
}
SessaoDoTerminal::SessaoDoTerminal(std::string painel, Consulta consulta)
    : painel_(std::move(painel)), consulta_(std::move(consulta)) {
  if (!consulta_) consulta_ = [](const std::string& alvo) {
    std::string resposta;
    corre({"tmux", "display-message", "-p", "-t", alvo,
        "#{window_active} #{pane_active} #{window_id}:#{pane_left}:#{pane_top}:#{pane_width}:#{pane_height}:#{pane_in_mode}"}, &resposta);
    return resposta;
  };
}
SessaoDoTerminal::~SessaoDoTerminal() {
  parar_->store(true); sino_.notify_all();
  if (trabalho_.joinable()) trabalho_.join();
}
void SessaoDoTerminal::inicia() {
  if (painel_.empty() || trabalho_.joinable()) return;
  trabalho_ = std::thread([this] {
    const EscopoDaBaixa escopo(parar_);
    while (!parar_->load()) {
      pulsa();
      std::unique_lock<std::mutex> chave(espera_);
      sino_.wait_for(chave,std::chrono::milliseconds(250),[this] { return parar_->load(); });
    }
  });
}
void SessaoDoTerminal::pulsa() {
  const auto agora = std::chrono::steady_clock::now();
  if (painel_.empty() || agora < proxima_) return;
  proxima_ = agora + std::chrono::milliseconds(250);
  const auto resposta = consulta_(painel_);
  std::lock_guard<std::mutex> chave(tranca_);
  cena_ = interpreta_cena_do_tmux(resposta, cena_);
}
CenaDoTerminal SessaoDoTerminal::retrato() const {
  std::lock_guard<std::mutex> chave(tranca_); return cena_;
}
}
