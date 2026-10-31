#include <doctest/doctest.h>
#include <fstream>
#include <limits>
#include <condition_variable>
#include <cstdlib>
#include "nucleo/separacao.hpp"
#include "nucleo/aquisicao.hpp"
#include "tui/separacao.hpp"
namespace nu = mysong::nucleo;
namespace {
struct PastaSeparacao {
  std::filesystem::path raiz;
  PastaSeparacao() { char pasta[]="/tmp/mysong-partes-XXXXXX"; raiz=::mkdtemp(pasta); }
  ~PastaSeparacao() { std::error_code erro; std::filesystem::remove_all(raiz,erro); }
  std::filesystem::path audio() {
    const auto caminho=raiz/"original.flac";
    std::filesystem::copy_file(std::filesystem::path(__FILE__).parent_path()/"dados/separacao.flac", caminho,
        std::filesystem::copy_options::overwrite_existing);
    return caminho;
  }
};
void gera(const std::vector<std::string>& args, bool completo=true) {
  const auto destino=std::filesystem::path(args[args.size()-3])/"htdemucs";
  std::filesystem::create_directories(destino);
  for (int i=0;i<(completo?4:3);++i)
    std::filesystem::copy_file(args.back(), destino/nu::ARQUIVOS_DAS_PARTES[i]);
}
class MotorDasPartes : public nu::Motor {
 public:
  nu::Estado e=nu::Estado::Parado;
  int cargas=0,misturas=0,originais=0;
  bool aceita=true;
  bool tocar(const std::string&) override { ++cargas; e=nu::Estado::Tocando; return true; }
  bool pausar() override { e=nu::Estado::Pausado; return true; }
  bool retomar() override { e=nu::Estado::Tocando; return true; }
  bool buscar(double) override { return true; }
  bool volume(int) override { return true; }
  double posicao() const override { return 25; }
  double duracao() const override { return 200; }
  nu::Estado estado() const override { return e; }
  void bombear() override {}
  bool mistura(const nu::ArquivosDasPartes&,const nu::VolumesDasPartes&) override { ++misturas; return aceita; }
  bool original() override { ++originais; return true; }
};
}
TEST_CASE("separação interpreta progresso e limita os volumes da mistura") {
  CHECK(nu::progresso_da_separacao(" 42%|abc")==42);
  CHECK(nu::progresso_da_separacao("100%|fim")==100);
  CHECK_FALSE(nu::progresso_da_separacao("101%"));
  CHECK_FALSE(nu::progresso_da_separacao("%"));
  CHECK_FALSE(nu::progresso_da_separacao("sem percentual"));
  const auto grafo=nu::grafo_da_mistura({2,3,4,5},{-10,200,50,0});
  CHECK(grafo.find("[aid2]volume=0.000000")!=std::string::npos);
  CHECK(grafo.find("[aid3]volume=1.000000")!=std::string::npos);
  CHECK(grafo.find("[aid4]volume=0.500000")!=std::string::npos);
  CHECK(grafo.find("normalize=0")!=std::string::npos);
  CHECK(nu::grafo_da_mistura({0,2,3,4},{100,100,100,100}).empty());
}
TEST_CASE("separação valida quatro partes e reaproveita cache sem executar novamente") {
  PastaSeparacao pasta; const auto audio=pasta.audio(); int execucoes=0;
  nu::SeparadorDeAudio separador(pasta.raiz/"cache","teste",[&](const auto& args,const auto& observa){
    ++execucoes; observa("Separating track x"); observa("50%|metade"); gera(args); return 0;
  });
  REQUIRE(separador.inicia(audio)); separador.espera();
  const auto pronto=separador.retrato();
  REQUIRE(pronto.estado==nu::EstadoDaSeparacao::Pronto);
  CHECK_FALSE(pronto.cache);
  for (const auto& arquivo:pronto.arquivos) CHECK(std::filesystem::file_size(arquivo)>0);
  CHECK(std::filesystem::exists(audio));
  REQUIRE(separador.inicia(audio)); separador.espera();
  CHECK(separador.retrato().cache); CHECK(execucoes==1);
  nu::SeparadorDeAudio leitor(pasta.raiz/"cache","");
  REQUIRE(leitor.inicia(audio)); leitor.espera();
  CHECK(leitor.retrato().estado==nu::EstadoDaSeparacao::Pronto);
  CHECK(leitor.retrato().cache);
  std::filesystem::resize_file(pronto.arquivos[0],1);
  REQUIRE(separador.inicia(audio)); separador.espera();
  CHECK(separador.retrato().estado==nu::EstadoDaSeparacao::Pronto); CHECK(execucoes==2);
}
TEST_CASE("separação descarta resultado incompleto e preserva o original") {
  PastaSeparacao pasta; const auto audio=pasta.audio(); const auto tamanho=std::filesystem::file_size(audio);
  nu::SeparadorDeAudio separador(pasta.raiz/"cache","teste",[](const auto& args,const auto&){gera(args,false);return 0;});
  REQUIRE(separador.inicia(audio)); separador.espera();
  CHECK(separador.retrato().estado==nu::EstadoDaSeparacao::Falhou);
  CHECK(std::filesystem::is_empty(pasta.raiz/"cache"));
  CHECK(std::filesystem::file_size(audio)==tamanho);
}
TEST_CASE("separação não aceita outra tarefa e cancela sem publicar partes") {
  PastaSeparacao pasta; const auto audio=pasta.audio(); std::atomic_bool entrou{false};
  nu::SeparadorDeAudio separador(pasta.raiz/"cache","teste",[&](const auto&,const auto&){
    entrou.store(true);
    while (!nu::baixa_interrompida()) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    return 1;
  });
  REQUIRE(separador.inicia(audio));
  for (int i=0;i<5000 && !entrou.load();++i) std::this_thread::sleep_for(std::chrono::milliseconds(1));
  REQUIRE(entrou.load()); CHECK_FALSE(separador.inicia(audio));
  separador.cancela(); separador.espera();
  CHECK(separador.retrato().estado==nu::EstadoDaSeparacao::Cancelado);
  CHECK(std::filesystem::is_empty(pasta.raiz/"cache"));
}
TEST_CASE("separação sem ferramenta informa falta e pode ser tentada novamente") {
  PastaSeparacao pasta;const auto audio=pasta.audio();
  nu::SeparadorDeAudio separador(pasta.raiz/"cache","");
  REQUIRE(separador.inicia(audio));separador.espera();
  CHECK(separador.retrato().estado==nu::EstadoDaSeparacao::Falhou);
  CHECK(separador.retrato().detalhe.find("instala-separador.sh")!=std::string::npos);
  REQUIRE(separador.inicia(pasta.raiz/"ausente"));separador.espera();
  CHECK(separador.retrato().estado==nu::EstadoDaSeparacao::Falhou);
}
TEST_CASE("mistura preserva fila geração posição pausa e só altera a música esperada") {
  MotorDasPartes motor;nu::Tocador tocador(motor);
  REQUIRE(tocador.tocar("a")); tocador.junta("b");tocador.pausar();
  const auto antes=tocador.retracto();
  CHECK_FALSE(tocador.aplica_mistura("b",{},{100,0,0,0})); CHECK(motor.misturas==0);
  REQUIRE(tocador.aplica_mistura("a",{},{100,0,0,0}));
  auto depois=tocador.retracto();
  CHECK(depois.audio_separado);CHECK(depois.reproducao==antes.reproducao);
  CHECK(*depois.faixa=="a");CHECK(depois.posicao==25);CHECK(depois.estado==nu::Estado::Pausado);
  CHECK(motor.cargas==1);CHECK(tocador.faixas()==std::vector<std::string>{"a","b"});
  REQUIRE(tocador.aplica_mistura("a",{},{0,0,0,0}));CHECK(tocador.retracto().mudo);
  REQUIRE(tocador.retorna_original("a"));CHECK_FALSE(tocador.retracto().mudo);CHECK_FALSE(tocador.retracto().audio_separado);
  REQUIRE(tocador.aplica_mistura("a",{},{100,0,0,0}));
  REQUIRE(tocador.proxima());CHECK_FALSE(tocador.retracto().audio_separado);
  CHECK_FALSE(tocador.aplica_mistura("a",{},{100,0,0,0}));
}
TEST_CASE("mistura recusada retorna ao áudio original") {
  MotorDasPartes motor;nu::Tocador tocador(motor);tocador.tocar("a");motor.aceita=false;
  CHECK_FALSE(tocador.aplica_mistura("a",{},{100,0,0,0}));
  CHECK_FALSE(tocador.retracto().audio_separado);CHECK(motor.originais==1);
}
TEST_CASE("painel de separação não aplica partes prontas em outra música") {
  PastaSeparacao pasta;const auto audio=pasta.audio();
  nu::SeparadorDeAudio separador(pasta.raiz/"cache","teste",[](const auto& args,const auto&){gera(args);return 0;});
  REQUIRE(separador.inicia(audio));separador.espera();
  MotorDasPartes motor;nu::Tocador tocador(motor);tocador.tocar("outro");
  mysong::tui::PainelDeSeparacao painel; painel.aberto=true;painel.alvo=audio.string();
  painel.tecla(ftxui::Event::Character('1'),separador,tocador);
  CHECK(motor.misturas==0);CHECK_FALSE(painel.aviso.empty());
  painel.tecla(ftxui::Event::Return,separador,tocador);
  CHECK(motor.misturas==1);CHECK(*tocador.retracto().faixa==audio.string());
  painel.tecla(ftxui::Event::Character('2'),separador,tocador);
  CHECK(painel.volumes==nu::VolumesDasPartes{0,100,100,100});
  painel.tecla(ftxui::Event::Character('0'),separador,tocador);CHECK_FALSE(tocador.retracto().audio_separado);
  painel.tecla(ftxui::Event::Escape,separador,tocador);CHECK_FALSE(painel.aberto);
}

TEST_CASE("separação rejeita original alterado durante o processamento") {
  PastaSeparacao pasta;const auto audio=pasta.audio();
  nu::SeparadorDeAudio separador(pasta.raiz/"cache","teste",[](const auto& args,const auto&){
    gera(args);
    std::ofstream(args.back(), std::ios::app) << "alterado";
    return 0;
  });
  REQUIRE(separador.inicia(audio)); separador.espera();
  CHECK(separador.retrato().estado==nu::EstadoDaSeparacao::Falhou);
  CHECK(std::filesystem::is_empty(pasta.raiz/"cache"));
}
TEST_CASE("separação registra falha externa sem publicar arquivos parciais") {
  PastaSeparacao pasta;const auto audio=pasta.audio();
  nu::SeparadorDeAudio separador(pasta.raiz/"cache","teste",[](const auto&,const auto& observa){
    observa("erro de exemplo");return 1;
  });
  REQUIRE(separador.inicia(audio)); separador.espera();
  CHECK(separador.retrato().estado==nu::EstadoDaSeparacao::Falhou);
  CHECK(std::filesystem::exists(pasta.raiz/"cache/registros"));
  CHECK(separador.retrato().detalhe.find("erro de exemplo")==std::string::npos);
  CHECK(separador.retrato().arquivos[0].empty());
}

TEST_CASE("separação lê áudio analisado e previsão real da saída de progresso") {
  const auto medidas = nu::medidas_da_separacao(" 25%|████| 65.5/262.0 [01:20<04:00, 1.22seconds/s]");
  REQUIRE(medidas.processado); CHECK(*medidas.processado == 65.5);
  REQUIRE(medidas.total); CHECK(*medidas.total == 262.0);
  REQUIRE(medidas.restante); CHECK(*medidas.restante == 240);
  CHECK(nu::medidas_da_separacao("50%|xx| 50/100 [01:10<1:02:03, 1s/seconds]").restante == 3723);
  const auto inicial = nu::medidas_da_separacao("0%| | 0.0/262.0 [00:00<?, ?seconds/s]");
  CHECK(inicial.processado == 0.0); CHECK(inicial.total == 262.0);
  CHECK_FALSE(inicial.restante);
  CHECK_FALSE(nu::medidas_da_separacao("sem progresso").restante);
  CHECK_FALSE(nu::medidas_da_separacao("50%|xx| 50/100 [01:10<1:99, 1s/seconds]").restante);
  CHECK_FALSE(nu::medidas_da_separacao("50%|xx| 200/100 [01:10<?, ?]").processado);
}
TEST_CASE("painel mostra previsão somente após progresso e informa falta de atualização") {
  nu::RetratoDaSeparacao r;
  r.estado = nu::EstadoDaSeparacao::Separando;
  r.inicio = std::chrono::steady_clock::now(); r.ultima_atividade = r.inicio;
  auto texto = mysong::tui::situacao_da_separacao(r, r.inicio);
  CHECK(texto.find("primeiro trecho") != std::string::npos);
  r.porcentagem = 25; r.medidas.restante = 240;
  texto = mysong::tui::situacao_da_separacao(r,r.inicio + std::chrono::seconds(30));
  CHECK(texto.find("4min") != std::string::npos);
  CHECK(texto.find("Previsão") != std::string::npos);
  texto = mysong::tui::situacao_da_separacao(r,r.inicio + std::chrono::seconds(120));
  CHECK(texto.find("Sem atualização") != std::string::npos);
  CHECK(texto.find("2min") != std::string::npos);
  CHECK(texto.find("Previsão") == std::string::npos);
  r.porcentagem = 100; r.medidas.restante = 0;
  texto = mysong::tui::situacao_da_separacao(r,r.inicio);
  CHECK(texto.find("salvando") != std::string::npos);
}
TEST_CASE("suspensão do processo de separação informa falha em vez de continuar processando") {
  PastaSeparacao pasta;const auto audio = pasta.audio();
  nu::SeparadorDeAudio separador(pasta.raiz/"cache", "teste", [](const auto&,const auto& observa) {
    observa("MYSONG_PROCESSO_SUSPENSO");return -1;
  });
  REQUIRE(separador.inicia(audio)); separador.espera();
  CHECK(separador.retrato().estado == nu::EstadoDaSeparacao::Falhou);
  CHECK(separador.retrato().detalhe.find("suspendeu") != std::string::npos);
}
