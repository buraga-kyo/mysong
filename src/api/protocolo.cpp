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
//                   «indisponivel» é nome que a Casa tem, com o subsystema em
//                   pé, e que ESTA instancia do servidor não ergueu; e
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
// Sómente pelas CONSTANTES da escala. Este cabeçalho declara o plano da fftw
// adiante e não arrasta a fftw3 consigo, do mesmo modo que a prova da
// mathematica já o inclue sem os directorios de inclusão d'ella.
#include "nucleo/espectro.hpp"
#include "nucleo/biblioteca.hpp"
#include "nucleo/estaleiro.hpp"

namespace mysong::api {
namespace {

using nucleo::Tocador;

// O TECTO da fila de baixa, e a razão de elle morar AQUI. A fila do nucleo é uma
// deque sem limite, e a tecla da tela enche-a de uma em uma, á velocidade de quem
// digita. O socket não: um cliente encommenda mil por segundo, e a memoria cresce
// até acabar. É a mesma guarda que este transporte já tem no tamanho da linha e
// no numero de clientes, agora numa terceira frente. Sessenta e quatro, e não
// dez: com dous obreiros e baixa de minutos, sessenta e quatro á espera já são
// horas de fila, e quem pede mais que isso não está a pedir musica.
constexpr std::size_t kTectoDaFilaDeBaixa = 64;

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

// O RETRACTO. Sete campos, e os sete SEMPRE, colhidos de UMA tomada da tranca
// do tocador: cliente que tenha de perguntar duas vezes para armar uma tela é
// cliente que verá a segunda resposta não casar com a primeira, porque entre
// as duas o mundo andou.
std::string retracto(Tocador& tocador) {
  const nucleo::Retracto agora = tocador.retracto();
  Objecto obra = abre_acerto();
  obra.par("estado", texto(nucleo::nome_do_estado(agora.estado)));
  obra.par("faixa", texto(agora.faixa));
  obra.par("posicao", duplo(agora.posicao));
  obra.par("duracao", duplo(agora.duracao));
  obra.par("volume", inteiro(agora.volume));
  obra.par("indice", inteiro(static_cast<long long>(agora.indice)));
  obra.par("tamanho", inteiro(static_cast<long long>(agora.tamanho)));
  return obra.fecha();
}
// AS FAIXAS DA FILA, pela copia trancada do tocador. A issue #50 aposentou o
// passeio que andava com ir_para e restaurava o assento: era mexida onde se
// queria leitura, e mexida sem tranca com o relogio a bater n'outro fio.
std::vector<std::string> faixas_da_fila(Tocador& tocador) {
  return tocador.faixas();
}

// «indisponivel» é a peça que EXISTE na obra e que ESTA INSTANCIA do servidor
// não ergueu: quem abriu o socket sem índice de acervo, ou sem fila de baixa.
// Não é «nao_implementado», que dizia «a feição não existe em parte alguma» e
// passou a ser mentira quando as issues #5, #8 e #11 fecharam; nem é «recusado»,
// que manda olhar o ESTADO do nucleo, quando aqui não ha estado que mudar de
// ordem para ordem. O remedio é de quem ERGUEU o servidor, e não de quem manda a
// ordem, e é por isso que leva codigo proprio.
std::string indisponivel(std::string_view peca) {
  return erro("indisponivel",
              "esta instancia do servidor nao ergueu " + std::string(peca));
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
  return responde(tocador, Arredores{}, linha);
}

std::string responde(Tocador& tocador, const Arredores& arredores,
                     std::string_view linha) {
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
    obra.par("indice",
             inteiro(static_cast<long long>(tocador.retracto().indice)));
    obra.par("tamanho", inteiro(static_cast<long long>(faixas.size())));
    return obra.fecha();
  }
  // O ESPECTRO (issue #5), pelo mesmo punho que a tela usa. Vae a ESCALA junto
  // com as bandas, e não sómente ellas: quem lê de fóra não tem tela para
  // adivinhar que as bordas se espaçam em logarithmo entre 40 e 16000 Hz, nem
  // que a magnitude vem comprimida em decibeis com o piso a valer zero. Bandas
  // sem escala são vinte e quatro numeros que o cliente não sabe pintar.
  //
  // Sem fonte de bandas, o tocador devolve QUANTAS_BANDAS zeros, e não erro: o
  // silencio é resposta legitima, e a escala vae na mesma.
  if (verbo == "espectro") {
    const std::vector<float> bandas = tocador.bandas();
    Objecto obra = abre_acerto();
    obra.par("bandas", vector_de_duplos(bandas));
    obra.par("quantas", inteiro(static_cast<long long>(bandas.size())));
    obra.par("escala", texto("logarithmica"));
    obra.par("hertz_minimo", duplo(nucleo::HERTZ_MINIMO));
    obra.par("hertz_maximo", duplo(nucleo::HERTZ_MAXIMO));
    obra.par("magnitude", texto("decibeis"));
    obra.par("piso_decibeis", duplo(nucleo::PISO_EM_DECIBEIS));
    return obra.fecha();
  }

  if (verbo == "juntar") {
    const Valor* caminho = argumento(msg, "caminho", Typo::Texto);
    if (caminho == nullptr) return falta("caminho", "texto");
    if (caminho->texto.empty())
      return erro("argumento_invalido", "o caminho da faixa vem vazio");
    const std::size_t tamanho = tocador.junta(caminho->texto);
    Objecto obra = abre_acerto();
    obra.par("tamanho", inteiro(static_cast<long long>(tamanho)));
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
    if (!tocador.ir_para(static_cast<std::size_t>(alvo->numero)))
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

  // Os dous que faltam. O NOME existe e o subsystema tambem, donde a falta é
  // sómente d'esta instancia, e é isso que a resposta diz.
  // A BIBLIOTHECA (issue #8). O CORTE é obrigatorio: tres cortes, e mais nenhum.
  // Deduzi-lo dos argumentos que viessem seria mais curto e falharia em
  // SILENCIO, que é o que esta Casa prohibe: quem digitasse «artistaa» receberia
  // a lista dos artistas com ok verdadeiro e nunca saberia que errou o nome.
  // Índice ausente responde como índice VAZIO, e não como erro: é o que a
  // Bibliotheca já faz com banco que não existe, e de fóra as duas são a mesma.
  if (verbo == "biblioteca") {
    const nucleo::Biblioteca* livraria = arredores.livraria;
    const Valor* corte = argumento(msg, "corte", Typo::Texto);
    if (corte == nullptr) return falta("corte", "texto");
    if (corte->texto != "artistas" && corte->texto != "albuns" &&
        corte->texto != "faixas")
      return erro("argumento_invalido",
                  "o corte \"" + corte->texto +
                      "\" nao existe: ha \"artistas\", \"albuns\" e \"faixas\"");
    if (corte->texto == "artistas") {
      const std::vector<std::string> nomes =
          livraria == nullptr ? std::vector<std::string>{} : livraria->artistas();
      Objecto obra = abre_acerto();
      obra.par("corte", texto("artistas"));
      obra.par("artistas", vector_de_textos(nomes));
      obra.par("tamanho", inteiro(static_cast<long long>(nomes.size())));
      return obra.fecha();
    }
    const Valor* quem = argumento(msg, "artista", Typo::Texto);
    if (quem == nullptr) return falta("artista", "texto");
    if (quem->texto.empty())
      return erro("argumento_invalido", "o nome do artista vem vazio");
    if (corte->texto == "albuns") {
      const std::vector<std::string> nomes =
          livraria == nullptr ? std::vector<std::string>{}
                              : livraria->albuns(quem->texto);
      Objecto obra = abre_acerto();
      obra.par("corte", texto("albuns"));
      obra.par("artista", texto(quem->texto));
      obra.par("albuns", vector_de_textos(nomes));
      obra.par("tamanho", inteiro(static_cast<long long>(nomes.size())));
      return obra.fecha();
    }
    const Valor* qual = argumento(msg, "album", Typo::Texto);
    if (qual == nullptr) return falta("album", "texto");
    if (qual->texto.empty())
      return erro("argumento_invalido", "o nome do album vem vazio");
    // Os QUATRO vectores sahem PARALELOS e do mesmo comprimento, que é o
    // «tamanho»: objecto dentro de objecto sae do subconjunto PLANO do jsonzinho.
    std::vector<nucleo::Faixa> achadas;
    if (livraria != nullptr)
      achadas = livraria->faixas_do_album(quem->texto, qual->texto);
    std::vector<long long> numeros, duracoes;
    std::vector<std::string> titulos, caminhos;
    for (const nucleo::Faixa& faixa : achadas) {
      numeros.push_back(faixa.numero);
      duracoes.push_back(faixa.duracao);
      titulos.push_back(faixa.titulo);
      caminhos.push_back(faixa.caminho);
    }
    Objecto obra = abre_acerto();
    obra.par("corte", texto("faixas"));
    obra.par("artista", texto(quem->texto));
    obra.par("album", texto(qual->texto));
    obra.par("numeros", vector_de_inteiros(numeros));
    obra.par("titulos", vector_de_textos(titulos));
    obra.par("caminhos", vector_de_textos(caminhos));
    obra.par("duracoes", vector_de_inteiros(duracoes));
    obra.par("tamanho", inteiro(static_cast<long long>(achadas.size())));
    return obra.fecha();
  }
  // A BAIXA (issue #11), pela MESMA fila que a tecla «s» da tela usa. NÃO se
  // espera pelo desfecho: o socket responde por linha, e uma baixa leva minutos;
  // esperar por ella prenderia a batida do servidor, e com ella o tocador, que
  // bate na mesma linha de execução. Aceite não é promessa de arquivo, e o que
  // acontecer depois lê-se nas contas que a proxima resposta trouxer.
  if (verbo == "baixar") {
    if (arredores.estaleiro == nullptr) return indisponivel("a fila de baixa");
    const Valor* onde = argumento(msg, "url", Typo::Texto);
    if (onde == nullptr) return falta("url", "texto");
    if (onde->texto.empty())
      return erro("argumento_invalido", "a url da faixa vem vazia");
    const nucleo::Andamento antes = arredores.estaleiro->andamento();
    if (antes.na_espera >= kTectoDaFilaDeBaixa)
      return erro("recusado", "a fila de baixa esta cheia: " +
                                  std::to_string(antes.na_espera) +
                                  " pedidos a espera");
    nucleo::Pedido pedido;
    pedido.url = onde->texto;
    // O que o operador DIZ ganha do que a rede disser, que é a regra que o
    // resolve() da aquisição já lavra. Campo que elle não diga fica vazio.
    if (const Valor* v = argumento(msg, "artista", Typo::Texto))
      pedido.artista = v->texto;
    if (const Valor* v = argumento(msg, "album", Typo::Texto))
      pedido.album = v->texto;
    if (const Valor* v = argumento(msg, "titulo", Typo::Texto))
      pedido.titulo = v->texto;
    if (const Valor* v = argumento(msg, "numero", Typo::Numero)) {
      if (!(v->numero >= 0.0 && v->numero <= 9999.0))
        return erro("argumento_invalido", "o numero da faixa esta fora de faixa");
      pedido.numero = static_cast<int>(v->numero);
    }
    arredores.estaleiro->encommenda(std::move(pedido));
    const nucleo::Andamento agora = arredores.estaleiro->andamento();
    Objecto obra = abre_acerto();
    obra.par("em_curso", inteiro(static_cast<long long>(agora.em_curso)));
    obra.par("na_espera", inteiro(static_cast<long long>(agora.na_espera)));
    obra.par("colhidas", inteiro(static_cast<long long>(agora.colhidas)));
    obra.par("falhadas", inteiro(static_cast<long long>(agora.falhadas)));
    obra.par("duvidosas", inteiro(static_cast<long long>(agora.duvidosas)));
    obra.par("ultima", texto(agora.ultima));
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
