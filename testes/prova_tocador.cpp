// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA PROVA DO TOCADOR, testes/prova_tocador.cpp
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
  // O bombear do dublê NÃO é vazio. Vazio, elle nunca produzia o unico
  // acontecimento em que posição e estado mudam na MESMA batida, e por isso a
  // bateria inteira era cega ao pregão de retracto composto.
  void bombear() override {
    if (!fim_pendente_) return;
    fim_pendente_ = false;
    posicao_ = 0.0;
    estado_ = Estado::Parado;
  }

  // Torniquetes de que só a prova se serve, para mover o mundo de mentira.
  void avanca(double delta) { posicao_ += delta; }
  void termina() { estado_ = Estado::Parado; }

  // Agenda o fim NATURAL da faixa, tal qual a libmpv o dá: posição a zero e
  // estado a Parado, ambos na batida seguinte, e não em batidas differentes.
  void acaba_na_proxima_batida() { fim_pendente_ = true; }

 private:
  double posicao_ = 0.0;
  Estado estado_ = Estado::Parado;
  bool fim_pendente_ = false;
};

}  // namespace

// ══════════════════════════════════════════════════════════════════════════

// O ENCADEAMENTO (issue #149): acabada a faixa, a seguinte entra sósinha. É na
// BATIDA que elle se dá, que é onde o fim da faixa se sabe, e por isso todos
// estes casos passam pelo `pulsa`.
TEST_CASE("acabada a faixa, a seguinte entra na mesma batida") {
  MotorDuble duble;
  Tocador tocador(duble);
  tocador.junta("uma.wav");
  tocador.junta("duas.wav");
  REQUIRE(tocador.tocar_corrente());
  duble.acaba_na_proxima_batida();
  tocador.pulsa();
  const std::vector<std::string> esperado = {"uma.wav", "duas.wav"};
  CHECK(duble.tocados == esperado);
  CHECK(tocador.retracto().estado == mysong::nucleo::Estado::Tocando);
  CHECK(tocador.retracto().faixa == "duas.wav");
}

TEST_CASE("sem repetição, acabada a ultima o tocador pára") {
  MotorDuble duble;
  Tocador tocador(duble);
  tocador.junta("uma.wav");
  REQUIRE(tocador.tocar_corrente());
  duble.acaba_na_proxima_batida();
  tocador.pulsa();
  const std::vector<std::string> esperado = {"uma.wav"};
  CHECK(duble.tocados == esperado);  // não tornou a tocar cousa alguma
  CHECK(tocador.retracto().estado == mysong::nucleo::Estado::Parado);
}

TEST_CASE("com repetir UMA a mesma torna, e com TODAS a lista gira") {
  MotorDuble uma_vez;
  Tocador d_uma(uma_vez);
  d_uma.junta("uma.wav");
  d_uma.junta("duas.wav");
  d_uma.repetir(mysong::nucleo::Repeticao::Uma);
  REQUIRE(d_uma.tocar_corrente());
  uma_vez.acaba_na_proxima_batida();
  d_uma.pulsa();
  const std::vector<std::string> mesma = {"uma.wav", "uma.wav"};
  CHECK(uma_vez.tocados == mesma);

  MotorDuble gira;
  Tocador de_todas(gira);
  de_todas.junta("uma.wav");
  de_todas.junta("duas.wav");
  de_todas.repetir(mysong::nucleo::Repeticao::Todas);
  REQUIRE(de_todas.tocar_corrente());
  REQUIRE(de_todas.proxima());  // está na ultima
  gira.acaba_na_proxima_batida();
  de_todas.pulsa();
  const std::vector<std::string> volta = {"uma.wav", "duas.wav", "uma.wav"};
  CHECK(gira.tocados == volta);
}

TEST_CASE("a faixa que o motor recusa não encadeia a seguinte") {
  MotorDuble duble;
  Tocador tocador(duble);
  tocador.junta("uma.wav");
  tocador.junta("duas.wav");
  tocador.junta("tres.wav");
  REQUIRE(tocador.tocar_corrente());
  duble.recusa_tocar = true;
  duble.acaba_na_proxima_batida();
  tocador.pulsa();
  // Tentou-se a segunda, e ella recusou: alli se pára, em vez de correr a
  // lista inteira em silencio n'um segundo.
  const std::vector<std::string> esperado = {"uma.wav"};
  CHECK(duble.tocados == esperado);
  CHECK(tocador.retracto().estado == mysong::nucleo::Estado::Parado);
}

TEST_CASE("com a fila vazia a batida do fim nada encadeia") {
  MotorDuble duble;
  Tocador tocador(duble);
  duble.acaba_na_proxima_batida();
  tocador.pulsa();
  CHECK(duble.tocados.empty());
  CHECK(tocador.retracto().estado == mysong::nucleo::Estado::Parado);
}

//   Da lavra do eminente Doutor BRAGA US., Braga Us ✒
TEST_CASE("o motor recebe cada faixa da fila, nos dous sentidos") {
  MotorDuble duble;
  Tocador tocador(duble);
  tocador.junta("uma.wav");
  tocador.junta("duas.wav");
  tocador.junta("tres.wav");

  CHECK(tocador.tocar_corrente());
  CHECK(tocador.proxima());
  CHECK(tocador.proxima());
  CHECK(tocador.anterior());
  CHECK(tocador.anterior());

  const std::vector<std::string> esperado = {"uma.wav", "duas.wav", "tres.wav",
                                             "duas.wav", "uma.wav"};
  CHECK(duble.tocados == esperado);
  CHECK(tocador.retracto().faixa == "uma.wav");
}

TEST_CASE("na borda da fila NADA se manda ao motor") {
  MotorDuble duble;
  Tocador tocador(duble);
  tocador.junta("uma.wav");

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

TEST_CASE("o assento sahe com as faixas, da mesma tomada") {
  // A issue #63: o verbo «fila» do protocolo lia as faixas n'uma tomada da
  // tranca e o indice n'outra, e entre ellas o mundo andava. Este punho devolve
  // os dous do MESMO instante, que é o que fecha a janella.
  MotorDuble duble;
  Tocador tocador(duble);
  for (const char* faixa : {"uma.wav", "duas.wav", "tres.wav"})
    tocador.junta(faixa);
  REQUIRE(tocador.ir_para(1));
  std::size_t assento = 99;
  const std::vector<std::string> faixas = tocador.faixas(&assento);
  CHECK(faixas.size() == 3);
  CHECK(assento == 1);
  // Sem o parametro, o punho é o de sempre: quem só quer a lista não muda.
  CHECK(tocador.faixas().size() == 3);
  // Fila vazia dá assento ZERO, e não o lixo que estivesse na variavel.
  MotorDuble outro;
  Tocador nova(outro);
  std::size_t nada = 77;
  CHECK(nova.faixas(&nada).empty());
  CHECK(nada == 0);
}

TEST_CASE("as transições de estado, todas quatro") {
  MotorDuble duble;
  Tocador tocador(duble);
  tocador.junta("uma.wav");

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
  tocador.junta("inexistente.wav");

  CHECK_FALSE(tocador.tocar_corrente());
  CHECK(tocador.estado() == Estado::Parado);
  CHECK(duble.tocados.empty());
}

TEST_CASE("o pregão chega a quem escuta") {
  MotorDuble duble;
  Tocador tocador(duble);
  tocador.junta("uma.wav");

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
  tocador.junta("uma.wav");

  CHECK(tocador.tocar_corrente());
  duble.avanca(1.0);
  tocador.pulsa();
  CHECK(tocador.estado() == Estado::Tocando);
}

TEST_CASE("o volume apara-se, e a faixa nova o herda") {
  MotorDuble duble;
  Tocador tocador(duble);
  tocador.junta("uma.wav");

  CHECK(tocador.volume(140));
  CHECK(tocador.volume() == 100);
  CHECK(duble.volume_recebido == 100);

  CHECK(tocador.volume(-5));
  CHECK(tocador.volume() == 0);
  CHECK(duble.volume_recebido == 0);

  duble.volume_recebido = -1;
  CHECK(tocador.tocar_corrente());
  CHECK(duble.volume_recebido == 0);  // a faixa nova herdou o volume corrente
}

// ── O MUDO (issue #106) ─────────────────────────────────────────────────────
// Cala o MOTOR, e não o volume: é d'ahi que desmudar devolve EXACTAMENTE o que
// havia, e não um numero que se lhe aproxime. A prova interroga o dublê, que é
// quem sabe o que se mandou, e não sómente o que o tocador devolve.
TEST_CASE("o mudo cala o motor, e desmudar devolve o volume exacto") {
  MotorDuble duble;
  Tocador tocador(duble);
  tocador.junta("uma.wav");
  CHECK(tocador.volume(37));

  CHECK(tocador.alterna_mudo());
  CHECK(tocador.mudo());
  CHECK(tocador.retracto().mudo);
  CHECK(duble.volume_recebido == 0);  // o motor cala-se
  CHECK(tocador.volume() == 37);      // e o volume fica onde estava

  // A faixa nova nasce CALADA: sem isto, o F8 desfazia o F9 sem ninguem lh'o
  // pedir, e o operador ouvia a seguinte no volume que mandara calar.
  duble.volume_recebido = -1;
  CHECK(tocador.tocar_corrente());
  CHECK(duble.volume_recebido == 0);

  CHECK_FALSE(tocador.alterna_mudo());
  CHECK_FALSE(tocador.retracto().mudo);
  CHECK(duble.volume_recebido == 37);
}

// O F11 SOBRE O MUDO: pedir volume DESMUDA, e é isto que faz a tecla desmudar e
// subir sem ramo proprio na taboada da tela.
TEST_CASE("pedir volume desmuda, que é o F11 sobre o mudo") {
  MotorDuble duble;
  Tocador tocador(duble);
  CHECK(tocador.volume(60));
  CHECK(tocador.alterna_mudo());
  CHECK(tocador.volume(65));  // o degrau de cinco que a taboada da tela conta
  CHECK_FALSE(tocador.mudo());
  CHECK(tocador.volume() == 65);
  CHECK(duble.volume_recebido == 65);
}

TEST_CASE("buscar apara-se pela duração, e recusa-se parado") {
  MotorDuble duble;
  duble.duracao_dita = 5.0;
  Tocador tocador(duble);
  tocador.junta("uma.wav");

  CHECK_FALSE(tocador.buscar(2.0));  // parado, nada ha que buscar
  CHECK(duble.alvo_buscado == doctest::Approx(-1.0));

  CHECK(tocador.tocar_corrente());
  CHECK(tocador.buscar(99.0));
  CHECK(duble.alvo_buscado == doctest::Approx(4.95));
  CHECK(tocador.buscar(-3.0));
  CHECK(duble.alvo_buscado == doctest::Approx(0.0));
}

// Nenhum pregão ha de levar meio retracto novo e meio velho. Aqui a faixa já
// passou dos dous segundos e meio, logo «Tocando com posição zero» não pode ser
// verdade, e «Parado com posição andada» tambem não: qualquer dos dous denuncia
// pregão emittido pelo meio do assentamento.
TEST_CASE("ao fim natural da faixa, pregão algum sahe com retracto composto") {
  MotorDuble duble;
  Tocador tocador(duble);
  tocador.junta("uma.wav");
  CHECK(tocador.tocar_corrente());
  duble.avanca(2.5);
  tocador.pulsa();  // antes de escutar: assenta a posição em dous e meio

  int compostos = 0;
  int estados = 0;
  int posicoes = 0;
  tocador.escuta([&](const mysong::nucleo::Evento& evento) {
    const bool parado_mas_andado =
        evento.estado == Estado::Parado && evento.posicao != 0.0;
    const bool tocando_mas_no_zero =
        evento.estado == Estado::Tocando && evento.posicao == 0.0;
    if (parado_mas_andado || tocando_mas_no_zero) ++compostos;
    if (evento.aviso == Aviso::EstadoMudou) ++estados;
    if (evento.aviso == Aviso::PosicaoAndou) ++posicoes;
  });

  duble.acaba_na_proxima_batida();
  tocador.pulsa();

  CHECK(tocador.estado() == Estado::Parado);
  CHECK(tocador.posicao() == doctest::Approx(0.0));
  CHECK(estados == 1);
  CHECK(posicoes == 1);
  CHECK(compostos == 0);
}

// ── OS DOUS MODOS (issue #62) ────────────────────────────────────────────────
// A tecla `x` cicla por tres valores, e é o TOCADOR que cicla: a tela sómente
// pede o verbo. Aferir a volta inteira, e não um passo só, é o que apanha o
// ciclo que anda mas não fecha.
TEST_CASE("o repetir cicla por nenhuma, uma, todas e torna ao principio") {
  using mysong::nucleo::Repeticao;
  MotorDuble duble;
  Tocador tocador(duble);
  CHECK(tocador.retracto().repeticao == Repeticao::Nenhuma);
  CHECK(tocador.cicla_repetir() == Repeticao::Uma);
  CHECK(tocador.retracto().repeticao == Repeticao::Uma);
  CHECK(tocador.cicla_repetir() == Repeticao::Todas);
  CHECK(tocador.cicla_repetir() == Repeticao::Nenhuma);
  CHECK(tocador.retracto().repeticao == Repeticao::Nenhuma);
  // E o punho que assenta o valor directo, que é o do socket e o do barramento.
  tocador.repetir(Repeticao::Todas);
  CHECK(tocador.retracto().repeticao == Repeticao::Todas);
}

TEST_CASE("o embaralhar alterna, e o retracto o diz do mesmo momento") {
  MotorDuble duble;
  Tocador tocador(duble);
  for (const char* faixa : {"uma.wav", "duas.wav"}) tocador.junta(faixa);
  CHECK_FALSE(tocador.retracto().embaralhado);
  CHECK(tocador.alterna_embaralhar());
  CHECK(tocador.retracto().embaralhado);
  CHECK_FALSE(tocador.alterna_embaralhar());
  CHECK_FALSE(tocador.retracto().embaralhado);
  tocador.embaralhar(true);
  CHECK(tocador.retracto().embaralhado);
}

// ══════════════════════════════════════════════════════════════════════════
