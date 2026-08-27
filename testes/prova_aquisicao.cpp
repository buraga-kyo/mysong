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

TEST_CASE("os argumentos do download não embutem etiqueta, e não sobrescrevem") {
  const std::vector<std::string> ditos =
      nu::argumentos_do_download("https://exemplo/x", "/acervo/A/B/01 - T");
  const auto tem = [&ditos](const std::string& q) {
    return std::find(ditos.begin(), ditos.end(), q) != ditos.end();
  };
  CHECK(tem("--no-overwrites"));
  CHECK(tem("--extract-audio"));
  CHECK(tem("--no-embed-metadata"));
  CHECK_FALSE(tem("--embed-metadata"));
  CHECK(tem("--no-playlist"));
  CHECK(ditos.back() == "https://exemplo/x");
  CHECK(ditos[ditos.size() - 2] == "--");
  // O molde leva a extensão do yt-dlp, e não uma que nós adivinhemos.
  CHECK(tem("/acervo/A/B/01 - T.%(ext)s"));
}

TEST_CASE("a busca pede o pseudo-endereco do yt-dlp, e apara o quanto") {
  const std::vector<std::string> ditos = nu::argumentos_da_busca("bach", 5);
  CHECK(ditos.back() == "ytsearch5:bach");
  CHECK(ditos[ditos.size() - 2] == "--");
  const auto tem = [&ditos](const std::string& q) {
    return std::find(ditos.begin(), ditos.end(), q) != ditos.end();
  };
  // Sem esta bandeira, buscar dez custa dez sondas de rede: o yt-dlp abriria
  // cada resultado para lhe ler os formatos.
  CHECK(tem("--flat-playlist"));
  // Oito campos, e nem um a mais: a ordem d'elles é o contracto de le_achados.
  CHECK(std::count(ditos.begin(), ditos.end(), std::string("--print")) == 8);
  // A aparadura pelas duas pontas. Zero não é pedido, e cem não cabe na tabella.
  CHECK(nu::argumentos_da_busca("x", 0).back() == "ytsearch1:x");
  CHECK(nu::argumentos_da_busca("x", -3).back() == "ytsearch1:x");
  CHECK(nu::argumentos_da_busca("x", 100).back() == "ytsearch20:x");
}

TEST_CASE("os achados lêem-se por LINHA, e titulo com barra não os parte") {
  // A barra vertical no titulo é o caso que um separador dentro da linha erraria.
  const std::string sahida =
      "Bach | Toccata e Fuga\nCanal do Orgao\n542\nhttps://y/1\nNA\nNA\nNA\nNA\n"
      "Bach\tPartita\nOutro Canal\nNA\nhttps://y/2\nNA\nNA\nNA\nNA\n";
  const std::vector<nu::Achado> achados = nu::le_achados(sahida);
  REQUIRE(achados.size() == 2);
  CHECK(achados[0].titulo == "Bach | Toccata e Fuga");
  CHECK(achados[0].canal == "Canal do Orgao");
  CHECK(achados[0].duracao == 542);
  CHECK(achados[0].url == "https://y/1");
  CHECK(achados[1].titulo == "Bach\tPartita");
  // O NA da duração é «não disse», e não é duração zero mentida como numero.
  CHECK(achados[1].duracao == 0);
  // E o NA num campo de TEXTO fica vazio, e não fica a palavra NA: mostrar «NA»
  // por canal seria inventar um canal chamado NA.
  const std::vector<nu::Achado> mudo =
      nu::le_achados("Titulo\nNA\n60\nhttps://y/9\nNA\nNA\nNA\nNA\n");
  REQUIRE(mudo.size() == 1);
  CHECK(mudo[0].canal.empty());
  // E a busca comum deixa vazios os campos da musica: NA não é artista.
  CHECK(mudo[0].artista.empty());
  CHECK(mudo[0].album.empty());
  CHECK(mudo[0].faixa.empty());
  CHECK(mudo[0].ano == 0);
}

TEST_CASE("achado meio não se mostra, e duração que não é numero não vira lixo") {
  // Tres linhas: o grupo de oito não fecha, e o achado meio cahe. Se ficasse, o
  // operador elegia-o e a baixa falhava sem URL.
  CHECK(nu::le_achados("Titulo\nCanal\n100\n").empty());
  // Sem URL não ha o que baixar, ainda que o grupo feche.
  CHECK(nu::le_achados("Titulo\nCanal\n100\n\nNA\nNA\nNA\nNA\n").empty());
  // Duração que o yt-dlp escreva por extenso não se lê a metade: fica zero.
  const std::vector<nu::Achado> um =
      nu::le_achados("T\nC\n9m02s\nhttps://y/3\nNA\nNA\nNA\nNA\n");
  REQUIRE(um.size() == 1);
  CHECK(um[0].duracao == 0);
}

// ── AS BANDEIRAS DO MOTOR (issue #55) ───────────────────────────────────────
// Estas provas existem porque a falta d'estas bandeiras deixou a Casa sem colher
// uma unica faixa. A prova afere as TRES chamadas juntas: quem tirar a bandeira de
// bandeiras_do_motor derruba as tres, e é assim que se sabe que a guarda vive.

namespace {

// tem — a bandeira e o seu valor, na ordem, e não a bandeira solta em qualquer
// logar. Bandeira sem valor é bandeira que o yt-dlp recusa.
bool tem(const std::vector<std::string>& ditos, const std::string& bandeira,
         const std::string& valor) {
  for (std::size_t i = 0; i + 1 < ditos.size(); ++i)
    if (ditos[i] == bandeira && ditos[i + 1] == valor) return true;
  return false;
}

bool menciona(const std::vector<std::string>& ditos, const std::string& agulha) {
  for (const std::string& dito : ditos)
    if (dito == agulha) return true;
  return false;
}

}  // namespace

TEST_CASE("as tres chamadas ao yt-dlp carregam o motor de JS") {
  const std::vector<std::vector<std::string>> tres = {
      nu::argumentos_da_sonda("https://exemplo/x"),
      nu::argumentos_do_download("https://exemplo/x", "/acervo/A/B/01 - T"),
      nu::argumentos_da_busca("bach", 5)};
  for (const std::vector<std::string>& ditos : tres) {
    // O nome do programa continua a ser o primeiro: as bandeiras entram DEPOIS
    // d'elle, que ninguem corre `--js-runtimes` como se fosse executavel.
    CHECK(ditos.front() == "yt-dlp");
    CHECK(tem(ditos, "--js-runtimes", nu::kMotorDeJs));
    CHECK(tem(ditos, "--remote-components", nu::kComponenteDoDesafio));
  }
}

TEST_CASE("o cookie NAO entra sem que se peca") {
  // Esta é a prova que importa mais, e a razão é medida: com o cookie do chrome o
  // yt-dlp responde «The page needs to be reloaded.» a toda URL. Ligá-lo por
  // omissão seria trocar uma falha por outra.
  const std::vector<std::vector<std::string>> tres = {
      nu::argumentos_da_sonda("https://exemplo/x"),
      nu::argumentos_do_download("https://exemplo/x", "/acervo/A/B/01 - T"),
      nu::argumentos_da_busca("bach", 5)};
  for (const std::vector<std::string>& ditos : tres) {
    CHECK_FALSE(menciona(ditos, "--cookies-from-browser"));
    CHECK_FALSE(menciona(ditos, nu::kNavegadorDoCookie));
  }
}

TEST_CASE("o cookie entra nas tres quando se pede") {
  const std::vector<std::vector<std::string>> tres = {
      nu::argumentos_da_sonda("https://exemplo/x", true),
      nu::argumentos_do_download("https://exemplo/x", "/acervo/A/B/01 - T", true),
      nu::argumentos_da_busca("bach", 5, true)};
  for (const std::vector<std::string>& ditos : tres)
    CHECK(tem(ditos, "--cookies-from-browser", nu::kNavegadorDoCookie));
}

TEST_CASE("bandeiras_do_motor cresce de dous pares para tres com o cookie") {
  // O tamanho afere-se porque uma implementação que devolvesse o cookie SEMPRE
  // passaria nas provas de presença e falharia aqui.
  CHECK(nu::bandeiras_do_motor(false).size() == 4u);
  CHECK(nu::bandeiras_do_motor(true).size() == 6u);
}

// ── A FONTE DA BUSCA (issue #56) ────────────────────────────────────────────

TEST_CASE("a fonte cicla pelas tres e volta, e o pedido nasce no YouTube") {
  // O ciclo é FECHADO: tres passos devolvem o começo. Quem acrescentar fonte
  // sem a pôr no ciclo deixa a tecla presa n'uma volta que não fecha.
  CHECK(nu::proxima_fonte(nu::Fonte::YouTube) == nu::Fonte::YouTubeMusic);
  CHECK(nu::proxima_fonte(nu::Fonte::YouTubeMusic) == nu::Fonte::Spotify);
  CHECK(nu::proxima_fonte(nu::Fonte::Spotify) == nu::Fonte::YouTube);
  // O nome é o que o cabeçalho mostra; fonte sem nome não ha de existir.
  CHECK(nu::nome_da_fonte(nu::Fonte::YouTube) == "YouTube");
  CHECK(nu::nome_da_fonte(nu::Fonte::YouTubeMusic) == "YouTube Music");
  CHECK(nu::nome_da_fonte(nu::Fonte::Spotify) == "Spotify");
  // O padrão do Pedido é o de hoje: quem não escolheu, busca no YouTube.
  CHECK(nu::Pedido{}.fonte == nu::Fonte::YouTube);
}

TEST_CASE("a art track chega com artista, album, faixa e ano") {
  // A sahida MEDIDA em 2026-08-27 contra o music.youtube.com, oito linhas.
  const std::vector<nu::Achado> uns = nu::le_achados(
      "Karma Police\nRadiohead\n264\nhttps://y/4\n"
      "Radiohead\nOK Computer\nKarma Police\n1997\n");
  REQUIRE(uns.size() == 1);
  CHECK(uns[0].artista == "Radiohead");
  CHECK(uns[0].album == "OK Computer");
  CHECK(uns[0].faixa == "Karma Police");
  CHECK(uns[0].ano == 1997);
  CHECK(uns[0].duracao == 264);
  // Ano por extenso é lixo, e lixo dá zero, e não lança.
  const std::vector<nu::Achado> torto =
      nu::le_achados("T\nC\n10\nhttps://y/5\nNA\nNA\nNA\nMCMXCVII\n");
  REQUIRE(torto.size() == 1);
  CHECK(torto[0].ano == 0);
}

TEST_CASE("os campos da musica nascem vazios, que vazio é «não sei»") {
  const nu::Achado nada;
  CHECK(nada.artista.empty());
  CHECK(nada.album.empty());
  CHECK(nada.faixa.empty());
  CHECK(nada.ano == 0);
  CHECK(nada.numero == 0);
  CHECK(nada.fonte == nu::Fonte::YouTube);
  CHECK(nu::Pedido{}.ano == 0);
}

//   Da lavra do eminente Doutor BRAGA US. — Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
