// ══════════════════════════════════════════════════════════════════════════
//   PROVA DA AQUISIÇÃO — testes/prova_aquisicao.cpp
// ══════════════════════════════════════════════════════════════════════════
// Caso algum d'esta bateria toca a rede. O que se afere é o que se HA DE correr,
// e o que se HA DE gravar: as cinco funcções puras. O `fork` e o `exec` provam-se
// á mão, contra o YouTube, e o PR diz o que se viu.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

#include <algorithm>
#include <string>
#include <vector>

#include "nucleo/aquisicao.hpp"

namespace nu = mysong::nucleo;

TEST_CASE("o saneamento tira o que o kernel proscreve, e mais o que engana") {
  // A barra TROCA-SE, e não se apaga: apagar collaria AC/DC em ACDC.
  CHECK(nu::saneia_nome("AC/DC") == "AC-DC");
  CHECK(nu::saneia_nome("a/b/c") == "a-b-c");
  // Ponto inicial faz arquivo occulto, e `..` subiria um degrau.
  CHECK(nu::saneia_nome(".occulto") == "occulto");
  CHECK(nu::saneia_nome("..") == "sem titulo");
  CHECK(nu::saneia_nome("...tres") == "tres");
  // Espaço das pontas sahe; do meio, fica.
  CHECK(nu::saneia_nome("  Tear  ") == "Tear");
  CHECK(nu::saneia_nome("Nota G") == "Nota G");
  // Nome que se reduz a nada tem resposta, e não erro.
  CHECK(nu::saneia_nome("") == "sem titulo");
  CHECK(nu::saneia_nome("   ") == "sem titulo");
  CHECK(nu::saneia_nome("/") == "-");
  // Acento atravessa intacto: elle é valido no ext4.
  CHECK(nu::saneia_nome("Máquina Analítica") == "Máquina Analítica");
}

TEST_CASE("o corte pelo comprimento recúa até ao byte lider") {
  // Duzentos e quarenta «á», que são quatrocentos e oitenta octetos. O corte ha de
  // deixar cadeia de comprimento valido E de UTF-8 valido: contando-se octetos sem
  // recuar, o ultimo caracter sahiria partido pelo meio.
  const std::string longo(240, 'x');
  CHECK(nu::saneia_nome(longo).size() == 240u);
  const std::string maior(300, 'x');
  CHECK(nu::saneia_nome(maior).size() <= 240u);
  std::string acentuado;
  for (int i = 0; i < 240; ++i) acentuado += "á";  // dous octetos cada
  const std::string cortado = nu::saneia_nome(acentuado);
  CHECK(cortado.size() <= 240u);
  // O corte não parte caracter: sendo cada «á» de dous octetos, o comprimento é
  // PAR, e todo lider tras a sua continuação. (Affirmar que o ULTIMO octeto não é
  // de continuação seria errado, e escrevi-o errado da primeira vez: cadeia UTF-8
  // valida acaba em continuação sempre que o ultimo caracter é multibyte.)
  CHECK(cortado.size() % 2u == 0u);
  CHECK(cortado.size() == 240u);  // cabendo cento e vinte «á», cabem todos
  for (std::size_t i = 0; i < cortado.size(); i += 2) {
    CHECK((static_cast<unsigned char>(cortado[i]) & 0xC0) == 0xC0);
    CHECK((static_cast<unsigned char>(cortado[i + 1]) & 0xC0) == 0x80);
  }
}

// resolve — o operador GANHA sempre, e o canal é o ultimo recurso.
TEST_CASE("o operador ganha da rede, campo a campo") {
  nu::EtiquetaRemota remota;
  remota.titulo = "Titulo Da Rede";
  remota.canal = "Canal Do YouTube";
  remota.artista = "Artista Da Rede";
  remota.album = "Album Da Rede";
  remota.numero = 9;

  nu::Pedido dito;
  dito.titulo = "Tear";
  dito.artista = "Ada Lovelace";
  dito.album = "Máquina";
  dito.numero = 3;
  const nu::Pedido meu = nu::resolve(dito, remota);
  CHECK(meu.titulo == "Tear");
  CHECK(meu.artista == "Ada Lovelace");
  CHECK(meu.album == "Máquina");
  CHECK(meu.numero == 3);

  // Calando o operador, vale a rede; e o artista vem do campo `artist`, e NÃO do
  // canal, que é o ultimo recurso.
  const nu::Pedido seu = nu::resolve({}, remota);
  CHECK(seu.titulo == "Titulo Da Rede");
  CHECK(seu.artista == "Artista Da Rede");
  CHECK(seu.album == "Album Da Rede");
  CHECK(seu.numero == 9);
}

TEST_CASE("faltando os dous, o artista fica Desconhecido e não fica o canal") {
  const nu::Pedido nada = nu::resolve({}, {});
  CHECK(nada.artista == "Desconhecido");
  CHECK(nada.titulo == "sem titulo");
  // Havendo SÓ canal, elle serve, que não ha melhor; mas serve em ultimo logar.
  nu::EtiquetaRemota canal;
  canal.canal = "Public Domain Classical Music";
  CHECK(nu::resolve({}, canal).artista == "Public Domain Classical Music");
}

// O CAMINHO na hierarchia, contra alvo escripto á mão.
TEST_CASE("o destino sahe na hierarchia Artista/Album/NN - Titulo") {
  nu::Pedido cheio;
  cheio.artista = "Ada Lovelace";
  cheio.album = "Máquina Analítica";
  cheio.titulo = "Tear";
  cheio.numero = 3;
  CHECK(nu::destino("/acervo", cheio).string() ==
        "/acervo/Ada Lovelace/Máquina Analítica/03 - Tear");
  // Numero de dous digitos não ganha zero á frente.
  cheio.numero = 12;
  CHECK(nu::destino("/acervo", cheio).string() ==
        "/acervo/Ada Lovelace/Máquina Analítica/12 - Tear");
  // Sem numero, o titulo fica sozinho; sem album, o artista contem a faixa.
  cheio.numero = 0;
  CHECK(nu::destino("/acervo", cheio).string() ==
        "/acervo/Ada Lovelace/Máquina Analítica/Tear");
  cheio.album.clear();
  CHECK(nu::destino("/acervo", cheio).string() == "/acervo/Ada Lovelace/Tear");
  // E o saneamento vale em CADA componente, e não sómente no titulo.
  nu::Pedido torto;
  torto.artista = "AC/DC";
  torto.album = ".occulto";
  torto.titulo = "a/b";
  CHECK(nu::destino("/acervo", torto).string() == "/acervo/AC-DC/occulto/a-b");
  // Extensão alguma se põe: quem a põe é o yt-dlp, que sabe em que fórma sahiu.
  CHECK(nu::destino("/acervo", cheio).extension().empty());
}

// As duas listas de argumentos, e o CONTRACTO entre a sonda e o leitor: a ordem
// dos seis `--print` é a ordem em que le_etiqueta_remota lê as linhas, e os dous
// aferem-se JUNTOS para que uma mudança n'um sem o outro morra aqui.
TEST_CASE("os argumentos da sonda casam com a ordem que o leitor espera") {
  const std::vector<std::string> ditos =
      nu::argumentos_da_sonda("https://exemplo/x");
  CHECK(ditos.front() == "yt-dlp");
  CHECK(ditos.back() == "https://exemplo/x");
  // O `--` antes da URL é load-bearing: sem elle, URL que principie por hyphen
  // seria lida como opção.
  CHECK(ditos[ditos.size() - 2] == "--");
  // Seis `--print`, na ordem: titulo, canal, artista, album, numero, duração.
  std::vector<std::string> moldes;
  for (std::size_t i = 0; i + 1 < ditos.size(); ++i)
    if (ditos[i] == "--print") moldes.push_back(ditos[i + 1]);
  REQUIRE(moldes.size() == 6u);
  CHECK(moldes[0] == "%(title)s");
  CHECK(moldes[1] == "%(uploader)s");
  CHECK(moldes[2] == "%(artist)s");
  CHECK(moldes[3] == "%(album)s");
  CHECK(moldes[4] == "%(track_number)s");
  CHECK(moldes[5] == "%(duration)s");

  // E o leitor lê nessa mesma ordem. Seis linhas escriptas á mão.
  const nu::EtiquetaRemota lida = nu::le_etiqueta_remota(
      "Titulo\nCanal\nArtista\nAlbum\n7\n185\n");
  CHECK(lida.titulo == "Titulo");
  CHECK(lida.canal == "Canal");
  CHECK(lida.artista == "Artista");
  CHECK(lida.album == "Album");
  CHECK(lida.numero == 7);
  CHECK(lida.duracao == 185);
}

TEST_CASE("o NA do yt-dlp é campo ausente, e não titulo") {
  const nu::EtiquetaRemota nas =
      nu::le_etiqueta_remota("Titulo\nCanal\nNA\nNA\nNA\nNA\n");
  CHECK(nas.titulo == "Titulo");
  CHECK(nas.artista.empty());  // e não «NA»
  CHECK(nas.album.empty());
  CHECK(nas.numero == 0);
  CHECK(nas.duracao == 0);
  // Sahida vazia dá etiqueta inteira vazia, e não estoura o indice.
  const nu::EtiquetaRemota nada = nu::le_etiqueta_remota("");
  CHECK(nada.titulo.empty());
  CHECK(nada.numero == 0);
  // Sahida curta: as que faltam ficam vazias, e não trazem lixo da anterior.
  const nu::EtiquetaRemota curta = nu::le_etiqueta_remota("Titulo\nCanal\n");
  CHECK(curta.titulo == "Titulo");
  CHECK(curta.canal == "Canal");
  CHECK(curta.artista.empty());
  CHECK(curta.numero == 0);
  // Numero que não é numero dá zero, e não lança: a rede manda lixo.
  const nu::EtiquetaRemota lixo =
      nu::le_etiqueta_remota("T\nC\nA\nB\nsete\n12x\n");
  CHECK(lixo.numero == 0);
  CHECK(lixo.duracao == 0);
}

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
