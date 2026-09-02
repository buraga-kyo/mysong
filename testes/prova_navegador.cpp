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
#include "nucleo/rol.hpp"
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

// achado — um achado da busca comum, como mostra_rede o recebe (issue #56): sem
// os campos da musica, donde o texto sahe do titulo e o autor do canal.
nu::Achado achado(const std::string& titulo, const std::string& canal,
                  int duracao, const std::string& url) {
  nu::Achado feito;
  feito.titulo = titulo;
  feito.canal = canal;
  feito.duracao = duracao;
  feito.url = url;
  return feito;
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
  CHECK(navegador.achado_eleito().url == "https://y/1");
  // A FRONTEIRA: caminho_eleito é vazio na rede. Se elle devolvesse a URL, a
  // janella enfileirava-a no motor e o mpv tentava tocar um endereço por arquivo.
  CHECK(navegador.caminho_eleito().empty());
  // E entrar n'um achado não desce degrau algum: quem chama pede o achado eleito.
  CHECK_FALSE(navegador.entra());
  CHECK(navegador.secao() == tui::Secao::Rede);

  navegador.desce();
  CHECK(navegador.achado_eleito().url == "https://y/2");
}

TEST_CASE("fóra da rede não ha URL eleita, e o filtro corta os achados") {
  Cova cova;
  REQUIRE(enche(cova.banco()));
  nu::Biblioteca livraria(cova.banco());
  tui::Navegador navegador(livraria);

  // Em Artistas não ha achado, ainda que haja linha eleita.
  REQUIRE_FALSE(navegador.vista().empty());
  CHECK_FALSE(navegador.ha_achado());

  navegador.mostra_rede({achado("Toccata em Re", "A", 1, "https://y/1"),
                         achado("Fuga em Sol", "B", 2, "https://y/2"),
                         achado("Toccata em Do", "C", 3, "https://y/3")});
  navegador.filtra("toccata");  // sem caixa, como nas outras secções
  REQUIRE(navegador.vista().size() == 2);
  CHECK(navegador.vista()[0].texto == "Toccata em Re");
  CHECK(navegador.vista()[1].texto == "Toccata em Do");
  // O ACHADO eleito segue o filtro pelo origem, e não pelo indice da vista: a
  // segunda linha filtrada é o TERCEIRO achado, e é o d'elle que a URL sahe.
  navegador.desce();
  CHECK(navegador.achado_eleito().url == "https://y/3");
  // O filtro corta a VISTA e não a fonte: limpando-o, os tres voltam.
  navegador.filtra("");
  CHECK(navegador.vista().size() == 3);
  // E olha o AUTOR tambem, como na secção Lista: com a fonte de musica o autor
  // é o artista, e buscar por elle é o gesto natural (issue #56).
  navegador.filtra("b");  // texto algum tem b; o autor da Fuga é B
  REQUIRE(navegador.vista().size() == 1);
  CHECK(navegador.vista()[0].texto == "Fuga em Sol");
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
  CHECK_FALSE(navegador.ha_achado());
}

// ── AS LISTAS (issue #10) ───────────────────────────────────────────────────
// A navegação das listas prova-se contra um Roleiro de verdade, n'uma cova propria:
// o banco d'ellas entra por parâmetro, como o do índice.

// CovaDoRol — o banco das listas, á parte do índice. São dous arquivos porque são
// dous bancos, e a razão está no tractado do rol.
class CovaDoRol {
 public:
  CovaDoRol() {
    caminho_ = std::filesystem::temp_directory_path() /
               ("mysong-navrol-" + std::to_string(::getpid()) + "-" +
                std::to_string(++semente_));
    std::filesystem::create_directories(caminho_);
  }
  ~CovaDoRol() {
    std::error_code erro;
    std::filesystem::remove_all(caminho_, erro);
  }
  CovaDoRol(const CovaDoRol&) = delete;
  CovaDoRol& operator=(const CovaDoRol&) = delete;
  std::filesystem::path banco() const { return caminho_ / "rol.sqlite3"; }

 private:
  std::filesystem::path caminho_;
  static int semente_;
};

int CovaDoRol::semente_ = 0;

TEST_CASE("sem roleiro, as secções das listas ficam vazias e nada estoura") {
  Cova cova;
  REQUIRE(enche(cova.banco()));
  nu::Biblioteca livraria(cova.banco());
  tui::Navegador navegador(livraria);  // punho nullo: corrida sem listas

  navegador.mostra_rois();
  CHECK(navegador.secao() == tui::Secao::Rois);
  CHECK(navegador.vista().empty());
  // As sete operações devolvem falso, e ordem alguma estoura.
  CHECK_FALSE(navegador.cria_rol("Da manhã"));
  CHECK_FALSE(navegador.renomeia_rol("Outra"));
  CHECK_FALSE(navegador.apaga_rol());
  CHECK_FALSE(navegador.junta_ao_rol("/a/1.mp3"));
  CHECK_FALSE(navegador.retira_do_rol());
  CHECK_FALSE(navegador.sobe_no_rol());
  CHECK_FALSE(navegador.desce_no_rol());
  CHECK_FALSE(navegador.entra());
  CHECK(navegador.rol_corrente() == 0);
  CHECK(navegador.nome_corrente().empty());
}

TEST_CASE("criar mostra a lista, e entrar n'ella abre o dentro") {
  Cova cova;
  CovaDoRol coval;
  REQUIRE(enche(cova.banco()));
  nu::Biblioteca livraria(cova.banco());
  nu::Roleiro roleiro(coval.banco());
  tui::Navegador navegador(livraria, &roleiro);

  REQUIRE(navegador.cria_rol("Da manhã"));
  // Criar LEVA á secção das listas: quem cria quer ver que ella nasceu.
  CHECK(navegador.secao() == tui::Secao::Rois);
  REQUIRE(navegador.vista().size() == 1);
  CHECK(navegador.vista()[0].texto == "Da manhã");
  CHECK(navegador.vista()[0].numero == 0);  // vazia, e a columna do numero conta
  CHECK(navegador.nome_do_rol_eleito() == "Da manhã");
  // Nome repetido não cria segunda, e a tela fica como estava.
  CHECK_FALSE(navegador.cria_rol("  Da manhã  "));
  CHECK(navegador.vista().size() == 1);

  CHECK_FALSE(navegador.entra());  // lista não é faixa: nada se toca
  CHECK(navegador.secao() == tui::Secao::NoRol);
  CHECK(navegador.rol_corrente() > 0);
  CHECK(navegador.trilha() == std::vector<std::string>{"Da manhã"});
  CHECK(navegador.vista().empty());
  // Voltar de dentro vae á lista das listas, e não ao acervo: é o degrau de que
  // se veio.
  CHECK(navegador.volta());
  CHECK(navegador.secao() == tui::Secao::Rois);
  // E o ALVO fica. É d'isto que depende o `a` do acervo: para juntar uma faixa é
  // preciso estar onde a faixa está, e a faixa não está dentro da lista. Se o alvo
  // se perdesse ao sahir, juntar do acervo nunca poderia funccionar.
  CHECK(navegador.rol_corrente() > 0);
  CHECK(navegador.nome_corrente() == "Da manhã");
  CHECK(navegador.volta());
  CHECK(navegador.secao() == tui::Secao::Artistas);
  CHECK(navegador.rol_corrente() > 0);
}

TEST_CASE("juntar tres faixas põe-nas na ordem, e o caminho eleito é o d'ellas") {
  Cova cova;
  CovaDoRol coval;
  REQUIRE(enche(cova.banco()));
  nu::Biblioteca livraria(cova.banco());
  nu::Roleiro roleiro(coval.banco());
  tui::Navegador navegador(livraria, &roleiro);
  REQUIRE(navegador.cria_rol("Da manhã"));
  // FÓRA de uma lista não ha alvo, e juntar recusa: o alvo é a lista em que se
  // ESTÁ, e não uma que se adivinhe. Quem recusa é a chave estrangeira, que não
  // conhece lista de id zero.
  CHECK_FALSE(navegador.junta_ao_rol("/acervo/A/B/1.mp3"));
  REQUIRE_FALSE(navegador.entra());

  REQUIRE(navegador.junta_ao_rol("/acervo/A/B/1.mp3"));
  REQUIRE(navegador.junta_ao_rol("/acervo/A/B/2.mp3"));
  REQUIRE(navegador.junta_ao_rol("/acervo/A/B/3.mp3"));
  REQUIRE(navegador.vista().size() == 3);
  CHECK(navegador.vista()[0].texto == "1.mp3");
  CHECK(navegador.vista()[0].numero == 1);
  CHECK(navegador.vista()[2].numero == 3);
  // O caminho eleito é o do disco, e é elle que a fila do nucleo recebe.
  CHECK(navegador.caminho_eleito() == "/acervo/A/B/1.mp3");
  navegador.ao_fim();
  CHECK(navegador.caminho_eleito() == "/acervo/A/B/3.mp3");
  // E entrar n'uma faixa da lista DIZ que era faixa: quem chama enche a fila.
  CHECK(navegador.entra());
}

TEST_CASE("mover para cima e para baixo troca a ordem, e o olho segue a faixa") {
  Cova cova;
  CovaDoRol coval;
  REQUIRE(enche(cova.banco()));
  nu::Biblioteca livraria(cova.banco());
  nu::Roleiro roleiro(coval.banco());
  tui::Navegador navegador(livraria, &roleiro);
  REQUIRE(navegador.cria_rol("Da manhã"));
  REQUIRE_FALSE(navegador.entra());
  for (const char* qual : {"/a/1.mp3", "/a/2.mp3", "/a/3.mp3"})
    REQUIRE(navegador.junta_ao_rol(qual));

  navegador.ao_fim();  // a terceira
  REQUIRE(navegador.sobe_no_rol());
  CHECK(navegador.vista()[1].texto == "3.mp3");
  // O OLHO segue a faixa que se moveu: sem isso, subir uma vez elegia a vizinha e
  // subir duas vezes movia a faixa errada.
  CHECK(navegador.eleito() == 1);
  REQUIRE(navegador.sobe_no_rol());
  CHECK(navegador.vista()[0].texto == "3.mp3");
  CHECK(navegador.eleito() == 0);
  // O primeiro não sobe, e a lista fica como estava.
  CHECK_FALSE(navegador.sobe_no_rol());
  CHECK(navegador.vista()[0].texto == "3.mp3");

  REQUIRE(navegador.desce_no_rol());
  CHECK(navegador.vista()[1].texto == "3.mp3");
  CHECK(navegador.eleito() == 1);
  navegador.ao_fim();
  // O ultimo não desce: a troca com a ordem que não ha recusa-se em baixo.
  CHECK_FALSE(navegador.desce_no_rol());
}

TEST_CASE("retirar tira a faixa certa, ainda com filtro posto") {
  Cova cova;
  CovaDoRol coval;
  REQUIRE(enche(cova.banco()));
  nu::Biblioteca livraria(cova.banco());
  nu::Roleiro roleiro(coval.banco());
  tui::Navegador navegador(livraria, &roleiro);
  REQUIRE(navegador.cria_rol("Da manhã"));
  REQUIRE_FALSE(navegador.entra());
  for (const char* qual : {"/a/alfa.mp3", "/a/beta.mp3", "/a/gama.mp3"})
    REQUIRE(navegador.junta_ao_rol(qual));

  // FILTRO posto: a vista tem uma linha, e ella é a TERCEIRA do banco. Se a ordem
  // se tirasse do indice da vista, retirar-se-hia a primeira.
  navegador.filtra("gama");
  REQUIRE(navegador.vista().size() == 1);
  CHECK(navegador.vista()[0].numero == 3);
  REQUIRE(navegador.retira_do_rol());
  navegador.filtra("");
  REQUIRE(navegador.vista().size() == 2);
  CHECK(navegador.vista()[0].texto == "alfa.mp3");
  CHECK(navegador.vista()[1].texto == "beta.mp3");
}

TEST_CASE("renomear conserva o dentro, e apagar sahe para a lista das listas") {
  Cova cova;
  CovaDoRol coval;
  REQUIRE(enche(cova.banco()));
  nu::Biblioteca livraria(cova.banco());
  nu::Roleiro roleiro(coval.banco());
  tui::Navegador navegador(livraria, &roleiro);
  REQUIRE(navegador.cria_rol("Da manhã"));
  REQUIRE_FALSE(navegador.entra());
  REQUIRE(navegador.junta_ao_rol("/a/1.mp3"));

  REQUIRE(navegador.renomeia_rol("  Da tarde  "));
  // A trilha muda com o nome: sem isso, o titulo da tabella continuava a dizer o
  // nome velho até se sahir e tornar a entrar.
  CHECK(navegador.trilha() == std::vector<std::string>{"Da tarde"});
  CHECK(navegador.nome_do_rol_eleito() == "Da tarde");
  CHECK(navegador.vista().size() == 1);

  REQUIRE(navegador.apaga_rol());
  // Apagada a lista em que se estava, não ha dentro onde ficar; e o ALVO vae-se
  // com ella, que apontar para lista que já não existe faria `a` falhar calado.
  CHECK(navegador.secao() == tui::Secao::Rois);
  CHECK(navegador.vista().empty());
  CHECK(navegador.rol_corrente() == 0);
  CHECK(navegador.nome_corrente().empty());
}

TEST_CASE("a lista sobrevive a reabrir o roleiro, com a ordem que se deixou") {
  Cova cova;
  CovaDoRol coval;
  REQUIRE(enche(cova.banco()));
  nu::Biblioteca livraria(cova.banco());
  {
    nu::Roleiro roleiro(coval.banco());
    tui::Navegador navegador(livraria, &roleiro);
    REQUIRE(navegador.cria_rol("Da manhã"));
    REQUIRE_FALSE(navegador.entra());
    for (const char* qual : {"/a/1.mp3", "/a/2.mp3", "/a/3.mp3"})
      REQUIRE(navegador.junta_ao_rol(qual));
    navegador.ao_fim();
    REQUIRE(navegador.sobe_no_rol());
  }
  // O aceite da tarefa pela porta da tela: fechar e abrir de novo, e a ordem é a
  // que se deixou.
  nu::Roleiro outra_vez(coval.banco());
  tui::Navegador depois(livraria, &outra_vez);
  depois.mostra_rois();
  REQUIRE(depois.vista().size() == 1);
  CHECK(depois.vista()[0].numero == 3);  // tres faixas
  REQUIRE_FALSE(depois.entra());
  REQUIRE(depois.vista().size() == 3);
  CHECK(depois.vista()[0].texto == "1.mp3");
  CHECK(depois.vista()[1].texto == "3.mp3");
  CHECK(depois.vista()[2].texto == "2.mp3");
}

TEST_CASE("juntar do ACERVO á lista alvo, que é o caminho de quem usa a cousa") {
  Cova cova;
  CovaDoRol coval;
  REQUIRE(enche(cova.banco()));
  nu::Biblioteca livraria(cova.banco());
  nu::Roleiro roleiro(coval.banco());
  tui::Navegador navegador(livraria, &roleiro);

  // O caminho de verdade, passo por passo: cria-se a lista, entra-se n'ella para a
  // eleger por alvo, volta-se ao acervo, desce-se até uma faixa, e junta-se.
  REQUIRE(navegador.cria_rol("Da manhã"));
  REQUIRE_FALSE(navegador.entra());
  REQUIRE(navegador.volta());
  REQUIRE(navegador.volta());
  REQUIRE(navegador.secao() == tui::Secao::Artistas);
  REQUIRE_FALSE(navegador.entra());  // no artista
  REQUIRE_FALSE(navegador.entra());  // no album
  REQUIRE(navegador.secao() == tui::Secao::Faixas);
  const std::string primeira = navegador.caminho_eleito();
  REQUIRE_FALSE(primeira.empty());
  REQUIRE(navegador.junta_ao_rol(primeira));
  navegador.desce();
  REQUIRE(navegador.junta_ao_rol(navegador.caminho_eleito()));

  // A lista tem as duas, na ordem em que se juntaram.
  navegador.mostra_rois();
  REQUIRE(navegador.vista().size() == 1);
  CHECK(navegador.vista()[0].numero == 2);
  REQUIRE_FALSE(navegador.entra());
  REQUIRE(navegador.vista().size() == 2);
  CHECK(navegador.vista()[0].chave == primeira);
}

// ── A ENTRADA PELA BARRA (issue #80) ────────────────────────────────────────
// O vai_para prova-se pelas duas metades do contracto: no chão entra-se de
// novo, como a tecla de atalho faria; sem chão devolve-se falso e NADA muda.

TEST_CASE("a barra entra no topo e na busca do acervo inteiro") {
  const Cova cova;
  REQUIRE(enche(cova.banco()));
  const nu::Biblioteca livraria(cova.banco());
  tui::Navegador navegador(livraria);
  REQUIRE_FALSE(navegador.entra());  // no artista
  REQUIRE_FALSE(navegador.entra());  // no album: secção das faixas
  REQUIRE(navegador.secao() == tui::Secao::Faixas);
  navegador.desce();
  CHECK(navegador.vai_para(tui::Secao::Artistas));
  CHECK(navegador.secao() == tui::Secao::Artistas);
  CHECK(navegador.trilha().empty());
  CHECK(navegador.eleito() == 0);
  // A busca com termo vazio é o acervo PLANO: as quatro faixas, e o filtro
  // refina d'ahi, que é o SEARCH da barra.
  CHECK(navegador.vai_para(tui::Secao::Busca));
  CHECK(navegador.vista().size() == 4);
  navegador.filtra("fuga");
  REQUIRE(navegador.vista().size() == 1);
  CHECK(navegador.vista()[0].texto == "Fuga");
}

TEST_CASE("a barra re-entra nos albuns e nas faixas pela trilha corrente") {
  const Cova cova;
  REQUIRE(enche(cova.banco()));
  const nu::Biblioteca livraria(cova.banco());
  tui::Navegador navegador(livraria);
  REQUIRE_FALSE(navegador.entra());
  REQUIRE_FALSE(navegador.entra());
  REQUIRE(navegador.secao() == tui::Secao::Faixas);
  navegador.desce();
  // Re-entrar na secção onde se está é entrar de novo: o eleito ao alto.
  CHECK(navegador.vai_para(tui::Secao::Faixas));
  CHECK(navegador.eleito() == 0);
  CHECK(navegador.vai_para(tui::Secao::Albuns));
  CHECK(navegador.secao() == tui::Secao::Albuns);
  CHECK(textos(navegador) == std::vector<std::string>{"Máquina", "Notas"});
  CHECK(navegador.trilha() == std::vector<std::string>{"Ada Lovelace"});
}

TEST_CASE("o degrau sem chão recusa sem mudar cousa alguma") {
  const Cova cova;
  REQUIRE(enche(cova.banco()));
  const nu::Biblioteca livraria(cova.banco());
  tui::Navegador navegador(livraria);
  const std::vector<std::string> antes = textos(navegador);
  // No topo não ha artista na trilha, rede buscada nem catalogo importado.
  CHECK_FALSE(navegador.vai_para(tui::Secao::Albuns));
  CHECK_FALSE(navegador.vai_para(tui::Secao::Faixas));
  CHECK_FALSE(navegador.vai_para(tui::Secao::Rede));
  CHECK_FALSE(navegador.vai_para(tui::Secao::Lista));
  CHECK_FALSE(navegador.vai_para(tui::Secao::NoRol));
  CHECK(navegador.secao() == tui::Secao::Artistas);
  CHECK(textos(navegador) == antes);
  CHECK(navegador.eleito() == 0);
}

TEST_CASE("a rede e o catalogo dão chão quando as fontes chegam") {
  const Cova cova;
  REQUIRE(enche(cova.banco()));
  const nu::Biblioteca livraria(cova.banco());
  tui::Navegador navegador(livraria);
  navegador.mostra_rede({achado("Toccata", "Canal A", 90, "https://y/1")});
  REQUIRE(navegador.volta());  // sahe-se da rede, e os achados FICAM
  REQUIRE(navegador.secao() == tui::Secao::Artistas);
  // Entrar pela barra mostra o que JÁ havia: busca alguma se re-dispara.
  CHECK(navegador.vai_para(tui::Secao::Rede));
  REQUIRE(navegador.vista().size() == 1);
  CHECK(navegador.vista()[0].texto == "Toccata");
  nu::Catalogo lista;
  lista.nome = "mix da prova";
  lista.faixas.push_back({"Um", "Alguem", 1, 90000, ""});
  navegador.mostra_catalogo(lista);
  REQUIRE(navegador.volta());
  CHECK(navegador.vai_para(tui::Secao::Lista));
  CHECK(navegador.secao() == tui::Secao::Lista);
  CHECK(navegador.vista().size() == 1);
}

TEST_CASE("a barra e a tecla P levam á mesma lista das listas") {
  const Cova cova;
  const CovaDoRol coval;
  REQUIRE(enche(cova.banco()));
  const nu::Biblioteca livraria(cova.banco());
  nu::Roleiro roleiro(coval.banco());
  tui::Navegador pela_barra(livraria, &roleiro);
  tui::Navegador pela_tecla(livraria, &roleiro);
  REQUIRE(pela_barra.cria_rol("viagem"));
  REQUIRE(pela_barra.volta());
  CHECK(pela_barra.vai_para(tui::Secao::Rois));
  pela_tecla.mostra_rois();  // o caminho do «P», tal e qual
  CHECK(pela_barra.secao() == pela_tecla.secao());
  CHECK(textos(pela_barra) == textos(pela_tecla));
}

// ── A ENTRADA N'UMA LISTA PELO ID (issue #93) ───────────────────────────────

TEST_CASE("as listas que a barra lê vêm do banco, e entrar n'uma pelo id abre-a") {
  const Cova cova;
  const CovaDoRol coval;
  REQUIRE(enche(cova.banco()));
  const nu::Biblioteca livraria(cova.banco());
  nu::Roleiro roleiro(coval.banco());
  tui::Navegador navegador(livraria, &roleiro);
  REQUIRE(navegador.cria_rol("da manhã"));
  REQUIRE(navegador.cria_rol("da noite"));
  REQUIRE(navegador.rois().size() == 2);
  CHECK(navegador.rois()[0].nome == "da manhã");  // a ordem é a do banco
  const int qual = navegador.rois()[0].id;
  navegador.ao_fim();           // a segunda, para o alvo ser o OUTRO
  REQUIRE_FALSE(navegador.entra());
  REQUIRE(navegador.junta_ao_rol("/a/1.mp3"));
  REQUIRE(navegador.vai_para(tui::Secao::Artistas));
  // Entra-se na PRIMEIRA sem passar pela lista das listas, e ella vira o alvo.
  CHECK(navegador.vai_para_rol(qual));
  CHECK(navegador.secao() == tui::Secao::NoRol);
  CHECK(navegador.rol_corrente() == qual);
  CHECK(navegador.nome_corrente() == "da manhã");
  CHECK(navegador.trilha() == std::vector<std::string>{"da manhã"});
  CHECK(navegador.vista().empty());  // a faixa foi para a outra
}

TEST_CASE("a lista que já não existe recusa a entrada, e nada se muta") {
  const Cova cova;
  const CovaDoRol coval;
  REQUIRE(enche(cova.banco()));
  const nu::Biblioteca livraria(cova.banco());
  tui::Navegador sem_roleiro(livraria);
  CHECK(sem_roleiro.rois().empty());  // punho nullo: corrida sem listas
  CHECK_FALSE(sem_roleiro.vai_para_rol(1));
  nu::Roleiro roleiro(coval.banco());
  tui::Navegador navegador(livraria, &roleiro);
  const std::vector<std::string> antes = textos(navegador);
  CHECK_FALSE(navegador.vai_para_rol(0));
  CHECK_FALSE(navegador.vai_para_rol(9999));
  CHECK(navegador.secao() == tui::Secao::Artistas);
  CHECK(textos(navegador) == antes);
}

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
