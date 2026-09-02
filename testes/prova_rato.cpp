// ══════════════════════════════════════════════════════════════════════════
//   PROVA DO RATO — testes/prova_rato.cpp
// ══════════════════════════════════════════════════════════════════════════
// As duas taboadas do rato (issue #95), sem terminal e sem tela: as caixas
// armam-se á mão, com as coordenadas escriptas, e o que se afere é o alvo que
// o ponto acha e o gesto que o alvo pede. É ella que apanha a collunha trocada.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <ftxui/component/mouse.hpp>
#include <ftxui/screen/box.hpp>

#include "tui/rato.hpp"

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

// A tela de mentira: sete degraus de nove collunhas á esquerda, cinco linhas de
// tabella á direita d'elles, a capa n'um quadro, e o transporte no pé. Os
// numeros são arbitrarios: o que se prova é a geometria, e não a composição.
tui::CaixasDaTela tela_de_mentira() {
  tui::CaixasDaTela caixas;
  for (int i = 0; i < 7; ++i) caixas.degraus.push_back({1, 9, 3 + i, 3 + i});
  for (int i = 0; i < 5; ++i) caixas.linhas.push_back({11, 60, 3 + i, 3 + i});
  caixas.primeira_linha = 20;
  caixas.capa = {62, 80, 3, 12};
  caixas.transporte.pausa = {1, 3, 30, 30};
  caixas.transporte.saltos = {4, 9, 30, 30};
  caixas.transporte.barra_cheia = {11, 20, 30, 30};
  caixas.transporte.barra_vazia = {21, 30, 30, 30};
  return caixas;
}

}  // namespace

TEST_CASE("cada peça da tela responde pelo seu ponto") {
  const tui::CaixasDaTela caixas = tela_de_mentira();
  const tui::Alvo degrau = tui::alvo_do_ponto(caixas, 5, 6);
  CHECK(degrau.peca == tui::Peca::Degrau);
  CHECK(degrau.indice == 3);
  // A linha sahe em indice ABSOLUTO: a segunda á vista, com vinte de rolagem.
  const tui::Alvo linha = tui::alvo_do_ponto(caixas, 30, 4);
  CHECK(linha.peca == tui::Peca::Linha);
  CHECK(linha.indice == 21);
  CHECK(tui::alvo_do_ponto(caixas, 70, 8).peca == tui::Peca::Capa);
  CHECK(tui::alvo_do_ponto(caixas, 2, 30).peca == tui::Peca::Pausa);
  CHECK(tui::alvo_do_ponto(caixas, 5, 30).peca == tui::Peca::Anterior);
  CHECK(tui::alvo_do_ponto(caixas, 8, 30).peca == tui::Peca::Proxima);
  // Fóra de tudo: a altura que sobra abaixo da lista, a orla, e o rodapé.
  CHECK(tui::alvo_do_ponto(caixas, 30, 9).peca == tui::Peca::Nada);
  CHECK(tui::alvo_do_ponto(caixas, 0, 0).peca == tui::Peca::Nada);
  CHECK(tui::alvo_do_ponto(caixas, 100, 40).peca == tui::Peca::Nada);
}

TEST_CASE("a fracção da barra vae de zero na primeira collunha a um na ultima") {
  const tui::CaixasDaTela caixas = tela_de_mentira();
  const tui::Alvo principio = tui::alvo_do_ponto(caixas, 11, 30);
  CHECK(principio.peca == tui::Peca::Progresso);
  CHECK(principio.fracao == doctest::Approx(0.0));
  CHECK(tui::alvo_do_ponto(caixas, 30, 30).fracao == doctest::Approx(1.0));
  CHECK(tui::alvo_do_ponto(caixas, 21, 30).fracao ==
        doctest::Approx(10.0 / 19.0));
}

TEST_CASE("a barra fica inteira com uma das metades por pintar") {
  tui::CaixasDaTela caixas = tela_de_mentira();
  // Principio da faixa: o cheio tem largura zero, e o FTXUI dá-lhe caixa vazia.
  caixas.transporte.barra_cheia = tui::caixa_por_pintar();
  CHECK(tui::alvo_do_ponto(caixas, 21, 30).peca == tui::Peca::Progresso);
  CHECK(tui::alvo_do_ponto(caixas, 21, 30).fracao == doctest::Approx(0.0));
  // Fim da faixa: agora é o vazio que se não pintou.
  caixas = tela_de_mentira();
  caixas.transporte.barra_vazia = tui::caixa_por_pintar();
  CHECK(tui::alvo_do_ponto(caixas, 20, 30).peca == tui::Peca::Progresso);
  CHECK(tui::alvo_do_ponto(caixas, 20, 30).fracao == doctest::Approx(1.0));
  // As duas por pintar: barra alguma ha, e o ponto não acha cousa alguma.
  caixas.transporte.barra_cheia = tui::caixa_por_pintar();
  CHECK(tui::alvo_do_ponto(caixas, 20, 30).peca == tui::Peca::Nada);
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
  // O direito é da issue #96, e o do meio não é de issue alguma.
  for (const Mouse::Button qual : {Mouse::Right, Mouse::Middle, Mouse::None})
    CHECK(tui::gesto_do_alvo(linha, qual, Mouse::Pressed, estado).gesto ==
          tui::Gesto::Nada);
  CHECK(tui::gesto_do_alvo(linha, Mouse::Left, Mouse::Pressed, estado).gesto ==
        tui::Gesto::Toca);
}

namespace {

// clicou — o gesto de um clique esquerdo n'um ponto, que é o que quasi todo
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
  // A linha, o degrau e o botão: TODO alvo dá a mesma cousa, que é o campo a
  // fechar-se. A tela não ha de mudar debaixo de quem está a digitar.
  for (const int y : {4, 6, 30})
    CHECK(clicou(caixas, y == 6 ? 5 : (y == 30 ? 2 : 30), y, digita).gesto ==
          tui::Gesto::FechaCampo);
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

// rodou — o gesto de um dente da roda n'um ponto.
tui::GestoDoRato rodou(const tui::CaixasDaTela& caixas, int x, int y, bool sobe,
                       const tui::EstadoDoRato& estado) {
  return tui::gesto_do_alvo(tui::alvo_do_ponto(caixas, x, y),
                            sobe ? Mouse::WheelUp : Mouse::WheelDown,
                            Mouse::Pressed, estado);
}

}  // namespace

TEST_CASE("a roda anda tres linhas na tabella, e um degrau sobre a barra") {
  const tui::CaixasDaTela caixas = tela_de_mentira();
  const tui::EstadoDoRato estado{false, 21, 40, 200.0};
  const tui::GestoDoRato desce = rodou(caixas, 30, 4, false, estado);
  CHECK(desce.gesto == tui::Gesto::RodaDesce);
  CHECK(desce.indice == tui::LINHAS_POR_DENTE);
  CHECK(rodou(caixas, 30, 4, true, estado).gesto == tui::Gesto::RodaSobe);
  CHECK(rodou(caixas, 5, 6, false, estado).gesto == tui::Gesto::DegrauDesce);
  CHECK(rodou(caixas, 5, 6, true, estado).gesto == tui::Gesto::DegrauSobe);
  // Fóra da lista e da barra a roda não governa cousa alguma: nem volume, nem
  // busca. Prometter-lhe officio seria prometter o que a issue não pediu.
  CHECK(rodou(caixas, 2, 30, true, estado).gesto == tui::Gesto::Nada);
  CHECK(rodou(caixas, 70, 8, true, estado).gesto == tui::Gesto::Nada);
}

TEST_CASE("o transporte, a capa e a busca dão o gesto que dizem") {
  const tui::CaixasDaTela caixas = tela_de_mentira();
  const tui::EstadoDoRato estado{false, 21, 40, 200.0};
  CHECK(clicou(caixas, 5, 30, estado).gesto == tui::Gesto::Anterior);
  CHECK(clicou(caixas, 8, 30, estado).gesto == tui::Gesto::Proxima);
  CHECK(clicou(caixas, 2, 30, estado).gesto == tui::Gesto::PausaOuRetoma);
  // A capa é o mesmo gesto do ⏯: quem clica na arte quer calar o que toca.
  CHECK(clicou(caixas, 70, 8, estado).gesto == tui::Gesto::PausaOuRetoma);
  const tui::GestoDoRato busca = clicou(caixas, 21, 30, estado);
  CHECK(busca.gesto == tui::Gesto::Busca);
  CHECK(busca.alvo == doctest::Approx(200.0 * 10.0 / 19.0));
  // Sem duração não se busca. Zero seria affirmar o principio, e o que ha é a
  // Casa ainda não saber quanto a faixa dura.
  const tui::EstadoDoRato sem{false, 21, 40, 0.0};
  CHECK(clicou(caixas, 21, 30, sem).gesto == tui::Gesto::Nada);
  CHECK(clicou(caixas, 5, 6, estado).gesto == tui::Gesto::EntraNoDegrau);
  CHECK(clicou(caixas, 5, 6, estado).indice == 3);
}
