// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO TOCADOR — src/nucleo/tocador.hpp
// ══════════════════════════════════════════════════════════════════════════
// Casa a potencia com a ordem: toma um Motor emprestado e uma Fila propria, e
// traduz «proxima» em «tocar a faixa que vem depois». É aqui que o estado se
// guarda e que o pregão se faz; o motor não sabe de fila, e a fila não sabe de
// som.
//
// DOMÍNIO ......... um Motor, qualquer que seja a sua carne, e ordens de
//                   operador: tocar, proxima, anterior, pausar, retomar,
//                   buscar, volume.
// CONTRA-DOMÍNIO .. booleano a cada ordem, o estado, e o pregão aos ouvintes.
// INVARIANTE ...... o tocador guarda REFERENCIA ao motor, e não a sua
//                   propriedade: quem o construiu ha de manter o motor vivo
//                   mais tempo que o tocador. Todo evento sahe DEPOIS de o
//                   estado interno já ter mudado, de sorte que o ouvinte nunca
//                   vê um retracto que já não vale.
// Q.E.D. .......... sendo o Motor abstracto, esta classe inteira se prova com
//                   um dublê em machina surda; e sendo o ouvinte um functor
//                   que se registra, o tocador emitte sem conhecer quem ouve.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstddef>
#include <mutex>
#include <string>
#include <vector>

#include "nucleo/analisador.hpp"
#include "nucleo/fila.hpp"
#include "nucleo/motor.hpp"

namespace mysong::nucleo {

// O RETRACTO: o instante inteiro do tocador, colhido debaixo de UMA tomada da
// tranca. Quem pergunta campo a campo colhe cada campo de um momento; quem
// arma tela, pregão de barramento ou assignatura quer o MESMO momento, e é
// para esses que o retracto existe. A faixa vae COPIADA: vista crua da fila
// não atravessa a tranca.
struct Retracto {
  Estado estado = Estado::Parado;
  std::string faixa;      // vazia quando nenhuma faixa está em curso
  double posicao = 0.0;   // em segundos, contados do inicio da faixa
  double duracao = 0.0;
  int volume = 100;
  std::size_t indice = 0;  // 0 tambem em fila vazia: pergunte-se ao tamanho
  std::size_t tamanho = 0;
};

class Tocador {
 public:
  explicit Tocador(Motor& motor) noexcept;

  Tocador(const Tocador&) = delete;
  Tocador& operator=(const Tocador&) = delete;

  // A fila é do tocador, e o cliente a arma por esta porta.
  Fila& fila() noexcept;
  const Fila& fila() const noexcept;

  // Os punhos TRANCADOS da fila, para quem chama com o relogio já a bater.
  // Juntar devolve o tamanho novo, para que «juntei e é o ultimo» não vire
  // duas perguntas com o mundo andando no meio. Nada d'isto desce ao motor:
  // tocar é ordem á parte, como sempre foi.
  std::size_t junta(std::string caminho);
  bool ir_para(std::size_t alvo);
  std::vector<std::string> faixas() const;

  // Registra quem escuta. Zero ouvintes é caso legitimo.
  void escuta(Ouvinte ouvinte);

  // Manda tocar a faixa corrente da fila. Falso em fila vazia.
  bool tocar_corrente();

  // Andam pela fila e mandam tocar a faixa nova. Falso na borda, e ahi NADA se
  // manda ao motor: a faixa que tocava continua a tocar.
  bool proxima();
  bool anterior();

  bool pausar();
  bool retomar();
  bool buscar(double segundos);
  bool volume(int porcento);

  Estado estado() const noexcept;
  int volume() const noexcept;
  double posicao() const;
  double duracao() const;

  // O instante inteiro, de uma tomada só. Vide o tractado do Retracto acima.
  Retracto retracto() const;

  // Uma batida do relogio: drena o motor e annuncia o que se moveu. Chama-se
  // de fóra, na cadencia de quem chama.
  void pulsa();

  // ── O ESPECTRO (issue #5), por ACRÉSCIMO. A fonte é emprestada e ANNULAVEL:
  // o nucleo existe sem analisador, e sem elle as bandas sahem em zero. O
  // tocador não sabe o que ha dentro d'ella: não conhece transformada, não
  // conhece PipeWire, e é por esta fronteira que a fachada serve o mesmo dado a
  // quem pergunte, sem que quem pergunte conheça o analisador.
  void observa(FonteDeBandas& fonte) noexcept;
  std::vector<float> bandas() const;

 private:
  void annuncia(Aviso aviso, std::string razao = {});
  void assenta_estado(Estado novo);
  bool tocar_corrente_trancado();

  // A TRANCA (issue #50): todo punho publico a toma, e é ella que faz o
  // tocador chamavel de mais de um fio sem corrida. O pregão sahe com ella
  // tomada, donde o contracto: ouvinte NÃO chama o tocador de volta, que a
  // tranca não é reentrante e a segunda tomada seria abraço de si mesma.
  mutable std::mutex tranca_;
  Motor& motor_;
  Fila fila_;
  std::vector<Ouvinte> ouvintes_;
  Estado estado_ = Estado::Parado;
  int volume_ = 100;
  double ultima_posicao_ = 0.0;
  FonteDeBandas* fonte_ = nullptr;  // emprestada, e nullo é caso legitimo
};

}  // namespace mysong::nucleo

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
