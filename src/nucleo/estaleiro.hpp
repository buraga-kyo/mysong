// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO ESTALEIRO — src/nucleo/estaleiro.hpp
// ══════════════════════════════════════════════════════════════════════════
// A FILA DAS BAIXAS, com limite declarado. Baixar é lento e é de rede, e o
// operador que elege cinco faixas de seguida não ha de esperar pela primeira
// para encommendar a segunda. Mas tambem não se abrem cinco processos de yt-dlp
// ao mesmo tempo: a rede é uma, e cinco a disputá-la acabam todas mais tarde do
// que duas acabariam.
//
// DOMÍNIO ......... encommendas (Pedido), e a OBRA que as cumpre, que entra por
//                   parametro e não por chamada directa.
// CONTRA-DOMÍNIO .. o andamento: quantas correm, quantas esperam, quantas
//                   colheram, quantas falharam, e a razão da ultima.
// INVARIANTE ...... nunca correm mais que `obreiros` obras ao mesmo tempo, e o
//                   PICO fica registrado para que a promessa se possa aferir em
//                   vez de se acreditar.
// Q.E.D. .......... entrando a obra por parametro, a bateria põe no logar d'ella
//                   uma obra de mentira que se deixa segurar, e afere o limite
//                   sem tocar a rede e sem esperar por relogio.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <filesystem>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>  // os obreiros guardam-se n'um vector, e juntam-se no fim

#include "nucleo/aquisicao.hpp"

namespace mysong::nucleo {

// O LIMITE. Dous, e não um: um serializa, e a segunda baixa esperaria pela
// primeira inteira. Dous, e não cinco: a rede é uma só.
inline constexpr std::size_t OBREIROS_DA_BAIXA = 2;

// O ANDAMENTO n'um instante. Cópia, e não punho para dentro: quem pergunta lê um
// retracto coherente, e não campos colhidos em instantes differentes.
struct Andamento {
  std::size_t em_curso = 0;
  std::size_t na_espera = 0;
  std::size_t colhidas = 0;
  std::size_t falhadas = 0;
  std::string ultima;  // a razão do ultimo desfecho; vazia quando nada acabou
};

// texto_do_andamento — a linha que a tela mostra. Funcção PURA, e por isso vive
// aqui e não na janella. Estaleiro quieto e sem historia devolve cadeia VAZIA:
// é o que faz a tela calar-se em vez de mostrar «0 a baixar».
std::string texto_do_andamento(const Andamento& andamento);

// O ESTALEIRO. Ergue `obreiros` fios que consomem a fila, e cada um chama a OBRA,
// que entra por parametro: é essa juncta que deixa a bateria pôr no logar d'ella
// uma obra que se possa segurar.
class Estaleiro {
 public:
  using Obra = std::function<Colheita(const Pedido&, std::filesystem::path*)>;

  Estaleiro(std::size_t obreiros, Obra obra);
  ~Estaleiro();  // fecha, e espera os obreiros: fio solto não sahe d'aqui

  Estaleiro(const Estaleiro&) = delete;
  Estaleiro& operator=(const Estaleiro&) = delete;

  void encommenda(Pedido pedido);
  Andamento andamento() const;

  // colheu — CONSOME a bandeira de «entrou faixa nova no disco». Sem o consumo, a
  // tela varreria o disco a cada quadro para sempre.
  bool colheu();

  // pico — o maior numero de obras simultaneas de toda a vida do estaleiro. Existe
  // para que o limite se AFIRA, e não se acredite.
  std::size_t pico() const;

  // espera_a_fila — bloqueia até a fila esvaziar E as obras em voo acabarem. NÃO
  // fecha o estaleiro: elle continua a aceitar encommenda depois. Existe porque
  // fecha() ABANDONA a espera, donde encommendar e fechar em seguida perderia obra;
  // quem quer o desfecho de todas pede este, e sómente depois fecha.
  void espera_a_fila();

  void fecha();  // pára de aceitar, acorda os obreiros e espera-os

  // fechado — verdadeiro depois de fecha() ter tomado a tranca. Existe para que o
  // abandono da espera se possa AFERIR: quem prova espera por este punho, e sómente
  // então solta a obra em voo, donde a ordem dos passos deixa de ser sorte.
  bool fechado() const;

 private:
  void obreiro();

  mutable std::mutex tranca_;
  std::condition_variable sino_;
  std::deque<Pedido> espera_;
  std::vector<std::thread> obreiros_;
  Obra obra_;
  std::size_t em_curso_ = 0;
  std::size_t pico_ = 0;
  std::size_t colhidas_ = 0;
  std::size_t falhadas_ = 0;
  std::string ultima_;
  bool colheu_ = false;
  bool fechado_ = false;
};

}  // namespace mysong::nucleo

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
