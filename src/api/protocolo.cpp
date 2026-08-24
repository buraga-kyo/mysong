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
// AS FAIXAS DA FILA. A Fila não tem porta que devolva a faixa de um indice
// qualquer: tem «corrente()» e tem «ir_para()». Alargá-la seria editar
// src/nucleo/fila.hpp, e a tarefa irmã reescreve o nucleo agora. Donde se anda
// pela fila e se RESTAURA o assento no fim. E anda-se na FILA, e não no TOCADOR:
// Fila::ir_para move o indice e nada desce ao motor, de sorte que este passeio
// não toca em som algum nem se ouve de fóra.
std::vector<std::string> faixas_da_fila(Tocador& tocador) {
  nucleo::Fila& fila = tocador.fila();
  std::vector<std::string> obra;
  if (fila.vazia()) return obra;
  const std::size_t assento = fila.indice();
  obra.reserve(fila.tamanho());
  for (std::size_t passo = 0; passo < fila.tamanho(); ++passo) {
    fila.ir_para(passo);
    obra.emplace_back(fila.corrente());
  }
  fila.ir_para(assento);
  return obra;
}

// «nao_implementado» é nome que a Casa TEM e cujo subsystema ainda não chegou. A
// issue vae na resposta, para que o implementador do outro lado saiba ONDE
// procurar quando aquillo passar a funccionar, em vez de ficar a supor se errou o
// nome ou se a feição não veio.
std::string reservado(std::string_view verbo, int issue) {
  Objecto obra;
  obra.par("ok", booleano(false));
  obra.par("erro", texto("nao_implementado"));
  obra.par("razao", texto("o verbo \"" + std::string(verbo) +
                          "\" esta reservado e o seu subsystema ainda nao existe"));
  obra.par("issue", inteiro(issue));
  return obra.fecha();
}
}  // namespace
}  // namespace mysong::api
// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
