// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DO PROTOCOLO, LAVRA — src/api/protocolo.cpp
// ══════════════════════════════════════════════════════════════════════════
// Cumpre o cabecalho. Não inclue socket algum, nem cabecalho de systema: fala
// com o Tocador abstracto e com o jsonzinho, e é d'ahi que lhe vem a inteira
// provabilidade em machina surda.
//
// DOMÍNIO ......... a Mensagem já lida, e o Tocador emprestado.
// CONTRA-DOMÍNIO .. uma linha de JSON, sempre.
// INVARIANTE ...... tres recusas, e tres codigos que NÃO se confundem:
//                   «verbo_desconhecido» é nome que a Casa não tem;
//                   «nao_implementado» é nome que a Casa TEM e cujo subsystema
//                   ainda não chegou, e vae com a issue que o trará; e
//                   «recusado» é o nucleo a dizer não a ordem legitima. Quem
//                   depura do outro lado precisa de as distinguir, porque o
//                   remedio de cada uma é differente.
// Q.E.D. .......... nenhuma linha d'esta unidade nomeia AF_UNIX, poll ou
//                   descriptor; logo o dublê a exercita inteira.
// ══════════════════════════════════════════════════════════════════════════
#include "api/protocolo.hpp"

#include <utility>
#include <vector>

#include "api/jsonzinho.hpp"

namespace mysong::api {
namespace {

using nucleo::Tocador;

// A moldura do ACERTO abre-se sempre por «ok», que é o que o cliente lê primeiro.
Objecto abre_acerto() {
  Objecto obra;
  obra.par("ok", booleano(true));
  return obra;
}

std::string feito() { return abre_acerto().fecha(); }

// A moldura do ERRO. Codigo para a machina, razão para o olho humano, e as duas
// SEMPRE: codigo sem razão manda depurar por adivinhação, e razão sem codigo
// manda o cliente comparar cadeias de texto que a proxima versão mudará.
std::string erro(std::string_view codigo, const std::string& razao) {
  Objecto obra;
  obra.par("ok", booleano(false));
  obra.par("erro", texto(codigo));
  obra.par("razao", texto(razao));
  return obra.fecha();
}

// «recusado» é o NUCLEO a dizer não a ordem legitima: pausar o que está parado,
// proxima na borda da fila. NÃO é a mensagem estar errada, que para isso ha
// «argumento_invalido»; nem o verbo não existir, que para isso ha os outros dous.
std::string recusado(std::string_view ordem) {
  return erro("recusado", "o nucleo recusou a ordem \"" + std::string(ordem) +
                              "\" no estado corrente");
}

std::string conforme(bool foi, std::string_view ordem) {
  return foi ? feito() : recusado(ordem);
}

// O RETRACTO. Sete campos, e os sete SEMPRE: cliente que tenha de perguntar duas
// vezes para armar uma tela é cliente que verá a segunda resposta não casar com a
// primeira, porque entre as duas o mundo andou.
std::string retracto(Tocador& tocador) {
  Objecto obra = abre_acerto();
  obra.par("estado", texto(nucleo::nome_do_estado(tocador.estado())));
  obra.par("faixa", texto(tocador.fila().corrente()));
  obra.par("posicao", duplo(tocador.posicao()));
  obra.par("duracao", duplo(tocador.duracao()));
  obra.par("volume", inteiro(tocador.volume()));
  obra.par("indice", inteiro(static_cast<long long>(tocador.fila().indice())));
  obra.par("tamanho", inteiro(static_cast<long long>(tocador.fila().tamanho())));
  return obra.fecha();
}
}  // namespace
}  // namespace mysong::api
// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
