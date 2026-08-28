// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA PROVA DOS FIOS — testes/prova_tocador_fios.cpp
// ══════════════════════════════════════════════════════════════════════════
// Bate no MESMO tocador de mais de um fio ao mesmo tempo, mil voltas cada,
// como a janella faz: o relogio n'um fio, as teclas n'outro. O oraculo de
// verdade é o sanitizador de fios, que o aceite da issue #50 corre sobre esta
// bateria; as asserções ficam para DEPOIS do join, que o doctest não assere
// de fio segundo.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <string>
#include <thread>

#include "nucleo/tocador.hpp"

namespace {

using mysong::nucleo::Estado;
using mysong::nucleo::Evento;
using mysong::nucleo::Retracto;
using mysong::nucleo::Tocador;

// O MOTOR SURDO: campos nús de proposito, sem guarda propria. É a tranca do
// tocador que ha de serializar quem lhe chega; falhando ella um punho que
// seja, é n'estes campos que o sanitizador aponta primeiro.
class MotorSurdo final : public mysong::nucleo::Motor {
 public:
  bool tocar(const std::string&) override {
    posicao_ = 0.0;
    estado_ = Estado::Tocando;
    return true;
  }
  bool pausar() override { estado_ = Estado::Pausado; return true; }
  bool retomar() override { estado_ = Estado::Tocando; return true; }
  bool buscar(double alvo) override { posicao_ = alvo; return true; }
  bool volume(int porcento) override { volume_ = porcento; return true; }
  double posicao() const override { return posicao_; }
  double duracao() const override { return 30.0; }
  Estado estado() const override { return estado_; }
  void bombear() override { posicao_ += 0.05; ++bombeadas_; }

  unsigned long bombeadas() const { return bombeadas_; }

 private:
  Estado estado_ = Estado::Parado;
  double posicao_ = 0.0;
  int volume_ = 100;
  unsigned long bombeadas_ = 0;
};

// Mil voltas por fio, como o aceite pede.
constexpr int VOLTAS = 1000;

}  // namespace

// O fio do relogio bombeia e assigna; o da tela dá TODA ordem de tecla que a
// janella dá, revezadas. Mil voltas cada, e as contas conferem-se no fim: a
// fila ha de ter as duas faixas de partida mais uma por volta de juntada, e o
// motor ha de ter sido bombeado uma vez por batida, nem mais nem menos.
TEST_CASE("dous fios, mil voltas: relogio e tela no mesmo tocador") {
  // O OUVINTE entra para que o PREGÃO entre na prova. Não havendo nenhum
  // registrado, o laço de annuncia() corre sobre vector vazio, e o caminho fica
  // de fóra do que o sanitizador varre logo elle, que sahe com a tranca TOMADA.
  // Conta, e mais nada: o contracto do tocador diz que ouvinte não o chama de
  // volta, que a tranca não é reentrante. E o contador vae NÚ, sem atomico
  // proprio, pela mesma razão dos campos do motor surdo: é a tranca que o ha de
  // serializar, e falhando ella um punho que seja, é aqui que o sanitizador
  // aponta.
  unsigned long pregoes = 0;
  MotorSurdo motor;
  Tocador tocador(motor);
  tocador.escuta([&pregoes](const Evento&) { ++pregoes; });
  tocador.junta("uma.wav");
  tocador.junta("duas.wav");
  CHECK(tocador.tocar_corrente());

  std::thread relogio([&] {
    for (int volta = 0; volta < VOLTAS; ++volta) {
      tocador.pulsa();
      (void)tocador.retracto();  // a assignatura do visivel faz o mesmo
      (void)tocador.bandas();
    }
  });
  std::thread tela([&] {
    for (int volta = 0; volta < VOLTAS; ++volta) {
      switch (volta % 8) {
        case 0: tocador.pausar(); break;
        case 1: tocador.retomar(); break;
        case 2: tocador.buscar(volta % 30); break;
        case 3: tocador.volume(volta % 150); break;
        case 4: tocador.proxima(); break;
        case 5: tocador.anterior(); break;
        case 6: tocador.junta("fio_" + std::to_string(volta)); break;
        case 7: tocador.ir_para(0); tocador.tocar_corrente(); break;
      }
    }
  });
  relogio.join();
  tela.join();

  const Retracto fim = tocador.retracto();
  CHECK(fim.tamanho == 2 + VOLTAS / 8);
  CHECK(motor.bombeadas() == static_cast<unsigned long>(VOLTAS));
  CHECK((fim.volume >= 0 && fim.volume <= 100));
  CHECK(pregoes > 0);  // o pregão chegou: o laço não correu sobre vector vazio
}

// O barramento de verdade corre hoje no fio do relogio, que é quem drena as
// mensagens em pulsa(); aqui elle corre em fio PROPRIO, de proposito. Se um
// dia as mensagens mudarem de fio, a tranca já respondeu por ellas.
TEST_CASE("tres fios, mil voltas: o barramento entra na conta") {
  MotorSurdo motor;
  Tocador tocador(motor);
  tocador.junta("uma.wav");
  CHECK(tocador.tocar_corrente());

  std::thread relogio([&] {
    for (int volta = 0; volta < VOLTAS; ++volta) {
      tocador.pulsa();
      (void)tocador.retracto();
    }
  });
  std::thread tela([&] {
    for (int volta = 0; volta < VOLTAS; ++volta) {
      if (volta % 2 == 0) tocador.pausar();
      else tocador.retomar();
      (void)tocador.faixas();
    }
  });
  std::thread barramento([&] {
    for (int volta = 0; volta < VOLTAS; ++volta) {
      const Retracto agora = tocador.retracto();  // Get e GetAll fazem isto
      tocador.volume(agora.volume);               // e Set(Volume) faz isto
      if (volta % 100 == 99) tocador.proxima();   // Next, na borda da fila
    }
  });
  relogio.join();
  tela.join();
  barramento.join();

  const Retracto fim = tocador.retracto();
  CHECK(fim.tamanho == 1);
  CHECK(fim.faixa == "uma.wav");
  CHECK(motor.bombeadas() == static_cast<unsigned long>(VOLTAS));
}

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
