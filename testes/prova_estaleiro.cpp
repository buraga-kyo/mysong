// ══════════════════════════════════════════════════════════════════════════
//   PROVA DO ESTALEIRO, testes/prova_estaleiro.cpp
// ══════════════════════════════════════════════════════════════════════════
// Caso algum d'esta bateria toca a rede. A OBRA entra por parametro, e no logar
// d'ella põe-se aqui uma que se deixa SEGURAR: os casos param a obra a meio,
// olham o estaleiro, e sómente então a soltam. Donde o limite se afere por
// construcção, e não por relogio: prova do genero «esperei um segundo e não
// passou de dous» é prova que a machina carregada perde.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <condition_variable>
#include <mutex>
#include <thread>
#include <string>
#include <vector>

#include "nucleo/estaleiro.hpp"

namespace nu = mysong::nucleo;

namespace {

// A CANCELLA. A obra de mentira para n'ella, e sómente passa quando o caso
// abrir. É o que substitue a espera por relogio.
class Cancella {
 public:
  void espera() {
    std::unique_lock<std::mutex> chave(tranca_);
    sino_.wait(chave, [this] { return aberta_; });
  }
  void abre() {
    {
      std::lock_guard<std::mutex> chave(tranca_);
      aberta_ = true;
    }
    sino_.notify_all();
  }
  // chegaram, espera que `quantos` obreiros tenham CHEGADO á cancella. Sem isto o
  // caso olharia o estaleiro antes de elle ter começado, e leria zero por engano.
  void chegaram(std::size_t quantos) {
    std::unique_lock<std::mutex> chave(tranca_);
    sino_.wait(chave, [this, quantos] { return chegados_ >= quantos; });
  }
  void chego() {
    {
      std::lock_guard<std::mutex> chave(tranca_);
      ++chegados_;
    }
    sino_.notify_all();
  }

 private:
  std::mutex tranca_;
  std::condition_variable sino_;
  bool aberta_ = false;
  std::size_t chegados_ = 0;
};

}  // namespace

TEST_CASE("Spotify e YouTube conservam progresso e identidade separados") {
  Cancella cancella;
  nu::Estaleiro estaleiro(2, [&cancella](const nu::Pedido& pedido,
                                         std::filesystem::path*) {
    pedido.noticia(std::nullopt, {});
    pedido.noticia(pedido.fonte == nu::Fonte::Spotify ? 37 : 82, {});
    cancella.chego();
    cancella.espera();
    return pedido.fonte == nu::Fonte::Spotify ? nu::Colheita::Colhido
                                               : nu::Colheita::UrlRecusada;
  });
  nu::Pedido spotify;
  spotify.fonte = nu::Fonte::Spotify;
  spotify.titulo = "uma faixa";
  estaleiro.encommenda(spotify);
  nu::Pedido youtube;
  youtube.fonte = nu::Fonte::YouTube;
  estaleiro.encommenda(youtube);
  cancella.chegaram(2);
  const auto meio = estaleiro.andamento().registros;
  REQUIRE(meio.size() == 2);
  CHECK(meio[0].id != meio[1].id);
  CHECK(meio[0].porcentagem == 37);
  CHECK(meio[1].porcentagem == 82);
  CHECK(meio[0].estado == nu::EstadoDaBaixa::Baixando);
  cancella.abre();
  estaleiro.espera_a_fila();

TEST_CASE("o estaleiro não corre mais obras ao mesmo tempo que o limite") {
  Cancella cancella;
  // Cinco encommendas, dous obreiros. As duas primeiras chegam á cancella e param
  // lá; as tres outras ficam na espera. É nesse instante que se olha.
  nu::Estaleiro estaleiro(2, [&cancella](const nu::Pedido&,
                                         std::filesystem::path*) {
    cancella.chego();
    cancella.espera();
    return nu::Colheita::Colhido;
  });
  for (int i = 0; i < 5; ++i) {
    nu::Pedido pedido;
    pedido.url = "https://exemplo/" + std::to_string(i);
    estaleiro.encommenda(pedido);
  }
  cancella.chegaram(2);
  const nu::Andamento parado = estaleiro.andamento();
  CHECK(parado.em_curso == 2);
  CHECK(parado.na_espera == 3);
  CHECK(parado.colhidas == 0);
  cancella.abre();
  estaleiro.fecha();
  // O PICO é a prova do limite: em toda a vida do estaleiro, dous foi o maximo.
  CHECK(estaleiro.pico() == 2);
}

TEST_CASE("fechar ABANDONA a espera, e conta sómente o que correu") {
  Cancella cancella;
  nu::Estaleiro estaleiro(1, [&cancella](const nu::Pedido&,
                                         std::filesystem::path*) {
    cancella.chego();
    cancella.espera();
    return nu::Colheita::Colhido;
  });
  for (int i = 0; i < 4; ++i) estaleiro.encommenda(nu::Pedido{});
  cancella.chegaram(1);  // um obreiro, e elle está DENTRO da obra, parado
  CHECK(estaleiro.andamento().na_espera == 3);

  // A ORDEM d'estes tres passos é o que faz o caso determinado, e não sorteado. O
  // fechamento corre em fio proprio porque elle junta os obreiros e havia de
  // esperar por uma obra que sómente este fio pode soltar. Espera-se pelo punho
  // fechado(), e sómente então se abre a cancella: donde a limpeza da espera é
  // PROVADAMENTE anterior á obra tornar a olhar a fila.
  std::thread fechador([&estaleiro] { estaleiro.fecha(); });
  while (!estaleiro.fechado()) std::this_thread::yield();
  cancella.abre();
  fechador.join();

  const nu::Andamento fim = estaleiro.andamento();
  CHECK(fim.na_espera == 0);
  CHECK(fim.colhidas == 1);  // UMA, exactamente: as tres da espera abandonaram-se
  CHECK(fim.em_curso == 0);
  // E encommendar depois de fechado não põe cousa alguma na fila.
  estaleiro.encommenda(nu::Pedido{});
  CHECK(estaleiro.andamento().na_espera == 0);
}

TEST_CASE("a bandeira da colheita consome-se, e a falha não a levanta") {
  nu::Estaleiro colhedor(1, [](const nu::Pedido&, std::filesystem::path*) {
    return nu::Colheita::Colhido;
  });
  colhedor.encommenda(nu::Pedido{});
  // espera_a_fila, e não fecha: fecha ABANDONA a espera, e a obra podia nunca ter
  // corrido. Este caso quer o desfecho, e por isso pede o desfecho.
  colhedor.espera_a_fila();
  CHECK(colhedor.colheu());
  // A SEGUNDA leitura é falsa: a bandeira consumiu-se. Sem o consumo, a tela
  // veria «ha faixa nova» a cada quadro e varreria o disco para sempre.
  CHECK_FALSE(colhedor.colheu());

  nu::Estaleiro falhador(1, [](const nu::Pedido&, std::filesystem::path*) {
    return nu::Colheita::UrlRecusada;
  });
  falhador.encommenda(nu::Pedido{});
  falhador.espera_a_fila();
  CHECK_FALSE(falhador.colheu());
  const nu::Andamento fim = falhador.andamento();
  CHECK(fim.falhadas == 1);
  CHECK(fim.colhidas == 0);
  CHECK(fim.ultima == "o yt-dlp não leu essa URL");
}

TEST_CASE("estaleiro quieto e sem historia não tem recado") {
  CHECK(nu::texto_do_andamento(nu::Andamento{}).empty());
  // E a razão sósinha tambem não é recado: «(baixado)» na linha da trilha, sem
  // contador algum a acompanhá-la, não diz de quê.
  nu::Andamento orfa;
  orfa.ultima = "baixado";
  CHECK(nu::texto_do_andamento(orfa).empty());
}

TEST_CASE("o recado diz sómente o que não é zero, e concorda o plural") {
  nu::Andamento andamento;
  andamento.em_curso = 2;
  andamento.na_espera = 3;
  CHECK(nu::texto_do_andamento(andamento) == "2 a baixar, 3 na espera");

  nu::Andamento uma;
  uma.colhidas = 1;
  uma.ultima = "baixado";
  CHECK(nu::texto_do_andamento(uma) == "1 colhida (baixado)");

  nu::Andamento duas;
  duas.colhidas = 2;
  duas.falhadas = 1;
  CHECK(nu::texto_do_andamento(duas) == "2 colhidas, 1 falhada");
}

TEST_CASE("a duvidosa conta á parte da falhada") {
  nu::Estaleiro estaleiro(1, [](const nu::Pedido&, std::filesystem::path*) {
    return nu::Colheita::Duvidosa;
  });
  estaleiro.encommenda(nu::Pedido{});
  estaleiro.espera_a_fila();
  const nu::Andamento fim = estaleiro.andamento();
  // NÃO é falha, e não levanta a bandeira da colheita: faixa que não casou pede
  // olho humano, e dizer «falhou» faria o operador tentar outra vez o mesmo.
  CHECK(fim.duvidosas == 1);
  CHECK(fim.falhadas == 0);
  CHECK(fim.colhidas == 0);
  CHECK_FALSE(estaleiro.colheu());
  // E o recado diz-a pelo nome, que é o que a tarefa pede quando manda que a faixa
  // duvidosa não se confunda com a faixa boa.
  CHECK(nu::texto_do_andamento(fim).find("duvidosa") != std::string::npos);
  CHECK(nu::texto_do_andamento(fim).find("falhada") == std::string::npos);
}

TEST_CASE("o colhido duvidoso conta por duvidosa e levanta a bandeira") {
  // O desfecho novo da issue #57: baixou pelo criterio de hoje, sem gravação
  // casada. As duas verdades aferem-se juntas: conta em DUVIDOSAS, que o que
  // ella pede é olho humano e «falhada» faria o operador tentar o mesmo outra
  // vez; e levanta a bandeira da colheita, que o arquivo FICOU no disco e a
  // tela ha de o varrer. Nenhum outro desfecho diz as duas cousas de uma vez.
  nu::Estaleiro estaleiro(1, [](const nu::Pedido&, std::filesystem::path*) {
    return nu::Colheita::ColhidoDuvidoso;
  });
  estaleiro.encommenda(nu::Pedido{});
  estaleiro.espera_a_fila();
  CHECK(estaleiro.colheu());
  const nu::Andamento fim = estaleiro.andamento();
  CHECK(fim.duvidosas == 1);
  CHECK(fim.colhidas == 0);
  CHECK(fim.falhadas == 0);
  CHECK(fim.ultima == "baixado por titulo, sem a gravação: confira");
}

//   Da lavra do eminente Doutor BRAGA US., Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
