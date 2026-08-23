// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA PROVA DO TOCADOR — testes/prova_tocador.cpp
// ══════════════════════════════════════════════════════════════════════════
// Prova o tocador com um motor DUBLÊ: uma carne de mentira que obedece á
// mesma interface e não abre mpv, nem placa de som, nem arquivo. É o que a
// issue pede com essas palavras, e o que faz esta prova rodar em machina surda.
//
// DOMÍNIO ......... um Tocador armado sobre o dublê, e ordens de operador.
// CONTRA-DOMÍNIO .. veredicto do doctest, e por elle o status do ctest.
// INVARIANTE ...... o dublê REGISTRA o que lhe mandaram, e não só devolve; é
//                   d'ahi que a prova assere a CHAMADA, e não o resultado.
//                   Tocador que devolvesse verdadeiro sem mandar nada ao motor
//                   passaria por uma prova de resultado, e cae n'esta.
// Q.E.D. .......... prova-se aqui a mechanica: fila, transições e pregão. O
//                   contracto com a libmpv NÃO se prova aqui, e nenhum caso
//                   d'este arquivo o finge: para elle ha o binario que soa.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <string>
#include <vector>

#include "nucleo/tocador.hpp"
namespace {

using mysong::nucleo::Aviso;
using mysong::nucleo::Estado;
using mysong::nucleo::Tocador;
// O DUBLÊ. Registra o que lhe mandaram, para que a prova o interrogue.
class MotorDuble final : public mysong::nucleo::Motor {
 public:
  std::vector<std::string> tocados;
  bool recusa_tocar = false;
  int volume_recebido = -1;
  double alvo_buscado = -1.0;
  double duracao_dita = 10.0;

  bool tocar(const std::string& caminho) override {
    if (recusa_tocar) return false;
    tocados.push_back(caminho);
    posicao_ = 0.0;
    estado_ = Estado::Tocando;
    return true;
  }

  bool pausar() override { estado_ = Estado::Pausado; return true; }
  bool retomar() override { estado_ = Estado::Tocando; return true; }
  bool buscar(double segundos) override { alvo_buscado = segundos; return true; }
  bool volume(int porcento) override { volume_recebido = porcento; return true; }

  double posicao() const override { return posicao_; }
  double duracao() const override { return duracao_dita; }
  Estado estado() const override { return estado_; }
  void bombear() override {}

  // Torniquetes de que só a prova se serve, para mover o mundo de mentira.
  void avanca(double delta) { posicao_ += delta; }
  void termina() { estado_ = Estado::Parado; }

 private:
  double posicao_ = 0.0;
  Estado estado_ = Estado::Parado;
};

}  // namespace

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
TEST_CASE("o motor recebe cada faixa da fila, nos dous sentidos") {
  MotorDuble duble;
  Tocador tocador(duble);
  tocador.fila().junta("uma.wav");
  tocador.fila().junta("duas.wav");
  tocador.fila().junta("tres.wav");

  CHECK(tocador.tocar_corrente());
  CHECK(tocador.proxima());
  CHECK(tocador.proxima());
  CHECK(tocador.anterior());
  CHECK(tocador.anterior());

  const std::vector<std::string> esperado = {"uma.wav", "duas.wav", "tres.wav",
                                             "duas.wav", "uma.wav"};
  CHECK(duble.tocados == esperado);
  CHECK(tocador.fila().corrente() == "uma.wav");
}

TEST_CASE("na borda da fila NADA se manda ao motor") {
  MotorDuble duble;
  Tocador tocador(duble);
  tocador.fila().junta("uma.wav");

  CHECK(tocador.tocar_corrente());
  CHECK_FALSE(tocador.proxima());
  CHECK_FALSE(tocador.anterior());
  CHECK(duble.tocados.size() == 1);
  CHECK(tocador.estado() == Estado::Tocando);
}

TEST_CASE("fila vazia não faz o tocador mandar nada") {
  MotorDuble duble;
  Tocador tocador(duble);

  CHECK_FALSE(tocador.tocar_corrente());
  CHECK_FALSE(tocador.proxima());
  CHECK_FALSE(tocador.anterior());
  CHECK(duble.tocados.empty());
  CHECK(tocador.estado() == Estado::Parado);
}

TEST_CASE("as transições de estado, todas quatro") {
  MotorDuble duble;
  Tocador tocador(duble);
  tocador.fila().junta("uma.wav");

  CHECK(tocador.estado() == Estado::Parado);
  CHECK(tocador.tocar_corrente());
  CHECK(tocador.estado() == Estado::Tocando);

  CHECK(tocador.pausar());
  CHECK(tocador.estado() == Estado::Pausado);
  CHECK_FALSE(tocador.pausar());  // pausar quem já pausou não é transição

  CHECK(tocador.retomar());
  CHECK(tocador.estado() == Estado::Tocando);
  CHECK_FALSE(tocador.retomar());

  duble.termina();  // a faixa acaba por si, como acaba no mundo
  tocador.pulsa();
  CHECK(tocador.estado() == Estado::Parado);
}

TEST_CASE("o motor que recusa não deixa o tocador a crer que toca") {
  MotorDuble duble;
  duble.recusa_tocar = true;
  Tocador tocador(duble);
  tocador.fila().junta("inexistente.wav");

  CHECK_FALSE(tocador.tocar_corrente());
  CHECK(tocador.estado() == Estado::Parado);
  CHECK(duble.tocados.empty());
}

TEST_CASE("o pregão chega a quem escuta") {
  MotorDuble duble;
  Tocador tocador(duble);
  tocador.fila().junta("uma.wav");

  int faixas = 0;
  int estados = 0;
  int posicoes = 0;
  tocador.escuta([&](const mysong::nucleo::Evento& evento) {
    switch (evento.aviso) {
      case Aviso::FaixaMudou: ++faixas; break;
      case Aviso::EstadoMudou: ++estados; break;
      case Aviso::PosicaoAndou: ++posicoes; break;
      case Aviso::FalhouAoTocar: break;
    }
  });

  CHECK(tocador.tocar_corrente());
  duble.avanca(1.5);
  tocador.pulsa();

  CHECK(faixas == 1);
  CHECK(estados == 1);
  CHECK(posicoes == 1);
}

TEST_CASE("zero ouvintes não é erro: tudo corre igual") {
  MotorDuble duble;
  Tocador tocador(duble);
  tocador.fila().junta("uma.wav");

  CHECK(tocador.tocar_corrente());
  duble.avanca(1.0);
  tocador.pulsa();
  CHECK(tocador.estado() == Estado::Tocando);
}

// ══════════════════════════════════════════════════════════════════════════
