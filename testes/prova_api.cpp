// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA PROVA DA API, BANDA PURA — testes/prova_api.cpp
// ══════════════════════════════════════════════════════════════════════════
// Prova o jsonzinho e o protocolo em MACHINA SURDA: sem socket, sem barramento,
// sem placa de som e sem arquivo em disco. É onde dezeseis verbos e todo o
// enquadramento se provam de graça; o que de facto precisa de socket mora em
// prova_api_viva.cpp, e são cinco casos, não trinta.
//
// DOMÍNIO ......... cadeias, e sómente cadeias: nomes de faixa que trazem o que
//                   o systema de arquivos permitte, e linhas de mensagem que
//                   podem ser qualquer cousa, inclusive lixo.
// CONTRA-DOMÍNIO .. veredicto do doctest, e por elle o status do ctest.
// INVARIANTE ...... toda resposta d'esta obra é UMA linha. É o que se assere por
//                   find('\n') == npos, e não por inspecção de olho: o
//                   enquadramento de uma-mensagem-por-linha cae inteiro se um
//                   nome de faixa levar mudança de linha crua ao emissor.
// Q.E.D. .......... o dublê aqui NÃO é mais simples que o mundo nas tres cousas
//                   em que o mundo morde: nome de arquivo com aspas e UTF-8, fim
//                   natural da faixa mudando posição E estado na mesma batida, e
//                   ordem que o nucleo recusa. Dublê mais simples que o mundo é
//                   onde o defeito se aloja, e por isso se enumeram as
//                   differenças em vez de se as presumir ausentes.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <string>
#include <system_error>
#include <vector>

#include <unistd.h>

#include "api/jsonzinho.hpp"
#include "api/protocolo.hpp"
#include "nucleo/biblioteca.hpp"
#include "nucleo/estaleiro.hpp"

namespace {
using mysong::api::analysa;
using mysong::api::escapa;
using mysong::api::Mensagem;
using mysong::api::texto;
using mysong::api::Typo;
using mysong::nucleo::Estado;
using mysong::nucleo::Tocador;

// O DUBLÊ, escripto de novo aqui e NÃO emprestado: testes/prova_tocador.cpp é da
// tarefa irmã mysong-5, e extrahi-lo para cabeçalho comum exigiria editá-lo. Vinte
// e cinco linhas duplicadas custam menos que um conflicto em arquivo que outra
// lavra está a reescrever agora.
class MotorDuble final : public mysong::nucleo::Motor {
 public:
  std::vector<std::string> tocados;
  int volume_recebido = -1;
  double alvo_buscado = -1.0;
  double duracao_dita = 10.0;

  bool tocar(const std::string& caminho) override {
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

  // O bombear NÃO é vazio. Vazio, elle nunca produziria o unico acontecimento em
  // que posição e estado mudam na MESMA batida, e a bateria ficaria cega ao
  // retracto composto que o tractado do tocador descreve ao longo de sete linhas.
  void bombear() override {
    if (!fim_pendente_) return;
    fim_pendente_ = false;
    posicao_ = 0.0;
    estado_ = Estado::Parado;
  }

  void acaba_na_proxima_batida() { fim_pendente_ = true; }
  void avanca(double delta) { posicao_ += delta; }

 private:
  double posicao_ = 0.0;
  Estado estado_ = Estado::Parado;
  bool fim_pendente_ = false;
};
// COLHE um campo da resposta, para que a prova não compare cadeias inteiras e se
// quebre a cada palavra que uma razão venha a mudar. Prova presa á letra da razão
// é prova que se rompe por lavra de estylo, e ahi deixa de ser sinal.
std::string campo(const std::string& resposta, const std::string& chave) {
  const Mensagem lida = analysa(resposta);
  if (!lida.valida) return "<NAO E JSON>";
  const mysong::api::Valor* achado = lida.acha(chave);
  if (achado == nullptr) return "<AUSENTE>";
  switch (achado->typo) {
    case Typo::Texto:    return achado->texto;
    case Typo::Booleano: return achado->booleano ? "true" : "false";
    case Typo::Numero:   return mysong::api::duplo(achado->numero);
    case Typo::Vector:   return std::to_string(achado->itens.size()) + " itens";
    case Typo::Nulo:     break;
  }
  return "null";
}

// Uma COVA com um índice lavrado á mão: dous artistas, dous albuns de um
// d'elles, duas faixas n'um album, e titulo com aspa e com UTF-8, que é o que o
// mundo tem. O banco entra por parametro, e é por isso que a prova não pode
// tocar o índice do operador.
class Cova {
 public:
  Cova() {
    static int conta = 0;
    caminho_ = std::filesystem::temp_directory_path() /
               ("mysong-api-" + std::to_string(::getpid()) + "-" +
                std::to_string(++conta));
    std::error_code erro;
    std::filesystem::create_directories(caminho_, erro);
  }
  ~Cova() { std::error_code erro; std::filesystem::remove_all(caminho_, erro); }
  Cova(const Cova&) = delete;
  Cova& operator=(const Cova&) = delete;

  std::filesystem::path banco() const { return caminho_ / "indice.sqlite3"; }

  void semeia() const {
    mysong::nucleo::Escriba escriba(banco());
    const struct { const char* quem; const char* qual; const char* titulo; } acervo[] = {
        {"Bach", "Cantatas", "Aria"},   {"Bach", "Cantatas", "Cor\"o"},
        {"Bach", "Suites", "Prelude"},  {"Coltrane", "Blue", "音楽"}};
    int numero = 0;
    for (const auto& linha : acervo) {
      mysong::nucleo::Faixa faixa;
      faixa.artista = linha.quem;
      faixa.album = linha.qual;
      faixa.titulo = linha.titulo;
      faixa.caminho = std::string("/acervo/") + linha.titulo + ".flac";
      faixa.numero = ++numero;
      faixa.duracao = 200 + numero;
      escriba.grava(faixa);
    }
    escriba.conclui();
  }

 private:
  std::filesystem::path caminho_;
};

std::string fala(Tocador& tocador, const std::string& linha) {
  return mysong::api::responde(tocador, linha);
}
}  // namespace

TEST_CASE("o escape nao deixa passar byte que parta o enquadramento") {
  CHECK(escapa("as\"pas") == "as\\\"pas");
  CHECK(escapa("contra\\barra") == "contra\\\\barra");
  CHECK(escapa("linha\nnova") == "linha\\nnova");
  CHECK(escapa("volta\rcarro") == "volta\\rcarro");
  CHECK(escapa("tabu\tlado") == "tabu\\tlado");
  CHECK(escapa(std::string("controle\x01""cru")) == "controle\\u0001cru");
  // O UTF-8 passa INTACTO: o JSON o admitte cru, e transcreve-lo seria inflar a
  // mensagem sem ganho algum de correcção.
  CHECK(escapa("Coração") == "Coração");
  CHECK(escapa("音楽") == "音楽");
}

// O caso que de facto guarda a invariante d'esta Casa. Nome de musica com
// mudanca de linha crua é o que parte o enquadramento de uma-mensagem-por-linha,
// e é o que o mundo tem e o dublê ingenuo nao teria.
TEST_CASE("faixa hostil emittida continua a ser UMA linha") {
  const std::string hostil = "01 - As \"Melhores\"\\ Canções\nde 音楽\x02.flac";
  const std::string emittido = texto(hostil);
  CHECK(emittido.find('\n') == std::string::npos);
  CHECK(emittido.front() == '"');
  CHECK(emittido.back() == '"');
  // E o que sahiu volta a entrar: o escape e o parser são um do outro.
  const Mensagem volta = analysa("{\"faixa\":" + emittido + "}");
  REQUIRE(volta.valida);
  REQUIRE(volta.acha("faixa") != nullptr);
  CHECK(volta.acha("faixa")->typo == Typo::Texto);
  CHECK(volta.acha("faixa")->texto == hostil);
}
// Rejeitar NAO é falhar: «json_malformado» é resposta prevista do contracto, e
// parser permissivo é que seria o risco, porque acceitaria por adivinhação o que
// o contracto não promette.
TEST_CASE("o parser recusa o que sae do subconjunto, e diz por que") {
  const char* torpes[] = {
      "{\"verbo\":",              // truncado no valor
      "naoejson",                 // nem objecto
      "[]",                       // vector, e não objecto
      "{\"verbo\":\"estado\"",    // sem o fecho
      "{\"verbo\":\"esta",        // cadeia sem aspa de fecho
      "{verbo:\"estado\"}",       // chave sem aspas
      "{\"verbo\":\"estado\",}",  // virgula pendurada
      "{\"verbo\":\"\\q\"}",      // escape que o subconjunto nao conhece
      "{\"a\":1,\"a\":2}",        // chave repetida
      "{\"n\":1.2.3}",            // numero mal formado
      "{\"verbo\":\"estado\"} sobra",
  };
  for (const char* torpe : torpes) {
    const Mensagem lida = analysa(torpe);
    CHECK_FALSE(lida.valida);
    CHECK_FALSE(lida.razao.empty());  // razão calada seria o silêncio proibido
  }
}

TEST_CASE("o parser acceita o subconjunto inteiro, e sómente elle") {
  CHECK(analysa("{}").valida);
  CHECK(analysa("  { \"verbo\" : \"estado\" }  ").valida);
  const Mensagem rica = analysa(
      "{\"t\":\"vae\",\"n\":-2.5,\"b\":true,\"z\":null,\"u\":\"\\u00e7\\ud83c\\udfb5\"}");
  REQUIRE(rica.valida);
  CHECK(rica.acha("t")->typo == Typo::Texto);
  CHECK(rica.acha("n")->typo == Typo::Numero);
  CHECK(rica.acha("n")->numero == doctest::Approx(-2.5));
  CHECK(rica.acha("b")->booleano);
  CHECK(rica.acha("z")->typo == Typo::Nulo);
  CHECK(rica.acha("u")->texto == "ç\xF0\x9F\x8E\xB5");  // par de substitutos
  CHECK(rica.acha("naoexiste") == nullptr);
}
TEST_CASE("os verbos de leitura devolvem o contracto e o retracto") {
  MotorDuble duble;
  Tocador tocador(duble);

  CHECK(campo(fala(tocador, "{\"verbo\":\"versao\"}"), "protocolo") == "2.000");
  CHECK(campo(fala(tocador, "{\"verbo\":\"versao\"}"), "obra") == "mysong");

  // Fila vazia é retracto legitimo, e não erro.
  const std::string vazio = fala(tocador, "{\"verbo\":\"estado\"}");
  CHECK(campo(vazio, "ok") == "true");
  CHECK(campo(vazio, "estado") == "Parado");
  CHECK(campo(vazio, "faixa").empty());
  CHECK(campo(vazio, "tamanho") == "0.000");

  CHECK(campo(fala(tocador, "{\"verbo\":\"juntar\",\"caminho\":\"uma.wav\"}"),
              "tamanho") == "1.000");
  CHECK(campo(fala(tocador, "{\"verbo\":\"juntar\",\"caminho\":\"duas.wav\"}"),
              "tamanho") == "2.000");
  REQUIRE(fala(tocador, "{\"verbo\":\"tocar\"}") == "{\"ok\":true}");
  duble.avanca(3.5);

  const std::string cheio = fala(tocador, "{\"verbo\":\"estado\"}");
  CHECK(campo(cheio, "estado") == "Tocando");
  CHECK(campo(cheio, "faixa") == "uma.wav");
  CHECK(campo(cheio, "posicao") == "3.500");
  CHECK(campo(cheio, "duracao") == "10.000");
  CHECK(campo(cheio, "volume") == "100.000");
  CHECK(campo(cheio, "indice") == "0.000");
  CHECK(campo(cheio, "tamanho") == "2.000");
}

TEST_CASE("o espectro responde as bandas, e diz em que escala ellas estao") {
  MotorDuble duble;
  Tocador tocador(duble);
  const std::string resposta = fala(tocador, "{\"verbo\":\"espectro\"}");
  CHECK(campo(resposta, "ok") == "true");
  CHECK(campo(resposta, "escala") == "logarithmica");
  CHECK(campo(resposta, "magnitude") == "decibeis");
  CHECK(campo(resposta, "hertz_minimo") == "40.000");
  CHECK(campo(resposta, "hertz_maximo") == "16000.000");
  CHECK(campo(resposta, "piso_decibeis") == "-60.000");
  CHECK(campo(resposta, "quantas") == "24.000");
  // As bandas voltam a ENTRAR pelo nosso proprio leitor, e é por isso que se
  // aferem por VALOR, em vez de se procurarem por texto dentro da resposta.
  const Mensagem lida = analysa(resposta);
  REQUIRE(lida.valida);
  REQUIRE(lida.acha("bandas") != nullptr);
  REQUIRE(lida.acha("bandas")->typo == Typo::Vector);
  const std::vector<double>& bandas = lida.acha("bandas")->numeros;
  CHECK(bandas.size() == mysong::nucleo::QUANTAS_BANDAS);
  // Sem fonte de bandas o tocador dá zeros, e o silencio é resposta e não erro.
  for (const double banda : bandas) CHECK(banda == doctest::Approx(0.0));
  CHECK(resposta.find('\n') == std::string::npos);
}

// O passeio pela fila NÃO ha de tocar faixa alguma: anda-se na Fila, e não no
// Tocador. Andar pelo Tocador faria soar duas faixas para listar dous nomes, e é
// defeito que a prova de resultado não pegaria e a de CHAMADA pega.
TEST_CASE("a fila lista-se sem tocar nada, e o assento volta ao logar") {
  MotorDuble duble;
  Tocador tocador(duble);
  // O pedido monta-se com o emissor da Casa, e NÃO por concatenação de aspas: a
  // primeira lavra d'este caso concatenava, e a faixa com aspa produzia pedido
  // malformado, donde a prova accusava o codigo por defeito que era d'ella.
  for (const char* faixa : {"uma.wav", "du\"as.wav", "tres音.wav"})
    fala(tocador, "{\"verbo\":\"juntar\",\"caminho\":" + mysong::api::texto(faixa) + "}");
  fala(tocador, "{\"verbo\":\"ir_para\",\"indice\":1}");
  const std::size_t tocados_antes = duble.tocados.size();

  const std::string listada = fala(tocador, "{\"verbo\":\"fila\"}");
  CHECK(campo(listada, "tamanho") == "3.000");
  CHECK(campo(listada, "indice") == "1.000");
  CHECK(listada.find("\"du\\\"as.wav\"") != std::string::npos);
  CHECK(listada.find("tres音.wav") != std::string::npos);
  CHECK(duble.tocados.size() == tocados_antes);  // o passeio nao mandou tocar
  CHECK(tocador.retracto().indice == 1);         // e o assento voltou
}
TEST_CASE("a bibliotheca navega os tres cortes do índice") {
  MotorDuble duble;
  Tocador tocador(duble);
  const Cova cova;
  cova.semeia();
  const mysong::nucleo::Biblioteca livraria(cova.banco());
  REQUIRE(livraria.aberta());
  mysong::api::Arredores arredores;
  arredores.livraria = &livraria;
  const auto pergunta = [&](const std::string& linha) {
    return mysong::api::responde(tocador, arredores, linha);
  };

  const std::string quem =
      pergunta("{\"verbo\":\"biblioteca\",\"corte\":\"artistas\"}");
  CHECK(campo(quem, "corte") == "artistas");
  CHECK(campo(quem, "tamanho") == "2.000");
  CHECK(quem.find("\"Bach\"") != std::string::npos);
  CHECK(quem.find("\"Coltrane\"") != std::string::npos);

  const std::string quaes = pergunta(
      "{\"verbo\":\"biblioteca\",\"corte\":\"albuns\",\"artista\":\"Bach\"}");
  CHECK(campo(quaes, "corte") == "albuns");
  CHECK(campo(quaes, "artista") == "Bach");
  CHECK(campo(quaes, "tamanho") == "2.000");

  const std::string faixas = pergunta(
      "{\"verbo\":\"biblioteca\",\"corte\":\"faixas\",\"artista\":\"Bach\","
      "\"album\":\"Cantatas\"}");
  CHECK(campo(faixas, "corte") == "faixas");
  CHECK(campo(faixas, "tamanho") == "2.000");
  // A faixa com aspa sae ESCAPADA, e a resposta continua a ser UMA linha.
  CHECK(faixas.find("Cor\\\"o") != std::string::npos);
  CHECK(faixas.find('\n') == std::string::npos);
}

TEST_CASE("os quatro vectores da faixa sahem paralelos") {
  MotorDuble duble;
  Tocador tocador(duble);
  const Cova cova;
  cova.semeia();
  const mysong::nucleo::Biblioteca livraria(cova.banco());
  mysong::api::Arredores arredores;
  arredores.livraria = &livraria;
  const auto pergunta = [&](const std::string& linha) {
    return mysong::api::responde(tocador, arredores, linha);
  };
  const Mensagem lida = analysa(pergunta(
      "{\"verbo\":\"biblioteca\",\"corte\":\"faixas\",\"artista\":\"Bach\","
      "\"album\":\"Cantatas\"}"));
  REQUIRE(lida.valida);
  REQUIRE(lida.acha("numeros") != nullptr);
  // Os quatro têm SEMPRE o mesmo comprimento, e elle é o «tamanho»: a posição i
  // dos quatro é a mesma faixa, e é n'isso que o cliente de fóra se apoia.
  REQUIRE(lida.acha("numeros")->numeros.size() == 2);
  CHECK(lida.acha("duracoes")->numeros.size() == 2);
  CHECK(lida.acha("titulos")->itens.size() == 2);
  CHECK(lida.acha("caminhos")->itens.size() == 2);
  CHECK(lida.acha("numeros")->numeros[0] == doctest::Approx(1.0));
  CHECK(lida.acha("caminhos")->itens[0] == "/acervo/Aria.flac");

  // Artista que não existe dá vector VAZIO e ok verdadeiro: no índice não ha
  // artista sem album, donde as duas cousas são a mesma vistas de fóra.
  const std::string ninguem = pergunta(
      "{\"verbo\":\"biblioteca\",\"corte\":\"albuns\",\"artista\":\"Ninguem\"}");
  CHECK(campo(ninguem, "ok") == "true");
  CHECK(campo(ninguem, "tamanho") == "0.000");
}

TEST_CASE("o corte da bibliotheca é obrigatorio, e o índice ausente é vazio") {
  MotorDuble duble;
  Tocador tocador(duble);
  // SEM arredores: índice ausente responde como índice VAZIO, e não como erro.
  const std::string vazio =
      fala(tocador, "{\"verbo\":\"biblioteca\",\"corte\":\"artistas\"}");
  CHECK(campo(vazio, "ok") == "true");
  CHECK(campo(vazio, "tamanho") == "0.000");

  const char* tortos[] = {
      "{\"verbo\":\"biblioteca\"}",
      "{\"verbo\":\"biblioteca\",\"corte\":7}",
      "{\"verbo\":\"biblioteca\",\"corte\":\"artistaa\"}",
      "{\"verbo\":\"biblioteca\",\"corte\":\"albuns\"}",
      "{\"verbo\":\"biblioteca\",\"corte\":\"albuns\",\"artista\":\"\"}",
      "{\"verbo\":\"biblioteca\",\"corte\":\"faixas\",\"artista\":\"Bach\"}",
  };
  for (const char* torto : tortos) {
    CHECK(campo(fala(tocador, torto), "erro") == "argumento_invalido");
    CHECK_FALSE(campo(fala(tocador, torto), "razao").empty());
  }
  // O corte inventado accusa o CORTE, e não o artista que elle nem chegou a
  // pedir: mandar olhar o argumento errado é pior que não mandar olhar nada.
  CHECK(campo(fala(tocador, "{\"verbo\":\"biblioteca\",\"corte\":\"artistaa\"}"),
              "razao")
            .find("corte") != std::string::npos);
}

TEST_CASE("os verbos de commando descem ao motor, e prova-se a CHAMADA") {
  MotorDuble duble;
  Tocador tocador(duble);
  for (const char* faixa : {"uma.wav", "duas.wav", "tres.wav"})
    fala(tocador, "{\"verbo\":\"juntar\",\"caminho\":" + mysong::api::texto(faixa) + "}");

  CHECK(campo(fala(tocador, "{\"verbo\":\"tocar\"}"), "ok") == "true");
  CHECK(campo(fala(tocador, "{\"verbo\":\"proxima\"}"), "ok") == "true");
  CHECK(campo(fala(tocador, "{\"verbo\":\"proxima\"}"), "ok") == "true");
  CHECK(campo(fala(tocador, "{\"verbo\":\"anterior\"}"), "ok") == "true");
  // A CHAMADA, e não só o booleano devolvido: protocolo que respondesse «ok» sem
  // mandar cousa alguma ao nucleo passaria por prova de resultado, e cae n'esta.
  const std::vector<std::string> esperado = {"uma.wav", "duas.wav", "tres.wav",
                                             "duas.wav"};
  CHECK(duble.tocados == esperado);

  CHECK(campo(fala(tocador, "{\"verbo\":\"pausar\"}"), "ok") == "true");
  CHECK(tocador.estado() == Estado::Pausado);
  CHECK(campo(fala(tocador, "{\"verbo\":\"retomar\"}"), "ok") == "true");
  CHECK(tocador.estado() == Estado::Tocando);
  // «parar» PAUSA, hoje, e o documento o declara com essas palavras.
  CHECK(campo(fala(tocador, "{\"verbo\":\"parar\"}"), "ok") == "true");
  CHECK(tocador.estado() == Estado::Pausado);

  CHECK(campo(fala(tocador, "{\"verbo\":\"volume\",\"porcento\":70}"), "volume") ==
        "70.000");
  CHECK(duble.volume_recebido == 70);
  // O aparo ANNUNCIA-SE: quem mandou 150 lê 100, e não fica a crer que assentou
  // 150 para estranhar depois pelo ouvido que o som não subiu.
  CHECK(campo(fala(tocador, "{\"verbo\":\"volume\",\"porcento\":150}"), "volume") ==
        "100.000");
  CHECK(duble.volume_recebido == 100);

  CHECK(campo(fala(tocador, "{\"verbo\":\"retomar\"}"), "ok") == "true");
  CHECK(campo(fala(tocador, "{\"verbo\":\"buscar\",\"segundos\":4.5}"), "ok") == "true");
  CHECK(duble.alvo_buscado == doctest::Approx(4.5));
}
TEST_CASE("a ordem que o nucleo recusa volta como recusado, e nunca como ok") {
  MotorDuble duble;
  Tocador tocador(duble);
  // Com a fila vazia e o tocador parado, quatro ordens legitimas recusam-se.
  CHECK(campo(fala(tocador, "{\"verbo\":\"tocar\"}"), "erro") == "recusado");
  CHECK(campo(fala(tocador, "{\"verbo\":\"pausar\"}"), "erro") == "recusado");
  CHECK(campo(fala(tocador, "{\"verbo\":\"retomar\"}"), "erro") == "recusado");
  CHECK(campo(fala(tocador, "{\"verbo\":\"buscar\",\"segundos\":1}"), "erro") ==
        "recusado");
  // E recusa NÃO é silêncio: a razão vae sempre, e nunca vazia.
  CHECK_FALSE(campo(fala(tocador, "{\"verbo\":\"pausar\"}"), "razao").empty());

  fala(tocador, "{\"verbo\":\"juntar\",\"caminho\":\"soh.wav\"}");
  REQUIRE(campo(fala(tocador, "{\"verbo\":\"tocar\"}"), "ok") == "true");

  // Na borda da fila NADA se manda ao motor, e a faixa em curso segue a tocar.
  const std::size_t antes = duble.tocados.size();
  CHECK(campo(fala(tocador, "{\"verbo\":\"proxima\"}"), "erro") == "recusado");
  CHECK(campo(fala(tocador, "{\"verbo\":\"anterior\"}"), "erro") == "recusado");
  CHECK(campo(fala(tocador, "{\"verbo\":\"ir_para\",\"indice\":9}"), "erro") ==
        "recusado");
  CHECK(duble.tocados.size() == antes);
  CHECK(tocador.estado() == Estado::Tocando);
}
TEST_CASE("a baixa encommenda-se, e a resposta nao espera pelo desfecho") {
  MotorDuble duble;
  Tocador tocador(duble);
  // A CANCELLA: a obra de mentira para n'ella, e a resposta do socket ha de
  // voltar com a obra ainda presa. É assim que o «não espera» se afere por
  // construcção, e não por relogio, que relogio a machina carregada perde.
  std::mutex tranca;
  std::condition_variable sino;
  bool solta = false;
  std::vector<mysong::nucleo::Pedido> encommendados;
  mysong::nucleo::Estaleiro estaleiro(
      1, [&](const mysong::nucleo::Pedido& pedido, std::filesystem::path*) {
        std::unique_lock<std::mutex> chave(tranca);
        encommendados.push_back(pedido);
        sino.notify_all();
        sino.wait(chave, [&solta] { return solta; });
        return mysong::nucleo::Colheita::Colhido;
      });
  mysong::api::Arredores arredores;
  arredores.estaleiro = &estaleiro;

  const std::string aceite = mysong::api::responde(
      tocador, arredores,
      "{\"verbo\":\"baixar\",\"url\":\"https://ha.de/ser\",\"artista\":\"Bach\","
      "\"numero\":3}");
  CHECK(campo(aceite, "ok") == "true");
  CHECK(campo(aceite, "colhidas") == "0.000");
  CHECK(campo(aceite, "ultima").empty());
  {
    std::unique_lock<std::mutex> chave(tranca);
    sino.wait(chave, [&] { return !encommendados.empty(); });
    // O que o operador DISSE chega inteiro á obra, e não sómente a URL.
    CHECK(encommendados[0].url == "https://ha.de/ser");
    CHECK(encommendados[0].artista == "Bach");
    CHECK(encommendados[0].numero == 3);
    solta = true;
  }
  sino.notify_all();
  estaleiro.espera_a_fila();
}

TEST_CASE("a baixa recusa a mensagem torta antes de encommendar") {
  MotorDuble duble;
  Tocador tocador(duble);
  mysong::nucleo::Estaleiro estaleiro(
      1, [](const mysong::nucleo::Pedido&, std::filesystem::path*) {
        return mysong::nucleo::Colheita::SemFerramenta;
      });
  mysong::api::Arredores arredores;
  arredores.estaleiro = &estaleiro;
  const char* tortos[] = {
      "{\"verbo\":\"baixar\"}",
      "{\"verbo\":\"baixar\",\"url\":\"\"}",
      "{\"verbo\":\"baixar\",\"url\":7}",
      "{\"verbo\":\"baixar\",\"url\":\"https://x\",\"numero\":-1}",
      "{\"verbo\":\"baixar\",\"url\":\"https://x\",\"numero\":1e9}",
  };
  for (const char* torto : tortos) {
    const std::string resposta = mysong::api::responde(tocador, arredores, torto);
    CHECK(campo(resposta, "erro") == "argumento_invalido");
    CHECK_FALSE(campo(resposta, "razao").empty());
  }
  // E NADA se encommendou: a mensagem torta não deixa resto na fila.
  estaleiro.espera_a_fila();
  CHECK(estaleiro.andamento().colhidas == 0);
  CHECK(estaleiro.andamento().falhadas == 0);
}

// A prova de que verbo algum sae MUDO, e de que as quatro recusas se distinguem.
// Distinguir importa porque o remedio de cada uma é differente: «recusado» manda
// olhar o estado do tocador, «argumento_invalido» manda olhar a mensagem,
// «indisponivel» manda olhar quem ergueu o servidor, e «verbo_desconhecido»
// manda olhar o nome que se escreveu.
TEST_CASE("verbo algum sae mudo, e as quatro recusas nao se confundem") {
  MotorDuble duble;
  Tocador tocador(duble);

  // Os tres nomes que a issue #65 honrou. Nenhum d'elles responde mais
  // «nao_implementado», e o emissor já nem sabe escrever essa palavra. Sem
  // arredores: o espectro responde as suas bandas, que o tocador as dá em zero
  // sem fonte; a bibliotheca responde como índice vazio; e a baixa, que não tem
  // como fingir fila, diz «indisponivel», que manda olhar quem ergueu o
  // servidor. Tres respostas differentes, e nenhuma d'ellas silencio.
  CHECK(campo(fala(tocador, "{\"verbo\":\"espectro\"}"), "ok") == "true");
  CHECK(campo(fala(tocador, "{\"verbo\":\"biblioteca\",\"corte\":\"artistas\"}"),
              "ok") == "true");
  const std::string sem_fila =
      fala(tocador, "{\"verbo\":\"baixar\",\"url\":\"https://ha.de/ser\"}");
  CHECK(campo(sem_fila, "erro") == "indisponivel");
  CHECK_FALSE(campo(sem_fila, "razao").empty());

  CHECK(campo(fala(tocador, "{\"verbo\":\"voar\"}"), "erro") == "verbo_desconhecido");
  CHECK(campo(fala(tocador, "{\"nada\":1}"), "erro") == "verbo_ausente");
  CHECK(campo(fala(tocador, "{\"verbo\":1}"), "erro") == "verbo_ausente");
  CHECK(campo(fala(tocador, "{\"verbo\":"), "erro") == "json_malformado");

  const char* tortos[] = {
      "{\"verbo\":\"volume\"}",
      "{\"verbo\":\"volume\",\"porcento\":\"alto\"}",
      "{\"verbo\":\"juntar\"}",
      "{\"verbo\":\"juntar\",\"caminho\":\"\"}",
      "{\"verbo\":\"ir_para\",\"indice\":-1}",
      "{\"verbo\":\"ir_para\",\"indice\":1e30}",
      "{\"verbo\":\"buscar\"}",
  };
  for (const char* torto : tortos)
    CHECK(campo(fala(tocador, torto), "erro") == "argumento_invalido");

  // A linha em branco é o UNICO caminho d'esta obra que devolve cadeia vazia.
  CHECK(fala(tocador, "").empty());
  CHECK(fala(tocador, "   \t  ").empty());
  // E toda resposta que NÃO seja vazia é UMA linha, sempre.
  CHECK(fala(tocador, "{\"verbo\":\"estado\"}").find('\n') == std::string::npos);
  CHECK(fala(tocador, "{\"verbo\":\"voar\"}").find('\n') == std::string::npos);
}
// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
