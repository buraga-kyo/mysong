// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO TOCADOR, LAVRA — src/nucleo/tocador.cpp
// ══════════════════════════════════════════════════════════════════════════
// Cumpre o cabecalho. Não inclue mpv algum: fala com o Motor abstracto e mais
// nada, e é d'ahi que lhe vem a inteira provabilidade.
//
// DOMÍNIO ......... o estado interno, e o Motor emprestado.
// CONTRA-DOMÍNIO .. booleanos, o estado, e o pregão aos ouvintes.
// INVARIANTE ...... nenhum evento sahe antes de o estado que elle retracta já
//                   estar assentado. Quem annuncia lê o estado, e não o
//                   adivinha.
// Q.E.D. .......... nenhuma linha d'esta unidade nomeia mpv, PipeWire ou
//                   arquivo; logo o dublê a exercita inteira, e o que a prova
//                   de som acrescenta é o contracto com a libmpv, e não esta
//                   mechanica.
// ══════════════════════════════════════════════════════════════════════════
#include "nucleo/tocador.hpp"

#include <mutex>
#include <utility>

namespace mysong::nucleo {

Tocador::Tocador(Motor& motor) noexcept : motor_(motor) {}

// Os punhos trancados da fila. Movem fila e indice, e nada mandam ao motor.
std::size_t Tocador::junta(std::string caminho) {
  std::lock_guard<std::mutex> chave(tranca_);
  fila_.junta(std::move(caminho));
  return fila_.tamanho();
}

bool Tocador::ir_para(std::size_t alvo) {
  std::lock_guard<std::mutex> chave(tranca_);
  return fila_.ir_para(alvo);
}

// A copia sahe INTEIRA debaixo da chave: vista crua não atravessa a tranca.
std::vector<std::string> Tocador::faixas(std::size_t* indice) const {
  std::lock_guard<std::mutex> chave(tranca_);
  if (indice != nullptr) *indice = fila_.vazia() ? 0 : fila_.indice();
  return fila_.todas();
}

// Ouvinte vazio não se guarda: guardá-lo seria adiar para a hora do pregão uma
// verificação que se faz de graça na hora do registro.
void Tocador::escuta(Ouvinte ouvinte) {
  std::lock_guard<std::mutex> chave(tranca_);
  if (ouvinte) ouvintes_.push_back(std::move(ouvinte));
}

// O pregão. Leva o retracto inteiro, e lê o estado em vez de o adivinhar.
void Tocador::annuncia(Aviso aviso, std::string razao) {
  Evento evento;
  evento.aviso = aviso;
  evento.estado = estado_;
  evento.faixa = std::string(fila_.corrente());
  evento.posicao = ultima_posicao_;
  evento.razao = std::move(razao);
  for (const Ouvinte& ouvinte : ouvintes_) ouvinte(evento);
}

// Só annuncia se de facto mudou, e SEMPRE depois de assentar.
void Tocador::assenta_estado(Estado novo) {
  if (novo == estado_) return;
  estado_ = novo;
  annuncia(Aviso::EstadoMudou);
}

// Manda tocar o que a fila aponta. O volume corrente vae com a faixa nova, que
// de outra sorte nasceria no volume de fabrica do motor.
bool Tocador::tocar_corrente() {
  std::lock_guard<std::mutex> chave(tranca_);
  return tocar_corrente_trancado();
}

// O miolo, chamado SEMPRE com a tranca já tomada: é por elle que proxima() e
// anterior() tocam sem tomar a tranca segunda vez.
bool Tocador::tocar_corrente_trancado() {
  if (fila_.vazia()) return false;
  const std::string caminho(fila_.corrente());
  if (!motor_.tocar(caminho)) {
    assenta_estado(Estado::Parado);
    annuncia(Aviso::FalhouAoTocar, "o motor recusou " + caminho);
    return false;
  }
  ultima_posicao_ = 0.0;
  motor_.volume(volume_);
  assenta_estado(Estado::Tocando);
  annuncia(Aviso::FaixaMudou);
  return true;
}

// A fila anda PRIMEIRO, e só depois se manda tocar. Na borda ella não anda,
// nada se manda, e a faixa em curso segue intacta.
bool Tocador::proxima() {
  std::lock_guard<std::mutex> chave(tranca_);
  return fila_.proxima() && tocar_corrente_trancado();
}

bool Tocador::anterior() {
  std::lock_guard<std::mutex> chave(tranca_);
  return fila_.anterior() && tocar_corrente_trancado();
}

// Pausar só faz sentido a tocar; retomar, só a pausado. Fóra d'ahi a ordem se
// recusa em vez de se mandar ao motor uma transição que elle não pode honrar.
bool Tocador::pausar() {
  std::lock_guard<std::mutex> chave(tranca_);
  if (estado_ != Estado::Tocando || !motor_.pausar()) return false;
  assenta_estado(Estado::Pausado);
  return true;
}

bool Tocador::retomar() {
  std::lock_guard<std::mutex> chave(tranca_);
  if (estado_ != Estado::Pausado || !motor_.retomar()) return false;
  assenta_estado(Estado::Tocando);
  return true;
}

// O alvo apara-se pela duração ANTES de descer ao motor, com a fonte unica de
// aparo que mora no tractado do motor.
bool Tocador::buscar(double segundos) {
  std::lock_guard<std::mutex> chave(tranca_);
  if (estado_ == Estado::Parado) return false;
  return motor_.buscar(aparar_busca(segundos, motor_.duracao()));
}

// O volume guarda-se aqui, e não sómente no motor: é elle que a faixa seguinte
// ha de herdar. E é do MOTOR, nunca do systema.
bool Tocador::volume(int porcento) {
  std::lock_guard<std::mutex> chave(tranca_);
  volume_ = aparar_volume(porcento);
  return motor_.volume(volume_);
}

Estado Tocador::estado() const noexcept {
  std::lock_guard<std::mutex> chave(tranca_);
  return estado_;
}

int Tocador::volume() const noexcept {
  std::lock_guard<std::mutex> chave(tranca_);
  return volume_;
}

double Tocador::posicao() const {
  std::lock_guard<std::mutex> chave(tranca_);
  return motor_.posicao();
}

double Tocador::duracao() const {
  std::lock_guard<std::mutex> chave(tranca_);
  return motor_.duracao();
}

// Tudo debaixo da MESMA chave: estado e posição do mesmo momento, e a faixa
// copiada antes de a tranca se soltar.
Retracto Tocador::retracto() const {
  std::lock_guard<std::mutex> chave(tranca_);
  Retracto obra;
  obra.estado = estado_;
  obra.faixa = std::string(fila_.corrente());
  obra.posicao = motor_.posicao();
  obra.duracao = motor_.duracao();
  obra.volume = volume_;
  obra.indice = fila_.vazia() ? 0 : fila_.indice();
  obra.tamanho = fila_.tamanho();
  obra.embaralhado = fila_.embaralhado();
  obra.repeticao = fila_.repeticao();
  return obra;
}

// Uma batida: drena o motor, colhe o que mudou, assenta AMBOS, e só então
// annuncia. Assentar ambos antes de qualquer pregão é o que cumpre a
// invariante do cabecalho: ao fim natural da faixa a posição vae a zero e o
// estado vae a Parado na MESMA batida, e um pregão emittido pelo meio levaria
// metade do retracto novo e metade do velho, que é composto que nunca foi
// verdade. Assentar o estado primeiro não bastaria: o EstadoMudou sahiria com
// a posição velha, que é o mesmo defeito virado do outro lado.
void Tocador::pulsa() {
  std::lock_guard<std::mutex> chave(tranca_);
  motor_.bombear();
  const Estado visto = motor_.estado();
  const double agora = motor_.posicao();
  const bool mudou_estado = visto != estado_;
  const bool andou = agora != ultima_posicao_;

  estado_ = visto;
  ultima_posicao_ = agora;

  if (mudou_estado) annuncia(Aviso::EstadoMudou);
  if (andou) annuncia(Aviso::PosicaoAndou);

  // A fonte das bandas bate no mesmo relogio do tocador, e não num seu: assim
  // quem já chama pulsa() ganha o relogio de guarda do espectro de graça, e não
  // ha uma segunda cadencia para alguem esquecer de bater.
  if (fonte_ != nullptr) fonte_->pulsa();
}

// ── OS DOUS MODOS (issue #62), trancados como todo o resto.
void Tocador::embaralhar(bool ligado) {
  std::lock_guard<std::mutex> chave(tranca_);
  fila_.embaralhar(ligado);
}

// Alternar de UMA tomada. Fosse o chamador a ler e depois escrever, outro fio
// caberia no meio, e a tecla assentaria o contrario do que o operador viu.
bool Tocador::alterna_embaralhar() {
  std::lock_guard<std::mutex> chave(tranca_);
  const bool novo = !fila_.embaralhado();
  fila_.embaralhar(novo);
  return novo;
}

void Tocador::repetir(Repeticao modo) {
  std::lock_guard<std::mutex> chave(tranca_);
  fila_.repetir(modo);
}

// O ciclo da tecla: nenhuma, uma, todas, e torna ao principio. O switch é
// exhaustivo de proposito: valor novo no enum accende aviso aqui, e não silencio.
Repeticao Tocador::cicla_repetir() {
  std::lock_guard<std::mutex> chave(tranca_);
  Repeticao novo = Repeticao::Nenhuma;
  switch (fila_.repeticao()) {
    case Repeticao::Nenhuma: novo = Repeticao::Uma; break;
    case Repeticao::Uma: novo = Repeticao::Todas; break;
    case Repeticao::Todas: novo = Repeticao::Nenhuma; break;
  }
  fila_.repetir(novo);
  return novo;
}

void Tocador::observa(FonteDeBandas& fonte) noexcept {
  std::lock_guard<std::mutex> chave(tranca_);
  fonte_ = &fonte;
}

std::vector<float> Tocador::bandas() const {
  std::lock_guard<std::mutex> chave(tranca_);
  if (fonte_ == nullptr) return std::vector<float>(QUANTAS_BANDAS, 0.0f);
  return fonte_->bandas();
}

}  // namespace mysong::nucleo

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
