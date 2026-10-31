#include <doctest/doctest.h>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/screen.hpp>
#include <fstream>
#include <condition_variable>
#include "tui/redimensionamento.hpp"
#include "nucleo/aquisicao.hpp"
#include <set>
#include "nucleo/sessao_do_terminal.hpp"
#include "nucleo/letra.hpp"
#include "nucleo/lousa.hpp"
#include "nucleo/fila.hpp"
#include "tui/mp3.hpp"
#include "tui/vigilia.hpp"
namespace nu=mysong::nucleo;
namespace tui=mysong::tui;
namespace {
struct Pasta {
  std::filesystem::path raiz;
  Pasta() { char caminho[]="/tmp/mysong-acertos-XXXXXX";raiz=::mkdtemp(caminho); }
  ~Pasta() { std::error_code erro;std::filesystem::remove_all(raiz,erro); }
};
}
TEST_CASE("tmux distingue painel vizinho de janela escondida e acompanha o tamanho") {
  const auto primeiro=nu::interpreta_cena_do_tmux("1 1 @0:0:0:100:30:0\n");
  CHECK(primeiro.visivel);CHECK(primeiro.ativo);
  const auto vizinho=nu::interpreta_cena_do_tmux("1 0 @0:0:0:100:30:0",primeiro);
  CHECK(vizinho.visivel);CHECK_FALSE(vizinho.ativo);CHECK(vizinho.geracao>primeiro.geracao);
  const auto outra=nu::interpreta_cena_do_tmux("0 0 @0:0:0:100:30:0",vizinho);
  CHECK_FALSE(outra.visivel);CHECK_FALSE(outra.ativo);
  const auto retorno=nu::interpreta_cena_do_tmux("1 1 @0:30:0:70:30:0",outra);
  CHECK(retorno.visivel);CHECK(retorno.geracao>outra.geracao);
  CHECK(nu::interpreta_cena_do_tmux("erro",retorno).geracao==retorno.geracao);
  CHECK(nu::interpreta_cena_do_tmux("1 1 @0:30:0:70:30:0",retorno).geracao==retorno.geracao);
}
TEST_CASE("redimensionar repõe imagens iguais e ainda permite removê-las") {
  std::string ordens;
  nu::EscoadouroDaLousa cano([&](std::string_view texto){ordens+=texto;return static_cast<ssize_t>(texto.size());});
  REQUIRE(cano.deseja("capa","add\n",true));REQUIRE(cano.drena());
  const auto inicial=ordens;
  cano.deseja("capa","add\n",true);cano.drena();CHECK(ordens==inicial);
  cano.invalida();cano.drena();CHECK(ordens=="add\nadd\n");
  cano.invalida();cano.deseja("capa","remove\n",false);cano.drena();CHECK(ordens=="add\nadd\nremove\n");
}
TEST_CASE("modo MP3 conserva três controles clicáveis e a contagem real") {
  tui::Retracto r;r.acervo=157;r.titulo="/musicas/Teste.mp3";
  for (int largura:{3,8,14,24,40,75}) for (int altura:{1,3,6,9}) {
    tui::CaixasDoCabecalho caixas;
    auto tela=ftxui::Screen::Create(ftxui::Dimension::Fixed(largura),ftxui::Dimension::Fixed(altura));
    ftxui::Render(tela,tui::elemento_do_mp3(r,largura,altura,&caixas));
    CHECK_FALSE(caixas.botao_tocar.IsEmpty());CHECK_FALSE(caixas.botao_anterior.IsEmpty());CHECK_FALSE(caixas.botao_seguinte.IsEmpty());
    for (const auto& caixa:{caixas.botao_tocar,caixas.botao_anterior,caixas.botao_seguinte}) {
      CHECK(caixa.x_min>=0);CHECK(caixa.x_max<largura);CHECK(caixa.y_min>=0);CHECK(caixa.y_max<altura);
    }
    if (largura>=24 && altura>=3) CHECK(tela.ToString().find("157")!=std::string::npos);
  }
}
TEST_CASE("clique manual inicia um percurso embaralhado completo até depois da última posição") {
  nu::Fila fila;
  for (int i=0;i<20;++i) fila.junta(std::to_string(i));
  fila.embaralhar(true);
  const auto ordem=fila.ordem();
  fila.ir_para(ordem.back());fila.reinicia_percurso();
  std::set<std::size_t> tocadas{fila.indice()};
  for (int i=1;i<20;++i) { REQUIRE(fila.proxima());CHECK(tocadas.insert(fila.indice()).second); }
  CHECK_FALSE(fila.proxima());CHECK(tocadas.size()==20);
}
TEST_CASE("LRC aplica offset global mesmo após os versos e ignora texto parecido no verso") {
  auto linhas=nu::analysa_lrc("[00:02.00] primeiro\n[00:05.00] segundo\n[offset:-1500]\n");
  REQUIRE(linhas.size()==2);CHECK(linhas[0].tempo==3.5);CHECK(linhas[1].tempo==6.5);
  linhas=nu::analysa_lrc("[offset:500]\n[00:02.00] verso [offset:999]\n");
  REQUIRE(linhas.size()==1);CHECK(linhas[0].tempo==1.5);
}
TEST_CASE("letra de clipe mais longo guarda tempos para ajuste mas não os aplica automaticamente") {
  Pasta pasta;const auto audio=pasta.raiz/"musica.mp3";
  const auto letra=nu::escolhe_letra(R"([{"trackName":"Teste","artistName":"Autor","duration":100,"syncedLyrics":"[00:03] verso","plainLyrics":"verso"}])","Autor","Teste",130);
  REQUIRE(letra.sincronizada.empty());REQUIRE_FALSE(letra.sincronizada_alternativa.empty());
  REQUIRE(nu::grava_lrc(audio,letra));CHECK(nu::le_lrc_do_disco(audio).empty());
  CHECK(nu::le_letra_plana_do_disco(audio)==std::vector<std::string>{"verso"});
  REQUIRE(nu::ajusta_atraso_da_letra(audio,30));
  auto linhas=nu::le_lrc_do_disco(audio);REQUIRE(linhas.size()==1);CHECK(linhas[0].tempo==33);
  CHECK(nu::atraso_da_letra(audio)==30);
  REQUIRE(nu::ajusta_atraso_da_letra(audio,-0.5));CHECK(nu::le_lrc_do_disco(audio)[0].tempo==32.5);
  REQUIRE(nu::ajusta_atraso_da_letra(audio,0,true));CHECK(nu::le_lrc_do_disco(audio)[0].tempo==3);
}
TEST_CASE("letra nova sem tempos não deixa tocar uma sincronização antiga incompatível") {
  Pasta pasta;const auto audio=pasta.raiz/"musica.mp3";
  std::ofstream(nu::caminho_do_lrc(audio))<<"[00:02] antiga\n";
  nu::Letra letra;letra.plana="nova";
  REQUIRE(nu::grava_lrc(audio,letra));CHECK(nu::le_lrc_do_disco(audio).empty());
  CHECK(nu::le_letra_plana_do_disco(audio)==std::vector<std::string>{"nova"});
}

TEST_CASE("batida de animação não é redimensionamento nem agenda outra reposição de capas") {
  CHECK_FALSE(tui::redimensionou(ftxui::Event::Custom));
  CHECK(tui::redimensionou(ftxui::Event::Special("mysong:resize")));
  CHECK_FALSE(tui::redimensionou(ftxui::Event::Character('a')));
}

TEST_CASE("recuperação da imagem descarta bytes partidos do processo antigo e repõe a intenção atual") {
  bool quebrado=true;
  std::string ordens;
  nu::EscoadouroDaLousa cano([&](std::string_view texto) -> ssize_t {
    if (quebrado) { errno=EPIPE; return -1; }
    ordens+=texto;return static_cast<ssize_t>(texto.size());
  });
  cano.deseja("capa","add\n",true);
  CHECK(cano.falhou());
  quebrado=false;cano.reinicia();REQUIRE(cano.drena());
  CHECK(ordens=="add\n");CHECK_FALSE(cano.falhou());
}


TEST_CASE("redimensionamento retira uma vez e repõe somente o último tamanho") {
  tui::Redimensionamento ciclo;
  using Clock=tui::Redimensionamento::Relogio;
  const auto inicio=Clock::time_point{};
  auto quadro=ciclo.observa(140,40,inicio);
  CHECK_FALSE(quadro.suspensas);CHECK_FALSE(quadro.ocultar);CHECK_FALSE(quadro.repor);
  int retiradas=0;
  for (int i=1;i<=40;++i) {
    const auto agora=inicio+std::chrono::milliseconds(i*25);
    quadro=ciclo.observa(140+i,40,agora);
    retiradas+=quadro.ocultar;
    CHECK(quadro.suspensas);CHECK_FALSE(quadro.repor);CHECK_FALSE(ciclo.pulsa(agora));
  }
  CHECK(retiradas==1);
  CHECK(ciclo.movendo());
  CHECK_FALSE(ciclo.pulsa(inicio+std::chrono::milliseconds(1299)));
  REQUIRE(ciclo.pulsa(inicio+std::chrono::milliseconds(1300)));
  CHECK_FALSE(ciclo.movendo());
  quadro=ciclo.observa(180,40,inicio+std::chrono::milliseconds(1300));
  CHECK_FALSE(quadro.suspensas);CHECK(quadro.repor);CHECK_FALSE(quadro.ocultar);
  CHECK_FALSE(ciclo.observa(180,40,inicio+std::chrono::milliseconds(1400)).repor);
  CHECK_FALSE(ciclo.pulsa(inicio+std::chrono::seconds(5)));
}
TEST_CASE("quadros idênticos não prolongam o redimensionamento") {
  tui::Redimensionamento ciclo;
  const auto inicio=tui::Redimensionamento::Relogio::time_point{};
  ciclo.observa(140,40,inicio);ciclo.observa(130,40,inicio);
  for (int i=1;i<15;++i) ciclo.observa(130,40,inicio+std::chrono::milliseconds(i*10));
  CHECK(ciclo.pulsa(inicio+std::chrono::milliseconds(300)));
  CHECK(ciclo.observa(130,40,inicio+std::chrono::milliseconds(300)).repor);
}
TEST_CASE("nova medida antes da reposição cancela o quadro intermediário sem piscar") {
  tui::Redimensionamento ciclo;
  const auto inicio=tui::Redimensionamento::Relogio::time_point{};
  ciclo.observa(140,40,inicio);
  CHECK(ciclo.observa(130,40,inicio).ocultar);
  REQUIRE(ciclo.pulsa(inicio+std::chrono::milliseconds(300)));
  const auto quadro=ciclo.observa(120,40,inicio+std::chrono::milliseconds(301));
  CHECK(quadro.suspensas);CHECK_FALSE(quadro.ocultar);CHECK_FALSE(quadro.repor);
  REQUIRE(ciclo.pulsa(inicio+std::chrono::milliseconds(601)));
  CHECK(ciclo.observa(120,40,inicio+std::chrono::milliseconds(601)).repor);
}
TEST_CASE("consulta lenta ao tmux não bloqueia leitura do quadro nem a batida do tocador") {
  std::mutex tranca;std::condition_variable sino;
  bool entrou=false,libera=false,chegou=false;
  auto gasto=std::chrono::steady_clock::duration::zero();
  {
    nu::SessaoDoTerminal sessao("%0",[&](const auto&) {
      std::unique_lock<std::mutex> chave(tranca);entrou=true;sino.notify_all();
      sino.wait(chave,[&]{return libera;});
      return std::string("1 1 @0:0:0:140:40:0");
    });
    sessao.inicia();
    { std::unique_lock<std::mutex> chave(tranca);chegou=sino.wait_for(chave,std::chrono::seconds(2),[&]{return entrou;}); }
    const auto antes=std::chrono::steady_clock::now();
    for (int i=0;i<100;++i) sessao.retrato();
    gasto=std::chrono::steady_clock::now()-antes;
    { std::lock_guard<std::mutex> chave(tranca);libera=true; } sino.notify_all();
  }
  CHECK(chegou);CHECK(gasto<std::chrono::milliseconds(50));
}

TEST_CASE("medidas recebidas entre quadros não fazem as imagens voltar entre lotes do tmux") {
  tui::Redimensionamento ciclo;
  const auto inicio=tui::Redimensionamento::Relogio::time_point{};
  ciclo.observa(140,40,inicio);
  int retiradas=0;
  for (int i=1;i<=40;++i) {
    const auto agora=inicio+std::chrono::milliseconds(i*250);
    REQUIRE(ciclo.atualiza(140+i,40,agora));
    CHECK_FALSE(ciclo.pulsa(agora+std::chrono::milliseconds(200)));
    if (i%3==0) {
      const auto quadro=ciclo.observa(140+i,40,agora+std::chrono::milliseconds(210));
      retiradas+=quadro.ocultar;CHECK(quadro.suspensas);CHECK_FALSE(quadro.repor);
    }
  }
  CHECK(retiradas==1);
  REQUIRE(ciclo.pulsa(inicio+std::chrono::milliseconds(10300)));
  const auto quadro=ciclo.observa(180,40,inicio+std::chrono::milliseconds(10300));
  CHECK(quadro.repor);CHECK_FALSE(quadro.ocultar);
  CHECK_FALSE(ciclo.pulsa(inicio+std::chrono::milliseconds(12000)));
}
TEST_CASE("fechar o observador do tmux cancela consulta externa silenciosa") {
  std::atomic<bool> entrou{false};
  bool iniciou=false;
  auto fim=std::chrono::steady_clock::now();
  {
    nu::SessaoDoTerminal sessao("%0",[&](const auto&) {
      nu::corre({"sh","-c","printf 'pronto\\n'; sleep 20"},nullptr,[&](std::string_view) { entrou.store(true); });
      return std::string{};
    });
    sessao.inicia();
    for (int i=0;i<200 && !entrou.load();++i) std::this_thread::sleep_for(std::chrono::milliseconds(5));
    iniciou=entrou.load();fim=std::chrono::steady_clock::now();
  }
  CHECK(iniciou);CHECK(std::chrono::steady_clock::now()-fim<std::chrono::seconds(1));
}

TEST_CASE("reativação da imagem espera todas as ordens pendentes e não fica no cache de imagens") {
  std::string enviado;
  bool bloqueado=true;
  nu::EscoadouroDaLousa cano([&](std::string_view texto) -> ssize_t {
    if (bloqueado) { errno=EAGAIN;return -1; }
    const auto quantos=std::min<std::size_t>(3,texto.size());
    enviado.append(texto.data(),quantos);return static_cast<ssize_t>(quantos);
  });
  CHECK_FALSE(cano.deseja("capa","CAPA\n",true));
  CHECK_FALSE(cano.apos_imagens("REEXIBIR\n"));
  CHECK_FALSE(cano.deseja("miniatura","MINIATURA\n",true));
  CHECK(cano.pendente());
  bloqueado=false;
  REQUIRE(cano.drena());
  CHECK(enviado=="CAPA\nMINIATURA\nREEXIBIR\n");
  CHECK_FALSE(cano.pendente());
  cano.invalida();REQUIRE(cano.drena());
  CHECK(enviado=="CAPA\nMINIATURA\nREEXIBIR\nCAPA\nMINIATURA\n");
  REQUIRE(cano.apos_imagens("REEXIBIR\n"));
  CHECK(enviado=="CAPA\nMINIATURA\nREEXIBIR\nCAPA\nMINIATURA\nREEXIBIR\n");
}
