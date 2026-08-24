// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DA PROVA DA API, BANDA PURA — testes/prova_api.cpp
// ══════════════════════════════════════════════════════════════════════════
// Prova o jsonzinho e o protocolo em MACHINA SURDA: sem socket, sem barramento,
// sem placa de som e sem arquivo em disco. É onde treze verbos e todo o
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

#include <string>
#include <vector>

#include "api/jsonzinho.hpp"
#include "api/protocolo.hpp"

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
    case Typo::Nulo:     break;
  }
  return "null";
}

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

  CHECK(campo(fala(tocador, "{\"verbo\":\"versao\"}"), "protocolo") == "1.000");
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

// O passeio pela fila NÃO ha de tocar faixa alguma: anda-se na Fila, e não no
// Tocador. Andar pelo Tocador faria soar duas faixas para listar dous nomes, e é
// defeito que a prova de resultado não pegaria e a de CHAMADA pega.
TEST_CASE("a fila lista-se sem tocar nada, e o assento volta ao logar") {
  MotorDuble duble;
  Tocador tocador(duble);
  for (const char* faixa : {"uma.wav", "du\"as.wav", "tres音.wav"})
    fala(tocador, std::string("{\"verbo\":\"juntar\",\"caminho\":\"") + faixa + "\"}");
  fala(tocador, "{\"verbo\":\"ir_para\",\"indice\":1}");
  const std::size_t tocados_antes = duble.tocados.size();

  const std::string listada = fala(tocador, "{\"verbo\":\"fila\"}");
  CHECK(campo(listada, "tamanho") == "3.000");
  CHECK(campo(listada, "indice") == "1.000");
  CHECK(listada.find("\"du\\\"as.wav\"") != std::string::npos);
  CHECK(listada.find("tres音.wav") != std::string::npos);
  CHECK(duble.tocados.size() == tocados_antes);  // o passeio nao mandou tocar
  CHECK(tocador.fila().indice() == 1);           // e o assento voltou
}
// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
