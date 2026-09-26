// ══════════════════════════════════════════════════════════════════════════
//   PROVA DA AQUISIÇÃO, testes/prova_aquisicao.cpp
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

TEST_CASE("progresso do yt-dlp aceita linhas parciais e desconhecidas") {
  CHECK(nu::progresso_do_yt_dlp("[download]  12.5% of 4MiB") == 12);
  CHECK(nu::progresso_do_yt_dlp("[download] 100% of 4MiB") == 100);
  CHECK_FALSE(nu::progresso_do_yt_dlp("[download] Destination: faixa.mp3"));
  CHECK_FALSE(nu::progresso_do_yt_dlp("[download] 12."));
  CHECK_FALSE(nu::progresso_do_yt_dlp("[download] 101%"));
  CHECK_FALSE(nu::progresso_do_yt_dlp("ERROR: rede indisponível"));
}

TEST_CASE("leitor entrega linhas de stdout e erro sem bloquear o chamador") {
  std::vector<std::string> linhas;
  const int codigo = nu::corre(
      {"sh", "-c", "printf '[download] 4.2%%\\r[download] 8%%\\n' ; printf 'ERROR: falha\\n' >&2"},
      nullptr, [&linhas](std::string_view linha) { linhas.emplace_back(linha); },
      true);
  CHECK(codigo == 0);
  REQUIRE(linhas.size() == 3);
  CHECK(nu::progresso_do_yt_dlp(linhas[0]) == 4);
  CHECK(nu::progresso_do_yt_dlp(linhas[1]) == 8);
  CHECK(linhas[2] == "ERROR: falha");
}

namespace {

// um_achado, o achado com os tres campos de que os casos dos eleitores vivem:
// titulo, duração e URL. Existe por causa da juncção das duas tarefas irmãs: o
// Achado ganhou os campos da musica (issue #56) e o id do track (issue #57), e a
// inicialização por lista deixava-os sem menção, que o compilador accusa com
// -Wmissing-field-initializers. Nomear campo a campo mantem a bateria calada.
nu::Achado um_achado(std::string titulo, int duracao, std::string url) {
  nu::Achado achado;
  achado.titulo = std::move(titulo);
  achado.canal = "canal";
  achado.duracao = duracao;
  achado.url = std::move(url);
  return achado;
}

}  // namespace

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

// resolve, o operador GANHA sempre, e o canal é o ultimo recurso.
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
TEST_CASE("o destino sahe na hierarchia Artistas/Artista/Musicas/NN - Titulo") {
  nu::Pedido cheio;
  cheio.artista = "Ada Lovelace";
  cheio.album = "Máquina Analítica";
  cheio.titulo = "Tear";
  cheio.numero = 3;
  CHECK(nu::destino("/acervo", cheio).string() ==
        "/acervo/Artistas/Ada Lovelace/Musicas/03 - Tear");
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

TEST_CASE("playlist é reconhecida e enumerada sem baixar") {
  CHECK(nu::eh_playlist_url(
      "https://www.youtube.com/playlist?list=PL123"));
  CHECK(nu::eh_playlist_url(
      "https://music.youtube.com/watch?v=abc&list=PL123"));
  CHECK(nu::eh_playlist_url(
      "https://open.spotify.com/playlist/abc123"));
  CHECK_FALSE(nu::eh_playlist_url("https://youtu.be/abc"));

  const std::vector<std::string> ditos = nu::argumentos_da_playlist(
      "https://www.youtube.com/playlist?list=PL123");
  const auto tem = [&ditos](const std::string& procurado) {
    return std::find(ditos.begin(), ditos.end(), procurado) != ditos.end();
  };
  CHECK(tem("--flat-playlist"));
  CHECK(tem("--skip-download"));
  CHECK_FALSE(tem("--no-playlist"));
  CHECK(ditos.back() == "https://www.youtube.com/playlist?list=PL123");
}

TEST_CASE("linhas da playlist conservam a ordem e ignoram NA") {
  const std::vector<std::string> urls = nu::le_urls_da_playlist(
      "https://youtu.be/primeira\nNA\n\nhttps://youtu.be/terceira\n");
  REQUIRE(urls.size() == 2);
  CHECK(urls[0] == "https://youtu.be/primeira");
  CHECK(urls[1] == "https://youtu.be/terceira");
}

// A CAPA (issue #81). Sem estas duas bandeiras o yt-dlp não colhe miniatura alguma,
// e o painel NOW PLAYING fica com o marcador para toda faixa que a Casa baixe. O
// caso afere a LISTA, e por isso corre sem rede e sem yt-dlp installado.
TEST_CASE("os argumentos do download pedem a miniatura convertida a jpeg") {
  const std::vector<std::string> ditos =
      nu::argumentos_do_download("https://exemplo/x", "/acervo/A/B/01 - T");
  const auto tem = [&ditos](const std::string& q) {
    return std::find(ditos.begin(), ditos.end(), q) != ditos.end();
  };
  CHECK(tem("--embed-thumbnail"));
  // O `jpg` tem de vir COLLADO ao `--convert-thumbnails`: solto, elle seria tomado
  // por URL, e o yt-dlp recusaria a chamada inteira.
  const auto onde = std::find(ditos.begin(), ditos.end(), "--convert-thumbnails");
  REQUIRE(onde != ditos.end());
  REQUIRE(onde + 1 != ditos.end());
  CHECK(*(onde + 1) == "jpg");
  // E o texto da etiqueta continua a ser cousa nossa, que é o que a #34 mediu.
  CHECK(tem("--no-embed-metadata"));
}

TEST_CASE("a busca pede o pseudo-endereco do yt-dlp, e apara o quanto") {
  const std::vector<std::string> ditos =
      nu::argumentos_da_busca("bach", 5, nu::Fonte::YouTube);
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
  CHECK(nu::argumentos_da_busca("x", 0, nu::Fonte::YouTube).back() ==
        "ytsearch1:x");
  CHECK(nu::argumentos_da_busca("x", -3, nu::Fonte::YouTube).back() ==
        "ytsearch1:x");
  CHECK(nu::argumentos_da_busca("x", 100, nu::Fonte::YouTube).back() ==
        "ytsearch20:x");
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

TEST_CASE("a tolerancia de doze segundos declara-se n'um logar só") {
  // A issue #63: os doze moravam em duas constantes, e duas verdades dão a que
  // se corrige e a que fica a errar. A fonte é a do musicbrainz.hpp, e esta
  // deriva d'ella; o `grep` do numero acha UMA declaração, e não duas.
  CHECK(nu::kToleranciaSeg == 12);
  CHECK(nu::kJanellaMs == 12000);
  CHECK(nu::TOLERANCIA_DO_CASAMENTO == nu::kToleranciaSeg);
}

TEST_CASE("o anno absurdo da rede não estoura as contas") {
  // A issue #63: o campo do anno vem de linha do yt-dlp, e a leitura antiga era
  // comportamento INDEFINIDO em transbordo. Vinte digitos são o caso que ella
  // errava; agora dão zero, que é «não se soube», e a conta segue.
  const std::vector<nu::Achado> vasto = nu::le_achados(
      "T\nC\n100\nhttps://y/1\nNA\nNA\nNA\n99999999999999999999\n");
  REQUIRE(vasto.size() == 1);
  CHECK(vasto[0].ano == 0);
  // O que passa do tecto de um dia tambem cae, e o anno de quatro digitos passa.
  const std::vector<nu::Achado> alto = nu::le_achados(
      "T\nC\n999999\nhttps://y/2\nNA\nNA\nNA\n1987\n");
  REQUIRE(alto.size() == 1);
  CHECK(alto[0].duracao == 0);
  CHECK(alto[0].ano == 1987);
  // Anno negativo e anno com letras cahem no crivo dos digitos, como sempre.
  const std::vector<nu::Achado> torto = nu::le_achados(
      "T\nC\n100\nhttps://y/3\nNA\nNA\nNA\n-1987\n"
      "T\nC\n100\nhttps://y/4\nNA\nNA\nNA\n19a7\n");
  REQUIRE(torto.size() == 2);
  CHECK(torto[0].ano == 0);
  CHECK(torto[1].ano == 0);
  // E a sonda da URL lê pela MESMA conta: vinte digitos na duração dão zero.
  const nu::EtiquetaRemota remota =
      nu::le_etiqueta_remota("T\nC\nA\nB\n7\n99999999999999999999\n");
  CHECK(remota.numero == 7);
  CHECK(remota.duracao == 0);
}

// ── AS BANDEIRAS DO MOTOR (issue #55) ───────────────────────────────────────
// Estas provas existem porque a falta d'estas bandeiras deixou a Casa sem colher
// uma unica faixa. A prova afere as QUATRO chamadas juntas (a busca da musica
// entrou com a issue #56): quem tirar a bandeira de bandeiras_do_motor derruba as
// quatro, e é assim que se sabe que a guarda vive.

namespace {

// tem, a bandeira e o seu valor, na ordem, e não a bandeira solta em qualquer
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

TEST_CASE("as quatro chamadas ao yt-dlp carregam o motor de JS") {
  const std::vector<std::vector<std::string>> quatro = {
      nu::argumentos_da_sonda("https://exemplo/x"),
      nu::argumentos_do_download("https://exemplo/x", "/acervo/A/B/01 - T"),
      nu::argumentos_da_busca("bach", 5, nu::Fonte::YouTube),
      nu::argumentos_da_busca("bach", 5, nu::Fonte::YouTubeMusic)};
  for (const std::vector<std::string>& ditos : quatro) {
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
  const std::vector<std::vector<std::string>> quatro = {
      nu::argumentos_da_sonda("https://exemplo/x"),
      nu::argumentos_do_download("https://exemplo/x", "/acervo/A/B/01 - T"),
      nu::argumentos_da_busca("bach", 5, nu::Fonte::YouTube),
      nu::argumentos_da_busca("bach", 5, nu::Fonte::YouTubeMusic)};
  for (const std::vector<std::string>& ditos : quatro) {
    CHECK_FALSE(menciona(ditos, "--cookies-from-browser"));
    CHECK_FALSE(menciona(ditos, nu::kNavegadorDoCookie));
  }
}

TEST_CASE("o cookie entra nas quatro quando se pede") {
  const std::vector<std::vector<std::string>> quatro = {
      nu::argumentos_da_sonda("https://exemplo/x", true),
      nu::argumentos_do_download("https://exemplo/x", "/acervo/A/B/01 - T", true),
      nu::argumentos_da_busca("bach", 5, nu::Fonte::YouTube, true),
      nu::argumentos_da_busca("bach", 5, nu::Fonte::YouTubeMusic, true)};
  for (const std::vector<std::string>& ditos : quatro)
    CHECK(tem(ditos, "--cookies-from-browser", nu::kNavegadorDoCookie));
}

TEST_CASE("bandeiras_do_motor cresce de dous pares para tres com o cookie") {
  // O tamanho afere-se porque uma implementação que devolvesse o cookie SEMPRE
  // passaria nas provas de presença e falharia aqui.
  CHECK(nu::bandeiras_do_motor(false).size() == 4u);
  CHECK(nu::bandeiras_do_motor(true).size() == 6u);
}

TEST_CASE("achado vindo da rede tras SEMPRE URL, que é o que segura o ISRC") {
  // Esta é a guarda de que o caminho do ISRC vive, e por isso tem caso proprio.
  // O eleitor d'esse caminho não criva (RULINGS R3): havendo lista, elle elege
  // alguem. O que impede a eleição de virar baixa errada é este passo: achado
  // sem URL NÃO sahe da leitura, donde todo achado elegivel tras endereço, a
  // recursão de `baixa` cae no ramo COM URL, e pedido sem URL nunca nasce de
  // rede. Se este descarte cahisse, a recursão tornaria ao laço da busca com o
  // mesmo pedido, e o laço não teria fim.
  const std::vector<nu::Achado> lidos = nu::le_achados(
      "Sem endereço\nCanal\n213\n\nNA\nNA\nNA\nNA\n"
      "Com endereço\nCanal\n214\nhttps://youtube/boa\nNA\nNA\nNA\nNA\n");
  REQUIRE(lidos.size() == 1);
  CHECK(lidos[0].titulo == "Com endereço");
  for (const nu::Achado& achado : lidos) CHECK_FALSE(achado.url.empty());
  // E o eleitor sem crivo elege este, que tras endereço: a baixa que se segue
  // tem para onde ir.
  const int qual = nu::achado_mais_proximo(lidos, 213);
  REQUIRE(qual == 0);
  CHECK_FALSE(nu::encommenda_do_achado(lidos[qual]).url.empty());
}

TEST_CASE("o ISRC ganha do titulo quando os dous discordam (aceite da issue)") {
  // A MESMA lista, dous eleitores, lado a lado. O cover tras o titulo do pedido
  // no seu e está a nove segundos do alvo; a art track da gravação tem titulo
  // estranho e está a um. O eleitor de hoje pesa o titulo e leva o COVER; o do
  // caminho ISRC ignora o titulo, desempata pela duração exacta e leva a
  // GRAVAÇÃO: é o aceite da issue #57, aferido com os dous no mesmo palco.
  const std::vector<nu::Achado> achados = {
      um_achado("Never Gonna Give You Up (cover)", 222, "https://youtube/cover"),
      um_achado("NGGYU (2022 Remaster)", 214, "https://youtube/gravacao"),
  };
  nu::Pedido pedido;
  pedido.titulo = "Never Gonna Give You Up";
  pedido.duracao = 213;
  CHECK(nu::melhor_achado(achados, pedido, nu::TOLERANCIA_DO_CASAMENTO) == 0);
  CHECK(nu::achado_mais_proximo(achados, 213) == 1);
}

TEST_CASE("sem alvo de duração, o caminho ISRC fica com o primeiro achado") {
  const std::vector<nu::Achado> achados = {
      um_achado("sem duração dita", 0, "https://youtube/a"),
      um_achado("com duração dita", 214, "https://youtube/b"),
  };
  CHECK(nu::achado_mais_proximo(achados, 0) == 0);    // sem alvo: o primeiro
  CHECK(nu::achado_mais_proximo(achados, 213) == 1);  // «não disse» não desempata
  CHECK(nu::achado_mais_proximo({}, 213) == -1);      // sem achado não ha indice
}

TEST_CASE("o enriquecimento põe o canonico por cima do remendo, e só elle") {
  nu::Pedido pedido;
  pedido.artista = "Rick Astley";
  pedido.titulo = "Never Gonna Give You Up";
  pedido.album = "Minha Lista";  // o remendo de hoje: o nome da lista
  pedido.numero = 7;             // e a posição n'ella
  nu::FichaMB ficha;
  ficha.album = "Whenever You Need Somebody";
  ficha.ano = 1987;
  ficha.numero = 1;
  ficha.titulo = "titulo da ficha, que NÃO entra";
  ficha.artista = "artista da ficha, que NÃO entra";
  const nu::Pedido feito = nu::enriquece(pedido, ficha);
  CHECK(feito.album == "Whenever You Need Somebody");
  CHECK(feito.ano == 1987);
  CHECK(feito.numero == 1);
  CHECK(feito.artista == "Rick Astley");
  CHECK(feito.titulo == "Never Gonna Give You Up");
  // Ficha vazia (o MusicBrainz não casou): o pedido fica tal e qual, remendos e
  // tudo, que é o caminho de hoje seguindo inteiro.
  const nu::Pedido intacto = nu::enriquece(pedido, nu::FichaMB{});
  CHECK(intacto.album == "Minha Lista");
  CHECK(intacto.ano == 0);
  CHECK(intacto.numero == 7);
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

TEST_CASE("o termo codifica-se por cento, e o cardinal não corta a consulta") {
  CHECK(nu::codifica_para_url("radiohead karma police") ==
        "radiohead%20karma%20police");
  // O `#` cru principiaria o fragmento no meio do termo; o `&` partiria a
  // consulta; o `+` viraria espaço na leitura do servidor.
  CHECK(nu::codifica_para_url("a#b&c+d/e?f") == "a%23b%26c%2Bd%2Fe%3Ff");
  CHECK(nu::codifica_para_url("A-z.0_9~") == "A-z.0_9~");
  // UTF-8 vae byte a byte: o «é» são dous.
  CHECK(nu::codifica_para_url("café") == "caf%C3%A9");
  CHECK(nu::codifica_para_url("").empty());
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

TEST_CASE("a busca da musica vae por URL codificada, sem flat e com recorte") {
  const std::vector<std::string> ditos =
      nu::argumentos_da_busca("karma police", 5, nu::Fonte::YouTubeMusic);
  // O alvo escripto á mão: termo codificado por cento, e o fragmento #songs no
  // fim, que é o que segura a prateleira só nas musicas.
  CHECK(ditos.back() ==
        "https://music.youtube.com/search?q=karma%20police#songs");
  CHECK(ditos[ditos.size() - 2] == "--");
  const auto tem = [&ditos](const std::string& q) {
    return std::find(ditos.begin(), ditos.end(), q) != ditos.end();
  };
  // SEM o flat, que com elle os campos da musica vêm NA (medido); o custo
  // paga-se com o recorte da playlist.
  CHECK_FALSE(tem("--flat-playlist"));
  CHECK(tem("--playlist-items"));
  CHECK(tem("1:5"));
  // Os mesmos oito campos da comum: o contracto de le_achados é UM.
  CHECK(std::count(ditos.begin(), ditos.end(), std::string("--print")) == 8);
  // O TECTO proprio da musica: cem pedidos aparam-se em DEZ, e não nos vinte da
  // comum. O dez vae escripto á mão, que é o numero que o custo medido fixou.
  const std::vector<std::string> cem =
      nu::argumentos_da_busca("x", 100, nu::Fonte::YouTubeMusic);
  CHECK(std::find(cem.begin(), cem.end(), std::string("1:10")) != cem.end());
}

TEST_CASE("a fonte Spotify busca no YouTube, argumento por argumento") {
  // O audio do catalogo vem do YouTube (fronteira da issue #13): pedido sem URL
  // com fonte Spotify ha de correr a MESMA busca de hoje, byte a byte.
  CHECK(nu::argumentos_da_busca("bach", 5, nu::Fonte::Spotify) ==
        nu::argumentos_da_busca("bach", 5, nu::Fonte::YouTube));
}

TEST_CASE("os oito --print da busca sahem na ordem que le_achados lê") {
  // A ORDEM é o contracto com le_achados: trocada, o artista pousava por album
  // em toda colheita real, e a contagem de oito seguia verde. O alvo vae
  // escripto á mão, e afere-se nas DUAS fontes que correm rede (a Spotify é
  // byte-egual á YouTube, e o caso acima a prende).
  const std::vector<std::string> alvo = {
      "%(title)s",  "%(uploader)s", "%(duration)s", "%(webpage_url)s",
      "%(artist)s", "%(album)s",    "%(track)s",    "%(release_year)s"};
  for (const nu::Fonte fonte : {nu::Fonte::YouTube, nu::Fonte::YouTubeMusic}) {
    const std::vector<std::string> ditos =
        nu::argumentos_da_busca("bach", 5, fonte);
    std::vector<std::string> moldes;
    for (std::size_t i = 0; i + 1 < ditos.size(); ++i)
      if (ditos[i] == "--print") moldes.push_back(ditos[i + 1]);
    CHECK(moldes == alvo);
  }
}

TEST_CASE("do achado da musica nasce a encommenda inteira, campo a campo") {
  nu::Achado musica;
  musica.titulo = "Karma Police (Remastered)";
  musica.canal = "Radiohead";
  musica.duracao = 264;
  musica.url = "https://y/4";
  musica.artista = "Radiohead";
  musica.album = "OK Computer";
  musica.faixa = "Karma Police";
  musica.ano = 1997;
  musica.fonte = nu::Fonte::YouTubeMusic;
  const nu::Pedido pedido = nu::encommenda_do_achado(musica);
  CHECK(pedido.url == "https://y/4");
  CHECK(pedido.artista == "Radiohead");
  CHECK(pedido.album == "OK Computer");
  // O titulo que vae é o CANONICO, e não o do video com o Remastered no meio.
  CHECK(pedido.titulo == "Karma Police");
  CHECK(pedido.ano == 1997);
  CHECK(pedido.fonte == nu::Fonte::YouTubeMusic);
  // Com URL a duração fica de fóra: ella só criva o casamento do pedido sem URL.
  CHECK(pedido.duracao == 0);
  CHECK(pedido.numero == 0);
}

TEST_CASE("do achado comum nasce o pedido de hoje: URL, fonte, e mais nada") {
  nu::Achado comum;
  comum.titulo = "Bach | Toccata e Fuga";
  comum.canal = "Canal do Orgao";
  comum.duracao = 542;
  comum.url = "https://y/1";
  const nu::Pedido cru = nu::encommenda_do_achado(comum);
  CHECK(cru.url == "https://y/1");
  CHECK(cru.artista.empty());
  CHECK(cru.album.empty());
  CHECK(cru.titulo.empty());  // o titulo fica com a sonda, via resolve
  CHECK(cru.ano == 0);
  CHECK(cru.duracao == 0);
  CHECK(cru.numero == 0);
  CHECK(cru.fonte == nu::Fonte::YouTube);
}

TEST_CASE("do catalogo nascem achados: filtro sem caixa, e a lista por album") {
  nu::Catalogo catalogo;
  catalogo.nome = "Minha Lista";
  // O ultimo campo é o id do track (issue #57), vazio aqui: estes casos são da
  // fonte da busca, e o id prova-se no caso proprio d'elle.
  catalogo.faixas = {
      {"Karma Police", "Radiohead", 1, 264500, ""},
      {"Paranoid Android", "Radiohead", 2, 386000, ""},
      {"Ageispolis", "Aphex Twin", 3, 322499, ""},
  };
  // Por TITULO, sem caixa, e com todo campo no seu logar.
  const std::vector<nu::Achado> uns = nu::achados_do_catalogo(catalogo, "karma");
  REQUIRE(uns.size() == 1);
  CHECK(uns[0].titulo == "Karma Police");
  CHECK(uns[0].faixa == "Karma Police");
  CHECK(uns[0].artista == "Radiohead");
  CHECK(uns[0].album == "Minha Lista");  // o album é o NOME da lista
  CHECK(uns[0].numero == 1);             // a posição vira o «NN - » do nome
  CHECK(uns[0].duracao == 265);          // 264500 ms arredondam para cima
  CHECK(uns[0].fonte == nu::Fonte::Spotify);
  CHECK(uns[0].url.empty());  // sem URL: a baixa busca por si e casa pela duração
  // Por ARTISTA, que com musica se busca tanto um como o outro.
  CHECK(nu::achados_do_catalogo(catalogo, "RADIOHEAD").size() == 2);
  // O meio que não chega á metade arredonda para baixo.
  CHECK(nu::achados_do_catalogo(catalogo, "ageis")[0].duracao == 322);
  // Termo que nada casa dá lista vazia, que é resposta e não erro.
  CHECK(nu::achados_do_catalogo(catalogo, "bach").empty());
  // E termo vazio dá o catalogo inteiro: é o que a troca de fonte mostra.
  CHECK(nu::achados_do_catalogo(catalogo, "").size() == 3);
}

TEST_CASE("o id do track atravessa o achado do catalogo até a encommenda") {
  // A COSTURA das duas tarefas: a fonte Spotify da tela (issue #56) faz o
  // achado no catalogo e a encommenda sahe d'elle, e não do encommenda_do
  // _catalogo da secção Lista. Sem o id atravessar aqui, a faixa perdia o
  // casamento pelo LINK e cahia na busca por titulo dentro do MusicBrainz.
  nu::Catalogo catalogo;
  catalogo.nome = "Minha Lista";
  nu::FaixaDoCatalogo faixa;
  faixa.titulo = "Never Gonna Give You Up";
  faixa.artista = "Rick Astley";
  faixa.numero = 1;
  faixa.duracao_ms = 213000;
  faixa.id_do_track = "1Ojc3QD0dfJ5HG8uzLsfTg";
  catalogo.faixas = {faixa};
  const std::vector<nu::Achado> achados =
      nu::achados_do_catalogo(catalogo, "never");
  REQUIRE(achados.size() == 1);
  CHECK(achados[0].id_spotify == "1Ojc3QD0dfJ5HG8uzLsfTg");
  CHECK(nu::encommenda_do_achado(achados[0]).id_spotify ==
        "1Ojc3QD0dfJ5HG8uzLsfTg");
  // Achado de busca na rede não tras id, e ahi o pedido sahe sem elle: é o
  // caminho segundo do MusicBrainz, e não engano.
  CHECK(nu::encommenda_do_achado(nu::Achado{}).id_spotify.empty());
}

TEST_CASE("os campos da musica nascem vazios, que vazio é «não sei»") {
  const nu::Achado nada;
  CHECK(nada.id_spotify.empty());  // o id do track (issue #57) tambem
  CHECK(nada.artista.empty());
  CHECK(nada.album.empty());
  CHECK(nada.faixa.empty());
  CHECK(nada.ano == 0);
  CHECK(nada.numero == 0);
  CHECK(nada.fonte == nu::Fonte::YouTube);
  CHECK(nu::Pedido{}.ano == 0);
}

//   Da lavra do eminente Doutor BRAGA US., Braga Us ✒
// ══════════════════════════════════════════════════════════════════════════
