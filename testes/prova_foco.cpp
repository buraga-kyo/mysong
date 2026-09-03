// ══════════════════════════════════════════════════════════════════════════
//   PROVA DO FOCO — testes/prova_foco.cpp
// ══════════════════════════════════════════════════════════════════════════
// A taboada das setas (issue #107) sobre uma geometria FIXA: as caixas são as
// da tela d'elle, de 167 por 67, escriptas á mão. Prova-se cada seta de cada
// peça, inclusive a que não tem candidata; que o Enter aperta o MESMO gesto
// que o clique; e, em écran de papel, que a peça com foco accende.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <ftxui/component/mouse.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/pixel.hpp>
#include <ftxui/screen/screen.hpp>

#include <optional>
#include <string>
#include <vector>

#include "tui/cabecalho.hpp"
#include "tui/foco.hpp"
#include "tui/rato.hpp"
#include "tui/tokens.hpp"

namespace tk = mysong::tui::tokens;
namespace tui = mysong::tui;
using tui::Direcao;
using tui::Focavel;

namespace {

// A TELA D'ELLE, de 167 por 67, em caixas escriptas á mão. As da fita são as
// que a prova da linha já afere cella a cella, com a fita no PÉ (issue #125):
// duas fileiras, a 64 e a 65, e o grupo das abas a principiar na collunha 64.
// As do corpo sahem da sala: pauta de 83 collunhas á esquerda, painel de 83 á
// direita, e a capa no alto d'elle. Numero algum d'aqui se adivinha.
tui::CaixasDaTela tela_d_elle() {
  tui::CaixasDaTela caixas;
  tui::CaixasDoCabecalho& pe = caixas.cabecalho;
  // A ordem d'elle (issue #134): as abas, os botões, o meio (a onda, que é o
  // trilho), e a ponta direita. As collunhas são as da fita pintada em 167.
  pe.aba_mysong = {0, 10, 64, 65};
  pe.aba_playlists = {12, 24, 64, 65};
  pe.aba_download = {26, 37, 64, 65};
  pe.botao_tocar = {39, 41, 64, 65};
  pe.botao_anterior = {43, 45, 64, 65};
  pe.botao_seguinte = {47, 49, 64, 65};
  pe.trilho = {51, 105, 64, 65};
  pe.tempo = {107, 121, 64, 65};
  pe.volume = {123, 130, 64, 65};
  pe.embaralhar = {132, 145, 64, 65};
  pe.repetir = {147, 157, 64, 65};
  pe.ajuda = {159, 166, 64, 65};
  caixas.pauta = {0, 82, 1, 62};
  for (int i = 0; i < 5; ++i) caixas.linhas.push_back({0, 82, 1 + i, 1 + i});
  caixas.capa = {84, 166, 0, 27};
  return caixas;
}

// salto_de — o atalho que faz a taboada caber n'uma linha por caso.
Focavel salto_de(Focavel d_onde, Direcao rumo) {
  return tui::salto(tela_d_elle(), d_onde, rumo);
}

ftxui::Screen papel(ftxui::Element quadro, int largura, int altura = 1) {
  ftxui::Screen ecran = ftxui::Screen::Create(ftxui::Dimension::Fixed(largura),
                                              ftxui::Dimension::Fixed(altura));
  ftxui::Render(ecran, quadro);
  return ecran;
}

ftxui::Color cor(std::string_view token) {
  const tk::Triade c = tk::rgb(token);
  return ftxui::Color::RGB(c.r, c.g, c.b);
}

tui::Retracto tocando() {
  tui::Retracto d_ella;
  d_ella.estado = mysong::nucleo::Estado::Tocando;
  d_ella.posicao = 19.0;
  d_ella.duracao = 189.0;
  d_ella.volume = 100;
  return d_ella;
}

}  // namespace

TEST_CASE("as quatro setas dizem o rumo, e as outras teclas ficam alheias") {
  CHECK(tui::rumo_da_tecla(ftxui::Event::ArrowUp) == Direcao::Cima);
  CHECK(tui::rumo_da_tecla(ftxui::Event::ArrowDown) == Direcao::Baixo);
  CHECK(tui::rumo_da_tecla(ftxui::Event::ArrowLeft) == Direcao::Esquerda);
  CHECK(tui::rumo_da_tecla(ftxui::Event::ArrowRight) == Direcao::Dextra);
  // O `j` e o `k` NÃO são setas: elles andam na lista, e quem os cumpre é a
  // taboada do commando. Alheio aqui é o que os deixa seguir para lá.
  for (const ftxui::Event& qual :
       {ftxui::Event::Character('j'), ftxui::Event::Character('k'),
        ftxui::Event::Return, ftxui::Event::Escape, ftxui::Event::Tab})
    CHECK(tui::rumo_da_tecla(qual) == Direcao::Nenhuma);
}

TEST_CASE("o `↓` da pauta desce á fita, e o `←` d'ella não sahe") {
  // Desce ao segmento que está POR BAIXO do CENTRO d'ella: a pauta toma as 83
  // primeiras collunhas, e o centro cae na 41, que é o botão de tocar (issue
  // #134). As abas ficam á esquerda d'elle, a um `←` de distancia.
  CHECK(salto_de(Focavel::Pauta, Direcao::Baixo) == Focavel::Tocar);
  // Á esquerda da pauta não ha visinha alguma. É esta linha que diz que a seta
  // esquerda já não volta degrau algum: ella nem sequer move o foco.
  CHECK(salto_de(Focavel::Pauta, Direcao::Esquerda) == Focavel::Pauta);
  // Á direita está o painel, e n'elle a capa, que é o botão de pausa.
  CHECK(salto_de(Focavel::Pauta, Direcao::Dextra) == Focavel::Capa);
  // Acima da pauta não ha peça alguma: o foco FICA. (Na janella, o `↑` alli
  // anda na LISTA, e nem chega a pedir salto.)
  CHECK(salto_de(Focavel::Pauta, Direcao::Cima) == Focavel::Pauta);
}

TEST_CASE("as setas de lado percorrem o cabeçalho de ponta a ponta") {
  const Focavel fita[11] = {
      Focavel::AbaMySong, Focavel::AbaPlaylists, Focavel::AbaDownload,
      Focavel::Tocar,     Focavel::Anterior,     Focavel::Seguinte,
      Focavel::Trilho,    Focavel::Volume,       Focavel::Embaralhar,
      Focavel::Repetir,   Focavel::Ajuda};
  for (int i = 0; i + 1 < 11; ++i) {
    CHECK(salto_de(fita[i], Direcao::Dextra) == fita[i + 1]);
    CHECK(salto_de(fita[i + 1], Direcao::Esquerda) == fita[i]);
  }
  // As duas PONTAS não dão a volta: sem candidata, o foco fica. Dar a volta
  // levaria o olho ao canto opposto d'onde elle olhava.
  CHECK(salto_de(Focavel::AbaMySong, Direcao::Esquerda) == Focavel::AbaMySong);
  CHECK(salto_de(Focavel::Ajuda, Direcao::Dextra) == Focavel::Ajuda);
  // E abaixo da fita não ha nada, que o rodapé das dicas não recebe foco: as
  // onze ficam onde estão.
  for (const Focavel qual : fita)
    CHECK(salto_de(qual, Direcao::Baixo) == qual);
}

TEST_CASE("o `↑` da fita torna ao corpo que cada segmento tem por cima") {
  // As abas, os botões e o meio têm a PAUTA por cima, e é a ella que sobem:
  // o meio cruza tambem a capa, mas a pauta está mais perto d'elle.
  for (const Focavel qual :
       {Focavel::AbaMySong, Focavel::AbaPlaylists, Focavel::AbaDownload,
        Focavel::Tocar, Focavel::Anterior, Focavel::Seguinte, Focavel::Trilho})
    CHECK(salto_de(qual, Direcao::Cima) == Focavel::Pauta);
  // Os tres da direita têm a CAPA, que mora no painel por cima d'elles. Não é
  // capricho: a capa está mesmo alli, e mandá-los á pauta faria a seta saltar
  // meia tela por cima do que ella tem em frente.
  for (const Focavel qual : {Focavel::Volume, Focavel::Embaralhar,
                             Focavel::Repetir, Focavel::Ajuda})
    CHECK(salto_de(qual, Direcao::Cima) == Focavel::Capa);
  // E da capa desce-se á fita, e vae-se á pauta pelo lado.
  CHECK(salto_de(Focavel::Capa, Direcao::Baixo) == Focavel::Volume);
  CHECK(salto_de(Focavel::Capa, Direcao::Esquerda) == Focavel::Pauta);
  CHECK(salto_de(Focavel::Capa, Direcao::Dextra) == Focavel::Capa);
  CHECK(salto_de(Focavel::Capa, Direcao::Cima) == Focavel::Capa);
}

TEST_CASE("o meio da fita anda entre os botões e o volume, e sobe á pauta") {
  CHECK(salto_de(Focavel::Trilho, Direcao::Cima) == Focavel::Pauta);
  CHECK(salto_de(Focavel::Trilho, Direcao::Baixo) == Focavel::Trilho);
  CHECK(salto_de(Focavel::Trilho, Direcao::Esquerda) == Focavel::Seguinte);
  CHECK(salto_de(Focavel::Trilho, Direcao::Dextra) == Focavel::Volume);
}

TEST_CASE("peça por pintar não recebe foco nem o dá") {
  // Tela de nascença: caixa alguma se pintou. Seta alguma move o foco, e é
  // estructural: caixa vazia não é candidata, e a corrente vazia nem procura.
  const tui::CaixasDaTela nascida;
  for (const Direcao rumo :
       {Direcao::Cima, Direcao::Baixo, Direcao::Esquerda, Direcao::Dextra})
    CHECK(tui::salto(nascida, Focavel::Pauta, rumo) == Focavel::Pauta);
  // Tela ESTREITA: sem painel não ha capa, e o `→` da pauta fica onde está.
  tui::CaixasDaTela sem_painel = tela_d_elle();
  sem_painel.capa = tui::caixa_por_pintar();
  CHECK(tui::salto(sem_painel, Focavel::Pauta, Direcao::Dextra) ==
        Focavel::Pauta);
  // E o foco que estava n'ella TORNA Á PAUTA, em vez de ficar preso: peça sem
  // caixa não tem candidata alguma, e as quatro setas ficariam mudas.
  for (const Direcao rumo :
       {Direcao::Cima, Direcao::Baixo, Direcao::Esquerda, Direcao::Dextra})
    CHECK(tui::salto(sem_painel, Focavel::Capa, rumo) == Focavel::Pauta);
  // E rumo nenhum não mexe em cousa alguma.
  CHECK(salto_de(Focavel::Tocar, Direcao::Nenhuma) == Focavel::Tocar);
}

TEST_CASE("cada peça com foco aperta o mesmo alvo que o clique n'ella") {
  const tui::CaixasDaTela caixas = tela_d_elle();
  const Focavel botoes[10] = {
      Focavel::AbaMySong, Focavel::AbaPlaylists, Focavel::AbaDownload,
      Focavel::Tocar,     Focavel::Anterior,     Focavel::Seguinte,
      Focavel::Volume,    Focavel::Embaralhar,   Focavel::Repetir,
      Focavel::Ajuda};
  for (const Focavel qual : botoes) {
    const ftxui::Box d_ella = tui::caixa_da_peca(caixas, qual);
    const tui::Alvo pelo_dedo =
        tui::alvo_do_ponto(caixas, d_ella.x_min, d_ella.y_min);
    const tui::Alvo pela_tecla = tui::alvo_do_foco(qual);
    CHECK(pela_tecla.peca == pelo_dedo.peca);
    CHECK(pela_tecla.indice == pelo_dedo.indice);
  }
  // A CAPA é o botão de pausa e retoma, como no rato.
  CHECK(tui::alvo_do_foco(Focavel::Capa).peca == tui::Peca::Capa);
  CHECK(tui::alvo_do_ponto(caixas, 100, 10).peca == tui::Peca::Capa);
  // As DUAS que a tecla não aperta. A pauta tem taboada propria (Enter toca,
  // Espaço pausa); e o trilho pede a collunha em que o dedo pousou, que tecla
  // alguma carrega: buscar o segundo zero seria affirmar o principio da faixa
  // por um Enter que ninguem pediu.
  CHECK(tui::alvo_do_foco(Focavel::Pauta).peca == tui::Peca::Nada);
  CHECK(tui::alvo_do_foco(Focavel::Trilho).peca == tui::Peca::Nada);
}

TEST_CASE("o Enter na peça com foco desagua no gesto do clique") {
  const tui::EstadoDoRato estado{false, 3, 42, 189.0};
  const auto gesto = [&estado](Focavel qual) {
    return tui::gesto_do_alvo(tui::alvo_do_foco(qual), ftxui::Mouse::Left,
                              ftxui::Mouse::Pressed, estado)
        .gesto;
  };
  CHECK(gesto(Focavel::AbaMySong) == tui::Gesto::VaiParaAba);
  CHECK(gesto(Focavel::AbaPlaylists) == tui::Gesto::VaiParaAba);
  CHECK(tui::gesto_do_alvo(tui::alvo_do_foco(Focavel::AbaPlaylists),
                           ftxui::Mouse::Left, ftxui::Mouse::Pressed, estado)
            .indice == 1);
  CHECK(gesto(Focavel::AbaDownload) == tui::Gesto::VaiParaAba);
  CHECK(gesto(Focavel::Tocar) == tui::Gesto::PausaOuRetoma);
  CHECK(gesto(Focavel::Capa) == tui::Gesto::PausaOuRetoma);
  CHECK(gesto(Focavel::Anterior) == tui::Gesto::Anterior);
  CHECK(gesto(Focavel::Seguinte) == tui::Gesto::Proxima);
  CHECK(gesto(Focavel::Volume) == tui::Gesto::Muda);
  CHECK(gesto(Focavel::Embaralhar) == tui::Gesto::Embaralha);
  CHECK(gesto(Focavel::Repetir) == tui::Gesto::Repete);
  CHECK(gesto(Focavel::Ajuda) == tui::Gesto::Ajuda);
  CHECK(gesto(Focavel::Pauta) == tui::Gesto::Nada);
  CHECK(gesto(Focavel::Trilho) == tui::Gesto::Nada);
}

TEST_CASE("o segmento com foco accende em glow_core com texto panel") {
  // Uma collunha DE DENTRO de cada segmento, na linha do alto. São as mesmas
  // que a prova da linha do alto já afere, e por isso não se adivinham.
  const struct {
    Focavel peca;
    int collunha;
  } onde[10] = {{Focavel::AbaMySong, 2},    {Focavel::AbaPlaylists, 14},
                {Focavel::AbaDownload, 28}, {Focavel::Tocar, 40},
                {Focavel::Anterior, 44},    {Focavel::Seguinte, 48},
                {Focavel::Volume, 125},     {Focavel::Embaralhar, 134},
                {Focavel::Repetir, 149},    {Focavel::Ajuda, 162}};
  for (const auto& qual : onde) {
    const ftxui::Screen tela =
        papel(tui::elemento_do_cabecalho(tocando(), tui::Aba::MySong, {}, 167,
                                         nullptr, qual.peca),
              167);
    const ftxui::Pixel& cella = tela.PixelAt(qual.collunha, 0);
    CHECK(cella.background_color == cor(tk::glow_core));
    CHECK(cella.foreground_color == cor(tk::panel));
  }
}

TEST_CASE("o foco na aba corrente ganha da corrente, e as visinhas não mudam") {
  const ftxui::Screen tela =
      papel(tui::elemento_do_cabecalho(tocando(), tui::Aba::MySong, {}, 167,
                                       nullptr, Focavel::AbaMySong),
            167);
  // A aba é a corrente E tem o foco: pinta-se de FOCO. Quem anda com as setas
  // ha de ver onde a mão está, e onde se ESTÁ di-lo tambem a chapa da pauta.
  CHECK(tela.PixelAt(2, 0).background_color == cor(tk::glow_core));
  // As outras duas ficam no repouso do chrome, e os botões no panel_hi: o foco
  // accende UMA peça, e nunca a linha toda.
  CHECK(tela.PixelAt(14, 0).background_color == cor(tk::raised));
  CHECK(tela.PixelAt(40, 0).background_color == cor(tk::panel_hi));
  // E o estado da aba di-lo sem se pintar cousa alguma: é por este enum que a
  // irmã do letreiro (issue #108) escolhe a chapa em XIROD.
  CHECK(tui::estado_da_aba(tui::Aba::MySong, tui::Aba::MySong,
                           Focavel::AbaMySong) == tui::EstadoDaAba::ComFoco);
  CHECK(tui::estado_da_aba(tui::Aba::MySong, tui::Aba::MySong,
                           Focavel::Pauta) == tui::EstadoDaAba::Corrente);
  CHECK(tui::estado_da_aba(tui::Aba::Download, tui::Aba::MySong,
                           Focavel::Pauta) == tui::EstadoDaAba::Apagada);
}

TEST_CASE("a aba com foco governa tambem a chapa em XIROD que a cobre") {
  CHECK(tui::aba_com_foco(Focavel::AbaPlaylists) == tui::Aba::Playlists);
  CHECK(tui::aba_com_foco(Focavel::AbaDownload) == tui::Aba::Download);
  // Peça focavel que não é aba não dá aba alguma: é o vazio que o
  // `ordens_das_chapas` já sabe ler.
  CHECK_FALSE(tui::aba_com_foco(Focavel::Pauta).has_value());
  CHECK_FALSE(tui::aba_com_foco(Focavel::Volume).has_value());
  // A chapa da aba focada sae do MESMO degrau que pinta a cella debaixo
  // d'ella (issue #108 casada com a #107): glow_core com tinta panel, e ganha
  // da corrente ainda quando a aba é as duas cousas.
  const tui::CaixasDaTela caixas = tela_d_elle();
  const std::optional<tui::Aba> focada = tui::aba_com_foco(Focavel::AbaMySong);
  const std::vector<tui::ChapaDaAba> com = tui::ordens_das_chapas(
      caixas.cabecalho, tui::Aba::MySong, true, true,
      focada ? &*focada : nullptr);
  REQUIRE(com.size() == 3);
  CHECK(com[0].estado == tui::EstadoDaAba::ComFoco);
  CHECK(com[0].poe);
  CHECK(tui::pedido_da_chapa(com[0]).fundo == std::string(tk::glow_core));
  CHECK(tui::pedido_da_chapa(com[0]).tinta == std::string(tk::panel));
  // Sem punho, a mesma aba é a CORRENTE, e a chapa sae no bloco de violeta.
  const std::vector<tui::ChapaDaAba> sem =
      tui::ordens_das_chapas(caixas.cabecalho, tui::Aba::MySong, true, true);
  CHECK(sem[0].estado == tui::EstadoDaAba::Corrente);
  CHECK(tui::pedido_da_chapa(sem[0]).fundo == std::string(tk::v600));
}

TEST_CASE("o meio da fita com foco accende o andado, e o que falta fica quieto") {
  tui::Retracto meio = tocando();
  meio.posicao = 94.5;  // metade de 189: metade das 55 collunhas do meio
  const ftxui::Screen quieto =
      papel(tui::elemento_do_cabecalho(meio, tui::Aba::MySong, {}, 167), 167);
  const ftxui::Screen aceso =
      papel(tui::elemento_do_cabecalho(meio, tui::Aba::MySong, {}, 167, nullptr,
                                       Focavel::Trilho),
            167);
  CHECK(quieto.PixelAt(51, 0).foreground_color == cor(tk::v600));
  CHECK(aceso.PixelAt(51, 0).foreground_color == cor(tk::glow_core));
  // O que FALTA não accende: linha inteira em glow deixaria de dizer por onde
  // a faixa vae, que é o officio do trilho.
  CHECK(aceso.PixelAt(105, 0).foreground_color == cor(tk::line_dim));
}

TEST_CASE("a capa com foco ganha orla de glow_core") {
  const ftxui::Screen tela =
      papel(tui::orla_do_foco(ftxui::text("arte") |
                              ftxui::size(ftxui::WIDTH, ftxui::EQUAL, 4) |
                              ftxui::size(ftxui::HEIGHT, ftxui::EQUAL, 1)),
            6, 3);
  // Os quatro cantos do quadro, e a tinta d'elles.
  CHECK(tela.PixelAt(0, 0).character == "╭");
  CHECK(tela.PixelAt(5, 0).character == "╮");
  CHECK(tela.PixelAt(0, 2).character == "╰");
  CHECK(tela.PixelAt(5, 2).character == "╯");
  CHECK(tela.PixelAt(0, 0).foreground_color == cor(tk::glow_core));
  CHECK(tela.PixelAt(3, 0).foreground_color == cor(tk::glow_core));
  // E o que a orla guarda fica INTACTO por dentro d'ella.
  CHECK(tela.PixelAt(1, 1).character == "a");
}

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
