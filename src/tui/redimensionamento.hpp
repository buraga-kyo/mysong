#pragma once
#include <chrono>
#include <mutex>

namespace mysong::tui {
struct AtualizacaoDasImagens {
  bool suspensas = false;
  bool ocultar = false;
  bool repor = false;
};
// Uma retirada ao começar e uma reposição após o último tamanho recebido.
class Redimensionamento {
 public:
  using Relogio = std::chrono::steady_clock;
  AtualizacaoDasImagens observa(int largura, int altura,
                               Relogio::time_point agora = Relogio::now());
  bool atualiza(int largura, int altura, Relogio::time_point agora = Relogio::now());
  bool pulsa(Relogio::time_point agora = Relogio::now());
  bool movendo() const;
  // A margem cobre os lotes de ~250ms observados no tmux.
  static constexpr auto ESTABILIZACAO = std::chrono::milliseconds(300);
 private:
  bool atualiza_trancado(int largura, int altura, Relogio::time_point agora);
  mutable std::mutex tranca_;
  bool iniciou_ = false, movendo_ = false, ocultas_ = false, retirar_ = false;
  int largura_ = 0, altura_ = 0;
  Relogio::time_point ultima_mudanca_{};
};
}
