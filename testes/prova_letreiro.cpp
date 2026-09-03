// ══════════════════════════════════════════════════════════════════════════
//   PROVA DO LETREIRO — testes/prova_letreiro.cpp
// ══════════════════════════════════════════════════════════════════════════
// A linha do pango-view, a margem que casa a proporção, a chave do cache, a
// decisão de haver letreiro, e a ordem das tres chapas sobre um cabeçalho de
// PAPEL. Programa algum se corre, e X11 algum se abre: as cinco cousas são
// funcções puras, e é para isto que ellas o são.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <cstdlib>

#include <ftxui/dom/node.hpp>
#include <ftxui/screen/screen.hpp>

#include <filesystem>
#include <string>
#include <vector>

#include "nucleo/letreiro.hpp"
#include "tui/cabecalho.hpp"

namespace nu = mysong::nucleo;
namespace tui = mysong::tui;

namespace {

// O pedido da aba corrente, que é o que o cabeçalho manda rasterizar: a
// palavra, o par de côres do bloco solido, e as sete cellas d'ella.
nu::PedidoDaChapa da_corrente() {
  nu::PedidoDaChapa pedido;
  pedido.texto = "MY SONG";
  pedido.tinta = "#f4effe";
  pedido.fundo = "#7c3aed";
  pedido.cellulas = 7;
  return pedido;
}

// O cabeçalho pintado em PAPEL, com as caixas enchidas pelo `reflect`: é o
// unico modo de a prova ter as caixas que o pintor terá, e não caixas
// escriptas á mão que poderiam mentir sobre onde a palavra cae.
ftxui::Screen papel(tui::CaixasDoCabecalho* caixas, tui::Aba corrente) {
  tui::Retracto retracto;
  retracto.volume = 100;
  ftxui::Screen ecran = ftxui::Screen::Create(ftxui::Dimension::Fixed(167),
                                              ftxui::Dimension::Fixed(1));
  ftxui::Element linha = tui::elemento_do_cabecalho(
      retracto, corrente, "uma faixa qualquer", 167, caixas);
  ftxui::Render(ecran, linha);
  return ecran;
}

}  // namespace

TEST_CASE("a linha do pango-view sahe argumento a argumento") {
  const std::vector<std::string> linha =
      nu::argumentos_do_letreiro(da_corrente(), 13, "/tmp/chapa.png");
  CHECK(linha == std::vector<std::string>{
                     "pango-view", "--font=Xirod 22", "--foreground=#f4effe",
                     "--background=#7c3aed", "--margin=13 0", "-q", "-o",
                     "/tmp/chapa.png", "-t", "MY SONG"});
}

TEST_CASE("a margem casa a proporção da chapa com a da caixa") {
  // A palavra MEDIDA n'esta machina: duzentos pixeis por trinta e sete, em
  // sete cellas de nove por vinte. A caixa é 63 por 20, d'onde a chapa ha de
  // ficar com 63 de altura, e a folga é treze de cada lado.
  CHECK(nu::margem_da_chapa({200, 37}, 7, nu::CELLULA_DA_CASA) == 13);
  // E a folga NUNCA transborda: arredondada para baixo, a chapa fica sempre ao
  // menos tão larga quanto a caixa pede, d'onde é a LARGURA que manda na
  // reducção e a altura cabe na linha.
  const std::size_t alta =
      37 + 2 * nu::margem_da_chapa({200, 37}, 7, nu::CELLULA_DA_CASA);
  CHECK(200 * nu::CELLULA_DA_CASA.altura >= alta * 7 * nu::CELLULA_DA_CASA.largura);
  // Chapa JÁ mais alta que a caixa não pede folga; nem a medida que se não
  // leu, nem a caixa de largura nenhuma.
  CHECK(nu::margem_da_chapa({200, 200}, 7, nu::CELLULA_DA_CASA) == 0);
  CHECK(nu::margem_da_chapa({0, 0}, 7, nu::CELLULA_DA_CASA) == 0);
  CHECK(nu::margem_da_chapa({200, 37}, 0, nu::CELLULA_DA_CASA) == 0);
}

TEST_CASE("a chave do cache muda com todo campo do pedido") {
  const std::string base = nu::chave_do_letreiro(da_corrente());
  CHECK(base.size() == 16);
  CHECK(nu::chave_do_letreiro(da_corrente()) == base);
  for (int qual = 0; qual < 4; ++qual) {
    nu::PedidoDaChapa outro = da_corrente();
    if (qual == 0) outro.cellulas = 9;
    if (qual == 1) outro.tinta = "#cbb6ff";
    if (qual == 2) outro.corpo = 18;
    if (qual == 3) outro.familia = "JetBrainsMono Nerd Font";
    CHECK(nu::chave_do_letreiro(outro) != base);
  }
  // A COSTURA: sem o octeto nullo entre os campos, estes dous dariam a mesma
  // somma, e o cabeçalho mostraria a chapa de um no logar do outro.
  nu::PedidoDaChapa um = da_corrente(), dous = da_corrente();
  um.texto = "AB";
  um.tinta = "C";
  dous.texto = "A";
  dous.tinta = "BC";
  CHECK(nu::chave_do_letreiro(um) != nu::chave_do_letreiro(dous));
}

TEST_CASE("a chapa mora em letreiro, ao lado das capas") {
  // O ambiente restaura-se no fim: a bateria corre n'um processo só, e prova
  // que deixasse a variavel mudada faria a visinha ler outro cache.
  const char* const antes = std::getenv("XDG_CACHE_HOME");
  const std::string guardado = antes == nullptr ? std::string() : antes;
  ::setenv("XDG_CACHE_HOME", "/tmp/cova-do-letreiro", 1);
  CHECK(nu::caminho_da_chapa_em_cache(da_corrente()) ==
        std::filesystem::path("/tmp/cova-do-letreiro/mysong/letreiro/" +
                              nu::chave_do_letreiro(da_corrente()) + ".png"));
  if (guardado.empty())
    ::unsetenv("XDG_CACHE_HOME");
  else
    ::setenv("XDG_CACHE_HOME", guardado.c_str(), 1);
}

TEST_CASE("ha letreiro sómente com lousa, com pango-view e com a Xirod") {
  using nu::ModoDaLousa;
  const nu::Parecer de_pe =
      nu::parecer_do_letreiro(ModoDaLousa::Auto, true, true);
  CHECK(de_pe.de_pe);
  CHECK(de_pe.razao == "Xirod, pango-view");
  CHECK(nu::texto_do_letreiro(de_pe) == "\n  letreiro: Xirod, pango-view\n");
  // Sem o PROGRAMA, e a razão diz o pacote e não a fonte: quem o não tem ha de
  // ler o remedio que lhe serve.
  const nu::Parecer sem_pango =
      nu::parecer_do_letreiro(ModoDaLousa::Auto, false, true);
  CHECK_FALSE(sem_pango.de_pe);
  CHECK(sem_pango.razao.find("pango-view") != std::string::npos);
  // Sem a FONTE, com o programa presente.
  const nu::Parecer sem_fonte =
      nu::parecer_do_letreiro(ModoDaLousa::Auto, true, false);
  CHECK_FALSE(sem_fonte.de_pe);
  CHECK(sem_fonte.razao.find("Xirod") != std::string::npos);
  // A alavanca da lousa desliga o letreiro, e o `sim` d'ella não o accende
  // sem os dous: fonte que não está não se finge por ajuste.
  CHECK_FALSE(nu::parecer_do_letreiro(ModoDaLousa::Nao, true, true).de_pe);
  CHECK_FALSE(nu::parecer_do_letreiro(ModoDaLousa::Sim, true, false).de_pe);
}

TEST_CASE("a caixa da palavra cae exactamente sobre a palavra") {
  tui::CaixasDoCabecalho caixas;
  const ftxui::Screen ecran = papel(&caixas, tui::Aba::MySong);
  for (const tui::Aba aba :
       {tui::Aba::MySong, tui::Aba::Playlists, tui::Aba::Download}) {
    const ftxui::Box segmento =
        aba == tui::Aba::MySong     ? caixas.aba_mysong
        : aba == tui::Aba::Playlists ? caixas.aba_playlists
                                     : caixas.aba_download;
    const ftxui::Box palavra = tui::caixa_da_palavra(segmento);
    std::string dita;
    for (int x = palavra.x_min; x <= palavra.x_max; ++x)
      dita += ecran.PixelAt(x, 0).character;
    CHECK(dita == tui::palavra_da_aba(aba));
  }
  // A primeira aba abre a fita, d'onde a palavra d'ella começa na cella tres:
  // o espaço, o glifo e o espaço da guarnição.
  CHECK(tui::caixa_da_palavra(caixas.aba_mysong).x_min == 3);
  // Caixa por pintar não dá palavra alguma, e é o que guarda o primeiro quadro
  // de mandar chapa para um canto que ainda não existe.
  const ftxui::Box nenhuma = tui::caixa_da_palavra(tui::caixa_por_pintar());
  CHECK(nenhuma.x_max < nenhuma.x_min);
}

TEST_CASE("as tres ordens de chapa sahem do quadro do cabeçalho") {
  tui::CaixasDoCabecalho caixas;
  papel(&caixas, tui::Aba::Playlists);
  const std::vector<tui::ChapaDaAba> ordens =
      tui::ordens_das_chapas(caixas, tui::Aba::Playlists, true, true);
  REQUIRE(ordens.size() == 3);
  CHECK(ordens[0].aba == tui::Aba::MySong);
  CHECK(ordens[0].estado == tui::EstadoDaAba::Apagada);
  CHECK(ordens[1].estado == tui::EstadoDaAba::Corrente);
  CHECK(ordens[2].estado == tui::EstadoDaAba::Apagada);
  CHECK(ordens[0].collunha == 3);
  for (const tui::ChapaDaAba& ordem : ordens) {
    CHECK(ordem.poe);
    CHECK(ordem.linha == 0);
    CHECK(ordem.largura == tui::palavra_da_aba(ordem.aba).size());
  }
  // O FOCO FÓRA do terminal tira as tres, que é a disciplina da capa; e sem
  // letreiro tambem, e por quadro ainda por pintar tambem.
  const tui::CaixasDoCabecalho por_pintar;
  for (const auto& caso : {tui::ordens_das_chapas(caixas, tui::Aba::MySong, true, false),
                           tui::ordens_das_chapas(caixas, tui::Aba::MySong, false, true),
                           tui::ordens_das_chapas(por_pintar, tui::Aba::MySong, true, true)})
    for (const tui::ChapaDaAba& ordem : caso) CHECK_FALSE(ordem.poe);
  // O foco da issue irmã ganha da corrente, e a aba focada accende n'elle.
  const tui::Aba focada = tui::Aba::Playlists;
  CHECK(tui::ordens_das_chapas(caixas, tui::Aba::Playlists, true, true,
                               &focada)[1]
            .estado == tui::EstadoDaAba::ComFoco);
}
