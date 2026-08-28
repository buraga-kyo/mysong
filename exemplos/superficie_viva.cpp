// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA SUPERFÍCIE VIVA — exemplos/superficie_viva.cpp
// ══════════════════════════════════════════════════════════════════════════
// O binario com que o aceite d'esta issue se prova, á maneira do toca_tom que a
// issue #3 deixou. Ergue o motor de carne, o tocador e o socket de commando, e bate
// os dous num laço só. NÃO linka FTXUI, e é parte do aceite que não linke: a
// superfície de commando não conhece a TUI, e um binario que a carregasse deixaria
// a affirmação sem prova mecanica.
//
// Por que existe, e vale dizê-lo: o binario «mysong» de hoje NÃO toca. O seu
// erguer_tocador() pinta a marca e espera a tecla q, e Tocador algum se constroe
// ahi; ligar a tela ao nucleo é a issue #7. Donde «com o mysong tocando», que é como
// o aceite está escripto, não tem hoje onde se dar, e prova-se aqui.
//
// DOMÍNIO ......... os caminhos de faixa que vierem na linha de commando.
// CONTRA-DOMÍNIO .. som na saída de áudio, e um socket que responde.
// INVARIANTE ...... UMA linha de execução, e uma só. O tocador não é seguro a
//                   threads, e o socket bate na MESMA linha que elle: ordem alguma
//                   se intercala no meio de uma transição do nucleo, e a garantia é
//                   estructural e não vigilancia.
// Q.E.D. .......... o que falta degrada e não aborta: sem faixa alguma, o socket
//                   sobe e responde; sem socket, o som toca. Cada falta escreve a
//                   sua razão no stderr, e nenhuma d'ellas cala.
// ══════════════════════════════════════════════════════════════════════════
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>

#include <ctime>

#include "api/socket.hpp"
#include "nucleo/analisador.hpp"
#include "nucleo/biblioteca.hpp"
#include "nucleo/estaleiro.hpp"
#include "nucleo/motor.hpp"
#include "nucleo/tocador.hpp"

namespace {

// posto — o caminho que a variavel de ambiente disser, e vazio não a havendo.
std::filesystem::path posto(const char* nome) {
  const char* valor = std::getenv(nome);
  if (valor == nullptr || valor[0] == '\0') return {};
  return std::filesystem::path(valor);
}

// A bandeira do sinal. Volatil e de typo atomico do C, que é o que se pode tocar
// dentro de um manejador de sinal sem entrar em terreno que a norma não define.
volatile std::sig_atomic_t pedido_de_sahida = 0;

extern "C" void ao_sinal(int) { pedido_de_sahida = 1; }

}  // namespace

int main(int argc, char** argv) {
  namespace api = mysong::api;
  namespace nucleo = mysong::nucleo;

  std::string razao;
  auto motor = nucleo::MotorMpv::abrir(&razao);
  if (!motor) {
    std::cerr << "superficie_viva: a libmpv recusou: " << razao << "\n";
    return 1;
  }
  nucleo::Tocador tocador(*motor);
  for (int passo = 1; passo < argc; ++passo) tocador.junta(argv[passo]);

  // O ANALISADOR, pelo mesmo padrão da janella: espectro é ornamento, e não
  // requisito. Sem elle o tocador toca e as bandas sahem em zero; mas sem elle
  // aqui, o verbo «espectro» d'este socket JAMAIS mostraria banda que se movesse,
  // e o aceite não teria onde se dar.
  nucleo::Analisador analisador;
  if (analisador.vivo())
    tocador.observa(analisador);
  else
    std::cerr << "superficie_viva: sem espectro: " << analisador.razao() << "\n";

  // O socket DEGRADA e não aborta: faltando-lhe caminho ou estando o caminho tomado,
  // escreve-se a razão e o tocador segue a tocar. Superfície de commando que
  // derrubasse o tocador seria pior que superfície nenhuma.
  // O ÍNDICE e o ACERVO entram por variavel de ambiente, e este exemplo NÃO os
  // redescobre: quem sabe onde elles moram é a janella, e uma segunda descoberta
  // aqui seria uma segunda verdade, que a que se corrigisse deixava a outra a
  // errar. Faltando cada uma, diz-se a razão e o servidor sobe sem aquella peça:
  // o protocolo sabe responder á falta, e é ahi que ella se prova.
  api::Arredores arredores;
  std::optional<nucleo::Biblioteca> livraria;
  const std::filesystem::path indice = posto("MYSONG_INDICE");
  if (indice.empty()) {
    std::cerr << "superficie_viva: SEM indice de acervo: defina MYSONG_INDICE\n";
  } else {
    livraria.emplace(indice);
    arredores.livraria = &*livraria;
  }

  std::optional<nucleo::Estaleiro> estaleiro;
  const std::filesystem::path acervo = posto("MYSONG_ACERVO");
  if (acervo.empty()) {
    std::cerr << "superficie_viva: SEM fila de baixa: defina MYSONG_ACERVO\n";
  } else {
    estaleiro.emplace(nucleo::OBREIROS_DA_BAIXA,
                      [acervo](const nucleo::Pedido& pedido,
                               std::filesystem::path* ficou) {
                        return nucleo::baixa(acervo, pedido, ficou);
                      });
    arredores.estaleiro = &*estaleiro;
  }

  std::string razao_do_socket;
  auto servidor = api::Servidor::abrir(
      tocador, api::caminho_padrao_do_socket(), &razao_do_socket, arredores);
  if (servidor)
    std::cerr << "superficie_viva: socket de commando em " << servidor->caminho()
              << "\n";
  else
    std::cerr << "superficie_viva: SEM socket de commando: " << razao_do_socket << "\n";

  if (tocador.retracto().tamanho > 0 && !tocador.tocar_corrente())
    std::cerr << "superficie_viva: o motor recusou a primeira faixa\n";

  std::signal(SIGINT, ao_sinal);
  std::signal(SIGTERM, ao_sinal);

  // O LAÇO. Bate os dous na MESMA linha de execução, a vinte por segundo. A cadencia
  // é parâmetro d'este laço e não constante enterrada em parte alguma: se um dia
  // faltar, baixa-se aqui, e não se ergue thread, que exigiria mutex no tocador.
  const ::timespec cadencia{0, 50L * 1000L * 1000L};
  while (pedido_de_sahida == 0) {
    tocador.pulsa();
    if (servidor) servidor->pulsa();
    ::nanosleep(&cadencia, nullptr);
  }
  std::cerr << "superficie_viva: sahindo, e o socket vae com o destructor\n";
  return 0;
}
// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
