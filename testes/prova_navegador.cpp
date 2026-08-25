// ══════════════════════════════════════════════════════════════════════════
//   PROVA DO NAVEGADOR — testes/prova_navegador.cpp
// ══════════════════════════════════════════════════════════════════════════
// Percorre o caminho inteiro do aceite, de Artistas a uma faixa, sobre um índice
// que esta bateria escreve. Sem terminal, sem motor, sem som.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <unistd.h>

#include <filesystem>
#include <string>
#include <vector>

#include "nucleo/biblioteca.hpp"
#include "tui/navegador.hpp"

namespace nu = mysong::nucleo;
namespace tui = mysong::tui;

namespace {

class Cova {
 public:
  Cova() {
    caminho_ = std::filesystem::temp_directory_path() /
               ("mysong-nav-" + std::to_string(::getpid()) + "-" +
                std::to_string(++semente_));
    std::filesystem::create_directories(caminho_);
  }
  ~Cova() {
    std::error_code erro;
    std::filesystem::remove_all(caminho_, erro);
  }
  Cova(const Cova&) = delete;
  Cova& operator=(const Cova&) = delete;
  std::filesystem::path banco() const { return caminho_ / "indice.sqlite3"; }

 private:
  std::filesystem::path caminho_;
  static int semente_;
};

int Cova::semente_ = 0;

}  // namespace

namespace {

nu::Faixa faz(const std::string& artista, const std::string& album,
              const std::string& titulo, int numero) {
  nu::Faixa faixa;
  faixa.caminho = "/acervo/" + artista + "/" + album + "/" + titulo + ".mp3";
  faixa.raiz = "/acervo";
  faixa.artista = artista;
  faixa.album = album;
  faixa.titulo = titulo;
  faixa.numero = numero;
  faixa.duracao = 100 + numero;
  return faixa;
}

// O ACERVO da prova, escripto Á MÃO aqui em cima para que os casos aferem a ORDEM
// contra uma taboa que se lê, e não contra o que a obra devolveu.
//   Ada Lovelace / Máquina  : 3 Tear, 7 Nota G
//   Ada Lovelace / Notas    : 1 Traducção
//   Bach         / Cravo     : 2 Fuga
bool enche(const std::filesystem::path& banco) {
  nu::Escriba escriba(banco);
  if (!escriba.aberto()) return false;
  return escriba.grava(faz("Ada Lovelace", "Máquina", "Tear", 3)) &&
         escriba.grava(faz("Ada Lovelace", "Máquina", "Nota G", 7)) &&
         escriba.grava(faz("Ada Lovelace", "Notas", "Traducção", 1)) &&
         escriba.grava(faz("Bach", "Cravo", "Fuga", 2)) &&
         escriba.conclui();
}

std::vector<std::string> textos(const tui::Navegador& navegador) {
  std::vector<std::string> fóra;
  for (const tui::Linha& linha : navegador.vista()) fóra.push_back(linha.texto);
  return fóra;
}

}  // namespace

// O CAMINHO DO ACEITE, de ponta a ponta: de Artistas a um artista, d'elle a um
// album, e d'alli a uma faixa que se manda tocar. Cada degrau afere-se contra a
// taboa escripta no arnês.
TEST_CASE("de artistas a uma faixa, o caminho inteiro do aceite") {
  const Cova cova;
  REQUIRE(enche(cova.banco()));
  const nu::Biblioteca livraria(cova.banco());
  tui::Navegador navegador(livraria);

  CHECK(navegador.secao() == tui::Secao::Artistas);
  CHECK(textos(navegador) == std::vector<std::string>{"Ada Lovelace", "Bach"});
  CHECK(navegador.trilha().empty());
  CHECK(navegador.eleito() == 0u);
  // Artista não tem caminho: pedi-lo aqui devolve vazio de proposito.
  CHECK(navegador.caminho_eleito().empty());

  CHECK_FALSE(navegador.entra());  // entrou n'um artista, e não n'uma faixa
  CHECK(navegador.secao() == tui::Secao::Albuns);
  CHECK(navegador.trilha() == std::vector<std::string>{"Ada Lovelace"});
  CHECK(textos(navegador) == std::vector<std::string>{"Máquina", "Notas"});

  CHECK_FALSE(navegador.entra());  // entrou n'um album
  CHECK(navegador.secao() == tui::Secao::Faixas);
  CHECK(navegador.trilha() ==
        std::vector<std::string>{"Ada Lovelace", "Máquina"});
  // Ordem de NUMERO: gravou-se Tear com tres e Nota G com sete.
  CHECK(textos(navegador) == std::vector<std::string>{"Tear", "Nota G"});
  CHECK(navegador.vista()[0].numero == 3);
  CHECK(navegador.vista()[0].duracao == 103);
  CHECK(navegador.vista()[0].autor == "Ada Lovelace");

  CHECK(navegador.entra());  // AGORA é faixa: quem chama manda tocar
  CHECK(navegador.caminho_eleito() ==
        "/acervo/Ada Lovelace/Máquina/Tear.mp3");
  // E entrar n'uma faixa não muda a secção: continua-se onde se estava.
  CHECK(navegador.secao() == tui::Secao::Faixas);
}

// A LISTA NÃO DÁ A VOLTA. Descer no ultimo fica no ultimo, e subir no primeiro
// fica no primeiro: dar a volta n'uma lista de mil artistas faria o operador
// perder o logar sem saber como.
TEST_CASE("a lista não dá a volta nas duas pontas") {
  const Cova cova;
  REQUIRE(enche(cova.banco()));
  const nu::Biblioteca livraria(cova.banco());
  tui::Navegador navegador(livraria);
  REQUIRE(navegador.vista().size() == 2u);

  CHECK(navegador.eleito() == 0u);
  navegador.sobe();
  CHECK(navegador.eleito() == 0u);  // no primeiro, subir não passa
  navegador.desce();
  CHECK(navegador.eleito() == 1u);
  navegador.desce();
  CHECK(navegador.eleito() == 1u);  // no ultimo, descer não passa
  navegador.ao_principio();
  CHECK(navegador.eleito() == 0u);
  navegador.ao_fim();
  CHECK(navegador.eleito() == 1u);
}

// Voltar sobe UM degrau, e no alto não faz nada.
TEST_CASE("voltar sobe um degrau, e no alto devolve falso") {
  const Cova cova;
  REQUIRE(enche(cova.banco()));
  const nu::Biblioteca livraria(cova.banco());
  tui::Navegador navegador(livraria);
  navegador.entra();  // Albuns de Ada Lovelace
  navegador.entra();  // Faixas de Máquina
  REQUIRE(navegador.secao() == tui::Secao::Faixas);

  CHECK(navegador.volta());
  CHECK(navegador.secao() == tui::Secao::Albuns);
  CHECK(navegador.trilha() == std::vector<std::string>{"Ada Lovelace"});
  CHECK(navegador.volta());
  CHECK(navegador.secao() == tui::Secao::Artistas);
  CHECK(navegador.trilha().empty());
  CHECK_FALSE(navegador.volta());  // no alto, nada
  CHECK(navegador.secao() == tui::Secao::Artistas);
}

TEST_CASE("o filtro corta a vista, sem caixa, e limpa-se com cadeia vazia") {
  const Cova cova;
  REQUIRE(enche(cova.banco()));
  const nu::Biblioteca livraria(cova.banco());
  tui::Navegador navegador(livraria);

  navegador.filtra("bach");  // minusculas contra «Bach»
  CHECK(textos(navegador) == std::vector<std::string>{"Bach"});
  CHECK(navegador.termo() == "bach");
  navegador.filtra("ADA");   // maiusculas contra «Ada Lovelace»
  CHECK(textos(navegador) == std::vector<std::string>{"Ada Lovelace"});
  navegador.filtra("zzz");
  CHECK(navegador.vista().empty());
  CHECK(navegador.eleito() == 0u);          // vista vazia, eleito em zero
  CHECK(navegador.caminho_eleito().empty()); // e nada se manda tocar
  navegador.filtra("");
  CHECK(textos(navegador) == std::vector<std::string>{"Ada Lovelace", "Bach"});
}

// O ELEITO nunca sahe da vista. É o invariante, e a prova exercita-o pelo caminho
// que o quebraria: eleger o ultimo de uma lista longa e depois encurtá-la.
TEST_CASE("o eleito apara-se quando a vista encurta") {
  const Cova cova;
  REQUIRE(enche(cova.banco()));
  const nu::Biblioteca livraria(cova.banco());
  tui::Navegador navegador(livraria);
  navegador.ao_fim();
  REQUIRE(navegador.eleito() == 1u);
  navegador.filtra("Bach");            // um só sobra
  REQUIRE(navegador.vista().size() == 1u);
  CHECK(navegador.eleito() == 0u);     // e o eleito cabe n'ella
  CHECK(navegador.vista()[navegador.eleito()].texto == "Bach");
}

// O termo NÃO se herda ao descer: filtrar por «ada» e entrar mostra os albuns
// TODOS d'ella, e não sómente os que casassem com «ada».
TEST_CASE("o termo não se herda ao descer nem ao subir") {
  const Cova cova;
  REQUIRE(enche(cova.banco()));
  const nu::Biblioteca livraria(cova.banco());
  tui::Navegador navegador(livraria);
  navegador.filtra("ada");
  REQUIRE(navegador.vista().size() == 1u);
  navegador.entra();
  CHECK(navegador.termo().empty());
  CHECK(textos(navegador) == std::vector<std::string>{"Máquina", "Notas"});
  navegador.filtra("Notas");
  navegador.volta();
  CHECK(navegador.termo().empty());
  CHECK(textos(navegador) == std::vector<std::string>{"Ada Lovelace", "Bach"});
}

// Recarregar depois de o acervo mudar. Se o artista sahiu do disco, cahe-se a
// Artistas; se sómente o album sahiu, cahe-se a Albuns, e não ao alto.
TEST_CASE("recarregar conserva a trilha que sobreviveu, e cede a que não") {
  const Cova cova;
  REQUIRE(enche(cova.banco()));
  {
    const nu::Biblioteca livraria(cova.banco());
    tui::Navegador navegador(livraria);
    navegador.entra();
    navegador.entra();
    REQUIRE(navegador.secao() == tui::Secao::Faixas);
    navegador.recarrega();  // nada mudou: fica onde estava
    CHECK(navegador.secao() == tui::Secao::Faixas);
    CHECK(navegador.trilha() ==
          std::vector<std::string>{"Ada Lovelace", "Máquina"});
  }
  // O album «Máquina» sahe do acervo; o artista fica, com o outro album.
  {
    nu::Escriba escriba(cova.banco());
    REQUIRE(escriba.aberto());
    REQUIRE(escriba.grava(faz("Ada Lovelace", "Notas", "Traducção", 1)));
    REQUIRE(escriba.grava(faz("Bach", "Cravo", "Fuga", 2)));
    REQUIRE(escriba.conclui());
  }
  const nu::Biblioteca depois(cova.banco());
  tui::Navegador navegador(depois);
  navegador.entra();  // Albuns de Ada
  navegador.entra();  // Faixas de Notas
  REQUIRE(navegador.trilha() ==
          std::vector<std::string>{"Ada Lovelace", "Notas"});
  CHECK(navegador.secao() == tui::Secao::Faixas);
}

TEST_CASE("acervo vazio dá vista vazia, e ordem alguma estoura") {
  const Cova cova;  // banco algum se escreve
  const nu::Biblioteca livraria(cova.banco());
  tui::Navegador navegador(livraria);
  CHECK(navegador.vista().empty());
  CHECK(navegador.eleito() == 0u);
  navegador.desce();
  navegador.sobe();
  navegador.ao_fim();
  navegador.ao_principio();
  CHECK(navegador.eleito() == 0u);
  CHECK_FALSE(navegador.entra());
  CHECK_FALSE(navegador.volta());
  CHECK(navegador.caminho_eleito().empty());
  navegador.filtra("qualquer");
  CHECK(navegador.vista().empty());
  navegador.recarrega();
  CHECK(navegador.vista().empty());
}

// A ROLAGEM, com os alvos escriptos á mão. Cem linhas n'uma tabella de dez.
TEST_CASE("a rolagem rola o menos que baste, e não perde a posição") {
  // Cabendo tudo, fatia alguma se rola.
  CHECK(tui::primeira_a_mostrar(0, 5, 10, 0) == 0u);
  CHECK(tui::primeira_a_mostrar(4, 5, 10, 0) == 0u);
  // Descendo dentro da fatia, ella não se mexe.
  CHECK(tui::primeira_a_mostrar(9, 100, 10, 0) == 0u);
  // Passando UMA linha do fundo, rola-se UMA linha, e não meia tela.
  CHECK(tui::primeira_a_mostrar(10, 100, 10, 0) == 1u);
  CHECK(tui::primeira_a_mostrar(11, 100, 10, 1) == 2u);
  // Subindo acima do topo, rola-se para o eleito.
  CHECK(tui::primeira_a_mostrar(30, 100, 10, 40) == 30u);
  // Voltando ao logar de antes, devolve-se a MESMA fatia.
  CHECK(tui::primeira_a_mostrar(45, 100, 10, 40) == 40u);
  // No fim da lista, a fatia encosta-se ao fim e não passa d'elle.
  CHECK(tui::primeira_a_mostrar(99, 100, 10, 0) == 90u);
  CHECK(tui::primeira_a_mostrar(99, 100, 10, 95) == 90u);
  // A lista encurtou debaixo da fatia: ella encosta-se ao fim.
  CHECK(tui::primeira_a_mostrar(0, 12, 10, 40) == 0u);
  CHECK(tui::primeira_a_mostrar(11, 12, 10, 40) == 2u);
  // As duas degenerescencias: altura zero e lista vazia.
  CHECK(tui::primeira_a_mostrar(0, 100, 0, 7) == 0u);
  CHECK(tui::primeira_a_mostrar(0, 0, 10, 7) == 0u);
}

// ── A SECÇÃO DA REDE (issue #12) ────────────────────────────────────────────
// A unica cujas linhas não vêm da bibliotheca. Os casos abaixo guardam a fronteira
// entre ella e o acervo, que é o que impede uma URL de cahir na fila do motor.

// achado — uma linha da rede, como mostra_rede a recebe: o titulo por texto, a URL
// por chave, o canal por autor.
tui::Linha achado(const std::string& titulo, const std::string& canal,
                  int duracao, const std::string& url) {
  return {titulo, url, 0, duracao, canal};
}

TEST_CASE("a rede põe linhas de fóra na tela, e a URL não é caminho") {
  Cova cova;
  REQUIRE(enche(cova.banco()));
  nu::Biblioteca livraria(cova.banco());
  tui::Navegador navegador(livraria);

  navegador.mostra_rede({achado("Toccata", "Canal A", 542, "https://y/1"),
                         achado("Fuga", "Canal B", 300, "https://y/2")});
  CHECK(navegador.secao() == tui::Secao::Rede);
  REQUIRE(navegador.vista().size() == 2);
  CHECK(navegador.vista()[0].texto == "Toccata");
  CHECK(navegador.vista()[0].autor == "Canal A");
  CHECK(navegador.vista()[0].duracao == 542);
  CHECK(navegador.url_eleita() == "https://y/1");
  // A FRONTEIRA: caminho_eleito é vazio na rede. Se elle devolvesse a URL, a
  // janella enfileirava-a no motor e o mpv tentava tocar um endereço por arquivo.
  CHECK(navegador.caminho_eleito().empty());
  // E entrar n'um achado não desce degrau algum: quem chama pede a url_eleita.
  CHECK_FALSE(navegador.entra());
  CHECK(navegador.secao() == tui::Secao::Rede);

  navegador.desce();
  CHECK(navegador.url_eleita() == "https://y/2");
}

TEST_CASE("fóra da rede não ha URL eleita, e o filtro corta os achados") {
  Cova cova;
  REQUIRE(enche(cova.banco()));
  nu::Biblioteca livraria(cova.banco());
  tui::Navegador navegador(livraria);

  // Em Artistas, url_eleita é vazia ainda que haja linha eleita.
  REQUIRE_FALSE(navegador.vista().empty());
  CHECK(navegador.url_eleita().empty());

  navegador.mostra_rede({achado("Toccata em Re", "A", 1, "https://y/1"),
                         achado("Fuga em Sol", "B", 2, "https://y/2"),
                         achado("Toccata em Do", "C", 3, "https://y/3")});
  navegador.filtra("toccata");  // sem caixa, como nas outras secções
  REQUIRE(navegador.vista().size() == 2);
  CHECK(navegador.vista()[0].texto == "Toccata em Re");
  CHECK(navegador.vista()[1].texto == "Toccata em Do");
  // O filtro corta a VISTA e não a fonte: limpando-o, os tres voltam.
  navegador.filtra("");
  CHECK(navegador.vista().size() == 3);
}

TEST_CASE("recarregar na rede não mexe na vista, e voltar sahe da secção") {
  Cova cova;
  REQUIRE(enche(cova.banco()));
  nu::Biblioteca livraria(cova.banco());
  tui::Navegador navegador(livraria);

  navegador.mostra_rede({achado("Toccata", "A", 1, "https://y/1")});
  // Recarregar corre quando a varredura conclue, e ella conclue a qualquer hora.
  // Na rede ella NÃO ha de mexer na vista: apagaria os achados por baixo do olho
  // do operador, no meio de elle escolher qual baixar.
  navegador.recarrega();
  CHECK(navegador.secao() == tui::Secao::Rede);
  CHECK(navegador.vista().size() == 1);

  CHECK(navegador.volta());
  CHECK(navegador.secao() == tui::Secao::Artistas);
  // Voltando, a vista é o ACERVO outra vez, e não os achados.
  CHECK_FALSE(navegador.vista().empty());
  CHECK(navegador.vista()[0].texto == "Ada Lovelace");
  CHECK(navegador.url_eleita().empty());
}

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
