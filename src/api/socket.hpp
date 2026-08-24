// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO SOCKET — src/api/socket.hpp
// ══════════════════════════════════════════════════════════════════════════
// O BRAÇO do protocolo: o transporte, e sómente elle. Não sabe que verbos ha,
// nem que a resposta é JSON; sabe abrir uma porta no systema de arquivos, colher
// linhas d'ella e devolver o que o protocolo disser. Nasce SÓ pela fabrica, á
// maneira do MotorMpv: quem não conseguiu abrir não tem objecto, e não um objecto
// a que se deva perguntar se serve.
//
// DOMÍNIO ......... um caminho no systema de arquivos, INJECTAVEL de proposito; e
//                   um Tocador emprestado, a quem as linhas se dirigem.
// CONTRA-DOMÍNIO .. um socket de fluxo AF_UNIX vivo, e uma linha de resposta por
//                   cada linha de pergunta.
// INVARIANTE ...... o arquivo do socket some por QUALQUER caminho de sahida, e
//                   excepção inclusa: quem o desliga é o destructor, e não um
//                   trecho de limpeza que cada retorno tenha de lembrar. E linha
//                   de execução propria NÃO se cria: pulsa() corre na mesma linha
//                   que Tocador::pulsa(), de sorte que ordem alguma se intercala
//                   no meio de uma transição do nucleo.
// Q.E.D. .......... sendo o caminho injectavel, cada caso de prova abre o SEU
//                   socket num directorio temporario proprio, donde dous casos
//                   não colidem nem se envenenam; e sendo o cerebro uma funcção
//                   pura que mora no tractado do protocolo, o que aqui se prova é
//                   sómente o transporte, e são cinco casos, não trinta.
// ══════════════════════════════════════════════════════════════════════════
#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include "nucleo/tocador.hpp"

namespace mysong::api {

// O caminho de fabrica: $XDG_RUNTIME_DIR/mysong.sock. VAZIO quando a variavel não
// está definida, e recuo algum a /tmp, que é escripta de todos: socket de
// commando ahi deixaria qualquer usuario da machina governar o tocador alheio.
std::string caminho_padrao_do_socket();

}  // namespace mysong::api

class Servidor {
 public:
  // Teto de clientes ao mesmo tempo. Além d'elle responde-se «lotado» e fecha-se,
  // e não se deixa de aceitar em silêncio.
  static constexpr std::size_t kTetoDeClientes = 16;
  // Teto de UMA linha sem o \n que a termina. Cliente mudo não ha de crescer
  // memoria sem fim, e cortar com codigo dito é melhor que crescer até morrer.
  static constexpr std::size_t kTetoDaLinha = 64u * 1024u;

  // Vazio se não se pudo abrir; a razão, se se pedir, sahe legivel por olho. Não
  // ha construtor publico: servidor invalido não é exprimivel.
  static std::optional<Servidor> abrir(nucleo::Tocador& tocador,
                                       const std::string& caminho,
                                       std::string* razao = nullptr);

  ~Servidor();
  Servidor(const Servidor&) = delete;
  Servidor& operator=(const Servidor&) = delete;
  // Move consentido, e sómente elle: é o que a fabrica precisa para devolver.
  Servidor(Servidor&& outro) noexcept;
  Servidor& operator=(Servidor&&) = delete;

  // Uma batida: aceita quem chegou, colhe as linhas prontas, responde, e escoa o
  // que ficou por escrever. NÃO bloqueia em cousa alguma, e por isso se chama da
  // mesma linha de execução que bate o tocador.
  void pulsa();

  const std::string& caminho() const noexcept;

 private:
  struct Cliente {
    int fd = -1;
    std::string entrada;  // o que chegou e ainda não fez linha
    std::string sahida;   // o que se deve escrever e ainda não cabeu
  };

  Servidor(nucleo::Tocador& tocador, int escuta, std::string caminho) noexcept;
  void aceita();
  void colhe(Cliente& cliente);
  void escoa(Cliente& cliente);
  void encerra(Cliente& cliente) noexcept;

  nucleo::Tocador* tocador_ = nullptr;
  int escuta_ = -1;
  std::string caminho_;
  std::vector<Cliente> clientes_;
};
// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
