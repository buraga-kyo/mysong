// ══════════════════════════════════════════════════════════════════════════
//   PROVA DO RATO, testes/prova_rato.cpp
// ══════════════════════════════════════════════════════════════════════════
// As duas taboadas do rato (issue #95), sem terminal e sem tela: as caixas
// armam-se á mão, com as coordenadas escriptas, e o que se afere é o alvo que
// o ponto acha e o gesto que o alvo pede. É ella que apanha a collunha trocada.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <ftxui/component/mouse.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/box.hpp>
#include <ftxui/screen/screen.hpp>

#include <unistd.h>

#include <filesystem>
#include <string>
#include <system_error>

#include "nucleo/biblioteca.hpp"
#include "tui/cabecalho.hpp"
#include "tui/rato.hpp"
#include "tui/tabella.hpp"
#include "tui/transporte.hpp"
#include "tui/vigilia.hpp"

namespace tui = mysong::tui;
using ftxui::Mouse;

TEST_CASE("a caixa por pintar não casa com ponto algum") {
  const ftxui::Box vazia = tui::caixa_por_pintar();
  CHECK(vazia.IsEmpty());
  CHECK_FALSE(vazia.Contain(0, 0));
  // E a de omissão do FTXUI CASA com o canto: é d'esta medida que a regra
  // nasce, e é ella que faria o primeiro clique acertar a tela toda.
  CHECK(ftxui::Box{}.Contain(0, 0));
  const tui::CaixasDaTela nascida;
  CHECK(tui::alvo_do_ponto(nascida, 0, 0).peca == tui::Peca::Nada);
}

namespace {

// A tela de mentira: o cabeçalho na fileira zero, o trilho na um, cinco linhas
// de pauta, e a capa n'um quadro á direita. Os numeros são arbitrarios: o que
// se prova é a geometria, e não a composição.
tui::CaixasDaTela tela_de_mentira() {
  tui::CaixasDaTela caixas;
  tui::CaixasDoCabecalho& alto = caixas.cabecalho;
  alto.aba_mysong = {0, 9, 0, 0};
  alto.aba_playlists = {10, 21, 0, 0};
  alto.aba_download = {22, 32, 0, 0};
  alto.botao_tocar = {33, 35, 0, 0};
  alto.botao_anterior = {36, 38, 0, 0};
  alto.botao_seguinte = {39, 41, 0, 0};
  alto.tempo = {61, 75, 0, 0};
  alto.volume = {76, 84, 0, 0};
  alto.embaralhar = {85, 97, 0, 0};
  alto.repetir = {98, 107, 0, 0};
  alto.ajuda = {108, 115, 0, 0};
  alto.trilho = {11, 30, 1, 1};
  for (int i = 0; i < 5; ++i) caixas.linhas.push_back({11, 60, 3 + i, 3 + i});
  caixas.primeira_linha = 20;
  caixas.capa = {62, 80, 3, 12};
  return caixas;
}

}  // namespace

TEST_CASE("cada peça da tela responde pelo seu ponto") {
  const tui::CaixasDaTela caixas = tela_de_mentira();
  // As tres abas, pela ordem em que a fita as põe.
  for (int i = 0; i < 3; ++i) {
    const tui::Alvo aba = tui::alvo_do_ponto(caixas, 5 + 12 * i, 0);
    CHECK(aba.peca == tui::Peca::Aba);
    CHECK(aba.indice == static_cast<std::size_t>(i));
  }
  // A linha sahe em indice ABSOLUTO: a segunda á vista, com vinte de rolagem.
  const tui::Alvo linha = tui::alvo_do_ponto(caixas, 30, 4);
  CHECK(linha.peca == tui::Peca::Linha);
  CHECK(linha.indice == 21);
  CHECK(tui::alvo_do_ponto(caixas, 70, 8).peca == tui::Peca::Capa);
  CHECK(tui::alvo_do_ponto(caixas, 34, 0).peca == tui::Peca::Pausa);
  CHECK(tui::alvo_do_ponto(caixas, 37, 0).peca == tui::Peca::Anterior);
  CHECK(tui::alvo_do_ponto(caixas, 40, 0).peca == tui::Peca::Proxima);
  CHECK(tui::alvo_do_ponto(caixas, 90, 0).peca == tui::Peca::Embaralhar);
  CHECK(tui::alvo_do_ponto(caixas, 100, 0).peca == tui::Peca::Repetir);
  CHECK(tui::alvo_do_ponto(caixas, 110, 0).peca == tui::Peca::Ajuda);
  // O nome e o tempo não respondem: elles dizem, e não fazem.
  CHECK(tui::alvo_do_ponto(caixas, 50, 0).peca == tui::Peca::Nada);
  CHECK(tui::alvo_do_ponto(caixas, 65, 0).peca == tui::Peca::Nada);
  // Fóra de tudo: a altura que sobra abaixo da lista, e o rodapé.
  CHECK(tui::alvo_do_ponto(caixas, 30, 9).peca == tui::Peca::Nada);
  CHECK(tui::alvo_do_ponto(caixas, 100, 40).peca == tui::Peca::Nada);
}

TEST_CASE("a fracção do trilho vae de zero na primeira collunha a um na ultima") {
  const tui::CaixasDaTela caixas = tela_de_mentira();
  const tui::Alvo principio = tui::alvo_do_ponto(caixas, 11, 1);
  CHECK(principio.peca == tui::Peca::Progresso);
  CHECK(principio.fracao == doctest::Approx(0.0));
  CHECK(tui::alvo_do_ponto(caixas, 30, 1).fracao == doctest::Approx(1.0));
  CHECK(tui::alvo_do_ponto(caixas, 21, 1).fracao ==
        doctest::Approx(10.0 / 19.0));
}

TEST_CASE("o trilho por pintar não casa com ponto algum") {
  // O trilho é UM elemento, e não duas metades como a barra do pé: no
  // principio da faixa nada tem largura zero, e caixa alguma sahe vazia por
  // isso. Vazia sahe sómente quando a linha se não pintou.
  tui::CaixasDaTela caixas = tela_de_mentira();
  caixas.cabecalho.trilho = tui::caixa_por_pintar();
  CHECK(tui::alvo_do_ponto(caixas, 20, 1).peca == tui::Peca::Nada);
}

TEST_CASE("sómente o botão esquerdo a descer governa alguma cousa") {
  const tui::CaixasDaTela caixas = tela_de_mentira();
  const tui::Alvo linha = tui::alvo_do_ponto(caixas, 30, 4);
  const tui::EstadoDoRato estado{false, 21, 40, 200.0};
  // O soltar chega SEMPRE, que o modo 1000 manda o `m` do SGR; sem esta guarda
  // cada clique valeria por dous. A mexida não chega, que o 1003 se não liga.
  for (const Mouse::Motion mexeu : {Mouse::Released, Mouse::Moved})
    CHECK(tui::gesto_do_alvo(linha, Mouse::Left, mexeu, estado).gesto ==
          tui::Gesto::Nada);
  // O do meio não é de issue alguma, e continua mudo. O direito tem officio
  // desde a issue #96, e por isso sahiu d'esta lista: elle abre o menu.
  for (const Mouse::Button qual : {Mouse::Middle, Mouse::None})
    CHECK(tui::gesto_do_alvo(linha, qual, Mouse::Pressed, estado).gesto ==
          tui::Gesto::Nada);
  CHECK(tui::gesto_do_alvo(linha, Mouse::Left, Mouse::Pressed, estado).gesto ==
        tui::Gesto::Toca);
}

namespace {

// clicou, o gesto de um clique esquerdo n'um ponto, que é o que quasi todo
// caso abaixo pergunta. Sem elle, a linha da chamada não cabe na medida.
tui::GestoDoRato clicou(const tui::CaixasDaTela& caixas, int x, int y,
                        const tui::EstadoDoRato& estado) {
  return tui::gesto_do_alvo(tui::alvo_do_ponto(caixas, x, y), Mouse::Left,
                            Mouse::Pressed, estado);
}

}  // namespace

TEST_CASE("com o campo aberto o clique fecha-o, e pára ahi") {
  const tui::CaixasDaTela caixas = tela_de_mentira();
  const tui::EstadoDoRato digita{true, 21, 40, 200.0};
  // A linha, a aba e o botão: TODO alvo dá a mesma cousa, que é o campo a
  // fechar-se. A tela não ha de mudar debaixo de quem está a digitar.
  CHECK(clicou(caixas, 30, 4, digita).gesto == tui::Gesto::FechaCampo);
  CHECK(clicou(caixas, 5, 0, digita).gesto == tui::Gesto::FechaCampo);
  CHECK(clicou(caixas, 34, 0, digita).gesto == tui::Gesto::FechaCampo);
  // E a roda tambem: o rato não escreve no termo por caminho algum.
  CHECK(tui::gesto_do_alvo(tui::alvo_do_ponto(caixas, 30, 4), Mouse::WheelDown,
                           Mouse::Pressed, digita)
            .gesto == tui::Gesto::FechaCampo);
}

TEST_CASE("o clique elege a linha, e o clique na JÁ eleita toca-a") {
  const tui::CaixasDaTela caixas = tela_de_mentira();
  const tui::EstadoDoRato estado{false, 21, 40, 200.0};
  const tui::GestoDoRato outra = clicou(caixas, 30, 6, estado);
  CHECK(outra.gesto == tui::Gesto::Elege);
  CHECK(outra.indice == 23);
  CHECK(clicou(caixas, 30, 4, estado).gesto == tui::Gesto::Toca);
  // A vista encolheu entre a pintura e o clique: não se elege ás cegas.
  const tui::EstadoDoRato curta{false, 0, 21, 200.0};
  CHECK(clicou(caixas, 30, 4, curta).gesto == tui::Gesto::Nada);
}

namespace {

// rodou, o gesto de um dente da roda n'um ponto.
tui::GestoDoRato rodou(const tui::CaixasDaTela& caixas, int x, int y, bool sobe,
                       const tui::EstadoDoRato& estado) {
  return tui::gesto_do_alvo(tui::alvo_do_ponto(caixas, x, y),
                            sobe ? Mouse::WheelUp : Mouse::WheelDown,
                            Mouse::Pressed, estado);
}

}  // namespace

TEST_CASE("a roda anda tres linhas na pauta, e fica muda em toda a mais peça") {
  const tui::CaixasDaTela caixas = tela_de_mentira();
  const tui::EstadoDoRato estado{false, 21, 40, 200.0};
  const tui::GestoDoRato desce = rodou(caixas, 30, 4, false, estado);
  CHECK(desce.gesto == tui::Gesto::RodaDesce);
  CHECK(desce.indice == tui::LINHAS_POR_DENTE);
  CHECK(rodou(caixas, 30, 4, true, estado).gesto == tui::Gesto::RodaSobe);
  // Sobre a barra a roda andava um degrau; sobre uma fita de tres abas ella
  // trocaria de secção por acaso, com o dedo a caminho de outra peça.
  CHECK(rodou(caixas, 5, 0, true, estado).gesto == tui::Gesto::Nada);
  CHECK(rodou(caixas, 5, 0, false, estado).gesto == tui::Gesto::Nada);
  // E fóra da pauta ella não governa cousa alguma: nem volume, nem busca.
  CHECK(rodou(caixas, 34, 0, true, estado).gesto == tui::Gesto::Nada);
  CHECK(rodou(caixas, 70, 8, true, estado).gesto == tui::Gesto::Nada);
}

TEST_CASE("o cabeçalho, a capa e a busca dão o gesto que dizem") {
  const tui::CaixasDaTela caixas = tela_de_mentira();
  const tui::EstadoDoRato estado{false, 21, 40, 200.0};
  CHECK(clicou(caixas, 37, 0, estado).gesto == tui::Gesto::Anterior);
  CHECK(clicou(caixas, 40, 0, estado).gesto == tui::Gesto::Proxima);
  CHECK(clicou(caixas, 34, 0, estado).gesto == tui::Gesto::PausaOuRetoma);
  CHECK(clicou(caixas, 90, 0, estado).gesto == tui::Gesto::Embaralha);
  CHECK(clicou(caixas, 100, 0, estado).gesto == tui::Gesto::Repete);
  // A capa é o mesmo gesto do botão de tocar: quem clica na arte quer calar.
  CHECK(clicou(caixas, 70, 8, estado).gesto == tui::Gesto::PausaOuRetoma);
  const tui::GestoDoRato busca = clicou(caixas, 21, 1, estado);
  CHECK(busca.gesto == tui::Gesto::Busca);
  CHECK(busca.alvo == doctest::Approx(200.0 * 10.0 / 19.0));
  // Sem duração não se busca. Zero seria affirmar o principio, e o que ha é a
  // Casa ainda não saber quanto a faixa dura.
  const tui::EstadoDoRato sem{false, 21, 40, 0.0};
  CHECK(clicou(caixas, 21, 1, sem).gesto == tui::Gesto::Nada);
  const tui::GestoDoRato aba = clicou(caixas, 15, 0, estado);
  CHECK(aba.gesto == tui::Gesto::VaiParaAba);
  CHECK(aba.indice == 1);
}

namespace {

// papel, o écran de PAPEL, com os escapes dentro. Compara-se o `ToString`, e
// não as cellas nuas, de proposito: a côr entra na comparação, e caracter egual
// com tinta differente já seria a caixa a mudar a pintura.
std::string papel(ftxui::Element quadro, int largura, int altura) {
  ftxui::Screen ecran = ftxui::Screen::Create(
      ftxui::Dimension::Fixed(largura), ftxui::Dimension::Fixed(altura));
  ftxui::Render(ecran, quadro);
  return ecran.ToString();
}

}  // namespace

TEST_CASE("a caixa não muda um pixel do cabeçalho") {
  tui::Retracto retracto;
  retracto.estado = mysong::nucleo::Estado::Tocando;
  retracto.posicao = 30.0;
  retracto.duracao = 120.0;
  for (const int largura : {60, 120, 167}) {
    const std::size_t larg = static_cast<std::size_t>(largura);
    tui::CaixasDoCabecalho caixas;
    CHECK(papel(tui::elemento_do_cabecalho(retracto, tui::Aba::MySong, {},
                                           larg),
                largura, 1) ==
          papel(tui::elemento_do_cabecalho(retracto, tui::Aba::MySong, {},
                                           larg, &caixas),
                largura, 1));
    // E as caixas encheram-se: sem isto, a egualdade valeria tambem para quem
    // se esquecesse de as pôr, e a prova não provaria cousa alguma.
    CHECK_FALSE(caixas.aba_mysong.IsEmpty());
    CHECK_FALSE(caixas.botao_tocar.IsEmpty());
    // O MEIO (a onda, que é o trilho) sómente onde ha collunha para elle: as
    // fixas pedem 51, e caixa de largura zero nasce vazia (issue #134).
    if (largura > 51) CHECK_FALSE(caixas.trilho.IsEmpty());
    else CHECK(caixas.trilho.IsEmpty());
  }
}

// O CLIQUE NAS DUAS LINHAS (issue #125). A fita do pé tem duas fileiras, e a
// caixa de cada segmento tem-nas ambas: o dedo não sabe de linhas, e o clique
// na de baixo ha de fazer o que o da de cima faz.
TEST_CASE("o clique em qualquer das duas linhas do segmento faz o mesmo") {
  tui::Retracto retracto;
  tui::CaixasDaTela tela;
  ftxui::Screen ecran = ftxui::Screen::Create(ftxui::Dimension::Fixed(167),
                                              ftxui::Dimension::Fixed(2));
  ftxui::Render(ecran, tui::elemento_do_cabecalho(
                           retracto, tui::Aba::MySong, {}, 167,
                           &tela.cabecalho, tui::Focavel::Pauta, 2));
  REQUIRE(tela.cabecalho.aba_playlists.y_max -
              tela.cabecalho.aba_playlists.y_min == 1);
  for (const int linha : {0, 1}) {
    const tui::Alvo aba = tui::alvo_do_ponto(tela, 14, linha);
    CHECK(aba.peca == tui::Peca::Aba);
    CHECK(aba.indice == 1);
    CHECK(tui::gesto_do_alvo(aba, Mouse::Left, Mouse::Pressed, {}).gesto ==
          tui::Gesto::VaiParaAba);
    // E o botão de tocar responde pelas duas fileiras d'elle tambem.
    CHECK(tui::alvo_do_ponto(tela, 40, linha).peca == tui::Peca::Pausa);
    // A roda fica MUDA sobre a fita, nas duas linhas: a caixa é de duas, e a
    // regra da roda não olha a fileira.
    CHECK(tui::gesto_do_alvo(aba, Mouse::WheelUp, Mouse::Pressed, {}).gesto ==
          tui::Gesto::Nada);
  }
}

TEST_CASE("a caixa não muda um pixel da capa") {
  const mysong::nucleo::CapaPintada sem_capa;  // o marcador do album sem arte
  ftxui::Box caixa;
  CHECK(papel(tui::elemento_da_capa(sem_capa, 20, 11), 20, 11) ==
        papel(tui::elemento_da_capa(sem_capa, 20, 11, &caixa), 20, 11));
  CHECK_FALSE(caixa.IsEmpty());
  // Terminal apertado não mostra capa alguma, e a caixa fica VAZIA em vez de
  // ficar a do quadro anterior a apanhar cliques sobre a tabella.
  tui::elemento_da_capa(sem_capa, 0, 0, &caixa);
  CHECK(caixa.IsEmpty());
}

namespace {

// A cova e o índice: a pauta pede um Navegador, e elle pede uma Bibliotheca.
// O acervo fica VAZIO, e a vista põe-se por `mostra_rede`.
class Cova {
 public:
  Cova() {
    caminho_ = std::filesystem::temp_directory_path() /
               ("mysong-rato-" + std::to_string(::getpid()));
    std::filesystem::create_directories(caminho_);
    mysong::nucleo::Escriba escriba(caminho_ / "indice.sqlite3");
    escriba.conclui();
  }
  ~Cova() { std::error_code erro; std::filesystem::remove_all(caminho_, erro); }
  Cova(const Cova&) = delete;
  Cova& operator=(const Cova&) = delete;
  std::filesystem::path banco() const { return caminho_ / "indice.sqlite3"; }
 private:
  std::filesystem::path caminho_;
};

}  // namespace

TEST_CASE("a caixa não muda um pixel da pauta") {
  Cova cova;
  const mysong::nucleo::Biblioteca livraria(cova.banco());
  mysong::nucleo::Achado um;
  um.titulo = "Toccata";
  tui::Navegador navegador(livraria);  // acervo vasio: a vista vem da rede
  navegador.mostra_rede({um, um, um});
  std::vector<ftxui::Box> linhas;
  CHECK(papel(tui::elemento_da_tabella(navegador, 0, 5, 60), 60, 5) ==
        papel(tui::elemento_da_tabella(navegador, 0, 5, 60, {}, &linhas),
              60, 5));
  // Tres na vista e cinco de altura: caixa alguma para o que se não pintou.
  CHECK(linhas.size() == 3);
}

TEST_CASE("o evento de rato conta por tecla de gente e acorda a vigilia") {
  ftxui::Mouse rato;
  rato.button = Mouse::Left;
  rato.motion = Mouse::Pressed;
  CHECK(tui::eh_tecla_de_gente(ftxui::Event::Mouse("", rato)));
  // O Custom continua de fóra: é a batida do proprio relogio, e batida que
  // acordasse faria o somno da issue #82 impossivel.
  CHECK_FALSE(tui::eh_tecla_de_gente(ftxui::Event::Custom));
  tui::Vigilia vigilia;
  vigilia.perde();
  REQUIRE_FALSE(vigilia.pede_batida());
  vigilia.ganha();  // é o que o ramo do rato faz por esta tecla de gente
  CHECK(vigilia.pede_batida());
  CHECK(vigilia.acordou());
}

TEST_CASE("com a janella do video de pé o clique no trilho fica inerte") {
  const tui::CaixasDaTela caixas = tela_de_mentira();
  // A janella arma o estado com duração ZERO emquanto o video corre: a que o
  // retracto sabe é a do AUDIO pausado, e a janella que corre é a da faixa
  // ELEITA, que nem sempre é a mesma. Buscar por ella seria mandar o video a
  // uma posição contada n'outro arco.
  const tui::EstadoDoRato com_video{false, 21, 40, 0.0};
  CHECK(clicou(caixas, 11, 1, com_video).gesto == tui::Gesto::Nada);
  CHECK(clicou(caixas, 21, 1, com_video).gesto == tui::Gesto::Nada);
  // O resto do transporte SEGUE a governar: o `cumprir` rotea-o á janella.
  CHECK(clicou(caixas, 34, 0, com_video).gesto == tui::Gesto::PausaOuRetoma);
  CHECK(clicou(caixas, 37, 0, com_video).gesto == tui::Gesto::Anterior);
}

TEST_CASE("com o campo aberto o botão direito fecha-o, e menu algum abre") {
  const tui::CaixasDaTela caixas = tela_de_mentira();
  const tui::EstadoDoRato digita{true, 21, 40, 200.0};
  const tui::Alvo linha = tui::alvo_do_ponto(caixas, 30, 4);
  // A guarda do campo vem antes da do menu, e não ao contrario: com o campo de
  // pé a tela não ha de mudar debaixo de quem digita, e menu que abrisse alli
  // tomaria as teclas ao termo em curso a meio de uma palavra.
  for (const Mouse::Button qual : {Mouse::Left, Mouse::Right})
    CHECK(tui::gesto_do_alvo(linha, qual, Mouse::Pressed, digita).gesto ==
          tui::Gesto::FechaCampo);
}

TEST_CASE("o botão direito n'uma linha abre o menu, e sómente n'uma linha") {
  const tui::CaixasDaTela caixas = tela_de_mentira();
  const tui::EstadoDoRato estado{false, 21, 40, 200.0};
  const tui::GestoDoRato d_ella =
      tui::gesto_do_alvo(tui::alvo_do_ponto(caixas, 30, 4), Mouse::Right,
                         Mouse::Pressed, estado);
  CHECK(d_ella.gesto == tui::Gesto::AbreMenu);
  CHECK(d_ella.indice == 21);  // o indice ABSOLUTO, com a rolagem sommada
  // No cabeçalho e na capa não ha faixa alguma de que o menu fosse.
  for (const tui::Alvo alheio : {tui::alvo_do_ponto(caixas, 5, 0),
                                 tui::alvo_do_ponto(caixas, 70, 5)})
    CHECK(tui::gesto_do_alvo(alheio, Mouse::Right, Mouse::Pressed, estado)
              .gesto == tui::Gesto::Nada);
  // E o soltar não abre: elle chega sempre, e o menu piscaria e sumia.
  CHECK(tui::gesto_do_alvo(tui::alvo_do_ponto(caixas, 30, 4), Mouse::Right,
                           Mouse::Released, estado)
            .gesto == tui::Gesto::Nada);
}

// O ARRASTO (issue #153): a máquina que pega, arrasta e larga. Afere-se como
// valores, sem terminal e sem pauta: o que ella promette é a ORDEM dos gestos.
TEST_CASE("o arrasto pega n'uma linha, anda, e larga n'outra") {
  const tui::CaixasDaTela caixas = tela_de_mentira();
  tui::Arrasto mao;
  const auto na_linha = [&caixas](int y) {
    return tui::alvo_do_ponto(caixas, 30, y);
  };
  // O botão desce na terceira linha visivel (indice absoluto 22).
  tui::RespostaDoArrasto d = tui::gesto_do_arrasto(
      mao, na_linha(5), Mouse::Left, Mouse::Pressed, true);
  CHECK(d.gesto == tui::GestoDoArrasto::Pega);
  CHECK(mao.pegou);
  CHECK(mao.origem == 22);
  CHECK_FALSE(mao.andou);
  // A mão anda para a primeira linha visivel (indice 20).
  d = tui::gesto_do_arrasto(mao, na_linha(3), Mouse::Left, Mouse::Moved, true);
  CHECK(d.gesto == tui::GestoDoArrasto::Arrasta);
  CHECK(mao.alvo == 20);
  CHECK(mao.andou);
  // E larga alli: ha movimento a cumprir, de 22 para 20.
  d = tui::gesto_do_arrasto(mao, na_linha(3), Mouse::Left, Mouse::Released, true);
  CHECK(d.gesto == tui::GestoDoArrasto::Larga);
  CHECK(d.de == 22);
  CHECK(d.para == 20);
  CHECK_FALSE(mao.pegou);  // a mão esvazia-se
}

TEST_CASE("largar onde se pegou não move nada, e é o clique de sempre") {
  const tui::CaixasDaTela caixas = tela_de_mentira();
  tui::Arrasto mao;
  const tui::Alvo alvo = tui::alvo_do_ponto(caixas, 30, 4);
  tui::gesto_do_arrasto(mao, alvo, Mouse::Left, Mouse::Pressed, true);
  const tui::RespostaDoArrasto d =
      tui::gesto_do_arrasto(mao, alvo, Mouse::Left, Mouse::Released, true);
  CHECK(d.gesto == tui::GestoDoArrasto::Desiste);
  CHECK_FALSE(mao.pegou);
}

TEST_CASE("a vista que não se arruma não deixa pegar, e o botão direito tambem não") {
  const tui::CaixasDaTela caixas = tela_de_mentira();
  tui::Arrasto mao;
  const tui::Alvo alvo = tui::alvo_do_ponto(caixas, 30, 4);
  // `pode` falso: artistas, albuns e achados da rede.
  CHECK(tui::gesto_do_arrasto(mao, alvo, Mouse::Left, Mouse::Pressed, false)
            .gesto == tui::GestoDoArrasto::Nada);
  CHECK_FALSE(mao.pegou);
  // O botão DIREITO abre o menu, e não arrasta.
  CHECK(tui::gesto_do_arrasto(mao, alvo, Mouse::Right, Mouse::Pressed, true)
            .gesto == tui::GestoDoArrasto::Nada);
  CHECK_FALSE(mao.pegou);
  // O botão que desce FÓRA de linha alguma (na fita) não pega, e esvazia a mão.
  const tui::Alvo fóra = tui::alvo_do_ponto(caixas, 5, 0);
  CHECK(tui::gesto_do_arrasto(mao, fóra, Mouse::Left, Mouse::Pressed, true)
            .gesto == tui::GestoDoArrasto::Nada);
  CHECK_FALSE(mao.pegou);
}

// O DUPLO CLIQUE (issue #169). Todo duplo clique treme, e o tremor cahia n'uma
// linha visinha: o gesto virava ARRASTO, movia a faixa e engolia o clique que
// ia tocá-la. Duas regras o corrigem, e são estas que estes casos prendem: o
// largar lê a linha do PROPRIO evento, e o arrasto pede MAIS de uma linha.
TEST_CASE("o tremor de uma linha não arrasta, e o clique segue") {
  const tui::CaixasDaTela caixas = tela_de_mentira();
  tui::Arrasto mao;
  const auto na_linha = [&caixas](int y) {
    return tui::alvo_do_ponto(caixas, 30, y);
  };
  // Pega na terceira linha visivel (indice 22) e treme para a visinha (21).
  tui::gesto_do_arrasto(mao, na_linha(5), Mouse::Left, Mouse::Pressed, true);
  const tui::RespostaDoArrasto tremeu =
      tui::gesto_do_arrasto(mao, na_linha(4), Mouse::Left, Mouse::Moved, true);
  CHECK(tremeu.gesto == tui::GestoDoArrasto::Arrasta);
  CHECK_FALSE(mao.andou);  // UMA linha não é arrasto: é tremor
  // Largando na visinha, DESISTE: o clique de sempre segue, e é elle que toca.
  const tui::RespostaDoArrasto larga =
      tui::gesto_do_arrasto(mao, na_linha(4), Mouse::Left, Mouse::Released, true);
  CHECK(larga.gesto == tui::GestoDoArrasto::Desiste);
}

TEST_CASE("o largar lê a linha do evento, e não a ultima por onde se passou") {
  const tui::CaixasDaTela caixas = tela_de_mentira();
  tui::Arrasto mao;
  const auto na_linha = [&caixas](int y) {
    return tui::alvo_do_ponto(caixas, 30, y);
  };
  // A pauta de mentira tem cinco linhas (as fileiras 3 a 7, indices 20 a 24).
  // Pega em 20, passa por 23 (tres linhas: arrasto de facto) e TORNA a 20.
  tui::gesto_do_arrasto(mao, na_linha(3), Mouse::Left, Mouse::Pressed, true);
  tui::gesto_do_arrasto(mao, na_linha(6), Mouse::Left, Mouse::Moved, true);
  CHECK(mao.andou);
  const tui::RespostaDoArrasto torna =
      tui::gesto_do_arrasto(mao, na_linha(3), Mouse::Left, Mouse::Released, true);
  // Largou onde pegou: movimento algum, ainda que tenha passado por outra.
  CHECK(torna.gesto == tui::GestoDoArrasto::Desiste);

  // E largando de facto n'outra, é a linha do EVENTO que manda.
  tui::Arrasto outra;
  tui::gesto_do_arrasto(outra, na_linha(3), Mouse::Left, Mouse::Pressed, true);
  tui::gesto_do_arrasto(outra, na_linha(6), Mouse::Left, Mouse::Moved, true);
  const tui::RespostaDoArrasto d = tui::gesto_do_arrasto(
      outra, na_linha(5), Mouse::Left, Mouse::Released, true);
  CHECK(d.gesto == tui::GestoDoArrasto::Larga);
  CHECK(d.de == 20);
  CHECK(d.para == 22);  // a linha em que se largou, e não a 23 por onde passou
}

TEST_CASE("a mão que sae da pauta guarda o alvo que tinha") {
  const tui::CaixasDaTela caixas = tela_de_mentira();
  tui::Arrasto mao;
  tui::gesto_do_arrasto(mao, tui::alvo_do_ponto(caixas, 30, 5), Mouse::Left,
                        Mouse::Pressed, true);
  tui::gesto_do_arrasto(mao, tui::alvo_do_ponto(caixas, 30, 3), Mouse::Left,
                        Mouse::Moved, true);
  CHECK(mao.alvo == 20);
  // Passa por cima da capa: alvo algum se lhe conhece, e o que estava FICA.
  const tui::RespostaDoArrasto d = tui::gesto_do_arrasto(
      mao, tui::alvo_do_ponto(caixas, 70, 8), Mouse::Left, Mouse::Moved, true);
  CHECK(d.gesto == tui::GestoDoArrasto::Nada);
  CHECK(mao.alvo == 20);
  CHECK(mao.pegou);
}

//   Da lavra do eminente Doutor BURAGA KYO., buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
