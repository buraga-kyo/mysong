// ══════════════════════════════════════════════════════════════════════════
//   TRACTADO DAS PROVAS DA BIBLIOTHECA — testes/prova_biblioteca.cpp
// ══════════════════════════════════════════════════════════════════════════
// Prova o ÍNDICE, e não o disco: nenhum caso d'este arquivo abre arquivo de
// audio, chama ffmpeg, ou toca taglib. As faixas entram amarradas á mão, o que
// deixa a prova das consultas independente da prova da varredura.
//
// E nenhum caso toca o índice do operador. Todo banco d'esta bateria nasce em
// directorio temporario proprio e morre com o caso; a ~/.local/share/mysong
// real não se lê nem se escreve, e é o C16 que o afere antes e depois.
//
// DOMÍNIO ......... bancos temporarios, enchidos linha a linha pelo caso.
// CONTRA-DOMÍNIO .. o veredicto do doctest, e por elle o do ctest.
// INVARIANTE ...... o alvo de cada asserção está ESCRIPTO no caso, e nunca se
//                   colhe da obra sob exame: prova que pergunta á obra o que a
//                   obra devia responder não prova cousa alguma.
#include <doctest/doctest.h>

#include <unistd.h>

#include <filesystem>
#include <string>
#include <vector>

#include "nucleo/biblioteca.hpp"

namespace nu = mysong::nucleo;

namespace {
// O caracter de substituição, por extenso e n'uma constante, porque colado a
// uma letra hexadecimal dentro de um literal elle seria lido como outro byte.
const std::string kTroca = "\xEF\xBF\xBD";

// Uma COVA de prova: directorio proprio, que nasce com o caso e morre com elle.
//
// É a peça que cumpre a prohibição, e não uma commodidade. Nenhum caso d'esta
// bateria toca ~/.local/share/mysong nem ~/Música: o caminho do banco entra por
// parametro, e é sómente por isso que uma corrida de prova não pode corromper o
// índice do operador. O nome leva o pid e um contador, para que duas corridas em
// paralello, ou dous casos do mesmo binario, não se pisem.
class Cova {
 public:
  Cova() {
    static int contador = 0;
    caminho_ = std::filesystem::temp_directory_path() /
               ("mysong-prova-" + std::to_string(::getpid()) + "-" +
                std::to_string(++contador));
    std::error_code erro;
    std::filesystem::remove_all(caminho_, erro);
    std::filesystem::create_directories(caminho_, erro);
  }
  // Apaga SÓMENTE o caminho que este objecto construiu, e nunca por padrão
  // largo: apagar por padrão é como se apaga o que não era nosso.
  ~Cova() {
    std::error_code erro;
    std::filesystem::remove_all(caminho_, erro);
  }
  Cova(const Cova&) = delete;
  Cova& operator=(const Cova&) = delete;

  const std::filesystem::path& raiz() const { return caminho_; }
  std::filesystem::path banco() const { return caminho_ / "indice.sqlite3"; }

 private:
  std::filesystem::path caminho_;
};

// Uma faixa armada á mão. Os campos que a prova não afere levam valores que
// variam com o numero, para que uma troca de columnas na gravação apparecesse
// como valor trocado, e não como dous zeros que se parecem.
nu::Faixa faz(const std::string& artista, const std::string& album,
              const std::string& titulo, int numero) {
  nu::Faixa faixa;
  faixa.caminho = "/acervo/" + artista + "/" + album + "/" + titulo + ".mp3";
  faixa.raiz = "/acervo";
  faixa.artista = artista;
  faixa.album = album;
  faixa.titulo = titulo;
  faixa.numero = numero;
  faixa.anno = 1800 + numero;
  faixa.duracao = 100 + numero;
  faixa.modificado = 1000 + numero;
  faixa.tamanho = 2000 + numero;
  return faixa;
}

// Enche um banco com o mesmo acervo de quatro faixas, para que os casos das
// consultas afiram a ORDEM contra uma taboa que se lê aqui em cima.
bool enche(const std::filesystem::path& banco) {
  nu::Escriba escriba(banco);
  if (!escriba.aberto()) return false;
  return escriba.grava(faz("Ada Lovelace", "Máquina Analítica", "Tear", 3)) &&
         escriba.grava(faz("Ada Lovelace", "Máquina Analítica", "Nota G", 7)) &&
         escriba.grava(faz("Ada Lovelace", "Notas de Menabrea", "Traducção", 1)) &&
         escriba.grava(faz("Bach", "Cravo Bem Temperado", "Fuga", 2)) &&
         escriba.conclui();
}
}  // namespace

TEST_CASE("saneia_utf8 conserva o valido e troca o invalido") {
  CHECK(nu::saneia_utf8("") == "");
  CHECK(nu::saneia_utf8("Ada Lovelace") == "Ada Lovelace");
  CHECK(nu::saneia_utf8("Máquina Analítica") == "Máquina Analítica");
  CHECK(nu::saneia_utf8("\xF0\x9F\x8E\xB5") == "\xF0\x9F\x8E\xB5");
  // Um byte de arranque a que falta a continuação: cae elle, e o '(' fica.
  CHECK(nu::saneia_utf8("\xC3\x28") == kTroca + "(");
  CHECK(nu::saneia_utf8("\xFF") == kTroca);
  CHECK(nu::saneia_utf8("\xC3") == kTroca);
  // Latin-1 mal etiquetado: o 0xE9 do «é» não é UTF-8, e o resto sobrevive.
  CHECK(nu::saneia_utf8("Ada\xE9Lovelace") == "Ada" + kTroca + "Lovelace");
  // Sobrelongo, e metade de par substituto: forma boa, valor proibido.
  CHECK(nu::saneia_utf8("\xC0\xAF") == kTroca + kTroca);
  CHECK(nu::saneia_utf8("\xED\xA0\x80") == kTroca + kTroca + kTroca);
}

TEST_CASE("o escriba grava e a bibliotheca conta o que se gravou") {
  const Cova cova;
  {
    nu::Escriba escriba(cova.banco());
    REQUIRE(escriba.aberto());
    // O temporario existe, e o banco AINDA NÃO: é o coração da promessa.
    CHECK(std::filesystem::exists(escriba.temporario()));
    CHECK_FALSE(std::filesystem::exists(cova.banco()));
    CHECK(escriba.grava(faz("Ada Lovelace", "Máquina Analítica", "Tear", 3)));
    CHECK(escriba.grava(faz("Ada Lovelace", "Máquina Analítica", "Nota G", 7)));
    CHECK(escriba.grava(faz("Ada Lovelace", "Notas de Menabrea", "Traducção", 1)));
    CHECK(escriba.grava(faz("Bach", "Cravo Bem Temperado", "Fuga", 2)));
    REQUIRE(escriba.conclui());
    CHECK_FALSE(std::filesystem::exists(escriba.temporario()));
  }
  CHECK(std::filesystem::exists(cova.banco()));
  const nu::Biblioteca livraria(cova.banco());
  REQUIRE(livraria.aberta());
  CHECK(livraria.versao() == 1);
  CHECK(livraria.total() == 4);
}

TEST_CASE("artistas e albuns sahem sem repetição e em ordem") {
  const Cova cova;
  REQUIRE(enche(cova.banco()));
  const nu::Biblioteca livraria(cova.banco());
  // Duas faixas de Ada no mesmo album, e o nome d'ella sahe UMA vez.
  CHECK(livraria.artistas() ==
        std::vector<std::string>{"Ada Lovelace", "Bach"});
  CHECK(livraria.albuns("Ada Lovelace") ==
        std::vector<std::string>{"Máquina Analítica", "Notas de Menabrea"});
  CHECK(livraria.albuns("Bach") ==
        std::vector<std::string>{"Cravo Bem Temperado"});
  CHECK(livraria.albuns("Quem Não Existe").empty());
}

TEST_CASE("as faixas de um album sahem em ordem de numero") {
  const Cova cova;
  REQUIRE(enche(cova.banco()));
  const nu::Biblioteca livraria(cova.banco());
  const std::vector<nu::Faixa> album =
      livraria.faixas_do_album("Ada Lovelace", "Máquina Analítica");
  REQUIRE(album.size() == 2);
  // Gravou-se «Tear» com o numero 3 e «Nota G» com o 7, e nesta ordem sahem.
  CHECK(album[0].titulo == "Tear");
  CHECK(album[0].numero == 3);
  CHECK(album[0].anno == 1803);
  CHECK(album[0].duracao == 103);
  CHECK(album[0].modificado == 1003);
  CHECK(album[0].tamanho == 2003);
  CHECK(album[0].deduzido == 0u);
  CHECK(album[1].titulo == "Nota G");
  CHECK(album[1].numero == 7);
  CHECK(livraria.faixas_do_album("Ada Lovelace", "Album Que Não Ha").empty());
}

TEST_CASE("a busca casa por pedaço de titulo") {
  const Cova cova;
  REQUIRE(enche(cova.banco()));
  const nu::Biblioteca livraria(cova.banco());
  const std::vector<nu::Faixa> achadas = livraria.busca_faixa("ota");
  REQUIRE(achadas.size() == 1);
  CHECK(achadas[0].titulo == "Nota G");
  CHECK(achadas[0].artista == "Ada Lovelace");
  CHECK(livraria.busca_faixa("Tear").size() == 1);
  CHECK(livraria.busca_faixa("zzz").empty());
}

// ══════════════════════════════════════════════════════════════════════════
//   Da lavra do eminente Doutor BRAGA US, Professor de Sciências Mathemáticas
//   e Geómetra desta Casa. Manuscripto lavrado no Anno da Graça de MDCCCXCVIII.
//                                                          — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
