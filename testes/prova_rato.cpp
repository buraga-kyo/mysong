// ══════════════════════════════════════════════════════════════════════════
//   PROVA DO RATO — testes/prova_rato.cpp
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
  alto.nome = {42, 60, 0, 0};
  alto.tempo = {61, 75, 0, 0};
  alto.volume = {76, 84, 0, 0};
  alto.embaralhar = {85, 97, 0, 0};
  alto.repetir = {98, 107, 0, 0};
  alto.trilho = {11, 30, 1, 1};
  for (int i = 0; i < 5; ++i) caixas.linhas.push_back({11, 60, 3 + i, 3 + i});
  caixas.primeira_linha = 20;
  caixas.capa = {62, 80, 3, 12};
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

namespace {

// papel — o écran de PAPEL, com os escapes dentro. Compara-se o `ToString`, e
// não as cellas nuas, de proposito: a côr entra na comparação, e caracter egual
// com tinta differente já seria a caixa a mudar a pintura.
std::string papel(ftxui::Element quadro, int largura, int altura) {
  ftxui::Screen ecran = ftxui::Screen::Create(
      ftxui::Dimension::Fixed(largura), ftxui::Dimension::Fixed(altura));
  ftxui::Render(ecran, quadro);
  return ecran.ToString();
}

}  // namespace

TEST_CASE("a caixa não muda um pixel do transporte") {
  tui::Retracto retracto;
  retracto.estado = mysong::nucleo::Estado::Tocando;
  retracto.posicao = 30.0;
  retracto.duracao = 120.0;
  for (const int largura : {30, 60, 100}) {
    const std::size_t larg = static_cast<std::size_t>(largura);
    tui::CaixasDoTransporte caixas;
    CHECK(papel(tui::elemento_do_transporte(retracto, larg), largura, 1) ==
          papel(tui::elemento_do_transporte(retracto, larg, &caixas), largura, 1));
    // E as caixas encheram-se: sem isto, a egualdade valeria tambem para quem
    // se esquecesse de as pôr, e a prova não provaria cousa alguma.
    CHECK_FALSE(caixas.pausa.IsEmpty());
    CHECK_FALSE(caixas.saltos.IsEmpty());
    CHECK_FALSE(caixas.progresso().IsEmpty());
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

// A cova e o índice: a barra e a tabella pedem um Navegador, e elle pede uma
// Bibliotheca. O acervo fica VAZIO, e a vista põe-se por `mostra_rede`.
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

TEST_CASE("a caixa não muda um pixel da barra nem da tabella") {
  Cova cova;
  const mysong::nucleo::Biblioteca livraria(cova.banco());
  mysong::nucleo::Achado um;
  um.titulo = "Toccata";
  tui::Navegador navegador(livraria);  // acervo vasio: a vista vem da rede
  navegador.mostra_rede({um, um, um});
  std::vector<ftxui::Box> degraus;
  const int larg_barra = static_cast<int>(tui::LARGURA_DA_BARRA);
  CHECK(papel(tui::elemento_da_barra(navegador, true, 2), larg_barra, 7) ==
        papel(tui::elemento_da_barra(navegador, true, 2, 0, &degraus),
              larg_barra, 7));
  // Uma caixa por DEGRAU, e sómente por degrau: o titulo e a risca não o são.
  // Sem roleiro não ha listas, donde a conta é a das minhas musicas mais as
  // quatro de navegar, que é o que a taboada da barra diz.
  CHECK(degraus.size() == tui::degraus_da_barra(0));
  CHECK_FALSE(degraus.back().IsEmpty());
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

TEST_CASE("com a janella do video de pé o clique na barra fica inerte") {
  const tui::CaixasDaTela caixas = tela_de_mentira();
  // A janella arma o estado com duração ZERO emquanto o video corre: a que o
  // retracto sabe é a do AUDIO pausado, e a janella que corre é a da faixa
  // ELEITA, que nem sempre é a mesma. Buscar por ella seria mandar o video a
  // uma posição contada n'outro arco.
  const tui::EstadoDoRato com_video{false, 21, 40, 0.0};
  CHECK(clicou(caixas, 11, 30, com_video).gesto == tui::Gesto::Nada);
  CHECK(clicou(caixas, 21, 30, com_video).gesto == tui::Gesto::Nada);
  // O resto do transporte SEGUE a governar: o `cumprir` rotea-o á janella.
  CHECK(clicou(caixas, 2, 30, com_video).gesto == tui::Gesto::PausaOuRetoma);
  CHECK(clicou(caixas, 5, 30, com_video).gesto == tui::Gesto::Anterior);
}

TEST_CASE("o botão direito é mudo tambem com o campo de digitar aberto") {
  const tui::CaixasDaTela caixas = tela_de_mentira();
  const tui::EstadoDoRato digita{true, 21, 40, 200.0};
  const tui::Alvo linha = tui::alvo_do_ponto(caixas, 30, 4);
  // Desempate conservador: a guarda do BOTÃO vem antes da do campo, donde o
  // direito não fecha nada. Elle é da issue #96, e fechar o campo seria
  // dar-lhe officio antes de ella lho definir.
  CHECK(tui::gesto_do_alvo(linha, Mouse::Right, Mouse::Pressed, digita).gesto ==
        tui::Gesto::Nada);
  CHECK(tui::gesto_do_alvo(linha, Mouse::Left, Mouse::Pressed, digita).gesto ==
        tui::Gesto::FechaCampo);
}

//   Da lavra do eminente Doutor BURAGA KYO. — buraga-kyo ✒
// ══════════════════════════════════════════════════════════════════════════
