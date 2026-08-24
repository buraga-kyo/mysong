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
// OS ARGUMENTOS. Argumento ausente ou de typo errado é «argumento_invalido», e
// jamais «recusado». A differença importa a quem depura do outro lado, e importa
// muito: «recusado» manda olhar o ESTADO do tocador, e «argumento_invalido» manda
// olhar a MENSAGEM que se escreveu. Confundi-los manda o cliente procurar o
// defeito no logar errado.
const Valor* argumento(const Mensagem& msg, std::string_view nome, Typo typo) {
  const Valor* achado = msg.acha(std::string(nome));
  return (achado != nullptr && achado->typo == typo) ? achado : nullptr;
}

std::string falta(std::string_view nome, std::string_view typo) {
  return erro("argumento_invalido",
              "o argumento \"" + std::string(nome) + "\" falta ou nao e do typo " +
                  std::string(typo));
}
}  // namespace
std::string responde(Tocador& tocador, std::string_view linha) {
  // Linha em branco não é pergunta e não é erro: nada se responde a ella. É o
  // UNICO caminho d'esta obra que devolve cadeia vazia, e é por isso que o
  // cliente que abra e feche sem falar não recebe erro algum.
  if (linha.find_first_not_of(" \t\r\n") == std::string_view::npos) return {};

  const Mensagem msg = analysa(linha);
  if (!msg.valida) return erro("json_malformado", msg.razao);

  const Valor* nome = argumento(msg, "verbo", Typo::Texto);
  if (nome == nullptr)
    return erro("verbo_ausente",
                "a mensagem nao traz a chave \"verbo\" com valor de texto");
  const std::string& verbo = nome->texto;

  // O CONTRACTO, para que o outro lado o possa exigir antes de confiar.
  if (verbo == "versao") {
    Objecto obra = abre_acerto();
    obra.par("obra", texto("mysong"));
    obra.par("protocolo", inteiro(kVersaoDoProtocolo));
    return obra.fecha();
  }
  if (verbo == "estado") return retracto(tocador);

  if (verbo == "fila") {
    const std::vector<std::string> faixas = faixas_da_fila(tocador);
    Objecto obra = abre_acerto();
    obra.par("faixas", vector_de_textos(faixas));
    obra.par("indice", inteiro(static_cast<long long>(tocador.fila().indice())));
    obra.par("tamanho", inteiro(static_cast<long long>(faixas.size())));
    return obra.fecha();
  }
  if (verbo == "juntar") {
    const Valor* caminho = argumento(msg, "caminho", Typo::Texto);
    if (caminho == nullptr) return falta("caminho", "texto");
    if (caminho->texto.empty())
      return erro("argumento_invalido", "o caminho da faixa vem vazio");
    tocador.fila().junta(caminho->texto);
    Objecto obra = abre_acerto();
    obra.par("tamanho", inteiro(static_cast<long long>(tocador.fila().tamanho())));
    return obra.fecha();
  }
  // «ir_para» move E manda tocar, á maneira de «proxima» e «anterior»: mover sem
  // tocar deixaria o tocador a soar a faixa velha com o indice na nova, que é
  // estado composto que nenhum cliente saberia ler.
  if (verbo == "ir_para") {
    const Valor* alvo = argumento(msg, "indice", Typo::Numero);
    if (alvo == nullptr) return falta("indice", "numero");
    if (!(alvo->numero >= 0.0 && alvo->numero < 1e9))
      return erro("argumento_invalido", "o indice esta fora de faixa razoavel");
    if (!tocador.fila().ir_para(static_cast<std::size_t>(alvo->numero)))
      return recusado("ir_para");
    return conforme(tocador.tocar_corrente(), "ir_para");
  }

  if (verbo == "tocar")    return conforme(tocador.tocar_corrente(), "tocar");
  if (verbo == "pausar")   return conforme(tocador.pausar(), "pausar");
  if (verbo == "retomar")  return conforme(tocador.retomar(), "retomar");
  if (verbo == "proxima")  return conforme(tocador.proxima(), "proxima");
  if (verbo == "anterior") return conforme(tocador.anterior(), "anterior");
  // «parar» PAUSA, hoje. O nucleo não tem parada distincta da pausa, e alargar a
  // sua interface seria editar src/nucleo/tocador.hpp, que a tarefa irmã está a
  // reescrever. Das tres sahidas — não ter o verbo, mentir que para, ou tê-lo com
  // o effeito que ha e dizê-lo — esta é a unica que não deixa o cliente a crer em
  // cousa falsa. O documento o declara com estas palavras; e o NOME do verbo já é
  // o certo, donde quando Tocador::parar() existir muda-se esta linha e o cliente
  // do outro lado não muda uma letra.
  if (verbo == "parar")    return conforme(tocador.pausar(), "parar");

  if (verbo == "buscar") {
    const Valor* alvo = argumento(msg, "segundos", Typo::Numero);
    if (alvo == nullptr) return falta("segundos", "numero");
    // O aparo pelas bordas da faixa mora no tractado do motor, em fonte unica, e
    // o Tocador o applica: não se repete aqui o que já está lavrado lá.
    return conforme(tocador.buscar(alvo->numero), "buscar");
  }
  if (verbo == "volume") {
    const Valor* alvo = argumento(msg, "porcento", Typo::Numero);
    if (alvo == nullptr) return falta("porcento", "numero");
    const double bruto = alvo->numero;
    const int pedido = bruto < 0.0 ? 0 : (bruto > 100.0 ? 100 : static_cast<int>(bruto));
    if (!tocador.volume(pedido)) return recusado("volume");
    // Responde-se o volume APARADO, e não o que se pediu: quem mandou 150 há de
    // ler 100, em vez de ficar a crer que assentou 150 e a estranhar o som.
    Objecto obra = abre_acerto();
    obra.par("volume", inteiro(tocador.volume()));
    return obra.fecha();
  }

  return erro("verbo_desconhecido",
              "esta Casa nao conhece o verbo \"" + verbo + "\"");
}
}  // namespace mysong::api
// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
