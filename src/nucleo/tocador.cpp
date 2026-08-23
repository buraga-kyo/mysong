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

#include <utility>

namespace mysong::nucleo {

Tocador::Tocador(Motor& motor) noexcept : motor_(motor) {}

Fila& Tocador::fila() noexcept { return fila_; }
const Fila& Tocador::fila() const noexcept { return fila_; }

// Ouvinte vazio não se guarda: guardá-lo seria adiar para a hora do pregão uma
// verificação que se faz de graça na hora do registro.
void Tocador::escuta(Ouvinte ouvinte) {
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
  return fila_.proxima() && tocar_corrente();
}

bool Tocador::anterior() {
  return fila_.anterior() && tocar_corrente();
}

// Pausar só faz sentido a tocar; retomar, só a pausado. Fóra d'ahi a ordem se
// recusa em vez de se mandar ao motor uma transição que elle não pode honrar.
bool Tocador::pausar() {
  if (estado_ != Estado::Tocando || !motor_.pausar()) return false;
  assenta_estado(Estado::Pausado);
  return true;
}

bool Tocador::retomar() {
  if (estado_ != Estado::Pausado || !motor_.retomar()) return false;
  assenta_estado(Estado::Tocando);
  return true;
}

// O alvo apara-se pela duração ANTES de descer ao motor, com a fonte unica de
// aparo que mora no tractado do motor.
bool Tocador::buscar(double segundos) {
  if (estado_ == Estado::Parado) return false;
  return motor_.buscar(aparar_busca(segundos, motor_.duracao()));
}

// O volume guarda-se aqui, e não sómente no motor: é elle que a faixa seguinte
// ha de herdar. E é do MOTOR, nunca do systema.
bool Tocador::volume(int porcento) {
  volume_ = aparar_volume(porcento);
  return motor_.volume(volume_);
}

Estado Tocador::estado() const noexcept { return estado_; }
int Tocador::volume() const noexcept { return volume_; }
double Tocador::posicao() const { return motor_.posicao(); }
double Tocador::duracao() const { return motor_.duracao(); }

// Uma batida: drena o motor, annuncia o que andou, e assenta o estado que o
// motor de facto tem. A ordem importa: a posição sobe antes do pregão, para
// que o retracto que sahe já traga a posição nova.
void Tocador::pulsa() {
  motor_.bombear();
  const Estado visto = motor_.estado();
  const double agora = motor_.posicao();
  if (agora != ultima_posicao_) {
    ultima_posicao_ = agora;
    annuncia(Aviso::PosicaoAndou);
  }
  assenta_estado(visto);
}

}  // namespace mysong::nucleo

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
