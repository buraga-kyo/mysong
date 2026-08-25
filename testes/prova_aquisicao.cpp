// ══════════════════════════════════════════════════════════════════════════
//   PROVA DA AQUISIÇÃO — testes/prova_aquisicao.cpp
// ══════════════════════════════════════════════════════════════════════════
// Caso algum d'esta bateria toca a rede. O que se afere é o que se HA DE correr,
// e o que se HA DE gravar: as cinco funcções puras. O `fork` e o `exec` provam-se
// á mão, contra o YouTube, e o PR diz o que se viu.
// ══════════════════════════════════════════════════════════════════════════
#include <doctest/doctest.h>

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

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
