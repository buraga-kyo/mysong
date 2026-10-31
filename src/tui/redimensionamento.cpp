#include "tui/redimensionamento.hpp"
namespace mysong::tui {
AtualizacaoDasImagens Redimensionamento::observa(int largura, int altura,
    Relogio::time_point agora) {
  std::lock_guard<std::mutex> chave(tranca_);
  atualiza_trancado(largura,altura,agora);
  const bool ocultar = retirar_;
  retirar_ = false;
  const bool repor = !movendo_ && ocultas_;
  if (repor) ocultas_ = false;
  return {movendo_,ocultar,repor};
}
bool Redimensionamento::atualiza_trancado(int largura, int altura, Relogio::time_point agora) {
  const bool mudou = iniciou_ && (largura != largura_ || altura != altura_);
  iniciou_ = true;
  largura_ = largura; altura_ = altura;
  if (mudou) {
    if (!ocultas_) retirar_ = true;
    ocultas_ = true; movendo_ = true; ultima_mudanca_ = agora;
  }
  return mudou;
}
bool Redimensionamento::atualiza(int largura, int altura, Relogio::time_point agora) {
  std::lock_guard<std::mutex> chave(tranca_);
  return atualiza_trancado(largura,altura,agora);
}
bool Redimensionamento::movendo() const {
  std::lock_guard<std::mutex> chave(tranca_);
  return movendo_;
}
bool Redimensionamento::pulsa(Relogio::time_point agora) {
  std::lock_guard<std::mutex> chave(tranca_);
  if (!movendo_ || agora - ultima_mudanca_ < ESTABILIZACAO) return false;
  movendo_ = false;
  return true;
}
}
